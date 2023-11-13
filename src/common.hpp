#pragma once

#include <iostream>
#include <Windows.h>
#include <chrono>
#include <TlHelp32.h>
#include <filesystem>
#include <thread>
#include <fstream>
#include <stack>
#include <regex>
#include <format>
#include <cpr/cpr.h>

#include <logger.hpp>
#include <nlohmann/json.hpp>

#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")

using namespace std::chrono_literals;
