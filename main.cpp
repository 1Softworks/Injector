// basic injector
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

void print(const char* msg, WORD c = 7) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), c);
    printf("%s", msg);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

DWORD get_pid(const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(pe32);
    DWORD pid = 0;

    if (Process32FirstW(snap, &pe32)) {
        do {
            if (_wcsicmp(pe32.szExeFile, name) == 0) {
                pid = pe32.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe32));
    }

    CloseHandle(snap);
    return pid;
}

bool inject(DWORD pid, const char* dll) {
    HANDLE proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!proc) return false;

    SIZE_T len = strlen(dll) + 1;
    LPVOID mem = VirtualAllocEx(proc, NULL, len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!mem) {
        CloseHandle(proc);
        return false;
    }

    if (!WriteProcessMemory(proc, mem, dll, len, NULL)) {
        VirtualFreeEx(proc, mem, 0, MEM_RELEASE);
        CloseHandle(proc);
        return false;
    }

    HANDLE thread = CreateRemoteThread(proc, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, mem, 0, NULL);
    if (!thread) {
        VirtualFreeEx(proc, mem, 0, MEM_RELEASE);
        CloseHandle(proc);
        return false;
    }

    WaitForSingleObject(thread, INFINITE);
    VirtualFreeEx(proc, mem, 0, MEM_RELEASE);
    CloseHandle(thread);
    CloseHandle(proc);
    return true;
}

int main() {
    SetConsoleTitleA("injector [https://github.com/1Softworks/Injector]");

    char process[MAX_PATH];
    char dll_path[MAX_PATH];
    wchar_t wprocess[MAX_PATH];

    print("enter process name: ", 11);
    scanf_s("%s", process, (unsigned)sizeof(process));
    MultiByteToWideChar(CP_UTF8, 0, process, -1, wprocess, MAX_PATH);

    OPENFILENAMEA ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = "dll files\0*.dll\0all files\0*.*\0";
    ofn.lpstrFile = dll_path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "dll";
    dll_path[0] = '\0';

    print("\nselect dll file...\n", 11);
    if (!GetOpenFileNameA(&ofn)) {
        print("no dll selected\n", 12);
        Sleep(2000);
        return 1;
    }

    print("\nwaiting for process...\n", 11);
    DWORD pid = 0;
    while (!pid) {
        pid = get_pid(wprocess);
        Sleep(100);
    }

    print("process found! injecting...\n", 10);

    if (!inject(pid, dll_path)) {
        print("injection failed\n", 12);
        Sleep(2000);
        return 1;
    }

    print("injection successful!\n", 10);
    Sleep(2000);
    return 0;
}
