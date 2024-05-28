// basic injector
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <ctime>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

void whatcolorisyourbugatti(WORD color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void print_time(const char* message, WORD color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE) {
    time_t now = time(0);
    tm timeinfo;
    localtime_s(&timeinfo, &now);
    char timeStr[9];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

    whatcolorisyourbugatti(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    printf("[%s] ", timeStr);
    whatcolorisyourbugatti(color);
    printf("%s\n", message);
}

DWORD pid(const wchar_t* processName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32W processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32W);

    if (!Process32FirstW(snapshot, &processEntry)) {
        CloseHandle(snapshot);
        return 0;
    }

    do {
        if (wcscmp(processEntry.szExeFile, processName) == 0) {
            DWORD processId = processEntry.th32ProcessID;
            CloseHandle(snapshot);
            return processId;
        }
    } while (Process32NextW(snapshot, &processEntry));

    CloseHandle(snapshot);
    return 0;
}

bool inject(DWORD processId, const char* dllPath) {
    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!process) {
        return false;
    }

    LPVOID allocatedMemory = VirtualAllocEx(process, NULL, strlen(dllPath) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!allocatedMemory) {
        CloseHandle(process);
        return false;
    }

    if (!WriteProcessMemory(process, allocatedMemory, dllPath, strlen(dllPath) + 1, NULL)) {
        VirtualFreeEx(process, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }

    HANDLE thread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, allocatedMemory, 0, NULL);
    if (!thread) {
        VirtualFreeEx(process, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }

    WaitForSingleObject(thread, INFINITE);

    VirtualFreeEx(process, allocatedMemory, 0, MEM_RELEASE);
    CloseHandle(thread);
    CloseHandle(process);

    return true;
}


int main() {
    char dllPath[MAX_PATH];
    // Replace dll.dll with your dll
    GetFullPathNameA("dll.dll", MAX_PATH, dllPath, NULL);
    // Put the target process here
    const wchar_t* targetProcessName = L"example.exe";
    DWORD processId = pid(targetProcessName);
    print_time("Found process");
    print_time("Injected DLL");
    if (processId == 0) {
        printf("Process Not Found");
        Sleep(3000);
        return 1;
    }

    if (!inject(processId, dllPath)) {
        printf("error");
        return 1;
    }
     

    Sleep(4000);
    return 0;
}
