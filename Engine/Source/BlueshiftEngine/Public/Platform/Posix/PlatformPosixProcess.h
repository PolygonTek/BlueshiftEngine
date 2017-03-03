#pragma once

BE_NAMESPACE_BEGIN

class BE_API PlatformPosixProcess : public PlatformBaseProcess {
public:
	static const wchar_t *		ComputerName();
	static const wchar_t *		UserName();
    static int                  NumberOfLogicalProcessors();
    
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

#ifndef USE_BASE_PLATFORM_POSIX_PROCESS
typedef PlatformPosixProcess    PlatformProcess;
#endif

BE_NAMESPACE_END