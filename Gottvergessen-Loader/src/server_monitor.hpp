#pragma once
#include <common.hpp>

namespace gottvergessen
{
	class server_monitor
	{
	public:
		void send_to_server(std::string_view prefix, std::string_view message);
		void send_info(std::string_view message);
		void send_warning(std::string_view message);
		virtual bool is_enabled() const { return m_enable; }
		virtual void enable() { m_enable = true; }
		virtual void disable() { m_enable = false; }
	private:
		void raw_to_console(std::string_view prefix, std::string_view message);
	private:
		bool m_enable = false;
	};
}