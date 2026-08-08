#pragma once
#include "process/injection_method.hpp"

namespace gottvergessen
{
	class remote_thread_injection : public injection_method
	{
	public:
		remote_thread_injection() = default;
		~remote_thread_injection() override = default;

		bool inject(const std::string& process_name, std::uint32_t pid, const std::filesystem::path& dll_path) override;
	};
}
