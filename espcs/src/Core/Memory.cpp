#include "Memory.hpp"
#include <tlhelp32.h>

MemoryManager::MemoryManager() 
    : m_process_handle(nullptr), 
      m_process_id(0), 
      m_window_handle(nullptr), 
      m_attached(false),
      m_NtOpenProcess(nullptr), 
      m_NtReadVirtualMemory(nullptr), 
      m_NtWriteVirtualMemory(nullptr),
      m_NtQuerySystemInformation(nullptr), 
      m_NtQueryInformationProcess(nullptr) {
}

MemoryManager::~MemoryManager() {
    Detach();
}

bool MemoryManager::InitializeNtApi() {
    std::cout << "[Memory] Initializing NT API..." << std::endl;
    
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (!ntdll) {
        std::cout << "[Memory] Failed to get ntdll.dll handle" << std::endl;
        return false;
    }
    
    // Get NT API function addresses
    m_NtOpenProcess = reinterpret_cast<NtOpenProcessFunc>(
        GetProcAddress(ntdll, "NtOpenProcess"));
    
    m_NtReadVirtualMemory = reinterpret_cast<NtReadVirtualMemoryFunc>(
        GetProcAddress(ntdll, "NtReadVirtualMemory"));
    
    m_NtWriteVirtualMemory = reinterpret_cast<NtWriteVirtualMemoryFunc>(
        GetProcAddress(ntdll, "NtWriteVirtualMemory"));
    
    m_NtQuerySystemInformation = reinterpret_cast<NtQuerySystemInformationFunc>(
        GetProcAddress(ntdll, "NtQuerySystemInformation"));
    
    m_NtQueryInformationProcess = reinterpret_cast<NtQueryInformationProcessFunc>(
        GetProcAddress(ntdll, "NtQueryInformationProcess"));
    
    // Check if all functions were found
    if (!m_NtOpenProcess || !m_NtReadVirtualMemory || !m_NtWriteVirtualMemory || 
        !m_NtQuerySystemInformation || !m_NtQueryInformationProcess) {
        std::cout << "[Memory] Failed to get NT API function addresses" << std::endl;
        return false;
    }
    
    std::cout << "[Memory] NT API initialized successfully" << std::endl;
    return true;
}

bool MemoryManager::AttachToProcess(const std::string& process_name) {
    std::cout << "[Memory] Attempting to attach to process: " << process_name << std::endl;
    
    // Initialize NT API first
    if (!InitializeNtApi()) {
        return false;
    }
    
    m_process_name = process_name;
    
    // Find process using NT API
    m_process_id = FindProcessIdNt(process_name);
    if (m_process_id == 0) {
        std::cout << "[Memory] Process not found: " << process_name << std::endl;
        return false;
    }
    
    std::cout << "[Memory] Found process ID: " << m_process_id << std::endl;
    
    // Open process using NT API
    if (!OpenProcessNt(m_process_id)) {
        std::cout << "[Memory] Failed to open process handle" << std::endl;
        return false;
    }
    
    // Find window handle (still using Win32 API for this)
    m_window_handle = FindWindowHandle(m_process_id);
    if (m_window_handle == nullptr) {
        std::cout << "[Memory] Warning: Could not find window handle" << std::endl;
    }
    
    m_attached = true;
    std::cout << "[Memory] Successfully attached to " << process_name << " using NT API" << std::endl;
    return true;
}

void MemoryManager::Detach() {
    if (m_process_handle) {
        CloseHandle(m_process_handle);
        m_process_handle = nullptr;
    }
    
    m_process_id = 0;
    m_window_handle = nullptr;
    m_attached = false;
    
    if (!m_process_name.empty()) {
        std::cout << "[Memory] Detached from " << m_process_name << std::endl;
        m_process_name.clear();
    }
}

bool MemoryManager::ReadRaw(Address address, void* buffer, size_t size) {
    if (!m_attached || !m_process_handle || !m_NtReadVirtualMemory) {
        return false;
    }
    
    SIZE_T bytes_read = 0;
    NTSTATUS status = m_NtReadVirtualMemory(
        m_process_handle,
        reinterpret_cast<PVOID>(address),
        buffer,
        size,
        &bytes_read
    );
    
    return NT_SUCCESS(status) && (bytes_read == size);
}

bool MemoryManager::WriteRaw(Address address, const void* buffer, size_t size) {
    if (!m_attached || !m_process_handle || !m_NtWriteVirtualMemory) {
        return false;
    }
    
    SIZE_T bytes_written = 0;
    NTSTATUS status = m_NtWriteVirtualMemory(
        m_process_handle,
        reinterpret_cast<PVOID>(address),
        const_cast<PVOID>(buffer),
        size,
        &bytes_written
    );
    
    return NT_SUCCESS(status) && (bytes_written == size);
}

DWORD MemoryManager::FindProcessIdNt(const std::string& process_name) {
    if (!m_NtQuerySystemInformation) {
        return 0;
    }
    
    std::cout << "[Memory] Searching for process using NT API..." << std::endl;
    
    // Convert process name to wide string for comparison
    std::wstring wide_process_name = StringToWideString(process_name);
    
    // Get system process information
    ULONG buffer_size = 0x10000; // Start with 64KB
    std::unique_ptr<BYTE[]> buffer;
    NTSTATUS status;
    
    do {
        buffer = std::make_unique<BYTE[]>(buffer_size);
        status = m_NtQuerySystemInformation(
            CustomSystemProcessInformation, // Use our custom enum value
            buffer.get(),
            buffer_size,
            &buffer_size
        );
        
        if (status == STATUS_INFO_LENGTH_MISMATCH) {
            buffer_size *= 2; // Double the buffer size
        }
    } while (status == STATUS_INFO_LENGTH_MISMATCH);
    
    if (!NT_SUCCESS(status)) {
        std::cout << "[Memory] NtQuerySystemInformation failed: 0x" << std::hex << status << std::endl;
        return 0;
    }
    
    // Parse process information
    PMY_SYSTEM_PROCESS_INFO process_info = reinterpret_cast<PMY_SYSTEM_PROCESS_INFO>(buffer.get());
    
    while (true) {
        if (process_info->ImageName.Buffer && process_info->ImageName.Length > 0) {
            std::wstring current_name(process_info->ImageName.Buffer, 
                                    process_info->ImageName.Length / sizeof(WCHAR));
            
            if (current_name == wide_process_name) {
                DWORD pid = static_cast<DWORD>(reinterpret_cast<uintptr_t>(process_info->UniqueProcessId));
                std::cout << "[Memory] Found process via NT API: " << process_name << " (PID: " << pid << ")" << std::endl;
                return pid;
            }
        }
        
        if (process_info->NextEntryOffset == 0) {
            break;
        }
        
        process_info = reinterpret_cast<PMY_SYSTEM_PROCESS_INFO>(
            reinterpret_cast<BYTE*>(process_info) + process_info->NextEntryOffset);
    }
    
    return 0;
}

bool MemoryManager::OpenProcessNt(DWORD process_id) {
    if (!m_NtOpenProcess) {
        return false;
    }
    
    MY_CLIENT_ID client_id = {};
    client_id.UniqueProcess = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(process_id));
    client_id.UniqueThread = nullptr;
    
    MY_OBJECT_ATTRIBUTES obj_attr = {};
    obj_attr.Length = sizeof(MY_OBJECT_ATTRIBUTES);
    obj_attr.RootDirectory = nullptr;
    obj_attr.ObjectName = nullptr;
    obj_attr.Attributes = 0;
    obj_attr.SecurityDescriptor = nullptr;
    obj_attr.SecurityQualityOfService = nullptr;
    
    NTSTATUS status = m_NtOpenProcess(
        &m_process_handle,
        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION,
        &obj_attr,
        &client_id
    );
    
    if (!NT_SUCCESS(status)) {
        std::cout << "[Memory] NtOpenProcess failed: 0x" << std::hex << status << std::endl;
        
        // Try with read-only permissions
        status = m_NtOpenProcess(
            &m_process_handle,
            PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
            &obj_attr,
            &client_id
        );
        
        if (NT_SUCCESS(status)) {
            std::cout << "[Memory] Opened process with read-only permissions via NT API" << std::endl;
            return true;
        }
        
        std::cout << "[Memory] NtOpenProcess failed with read-only: 0x" << std::hex << status << std::endl;
        return false;
    }
    
    std::cout << "[Memory] Successfully opened process via NT API" << std::endl;
    return true;
}

ModuleInfo MemoryManager::GetModule(const std::string& module_name) {
    if (!m_attached) {
        return ModuleInfo();
    }
    
    std::cout << "[Memory] Searching for module: " << module_name << std::endl;
    
    // Use toolhelp32 for module enumeration (still reliable and commonly used)
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_process_id);
    if (snapshot == INVALID_HANDLE_VALUE) {
        std::cout << "[Memory] Failed to create module snapshot" << std::endl;
        return ModuleInfo();
    }
    
    MODULEENTRY32W module_entry;
    module_entry.dwSize = sizeof(MODULEENTRY32W);
    
    if (Module32FirstW(snapshot, &module_entry)) {
        do {
            // Convert wide string to regular string for comparison
            std::string current_module_name = WideStringToString(module_entry.szModule);
            
            if (current_module_name == module_name) {
                CloseHandle(snapshot);
                
                Address base = reinterpret_cast<Address>(module_entry.modBaseAddr);
                size_t size = module_entry.modBaseSize;
                
                std::cout << "[Memory] Found module " << module_name 
                         << " at 0x" << std::hex << base 
                         << " (size: " << std::dec << size << " bytes)" << std::endl;
                
                return ModuleInfo(base, size, module_name);
            }
        } while (Module32NextW(snapshot, &module_entry));
    }
    
    CloseHandle(snapshot);
    std::cout << "[Memory] Module not found: " << module_name << std::endl;
    return ModuleInfo();
}

HWND MemoryManager::FindWindowHandle(DWORD process_id) {
    HWND result_hwnd = nullptr;
    
    // Structure to pass data to the enum callback
    struct EnumData {
        DWORD target_pid;
        HWND result;
    } enum_data = { process_id, nullptr };
    
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        EnumData* data = reinterpret_cast<EnumData*>(lParam);
        DWORD window_pid;
        GetWindowThreadProcessId(hwnd, &window_pid);
        
        if (window_pid == data->target_pid && IsWindowVisible(hwnd)) {
            WCHAR window_title[256];
            if (GetWindowTextW(hwnd, window_title, sizeof(window_title)/sizeof(WCHAR)) > 0) {
                data->result = hwnd;
                return FALSE; // Stop enumeration
            }
        }
        return TRUE; // Continue enumeration
    }, reinterpret_cast<LPARAM>(&enum_data));
    
    return enum_data.result;
}

std::string MemoryManager::WideStringToString(const std::wstring& wide_string) {
    if (wide_string.empty()) {
        return std::string();
    }
    
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wide_string[0], 
                                         static_cast<int>(wide_string.size()), 
                                         nullptr, 0, nullptr, nullptr);
    std::string result(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wide_string[0], 
                       static_cast<int>(wide_string.size()), 
                       &result[0], size_needed, nullptr, nullptr);
    return result;
}

std::wstring MemoryManager::StringToWideString(const std::string& string) {
    if (string.empty()) {
        return std::wstring();
    }
    
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &string[0], 
                                         static_cast<int>(string.size()), 
                                         nullptr, 0);
    std::wstring result(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &string[0], 
                       static_cast<int>(string.size()), 
                       &result[0], size_needed);
    return result;
}