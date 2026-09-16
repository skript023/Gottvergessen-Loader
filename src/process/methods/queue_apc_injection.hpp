#pragma once

#include "process/injection_method.hpp"

namespace gottvergessen
{
	class queue_apc_injection final : public injection_method
	{
	public:
		bool inject(
		    const std::string& process_name,
		    std::uint32_t pid,
		    const std::filesystem::path& dll_path) override;
	};
}
