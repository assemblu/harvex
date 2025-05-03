#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <vector>
#include <stdexcept>

namespace mem
{
    HANDLE hproc = nullptr;
    DWORD proc_id = 0;

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

    bool Attach(const wchar_t* proc_name, DWORD access_rights = PROCESS_ALL_ACCESS)
    {
        proc_id = GetProcId(proc_name);
        if (proc_id == 0)
            return false;
        hproc = OpenProcess(access_rights, FALSE, proc_id);
        if (hproc == nullptr || hproc == INVALID_HANDLE_VALUE)
        {
            hproc = nullptr;
            proc_id = 0;
            return false;
        }

        return true;
    }

    void Detach()
    {
        if (hproc != nullptr)
        {
            if (hproc != nullptr)
            {
                CloseHandle(hproc);
                hproc = nullptr;
                proc_id = 0;
            }
        }
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
    T Read(uintptr_t address)
    {
        T value;
        if (!hproc || !ReadProcessMemory(hproc, (LPCVOID)address, &value, sizeof(T), nullptr))
        {
            return T{};
        }

        return value;
    }

    template<typename T>
    bool Write(uintptr_t address, T value)
    {
        if (!hproc) return false;
        return WriteProcessMemory(hproc, (LPVOID)address, &value, sizeof(T), nullptr);
    }

    std::string ReadString(uintptr_t address, size_t max_len = 256)
    {
        std::vector<char> buffer(max_len);
        if (!hproc || !ReadProcessMemory(hproc, (LPVOID)address, buffer.data(), max_len, nullptr))
        {
            return "";
        }
        buffer[max_len - 1] = '\0';
        size_t len = strnlen(buffer.data(), max_len);
        return std::string(buffer.data(), len);
    }

    uintptr_t FindPattern(uintptr_t start, size_t size, const char* pattern, const char* mask)
    {
        std::vector<BYTE> buffer(size);
        SIZE_T bytesRead;


        if (!hproc || !ReadProcessMemory(hproc, (LPCVOID)start, buffer.data(), size, &bytesRead))
        {
            return 0;
        }

        size_t read_size = (bytesRead <= size) ? bytesRead : size;
        size_t pattern_len = strlen(mask);

		for (size_t i = 0; (i + pattern_len) <= read_size; i++)
		{
			bool found = true;
            for (size_t j  = 0; j < pattern_len; j++)
			{
                if (mask[j] != '?' && mask[j] != 'x') continue;
				if (mask[j] == 'x' && buffer[i + j] != static_cast<BYTE>(pattern[j]))
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

        return 0;
    }
}
