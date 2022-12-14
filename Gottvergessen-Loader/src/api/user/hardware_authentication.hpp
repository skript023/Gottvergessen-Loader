#pragma once
#include "common.hpp"
#include "process/subprocess.hpp"

namespace gottvergessen
{
	struct hardware_information
	{
		const std::string m_computer_uuid;
		const std::string m_computer_name;
		const uint32_t m_logical_processor;
		const std::string m_core_processor;
	};

	class hardware_authentication
	{
	public:
		explicit hardware_authentication():
			m_computer_uuid(initialize_computer_uuid()),
			m_computer_name(initialize_computer_name()),
			m_logical_processor(std::thread::hardware_concurrency())
		{}
		virtual ~hardware_authentication() = default;

		hardware_authentication(hardware_authentication const&) = delete;
		hardware_authentication& operator=(hardware_authentication const&) = delete;
		hardware_authentication(hardware_authentication&&) noexcept = delete;
		hardware_authentication& operator=(hardware_authentication&&) noexcept = delete;

		std::string initialize_computer_uuid()
		{
			subprocess::OutBuffer unique_id = subprocess::check_output("wmic csproduct get uuid");
			std::string data(unique_id.buf.begin(), unique_id.buf.end());
			data = std::regex_replace(data, std::regex(R"(\UUID)"), "");

			data.erase(std::remove_if(data.begin(), data.end(), [](unsigned char x) { return std::isspace(x); }), data.end());

			return data;
		}

		std::string initialize_computer_name()
		{
			auto subprocess = subprocess::check_output("hostname");
			std::string result(subprocess.buf.begin(), subprocess.buf.end());
			return result;
		}

		uint32_t get_thread_count() const { return m_logical_processor; }
		std::string get_bios() const { return m_computer_uuid; }
		std::string get_computer_name() const { return m_computer_name; }
	private:
		std::string m_computer_uuid;
		std::string m_computer_name;
		uint32_t m_logical_processor;
	};
}