#pragma once
#include "process/injection_method.hpp"

namespace gottvergessen
{
	class thread_hijack_injection : public injection_method
	{
	public:
		thread_hijack_injection() = default;
		~thread_hijack_injection() override = default;

		bool inject(const std::string& process_name, std::uint32_t pid, const std::filesystem::path& dll_path) override;
	};
}
