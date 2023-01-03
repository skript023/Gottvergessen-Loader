#include <thread_pool.hpp>
#include <server_monitor.hpp>
#include <api/http_request.hpp>
#include <api/url_encryption.hpp>
#include <api/user/user_authentication.hpp>

namespace gottvergessen
{
	void server_monitor::send_to_server(std::string_view prefix, std::string_view message)
	{
		send_to_database(prefix, message);
	}

	void server_monitor::send_info(std::string_view message)
	{
		this->send_to_server("INFO", message);
	}

	void server_monitor::send_warning(std::string_view message)
	{
		this->send_to_server("WARNING", message);
	}

	void server_monitor::send_to_database(std::string_view prefix, std::string_view message)
	{
		if (this->is_enabled())
		{
			try
			{
				auto fullname = g_user_authentication->authorized() ? g_user_authentication->get_fullname() : xorstr("Prelogin");
				
				nlohmann::ordered_json json = {
					{ xorstr("prefix"), prefix },
					{ xorstr("message"), message },
					{ xorstr("owner"), fullname }
				};

				g_thread_pool->add_job([=]
				{
					cpr::Url uri = xorstr("https://gottvergessen.000webhostapp.com/api/v1/logging");
					cpr::Body body = json.dump();
					cpr::Header header = { {xorstr("Content-Type"), xorstr("application/json")} };
					auto res = cpr::Post(uri, body, header);
				});
			}
			catch (const std::exception&)
			{
				LOG(WARNING) << "Failed interact with server";
			}
		}
	}
}