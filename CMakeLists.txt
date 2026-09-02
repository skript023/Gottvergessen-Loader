cmake_minimum_required(VERSION 3.20)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(GottvergessenLoader CXX ASM_MASM)

option(BUILD_IMGUI_FRONTEND "Build the legacy ImGui executable" OFF)

set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")

# Git commit embed
include(scripts/git.cmake)

# Fetch modules
message("\nFetching modules")
include(scripts/g3log.cmake)
include(scripts/json.cmake)
include(scripts/cpr.cmake)
if(BUILD_IMGUI_FRONTEND)
    include(scripts/stbi.cmake)
    include(scripts/imgui.cmake)
endif()


if(BUILD_IMGUI_FRONTEND)
    message(STATUS "GottvergessenLoader legacy ImGui frontend")
    file(GLOB_RECURSE SRC_MAIN
        "${SRC_DIR}/**.hpp"
        "${SRC_DIR}/**.h"
        "${SRC_DIR}/**.cpp"
        "${SRC_DIR}/**.cc"
        "${SRC_DIR}/**.cxx"
        "${SRC_DIR}/**.asm"
    )
    list(FILTER SRC_MAIN EXCLUDE REGEX "/electron/")

    if(MSVC)
      add_compile_options(/bigobj)
    else()
      add_compile_options(-Wa,-mbig-obj)
    endif()

    if(CMAKE_SYSTEM_NAME MATCHES "Windows")
      enable_language("RC")
      set(ICON "${SRC_DIR}/gottvergessen_loader.rc")
    endif()

    add_executable(GottvergessenLoader ${SRC_MAIN} ${ICON})
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set_property(TARGET GottvergessenLoader PROPERTY CXX_STANDARD 23)
    source_group(TREE ${SRC_DIR} PREFIX "src" FILES ${SRC_MAIN})
    target_include_directories(GottvergessenLoader PRIVATE
        "${SRC_DIR}"
        "${json_SOURCE_DIR}/single_include"
        "${stbi_SOURCE_DIR}"
        "${imgui_SOURCE_DIR}"
    )
    target_precompile_headers(GottvergessenLoader PRIVATE "${SRC_DIR}/common.hpp")
    target_link_libraries(GottvergessenLoader PRIVATE g3log imgui cpr crypt32 bcrypt)
    set_property(TARGET GottvergessenLoader PROPERTY COMPILE_WARNING_AS_ERROR ON)
    target_compile_definitions(GottvergessenLoader PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

# Standard C ABI DLL loaded by Electron through FFI. It is a normal CMake
# target and has no dependency on Node.js or Electron headers.
if(WIN32)
    set(NATIVE_DLL_SOURCES
        "${SRC_DIR}/electron/native_dll.cpp"
        "${SRC_DIR}/process/injection.cpp"
        "${SRC_DIR}/process/methods/remote_thread_injection.cpp"
        "${SRC_DIR}/process/methods/thread_hijack_injection.cpp"
        "${SRC_DIR}/process/methods/manual_map_injection.cpp"
        "${SRC_DIR}/process/methods/reflective_injection.cpp"
        "${SRC_DIR}/file_manager.cpp"
        "${SRC_DIR}/file_manager/file.cpp"
        "${SRC_DIR}/file_manager/folder.cpp"
        "${SRC_DIR}/api/encrypted_downloader.cpp"
        "${SRC_DIR}/crypto/win_aes.cpp"
    )

    add_library(GottvergessenNative SHARED ${NATIVE_DLL_SOURCES})
    set_target_properties(GottvergessenNative PROPERTIES
        OUTPUT_NAME "native-core"
        CXX_STANDARD 23
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    target_compile_definitions(GottvergessenNative PRIVATE
        GOTTVERGESSEN_NATIVE_ADDON
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    target_include_directories(GottvergessenNative PRIVATE
        "${SRC_DIR}"
        "${json_SOURCE_DIR}/single_include"
    )
    target_link_libraries(GottvergessenNative PRIVATE
        g3log
        cpr
        crypt32
        bcrypt
    )
endif()
