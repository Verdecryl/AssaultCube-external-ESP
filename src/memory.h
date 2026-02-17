#pragma once
#pragma once
#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>

class Memory
{
private:
    DWORD id = 0;
    HANDLE process = NULL;

public:
    Memory(const char* processName) {
        PROCESSENTRY32 entry;
        entry.dwSize = sizeof(PROCESSENTRY32);

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

        if (Process32First(snapshot, &entry)) {
            do {
                if (!strcmp(entry.szExeFile, processName)) {
                    this->id = entry.th32ProcessID;
                    this->process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, this->id);
                    break;
                }
            } while (Process32Next(snapshot, &entry));
        }

        if (snapshot) CloseHandle(snapshot);
    }

    ~Memory() {
        if (this->process) CloseHandle(this->process);
    }

    DWORD GetProcessId() { return this->id; }
    HANDLE GetProcessHandle() { return this->process; }

    uintptr_t GetModuleAddress(const char* moduleName) {
        uintptr_t moduleAddr = 0;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, this->id);
        if (snapshot != INVALID_HANDLE_VALUE) {
            MODULEENTRY32 entry;
            entry.dwSize = sizeof(MODULEENTRY32);
            if (Module32First(snapshot, &entry)) {
                do {
                    if (!strcmp(entry.szModule, moduleName)) {
                        moduleAddr = (uintptr_t)entry.modBaseAddr;
                        break;
                    }
                } while (Module32Next(snapshot, &entry));
            }
        }
        CloseHandle(snapshot);
        return moduleAddr;
    }

    template <typename T>
    T Read(uintptr_t address) {
        T value;
        ReadProcessMemory(this->process, (LPCVOID)address, &value, sizeof(T), NULL);
        return value;
    }

    bool Read(uintptr_t address, void* buffer, SIZE_T size) {
        return ReadProcessMemory(this->process, (LPCVOID)address, buffer, size, NULL);
    }

    void ReadMatrix(uintptr_t address, float* matrix) {
        ReadProcessMemory(this->process, (LPCVOID)address, matrix, sizeof(float) * 16, NULL);
    }

    template <typename T>
    bool Write(uintptr_t address, T value) {
        return WriteProcessMemory(this->process, (LPVOID)address, &value, sizeof(T), NULL);
    }

};
