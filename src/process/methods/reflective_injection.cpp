#include "process/methods/reflective_injection.hpp"

namespace gottvergessen
{

	namespace
	{
		bool is_valid_dos_header(std::vector<std::uint8_t> const& image)
		{
			if (image.size() < sizeof(IMAGE_DOS_HEADER))
				return false;

			const auto* dos_header =
			    reinterpret_cast<const IMAGE_DOS_HEADER*>(image.data());

			return dos_header->e_magic == IMAGE_DOS_SIGNATURE;
		}

		bool is_valid_nt_header(
		    const std::vector<std::uint8_t>& image)
		{
			if (!is_valid_dos_header(image))
				return false;

			const auto* dos_header =
			    reinterpret_cast<const IMAGE_DOS_HEADER*>(image.data());

			if (dos_header->e_lfanew < 0)
				return false;

			const auto nt_offset =
			    static_cast<std::size_t>(dos_header->e_lfanew);

			if (nt_offset > image.size() - sizeof(DWORD))
			{
				return false;
			}

			const auto* signature =
			    reinterpret_cast<const DWORD*>(
			        image.data() + nt_offset);

			return *signature == IMAGE_NT_SIGNATURE;
		}

		bool read_file(std::filesystem::path const& path, std::vector<std::uint8_t>& output)
		{
			std::ifstream file(
			    path,
			    std::ios::binary | std::ios::ate);

			if (!file)
				return false;

			const auto size = file.tellg();

			if (size <= 0)
				return false;

			if (static_cast<std::uintmax_t>(size) > std::numeric_limits<std::size_t>::max())
			{
				return false;
			}

			output.resize(
			    static_cast<std::size_t>(size));

			file.seekg(0, std::ios::beg);

			file.read(
			    reinterpret_cast<char*>(output.data()),
			    static_cast<std::streamsize>(output.size()));

			return file.good();
		}

	}

	bool reflective_injection::inject(std::string const& process_name, std::uint32_t pid, std::filesystem::path const& dll_path)
	{
		if (!std::filesystem::exists(dll_path))
		{
			LOG(WARNING)
			    << dll_path.string()
			    << " file doesn't exist.";

			return false;
		}

		if (pid == 0)
		{
			LOG(WARNING)
			    << process_name
			    << " is not open.";

			return false;
		}

		if (!load_image(dll_path))
		{
			LOG(WARNING)
			    << "Failed to load PE image.";

			return false;
		}

		if (!validate_image(dll_path))
		{
			LOG(WARNING)
			    << "Invalid PE image.";

			return false;
		}

		LOG(HACKER)
		    << "PE image loaded successfully: "
		    << dll_path.filename().string();

		/*
     * The cross-process reflective mapping stage is intentionally
     * not implemented here.
     *
     * At this point m_image contains the complete DLL file image
     * and can safely be inspected by the PE parser.
     */

		return true;
	}

	bool reflective_injection::load_image(
	    const std::filesystem::path& dll_path)
	{
		m_image.clear();

		if (!read_file(dll_path, m_image))
		{
			LOG(WARNING)
			    << "Failed to read "
			    << dll_path.string();

			return false;
		}

		return !m_image.empty();
	}

	bool reflective_injection::validate_image(
	    const std::filesystem::path& dll_path) const
	{
		if (m_image.empty())
			return false;

		if (!is_valid_dos_header(m_image))
		{
			LOG(WARNING)
			    << dll_path.filename().string()
			    << ": invalid DOS header.";

			return false;
		}

		if (!is_valid_nt_header(m_image))
		{
			LOG(WARNING)
			    << dll_path.filename().string()
			    << ": invalid NT header.";

			return false;
		}

		const auto* dos_header =
		    reinterpret_cast<const IMAGE_DOS_HEADER*>(
		        m_image.data());

		const auto nt_offset =
		    static_cast<std::size_t>(
		        dos_header->e_lfanew);

#ifdef _WIN64

		if (nt_offset + sizeof(IMAGE_NT_HEADERS64) > m_image.size())
		{
			return false;
		}

		const auto* nt_header =
		    reinterpret_cast<const IMAGE_NT_HEADERS64*>(
		        m_image.data() + nt_offset);

		if (nt_header->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64)
		{
			LOG(WARNING)
			    << "PE architecture is not AMD64.";

			return false;
		}

		if (nt_header->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
		{
			LOG(WARNING)
			    << "Invalid PE32+ optional header.";

			return false;
		}

#else

		if (nt_offset + sizeof(IMAGE_NT_HEADERS32) > m_image.size())
		{
			return false;
		}

		const auto* nt_header =
		    reinterpret_cast<const IMAGE_NT_HEADERS32*>(
		        m_image.data() + nt_offset);

		if (nt_header->FileHeader.Machine != IMAGE_FILE_MACHINE_I386)
		{
			LOG(WARNING)
			    << "PE architecture is not x86.";

			return false;
		}

		if (nt_header->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
		{
			LOG(WARNING)
			    << "Invalid PE32 optional header.";

			return false;
		}

#endif

		return true;
	}

}