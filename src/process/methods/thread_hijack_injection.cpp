#include "process/methods/thread_hijack_injection.hpp"
#include "process/process_handle.hpp"
#include "logger.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace gottvergessen
{
	namespace
	{
		using nt_query_information_thread = LONG(NTAPI*)(
		    HANDLE,
		    ULONG,
		    void*,
		    ULONG,
		    ULONG*);

		struct thread_basic_information_local
		{
			LONG exit_status;
			void* teb_base_address;
			struct
			{
				HANDLE unique_process;
				HANDLE unique_thread;
			} client_id;
			ULONG_PTR affinity_mask;
			LONG priority;
			LONG base_priority;
		};

		// Windows 10/11 x64 TEB SameTebFlags. Loader-worker threads are not
		// safe hijack targets because they may already be inside loader code.
		constexpr std::size_t teb_same_teb_flags_offset = 0x17EE;
		constexpr USHORT teb_loader_worker = 0x2000;
		constexpr ULONG system_process_information = 5;
		constexpr LONG status_info_length_mismatch = static_cast<LONG>(0xC0000004L);
		constexpr ULONG thread_state_running = 2;
		constexpr ULONG wait_reason_queue = 0x0F;

		struct client_id_local
		{
			HANDLE unique_process;
			HANDLE unique_thread;
		};

		struct system_thread_information_local
		{
			LARGE_INTEGER kernel_time;
			LARGE_INTEGER user_time;
			LARGE_INTEGER create_time;
			ULONG wait_time;
			void* start_address;
			client_id_local client_id;
			LONG priority;
			LONG base_priority;
			ULONG context_switches;
			ULONG thread_state;
			ULONG wait_reason;
		};

		struct system_process_information_local
		{
			ULONG next_entry_offset;
			ULONG number_of_threads;
			std::uint8_t reserved[0xF8];
			system_thread_information_local threads[1];
		};

		using nt_query_system_information = LONG(NTAPI*)(
		    ULONG,
		    void*,
		    ULONG,
		    ULONG*);

		std::vector<DWORD> get_running_thread_ids(std::uint32_t pid)
		{
			std::vector<DWORD> result;
			const auto ntdll = GetModuleHandleW(L"ntdll.dll");
			const auto query = ntdll
			    ? reinterpret_cast<nt_query_system_information>(
			          GetProcAddress(ntdll, "NtQuerySystemInformation"))
			    : nullptr;
			if (!query)
				return result;

			std::vector<std::uint8_t> buffer(1U << 16);
			ULONG required = 0;
			LONG status = 0;
			do
			{
				status = query(
				    system_process_information,
				    buffer.data(),
				    static_cast<ULONG>(buffer.size()),
				    &required);
				if (status == status_info_length_mismatch)
					buffer.resize(std::max<std::size_t>(
					    buffer.size() * 2,
					    static_cast<std::size_t>(required) + 0x10000));
			}
			while (status == status_info_length_mismatch);

			if (status < 0)
				return result;

			for (std::size_t offset = 0;;)
			{
				auto* process = reinterpret_cast<const system_process_information_local*>(
				    buffer.data() + offset);
				if (process->number_of_threads != 0 &&
				    reinterpret_cast<std::uintptr_t>(process->threads[0].client_id.unique_process) == pid)
				{
					for (ULONG index = 0; index < process->number_of_threads; ++index)
					{
						const auto& thread = process->threads[index];
						if (thread.thread_state == thread_state_running &&
						    thread.wait_reason != wait_reason_queue)
							result.push_back(static_cast<DWORD>(
							    reinterpret_cast<std::uintptr_t>(thread.client_id.unique_thread)));
					}
					break;
				}
				if (process->next_entry_offset == 0)
					break;
				offset += process->next_entry_offset;
			}
			return result;
		}

		bool is_loader_worker_thread(HANDLE process, HANDLE thread)
		{
			const auto ntdll = GetModuleHandleW(L"ntdll.dll");
			const auto query = ntdll
			    ? reinterpret_cast<nt_query_information_thread>(
			          GetProcAddress(ntdll, "NtQueryInformationThread"))
			    : nullptr;
			if (!query)
				return false;

			thread_basic_information_local info{};
			if (query(thread, 0, &info, sizeof(info), nullptr) < 0 ||
			    !info.teb_base_address)
				return false;

			USHORT flags = 0;
			SIZE_T read = 0;
			const auto flags_address = reinterpret_cast<const std::uint8_t*>(
			    info.teb_base_address) + teb_same_teb_flags_offset;
			if (!ReadProcessMemory(
			        process, flags_address, &flags, sizeof(flags), &read) ||
			    read != sizeof(flags))
				return false;

			return (flags & teb_loader_worker) != 0;
		}

		void append_u64(std::vector<std::uint8_t>& code, std::uint64_t value)
		{
			const auto start = code.size();
			code.resize(start + sizeof(value));
			std::memcpy(code.data() + start, &value, sizeof(value));
		}

		void append_u32(std::vector<std::uint8_t>& code, std::uint32_t value)
		{
			const auto start = code.size();
			code.resize(start + sizeof(value));
			std::memcpy(code.data() + start, &value, sizeof(value));
		}

		void append_bytes(
		    std::vector<std::uint8_t>& code,
		    std::initializer_list<std::uint8_t> bytes)
		{
			code.insert(code.end(), bytes.begin(), bytes.end());
		}

		std::vector<std::uint8_t> build_x64_loader_stub(
		    std::uint64_t remote_path,
		    std::uint64_t load_library_ex,
		    std::uint64_t result_address,
		    std::uint64_t error_address,
		    std::uint64_t completion_address,
		    std::uint64_t return_address)
		{
			std::vector<std::uint8_t> code;
			// Keep the original RIP on the stack. GH x64 returns with RET after
			// restoring the interrupted register/flags frame.
			append_bytes(code, {0x48, 0x83, 0xEC, 0x08});
			append_bytes(code, {0xC7, 0x04, 0x24});
			append_u32(code, static_cast<std::uint32_t>(return_address));
			append_bytes(code, {0xC7, 0x44, 0x24, 0x04});
			append_u32(code, static_cast<std::uint32_t>(return_address >> 32));

			// Preserve flags and every general-purpose register.
			append_bytes(code, {
			    0x9C,
			    0x50, 0x51, 0x52, 0x53, 0x55, 0x56, 0x57,
			    0x41, 0x50, 0x41, 0x51, 0x41, 0x52, 0x41, 0x53,
			    0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57});

			// RBP keeps the saved stack while RSP is aligned for the x64 ABI.
			append_bytes(code, {0x48, 0x89, 0xE5});
			append_bytes(code, {0x48, 0x83, 0xE4, 0xF0});
			// Keep the Windows x64 call-site ABI: RSP is 16-byte aligned before
			// CALL and the first 32 bytes are reserved as shadow space.
			append_bytes(code, {0x48, 0x83, 0xEC, 0x20});

			append_bytes(code, {0x48, 0xB9});
			append_u64(code, remote_path);
			// LoadLibraryExW(path, nullptr, 0), matching the normal loader path.
			append_bytes(code, {0x48, 0x31, 0xD2});
			append_bytes(code, {0x45, 0x31, 0xC0});
			append_bytes(code, {0x48, 0xB8});
			append_u64(code, load_library_ex);
			append_bytes(code, {0xFF, 0xD0});

			// Store HMODULE and signal completion.
			append_bytes(code, {0x48, 0xBB});
			append_u64(code, result_address);
			append_bytes(code, {0x48, 0x89, 0x03});
			// Read the target thread's Win32 error directly from its TEB, as GH.
			append_bytes(code, {0x65, 0x48, 0x8B, 0x04, 0x25, 0x30, 0x00, 0x00, 0x00});
			append_bytes(code, {0x8B, 0x40, 0x68});
			append_bytes(code, {0x48, 0xBB});
			append_u64(code, error_address);
			append_bytes(code, {0x89, 0x03});
			append_bytes(code, {0x48, 0xBB});
			append_u64(code, completion_address);
			append_bytes(code, {0xC6, 0x03, 0x01});

			append_bytes(code, {0x48, 0x83, 0xC4, 0x20});
			append_bytes(code, {0x48, 0x89, 0xEC});
			append_bytes(code, {
			    0x41, 0x5F, 0x41, 0x5E, 0x41, 0x5D, 0x41, 0x5C,
			    0x41, 0x5B, 0x41, 0x5A, 0x41, 0x59, 0x41, 0x58,
			    0x5F, 0x5E, 0x5D, 0x5B,
			    0x5A, 0x59, 0x58,
			    0x9D});

			// Return to the original RIP using the stack entry prepared above.
			append_bytes(code, {0xC3});
			return code;
		}
	}

	bool thread_hijack_injection::inject(
	    const std::string& process_name,
	    std::uint32_t pid,
	    const std::filesystem::path& dll_path)
	{
#ifndef _WIN64
		LOG(WARNING) << "Thread Hijack is only implemented for x64 targets.";
		return false;
#else
		if (pid == 0 || !std::filesystem::exists(dll_path))
		{
			LOG(WARNING) << "Thread Hijack: invalid target or DLL path for "
			             << process_name;
			return false;
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
			LOG(WARNING) << "Thread Hijack: failed to acquire process handle. 0x"
			             << std::hex << GetLastError();
			return false;
		}

		const auto load_library_ex = reinterpret_cast<std::uint64_t>(
		    get_remote_proc_address(pid, L"kernel32.dll", "LoadLibraryExW"));
		if (!load_library_ex)
		{
			LOG(WARNING) << "Thread Hijack: unable to resolve remote loader functions.";
			CloseHandle(process);
			return false;
		}

		HANDLE thread = nullptr;
		CONTEXT context{};
		context.ContextFlags = CONTEXT_CONTROL;
		CONTEXT original_context{};
		auto thread_ids = get_running_thread_ids(pid);
		if (thread_ids.empty())
			thread_ids = get_process_thread_ids(pid);
		for (const DWORD thread_id : thread_ids)
		{
			HANDLE candidate = OpenThread(
			    THREAD_SUSPEND_RESUME |
			    THREAD_GET_CONTEXT |
			    THREAD_SET_CONTEXT |
			    THREAD_QUERY_INFORMATION,
			    FALSE,
			    thread_id);
			if (!candidate)
				continue;
			if (is_loader_worker_thread(process, candidate))
			{
				LOG(INFO) << "Thread Hijack: skipping loader-worker thread " << thread_id;
				CloseHandle(candidate);
				continue;
			}
			if (SuspendThread(candidate) == static_cast<DWORD>(-1))
			{
				CloseHandle(candidate);
				continue;
			}
			if (GetThreadContext(candidate, &context))
			{
				original_context = context;
				thread = candidate;
				break;
			}
			ResumeThread(candidate);
			CloseHandle(candidate);
		}
		if (!thread)
		{
			LOG(WARNING) << "Thread Hijack: no suspendable x64 thread found.";
			CloseHandle(process);
			return false;
		}

		const std::wstring absolute_path =
		    std::filesystem::absolute(dll_path).wstring();
		const SIZE_T path_bytes =
		    (absolute_path.size() + 1) * sizeof(wchar_t);
		const SIZE_T result_offset = (path_bytes + 7) & ~SIZE_T(7);
		const SIZE_T error_offset = result_offset + sizeof(std::uint64_t);
		const SIZE_T completion_offset = error_offset + sizeof(DWORD);
		const SIZE_T code_offset = (completion_offset + 15) & ~SIZE_T(15);
		const SIZE_T allocation_size = code_offset + 512;

		auto* remote = static_cast<std::uint8_t*>(VirtualAllocEx(
		    process,
		    nullptr,
		    allocation_size,
		    MEM_RESERVE | MEM_COMMIT,
		    PAGE_EXECUTE_READWRITE));
		if (!remote)
		{
			LOG(WARNING) << "Thread Hijack: remote allocation failed. 0x"
			             << std::hex << GetLastError();
			ResumeThread(thread);
			CloseHandle(thread);
			CloseHandle(process);
			return false;
		}

		const auto code = build_x64_loader_stub(
		    reinterpret_cast<std::uint64_t>(remote),
		    load_library_ex,
		    reinterpret_cast<std::uint64_t>(remote + result_offset),
		    reinterpret_cast<std::uint64_t>(remote + error_offset),
		    reinterpret_cast<std::uint64_t>(remote + completion_offset),
		    context.Rip);
		SIZE_T path_written = 0;
		SIZE_T code_written = 0;
		const bool written =
			WriteProcessMemory(
				process,
				remote,
				absolute_path.c_str(),
				path_bytes,
				&path_written) &&
			WriteProcessMemory(
				process,
				remote + code_offset,
				code.data(),
				code.size(),
				&code_written) &&
			path_written == path_bytes &&
			code_written == code.size();
		if (!written)
		{
			LOG(WARNING) << "Thread Hijack: failed to write remote loader. 0x"
			             << std::hex << GetLastError();
			VirtualFreeEx(process, remote, 0, MEM_RELEASE);
			ResumeThread(thread);
			CloseHandle(thread);
			CloseHandle(process);
			return false;
		}

		if (!FlushInstructionCache(process, remote + code_offset, code.size()))
		{
			LOG(WARNING) << "Thread Hijack: FlushInstructionCache failed. 0x"
			             << std::hex << GetLastError();
			VirtualFreeEx(process, remote, 0, MEM_RELEASE);
			ResumeThread(thread);
			CloseHandle(thread);
			CloseHandle(process);
			return false;
		}
		context.Rip =
		    reinterpret_cast<DWORD64>(remote + code_offset);
		if (!SetThreadContext(thread, &context))
		{
			LOG(WARNING) << "Thread Hijack: SetThreadContext failed. 0x"
			             << std::hex << GetLastError();
			VirtualFreeEx(process, remote, 0, MEM_RELEASE);
			ResumeThread(thread);
			CloseHandle(thread);
			CloseHandle(process);
			return false;
		}
		if (ResumeThread(thread) == static_cast<DWORD>(-1))
		{
			LOG(WARNING) << "Thread Hijack: ResumeThread failed. 0x"
			             << std::hex << GetLastError();
			VirtualFreeEx(process, remote, 0, MEM_RELEASE);
			CloseHandle(thread);
			CloseHandle(process);
			return false;
		}
		PostThreadMessageW(GetThreadId(thread), WM_NULL, 0, 0);

		std::uint8_t completed = 0;
		const auto deadline = std::chrono::steady_clock::now() + 10s;
		while (std::chrono::steady_clock::now() < deadline && !completed)
		{
			ReadProcessMemory(
			    process,
			    remote + completion_offset,
			    &completed,
			    sizeof(completed),
			    nullptr);
			if (!completed)
				std::this_thread::sleep_for(10ms);
		}

		std::uint64_t module = 0;
		DWORD loader_error = ERROR_SUCCESS;
		if (completed)
		{
			ReadProcessMemory(
			    process,
			    remote + result_offset,
			    &module,
			    sizeof(module),
			    nullptr);
			ReadProcessMemory(
			    process,
			    remote + error_offset,
			    &loader_error,
			    sizeof(loader_error),
			    nullptr);
		}

		bool released = false;
		for (int attempt = 0; completed && attempt < 50; ++attempt)
		{
			if (WaitForSingleObject(thread, 0) == WAIT_OBJECT_0)
			{
				released =
				    VirtualFreeEx(process, remote, 0, MEM_RELEASE) != FALSE;
				break;
			}
			if (SuspendThread(thread) != static_cast<DWORD>(-1))
			{
				CONTEXT current{};
				current.ContextFlags = CONTEXT_CONTROL;
				if (GetThreadContext(thread, &current))
				{
					const auto begin =
					    reinterpret_cast<std::uint64_t>(remote + code_offset);
					const auto end = begin + code.size();
					if (current.Rip < begin || current.Rip >= end)
						released =
						    VirtualFreeEx(process, remote, 0, MEM_RELEASE) != FALSE;
				}
				ResumeThread(thread);
				if (released)
					break;
			}
			std::this_thread::sleep_for(10ms);
		}

		if (!completed)
		{
			bool restored = false;
			if (SuspendThread(thread) != static_cast<DWORD>(-1))
			{
				if (SetThreadContext(thread, &original_context))
					restored = ResumeThread(thread) != static_cast<DWORD>(-1);
				else
					ResumeThread(thread);
			}
			if (restored)
			{
				VirtualFreeEx(process, remote, 0, MEM_RELEASE);
				LOG(WARNING) << "Thread Hijack timed out; original thread context restored.";
			}
			else
			{
				LOG(WARNING) << "Thread Hijack timed out; retained remote loader "
				             << "allocation because context recovery failed.";
			}
			CloseHandle(thread);
			CloseHandle(process);
			return false;
		}
		CloseHandle(thread);
		CloseHandle(process);
		if (module == 0)
		{
			LOG(WARNING) << "Thread Hijack: LoadLibraryExW returned NULL. LastError=0x"
			             << std::hex << loader_error;
			return false;
		}
		LOG(HACKER) << "Thread Hijack injection completed.";
		return true;
#endif
	}
}
