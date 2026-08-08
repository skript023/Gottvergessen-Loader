#include "process/methods/reflective_injection.hpp"

namespace gottvergessen
{

	namespace
	{
		template<typename T>
		bool range_is_valid(
		    std::size_t offset,
		    std::size_t count,
		    std::size_t size) noexcept
		{
			if (offset > size)
				return false;

			if (count > size - offset)
				return false;

			return true;
		}

		bool add_overflow(
		    std::size_t lhs,
		    std::size_t rhs,
		    std::size_t& result) noexcept
		{
			if (rhs > std::numeric_limits<std::size_t>::max() - lhs)
				return true;

			result = lhs + rhs;
			return false;
		}

		bool is_valid_dos_header(
		    const std::vector<std::uint8_t>& image) noexcept
		{
			if (!range_is_valid<IMAGE_DOS_HEADER>(
			        0,
			        sizeof(IMAGE_DOS_HEADER),
			        image.size()))
			{
				return false;
			}

			const auto* dos_header =
			    reinterpret_cast<const IMAGE_DOS_HEADER*>(image.data());

			return dos_header->e_magic == IMAGE_DOS_SIGNATURE;
		}

		bool get_nt_headers(
		    const std::vector<std::uint8_t>& image,
		    const IMAGE_NT_HEADERS64*& nt_headers) noexcept
		{
			nt_headers = nullptr;

			if (!is_valid_dos_header(image))
				return false;

			const auto* dos_header =
			    reinterpret_cast<const IMAGE_DOS_HEADER*>(image.data());

			if (dos_header->e_lfanew < 0)
				return false;

			const auto nt_offset =
			    static_cast<std::size_t>(dos_header->e_lfanew);

			if (!range_is_valid<DWORD>(
			        nt_offset,
			        sizeof(DWORD),
			        image.size()))
			{
				return false;
			}

			const auto* signature =
			    reinterpret_cast<const DWORD*>(
			        image.data() + nt_offset);

			if (*signature != IMAGE_NT_SIGNATURE)
				return false;

			if (!range_is_valid<IMAGE_NT_HEADERS64>(
			        nt_offset,
			        sizeof(IMAGE_NT_HEADERS64),
			        image.size()))
			{
				return false;
			}

			nt_headers =
			    reinterpret_cast<const IMAGE_NT_HEADERS64*>(
			        image.data() + nt_offset);

			return true;
		}

		bool get_section_headers(
		    const std::vector<std::uint8_t>& image,
		    const IMAGE_NT_HEADERS64* nt_headers,
		    const IMAGE_SECTION_HEADER*& sections) noexcept
		{
			sections = nullptr;

			if (nt_headers == nullptr)
				return false;

			const auto* nt_address =
			    reinterpret_cast<const std::uint8_t*>(nt_headers);

			const auto nt_offset =
			    static_cast<std::size_t>(
			        nt_address - image.data());

			const auto optional_header_offset =
			    nt_offset + offsetof(IMAGE_NT_HEADERS64, OptionalHeader);

			if (!range_is_valid<IMAGE_OPTIONAL_HEADER64>(
			        optional_header_offset,
			        nt_headers->FileHeader.SizeOfOptionalHeader,
			        image.size()))
			{
				return false;
			}

			const auto section_table_offset =
			    optional_header_offset + nt_headers->FileHeader.SizeOfOptionalHeader;

			const auto section_count =
			    static_cast<std::size_t>(
			        nt_headers->FileHeader.NumberOfSections);

			std::size_t section_table_size = 0;

			if (section_count > std::numeric_limits<std::size_t>::max() / sizeof(IMAGE_SECTION_HEADER))
			{
				return false;
			}

			section_table_size =
			    section_count * sizeof(IMAGE_SECTION_HEADER);

			if (!range_is_valid<IMAGE_SECTION_HEADER>(
			        section_table_offset,
			        section_table_size,
			        image.size()))
			{
				return false;
			}

			sections =
			    reinterpret_cast<const IMAGE_SECTION_HEADER*>(
			        image.data() + section_table_offset);

			return true;
		}

		bool read_file(
		    const std::filesystem::path& path,
		    std::vector<std::uint8_t>& output)
		{
			output.clear();

			std::ifstream file(
			    path,
			    std::ios::binary | std::ios::ate);

			if (!file)
				return false;

			const auto end_position = file.tellg();

			if (end_position <= 0)
				return false;

			const auto size =
			    static_cast<std::uintmax_t>(end_position);

			if (size > static_cast<std::uintmax_t>(
			        std::numeric_limits<std::size_t>::max()))
			{
				return false;
			}

			const auto file_size =
			    static_cast<std::size_t>(size);

			output.resize(file_size);

			file.seekg(0, std::ios::beg);

			if (!file)
			{
				output.clear();
				return false;
			}

			if (file_size != 0)
			{
				file.read(
				    reinterpret_cast<char*>(output.data()),
				    static_cast<std::streamsize>(file_size));

				if (file.gcount() != static_cast<std::streamsize>(file_size))
				{
					output.clear();
					return false;
				}
			}

			return true;
		}

		bool validate_sections(
		    const std::vector<std::uint8_t>& image,
		    const IMAGE_NT_HEADERS64* nt_headers)
		{
			const IMAGE_SECTION_HEADER* sections = nullptr;

			if (!get_section_headers(
			        image,
			        nt_headers,
			        sections))
			{
				return false;
			}

			const auto section_count =
			    static_cast<std::size_t>(
			        nt_headers->FileHeader.NumberOfSections);

			if (section_count == 0)
				return false;

			const auto size_of_image =
			    static_cast<std::size_t>(
			        nt_headers->OptionalHeader.SizeOfImage);

			const auto size_of_headers =
			    static_cast<std::size_t>(
			        nt_headers->OptionalHeader.SizeOfHeaders);

			if (size_of_image == 0)
				return false;

			if (size_of_headers == 0 || size_of_headers > image.size() || size_of_headers > size_of_image)
			{
				return false;
			}

			for (std::size_t i = 0; i < section_count; ++i)
			{
				const auto& section = sections[i];

				const auto raw_offset =
				    static_cast<std::size_t>(
				        section.PointerToRawData);

				const auto raw_size =
				    static_cast<std::size_t>(
				        section.SizeOfRawData);

				if (raw_size != 0)
				{
					if (!range_is_valid<std::uint8_t>(
					        raw_offset,
					        raw_size,
					        image.size()))
					{
						return false;
					}
				}

				const auto virtual_address =
				    static_cast<std::size_t>(
				        section.VirtualAddress);

				const auto virtual_size =
				    static_cast<std::size_t>(
				        section.Misc.VirtualSize);

				const auto mapped_size =
				    std::max(
				        virtual_size,
				        raw_size);

				if (mapped_size == 0)
					continue;

				if (virtual_address > size_of_image)
					return false;

				if (mapped_size > size_of_image - virtual_address)
				{
					return false;
				}
			}

			return true;
		}
	}

	DWORD reflective_injection::rva_to_offset(
	    DWORD rva,
	    UINT_PTR /*base_address*/) const
	{
		if (m_image.empty())
			return 0;

		const IMAGE_NT_HEADERS64* nt_headers = nullptr;

		if (!get_nt_headers(m_image, nt_headers))
			return 0;

		const IMAGE_SECTION_HEADER* sections = nullptr;

		if (!get_section_headers(
		        m_image,
		        nt_headers,
		        sections))
		{
			return 0;
		}

		/*
         * RVA inside PE headers.
         */
		if (rva < nt_headers->OptionalHeader.SizeOfHeaders)
		{
			if (static_cast<std::size_t>(rva) < m_image.size())
				return rva;

			return 0;
		}

		const auto section_count =
		    static_cast<std::size_t>(
		        nt_headers->FileHeader.NumberOfSections);

		const auto rva_value =
		    static_cast<std::size_t>(rva);

		for (std::size_t i = 0; i < section_count; ++i)
		{
			const auto& section = sections[i];

			const auto virtual_address =
			    static_cast<std::size_t>(
			        section.VirtualAddress);

			const auto virtual_size =
			    static_cast<std::size_t>(
			        section.Misc.VirtualSize);

			const auto raw_size =
			    static_cast<std::size_t>(
			        section.SizeOfRawData);

			const auto section_size =
			    std::max(
			        virtual_size,
			        raw_size);

			if (section_size == 0)
				continue;

			if (rva_value < virtual_address)
				continue;

			const auto relative =
			    rva_value - virtual_address;

			if (relative >= section_size)
				continue;

			/*
             * A virtual-only portion of a section has no
             * corresponding bytes in the file.
             */
			if (relative >= raw_size)
				return 0;

			const auto raw_offset =
			    static_cast<std::size_t>(
			        section.PointerToRawData);

			std::size_t file_offset = 0;

			if (add_overflow(
			        raw_offset,
			        relative,
			        file_offset))
			{
				return 0;
			}

			if (file_offset >= m_image.size())
				return 0;

			return static_cast<DWORD>(file_offset);
		}

		return 0;
	}

	DWORD reflective_injection::get_reflective_loader_offset() const
	{
		if (m_image.empty())
			return 0;

		const IMAGE_NT_HEADERS64* nt_headers = nullptr;

		if (!get_nt_headers(m_image, nt_headers))
			return 0;

		if (nt_headers->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_EXPORT)
		{
			return 0;
		}

		const auto& export_entry =
		    nt_headers->OptionalHeader
		        .DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];

		if (export_entry.VirtualAddress == 0 || export_entry.Size < sizeof(IMAGE_EXPORT_DIRECTORY))
		{
			return 0;
		}

		const auto export_offset =
		    rva_to_offset(
		        export_entry.VirtualAddress,
		        reinterpret_cast<UINT_PTR>(m_image.data()));

		if (export_offset == 0)
			return 0;

		if (!range_is_valid<IMAGE_EXPORT_DIRECTORY>(
		        export_offset,
		        sizeof(IMAGE_EXPORT_DIRECTORY),
		        m_image.size()))
		{
			return 0;
		}

		const auto* export_directory =
		    reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(
		        m_image.data() + export_offset);

		if (export_directory->NumberOfNames == 0 || export_directory->NumberOfFunctions == 0)
		{
			return 0;
		}

		const auto names_offset =
		    rva_to_offset(
		        export_directory->AddressOfNames,
		        reinterpret_cast<UINT_PTR>(m_image.data()));

		const auto functions_offset =
		    rva_to_offset(
		        export_directory->AddressOfFunctions,
		        reinterpret_cast<UINT_PTR>(m_image.data()));

		const auto ordinals_offset =
		    rva_to_offset(
		        export_directory->AddressOfNameOrdinals,
		        reinterpret_cast<UINT_PTR>(m_image.data()));

		if (names_offset == 0 || functions_offset == 0 || ordinals_offset == 0)
		{
			return 0;
		}

		const auto name_count =
		    static_cast<std::size_t>(
		        export_directory->NumberOfNames);

		const auto function_count =
		    static_cast<std::size_t>(
		        export_directory->NumberOfFunctions);

		if (!range_is_valid<DWORD>(
		        names_offset,
		        name_count * sizeof(DWORD),
		        m_image.size()))
		{
			return 0;
		}

		if (!range_is_valid<DWORD>(
		        functions_offset,
		        function_count * sizeof(DWORD),
		        m_image.size()))
		{
			return 0;
		}

		if (!range_is_valid<WORD>(
		        ordinals_offset,
		        name_count * sizeof(WORD),
		        m_image.size()))
		{
			return 0;
		}

		const auto* name_array =
		    reinterpret_cast<const DWORD*>(
		        m_image.data() + names_offset);

		const auto* function_array =
		    reinterpret_cast<const DWORD*>(
		        m_image.data() + functions_offset);

		const auto* ordinal_array =
		    reinterpret_cast<const WORD*>(
		        m_image.data() + ordinals_offset);

		constexpr std::string_view target_name =
		    "ReflectiveLoader";

		for (std::size_t i = 0; i < name_count; ++i)
		{
			const auto name_offset =
			    rva_to_offset(
			        name_array[i],
			        reinterpret_cast<UINT_PTR>(m_image.data()));

			if (name_offset == 0 || name_offset >= m_image.size())
			{
				continue;
			}

			const auto* name =
			    reinterpret_cast<const char*>(
			        m_image.data() + name_offset);

			/*
             * Make sure the string is actually terminated
             * inside the input buffer.
             */
			const auto remaining =
			    m_image.size() - name_offset;

			const auto* terminator =
			    static_cast<const char*>(
			        std::memchr(
			            name,
			            '\0',
			            remaining));

			if (terminator == nullptr)
				continue;

			const std::string_view current_name(
			    name,
			    static_cast<std::size_t>(
			        terminator - name));

			if (current_name != target_name)
				continue;

			const auto ordinal =
			    static_cast<std::size_t>(
			        ordinal_array[i]);

			if (ordinal >= function_count)
				return 0;

			const auto function_rva =
			    function_array[ordinal];

			/*
             * Reject an RVA that does not correspond to
             * bytes in the PE file.
             */
			const auto function_offset =
			    rva_to_offset(
			        function_rva,
			        reinterpret_cast<UINT_PTR>(m_image.data()));

			if (function_offset == 0)
				return 0;

			return function_offset;
		}

		return 0;
	}

	bool reflective_injection::inject(
	    std::string const& process_name,
	    std::uint32_t pid,
	    std::filesystem::path const& dll_path)
	{
		/*
         * Diagnostic-only path.
         *
         * The PE is fully loaded and validated here, but this
         * function intentionally does not perform remote-process
         * allocation or remote execution.
         */

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

		LOG(INFO)
		    << "PE image loaded and validated successfully: "
		    << dll_path.filename().string();

		const auto loader_offset =
		    get_reflective_loader_offset();

		if (loader_offset == 0)
		{
			LOG(WARNING)
			    << "ReflectiveLoader export was not found or "
			       "does not map to valid file data.";

			return false;
		}

		LOG(HACKER)
		    << "ReflectiveLoader file offset: 0x"
		    << std::uppercase
		    << std::hex
		    << loader_offset;

		// Step 2: Open target process
		HANDLE process_handle = OpenProcess(
		    PROCESS_ALL_ACCESS,
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

		const IMAGE_NT_HEADERS64* nt_headers = nullptr;

		if (!get_nt_headers(m_image, nt_headers))
		{
			LOG(WARNING)
			    << dll_path.filename().string()
			    << ": invalid NT header.";

			return false;
		}

#ifdef _WIN64

		if (nt_headers->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64)
		{
			LOG(WARNING)
			    << dll_path.filename().string()
			    << ": PE architecture is not AMD64.";

			return false;
		}

		if (nt_headers->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
		{
			LOG(WARNING)
			    << dll_path.filename().string()
			    << ": invalid PE32+ optional header.";

			return false;
		}

#else

		/*
         * This implementation intentionally targets the same
         * architecture as the original build.
         */
		LOG(WARNING)
		    << dll_path.filename().string()
		    << ": x86 build is not supported by this validator.";

		return false;

#endif

		if (nt_headers->FileHeader.NumberOfSections == 0)
		{
			LOG(WARNING)
			    << dll_path.filename().string()
			    << ": PE contains no sections.";

			return false;
		}

		if (nt_headers->FileHeader.SizeOfOptionalHeader < sizeof(IMAGE_OPTIONAL_HEADER64))
		{
			LOG(WARNING)
			    << dll_path.filename().string()
			    << ": optional header is truncated.";

			return false;
		}

		const auto& optional_header =
		    nt_headers->OptionalHeader;

		if (optional_header.SizeOfImage == 0)
		{
			LOG(WARNING)
			    << dll_path.filename().string()
			    << ": SizeOfImage is zero.";

			return false;
		}

		if (optional_header.SizeOfHeaders == 0 || optional_header.SizeOfHeaders > m_image.size())
		{
			LOG(WARNING)
			    << dll_path.filename().string()
			    << ": invalid SizeOfHeaders.";

			return false;
		}

		if (optional_header.SizeOfHeaders > optional_header.SizeOfImage)
		{
			LOG(WARNING)
			    << dll_path.filename().string()
			    << ": SizeOfHeaders exceeds SizeOfImage.";

			return false;
		}

		if (optional_header.SectionAlignment == 0 || optional_header.FileAlignment == 0)
		{
			LOG(WARNING)
			    << dll_path.filename().string()
			    << ": invalid PE alignment.";

			return false;
		}

		if (!validate_sections(
		        m_image,
		        nt_headers))
		{
			LOG(WARNING)
			    << dll_path.filename().string()
			    << ": invalid section layout.";

			return false;
		}

		/*
         * Entry point must be inside the declared image unless
         * the image explicitly has no entry point.
         */
		if (optional_header.AddressOfEntryPoint != 0 && optional_header.AddressOfEntryPoint >= optional_header.SizeOfImage)
		{
			LOG(WARNING)
			    << dll_path.filename().string()
			    << ": entry point is outside SizeOfImage.";

			return false;
		}

		/*
         * Validate data directories that are actually present.
         */
		const auto directory_count =
		    std::min<std::size_t>(
		        optional_header.NumberOfRvaAndSizes,
		        IMAGE_NUMBEROF_DIRECTORY_ENTRIES);

		for (std::size_t i = 0; i < directory_count; ++i)
		{
			const auto& directory =
			    optional_header.DataDirectory[i];

			if (directory.VirtualAddress == 0 || directory.Size == 0)
			{
				continue;
			}

			const auto directory_start =
			    static_cast<std::size_t>(
			        directory.VirtualAddress);

			const auto directory_size =
			    static_cast<std::size_t>(
			        directory.Size);

			if (directory_start >= optional_header.SizeOfImage)
			{
				LOG(WARNING)
				    << dll_path.filename().string()
				    << ": data directory "
				    << i
				    << " starts outside image.";

				return false;
			}

			if (directory_size > optional_header.SizeOfImage - directory_start)
			{
				LOG(WARNING)
				    << dll_path.filename().string()
				    << ": data directory "
				    << i
				    << " exceeds image.";

				return false;
			}
		}

		return true;
	}

}
