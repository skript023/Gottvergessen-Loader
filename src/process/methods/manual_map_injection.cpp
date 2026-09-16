#include "process/methods/manual_map_injection.hpp"
#include "process/injector_library.hpp"
#include "logger.hpp"

namespace gottvergessen
{
	bool manual_map_injection::inject(const std::string& process_name, std::uint32_t pid, const std::filesystem::path& dll_path)
	{
		HINSTANCE injection_module = load_injector_library();
		if (!injection_module)
		{
			LOG(WARNING) << "Manual Map failed: Injector library not found.";
			return false;
		}

		auto inject_library_fn = reinterpret_cast<f_InjectA>(GetProcAddress(injection_module, "InjectA"));
		if (!inject_library_fn)
		{
			LOG(WARNING) << "Manual Map failed: InjectA function missing.";
			FreeLibrary(injection_module);
			return false;
		}

		DWORD symbol_state = 0;
		DWORD import_state = 0;
		if (!prepare_injector_library(injection_module, symbol_state, import_state))
		{
			LOG(WARNING) << "Manual Map failed: injection runtime initialization failed. "
			             << "Symbol state: 0x" << std::hex << symbol_state
			             << ", import state: 0x" << import_state;
			FreeLibrary(injection_module);
			return false;
		}

		INJECTIONDATAA data = {};
		data.ProcessID = pid;
		data.Mode = INJECTION_MODE::IM_ManualMap;
		data.Method = LAUNCH_METHOD::LM_NtCreateThreadEx;
		data.Flags = MM_DEFAULT;
		data.Timeout = 2000;
		strncpy_s(data.szDllPath, sizeof(data.szDllPath), dll_path.string().c_str(), _TRUNCATE);

		DWORD result = inject_library_fn(&data);
		if (result != 0)
			LOG(WARNING) << "Manual Map runtime returned error: 0x" << std::hex << result;
		FreeLibrary(injection_module);

		return result == 0;
	}
}
