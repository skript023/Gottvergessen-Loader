#include "process/process_handle.hpp"
#include "logger.hpp"

#include <TlHelp32.h>
#include <algorithm>
#include <cstddef>
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

		struct handle_data
		{
			DWORD OwnerPID;
			ULONG_PTR hValue;
			DWORD Access;
		};

		using nt_query_system_information =
		    LONG(NTAPI*)(ULONG, void*, ULONG, ULONG*);

		bool enable_debug_privilege()
		{
			HANDLE token = nullptr;
			if (!OpenProcessToken(
					GetCurrentProcess(),
					TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
					&token))
				return false;

			TOKEN_PRIVILEGES privileges{};
			privileges.PrivilegeCount = 1;
			const bool found = LookupPrivilegeValueW(
				nullptr,
				L"SeDebugPrivilege",
				&privileges.Privileges[0].Luid) != FALSE;
			if (found)
			{
				privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
				AdjustTokenPrivileges(
					token,
					FALSE,
					&privileges,
					0,
					nullptr,
					nullptr);
			}
			const bool enabled = found && GetLastError() == ERROR_SUCCESS;
			CloseHandle(token);
			return enabled;
		}
		bool enum_handles(std::vector<system_handle_entry>& result)
		{
			if (!enable_debug_privilege())
				LOG(WARNING) << "Handle hijack: SeDebugPrivilege could not be enabled.";

			auto ntdll = GetModuleHandleW(L"ntdll.dll");
			auto query = reinterpret_cast<nt_query_system_information>(
			    GetProcAddress(ntdll, "NtQuerySystemInformation"));
			if (!query)
				return false;

			std::vector<std::uint8_t> buffer(0x10000);
			ULONG required = 0;
			LONG status = 0;
			do
			{
				status = query(
				    system_extended_handle_information,
				    buffer.data(),
				    static_cast<ULONG>(buffer.size()),
				    &required);
				if (status == status_info_length_mismatch)
				{
					const auto next_size = std::max<std::size_t>(
					    buffer.size() * 2,
					    static_cast<std::size_t>(required) + 0x10000);
					buffer.resize(next_size);
				}
			}
			while (status == status_info_length_mismatch);

			if (status < 0 ||
			    buffer.size() < offsetof(system_handle_information, handles))
				return false;

			const auto* info =
			    reinterpret_cast<const system_handle_information*>(buffer.data());
			const auto max_entries =
			    (buffer.size() - offsetof(system_handle_information, handles)) /
			    sizeof(system_handle_entry);
			if (info->handle_count > max_entries)
				return false;

			result.assign(info->handles, info->handles + info->handle_count);
			LOG(INFO) << result.size() << " handles found.";
			return true;
		}

		std::vector<handle_data> enum_process_handles()
		{
			std::vector<handle_data> result;
			std::vector<system_handle_entry> entries;
			if (!enum_handles(entries))
				return result;

			result.reserve(entries.size());
			for (const auto& entry : entries)
			{
				if (entry.owner_pid == 0)
					continue;
				result.push_back(handle_data{
				    static_cast<DWORD>(entry.owner_pid),
				    entry.handle_value,
				    entry.granted_access});
			}
			return result;
		}

		std::vector<handle_data> find_process_handles(
		    DWORD target_pid,
		    DWORD wanted_handle_access)
		{
			std::vector<handle_data> result;
			const DWORD own_pid = GetCurrentProcessId();
			const auto handles = enum_process_handles();

			for (const auto& item : handles)
			{
				if (item.OwnerPID == own_pid || item.OwnerPID == target_pid)
					continue;
				if ((item.Access & wanted_handle_access) != wanted_handle_access)
					continue;

				const HANDLE owner =
				    OpenProcess(PROCESS_DUP_HANDLE, FALSE, item.OwnerPID);
				if (!owner)
					continue;

				HANDLE duplicate = nullptr;
				const BOOL duplicated = DuplicateHandle(
				    owner,
				    reinterpret_cast<HANDLE>(item.hValue),
				    GetCurrentProcess(),
				    &duplicate,
				    0,
				    FALSE,
				    DUPLICATE_SAME_ACCESS);
				CloseHandle(owner);

				if (!duplicated)
					continue;
				if (GetProcessId(duplicate) == target_pid)
					result.push_back(item);
				CloseHandle(duplicate);
			}

			LOG(INFO) << result.size()
			          << " handle(s) to target process found.";
			return result;
		}

		HANDLE duplicate_existing_process_handle(
		    std::uint32_t target_pid,
		    DWORD desired_access)
		{
			const auto candidates = find_process_handles(
			    static_cast<DWORD>(target_pid), desired_access);
			for (const auto& candidate : candidates)
			{
				const HANDLE owner =
				    OpenProcess(PROCESS_DUP_HANDLE, FALSE, candidate.OwnerPID);
				if (!owner)
					continue;

				HANDLE duplicate = nullptr;
				const BOOL duplicated = DuplicateHandle(
				    owner,
				    reinterpret_cast<HANDLE>(candidate.hValue),
				    GetCurrentProcess(),
				    &duplicate,
				    0,
				    FALSE,
				    DUPLICATE_SAME_ACCESS);
				CloseHandle(owner);

				if (!duplicated)
					continue;
				if (GetProcessId(duplicate) == target_pid)
				{
					LOG(HACKER) << "Using duplicated process handle owned by PID "
					            << candidate.OwnerPID << " for target PID "
					            << target_pid;
					return duplicate;
				}
				CloseHandle(duplicate);
			}

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
