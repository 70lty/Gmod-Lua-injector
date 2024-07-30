#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <shlwapi.h> 
#pragma comment(lib, "shlwapi.lib")

DWORD GetProcId(const wchar_t* procName) {
    DWORD procId = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 procEntry;
        procEntry.dwSize = sizeof(procEntry);
        if (Process32First(hSnapshot, &procEntry)) {
            do {
                if (!_wcsicmp(procEntry.szExeFile, procName)) {
                    procId = procEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnapshot, &procEntry));
        }
    }
    CloseHandle(hSnapshot);
    return procId;
}

bool InjectDLL(DWORD procId, const wchar_t* dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);
    if (hProcess && hProcess != INVALID_HANDLE_VALUE) {
        void* loc = VirtualAllocEx(hProcess, 0, (wcslen(dllPath) + 1) * sizeof(wchar_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (loc) {
            if (WriteProcessMemory(hProcess, loc, dllPath, (wcslen(dllPath) + 1) * sizeof(wchar_t), 0)) {
                HANDLE hThread = CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryW, loc, 0, 0);
                if (hThread) {
                    CloseHandle(hThread);
                    CloseHandle(hProcess);
                    return true;
                }
            }
            VirtualFreeEx(hProcess, loc, 0, MEM_RELEASE);
        }
        CloseHandle(hProcess);
    }
    return false;
}

int main() {
    SetConsoleTitle(L"ZLua Injector");

    const wchar_t* defaultDllPath = L"C:\\Users\\zolty\\source\\repos\\ZluaDlll\\x64\\Debug\\ZluaDlll.dll";
    const wchar_t* procName = L"gmod.exe";

    wchar_t dllPath[MAX_PATH];
    wcscpy_s(dllPath, defaultDllPath);

    if (!PathFileExists(dllPath)) {
        std::wcout << L"ZluaDll non trouvé, voulez-vous injecter votre propre DLL ? (Y/N) ";
        wchar_t choice;
        std::wcin >> choice;

        if (choice == L'Y' || choice == L'y') {
            std::wcout << L"Veuillez entrer le chemin de votre DLL : ";
            std::wcin.ignore();
            std::wcin.getline(dllPath, MAX_PATH);

            if (!PathFileExists(dllPath)) {
                std::wcout << L"Erreur : Fichier DLL non trouvé !" << std::endl;
                MessageBeep(MB_ICONHAND);
                std::this_thread::sleep_for(std::chrono::seconds(1));
                return 1;
            }
        }
        else {
            std::wcout << L"Opération annulée." << std::endl;
            return 1;
        }
    }

    std::wcout << L"En attente de gmod.exe..." << std::endl;

    DWORD procId = 0;
    while (procId == 0) {
        procId = GetProcId(procName);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::wcout << L"Processus trouvé avec PID: " << procId << std::endl;

    if (InjectDLL(procId, dllPath)) {
        std::wcout << L"Injection réussie !" << std::endl;
        MessageBeep(MB_OK);
    }
    else {
        std::wcout << L"Échec de l'injection." << std::endl;
        MessageBeep(MB_ICONHAND);
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}
