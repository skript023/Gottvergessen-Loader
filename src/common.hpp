#pragma once

#include <iostream>
#include <Windows.h>
#include <psapi.h>
#include <chrono>
#include <TlHelp32.h>
#include <dwmapi.h>
#include <filesystem>
#include <thread>
#include <fstream>
#include <stack>
#include <regex>
#include <format>
#include <cpr/cpr.h>

#include <logger.hpp>
#include <nlohmann/json.hpp>
#include "fonts/icon_list.hpp"

#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

using namespace std::chrono_literals;
namespace gottvergessen
{
	inline std::atomic_bool g_running{ true };
}