#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <cstdint>
#include <string>

class Memory {
private:
    HANDLE processHandle;
    DWORD processId;
    uintptr_t moduleBase;

public:
    Memory(const wchar_t* processName, const wchar_t* moduleName) : processHandle(NULL), processId(0), moduleBase(0) {
        processId = GetProcessIdByName(processName);
        if (processId) {
            processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
            if (processHandle) {
                moduleBase = GetModuleBaseAddress(moduleName);
            }
        }
    }

    ~Memory() {
        if (processHandle) {
            CloseHandle(processHandle);
        }
    }

    std::string ReadString(uintptr_t address, size_t maxLength) {
        std::vector<char> buffer(maxLength);
        if (ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), buffer.data(), maxLength, nullptr)) {
            return std::string(buffer.data());
        }
        return "Unknown"; // Fallback if reading fails
    }

    bool ReadRaw(uintptr_t address, void* buffer, size_t size) {
        return ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), buffer, size, nullptr);
    }

    template<typename T>
    T Read(uintptr_t address) {
        T value{};
        if (!ReadRaw(address, &value, sizeof(T))) {
            // Handle error or return default value
        }
        return value;
    }


    template<typename T>
    bool Write(uintptr_t address, T value) {
        return WriteProcessMemory(processHandle, (LPVOID)address, &value, sizeof(T), nullptr);
    }


    template<typename T>
    bool ReadArray(uintptr_t address, T* buffer, size_t size) {
        return ReadProcessMemory(processHandle, (LPCVOID)address, buffer, size, nullptr);
    }


    uintptr_t ReadPointer(uintptr_t base, const std::vector<uintptr_t>& offsets) {
        uintptr_t addr = base;
        for (uintptr_t offset : offsets) {
            addr = Read<uintptr_t>(addr);
            if (!addr) return 0;
            addr += offset;
        }
        return addr;
    }


    uintptr_t GetModuleBase() const { return moduleBase; }


    bool IsValid() const { return processHandle != NULL && moduleBase != 0; }

private:

    DWORD GetProcessIdByName(const wchar_t* processName) {
        DWORD processId = 0;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W processEntry;
            processEntry.dwSize = sizeof(processEntry);
            if (Process32FirstW(snapshot, &processEntry)) {
                do {
                    if (_wcsicmp(processEntry.szExeFile, processName) == 0) {
                        processId = processEntry.th32ProcessID;
                        break;
                    }
                } while (Process32NextW(snapshot, &processEntry));
            }
            CloseHandle(snapshot);
        }
        return processId;
    }


    uintptr_t GetModuleBaseAddress(const wchar_t* moduleName) {
        uintptr_t moduleBase = 0;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
        if (snapshot != INVALID_HANDLE_VALUE) {
            MODULEENTRY32W moduleEntry;
            moduleEntry.dwSize = sizeof(moduleEntry);
            if (Module32FirstW(snapshot, &moduleEntry)) {
                do {
                    if (_wcsicmp(moduleEntry.szModule, moduleName) == 0) {
                        moduleBase = (uintptr_t)moduleEntry.modBaseAddr;
                        break;
                    }
                } while (Module32NextW(snapshot, &moduleEntry));
            }
            CloseHandle(snapshot);
        }
        return moduleBase;
    }
};