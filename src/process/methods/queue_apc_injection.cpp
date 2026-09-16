#include "process/methods/queue_apc_injection.hpp"
#include "process/process_handle.hpp"
#include "logger.hpp"

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

namespace gottvergessen
{
	bool queue_apc_injection::inject(
	    const std::string& process_name,
	    std::uint32_t pid,
	    const std::filesystem::path& dll_path)
	{
		if (pid == 0 || !std::filesystem::exists(dll_path))
		{
			LOG(WARNING) << "QueueUserAPC: invalid target or DLL path for "
			             << process_name;
			return false;
		}
		if (is_module_loaded(pid, dll_path))
		{
			LOG(HACKER) << "QueueUserAPC: module is already loaded.";
			return true;
		}

		constexpr DWORD process_access =
		    PROCESS_QUERY_LIMITED_INFORMATION |
		    PROCESS_VM_OPERATION |
		    PROCESS_VM_WRITE |
		    PROCESS_VM_READ;
		HANDLE process =
		    acquire_process_handle(pid, process_access, true);
		if (!process)
		{
			LOG(WARNING) << "QueueUserAPC: failed to acquire process handle. 0x"
			             << std::hex << GetLastError();
			return false;
		}

		const auto load_library = reinterpret_cast<PAPCFUNC>(
		    get_remote_proc_address(pid, L"kernel32.dll", "LoadLibraryW"));
		if (!load_library)
		{
			LOG(WARNING) << "QueueUserAPC: unable to resolve remote LoadLibraryW.";
			CloseHandle(process);
			return false;
		}

		const std::wstring absolute_path =
		    std::filesystem::absolute(dll_path).wstring();
		const SIZE_T path_bytes =
		    (absolute_path.size() + 1) * sizeof(wchar_t);
		void* remote_path = VirtualAllocEx(
		    process,
		    nullptr,
		    path_bytes,
		    MEM_RESERVE | MEM_COMMIT,
		    PAGE_READWRITE);
		if (!remote_path ||
		    !WriteProcessMemory(
		        process,
		        remote_path,
		        absolute_path.c_str(),
		        path_bytes,
		        nullptr))
		{
			LOG(WARNING) << "QueueUserAPC: failed to copy DLL path. 0x"
			             << std::hex << GetLastError();
			if (remote_path)
				VirtualFreeEx(process, remote_path, 0, MEM_RELEASE);
			CloseHandle(process);
			return false;
		}

		std::size_t queued_count = 0;
		for (const DWORD thread_id : get_process_thread_ids(pid))
		{
			HANDLE thread = OpenThread(THREAD_SET_CONTEXT, FALSE, thread_id);
			if (!thread)
				continue;
			if (QueueUserAPC(
			        load_library,
			        thread,
			        reinterpret_cast<ULONG_PTR>(remote_path)) != 0)
				++queued_count;
			CloseHandle(thread);
		}

		if (queued_count == 0)
		{
			LOG(WARNING) << "QueueUserAPC: no target thread accepted the APC.";
			VirtualFreeEx(process, remote_path, 0, MEM_RELEASE);
			CloseHandle(process);
			return false;
		}

		const auto deadline = std::chrono::steady_clock::now() + 10s;
		while (std::chrono::steady_clock::now() < deadline)
		{
			if (is_module_loaded(pid, dll_path))
			{
				std::this_thread::sleep_for(250ms);
				VirtualFreeEx(process, remote_path, 0, MEM_RELEASE);
				CloseHandle(process);
				LOG(HACKER) << "QueueUserAPC injection completed after queuing "
				            << queued_count << " thread(s).";
				return true;
			}
			std::this_thread::sleep_for(100ms);
		}

		// The APC remains queued and can still run later. Keep the small path
		// allocation valid instead of creating a use-after-free in the target.
		CloseHandle(process);
		LOG(WARNING) << "QueueUserAPC timed out: target threads did not enter an "
		             << "alertable wait; retained remote path allocation.";
		return false;
	}
}
