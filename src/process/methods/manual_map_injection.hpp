#pragma once
#include "process/injection_method.hpp"

namespace gottvergessen
{
	class manual_map_injection : public injection_method
	{
	public:
		manual_map_injection() = default;
		~manual_map_injection() override = default;

		bool inject(const std::string& process_name, std::uint32_t pid, const std::filesystem::path& dll_path) override;
	};
}
