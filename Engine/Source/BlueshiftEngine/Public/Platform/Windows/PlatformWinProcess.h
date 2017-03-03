#pragma once

#include "Core/WStr.h"

BE_NAMESPACE_BEGIN

struct ProcessHandle {
    ProcessHandle(HANDLE handle = nullptr) {
        this->handle = handle;
        this->hStdOutRead = nullptr;
        this->hStdOutWrite = nullptr;
        this->hStdInRead = nullptr;
        this->hStdInWrite = nullptr;
    }

    bool IsValid() const { return handle != nullptr; }

    void Close() {
        if (handle) {
            ::CloseHandle(handle);
            ::CloseHandle(hStdOutRead);
            ::CloseHandle(hStdOutWrite);
            ::CloseHandle(hStdInRead);
            ::CloseHandle(hStdInWrite);
            handle = nullptr;
            hStdOutRead = nullptr;
            hStdOutWrite = nullptr;
            hStdInRead = nullptr;
            hStdInWrite = nullptr;
        }
    }

    HANDLE handle;
    HANDLE hStdOutRead;
    HANDLE hStdOutWrite;
    HANDLE hStdInRead;
    HANDLE hStdInWrite;
};

class BE_API PlatformWinProcess : public PlatformBaseProcess {
public:
    static const wchar_t *      ExecutableFileName();
    static const wchar_t *		ComputerName();
    static const wchar_t *		UserName();
    static int                  NumberOfLogicalProcessors();

    static WStr					GetLastErrorText();

    static ProcessHandle        CreateProccess(const wchar_t *appPath, const wchar_t *args, const wchar_t *workingPath);
    static bool                 IsProccessRunning(ProcessHandle &processHandle);
    static void                 WaitForProccess(ProcessHandle &processHandle);
    static void                 TerminateProccess(ProcessHandle &processHandle);
    static bool                 GetProcessExitCode(ProcessHandle &processHandle, int32_t *exitCode);
    static bool                 ReadProcessOutput(ProcessHandle &processHandle, int bufferLength, char *buffer);

    static void                 Sleep(float seconds);

    // Loads a shared library
    static SharedLib            OpenLibrary(const char *filename);
    // Unloads a shared library
    static void                 CloseLibrary(SharedLib lib);
    // Returns address of a symbol from the library
    static void *               GetSymbol(SharedLib lib, const char *sym);
};

typedef PlatformWinProcess      PlatformProcess;

BE_NAMESPACE_END
