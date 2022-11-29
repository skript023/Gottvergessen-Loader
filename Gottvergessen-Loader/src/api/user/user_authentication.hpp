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
			nlohmann::ordered_json body = {
				{ xorstr("username"), username },
				{ xorstr("password"), password },
				{ xorstr("bios_uuid"), this->get_bios() },
				{ xorstr("computer_name"), this->get_computer_name() }
			};

			try
			{
				http::Request req(url);
				http::Response res = req.send(xorstr("POST"), body.dump(), { xorstr("Content-Type: application/json") });

				nlohmann::ordered_json j = nlohmann::json::parse(res.body.begin(), res.body.end());

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
			const std::string content_type = xorstr("Content-Type: application/json");
			std::string accept = xorstr("Accept: application/json");
			std::string auth = std::format("Authorization: Bearer {}", this->get_token());

			try
			{
				http::Request req(url_logout);
				http::Response res = req.send(xorstr("POST"), "", { auth, accept, content_type });

				nlohmann::ordered_json j = nlohmann::json::parse(res.body.begin(), res.body.end());

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
		bool authorized() { return !this->session_token.empty(); }
		std::string get_token() { return this->session_token; }
		std::string get_message() { return this->message; }
		std::string get_fullname() { return this->fullname; }
		std::string get_role() { return this->role; }
		int get_status() { return this->status; }
	protected:
		const std::string url = xorstr("http://localhost:8000/api/v1/auth/login");
	protected:
		const std::string url_logout = xorstr("http://localhost:8000/api/v1/auth/logout");
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