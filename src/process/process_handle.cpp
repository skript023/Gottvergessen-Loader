#include "process/process_handle.hpp"
#include "logger.hpp"

#include <TlHelp32.h>
#include <algorithm>
#include <cwchar>
#include <vector>

namespace gottvergessen
{
	namespace
	{
		constexpr LONG status_info_length_mismatch = static_cast<LONG>(0xC0000004L);
		constexpr ULONG system_extended_handle_information = 64;

		struct system_handle_entry
		{
			void* object;
			ULONG_PTR owner_pid;
			ULONG_PTR handle_value;
			ULONG granted_access;
			USHORT creator_back_trace_index;
			USHORT object_type_index;
			ULONG handle_attributes;
			ULONG reserved;
		};

		struct system_handle_information
		{
			ULONG_PTR handle_count;
			ULONG_PTR reserved;
			system_handle_entry handles[1];
		};

		using nt_query_system_information =
		    LONG(NTAPI*)(ULONG, void*, ULONG, ULONG*);

		HANDLE duplicate_existing_process_handle(
		    std::uint32_t target_pid,
		    DWORD desired_access)
		{
			auto ntdll = GetModuleHandleW(L"ntdll.dll");
			auto query = reinterpret_cast<nt_query_system_information>(
			    GetProcAddress(ntdll, "NtQuerySystemInformation"));
			if (!query)
				return nullptr;

			std::vector<std::uint8_t> buffer(1U << 20);
			ULONG required = 0;
			LONG status = 0;
			for (;;)
			{
				status = query(
				    system_extended_handle_information,
				    buffer.data(),
				    static_cast<ULONG>(buffer.size()),
				    &required);
				if (status != status_info_length_mismatch)
					break;
				buffer.resize(std::max<std::size_t>(
				    buffer.size() * 2,
				    static_cast<std::size_t>(required) + 0x10000));
			}
			if (status < 0)
				return nullptr;

			auto* info =
			    reinterpret_cast<const system_handle_information*>(buffer.data());
			DWORD current_owner_pid = 0;
			HANDLE current_owner = nullptr;

			auto close_owner = [&]() {
				if (current_owner)
					CloseHandle(current_owner);
				current_owner = nullptr;
				current_owner_pid = 0;
			};

			for (ULONG_PTR index = 0; index < info->handle_count; ++index)
			{
				const auto& item = info->handles[index];
				const DWORD owner_pid = static_cast<DWORD>(item.owner_pid);
				if (owner_pid == 0 ||
				    owner_pid == target_pid ||
				    owner_pid == GetCurrentProcessId())
					continue;
				if ((item.granted_access & desired_access) != desired_access)
					continue;

				if (owner_pid != current_owner_pid)
				{
					close_owner();
					current_owner =
					    OpenProcess(PROCESS_DUP_HANDLE, FALSE, owner_pid);
					current_owner_pid = owner_pid;
				}
				if (!current_owner)
					continue;

				HANDLE duplicate = nullptr;
				if (!DuplicateHandle(
				        current_owner,
				        reinterpret_cast<HANDLE>(item.handle_value),
				        GetCurrentProcess(),
				        &duplicate,
				        0,
				        FALSE,
				        DUPLICATE_SAME_ACCESS))
					continue;

				if (GetProcessId(duplicate) == target_pid)
				{
					close_owner();
					LOG(HACKER) << "Using duplicated process handle owned by PID "
					            << owner_pid << " for target PID " << target_pid;
					return duplicate;
				}
				CloseHandle(duplicate);
			}

			close_owner();
			return nullptr;
		}
	}

	HANDLE acquire_process_handle(
	    std::uint32_t pid,
	    DWORD desired_access,
	    bool prefer_hijacked)
	{
		if (prefer_hijacked)
		{
			if (HANDLE handle =
			        duplicate_existing_process_handle(pid, desired_access))
				return handle;
			LOG(WARNING) << "No compatible existing process handle was found; "
			             << "falling back to OpenProcess.";
		}
		return OpenProcess(desired_access, FALSE, pid);
	}

	void* get_remote_proc_address(
	    std::uint32_t pid,
	    const wchar_t* module_name,
	    const char* procedure_name)
	{
		const HMODULE local_module = GetModuleHandleW(module_name);
		if (!local_module)
			return nullptr;
		const auto local_procedure =
		    reinterpret_cast<std::uintptr_t>(
		        GetProcAddress(local_module, procedure_name));
		if (!local_procedure)
			return nullptr;
		const auto procedure_offset =
		    local_procedure - reinterpret_cast<std::uintptr_t>(local_module);

		const HANDLE snapshot = CreateToolhelp32Snapshot(
		    TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
		    pid);
		if (snapshot == INVALID_HANDLE_VALUE)
			return nullptr;

		MODULEENTRY32W entry{sizeof(entry)};
		void* result = nullptr;
		if (Module32FirstW(snapshot, &entry))
		{
			do
			{
				if (_wcsicmp(entry.szModule, module_name) == 0)
				{
					result = reinterpret_cast<void*>(
					    reinterpret_cast<std::uintptr_t>(entry.modBaseAddr) +
					    procedure_offset);
					break;
				}
			}
			while (Module32NextW(snapshot, &entry));
		}
		CloseHandle(snapshot);
		return result;
	}

	std::vector<DWORD> get_process_thread_ids(std::uint32_t pid)
	{
		std::vector<DWORD> result;
		const HANDLE snapshot =
		    CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		if (snapshot == INVALID_HANDLE_VALUE)
			return result;

		THREADENTRY32 entry{sizeof(entry)};
		if (Thread32First(snapshot, &entry))
		{
			do
			{
				if (entry.th32OwnerProcessID == pid)
					result.push_back(entry.th32ThreadID);
			}
			while (Thread32Next(snapshot, &entry));
		}
		CloseHandle(snapshot);
		return result;
	}

	bool is_module_loaded(
	    std::uint32_t pid,
	    const std::filesystem::path& module_path)
	{
		const std::wstring expected = module_path.filename().wstring();
		const HANDLE snapshot = CreateToolhelp32Snapshot(
		    TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
		    pid);
		if (snapshot == INVALID_HANDLE_VALUE)
			return false;

		MODULEENTRY32W entry{sizeof(entry)};
		bool found = false;
		if (Module32FirstW(snapshot, &entry))
		{
			do
			{
				if (_wcsicmp(entry.szModule, expected.c_str()) == 0)
				{
					found = true;
					break;
				}
			}
			while (Module32NextW(snapshot, &entry));
		}
		CloseHandle(snapshot);
		return found;
	}
}
