#pragma once
#include <common.hpp>

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

		void send_to_server(std::string_view prefix, std::string_view message);
		void send_info(std::string_view message);
		void send_warning(std::string_view message);
		virtual bool is_enabled() const { return m_enable; }
		virtual void enable() { m_enable = true; }
		virtual void disable() { m_enable = false; }
	private:
		void send_to_database(std::string_view prefix, std::string_view message);
	private:
		bool m_enable = false;
	};
}