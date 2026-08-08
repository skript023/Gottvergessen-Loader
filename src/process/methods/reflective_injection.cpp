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

	// Port of Stephen Fewer's Rva2Offset
	// Converts a Relative Virtual Address to a file offset
	// by walking section headers.
	DWORD reflective_injection::rva_to_offset(
	    DWORD rva, UINT_PTR base_address) const
	{
		const auto* nt_headers = reinterpret_cast<const IMAGE_NT_HEADERS*>(
		    base_address + reinterpret_cast<const IMAGE_DOS_HEADER*>(base_address)->e_lfanew);

		const auto* section_header = reinterpret_cast<const IMAGE_SECTION_HEADER*>(
		    reinterpret_cast<UINT_PTR>(&nt_headers->OptionalHeader) + nt_headers->FileHeader.SizeOfOptionalHeader);

		if (rva < section_header[0].PointerToRawData)
			return rva;

		for (WORD i = 0; i < nt_headers->FileHeader.NumberOfSections; i++)
		{
			if (rva >= section_header[i].VirtualAddress && rva < (section_header[i].VirtualAddress + section_header[i].SizeOfRawData))
			{
				return rva - section_header[i].VirtualAddress + section_header[i].PointerToRawData;
			}
		}

		return 0;
	}

	// Port of Stephen Fewer's GetReflectiveLoaderOffset
	// Parses the PE export directory to locate the exported
	// "ReflectiveLoader" function and returns its file offset.
	DWORD reflective_injection::get_reflective_loader_offset() const
	{
		if (m_image.empty())
			return 0;

		auto base_address = reinterpret_cast<UINT_PTR>(m_image.data());

		const auto* nt_headers = reinterpret_cast<const IMAGE_NT_HEADERS*>(
		    base_address + reinterpret_cast<const IMAGE_DOS_HEADER*>(base_address)->e_lfanew);

		const auto export_dir_rva =
		    nt_headers->OptionalHeader
		        .DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]
		        .VirtualAddress;

		if (export_dir_rva == 0)
			return 0;

		auto export_dir_offset = rva_to_offset(export_dir_rva, base_address);

		const auto* export_dir = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(
		    base_address + export_dir_offset);

		auto name_array_offset =
		    rva_to_offset(export_dir->AddressOfNames, base_address);

		auto address_array_offset =
		    rva_to_offset(export_dir->AddressOfFunctions, base_address);

		auto ordinals_offset =
		    rva_to_offset(export_dir->AddressOfNameOrdinals, base_address);

		const auto* name_array =
		    reinterpret_cast<const DWORD*>(base_address + name_array_offset);

		const auto* address_array =
		    reinterpret_cast<const DWORD*>(base_address + address_array_offset);

		const auto* ordinals =
		    reinterpret_cast<const WORD*>(base_address + ordinals_offset);

		for (DWORD i = 0; i < export_dir->NumberOfNames; i++)
		{
			auto name_offset = rva_to_offset(name_array[i], base_address);
			const auto* name =
			    reinterpret_cast<const char*>(base_address + name_offset);

			if (std::strstr(name, "ReflectiveLoader") != nullptr)
			{
				auto function_rva = address_array[ordinals[i]];
				return rva_to_offset(function_rva, base_address);
			}
		}

		return 0;
	}

	// Workflow aligned with Stephen Fewer's LoadRemoteLibraryR:
	//   1. Read & validate PE
	//   2. Find ReflectiveLoader export offset
	//   3. OpenProcess
	//   4. VirtualAllocEx (RWX, size of DLL image)
	//   5. WriteProcessMemory (write full DLL image)
	//   6. CreateRemoteThread (entry = remote base + loader offset)
	//   7. Cleanup
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

		// Step 1: Find the ReflectiveLoader export offset
		auto loader_offset = get_reflective_loader_offset();
		if (loader_offset == 0)
		{
			LOG(WARNING)
			    << "Failed to find ReflectiveLoader export in "
			    << dll_path.filename().string();

			return false;
		}

		LOG(HACKER)
		    << "ReflectiveLoader offset: 0x"
		    << std::uppercase << std::hex << loader_offset;

		// Step 2: Open target process
		HANDLE process_handle = OpenProcess(
		    PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
		    FALSE,
		    pid);

		if (process_handle == NULL)
		{
			LOG(WARNING)
			    << "Failed to OpenProcess(). 0x"
			    << std::uppercase << std::hex << GetLastError();

			return false;
		}

		auto cleanup = [&process_handle]() {
			if (process_handle)
			{
				CloseHandle(process_handle);
				process_handle = NULL;
			}
		};

		// Step 3: Allocate RWX memory in target process for the DLL image
		LPVOID remote_base = VirtualAllocEx(
		    process_handle,
		    NULL,
		    m_image.size(),
		    MEM_RESERVE | MEM_COMMIT,
		    PAGE_EXECUTE_READWRITE);

		if (remote_base == NULL)
		{
			LOG(WARNING)
			    << "Failed to VirtualAllocEx. 0x"
			    << std::uppercase << std::hex << GetLastError();

			cleanup();
			return false;
		}

		LOG(HACKER)
		    << "Remote allocation at: 0x"
		    << std::uppercase << std::hex
		    << reinterpret_cast<DWORD64>(remote_base);

		// Step 4: Write the DLL image into target process memory
		SIZE_T bytes_written = 0;
		if (!WriteProcessMemory(
		        process_handle,
		        remote_base,
		        m_image.data(),
		        m_image.size(),
		        &bytes_written))
		{
			LOG(WARNING)
			    << "Failed to WriteProcessMemory. 0x"
			    << std::uppercase << std::hex << GetLastError();

			VirtualFreeEx(process_handle, remote_base, 0, MEM_RELEASE);
			cleanup();
			return false;
		}

		LOG(HACKER)
		    << "Written " << std::dec << bytes_written
		    << " bytes to target process.";

		// Step 5: Calculate remote ReflectiveLoader address and create remote thread
		auto remote_loader = reinterpret_cast<LPTHREAD_START_ROUTINE>(
		    reinterpret_cast<UINT_PTR>(remote_base) + loader_offset);

		LOG(HACKER)
		    << "Remote ReflectiveLoader at: 0x"
		    << std::uppercase << std::hex
		    << reinterpret_cast<DWORD64>(remote_loader);

		HANDLE remote_thread = CreateRemoteThread(
		    process_handle,
		    NULL,
		    1024 * 1024,
		    remote_loader,
		    NULL,
		    NULL,
		    NULL);

		if (remote_thread == NULL)
		{
			LOG(WARNING)
			    << "Failed to CreateRemoteThread. 0x"
			    << std::uppercase << std::hex << GetLastError();

			VirtualFreeEx(process_handle, remote_base, 0, MEM_RELEASE);
			cleanup();
			return false;
		}

		// Step 6: Wait for the loader to finish and cleanup
		WaitForSingleObject(remote_thread, INFINITE);

		CloseHandle(remote_thread);
		cleanup();

		LOG(HACKER)
		    << "Reflective injection completed for "
		    << dll_path.filename().string();

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
