#pragma once
#include "common.hpp"

namespace gottvergessen
{
	class user_agent_provider
	{
	public:
		explicit user_agent_provider():
			m_logical_processor(std::thread::hardware_concurrency()),
			m_user_agent(build_user_agent())
		{}
		virtual ~user_agent_provider() = default;

		user_agent_provider(user_agent_provider const&) = delete;
		user_agent_provider& operator=(user_agent_provider const&) = delete;
		user_agent_provider(user_agent_provider&&) noexcept = delete;
		user_agent_provider& operator=(user_agent_provider&&) noexcept = delete;

		uint32_t get_thread_count() const { return m_logical_processor; }
		std::string get_user_agent() const { return m_user_agent; }
	private:
		std::string build_user_agent()
		{
			return std::format("Gottvergessen-Loader/1.0 (Windows; Threads: {})", m_logical_processor);
		}

		uint32_t m_logical_processor;
		std::string m_user_agent;
	};
}