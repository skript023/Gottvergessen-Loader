#include "process/injection.hpp"
#include "api/remote/download_binary.hpp"
#include "logger.hpp"

namespace gottvergessen
{
	injection::injection()
	{
		try
		{
			m_filename = file_manager::get_project_folder("./Binary");
		}
		catch (...) {}
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
		default:
			return std::make_unique<remote_thread_injection>();
		}
	}

	bool injection::inject_library_impl()
	{
		auto filename = m_filename.get_file(download_binary::get().get_binary_name()).get_path();
		set_target_process_impl(download_binary::get().injection_target());

		if (!this->validate_binary_impl(filename)) return false;

		LOG(HACKER) << "Waiting for process " << m_target_process;

		while (!injection_method::is_process_running(m_target_process))
			std::this_thread::sleep_for(100ms);

		m_pid = injection_method::get_process_id_by_name(m_target_process);

		LOG(HACKER) << "Process " << m_target_process << " PID : " << m_pid;

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