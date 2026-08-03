#pragma once
#include "common.hpp"
#include "api/http_request.hpp"
#include "api/url_encryption.hpp"
#include "hardware_authentication.hpp"
#include "file_manager.hpp"

namespace gottvergessen
{
	class user_authentication;
	inline user_authentication* g_user_authentication;

	enum class ProductGrade
	{
		NOT_OWNED,
		STADNARD,
		SILVER,
		GOLD,
		ADMIN
	};

	struct ExpiryDate
	{
		int year;
		int month;
		int day;
		int hour;
		int minute;
		int second;
	};

	class user_authentication : public user_agent_provider
	{
		friend class views;

	public:
		explicit user_authentication()
		{
			g_user_authentication = this;
		}

		virtual ~user_authentication()
		{
			g_user_authentication = nullptr;
		}

		user_authentication(user_authentication const& that) = delete;
		user_authentication& operator=(user_authentication const& that) = delete;
		user_authentication(user_authentication&& that) = delete;
		user_authentication& operator=(user_authentication&& that) = delete;

		void check_auto_login()
		{
			if (this->authorized())
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
				cpr::Url uri = url_refresh;
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
				fetch_profile();

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

		bool fetch_profile()
		{
			if (session_token.empty())
				return false;
			const std::string token = std::format("Bearer {}", this->get_token());
			try
			{
				cpr::Url uri = url_profile;
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
				}
			}
			catch (const std::exception&)
			{
				LOG(WARNING) << xorstr("Failed to fetch user profile from server.");
				return false;
			}
			return true;
		}

		bool login(const std::string username_param, const std::string password_param)
		{
			nlohmann::ordered_json json = {
			    {xorstr("username"), username_param},
			    {xorstr("password"), password_param}};

			try
			{
				cpr::Url uri = url;
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
				fetch_profile();

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
		void logout()
		{
			const std::string token = std::format("Bearer {}", this->get_token());

			try
			{
				cpr::Url uri = url_logout;
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
		}

		bool log_activity(const std::string& action_param, const std::string& description_param = "")
		{
			if (!this->authorized())
				return false;
			try
			{
				cpr::Url uri = xorstr("http://localhost:8180/activity");
				nlohmann::ordered_json j = {
				    {xorstr("action"), action_param},
				    {xorstr("description"), description_param}};
				cpr::Body body = j.dump();
				cpr::Header header{
				    {xorstr("Accept"), xorstr("application/json")},
				    {xorstr("Content-Type"), xorstr("application/json")},
				    {xorstr("Authorization"), std::format("Bearer {}", this->get_token())},
				    {xorstr("User-Agent"), this->get_user_agent()}};
				auto res = cpr::Post(uri, body, header);
				return res.status_code == 200 || res.status_code == 201;
			}
			catch (const std::exception&)
			{
				return false;
			}
		}

		ProductGrade owned_product()
		{
			return this->ownership;
		}
		std::string owned_product_info(ProductGrade product_grade)
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
			}

			return "DEVELOPER VERSION";
		}
		std::string ownership_expiry_date()
		{
			return this->expired_date;
		}
		bool authorized() const
		{
			return !this->session_token.empty();
		}
		std::string get_token() const
		{
			return this->session_token;
		}
		std::string get_message() const
		{
			return this->message;
		}
		std::string get_fullname() const
		{
			return this->fullname;
		}
		std::string get_username() const
		{
			return std::string(this->username);
		}
		std::string get_role() const
		{
			return this->role;
		}
		int get_status() const
		{
			return this->status;
		}

		nlohmann::ordered_json api_get(const std::string& endpoint)
		{
			if (!this->authorized())
				return nlohmann::ordered_json::value_t::discarded;

			cpr::Url uri = endpoint;
			cpr::Header header{
				{xorstr("Accept"), xorstr("application/json")},
				{xorstr("Content-Type"), xorstr("application/json")},
				{xorstr("Authorization"), std::format("Bearer {}", this->get_token())},
				{xorstr("User-Agent"), this->get_user_agent()}};

			auto res = cpr::Get(uri, header);
			return nlohmann::ordered_json::parse(res.text, nullptr, false);
		}

	private:
		std::filesystem::path get_session_file_path() const
		{
			auto folder = file_manager::get_project_folder(xorstr("./Config"));
			return folder.get_file(xorstr("./session.json")).get_path();
		}

		void save_session(const std::string& refresh_token)
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

		void clear_session()
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

		std::string extract_refresh_cookie(const cpr::Response& res) const
		{
			for (const auto& [key, value] : res.header)
			{
				if (key == "Set-Cookie" || key == "set-cookie")
				{
					// Parse "refresh_token=VALUE; ..." from Set-Cookie header
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

	protected:
		const std::string url = xorstr("http://localhost:8180/auth/login");
		const std::string url_logout = xorstr("http://localhost:8180/auth/logout");
		const std::string url_refresh = xorstr("http://localhost:8180/auth/refresh");
		const std::string url_profile = xorstr("http://localhost:8180/user/profile");

	private:
		inline static char username[32];
		inline static char password[32];

	protected:
		int status{0};
		std::string role;
		std::string message;
		std::string session_token;
		std::string fullname;
		std::string expired_date;
		ProductGrade ownership;
		ExpiryDate expired;
	};
}