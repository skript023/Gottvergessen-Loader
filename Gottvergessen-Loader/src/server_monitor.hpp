#pragma once
#include <common.hpp>

struct server_format
{
	std::string m_message;
	std::string m_file;
	std::string m_line;
};

namespace gottvergessen
{
	class server_monitor
	{
	public:
		explicit server_monitor() = default;
		virtual ~server_monitor() noexcept = default;

		server_monitor(server_monitor const& that) = delete;
		server_monitor& operator=(server_monitor const& that) = delete;
		server_monitor(server_monitor&& that) = delete;
		server_monitor& operator=(server_monitor&& that) = delete;

		void send_to_server(std::string_view prefix, std::string_view message, std::string_view file, std::string_view line);
		void send_server(std::string_view message, std::string_view file, std::string_view line);
		void send_fatal(std::string_view message, std::string_view file, std::string_view line);
		void send_warning(std::string_view message, std::string_view file, std::string_view line);
		virtual bool is_enabled() const { return m_enable; }
		virtual void enable() { m_enable = true; }
		virtual void disable() { m_enable = false; }
	private:
		void send_to_database(std::string_view prefix, std::string_view message, std::string_view file, std::string_view line);
	private:
		bool m_enable = false;
	};
}