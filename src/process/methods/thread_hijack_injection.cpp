#include "process/methods/thread_hijack_injection.hpp"
#include "process/injector_library.hpp"
#include "logger.hpp"

namespace gottvergessen
{
	bool thread_hijack_injection::inject(const std::string& process_name, std::uint32_t pid, const std::filesystem::path& dll_path)
	{
		HINSTANCE injection_module = LoadLibraryA(GH_INJ_MOD_NAMEA);
		if (!injection_module)
		{
			LOG(WARNING) << "Thread Hijack failed: Injector library not found.";
			return false;
		}

		auto inject_library_fn = reinterpret_cast<f_InjectA>(GetProcAddress(injection_module, "InjectA"));
		if (!inject_library_fn)
		{
			LOG(WARNING) << "Thread Hijack failed: InjectA function missing.";
			FreeLibrary(injection_module);
			return false;
		}

		INJECTIONDATAA data = {};
		data.ProcessID = pid;
		data.Mode = INJECTION_MODE::IM_LdrLoadDll;
		data.Method = LAUNCH_METHOD::LM_HijackThread;
		data.Flags = INJ_HIJACK_HANDLE | INJ_FAKE_HEADER;
		data.Timeout = 2000;
		strncpy_s(data.szDllPath, sizeof(data.szDllPath), dll_path.string().c_str(), _TRUNCATE);

		DWORD result = inject_library_fn(&data);
		FreeLibrary(injection_module);

		return result == 0;
	}
}
