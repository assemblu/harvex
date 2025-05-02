#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <vector>

namespace mem
{
    DWORD GetProcId(const wchar_t* procName)
    {
        DWORD procId = 0;
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

        if (hSnap != INVALID_HANDLE_VALUE)
        {
            PROCESSENTRY32W procEntry;
            procEntry.dwSize = sizeof(procEntry);

            if (Process32FirstW(hSnap, &procEntry))
            {
                do
                {
                    if (!_wcsicmp(procEntry.szExeFile, procName))
                    {
                        procId = procEntry.th32ProcessID;
                        break;
                    }
                } while (Process32NextW(hSnap, &procEntry));
            }
        }
        CloseHandle(hSnap);
        return procId;
    }

    uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* modName)
    {
        uintptr_t modBaseAddr = 0;
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);

        if (hSnap != INVALID_HANDLE_VALUE)
        {
            MODULEENTRY32W modEntry;
            modEntry.dwSize = sizeof(modEntry);

            if (Module32FirstW(hSnap, &modEntry))
            {
                do
                {
                    if (!_wcsicmp(modEntry.szModule, modName))
                    {
                        modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
                        break;
                    }
                } while (Module32NextW(hSnap, &modEntry));
            }
        }
        CloseHandle(hSnap);
        return modBaseAddr;
    }

    template<typename T>
    T Read(HANDLE hProcess, uintptr_t address)
    {
        T value;
        ReadProcessMemory(hProcess, (LPCVOID)address, &value, sizeof(T), nullptr);
        return value;
    }

    template<typename T>
    bool Write(HANDLE hProcess, uintptr_t address, T value)
    {
        return WriteProcessMemory(hProcess, (LPVOID)address, &value, sizeof(T), nullptr);
    }

    std::string ReadString(HANDLE hProcess, uintptr_t address, size_t maxLength = 256)
    {
        std::vector<char> buffer(maxLength);
        if (ReadProcessMemory(hProcess, (LPCVOID)address, buffer.data(), maxLength, nullptr))
        {
            return std::string(buffer.data());
        }
        return "";
    }

    uintptr_t FindPattern(HANDLE hProcess, uintptr_t start, size_t size, const char* pattern, const char* mask)
    {
        std::vector<BYTE> buffer(size);
        SIZE_T bytesRead;

        if (ReadProcessMemory(hProcess, (LPCVOID)start, buffer.data(), size, &bytesRead))
        {
            for (size_t i = 0; i < bytesRead; i++)
            {
                bool found = true;
                for (size_t j = 0; pattern[j]; j++)
                {
                    if (mask[j] == 'x' && buffer[i + j] != (BYTE)pattern[j])
                    {
                        found = false;
                        break;
                    }
                }

                if (found)
                {
                    return start + i;
                }
            }
        }

        return 0;
    }
}
