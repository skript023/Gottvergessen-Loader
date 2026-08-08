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
		ManualMap = 2,
		ReflectiveInjection = 3
	};

	struct process_info
	{
		std::uint32_t pid{ 0 };
		std::string name;
		std::string arch{ "x64" };
		bool is_accessible{ false };
	};

	class injection_method
	{
	public:
		virtual ~injection_method() = default;

		virtual bool inject(const std::string& process_name, std::uint32_t pid, const std::filesystem::path& dll_path) = 0;

		static std::vector<process_info> get_running_processes()
		{
			std::vector<process_info> list;
			auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (snapshot == INVALID_HANDLE_VALUE)
				return list;

			auto entry = PROCESSENTRY32{ sizeof(PROCESSENTRY32) };

			if (Process32First(snapshot, &entry))
			{
				do
				{
					if (entry.th32ProcessID == 0) continue;

					process_info info;
					info.pid = entry.th32ProcessID;
					info.name = entry.szExeFile;

					HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID);
					if (hProc != NULL)
					{
						DWORD exit_code = 0;
						if (GetExitCodeProcess(hProc, &exit_code) && exit_code == STILL_ACTIVE)
						{
							info.is_accessible = true;

							BOOL is_wow64 = FALSE;
							if (IsWow64Process(hProc, &is_wow64))
							{
								info.arch = is_wow64 ? "x86" : "x64";
							}
						}
						CloseHandle(hProc);
					}

					list.push_back(info);
				} while (Process32Next(snapshot, &entry));
			}

			CloseHandle(snapshot);
			return list;
		}

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
						HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID);
						if (hProc != NULL)
						{
							DWORD exit_code = 0;
							bool active = (GetExitCodeProcess(hProc, &exit_code) && exit_code == STILL_ACTIVE);
							CloseHandle(hProc);
							if (active)
							{
								CloseHandle(snapshot);
								return true;
							}
						}
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
						// Verify process is actually active and accessible (not a zombie)
						HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID);
						if (hProc != NULL)
						{
							DWORD exit_code = 0;
							bool active = (GetExitCodeProcess(hProc, &exit_code) && exit_code == STILL_ACTIVE);
							CloseHandle(hProc);
							if (active)
							{
								CloseHandle(snapshot);
								return entry.th32ProcessID;
							}
						}
					}
				} while (Process32Next(snapshot, &entry));
			}

			CloseHandle(snapshot);
			return 0;
		}
	};
}