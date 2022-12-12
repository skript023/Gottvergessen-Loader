#include <thread_pool.hpp>
#include <server_monitor.hpp>
#include <api/url_encryption.hpp>
#include <api/http_request.hpp>

namespace gottvergessen
{
	void server_monitor::send_to_server(std::string_view prefix, std::string_view message)
	{
		raw_to_console(prefix, message);
	}

	void server_monitor::send_info(std::string_view message)
	{
		this->send_to_server("INFO", message);
	}

	void server_monitor::send_warning(std::string_view message)
	{
		this->send_to_server("WARNING", message);
	}

	void server_monitor::raw_to_console(std::string_view prefix, std::string_view message)
	{
		nlohmann::ordered_json body = {
			{ xorstr("prefix"), prefix },
			{ xorstr("message"), message }
		};

		try
		{
			if (is_enabled())
			{
				g_thread_pool->add_job([body]
				{
					http::Request req("http://localhost:8000/api/v1/logging");
					http::Response res = req.send("POST", body.dump(), { xorstr("Content-Type: application/json") });
				});
			}
		}
		catch (const std::exception&)
		{

		}
	}
}