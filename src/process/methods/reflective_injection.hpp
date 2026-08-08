#pragma once

#include "process/injection_method.hpp"

namespace gottvergessen
{

	class reflective_injection final : public injection_method
	{
	public:
		reflective_injection() = default;
		~reflective_injection() override = default;

		bool inject(
		    const std::string& process_name,
		    std::uint32_t pid,
		    const std::filesystem::path& dll_path) override;

	private:
		bool validate_image(
		    const std::filesystem::path& dll_path) const;

		bool load_image(
		    const std::filesystem::path& dll_path);

	private:
		std::vector<std::uint8_t> m_image;
	};

}