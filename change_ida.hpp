// change ida window name to bypass anti debug measures
#define _CRT_SECURE_NO_WARNINGS
 
#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <string>
#include <iostream>
#include <chrono>
 
namespace logger {
    void log( const std::string& message ) {
        auto now = std::chrono::system_clock::now( );
        std::time_t now_c = std::chrono::system_clock::to_time_t( now );
 
        std::tm* current_time = std::localtime( &now_c );
        std::printf( "[%02d:%02d:%02d] %s\n" , current_time->tm_hour , current_time->tm_min , current_time->tm_sec , message.c_str( ) );
    }
    void error( const std::string& message ) {
        auto now = std::chrono::system_clock::now( );
        std::time_t now_c = std::chrono::system_clock::to_time_t( now );
 
        std::tm* current_time = std::localtime( &now_c );
        std::printf( "[! %02d:%02d:%02d !] %s\n" , current_time->tm_hour , current_time->tm_min , current_time->tm_sec , message.c_str( ) );
		system( "pause" );
    }
}
 
DWORD get_pid_from_name( const std::wstring& exe_name ) {
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS , 0 );
    PROCESSENTRY32W entry = { 0 };
    entry.dwSize = sizeof( entry );
 
    if ( Process32FirstW( snapshot , &entry ) ) {
        do {
            if ( _wcsicmp( entry.szExeFile , exe_name.c_str( ) ) == 0 ) {
                pid = entry.th32ProcessID;
                break;
            }
        } while ( Process32NextW( snapshot , &entry ) );
    }
 
    CloseHandle( snapshot );
    return pid;
}
 
BOOL CALLBACK change_window_name( HWND hwnd , LPARAM lParam ) {
    DWORD pid;
    GetWindowThreadProcessId( hwnd , &pid );
 
    if ( pid == ( DWORD ) lParam && IsWindowVisible( hwnd ) ) {
        SetWindowTextW( hwnd , L" " );
        return FALSE;
    }
    return TRUE;
}
 
int main( ) {
    std::wstring targetExe = L"ida64.exe";
    DWORD pid = get_pid_from_name( targetExe );
 
    if ( pid == 0 ) {
        logger::log( "failed to get pid." );
        logger::error( "is ida running?" );
        return 1;
    }
 
    EnumWindows( change_window_name , ( LPARAM ) pid );
    logger::log( "successfully changed ida's window title." );
    return 0;
}