#pragma once

#include <Windows.h>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace gottvergessen
{
	HANDLE acquire_process_handle(
	    std::uint32_t pid,
	    DWORD desired_access,
	    bool prefer_hijacked);

	void* get_remote_proc_address(
	    std::uint32_t pid,
	    const wchar_t* module_name,
	    const char* procedure_name);

	std::vector<DWORD> get_process_thread_ids(std::uint32_t pid);

	bool is_module_loaded(
	    std::uint32_t pid,
	    const std::filesystem::path& module_path);
}
