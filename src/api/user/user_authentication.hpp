#pragma once
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <d3d11.h>
#include "common.hpp"
#include "api/http_request.hpp"
#include "api/user/hardware_authentication.hpp"

namespace gottvergessen
{
	class user_authentication;

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

	// User Authentication Service Class (Static Singleton with _impl Methods)
	class user_authentication : public user_agent_provider
	{
		friend class views;

	public:
		static user_authentication& instance()
		{
			static user_authentication instance_val;
			return instance_val;
		}

		static user_authentication& get()
		{
			return instance();
		}

		static void init() { instance().init_impl(); }
		static void destroy() { instance().destroy_impl(); }

		// Static Facade Methods -> Delegasi ke *_impl()
		static void check_auto_login() { instance().check_auto_login_impl(); }
		static bool fetch_profile() { return instance().fetch_profile_impl(); }
		static bool login(const std::string username_param, const std::string password_param)
		{
			return instance().login_impl(username_param, password_param);
		}
		static void logout() { instance().logout_impl(); }
		static bool log_activity(const std::string& action_param, const std::string& description_param = "")
		{
			return instance().log_activity_impl(action_param, description_param);
		}

		static void clear_avatar_texture() { instance().clear_avatar_texture_impl(); }
		static void trigger_avatar_download() { instance().trigger_avatar_download_impl(); }
		static ID3D11ShaderResourceView* get_avatar_texture() { return instance().get_avatar_texture_impl(); }

		static nlohmann::ordered_json api_get(const std::string& endpoint)
		{
			return instance().api_get_impl(endpoint);
		}

		static bool authorized() { return instance().authorized_impl(); }
		static std::string get_token() { return instance().get_token_impl(); }
		static std::string get_message() { return instance().get_message_impl(); }
		static std::string get_fullname() { return instance().get_fullname_impl(); }
		static std::string get_username() { return instance().get_username_impl(); }
		static std::string get_role() { return instance().get_role_impl(); }
		static std::string get_avatar_url() { return instance().get_avatar_url_impl(); }
		static std::string get_user_agent() { return instance().get_user_agent_impl(); }
		static int get_status() { return instance().get_status_impl(); }
		static ProductGrade owned_product() { return instance().owned_product_impl(); }
		static std::string owned_product_info(ProductGrade product_grade)
		{
			return instance().owned_product_info_impl(product_grade);
		}
		static char* username_buf() { return instance().username; }
		static char* password_buf() { return instance().password; }
		static size_t username_buf_size() { return sizeof(instance().username); }
		static size_t password_buf_size() { return sizeof(instance().password); }
		static void clear_password_buf() { memset(instance().password, 0, sizeof(instance().password)); }

	public:
		char username[32]{};
		char password[32]{};

	private:
		user_authentication() = default;
		~user_authentication();

		user_authentication(user_authentication const&) = delete;
		user_authentication& operator=(user_authentication const&) = delete;
		user_authentication(user_authentication&&) = delete;
		user_authentication& operator=(user_authentication&&) = delete;

		// Implementation Private Methods (*_impl suffix)
		void init_impl();
		void destroy_impl();
		void check_auto_login_impl();
		bool fetch_profile_impl();
		bool login_impl(const std::string username_param, const std::string password_param);
		void logout_impl();
		bool log_activity_impl(const std::string& action_param, const std::string& description_param);

		void clear_avatar_texture_impl();
		void trigger_avatar_download_impl();
		ID3D11ShaderResourceView* get_avatar_texture_impl();

		nlohmann::ordered_json api_get_impl(const std::string& endpoint);

		bool authorized_impl() const { return !session_token.empty(); }
		std::string get_token_impl() const { return session_token; }
		std::string get_message_impl() const { return message; }
		std::string get_fullname_impl() const { return fullname; }
		std::string get_username_impl() const { return std::string(username); }
		std::string get_role_impl() const { return role; }
		std::string get_avatar_url_impl() const { return avatar_url; }
		std::string get_user_agent_impl() const { return user_agent_provider::get_user_agent(); }
		int get_status_impl() const { return status; }
		ProductGrade owned_product_impl() const { return ownership; }
		std::string owned_product_info_impl(ProductGrade product_grade) const;
		std::string ownership_expiry_date_impl() const { return expired_date; }

	private:
		std::filesystem::path get_session_file_path() const;
		void save_session(const std::string& refresh_token);
		void clear_session();
		std::string extract_refresh_cookie(const cpr::Response& res) const;

	protected:
		std::string get_url_login() const;
		std::string get_url_logout() const;
		std::string get_url_refresh() const;
		std::string get_url_profile() const;

	private:
		int status{0};
		std::string role;
		std::string message;
		std::string session_token;
		std::string fullname;
		std::string expired_date;
		std::string avatar_url;
		std::string user_id;
		ProductGrade ownership{ProductGrade::GOLD};
		ExpiryDate expired{};

		ID3D11ShaderResourceView* avatar_srv = nullptr;
		std::vector<uint8_t> avatar_raw_bytes;
		std::atomic<bool> avatar_bytes_ready{false};
		std::atomic<bool> avatar_loading{false};
		bool avatar_loaded{false};
		std::mutex avatar_mutex;
	};
}