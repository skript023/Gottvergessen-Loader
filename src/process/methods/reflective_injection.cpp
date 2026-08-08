#include "process/methods/reflective_injection.hpp"
#include "logger.hpp"
#include <fstream>

#define DEREF(name) *(UINT_PTR*)(name)
#define DEREF_64(name) *(DWORD64*)(name)
#define DEREF_32(name) *(DWORD*)(name)
#define DEREF_16(name) *(WORD*)(name)
#define DEREF_8(name) *(BYTE*)(name)

namespace gottvergessen
{
	namespace
	{
		bool enable_debug_privilege()
		{
			HANDLE token = NULL;
			if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
			{
				TOKEN_PRIVILEGES priv{ 0 };
				priv.PrivilegeCount = 1;
				priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

				if (LookupPrivilegeValueA(NULL, SE_DEBUG_NAME, &priv.Privileges[0].Luid))
				{
					AdjustTokenPrivileges(token, FALSE, &priv, 0, NULL, NULL);
				}

				CloseHandle(token);
				return true;
			}
			return false;
		}

		bool read_file(const std::filesystem::path& path, std::vector<std::uint8_t>& output)
		{
			output.clear();

			std::ifstream file(path, std::ios::binary | std::ios::ate);
			if (!file)
				return false;

			const auto size = file.tellg();
			if (size <= 0)
				return false;

			output.resize(static_cast<std::size_t>(size));
			file.seekg(0, std::ios::beg);
			file.read(reinterpret_cast<char*>(output.data()), static_cast<std::streamsize>(output.size()));

			return file.good();
		}
	}

	// Port of Stephen Fewer's Rva2Offset (from LoadLibraryR.c)
	DWORD reflective_injection::rva_to_offset(DWORD rva, UINT_PTR base_address) const
	{
		const auto* dos_header = reinterpret_cast<const IMAGE_DOS_HEADER*>(base_address);
		const auto* nt_headers = reinterpret_cast<const IMAGE_NT_HEADERS*>(base_address + dos_header->e_lfanew);

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

	// Port of Stephen Fewer's GetReflectiveLoaderOffset (from LoadLibraryR.c)
	DWORD reflective_injection::get_reflective_loader_offset() const
	{
		if (m_image.empty())
			return 0;

		auto base_address = reinterpret_cast<UINT_PTR>(m_image.data());

		const auto* dos_header = reinterpret_cast<const IMAGE_DOS_HEADER*>(base_address);
		if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
			return 0;

		const auto* nt_headers = reinterpret_cast<const IMAGE_NT_HEADERS*>(base_address + dos_header->e_lfanew);
		if (nt_headers->Signature != IMAGE_NT_SIGNATURE)
			return 0;

		// Architecture check aligned with Stephen Fewer's LoadLibraryR
#ifdef _WIN64
		if (nt_headers->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
			return 0;
#else
		if (nt_headers->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
			return 0;
#endif

		const auto export_dir_rva = nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
		if (export_dir_rva == 0)
			return 0;

		auto export_dir_offset = rva_to_offset(export_dir_rva, base_address);
		if (export_dir_offset == 0)
			return 0;

		const auto* export_dir = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(base_address + export_dir_offset);

		auto name_array_offset = rva_to_offset(export_dir->AddressOfNames, base_address);
		auto address_array_offset = rva_to_offset(export_dir->AddressOfFunctions, base_address);
		auto ordinals_offset = rva_to_offset(export_dir->AddressOfNameOrdinals, base_address);

		if (!name_array_offset || !address_array_offset || !ordinals_offset)
			return 0;

		const auto* name_array = reinterpret_cast<const DWORD*>(base_address + name_array_offset);
		const auto* address_array = reinterpret_cast<const DWORD*>(base_address + address_array_offset);
		const auto* ordinals = reinterpret_cast<const WORD*>(base_address + ordinals_offset);

		DWORD counter = export_dir->NumberOfNames;
		for (DWORD i = 0; i < counter; i++)
		{
			auto name_offset = rva_to_offset(name_array[i], base_address);
			if (name_offset == 0) continue;

			const auto* name = reinterpret_cast<const char*>(base_address + name_offset);
			if (std::strstr(name, "ReflectiveLoader") != nullptr)
			{
				auto function_rva = address_array[ordinals[i]];
				return rva_to_offset(function_rva, base_address);
			}
		}

		return 0;
	}

	bool reflective_injection::load_image(const std::filesystem::path& dll_path)
	{
		m_image.clear();
		return read_file(dll_path, m_image);
	}

	bool reflective_injection::validate_image(const std::filesystem::path& dll_path) const
	{
		if (m_image.size() < sizeof(IMAGE_DOS_HEADER))
		{
			LOG(WARNING) << dll_path.filename().string() << ": file too small for DOS header.";
			return false;
		}

		const auto* dos_header = reinterpret_cast<const IMAGE_DOS_HEADER*>(m_image.data());
		if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
		{
			LOG(WARNING) << dll_path.filename().string() << ": invalid DOS signature.";
			return false;
		}

		if (dos_header->e_lfanew < 0 || static_cast<std::size_t>(dos_header->e_lfanew) > m_image.size() - sizeof(IMAGE_NT_HEADERS))
		{
			LOG(WARNING) << dll_path.filename().string() << ": invalid NT header offset.";
			return false;
		}

		const auto* nt_headers = reinterpret_cast<const IMAGE_NT_HEADERS*>(m_image.data() + dos_header->e_lfanew);
		if (nt_headers->Signature != IMAGE_NT_SIGNATURE)
		{
			LOG(WARNING) << dll_path.filename().string() << ": invalid NT signature.";
			return false;
		}

#ifdef _WIN64
		if (nt_headers->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64)
		{
			LOG(WARNING) << dll_path.filename().string() << ": PE architecture is not AMD64.";
			return false;
		}
#else
		if (nt_headers->FileHeader.Machine != IMAGE_FILE_MACHINE_I386)
		{
			LOG(WARNING) << dll_path.filename().string() << ": PE architecture is not x86.";
			return false;
		}
#endif

		return true;
	}

	// Implementation of Stephen Fewer's LoadRemoteLibraryR workflow
	bool reflective_injection::inject(const std::string& process_name, std::uint32_t pid, const std::filesystem::path& dll_path)
	{
		if (!std::filesystem::exists(dll_path))
		{
			LOG(WARNING) << dll_path.string() << " file doesn't exist.";
			return false;
		}

		if (pid == 0)
		{
			LOG(WARNING) << process_name << " is not open.";
			return false;
		}

		if (!load_image(dll_path))
		{
			LOG(WARNING) << "Failed to read PE image " << dll_path.filename().string();
			return false;
		}

		if (!validate_image(dll_path))
		{
			LOG(WARNING) << "Invalid PE image header for " << dll_path.filename().string();
			return false;
		}

		LOG(HACKER) << "PE image loaded successfully: " << dll_path.filename().string();

		// Step 1: Locate ReflectiveLoader export offset in raw PE image
		DWORD loader_offset = get_reflective_loader_offset();
		if (loader_offset == 0)
		{
			LOG(WARNING) << "Failed to find ReflectiveLoader export in " << dll_path.filename().string();
			return false;
		}

		LOG(HACKER) << "ReflectiveLoader offset: 0x" << std::uppercase << std::hex << loader_offset;

		// Step 2: Enable SeDebugPrivilege
		enable_debug_privilege();

		// Step 3: Open target process
		HANDLE process_handle = OpenProcess(
			PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
			FALSE,
			pid);

		if (process_handle == NULL)
		{
			// Fallback attempt with PROCESS_ALL_ACCESS
			process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		}

		if (process_handle == NULL)
		{
			LOG(WARNING) << "Failed to OpenProcess(). 0x" << std::uppercase << std::hex << GetLastError();
			return false;
		}

		auto cleanup = [&process_handle]() {
			if (process_handle)
			{
				CloseHandle(process_handle);
				process_handle = NULL;
			}
		};

		// Step 4: Allocate RWX memory in target process for the full DLL image
		LPVOID remote_base = VirtualAllocEx(
			process_handle,
			NULL,
			m_image.size(),
			MEM_RESERVE | MEM_COMMIT,
			PAGE_EXECUTE_READWRITE);

		if (remote_base == NULL)
		{
			LOG(WARNING) << "Failed to VirtualAllocEx. 0x" << std::uppercase << std::hex << GetLastError();
			cleanup();
			return false;
		}

		LOG(HACKER) << "Remote allocation at: 0x" << std::uppercase << std::hex << reinterpret_cast<DWORD64>(remote_base);

		// Step 5: Write raw DLL image buffer to remote process memory
		SIZE_T bytes_written = 0;
		if (!WriteProcessMemory(process_handle, remote_base, m_image.data(), m_image.size(), &bytes_written) || bytes_written == 0)
		{
			LOG(WARNING) << "Failed to WriteProcessMemory. 0x" << std::uppercase << std::hex << GetLastError();
			VirtualFreeEx(process_handle, remote_base, 0, MEM_RELEASE);
			cleanup();
			return false;
		}

		LOG(HACKER) << "Written " << std::dec << bytes_written << " bytes to target process.";

		// Step 6: Calculate remote ReflectiveLoader address
		auto remote_loader = reinterpret_cast<LPTHREAD_START_ROUTINE>(
			reinterpret_cast<UINT_PTR>(remote_base) + loader_offset);

		LOG(HACKER) << "Remote ReflectiveLoader at: 0x" << std::uppercase << std::hex << reinterpret_cast<DWORD64>(remote_loader);

		// Step 7: Create remote thread calling ReflectiveLoader
		DWORD thread_id = 0;
		HANDLE remote_thread = CreateRemoteThread(
			process_handle,
			NULL,
			1024 * 1024,
			remote_loader,
			NULL,
			0,
			&thread_id);

		if (remote_thread == NULL)
		{
			LOG(WARNING) << "Failed to CreateRemoteThread. 0x" << std::uppercase << std::hex << GetLastError();
			VirtualFreeEx(process_handle, remote_base, 0, MEM_RELEASE);
			cleanup();
			return false;
		}

		// Step 8: Wait for loader thread to complete initialization
		WaitForSingleObject(remote_thread, INFINITE);

		CloseHandle(remote_thread);
		cleanup();

		LOG(HACKER) << "Reflective injection completed for " << dll_path.filename().string();
		return true;
	}
}
