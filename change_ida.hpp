// change ida window name to bypass anti debug measures
#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <string>
#include <iostream>
#include <chrono>
#include <format>
#include <string_view>

namespace logger {
    void log(std::string_view message) {
        const auto now = std::chrono::system_clock::now();
        const auto now_c = std::chrono::system_clock::to_time_t(now);
        
        if (const std::tm* current_time = std::localtime(&now_c)) {
            std::cout << std::format("[{:02d}:{:02d}:{:02d}] {}\n", 
                current_time->tm_hour, current_time->tm_min, current_time->tm_sec, message);
        }
    }
    
    void error(std::string_view message) {
        const auto now = std::chrono::system_clock::now();
        const auto now_c = std::chrono::system_clock::to_time_t(now);
        
        if (const std::tm* current_time = std::localtime(&now_c)) {
            std::cout << std::format("[! {:02d}:{:02d}:{:02d} !] {}\n", 
                current_time->tm_hour, current_time->tm_min, current_time->tm_sec, message);
            system("pause");
        }
    }
}

[[nodiscard]] DWORD get_pid_from_name(std::wstring_view exe_name) {
    DWORD pid = 0;
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, exe_name.data()) == 0) {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return pid;
}

BOOL CALLBACK change_window_name(HWND hwnd, LPARAM lParam) {
    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);

    if (pid == static_cast<DWORD>(lParam) && IsWindowVisible(hwnd)) {
        SetWindowTextW(hwnd, L" ");
        return FALSE;
    }
    return TRUE;
}

int main() {
    constexpr std::wstring_view targetExe = L"ida64.exe";
    const DWORD pid = get_pid_from_name(targetExe);

    if (pid == 0) {
        logger::log("failed to get pid.");
        logger::error("is ida running?");
        return 1;
    }

    EnumWindows(change_window_name, static_cast<LPARAM>(pid));
    logger::log("successfully changed ida's window title.");
    return 0;
}