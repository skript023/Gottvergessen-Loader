#include "file_manager.hpp"
#include "logger.hpp"
#include "process/injection.hpp"
#include "api/encrypted_downloader.hpp"
#include "api/environment.hpp"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <Windows.h>
#include <wincrypt.h>
#include <cstdlib>
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
	std::filesystem::path g_session_path;
	nlohmann::ordered_json g_binaries = nlohmann::ordered_json::array();
	int g_selected_binary{-1};

	constexpr const char* user_agent = "Gottvergessen-Loader/1.0";

	std::vector<unsigned char> protect_session(const std::string& value)
	{
		DATA_BLOB input{static_cast<DWORD>(value.size()), reinterpret_cast<BYTE*>(const_cast<char*>(value.data()))};
		DATA_BLOB output{};
		if (!CryptProtectData(&input, L"GottvergessenSessionData", nullptr, nullptr, nullptr,
			CRYPTPROTECT_UI_FORBIDDEN, &output))
			return {};
		std::vector<unsigned char> encrypted(output.pbData, output.pbData + output.cbData);
		LocalFree(output.pbData);
		return encrypted;
	}

	std::string unprotect_session(const std::vector<unsigned char>& value)
	{
		if (value.empty()) return {};
		DATA_BLOB input{static_cast<DWORD>(value.size()), const_cast<BYTE*>(value.data())};
		DATA_BLOB output{};
		if (CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr,
			CRYPTPROTECT_UI_FORBIDDEN, &output))
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
			if (key != "Set-Cookie" && key != "set-cookie") continue;
			constexpr std::string_view prefix = "refresh_token=";
			const auto position = value.find(prefix);
			if (position == std::string::npos) continue;
			const auto start = position + prefix.size();
			const auto end = value.find(';', start);
			return value.substr(start, end == std::string::npos ? std::string::npos : end - start);
		}
		return {};
	}

	void clear_saved_session()
	{
		std::error_code ignored;
		if (!g_session_path.empty()) std::filesystem::remove(g_session_path, ignored);
		if (!g_session_path.empty()) std::filesystem::remove(g_session_path.parent_path() / "session.json", ignored);
	}

	bool save_session(const std::string& refresh_token)
	{
		if (refresh_token.empty() || g_session_path.empty()) return false;
		std::filesystem::create_directories(g_session_path.parent_path());
		const auto payload = nlohmann::ordered_json{{"refresh_token", refresh_token}}.dump(4);
		const auto encrypted = protect_session(payload);
		if (encrypted.empty()) return false;
		std::ofstream file(g_session_path, std::ios::binary | std::ios::trunc);
		file.write(reinterpret_cast<const char*>(encrypted.data()), static_cast<std::streamsize>(encrypted.size()));
		if (!file.good()) return false;
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

GV_API const char* __cdecl gv_list_processes_json()
{
	try
	{
		const auto processes = injection_method::get_running_processes();
		g_result = "[";
		for (size_t index = 0; index < processes.size(); ++index)
		{
			const auto& process = processes[index];
			if (index != 0) g_result += ',';
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
		if (!valid) set_error("DLL validation failed");
		else clear_error();
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
		if (!success) set_error("Native injection returned false");
		else clear_error();
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

GV_API int __cdecl gv_login(const char* username, const char* password, int remember_me)
{
	if (!username || !*username || !password || !*password)
	{
		set_error("Username and password are required");
		return 0;
	}

	try
	{
		nlohmann::ordered_json body_json = {{"username", username}, {"password", password}};
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
		if (body.contains("data") && body["data"].is_object()) token = body["data"].value("token", "");
		if (token.empty()) token = body.value("token", "");
		if (response.status_code < 200 || response.status_code >= 300 || token.empty())
		{
			set_error(body.value("message", "Login failed"));
			return 0;
		}

		const std::string refresh = refresh_cookie(response);
		{
			std::scoped_lock lock(g_state_mutex);
			g_access_token = std::move(token);
			g_error.clear();
		}
		if (remember_me != 0)
		{
			// Keep login successful even when an older server does not issue a
			// refresh cookie; in that case only this app run remains authorized.
			if (refresh.empty() || !save_session(refresh)) clear_saved_session();
		}
		else clear_saved_session();
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
		std::filesystem::path session_path = g_session_path;
		if (!std::filesystem::exists(session_path))
		{
			const auto legacy = session_path.parent_path() / "session.json";
			if (!std::filesystem::exists(legacy)) return 0;
			session_path = legacy;
		}
		std::ifstream file(session_path, std::ios::binary | std::ios::ate);
		if (!file) return 0;
		const auto size = file.tellg();
		if (size <= 0) { clear_saved_session(); return 0; }
		std::vector<unsigned char> bytes(static_cast<size_t>(size));
		file.seekg(0);
		file.read(reinterpret_cast<char*>(bytes.data()), size);
		auto saved = nlohmann::ordered_json::parse(unprotect_session(bytes), nullptr, false);
		const std::string refresh = saved.is_object() ? saved.value("refresh_token", "") : "";
		if (refresh.empty()) { clear_saved_session(); return 0; }

		auto response = cpr::Post(
			cpr::Url{environment_manager::get_url("/auth/refresh")},
			cpr::Header{{"Accept", "application/json"}, {"Content-Type", "application/json"},
				{"User-Agent", user_agent}, {"Cookie", "refresh_token=" + refresh}});
		auto body = nlohmann::ordered_json::parse(response.text, nullptr, false);
		std::string token;
		if (body.is_object() && body.contains("data") && body["data"].is_object()) token = body["data"].value("token", "");
		if (token.empty() && body.is_object()) token = body.value("token", "");
		if (response.status_code < 200 || response.status_code >= 300 || token.empty())
		{
			clear_saved_session();
			set_error(body.is_object() ? body.value("message", "Saved session expired") : "Saved session expired");
			return 0;
		}
		{
			std::scoped_lock lock(g_state_mutex);
			g_access_token = std::move(token);
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
			cpr::Header{{"Accept", "application/json"}, {"Authorization", "Bearer " + token}});
		auto body = nlohmann::ordered_json::parse(response.text, nullptr, false);
		if (body.is_discarded() || response.status_code < 200 || response.status_code >= 300)
		{
			set_error("Failed to load binary catalog");
			return nullptr;
		}

		auto catalog = body.contains("data") && body["data"].is_array() ? body["data"] : body;
		if (!catalog.is_array()) catalog = nlohmann::ordered_json::array();
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

GV_API int __cdecl gv_download_and_inject()
{
	std::string token;
	nlohmann::ordered_json binary;
	{
		std::scoped_lock lock(g_state_mutex);
		if (g_access_token.empty() || g_selected_binary < 0 || g_selected_binary >= static_cast<int>(g_binaries.size()))
		{
			g_error = "Login and binary selection are required";
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
		return 0;
	}

	std::filesystem::path encrypted_path;
	std::filesystem::path decrypted_path;
	try
	{
		encrypted_path = std::filesystem::temp_directory_path() /
			("el_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".enc");
		if (!encrypted_downloader::download_encrypted_to_file(
			environment_manager::get_base_url(), binary_id, token, encrypted_path))
		{
			set_error("Encrypted binary download failed");
			return 0;
		}
		if (!encrypted_downloader::decrypt_file_to_temp(
			environment_manager::get_base_url(), binary_id, token, encrypted_path, decrypted_path))
		{
			set_error("Binary decryption failed");
			std::error_code ignored;
			std::filesystem::remove(encrypted_path, ignored);
			return 0;
		}

		if (!target.empty()) injection::set_target_process(target);
		const bool success = injection::inject_library(decrypted_path);
		std::error_code ignored;
		std::filesystem::remove(encrypted_path, ignored);
		std::filesystem::remove(decrypted_path, ignored);
		if (!success)
		{
			set_error("Native injection returned false");
			return 0;
		}
		clear_error();
		return 1;
	}
	catch (const std::exception& error)
	{
		std::error_code ignored;
		if (!encrypted_path.empty()) std::filesystem::remove(encrypted_path, ignored);
		if (!decrypted_path.empty()) std::filesystem::remove(decrypted_path, ignored);
		set_error(error);
		return 0;
	}
}
