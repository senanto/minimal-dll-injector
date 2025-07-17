#include <windows.h>
#include <iostream>
#include <string>

bool InjectDLL(DWORD pid, const std::string& dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        std::cerr << "OpenProcess failed" << GetLastError() << "\n";
        return false;
    }

    LPVOID allocAddr = VirtualAllocEx(hProcess, nullptr, dllPath.length() + 1,
                                      MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!allocAddr) {
        std::cerr << "VirtualAllocEx failed" << GetLastError() << "\n";
        CloseHandle(hProcess);
        return false;
    }

    if (!WriteProcessMemory(hProcess, allocAddr, dllPath.c_str(), dllPath.length() + 1, nullptr)) {
        std::cerr << "WriteProcessMemory failed" << GetLastError() << "\n";
        VirtualFreeEx(hProcess, allocAddr, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    LPTHREAD_START_ROUTINE pLoadLibrary = (LPTHREAD_START_ROUTINE)
        GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");

    if (!pLoadLibrary) {
        std::cerr << "LoadLibraryA address unavailable\n";
        VirtualFreeEx(hProcess, allocAddr, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, pLoadLibrary,
                                        allocAddr, 0, nullptr);
    if (!hThread) {
        std::cerr << "CreateRemoteThread failed" << GetLastError() << "\n";
        VirtualFreeEx(hProcess, allocAddr, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);

    VirtualFreeEx(hProcess, allocAddr, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    return true;
}

int main() {
    std::cout << "PID?\n> ";
    DWORD pid;
    std::cin >> pid;

    std::cin.ignore(); 

    std::cout << "DLL PATH LOCATION?\n> ";
    std::string dllPath;
    std::getline(std::cin, dllPath);

    std::cout << "\nPID: " << pid << "\nDLL: " << dllPath << "\n";
    std::cout << "\nDLL injection started...\n";

    if (InjectDLL(pid, dllPath)) {
        std::cout << "\nDLL injected.\n";
    } else {
        std::cerr << "\nDLL injection failed\n";
    }

    system("pause");
    return 0;
}
