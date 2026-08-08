#include "api/user/user_authentication.hpp"
#include "api/environment.hpp"
#include "file_manager.hpp"
#include "textures/textures.hpp"
#include "renderer.hpp"
#include "logger.hpp"
#include "api/url_encryption.hpp"
#include <fstream>
#include <format>
#include <cpr/cpr.h>

namespace gottvergessen
{
	user_authentication::~user_authentication()
	{
		clear_avatar_texture_impl();
	}

	void user_authentication::init_impl()
	{
	}

	void user_authentication::destroy_impl()
	{
		clear_avatar_texture_impl();
	}

	void user_authentication::clear_avatar_texture_impl()
	{
		std::lock_guard<std::mutex> lock(this->avatar_mutex);
		if (avatar_srv)
		{
			textures::destroy_texture(&avatar_srv);
			avatar_srv = nullptr;
		}
		avatar_loaded = false;
		avatar_loading = false;
		avatar_bytes_ready = false;
		avatar_raw_bytes.clear();
		avatar_raw_bytes.shrink_to_fit();
	}

	void user_authentication::trigger_avatar_download_impl()
	{
		if (avatar_url.empty() && !user_id.empty())
		{
			avatar_url = "/user/avatar/" + user_id;
		}

		if (avatar_url.empty())
			return;

		if (avatar_loading || avatar_loaded)
			return;

		std::string full_url;
		if (avatar_url.rfind("http://", 0) == 0 || avatar_url.rfind("https://", 0) == 0)
		{
			full_url = avatar_url;
		}
		else
		{
			full_url = environment_manager::get().get_url(avatar_url);
		}

		avatar_loading = true;
		std::thread([this, full_url, token = this->get_token_impl()]() {
			try
			{
				cpr::Header header{
				    {xorstr("Accept"), xorstr("image/*, */*")},
				    {xorstr("Authorization"), std::format("Bearer {}", token)},
				    {xorstr("User-Agent"), this->get_user_agent()}};
				auto res = cpr::Get(cpr::Url{full_url}, header);
				if (res.status_code == 200 && !res.text.empty())
				{
					std::lock_guard<std::mutex> lock(this->avatar_mutex);
					this->avatar_raw_bytes.assign(res.text.begin(), res.text.end());
					this->avatar_bytes_ready = true;
					LOG(INFO) << xorstr("Downloaded avatar binary successfully (") << res.text.size() << xorstr(" bytes) from ") << full_url;
				}
				else
				{
					LOG(WARNING) << xorstr("Failed to download avatar from ") << full_url << xorstr(" status: ") << res.status_code;
				}
			}
			catch (const std::exception& e)
			{
				LOG(WARNING) << xorstr("Exception downloading avatar: ") << e.what();
			}
			this->avatar_loading = false;
		}).detach();
	}

	ID3D11ShaderResourceView* user_authentication::get_avatar_texture_impl()
	{
		if (!avatar_loaded && !avatar_loading && (!avatar_url.empty() || !user_id.empty()))
		{
			trigger_avatar_download_impl();
		}

		if (avatar_bytes_ready)
		{
			std::lock_guard<std::mutex> lock(this->avatar_mutex);
			avatar_bytes_ready = false;
			if (g_renderer && g_renderer->m_device && !avatar_raw_bytes.empty())
			{
				if (avatar_srv)
				{
					textures::destroy_texture(&avatar_srv);
					avatar_srv = nullptr;
				}
				int w = 0, h = 0;
				if (textures::load_from_memory(avatar_raw_bytes.data(), static_cast<int>(avatar_raw_bytes.size()), g_renderer->m_device, &avatar_srv, &w, &h))
				{
					avatar_loaded = true;
					LOG(INFO) << xorstr("Successfully created avatar D3D11 texture: ") << w << "x" << h;
				}
				else
				{
					LOG(WARNING) << xorstr("Failed to decode avatar binary memory with stb_image.");
				}
				avatar_raw_bytes.clear();
				avatar_raw_bytes.shrink_to_fit();
			}
		}
		return avatar_srv;
	}

	void user_authentication::check_auto_login_impl()
	{
		if (this->authorized_impl())
			return;

		try
		{
			auto session_path = get_session_file_path();
			if (!std::filesystem::exists(session_path))
				return;

			std::ifstream file(session_path);
			if (!file.is_open())
				return;

			nlohmann::json saved;
			file >> saved;
			file.close();

			std::string refresh = saved.value("refresh_token", "");
			if (refresh.empty())
			{
				clear_session();
				return;
			}

			// Try to refresh the token via /auth/refresh
			cpr::Url uri = environment_manager::get().get_url("/auth/refresh");
			cpr::Header header{
			    {xorstr("Accept"), xorstr("application/json")},
			    {xorstr("Content-Type"), xorstr("application/json")},
			    {xorstr("User-Agent"), this->get_user_agent()},
			    {xorstr("Cookie"), std::format("refresh_token={}", refresh)}};

			auto res = cpr::Post(uri, header);
			auto j = nlohmann::ordered_json::parse(res.text, nullptr, false);

			if (j.is_discarded() || !j.value("success", false))
			{
				LOG(WARNING) << xorstr("Auto-login failed: refresh token expired or invalid.");
				clear_session();
				return;
			}

			// Extract new access token
			if (j.contains("data") && j["data"].is_object() && j["data"].contains("token"))
			{
				session_token = j["data"]["token"].get<std::string>();
			}
			else if (j.contains("token"))
			{
				session_token = j["token"].get<std::string>();
			}

			if (session_token.empty())
			{
				clear_session();
				return;
			}

			// Update refresh token from Set-Cookie if server sent a new one
			std::string new_refresh = extract_refresh_cookie(res);
			if (!new_refresh.empty())
			{
				refresh = new_refresh;
			}

			status = 200;
			message = "Auto-login success";

			// Restore user profile from server
			fetch_profile_impl();

			// Save updated session
			save_session(refresh);

			LOG(INFO) << xorstr("Auto-login successful.");
		}
		catch (const std::exception&)
		{
			LOG(WARNING) << xorstr("Auto-login failed, server might be offline.");
			clear_session();
		}
	}

	bool user_authentication::fetch_profile_impl()
	{
		if (session_token.empty())
			return false;
		const std::string token = std::format("Bearer {}", this->get_token_impl());
		try
		{
			cpr::Url uri = environment_manager::get().get_url("/user/profile");
			cpr::Header header{
			    {xorstr("Accept"), xorstr("application/json")},
			    {xorstr("Content-Type"), xorstr("application/json")},
			    {xorstr("Authorization"), token},
			    {xorstr("User-Agent"), this->get_user_agent()}};

			auto res = cpr::Get(uri, header);
			auto j = nlohmann::ordered_json::parse(res.text.begin(), res.text.end(), nullptr, false);
			if (!j.is_discarded() && j.value("success", false) && j.contains("data"))
			{
				auto& user_data = j["data"];
				if (user_data.contains("id") && user_data["id"].is_string())
				{
					user_id = user_data["id"].get<std::string>();
				}
				else if (user_data.contains("user_id") && user_data["user_id"].is_string())
				{
					user_id = user_data["user_id"].get<std::string>();
				}

				if (user_data.contains("firstname") && user_data["firstname"].is_string())
				{
					std::string fn = user_data["firstname"].get<std::string>();
					std::string ln = user_data.value("lastname", "");
					fullname = ln.empty() ? fn : (fn + " " + ln);
				}
				if (user_data.contains("username") && user_data["username"].is_string())
				{
					std::string u = user_data["username"].get<std::string>();
					strncpy_s(username, sizeof(username), u.c_str(), _TRUNCATE);
				}
				if (user_data.contains("role") && user_data["role"].is_string())
				{
					role = user_data["role"].get<std::string>();
				}
				if (user_data.contains("avatar") && user_data["avatar"].is_string())
				{
					avatar_url = user_data["avatar"].get<std::string>();
				}
				else if (user_data.contains("avatar_url") && user_data["avatar_url"].is_string())
				{
					avatar_url = user_data["avatar_url"].get<std::string>();
				}

				if (avatar_url.empty() && !user_id.empty())
				{
					avatar_url = "/user/avatar/" + user_id;
				}

				trigger_avatar_download_impl();
			}
		}
		catch (const std::exception&)
		{
			LOG(WARNING) << xorstr("Failed to fetch user profile from server.");
			return false;
		}
		return true;
	}

	bool user_authentication::login_impl(const std::string username_param, const std::string password_param)
	{
		nlohmann::ordered_json json = {
		    {xorstr("username"), username_param},
		    {xorstr("password"), password_param}};

		try
		{
			cpr::Url uri = environment_manager::get().get_url("/auth/login");
			cpr::Body body = json.dump();
			cpr::Header header = cpr::Header{
			    {xorstr("Accept"), xorstr("application/json")},
			    {xorstr("Content-Type"), xorstr("application/json")},
			    {xorstr("User-Agent"), this->get_user_agent()},
			};

			cpr::Response res = cpr::Post(uri, body, header);

			auto j = nlohmann::ordered_json::parse(res.text.begin(), res.text.end(), nullptr, false);
			if (j.is_discarded())
			{
				LOG(WARNING) << xorstr("Failed to parse login response JSON.");
				return false;
			}

			if (j.contains("data") && j["data"].is_object() && j["data"].contains("token"))
			{
				session_token = j["data"]["token"].get<std::string>();
			}
			else if (j.contains("token"))
			{
				session_token = j["token"].get<std::string>();
			}

			bool is_success = j.value("success", false) || !session_token.empty();

			if (!is_success)
			{
				message = j.value("message", "Login failed, invalid credentials.");
				return false;
			}

			status = 200;
			message = j.value("message", "Login success");
			fullname = j.contains("fullname") && j["fullname"].is_string() ? j["fullname"].get<std::string>() : username_param;
			role = j.contains("role") && j["role"].is_string() ? j["role"].get<std::string>() : "Customer";
			expired_date = j.contains("expired_date") && j["expired_date"].is_string() ? j["expired_date"].get<std::string>() : "Lifetime Access";
			if (j.contains("ownership") && j["ownership"].is_number_integer())
			{
				ownership = j["ownership"].get<ProductGrade>();
			}
			else
			{
				ownership = ProductGrade::GOLD;
			}

			// Fetch profile from /user/profile
			fetch_profile_impl();

			// Save session for auto-login
			std::string refresh = extract_refresh_cookie(res);
			if (!refresh.empty())
			{
				save_session(refresh);
			}
		}
		catch (const std::exception&)
		{
			LOG(WARNING) << xorstr("Login failed, is the host down?");
			return false;
		}

		return true;
	}

	void user_authentication::logout_impl()
	{
		const std::string token = std::format("Bearer {}", this->get_token_impl());

		try
		{
			cpr::Url uri = environment_manager::get().get_url("/auth/logout");
			cpr::Header header{
			    {xorstr("Content-Type"), xorstr("application/json")},
			    {xorstr("Authorization"), token},
			    {xorstr("Accept"), xorstr("application/json")},
			    {xorstr("User-Agent"), this->get_user_agent()}};

			auto res = cpr::Get(uri, header);
		}
		catch (const std::exception&)
		{
			LOG(WARNING) << xorstr("Logout failed, is the host down?");
		}

		clear_session();
		session_token.clear();
		fullname.clear();
		role.clear();
		avatar_url.clear();
		user_id.clear();
		clear_avatar_texture_impl();
	}

	bool user_authentication::log_activity_impl(const std::string& action_param, const std::string& description_param)
	{
		if (!this->authorized_impl())
			return false;
		try
		{
			cpr::Url uri = environment_manager::get().get_url("/activity");
			nlohmann::ordered_json j = {
			    {xorstr("action"), action_param},
			    {xorstr("description"), description_param}};
			cpr::Body body = j.dump();
			cpr::Header header{
			    {xorstr("Accept"), xorstr("application/json")},
			    {xorstr("Content-Type"), xorstr("application/json")},
			    {xorstr("Authorization"), std::format("Bearer {}", this->get_token_impl())},
			    {xorstr("User-Agent"), this->get_user_agent()}};
			auto res = cpr::Post(uri, body, header);
			return res.status_code == 200 || res.status_code == 201;
		}
		catch (const std::exception&)
		{
			return false;
		}
	}

	nlohmann::ordered_json user_authentication::api_get_impl(const std::string& endpoint)
	{
		if (!this->authorized_impl())
			return nlohmann::ordered_json::value_t::discarded;

		cpr::Url uri = endpoint;
		cpr::Header header{
			{xorstr("Accept"), xorstr("application/json")},
			{xorstr("Content-Type"), xorstr("application/json")},
			{xorstr("Authorization"), std::format("Bearer {}", this->get_token_impl())},
			{xorstr("User-Agent"), this->get_user_agent()}};

		auto res = cpr::Get(uri, header);
		return nlohmann::ordered_json::parse(res.text, nullptr, false);
	}

	std::string user_authentication::owned_product_info_impl(ProductGrade product_grade) const
	{
		switch (product_grade)
		{
		case ProductGrade::NOT_OWNED:
			return "NOT OWNED";
		case ProductGrade::STADNARD:
			return "STANDARD EDITION";
		case ProductGrade::SILVER:
			return "SILVER EDITION";
		case ProductGrade::GOLD:
			return "GOLD EDITION";
		case ProductGrade::ADMIN:
			return "DEVELOPER VERSION";
		default:
			return "DEVELOPER VERSION";
		}
	}

	std::filesystem::path user_authentication::get_session_file_path() const
	{
		auto folder = file_manager::get_project_folder(xorstr("./Config"));
		return folder.get_file(xorstr("./session.json")).get_path();
	}

	void user_authentication::save_session(const std::string& refresh_token)
	{
		try
		{
			nlohmann::json session = {
			    {"refresh_token", refresh_token}};
			std::ofstream file(get_session_file_path(), std::ios::trunc);
			file << session.dump(4);
			file.close();
		}
		catch (const std::exception&)
		{
			LOG(WARNING) << xorstr("Failed to save session file.");
		}
	}

	void user_authentication::clear_session()
	{
		try
		{
			auto path = get_session_file_path();
			if (std::filesystem::exists(path))
				std::filesystem::remove(path);
		}
		catch (const std::exception&)
		{
		}
	}

	std::string user_authentication::extract_refresh_cookie(const cpr::Response& res) const
	{
		for (const auto& [key, value] : res.header)
		{
			if (key == "Set-Cookie" || key == "set-cookie")
			{
				std::string cookie_str = value;
				const std::string prefix = "refresh_token=";
				auto pos = cookie_str.find(prefix);
				if (pos != std::string::npos)
				{
					auto start = pos + prefix.size();
					auto end = cookie_str.find(';', start);
					return cookie_str.substr(start, end == std::string::npos ? std::string::npos : end - start);
				}
			}
		}
		return "";
	}

	std::string user_authentication::get_url_login() const { return environment_manager::get().get_url("/auth/login"); }
	std::string user_authentication::get_url_logout() const { return environment_manager::get().get_url("/auth/logout"); }
	std::string user_authentication::get_url_refresh() const { return environment_manager::get().get_url("/auth/refresh"); }
	std::string user_authentication::get_url_profile() const { return environment_manager::get().get_url("/user/profile"); }
}
