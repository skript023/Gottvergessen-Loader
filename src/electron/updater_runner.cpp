#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>

namespace {
    std::wstring g_log_path;

    void log_msg(const std::wstring& msg) {
        if (g_log_path.empty()) return;
        try {
            std::wofstream log(g_log_path, std::ios::app);
            if (log.is_open()) {
                auto now = std::chrono::system_clock::now();
                auto in_time_t = std::chrono::system_clock::to_time_t(now);
                std::tm bt{};
                localtime_s(&bt, &in_time_t);
                log << L"[" << std::put_time(&bt, L"%Y-%m-%d %H:%M:%S") << L"] " << msg << std::endl;
            }
        } catch (...) {}
    }
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc < 4) {
        return 1;
    }

    DWORD target_pid = static_cast<DWORD>(_wtoi(argv[1]));
    std::wstring new_exe = argv[2];
    std::wstring target_exe = argv[3];
    DWORD parent_pid = (argc >= 5) ? static_cast<DWORD>(_wtoi(argv[4])) : 0;

    // Set log file in same dir as new_exe
    size_t last_slash = new_exe.find_last_of(L"\\/");
    if (last_slash != std::wstring::npos) {
        g_log_path = new_exe.substr(0, last_slash) + L"\\update.log";
    }

    log_msg(L"=== NATIVE C++ UPDATER STARTED ===");
    log_msg(L"Target PID: " + std::to_wstring(target_pid));
    if (parent_pid > 0) {
        log_msg(L"Parent PID: " + std::to_wstring(parent_pid));
    }
    log_msg(L"New Exe: " + new_exe);
    log_msg(L"Target Exe: " + target_exe);

    // 1. Wait natively for target and parent process handles to terminate
    auto wait_and_kill = [](DWORD pid, const std::wstring& label) {
        if (pid == 0) return;
        HANDLE h_proc = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, pid);
        if (h_proc != NULL) {
            log_msg(L"Waiting for " + label + L" (PID " + std::to_wstring(pid) + L") to terminate...");
            DWORD wait_res = WaitForSingleObject(h_proc, 8000);
            if (wait_res == WAIT_TIMEOUT) {
                log_msg(label + L" (PID " + std::to_wstring(pid) + L") timed out after 8s. Terminating...");
                TerminateProcess(h_proc, 0);
                WaitForSingleObject(h_proc, 2000);
            }
            CloseHandle(h_proc);
            log_msg(label + L" handle closed.");
        }
    };

    wait_and_kill(target_pid, L"Target Process");
    if (parent_pid > 0 && parent_pid != target_pid) {
        wait_and_kill(parent_pid, L"Parent Process");
    }

    // Wait an extra 350ms for OS file locks, AV filters, and mutexes to release
    Sleep(350);

    // 2. In-place replace target executable (with rename fallback if still locked)
    bool replaced = false;

    if (_wcsicmp(new_exe.c_str(), target_exe.c_str()) == 0) {
        log_msg(L"Target and New executable paths are identical. Skipping file replacement.");
        replaced = true;
    } else {
        std::wstring old_exe = target_exe + L".old";
        DeleteFileW(old_exe.c_str());

        DWORD move_flags = MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH;

        for (int i = 1; i <= 30; ++i) {
            // Attempt direct atomic / cross-volume move
            if (MoveFileExW(new_exe.c_str(), target_exe.c_str(), move_flags)) {
                replaced = true;
                log_msg(L"Direct MoveFileExW succeeded on attempt " + std::to_wstring(i));
                break;
            }

            DWORD err_direct = GetLastError();

            // If direct replace failed due to sharing lock / access denied, try rename-to-.old technique
            if (MoveFileExW(target_exe.c_str(), old_exe.c_str(), MOVEFILE_REPLACE_EXISTING)) {
                log_msg(L"Renamed target to .old on attempt " + std::to_wstring(i));
                if (MoveFileExW(new_exe.c_str(), target_exe.c_str(), move_flags)) {
                    replaced = true;
                    log_msg(L"MoveFileExW succeeded into target after renaming old binary!");
                    if (!DeleteFileW(old_exe.c_str())) {
                        MoveFileExW(old_exe.c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
                    }
                    break;
                } else if (CopyFileW(new_exe.c_str(), target_exe.c_str(), FALSE)) {
                    replaced = true;
                    log_msg(L"CopyFileW fallback succeeded into target after renaming old binary!");
                    DeleteFileW(new_exe.c_str());
                    if (!DeleteFileW(old_exe.c_str())) {
                        MoveFileExW(old_exe.c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
                    }
                    break;
                } else {
                    // Rollback if new_exe couldn't be moved or copied
                    MoveFileExW(old_exe.c_str(), target_exe.c_str(), MOVEFILE_REPLACE_EXISTING);
                }
            } else if (CopyFileW(new_exe.c_str(), target_exe.c_str(), FALSE)) {
                replaced = true;
                log_msg(L"Direct CopyFileW succeeded on attempt " + std::to_wstring(i));
                DeleteFileW(new_exe.c_str());
                break;
            }

            log_msg(L"Replace attempt " + std::to_wstring(i) + L" failed (Error: " + std::to_wstring(err_direct) + L"). Retrying in 300ms...");
            Sleep(300);
        }
    }

    // 3. Relaunch target executable
    std::wstring exe_to_launch = replaced ? target_exe : new_exe;
    std::wstring work_dir = exe_to_launch;
    size_t dir_slash = work_dir.find_last_of(L"\\/");
    if (dir_slash != std::wstring::npos) {
        work_dir = work_dir.substr(0, dir_slash);
    }

    log_msg(L"Relaunching: " + exe_to_launch + L" in " + work_dir);

    STARTUPINFOW si{sizeof(si)};
    PROCESS_INFORMATION pi{};
    std::wstring cmd_line = L"\"" + exe_to_launch + L"\"";

    // First attempt: Breakaway from job object
    BOOL ok = CreateProcessW(
        NULL,
        cmd_line.data(),
        NULL,
        NULL,
        FALSE,
        CREATE_BREAKAWAY_FROM_JOB | CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS,
        NULL,
        work_dir.c_str(),
        &si,
        &pi
    );

    // Second attempt: without breakaway if parent job disallowed it
    if (!ok && GetLastError() == ERROR_ACCESS_DENIED) {
        log_msg(L"CreateProcessW with breakaway returned ERROR_ACCESS_DENIED. Retrying without breakaway...");
        ok = CreateProcessW(
            NULL,
            cmd_line.data(),
            NULL,
            NULL,
            FALSE,
            CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS,
            NULL,
            work_dir.c_str(),
            &si,
            &pi
        );
    }

    if (ok) {
        log_msg(L"CreateProcessW succeeded with PID " + std::to_wstring(pi.dwProcessId));
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        DWORD err = GetLastError();
        log_msg(L"CreateProcessW failed (Error: " + std::to_wstring(err) + L"). Trying ShellExecuteExW fallback...");
        SHELLEXECUTEINFOW sei{sizeof(sei)};
        sei.cbSize = sizeof(sei);
        sei.fMask = SEE_MASK_NOZONECHECKS;
        sei.lpVerb = L"open";
        sei.lpFile = exe_to_launch.c_str();
        sei.lpDirectory = work_dir.c_str();
        sei.nShow = SW_SHOWNORMAL;
        if (ShellExecuteExW(&sei)) {
            log_msg(L"ShellExecuteExW fallback succeeded.");
        } else {
            log_msg(L"ShellExecuteExW failed (Error: " + std::to_wstring(GetLastError()) + L").");
        }
    }

    log_msg(L"=== NATIVE C++ UPDATER FINISHED ===");
    LocalFree(argv);
    return 0;
}
