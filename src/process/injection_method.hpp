#include "common.hpp"

namespace gottvergessen
{
	class injection_method
	{
	protected:
		std::uint32_t pid{};
		std::string name{};
	public:
		void set_target_process(const std::string process) { name = process; }
		std::string get_target_process() const { return name; }
		bool is_process_running()
		{
			auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (snapshot == INVALID_HANDLE_VALUE)
				return false;

			auto entry = PROCESSENTRY32{ sizeof(PROCESSENTRY32) };

			if (Process32First(snapshot, &entry))
			{
				do
				{
					if (!_stricmp(entry.szExeFile, name.c_str()))
					{
						CloseHandle(snapshot);
						return true;
					}
				} while (Process32Next(snapshot, &entry));
			}

			CloseHandle(snapshot);
			return false;
		}

		std::uint32_t get_process_id_by_name()
		{
			auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (snapshot == INVALID_HANDLE_VALUE)
				return false;

			auto entry = PROCESSENTRY32{ sizeof(PROCESSENTRY32) };

			if (Process32First(snapshot, &entry))
			{
				do
				{
					if (!_stricmp(entry.szExeFile, name.c_str()))
					{
						CloseHandle(snapshot);
						return entry.th32ProcessID;
					}
				} while (Process32Next(snapshot, &entry));
			}

			CloseHandle(snapshot);
			return 0;
		}

		virtual bool create_remote_thread(std::string file_name) = 0;
		//virtual bool thread_execution_hijacking(std::string file_name) = 0;
	};
}