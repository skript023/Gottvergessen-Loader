#pragma once
#include "common.hpp"
#include "api/http_request.hpp"
#include "api/url_encryption.hpp"
#include "hardware_authentication.hpp"

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

	class user_authentication : public hardware_authentication
	{
		friend class views;
	public:
		explicit user_authentication()
		{
			g_user_authentication = this;
		}

		virtual ~user_authentication()
		{
			if (this->authorized())
				this->logout();

			g_user_authentication = nullptr;
		}

		user_authentication(user_authentication const& that) = delete;
		user_authentication& operator=(user_authentication const& that) = delete;
		user_authentication(user_authentication&& that) = delete;
		user_authentication& operator=(user_authentication&& that) = delete;

		bool login(const std::string username, const std::string password)
		{
			nlohmann::ordered_json json = {
				{ xorstr("username"), username },
				{ xorstr("password"), password },
				{ xorstr("hardware_uuid"), this->get_bios() },
				{ xorstr("computer_name"), this->get_computer_name() }
			};

			try
			{
				cpr::Url uri = url;
				cpr::Body body = json.dump();
				cpr::Header header = cpr::Header {
					{ xorstr("Accept"), xorstr("application/json") },
					{ xorstr("Content-Type"), xorstr("application/json") },
				};

				cpr::Response res = cpr::Post(uri, body, header);

				nlohmann::ordered_json j = nlohmann::json::parse(res.text.begin(), res.text.end());

				status = j["status"];
				session_token = j["token"];
				fullname = j["fullname"];
				role = j["role"];
				ownership = j["ownership"].get<ProductGrade>();
				message = j["message"];
				expired_date = j["expired_date"];
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
				cpr::Header header { 
					{ xorstr("Content-Type"), xorstr("application/json") }, 
					{ xorstr("Authorization"), token}, {xorstr("Accept"), xorstr("application/json") }
				};

				auto res = cpr::Get(uri, header);

				nlohmann::ordered_json j = nlohmann::json::parse(res.text.begin(), res.text.end());

				status = j["status"];
				message = j["message"];
			}
			catch (const std::exception&)
			{
				LOG(WARNING) << xorstr("Logout failed, is the host down?");
			}
		}
		ProductGrade owned_product() { return this->ownership; }
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
		std::string ownership_expiry_date() { return this->expired_date; }
		bool authorized() const { return !this->session_token.empty(); }
		std::string get_token() const { return this->session_token; }
		std::string get_message() const { return this->message; }
		std::string get_fullname() const { return this->fullname; }
		std::string get_username() const { return std::string(this->username); }
		std::string get_role() const { return this->role; }
		int get_status() const { return this->status; }
	protected:
		const std::string url = xorstr("https://gottvergessen.000webhostapp.com/api/v1/auth/login");
	protected:
		const std::string url_logout = xorstr("https://gottvergessen.000webhostapp.com/api/v1/auth/logout");
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