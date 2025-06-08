#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <memory>
#include <string>

// Windows includes
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <iomanip>
#include <io.h>
#include <fcntl.h>

// Include our NT API memory system
#include "Core/Memory.hpp"

// Forward declarations
class CheatFramework;

// Global state
std::atomic<bool> g_running{true};
std::unique_ptr<CheatFramework> g_framework;

// Console management
bool g_console_allocated = false;

// Console control handler
BOOL WINAPI ConsoleHandler(DWORD dwType) {
    switch (dwType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
        std::wcout << L"\n[Framework] Shutting down..." << std::endl;
        g_running = false;
        return TRUE;
    }
    return FALSE;
}

// Create console for debugging
void CreateDebugConsole() {
    if (g_console_allocated) return;
    
    // Allocate a console for this GUI application
    if (AllocConsole()) {
        g_console_allocated = true;
        
        // Redirect stdout, stdin, stderr to console
        FILE* pCout;
        FILE* pCin;
        FILE* pCerr;
        
        freopen_s(&pCout, "CONOUT$", "w", stdout);
        freopen_s(&pCin, "CONIN$", "r", stdin);
        freopen_s(&pCerr, "CONOUT$", "w", stderr);
        
        // Make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
        // point to console as well
        std::ios::sync_with_stdio(true);
        
        // Set console title
        SetConsoleTitleW(L"CS2 External Cheat - Debug Console");
        
        // Set console control handler
        SetConsoleCtrlHandler(ConsoleHandler, TRUE);
        
        std::cout << "[Debug] Console allocated successfully" << std::endl;
    } else {
        MessageBoxW(NULL, L"Failed to allocate console", L"Error", MB_OK | MB_ICONERROR);
    }
}

// Cleanup console
void DestroyDebugConsole() {
    if (g_console_allocated) {
        std::cout << "[Debug] Cleaning up console..." << std::endl;
        FreeConsole();
        g_console_allocated = false;
    }
}

class CheatFramework {
private:
    std::atomic<bool> m_initialized{false};
    std::string m_target_process;
    std::unique_ptr<MemoryManager> m_memory;
    
    // Game module information
    ModuleInfo m_client_dll;
    ModuleInfo m_engine_dll;
    
public:
    explicit CheatFramework(const std::string& target_process) 
        : m_target_process(target_process) {
        m_memory = std::make_unique<MemoryManager>();
    }
    
    ~CheatFramework() {
        Shutdown();
    }
    
    bool Initialize() {
        std::cout << "[Framework] Initializing cheat framework..." << std::endl;
        std::cout << "[Framework] Target process: " << m_target_process << std::endl;
        
        // Try to attach to the process using NT API
        if (!m_memory->AttachToProcess(m_target_process)) {
            std::cout << "[Framework] Failed to attach to process. Make sure " << m_target_process << " is running." << std::endl;
            std::cout << "[Framework] Try running as administrator if the process is protected." << std::endl;
            return false;
        }
        
        // Get game modules
        if (!LoadGameModules()) {
            std::cout << "[Framework] Failed to load game modules" << std::endl;
            return false;
        }
        
        // Test memory operations
        TestMemoryOperations();
        
        m_initialized = true;
        std::cout << "[Framework] Framework initialized successfully!" << std::endl;
        return true;
    }
    
    void Run() {
        if (!m_initialized) {
            std::cerr << "[Framework] Framework not initialized!" << std::endl;
            return;
        }
        
        std::cout << "[Framework] Starting main loop..." << std::endl;
        std::cout << "[Framework] Press INSERT to toggle console, END to exit" << std::endl;
        
        while (g_running && m_initialized) {
            Update();
            Render();
            ProcessInput();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        std::cout << "[Framework] Main loop ended" << std::endl;
    }
    
    void Shutdown() {
        if (!m_initialized) return;
        
        std::cout << "[Framework] Shutting down framework..." << std::endl;
        m_initialized = false;
        
        if (m_memory) {
            m_memory->Detach();
        }
        
        std::cout << "[Framework] Framework shutdown complete" << std::endl;
    }
    
private:
    bool LoadGameModules() {
        std::cout << "[Framework] Loading game modules..." << std::endl;
        
        // Get client.dll
        m_client_dll = m_memory->GetModule("client.dll");
        if (!m_client_dll.IsValid()) {
            std::cout << "[Framework] Failed to find client.dll - make sure CS2 is running" << std::endl;
            return false;
        }
        
        // Get engine2.dll
        m_engine_dll = m_memory->GetModule("engine2.dll");
        if (!m_engine_dll.IsValid()) {
            std::cout << "[Framework] Failed to find engine2.dll" << std::endl;
            return false;
        }
        
        std::cout << "[Framework] Game modules loaded successfully!" << std::endl;
        return true;
    }
    
    void TestMemoryOperations() {
        std::cout << "[Framework] Testing NT API memory operations..." << std::endl;
        
        // Test reading from client.dll
        if (m_client_dll.IsValid()) {
            // Read DOS header
            uint16_t dos_signature = m_memory->Read<uint16_t>(m_client_dll.base_address);
            std::cout << "[Memory] client.dll DOS signature: 0x" << std::hex << dos_signature << std::dec;
            
            if (dos_signature == 0x5A4D) { // "MZ"
                std::cout << " (Valid PE file)" << std::endl;
                
                // Read more data to verify NT API is working
                uint32_t first_dword = m_memory->Read<uint32_t>(m_client_dll.base_address);
                std::cout << "[Memory] First DWORD of client.dll: 0x" << std::hex << first_dword << std::dec << std::endl;
                
                // Test reading a larger chunk
                uint8_t buffer[16];
                if (m_memory->ReadRaw(m_client_dll.base_address, buffer, sizeof(buffer))) {
                    std::cout << "[Memory] First 16 bytes: ";
                    for (int i = 0; i < 16; i++) {
                        std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(buffer[i]) << " ";
                    }
                    std::cout << std::dec << std::endl;
                } else {
                    std::cout << "[Memory] Failed to read raw memory" << std::endl;
                }
            } else {
                std::cout << " (Invalid PE file - something is wrong)" << std::endl;
            }
        }
        
        // Test reading from engine2.dll
        if (m_engine_dll.IsValid()) {
            uint16_t dos_signature = m_memory->Read<uint16_t>(m_engine_dll.base_address);
            std::cout << "[Memory] engine2.dll DOS signature: 0x" << std::hex << dos_signature << std::dec;
            std::cout << (dos_signature == 0x5A4D ? " (Valid)" : " (Invalid)") << std::endl;
        }
        
        std::cout << "[Framework] Memory operations test complete!" << std::endl;
    }
    
    void Update() {
        // Check if process is still running
        if (!m_memory->IsAttached()) {
            std::cout << "[Framework] Lost connection to process" << std::endl;
            g_running = false;
            return;
        }
        
        // TODO: Update game entities using NT API
        // TODO: Read player data using NT API
        // TODO: Update world-to-screen matrix using NT API
    }
    
    void Render() {
        // TODO: Render ESP elements
        // This is where you'd implement your overlay rendering
    }
    
    void ProcessInput() {
        // Exit key
        if (GetAsyncKeyState(VK_END) & 0x8000) {
            g_running = false;
        }
        
        // Toggle console visibility
        static bool insert_pressed = false;
        if (GetAsyncKeyState(VK_INSERT) & 0x8000) {
            if (!insert_pressed) {
                insert_pressed = true;
                HWND console_window = GetConsoleWindow();
                if (console_window) {
                    bool is_visible = IsWindowVisible(console_window);
                    ShowWindow(console_window, is_visible ? SW_HIDE : SW_SHOW);
                    std::cout << "[Debug] Console " << (is_visible ? "hidden" : "shown") << std::endl;
                }
            }
        } else {
            insert_pressed = false;
        }
        
        // Test key - read some memory when F1 is pressed
        static bool f1_pressed = false;
        if (GetAsyncKeyState(VK_F1) & 0x8000) {
            if (!f1_pressed) {
                f1_pressed = true;
                std::cout << "[Framework] F1 pressed - performing NT API memory test..." << std::endl;
                TestMemoryOperations();
            }
        } else {
            f1_pressed = false;
        }
    }
};

// Windows application entry point
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    // Suppress unused parameter warnings
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);
    
    // Create debug console first
    CreateDebugConsole();
    
    std::cout << "========================================" << std::endl;
    std::cout << "   CS2 External Cheat Framework" << std::endl;
    std::cout << "     Windows App + NT API" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "[Framework] Starting up..." << std::endl;
    std::cout << "[Debug] This is a Windows application with debug console" << std::endl;
    std::cout << "[Debug] Press INSERT to toggle console visibility" << std::endl;
    
    try {
        // Create framework instance
        g_framework = std::make_unique<CheatFramework>("cs2.exe");
        
        // Initialize the framework
        if (!g_framework->Initialize()) {
            std::cerr << "[Framework] Failed to initialize framework!" << std::endl;
            std::cout << "\nMake sure:" << std::endl;
            std::cout << "1. CS2 is running" << std::endl;
            std::cout << "2. You're running as administrator" << std::endl;
            std::cout << "3. Anti-virus isn't blocking the application" << std::endl;
            
            MessageBoxW(NULL, L"Failed to initialize framework! Check console for details.", L"Error", MB_OK | MB_ICONERROR);
            DestroyDebugConsole();
            return -1;
        }
        
        // Run the main loop
        g_framework->Run();
        
    } catch (const std::exception& e) {
        std::cerr << "[Framework] Exception: " << e.what() << std::endl;
        
        // Convert exception message to wide string for MessageBox
        std::string error_msg = "Exception: " + std::string(e.what());
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, error_msg.c_str(), -1, nullptr, 0);
        std::wstring wide_error_msg(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, error_msg.c_str(), -1, &wide_error_msg[0], size_needed);
        
        MessageBoxW(NULL, wide_error_msg.c_str(), L"Error", MB_OK | MB_ICONERROR);
        DestroyDebugConsole();
        return -1;
    }
    
    std::cout << "[Framework] Goodbye!" << std::endl;
    
    #ifdef _DEBUG
    std::cout << "[Debug] Press Enter to exit...";
    std::cin.get();
    #endif
    
    // Cleanup
    DestroyDebugConsole();
    return 0;
}