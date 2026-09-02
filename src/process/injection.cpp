#include "process/injection.hpp"
#ifndef GOTTVERGESSEN_NATIVE_ADDON
#include "api/remote/download_binary.hpp"
#endif
#include "logger.hpp"

#include <Windows.h>
#include <chrono>
#include <fstream>
#include <thread>

using namespace std::chrono_literals;

namespace gottvergessen
{
	injection::injection()
	{
		try
		{
			m_filename = file_manager::get_project_folder("./Binary");
		}
		catch (...)
		{
		}
		m_strategy = create_strategy(m_mode);
	}

	void injection::set_injection_mode_impl(InjectionMode mode)
	{
		m_mode = mode;
		m_strategy = create_strategy(m_mode);
	}

	std::unique_ptr<injection_method> injection::create_strategy(InjectionMode mode)
	{
		switch (mode)
		{
		case InjectionMode::CreateRemoteThread:
			return std::make_unique<remote_thread_injection>();
		case InjectionMode::ThreadHijack:
			return std::make_unique<thread_hijack_injection>();
		case InjectionMode::ManualMap:
			return std::make_unique<manual_map_injection>();
		case InjectionMode::ReflectiveInjection:
			return std::make_unique<reflective_injection>();
		default:
			return std::make_unique<remote_thread_injection>();
		}
	}

	bool injection::inject_library_impl(const std::filesystem::path& target_dll_path)
	{
	#ifdef GOTTVERGESSEN_NATIVE_ADDON
		if (target_dll_path.empty())
		{
			LOG(WARNING) << "The Electron native addon requires an explicit DLL path.";
			return false;
		}
		auto filename = target_dll_path;
	#else
		m_filename = file_manager::get_project_folder("./Binary");
		auto filename = target_dll_path.empty() ? m_filename.get_file(download_binary::get_binary_name()).get_path() : target_dll_path;
		if (m_target_process.empty() || m_target_process == "notepad.exe")
		{
			std::string target_from_server = download_binary::injection_target();
			if (!target_from_server.empty())
				set_target_process_impl(target_from_server);
		}
	#endif

		if (!this->validate_binary_impl(filename))
			return false;

		if (m_selected_pid != 0)
		{
			m_pid = m_selected_pid;
			LOG(HACKER) << "Using explicitly selected target process PID: " << m_pid;
		}
		else
		{
			LOG(HACKER) << "Waiting for process " << m_target_process;

			int wait_attempts = 0;
			while (!injection_method::is_process_running(m_target_process))
			{
				if (++wait_attempts > 50) // 5 seconds timeout max
				{
					LOG(WARNING) << "Timed out waiting for process " << m_target_process;
					return false;
				}
				std::this_thread::sleep_for(100ms);
			}

			constexpr int max_retries = 10;
			for (int attempt = 0; attempt < max_retries; ++attempt)
			{
				m_pid = injection_method::get_process_id_by_name(m_target_process);
				if (m_pid == 0)
				{
					std::this_thread::sleep_for(500ms);
					continue;
				}
				// Quick accessibility check — open and immediately close
				HANDLE test_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, m_pid);
				if (test_handle != NULL)
				{
					CloseHandle(test_handle);
					break;
				}
				LOG(HACKER) << "PID " << m_pid
				            << " not yet accessible (attempt "
				            << (attempt + 1) << "/" << max_retries
				            << "), retrying...";
				m_pid = 0;
				std::this_thread::sleep_for(500ms);
			}
			if (m_pid == 0)
			{
				LOG(WARNING) << "Failed to obtain accessible PID for " << m_target_process;
				return false;
			}

			LOG(HACKER) << "Process " << m_target_process << " PID : " << m_pid;
		}

		if (!m_strategy)
			m_strategy = create_strategy(m_mode);

		if (!m_strategy || !m_strategy->inject(m_target_process, m_pid, filename))
		{
			LOG(HACKER) << "Could not inject the binary using active strategy.";
			return false;
		}

		LOG(HACKER) << "Binary injection completed successfully.";
		return true;
	}

	bool injection::validate_binary_impl(std::filesystem::path filename)
	{
		if (!filename.is_absolute())
			filename = std::filesystem::absolute(filename);

		LOG(HACKER) << "Unpacking " << filename.filename().string();

		switch (this->validate_file_impl(filename))
		{
		case eValidType::ACCESS_FAILURE:
			LOG(WARNING) << "Failed to access binary on disk.";
			return false;
		case eValidType::TOO_SMALL:
			LOG(WARNING) << "binary file seems inconceivably small, request to inject ignored.";
			return false;
		case eValidType::ALLOCATION_FAILURE:
			LOG(WARNING) << "Failed to allocate memory when checking binary file.";
			return false;
		case eValidType::NOT_A_DLL:
			LOG(WARNING) << "The file given does not appear to be a valid binary.";
			return false;
		case eValidType::INVALID_PLATFORM:
			LOG(WARNING) << "The binary given did not match the target platform the injector.";
			return false;
		case eValidType::NEED_UPDATE:
			LOG(WARNING) << "The binary requires an update before injection.";
			return false;
		case eValidType::VALID:
			LOG(HACKER) << "Binary seems valid, proceeding with injection.";
			break;
		}

		return true;
	}

	eValidType injection::validate_file_impl(const std::filesystem::path& dllFile)
	{
		std::ifstream fileStream(dllFile, std::ios::binary | std::ios::ate);

		if (fileStream.fail())
		{
			fileStream.close();
			return eValidType::ACCESS_FAILURE;
		}

		const auto fileSize = fileStream.tellg();
		if (fileSize < 0x1000)
		{
			fileStream.close();
			return eValidType::TOO_SMALL;
		}

		auto* pSrcData = new uint8_t[static_cast<uintptr_t>(fileSize)];
		if (!pSrcData)
		{
			fileStream.close();
			return eValidType::ALLOCATION_FAILURE;
		}

		fileStream.seekg(0, std::ios::beg);
		fileStream.read(reinterpret_cast<char*>(pSrcData), fileSize);
		fileStream.close();

		if (reinterpret_cast<IMAGE_DOS_HEADER*>(pSrcData)->e_magic != 0x5A4D)
		{
			delete[] pSrcData;
			return eValidType::NOT_A_DLL;
		}

		const auto* pOldNtHeader = reinterpret_cast<IMAGE_NT_HEADERS*>(pSrcData + reinterpret_cast<IMAGE_DOS_HEADER*>(pSrcData)->e_lfanew);
		const auto* pOldFileHeader = &pOldNtHeader->FileHeader;

#ifdef _WIN64
		if (pOldFileHeader->Machine != IMAGE_FILE_MACHINE_AMD64)
		{
			delete[] pSrcData;
			return eValidType::INVALID_PLATFORM;
		}
#else
		if (pOldFileHeader->Machine != IMAGE_FILE_MACHINE_I386)
		{
			delete[] pSrcData;
			return eValidType::INVALID_PLATFORM;
		}
#endif

		delete[] pSrcData;
		return eValidType::VALID;
	}
}
