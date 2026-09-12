#include "common.hpp"
#include "file_manager.hpp"
#include "logger.hpp"
#include "process/injection.hpp"
#include "api/encrypted_downloader.hpp"
#include "api/environment.hpp"
#include "crypto/hwid.hpp"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <Windows.h>
#include <wincrypt.h>
#include <cstdlib>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace
{
	using namespace gottvergessen;

	std::unique_ptr<logger> g_native_logger;
	thread_local std::string g_result;
	std::string g_error;
	std::mutex g_state_mutex;
	std::string g_access_token;
	std::string g_login_expired_date;
	std::filesystem::path g_session_path;
	nlohmann::ordered_json g_binaries = nlohmann::ordered_json::array();
	int g_selected_binary{-1};
	int g_operation_progress{0};
	std::string g_operation_stage{"Idle"};
	bool g_operation_active{false};

	constexpr const char* user_agent = "Gottvergessen-Loader/1.0";

	std::string authorization_value(std::string token)
	{
		const auto first = token.find_first_not_of(" \t\r\n");
		const auto last = token.find_last_not_of(" \t\r\n");
		if (first == std::string::npos)
			return "Bearer ";
		token = token.substr(first, last - first + 1);
		if (token.size() >= 7)
		{
			std::string prefix = token.substr(0, 7);
			for (char& value : prefix)
				value = static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
			if (prefix == "bearer ")
				return "Bearer " + token.substr(7);
		}
		return "Bearer " + token;
	}

	std::string normalize_access_token(std::string token)
	{
		const std::string header = authorization_value(std::move(token));
		return header.size() > 7 ? header.substr(7) : std::string{};
	}

	void set_operation(int progress, std::string stage, bool active = true)
	{
		std::scoped_lock lock(g_state_mutex);
		g_operation_progress = (std::max)(0, (std::min)(100, progress));
		g_operation_stage = std::move(stage);
		g_operation_active = active;
	}

	std::vector<unsigned char> protect_session(const std::string& value)
	{
		DATA_BLOB input{static_cast<DWORD>(value.size()), reinterpret_cast<BYTE*>(const_cast<char*>(value.data()))};
		DATA_BLOB output{};
		if (!CryptProtectData(&input, L"GottvergessenSessionData", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output))
			return {};
		std::vector<unsigned char> encrypted(output.pbData, output.pbData + output.cbData);
		LocalFree(output.pbData);
		return encrypted;
	}

	std::string unprotect_session(const std::vector<unsigned char>& value)
	{
		if (value.empty())
			return {};
		DATA_BLOB input{static_cast<DWORD>(value.size()), const_cast<BYTE*>(value.data())};
		DATA_BLOB output{};
		if (CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output))
		{
			std::string decrypted(reinterpret_cast<char*>(output.pbData), output.cbData);
			LocalFree(output.pbData);
			return decrypted;
		}
		// Compatible with the legacy plaintext session.json fallback.
		return {value.begin(), value.end()};
	}

	std::string refresh_cookie(const cpr::Response& response)
	{
		for (const auto& [key, value] : response.header)
		{
			if (key != "Set-Cookie" && key != "set-cookie")
				continue;
			constexpr std::string_view prefix = "refresh_token=";
			const auto position = value.find(prefix);
			if (position == std::string::npos)
				continue;
			const auto start = position + prefix.size();
			const auto end = value.find(';', start);
			return value.substr(start, end == std::string::npos ? std::string::npos : end - start);
		}
		return {};
	}

	void clear_saved_session()
	{
		std::error_code ignored;
		if (!g_session_path.empty())
			std::filesystem::remove(g_session_path, ignored);
		if (!g_session_path.empty())
			std::filesystem::remove(g_session_path.parent_path() / "session.json", ignored);
	}

	bool save_session(const std::string& refresh_token)
	{
		if (refresh_token.empty() || g_session_path.empty())
			return false;
		std::filesystem::create_directories(g_session_path.parent_path());
		const auto payload = nlohmann::ordered_json{{"refresh_token", refresh_token}}.dump(4);
		const auto encrypted = protect_session(payload);
		if (encrypted.empty())
			return false;
		std::ofstream file(g_session_path, std::ios::binary | std::ios::trunc);
		file.write(reinterpret_cast<const char*>(encrypted.data()), static_cast<std::streamsize>(encrypted.size()));
		if (!file.good())
			return false;
		std::error_code ignored;
		std::filesystem::remove(g_session_path.parent_path() / "session.json", ignored);
		return true;
	}

	void set_error(const std::exception& error)
	{
		std::scoped_lock lock(g_state_mutex);
		g_error = error.what();
	}

	void set_error(std::string message)
	{
		std::scoped_lock lock(g_state_mutex);
		g_error = std::move(message);
	}

	void clear_error()
	{
		std::scoped_lock lock(g_state_mutex);
		g_error.clear();
	}

	std::string response_message(const cpr::Response& response, const nlohmann::ordered_json& body)
	{
		if (body.is_object())
		{
			if (body.contains("message") && body["message"].is_string())
				return body["message"].get<std::string>();
			if (body.contains("error") && body["error"].is_string())
				return body["error"].get<std::string>();
			if (body.contains("data") && body["data"].is_object())
			{
				const auto& data = body["data"];
				if (data.contains("message") && data["message"].is_string())
					return data["message"].get<std::string>();
			}
		}
		if (response.error.code != cpr::ErrorCode::OK && !response.error.message.empty())
			return response.error.message;
		return response.text.empty() ? "empty response" : "response was not valid JSON";
	}

	std::string escape_json(const std::string& input)
	{
		std::string output;
		output.reserve(input.size() + 8);
		for (const char value : input)
		{
			switch (value)
			{
			case '\\': output += "\\\\"; break;
			case '"': output += "\\\""; break;
			case '\n': output += "\\n"; break;
			case '\r': output += "\\r"; break;
			case '\t': output += "\\t"; break;
			default: output += value; break;
			}
		}
		return output;
	}
}

#define GV_API extern "C" __declspec(dllexport)

GV_API bool __cdecl gv_initialize(const wchar_t* base_directory)
{
	try
	{
		std::filesystem::path base_dir;
		if (base_directory && *base_directory)
			base_dir = base_directory;
		else if (const char* appdata = std::getenv("APPDATA"))
			base_dir = std::filesystem::path(appdata) / "Ellohim Menu";
		else
			base_dir = std::filesystem::temp_directory_path() / "Ellohim Menu";

		file_manager::init(base_dir);
		g_session_path = base_dir / "Config" / "session.dat";
		if (!g_native_logger)
		{
			g_native_logger = std::make_unique<logger>("Gottvergessen Electron");
			g_native_logger->enable();
		}
		clear_error();
		return true;
	}
	catch (const std::exception& error)
	{
		set_error(error);
		return false;
	}
}

GV_API void __cdecl gv_shutdown()
{
	try
	{
		g_running = false;

		if (g_native_logger)
		{
			g_native_logger.reset();
		}
	}
	catch (...)
	{
	}
}

GV_API const char* __cdecl gv_list_processes_json()
{
	try
	{
		const auto processes = injection_method::get_running_processes();
		g_result = "[";
		for (size_t index = 0; index < processes.size(); ++index)
		{
			const auto& process = processes[index];
			if (index != 0)
				g_result += ',';
			g_result += "{\"pid\":" + std::to_string(process.pid)
			    + ",\"name\":\"" + escape_json(process.name)
			    + "\",\"arch\":\"" + escape_json(process.arch)
			    + "\",\"accessible\":" + (process.is_accessible ? "true" : "false") + '}';
		}
		g_result += ']';
		clear_error();
		return g_result.c_str();
	}
	catch (const std::exception& error)
	{
		set_error(error);
		return nullptr;
	}
}

GV_API bool __cdecl gv_set_target(const char* process_name, unsigned int pid)
{
	if (!process_name || !*process_name || pid == 0)
	{
		set_error("Invalid target process");
		return false;
	}
	injection::set_target_process(process_name);
	injection::set_target_pid(pid);
	clear_error();
	return true;
}

GV_API bool __cdecl gv_set_injection_mode(int mode)
{
	if (mode < 0 || mode > 3)
	{
		set_error("Injection mode must be between 0 and 3");
		return false;
	}
	injection::set_injection_mode(static_cast<InjectionMode>(mode));
	clear_error();
	return true;
}

GV_API int __cdecl gv_validate_library(const wchar_t* dll_path)
{
	if (!dll_path || !*dll_path)
	{
		set_error("DLL path is required");
		return 0;
	}
	try
	{
		const bool valid = injection::validate_binary(std::filesystem::path(dll_path));
		if (!valid)
			set_error("DLL validation failed");
		else
			clear_error();
		return valid ? 1 : 0;
	}
	catch (const std::exception& error)
	{
		set_error(error);
		return 0;
	}
}

GV_API int __cdecl gv_inject(const wchar_t* dll_path)
{
	if (!dll_path || !*dll_path)
	{
		set_error("DLL path is required");
		return 0;
	}
	try
	{
		const bool success = injection::inject_library(std::filesystem::path(dll_path));
		if (!success)
			set_error("Native injection returned false");
		else
			clear_error();
		return success ? 1 : 0;
	}
	catch (const std::exception& error)
	{
		set_error(error);
		return 0;
	}
}

GV_API const char* __cdecl gv_last_error()
{
	return g_error.c_str();
}

GV_API const char* __cdecl gv_operation_status_json()
{
	std::scoped_lock lock(g_state_mutex);
	g_result = nlohmann::ordered_json{
	    {"progress", g_operation_progress},
	    {"stage", g_operation_stage},
	    {"active", g_operation_active}}
	               .dump();
	return g_result.c_str();
}

GV_API int __cdecl gv_login(const char* username, const char* password, int remember_me)
{
	if (!username || !*username || !password || !*password)
	{
		set_error("Username and password are required");
		return 0;
	}

	try
	{
		std::string hwid = utils::get_hwid();
		std::string device_name = utils::get_device_name();
		nlohmann::ordered_json body_json = {
			{"username", username},
			{"password", password},
			{"hwid", hwid},
			{"device_name", device_name}
		};
		auto response = cpr::Post(
		    cpr::Url{environment_manager::get_url("/auth/login")},
		    cpr::Body{body_json.dump()},
		    cpr::Header{{"Accept", "application/json"}, {"Content-Type", "application/json"}, {"User-Agent", user_agent}});
		auto body = nlohmann::ordered_json::parse(response.text, nullptr, false);
		if (body.is_discarded())
		{
			set_error("Server returned invalid login JSON");
			return 0;
		}

		std::string token;
		if (body.contains("data") && body["data"].is_object())
		{
			const auto& data = body["data"];
			token = data.value("access_token", data.value("accessToken", data.value("token", "")));
		}
		if (token.empty())
			token = body.value("access_token", body.value("accessToken", body.value("token", "")));
		if (response.status_code < 200 || response.status_code >= 300 || token.empty())
		{
			set_error(body.value("message", "Login failed"));
			return 0;
		}
		std::string login_exp;
		if (body.contains("expired_date") && body["expired_date"].is_string())
			login_exp = body["expired_date"].get<std::string>();
		else if (body.contains("expiry_date") && body["expiry_date"].is_string())
			login_exp = body["expiry_date"].get<std::string>();
		else if (body.contains("data") && body["data"].is_object())
		{
			const auto& data = body["data"];
			if (data.contains("expired_date") && data["expired_date"].is_string())
				login_exp = data["expired_date"].get<std::string>();
			else if (data.contains("expiry_date") && data["expiry_date"].is_string())
				login_exp = data["expiry_date"].get<std::string>();
		}

		const std::string refresh = refresh_cookie(response);
		// The current server truncates the 15-minute login lifetime to whole
		// hours, which produces an already-expired access token. Exchange the
		// refresh cookie immediately; /auth/refresh currently issues a 1-hour JWT.
		if (!refresh.empty())
		{
			auto refresh_response = cpr::Post(
			    cpr::Url{environment_manager::get_url("/auth/refresh")},
			    cpr::Header{{"Accept", "application/json"}, {"Content-Type", "application/json"}, {"User-Agent", user_agent}, {"Cookie", "refresh_token=" + refresh}});
			auto refresh_body = nlohmann::ordered_json::parse(refresh_response.text, nullptr, false);
			std::string refreshed_token;
			if (refresh_body.is_object() && refresh_body.contains("data") && refresh_body["data"].is_object())
				refreshed_token = refresh_body["data"].value("token", "");
			if (!refreshed_token.empty() && refresh_response.status_code >= 200 && refresh_response.status_code < 300)
				token = std::move(refreshed_token);
		}
		token = normalize_access_token(std::move(token));

		{
			std::scoped_lock lock(g_state_mutex);
			g_access_token = std::move(token);
			if (!login_exp.empty())
				g_login_expired_date = std::move(login_exp);
			g_error.clear();
		}
		if (remember_me != 0)
		{
			// Keep login successful even when an older server does not issue a
			// refresh cookie; in that case only this app run remains authorized.
			if (refresh.empty() || !save_session(refresh))
				clear_saved_session();
		}
		else
			clear_saved_session();
		return 1;
	}
	catch (const std::exception& error)
	{
		set_error(error);
		return 0;
	}
}

GV_API int __cdecl gv_restore_session()
{
	try
	{
		// 1. Try server-sided device login via HWID first
		std::string hwid = utils::get_hwid();
		std::string device_name = utils::get_device_name();
		nlohmann::ordered_json dev_body = {
			{"hwid", hwid},
			{"device_name", device_name}
		};
		auto dev_res = cpr::Post(
			cpr::Url{environment_manager::get_url("/auth/device-login")},
			cpr::Body{dev_body.dump()},
			cpr::Header{{"Accept", "application/json"}, {"Content-Type", "application/json"}, {"User-Agent", user_agent}});
		if (dev_res.status_code >= 200 && dev_res.status_code < 300)
		{
			auto body = nlohmann::ordered_json::parse(dev_res.text, nullptr, false);
			if (body.is_object() && body.value("success", false) && body.contains("data") && body["data"].is_object())
			{
				std::string token = body["data"].value("token", "");
				std::string dev_exp = body["data"].value("expired_date", body["data"].value("expiry_date", ""));
				if (!token.empty())
				{
					token = normalize_access_token(std::move(token));
					std::scoped_lock lock(g_state_mutex);
					g_access_token = std::move(token);
					if (!dev_exp.empty())
						g_login_expired_date = std::move(dev_exp);
					g_error.clear();
					return 1;
				}
			}
		}

		// 2. Fallback to saved session file
		std::filesystem::path session_path = g_session_path;
		if (!std::filesystem::exists(session_path))
		{
			const auto legacy = session_path.parent_path() / "session.json";
			if (!std::filesystem::exists(legacy))
				return 0;
			session_path = legacy;
		}
		std::ifstream file(session_path, std::ios::binary | std::ios::ate);
		if (!file)
			return 0;
		const auto size = file.tellg();
		if (size <= 0)
		{
			clear_saved_session();
			return 0;
		}
		std::vector<unsigned char> bytes(static_cast<size_t>(size));
		file.seekg(0);
		file.read(reinterpret_cast<char*>(bytes.data()), size);
		auto saved = nlohmann::ordered_json::parse(unprotect_session(bytes), nullptr, false);
		const std::string refresh = saved.is_object() ? saved.value("refresh_token", "") : "";
		if (refresh.empty())
		{
			clear_saved_session();
			return 0;
		}

		auto response = cpr::Post(
		    cpr::Url{environment_manager::get_url("/auth/refresh")},
		    cpr::Header{{"Accept", "application/json"}, {"Content-Type", "application/json"}, {"User-Agent", user_agent}, {"Cookie", "refresh_token=" + refresh}});
		auto body = nlohmann::ordered_json::parse(response.text, nullptr, false);
		std::string token;
		if (body.is_object() && body.contains("data") && body["data"].is_object())
		{
			const auto& data = body["data"];
			token = data.value("access_token", data.value("accessToken", data.value("token", "")));
		}
		if (token.empty() && body.is_object())
			token = body.value("access_token", body.value("accessToken", body.value("token", "")));
		if (response.status_code < 200 || response.status_code >= 300 || token.empty())
		{
			clear_saved_session();
			set_error(body.is_object() ? body.value("message", "Saved session expired") : "Saved session expired");
			return 0;
		}
		token = normalize_access_token(std::move(token));
		std::string refresh_exp;
		if (body.is_object() && body.contains("data") && body["data"].is_object())
			refresh_exp = body["data"].value("expired_date", body["data"].value("expiry_date", ""));
		{
			std::scoped_lock lock(g_state_mutex);
			g_access_token = std::move(token);
			if (!refresh_exp.empty())
				g_login_expired_date = std::move(refresh_exp);
			g_error.clear();
		}
		const std::string rotated = refresh_cookie(response);
		if (!save_session(rotated.empty() ? refresh : rotated))
		{
			set_error("Session restored, but could not update the saved session");
			return 0;
		}
		return 1;
	}
	catch (const std::exception& error)
	{
		clear_saved_session();
		set_error(error);
		return 0;
	}
}

GV_API int __cdecl gv_logout()
{
	std::string token;
	{
		std::scoped_lock lock(g_state_mutex);
		token = g_access_token;
		g_access_token.clear();
		g_login_expired_date.clear();
		g_binaries = nlohmann::ordered_json::array();
		g_selected_binary = -1;
	}

	clear_saved_session();

	if (!token.empty())
	{
		try
		{
			std::string hwid = utils::get_hwid();
			cpr::Get(
			    cpr::Url{environment_manager::get_url("/auth/logout")},
			    cpr::Header{
			        {"Accept", "application/json"},
			        {"Authorization", authorization_value(token)},
			        {"User-Agent", user_agent},
			        {"X-HWID", hwid}},
			    cpr::Timeout{3000});
		}
		catch (...)
		{
		}
	}
	clear_error();
	return 1;
}

GV_API const char* __cdecl gv_refresh_binaries()
{
	try
	{
		std::string token;
		{
			std::scoped_lock lock(g_state_mutex);
			token = g_access_token;
		}
		if (token.empty())
		{
			set_error("Login is required");
			return nullptr;
		}

		auto response = cpr::Get(
		    cpr::Url{environment_manager::get_url("/binary/my-binaries")},
		    cpr::Header{{"Accept", "application/json"}, {"Content-Type", "application/json"}, {"Authorization", authorization_value(token)}, {"User-Agent", user_agent}});
		auto body = nlohmann::ordered_json::parse(response.text, nullptr, false);
		if (body.is_discarded() || response.status_code < 200 || response.status_code >= 300)
		{
			set_error("GET " + environment_manager::get_url("/binary/my-binaries")
			    + " failed (HTTP " + std::to_string(response.status_code) + "): "
			    + response_message(response, body));
			return nullptr;
		}

		auto catalog = body.contains("data") && body["data"].is_array() ? body["data"] : body;
		if (!catalog.is_array())
			catalog = nlohmann::ordered_json::array();

		// Cross-reference with GET /license/my-licenses so each product carries its own license expiry & status
		try
		{
			auto lic_res = cpr::Get(
			    cpr::Url{environment_manager::get_url("/license/my-licenses")},
			    cpr::Header{{"Accept", "application/json"}, {"Content-Type", "application/json"}, {"Authorization", authorization_value(token)}, {"User-Agent", user_agent}},
			    cpr::Timeout{3500});
			auto lic_body = nlohmann::ordered_json::parse(lic_res.text, nullptr, false);
			if (!lic_body.is_discarded() && lic_res.status_code >= 200 && lic_res.status_code < 300)
			{
				auto lic_list = lic_body.contains("data") && lic_body["data"].is_array()
				    ? lic_body["data"]
				    : (lic_body.is_array() ? lic_body : nlohmann::ordered_json::array());

				for (auto& item : catalog)
				{
					if (!item.is_object()) continue;
					std::string bin_id = item.value("id", "");
					std::string bin_name = item.value("name", item.value("game", ""));

					for (const auto& lic : lic_list)
					{
						if (!lic.is_object()) continue;
						std::string lic_pid = lic.value("product_id", lic.value("binary_id", ""));
						std::string lic_pname = lic.value("product_name", "");
						if (lic.contains("product") && lic["product"].is_object())
						{
							if (lic_pid.empty()) lic_pid = lic["product"].value("id", "");
							if (lic_pname.empty()) lic_pname = lic["product"].value("name", "");
						}

						bool match = false;
						if (!bin_id.empty() && !lic_pid.empty() && bin_id == lic_pid) match = true;
						else if (!bin_name.empty() && !lic_pname.empty() && _stricmp(bin_name.c_str(), lic_pname.c_str()) == 0) match = true;

						if (match)
						{
							item["expiry_date"] = lic.value("expiry_date", lic.value("expired_date", "Lifetime"));
							item["license_status"] = lic.value("status", "ACTIVE");
							item["license_key"] = lic.value("license_key", "");
							break;
						}
					}
				}
			}
		}
		catch (...)
		{
		}

		{
			std::scoped_lock lock(g_state_mutex);
			g_binaries = catalog;
			g_selected_binary = catalog.empty() ? -1 : 0;
			g_result = catalog.dump();
			g_error.clear();
		}
		return g_result.c_str();
	}
	catch (const std::exception& error)
	{
		set_error(error);
		return nullptr;
	}
}

GV_API const char* __cdecl gv_profile_json()
{
	try
	{
		std::string token;
		std::string cached_login_expiry;
		{
			std::scoped_lock lock(g_state_mutex);
			token = g_access_token;
			cached_login_expiry = g_login_expired_date;
		}
		if (token.empty())
		{
			set_error("Login is required");
			return nullptr;
		}

		auto response = cpr::Get(
		    cpr::Url{environment_manager::get_url("/user/profile")},
		    cpr::Header{{"Accept", "application/json"}, {"Content-Type", "application/json"}, {"Authorization", authorization_value(token)}, {"User-Agent", user_agent}});
		auto body = nlohmann::ordered_json::parse(response.text, nullptr, false);
		if (body.is_discarded() || response.status_code < 200 || response.status_code >= 300)
		{
			set_error("GET " + environment_manager::get_url("/user/profile")
			    + " failed (HTTP " + std::to_string(response.status_code) + "): " + response_message(response, body));
			return nullptr;
		}
		auto profile = body.contains("data") && body["data"].is_object() ? body["data"] : body;
		if (!profile.is_object())
			profile = nlohmann::ordered_json::object();

		// Fetch licenses from GET /license/my-licenses to obtain official license expiry dates
		try
		{
			auto lic_res = cpr::Get(
			    cpr::Url{environment_manager::get_url("/license/my-licenses")},
			    cpr::Header{{"Accept", "application/json"}, {"Content-Type", "application/json"}, {"Authorization", authorization_value(token)}, {"User-Agent", user_agent}},
			    cpr::Timeout{3500});
			auto lic_body = nlohmann::ordered_json::parse(lic_res.text, nullptr, false);
			if (!lic_body.is_discarded() && lic_res.status_code >= 200 && lic_res.status_code < 300)
			{
				auto lic_list = lic_body.contains("data") && lic_body["data"].is_array()
				    ? lic_body["data"]
				    : (lic_body.is_array() ? lic_body : nlohmann::ordered_json::array());
				profile["licenses"] = lic_list;

				if (!profile.contains("expired_date") || !profile["expired_date"].is_string() || profile["expired_date"].get<std::string>().empty())
				{
					for (const auto& lic : lic_list)
					{
						if (lic.is_object())
						{
							std::string exp = lic.value("expiry_date", lic.value("expired_date", ""));
							if (!exp.empty())
							{
								profile["expired_date"] = exp;
								break;
							}
						}
					}
				}
			}
		}
		catch (...)
		{
		}

		if ((!profile.contains("expired_date") || !profile["expired_date"].is_string() || profile["expired_date"].get<std::string>().empty()) && !cached_login_expiry.empty())
		{
			profile["expired_date"] = cached_login_expiry;
		}

		g_result = profile.dump();
		clear_error();
		return g_result.c_str();
	}
	catch (const std::exception& error)
	{
		set_error(error);
		return nullptr;
	}
}

GV_API int __cdecl gv_select_binary(int index)
{
	std::scoped_lock lock(g_state_mutex);
	if (index < 0 || !g_binaries.is_array() || index >= static_cast<int>(g_binaries.size()))
	{
		g_error = "Invalid binary selection";
		return 0;
	}
	g_selected_binary = index;
	g_error.clear();
	return 1;
}

GV_API int __cdecl gv_save_binary_settings(const char* binary_id, const char* target_process, int mode)
{
	try
	{
		std::string token;
		{
			std::scoped_lock lock(g_state_mutex);
			token = g_access_token;
		}
		if (token.empty())
		{
			set_error("Login is required");
			return 0;
		}
		if (!binary_id || !*binary_id)
		{
			set_error("binary_id is required");
			return 0;
		}

		nlohmann::json payload = {
		    {"binary_id", binary_id},
		    {"target_process", target_process ? target_process : ""},
		    {"injection_mode", mode}
		};

		auto response = cpr::Post(
		    cpr::Url{environment_manager::get_url("/binary/settings")},
		    cpr::Header{{"Accept", "application/json"}, {"Content-Type", "application/json"}, {"Authorization", authorization_value(token)}, {"User-Agent", user_agent}},
		    cpr::Body{payload.dump()});

		auto body = nlohmann::ordered_json::parse(response.text, nullptr, false);
		if (body.is_discarded() || response.status_code < 200 || response.status_code >= 300)
		{
			set_error("POST " + environment_manager::get_url("/binary/settings")
			    + " failed (HTTP " + std::to_string(response.status_code) + "): "
			    + response_message(response, body));
			return 0;
		}

		// Update in-memory g_binaries so it stays in sync
		{
			std::scoped_lock lock(g_state_mutex);
			if (g_binaries.is_array())
			{
				for (auto& item : g_binaries)
				{
					if (item.contains("id") && item["id"].is_string() && item["id"].get<std::string>() == binary_id)
					{
						item["target_process"] = target_process ? target_process : "";
						item["injection_mode"] = mode;
						break;
					}
				}
			}
			g_error.clear();
		}

		return 1;
	}
	catch (const std::exception& error)
	{
		set_error(error);
		return 0;
	}
}

GV_API int __cdecl gv_download_and_inject()
{
	set_operation(3, "Preparing download");
	std::string token;
	nlohmann::ordered_json binary;
	{
		std::scoped_lock lock(g_state_mutex);
		if (g_access_token.empty() || g_selected_binary < 0 || g_selected_binary >= static_cast<int>(g_binaries.size()))
		{
			g_error = "Login and binary selection are required";
			g_operation_stage = "Failed";
			g_operation_active = false;
			return 0;
		}
		token = g_access_token;
		binary = g_binaries[g_selected_binary];
	}

	const std::string binary_id = binary.value("id", "");
	const std::string target = binary.value("target", "");
	if (binary_id.empty())
	{
		set_error("Selected binary has no server ID");
		set_operation(0, "Failed", false);
		return 0;
	}

	std::filesystem::path encrypted_path;
	std::filesystem::path decrypted_path;
	try
	{
		auto cache_dir = file_manager::get_base_dir() / "Cache" / "Binaries";
		std::error_code dir_ec;
		std::filesystem::create_directories(cache_dir, dir_ec);
		encrypted_path = cache_dir / (binary_id + ".enc");

		bool need_download = true;
		std::error_code check_ec;
		if (std::filesystem::exists(encrypted_path, check_ec))
		{
			auto file_sz = std::filesystem::file_size(encrypted_path, check_ec);
			// Valid encrypted file contains 32B AES Key + 12B IV + 16B tag + ciphertext
			if (!check_ec && file_sz >= 64)
			{
				need_download = false;
			}
		}

		if (need_download)
		{
			set_operation(8, "Downloading encrypted binary");
			if (!encrypted_downloader::download_encrypted_to_file(
			        environment_manager::get_base_url(),
			        binary_id,
			        token,
			        encrypted_path,
			        [](float progress) {
				        set_operation(10 + static_cast<int>(progress * 65.0f), "Downloading encrypted binary");
			        }))
			{
				set_error("Encrypted binary download failed");
				set_operation(0, "Download failed", false);
				std::error_code ignored;
				std::filesystem::remove(encrypted_path, ignored);
				return 0;
			}
		}
		else
		{
			set_operation(75, "Using cached binary payload from disk");
		}

		set_operation(80, "Decrypting binary");
		if (!encrypted_downloader::decrypt_file_to_temp(
		        environment_manager::get_base_url(),
		        binary_id,
		        token,
		        encrypted_path,
		        decrypted_path))
		{
			// If cached file was corrupted or payload format changed, clean and retry fresh download
			std::error_code ignored;
			std::filesystem::remove(encrypted_path, ignored);

			set_operation(20, "Cached binary payload invalid, re-downloading fresh payload");
			if (!encrypted_downloader::download_encrypted_to_file(
			        environment_manager::get_base_url(),
			        binary_id,
			        token,
			        encrypted_path,
			        [](float progress) {
				        set_operation(20 + static_cast<int>(progress * 55.0f), "Re-downloading encrypted binary");
			        }))
			{
				set_error("Encrypted binary download failed on retry");
				set_operation(0, "Download failed", false);
				return 0;
			}

			set_operation(80, "Decrypting fresh binary");
			if (!encrypted_downloader::decrypt_file_to_temp(
			        environment_manager::get_base_url(),
			        binary_id,
			        token,
			        encrypted_path,
			        decrypted_path))
			{
				set_error("Binary decryption failed");
				set_operation(0, "Decryption failed", false);
				std::filesystem::remove(encrypted_path, ignored);
				return 0;
			}
		}

		set_operation(92, "Starting native operation");
		if (!target.empty())
			injection::set_target_process(target);
		const bool success = injection::inject_library(decrypted_path);
		std::error_code ignored;
		// Clean up plaintext decrypted temporary DLL only! Keep encrypted_path in disk cache!
		std::filesystem::remove(decrypted_path, ignored);
		if (!success)
		{
			set_error("Native injection returned false");
			set_operation(0, "Operation failed", false);
			return 0;
		}
		clear_error();
		set_operation(100, "Completed", false);
		return 1;
	}
	catch (const std::exception& error)
	{
		std::error_code ignored;
		if (!decrypted_path.empty())
			std::filesystem::remove(decrypted_path, ignored);
		set_error(error);
		set_operation(0, "Operation failed", false);
		return 0;
	}
}

GV_API const char* __cdecl gv_get_token()
{
	std::scoped_lock lock(g_state_mutex);
	g_result = g_access_token;
	return g_result.c_str();
}

GV_API const char* __cdecl gv_get_hwid()
{
	try
	{
		g_result = utils::get_hwid();
		return g_result.c_str();
	}
	catch (...)
	{
		return "";
	}
}

GV_API const char* __cdecl gv_get_hwid_raw()
{
	try
	{
		g_result = utils::get_hwid_raw();
		return g_result.c_str();
	}
	catch (...)
	{
		return "";
	}
}

GV_API const char* __cdecl gv_get_device_name()
{
	try
	{
		g_result = utils::get_device_name();
		return g_result.c_str();
	}
	catch (...)
	{
		return "Desktop-PC";
	}
}

GV_API const char* __cdecl gv_get_backend_url()
{
	try
	{
		g_result = environment_manager::get_base_url();
		return g_result.c_str();
	}
	catch (...)
	{
		return "https://apie.rena.my.id";
	}
}

