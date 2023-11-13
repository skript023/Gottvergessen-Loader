#include "injection.hpp"
#include "api/remote/download_binary.hpp"

namespace gottvergessen
{
	injection::injection(const folder& dllFile) : m_filename(dllFile)
	{
		g_injection = this;
	}
	injection::~injection() noexcept
	{
		g_injection = nullptr;
	}

	bool injection::create_remote_thread(std::string file_name)
	{
		HANDLE m_handle{ NULL };
		HANDLE m_create_remote_thread{ NULL };
		LPVOID m_virtual_alloc{ NULL };

		auto cleanup = [m_handle, m_create_remote_thread, m_virtual_alloc]() -> void
		{
			VirtualFreeEx(m_handle, m_virtual_alloc, NULL, MEM_RELEASE);
			CloseHandle(m_handle);
			CloseHandle(m_create_remote_thread);
		};

		if (!std::filesystem::exists(file_name))
		{
			LOG(WARNING) << file_name << " file doesn't exist.";
			return false;
		}

		if (pid == NULL)
		{
			LOG(WARNING) << name << " is not open.";
			return false;
		}

		m_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		if (m_handle == NULL)
		{
			LOG(WARNING) << "Failed to OpenProcess(). 0x" << std::uppercase << std::hex << GetLastError();
			cleanup();
			return false;
		}

		m_virtual_alloc = VirtualAllocEx(m_handle, NULL, static_cast<long>(std::filesystem::absolute(file_name).string().size() * 2), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (m_virtual_alloc == NULL)
		{
			LOG(WARNING) << "Failed to VirtualAllocEx. 0x" << std::uppercase << std::hex << GetLastError();
			cleanup();
			return false;
		}

		auto m_write_process_memory = WriteProcessMemory(m_handle, m_virtual_alloc, std::filesystem::absolute(file_name).string().c_str(), static_cast<long>(std::filesystem::absolute(file_name).string().length() + 1), NULL);
		if (m_write_process_memory == NULL)
		{
			LOG(WARNING) << "Failed to write memory to process. 0x" << std::uppercase << std::hex << GetLastError();
			cleanup();
			return false;
		}

		auto m_loadlibrary_address = reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandleA("kernel32.dll"), ("LoadLibraryA")));
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

		cleanup();

		return true;
	}
	/*
	
	bool injection::thread_execution_hijacking(std::string filename)
	{
		HINSTANCE injection_module = LoadLibrary(GH_INJ_MOD_NAME);
		auto inject_library = (f_InjectA)GetProcAddress(injection_module, "InjectA");

		auto symbol_state = (f_GetSymbolState)GetProcAddress(injection_module, "GetSymbolState");
		auto import_state = (f_GetSymbolState)GetProcAddress(injection_module, "GetImportState");
		auto start_download = (f_StartDownload)GetProcAddress(injection_module, "StartDownload");

		start_download();

		while (symbol_state() != 0)
		{
			Sleep(10);
		}

		while (import_state() != 0)
		{
			Sleep(10);
		}

		if (this->get_process_id_by_name() == NULL)
		{
			LOG(WARNING) << "Injection failed, no process id found.";

			return false;
		}

		INJECTIONDATAA data = {
			"",
			this->get_process_id_by_name(),
			INJECTION_MODE::IM_LdrLoadDll,
			LAUNCH_METHOD::LM_HijackThread,
			INJ_HIJACK_HANDLE | INJ_FAKE_HEADER,
			2000,
			NULL,
			NULL,
			false
		};

		strcpy(data.szDllPath, filename.c_str());

		return inject_library(&data) == 0;
	}

	*/
	bool injection::inject_library()
	{
		auto filename = m_filename.get_file(g_download_binary->get_binary_name()).get_path();
		this->set_target_process(g_download_binary->injection_target());

		if (!this->validate_binary(filename)) return false;

		LOG(HACKER) << "Waiting for " << this->name;

		while (!this->is_process_running())
			std::this_thread::sleep_for(100ms);

		this->pid = this->get_process_id_by_name();

		LOG(HACKER) << "Process " << this->name << " PID : " << this->pid;

		if (!this->create_remote_thread(filename.string()))
		{
			LOG(HACKER) << "Could not inject the binary.";

			return false;
		}

		LOG(HACKER) << "Binary injection done.";

		return true;
	}

	bool injection::validate_binary(std::filesystem::path filename)
	{
		if (!filename.is_absolute())
			filename = std::filesystem::absolute(filename);

		LOG(HACKER) << "Unpacking " << filename.filename().string();

		switch (this->validate_file(filename))
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

	eValidType injection::validate_file(const std::filesystem::path& dllFile)
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
			delete pOldFileHeader;
			delete pOldNtHeader;

			return eValidType::INVALID_PLATFORM;
		}
#else
		if (pOldFileHeader->Machine != IMAGE_FILE_MACHINE_I386)
		{
			delete[] pSrcData;
			delete pOldFileHeader;
			delete pOldNtHeader;

			return eValidType::INVALID_PLATFORM;
		}
#endif

		return eValidType::VALID;
	}
}

//CRM whole sell
//CRM WIB (NCX WIB)