#include "process/methods/remote_thread_injection.hpp"
#include "logger.hpp"

namespace gottvergessen
{
	bool remote_thread_injection::inject(const std::string& process_name, std::uint32_t pid, const std::filesystem::path& dll_path)
	{
		HANDLE m_handle{ NULL };
		HANDLE m_create_remote_thread{ NULL };
		LPVOID m_virtual_alloc{ NULL };

		auto cleanup = [m_handle, m_create_remote_thread, m_virtual_alloc]() -> void
		{
			if (m_virtual_alloc && m_handle)
				VirtualFreeEx(m_handle, m_virtual_alloc, NULL, MEM_RELEASE);
			if (m_handle)
				CloseHandle(m_handle);
			if (m_create_remote_thread)
				CloseHandle(m_create_remote_thread);
		};

		std::string file_name = dll_path.string();
		if (!std::filesystem::exists(dll_path))
		{
			LOG(WARNING) << file_name << " file doesn't exist.";
			return false;
		}

		if (pid == 0)
		{
			LOG(WARNING) << process_name << " is not open.";
			return false;
		}

		m_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		if (m_handle == NULL)
		{
			LOG(WARNING) << "Failed to OpenProcess(). 0x" << std::uppercase << std::hex << GetLastError();
			cleanup();
			return false;
		}

		std::string abs_path = std::filesystem::absolute(dll_path).string();
		m_virtual_alloc = VirtualAllocEx(m_handle, NULL, static_cast<long>(abs_path.size() * 2), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (m_virtual_alloc == NULL)
		{
			LOG(WARNING) << "Failed to VirtualAllocEx. 0x" << std::uppercase << std::hex << GetLastError();
			cleanup();
			return false;
		}

		auto m_write_process_memory = WriteProcessMemory(m_handle, m_virtual_alloc, abs_path.c_str(), static_cast<long>(abs_path.length() + 1), NULL);
		if (m_write_process_memory == NULL)
		{
			LOG(WARNING) << "Failed to write memory to process. 0x" << std::uppercase << std::hex << GetLastError();
			cleanup();
			return false;
		}

		auto m_loadlibrary_address = reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"));
		if (m_loadlibrary_address == NULL)
		{
			LOG(WARNING) << "Failed to get kernel process. " << std::uppercase << std::hex << GetLastError();
			cleanup();
			return false;
		}

		LOG(HACKER) << "[Start Address]: 0x" << std::uppercase << std::hex << reinterpret_cast<DWORD64>(m_loadlibrary_address);

		m_create_remote_thread = CreateRemoteThread(m_handle, NULL, NULL, m_loadlibrary_address, m_virtual_alloc, NULL, NULL);
		if (m_create_remote_thread == NULL)
		{
			LOG(WARNING) << "Failed to remote thread at 0x" << std::uppercase << std::hex << GetLastError();
			cleanup();
			return false;
		}

		// Wait until LoadLibraryA finishes executing inside target process before cleaning up!
		WaitForSingleObject(m_create_remote_thread, INFINITE);

		cleanup();
		return true;
	}
}
