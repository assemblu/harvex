#pragma once

// Windows includes first
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
// DO NOT include winternl.h - it conflicts with our definitions

// Standard includes
#include <string>
#include <iostream>
#include <memory>
#include <iomanip>

// Basic types
using Address = std::uintptr_t;

// NT API Status codes and macros
#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

#ifndef STATUS_INFO_LENGTH_MISMATCH
#define STATUS_INFO_LENGTH_MISMATCH 0xC0000004L
#endif

// Forward declare NTSTATUS if not already defined
#ifndef NTSTATUS
typedef LONG NTSTATUS;
#endif

// NT API structures (define our own to avoid conflicts)
typedef struct _MY_UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} MY_UNICODE_STRING, *PMY_UNICODE_STRING;

typedef struct _MY_CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} MY_CLIENT_ID, *PMY_CLIENT_ID;

typedef struct _MY_OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PMY_UNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;
    PVOID SecurityQualityOfService;
} MY_OBJECT_ATTRIBUTES, *PMY_OBJECT_ATTRIBUTES;

// Custom System Information Classes to avoid conflicts
typedef enum _CUSTOM_SYSTEM_INFORMATION_CLASS {
    CustomSystemBasicInformation = 0,
    CustomSystemProcessInformation = 5,
    CustomSystemPerformanceInformation = 2,
    CustomSystemTimeOfDayInformation = 3,
} CUSTOM_SYSTEM_INFORMATION_CLASS;

// Custom Process Information Classes to avoid conflicts
typedef enum _CUSTOM_PROCESSINFOCLASS {
    CustomProcessBasicInformation = 0,
    CustomProcessQuotaLimits = 1,
    CustomProcessIoCounters = 2,
    CustomProcessVmCounters = 3,
    CustomProcessTimes = 4,
    CustomProcessBasePriority = 5,
    CustomProcessRaisePriority = 6,
    CustomProcessDebugPort = 7,
    CustomProcessExceptionPort = 8,
    CustomProcessAccessToken = 9,
    CustomProcessLdtInformation = 10,
    CustomProcessLdtSize = 11,
    CustomProcessDefaultHardErrorMode = 12,
    CustomProcessIoPortHandlers = 13,
    CustomProcessPooledUsageAndLimits = 14,
    CustomProcessWorkingSetWatch = 15,
    CustomProcessUserModeIOPL = 16,
    CustomProcessEnableAlignmentFaultFixup = 17,
    CustomProcessPriorityClass = 18,
    CustomProcessWx86Information = 19,
    CustomProcessHandleCount = 20,
    CustomProcessAffinityMask = 21,
    CustomProcessPriorityBoost = 22,
    CustomProcessDeviceMap = 23,
    CustomProcessSessionInformation = 24,
    CustomProcessForegroundInformation = 25,
    CustomProcessWow64Information = 26,
    CustomProcessImageFileName = 27,
    CustomProcessLUIDDeviceMapsEnabled = 28,
    CustomProcessBreakOnTermination = 29,
    CustomProcessDebugObjectHandle = 30,
    CustomProcessDebugFlags = 31,
    CustomProcessHandleTracing = 32,
    CustomProcessIoPriority = 33,
    CustomProcessExecuteFlags = 34,
    CustomProcessResourceManagement = 35,
    CustomProcessCookie = 36,
    CustomProcessImageInformation = 37,
    CustomProcessCycleTime = 38,
    CustomProcessPagePriority = 39,
    CustomProcessInstrumentationCallback = 40,
    CustomProcessThreadStackAllocation = 41,
    CustomProcessWorkingSetWatchEx = 42,
    CustomProcessImageFileNameWin32 = 43,
    CustomProcessImageFileMapping = 44,
    CustomProcessAffinityUpdateMode = 45,
    CustomProcessMemoryAllocationMode = 46,
    CustomMaxProcessInfoClass = 47
} CUSTOM_PROCESSINFOCLASS;

// System Process Information structure
typedef struct _MY_SYSTEM_PROCESS_INFO {
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    BYTE Reserved1[48];
    MY_UNICODE_STRING ImageName;
    LONG BasePriority;
    HANDLE UniqueProcessId;
    PVOID Reserved2;
    ULONG HandleCount;
    ULONG SessionId;
    PVOID Reserved3;
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG Reserved4;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    PVOID Reserved5;
    SIZE_T QuotaPagedPoolUsage;
    PVOID Reserved6;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER Reserved7[6];
} MY_SYSTEM_PROCESS_INFO, *PMY_SYSTEM_PROCESS_INFO;

// NT API function typedefs - MUST be declared before class
typedef NTSTATUS(NTAPI* NtOpenProcessFunc)(
    PHANDLE ProcessHandle,
    ACCESS_MASK DesiredAccess,
    PMY_OBJECT_ATTRIBUTES ObjectAttributes,
    PMY_CLIENT_ID ClientId
);

typedef NTSTATUS(NTAPI* NtReadVirtualMemoryFunc)(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    PVOID Buffer,
    SIZE_T BufferSize,
    PSIZE_T NumberOfBytesRead
);

typedef NTSTATUS(NTAPI* NtWriteVirtualMemoryFunc)(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    PVOID Buffer,
    SIZE_T BufferSize,
    PSIZE_T NumberOfBytesWritten
);

typedef NTSTATUS(NTAPI* NtQuerySystemInformationFunc)(
    CUSTOM_SYSTEM_INFORMATION_CLASS SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
);

typedef NTSTATUS(NTAPI* NtQueryInformationProcessFunc)(
    HANDLE ProcessHandle,
    CUSTOM_PROCESSINFOCLASS ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength,
    PULONG ReturnLength
);

// Module information structure
struct ModuleInfo {
    Address base_address;
    size_t size;
    std::string name;
    
    ModuleInfo() : base_address(0), size(0) {}
    ModuleInfo(Address base, size_t sz, const std::string& module_name) 
        : base_address(base), size(sz), name(module_name) {}
    
    bool IsValid() const { return base_address != 0 && size > 0; }
};

// NT API Memory Manager
class MemoryManager {
private:
    HANDLE m_process_handle;
    DWORD m_process_id;
    HWND m_window_handle;
    std::string m_process_name;
    bool m_attached;
    
    // NT API function pointers - using the typedefs declared above
    NtOpenProcessFunc m_NtOpenProcess;
    NtReadVirtualMemoryFunc m_NtReadVirtualMemory;
    NtWriteVirtualMemoryFunc m_NtWriteVirtualMemory;
    NtQuerySystemInformationFunc m_NtQuerySystemInformation;
    NtQueryInformationProcessFunc m_NtQueryInformationProcess;
    
public:
    MemoryManager();
    ~MemoryManager();
    
    // Core functionality
    bool AttachToProcess(const std::string& process_name);
    void Detach();
    bool IsAttached() const { return m_attached; }
    
    // Memory operations using NT API
    template<typename T>
    T Read(Address address);
    
    template<typename T>
    bool Write(Address address, const T& value);
    
    bool ReadRaw(Address address, void* buffer, size_t size);
    bool WriteRaw(Address address, const void* buffer, size_t size);
    
    // Module operations
    ModuleInfo GetModule(const std::string& module_name);
    
    // Getters
    HANDLE GetProcessHandle() const { return m_process_handle; }
    DWORD GetProcessId() const { return m_process_id; }
    HWND GetWindowHandle() const { return m_window_handle; }
    
private:
    // Initialization
    bool InitializeNtApi();
    
    // Helper functions using NT API
    DWORD FindProcessIdNt(const std::string& process_name);
    HWND FindWindowHandle(DWORD process_id);
    bool OpenProcessNt(DWORD process_id);
    
    // Utility functions
    std::string WideStringToString(const std::wstring& wide_string);
    std::wstring StringToWideString(const std::string& string);
};

// Template implementations
template<typename T>
T MemoryManager::Read(Address address) {
    T value{};
    if (!ReadRaw(address, &value, sizeof(T))) {
        return T{};
    }
    return value;
}

template<typename T>
bool MemoryManager::Write(Address address, const T& value) {
    return WriteRaw(address, &value, sizeof(T));
}