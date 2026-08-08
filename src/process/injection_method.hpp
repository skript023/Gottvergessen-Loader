#pragma once
#include "common.hpp"
#include <string>
#include <filesystem>
#include <cstdint>

namespace gottvergessen
{
	enum class InjectionMode
	{
		CreateRemoteThread = 0,
		ThreadHijack = 1,
		ManualMap = 2
	};

	class injection_method
	{
	public:
		virtual ~injection_method() = default;

		virtual bool inject(const std::string& process_name, std::uint32_t pid, const std::filesystem::path& dll_path) = 0;

		static bool is_process_running(const std::string& process_name)
		{
			auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (snapshot == INVALID_HANDLE_VALUE)
				return false;

			auto entry = PROCESSENTRY32{ sizeof(PROCESSENTRY32) };

			if (Process32First(snapshot, &entry))
			{
				do
				{
					if (!_stricmp(entry.szExeFile, process_name.c_str()))
					{
						CloseHandle(snapshot);
						return true;
					}
				} while (Process32Next(snapshot, &entry));
			}

			CloseHandle(snapshot);
			return false;
		}

		static std::uint32_t get_process_id_by_name(const std::string& process_name)
		{
			auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (snapshot == INVALID_HANDLE_VALUE)
				return 0;

			auto entry = PROCESSENTRY32{ sizeof(PROCESSENTRY32) };

			if (Process32First(snapshot, &entry))
			{
				do
				{
					if (!_stricmp(entry.szExeFile, process_name.c_str()))
					{
						CloseHandle(snapshot);
						return entry.th32ProcessID;
					}
				} while (Process32Next(snapshot, &entry));
			}

			CloseHandle(snapshot);
			return 0;
		}
	};
}