#pragma once

BE_NAMESPACE_BEGIN

// type for shared library
typedef void *SharedLib;

struct ProcessHandle;

class BE_API PlatformBaseProcess {
public:
    static const char *         PlatformName();
    static const wchar_t *      ExecutableFileName();
    static const wchar_t *      ComputerName();
    static const wchar_t *      UserName();
    static int                  NumberOfLogicalProcessors();

    static ProcessHandle        CreateProccess(const wchar_t *appPath, const wchar_t *args, const wchar_t *workingPath);
    static bool                 IsProccessRunning(ProcessHandle &processHandle);
    static void                 WaitForProccess(ProcessHandle &processHandle);
    static void                 TerminateProccess(ProcessHandle &processHandle);
    static bool                 GetProcessExitCode(ProcessHandle &processHandle, int32_t *exitCode);
    static bool                 ReadProcessOutput(ProcessHandle &processHandle, int bufferLength, char *buffer);

    static void                 Sleep(float seconds);
    
                                /// Loads a shared library
    static SharedLib            OpenLibrary(const char *filename);
                                /// Unloads a shared library
    static void                 CloseLibrary(SharedLib lib);
                                /// Returns address of a symbol from the library
    static void *               GetSymbol(SharedLib lib, const char *sym);
};

BE_NAMESPACE_END

#ifdef __WIN32__
#include "Windows/PlatformWinProcess.h"
#elif defined(__MACOSX__)
#include "MacOS/PlatformMacOSProcess.h"
#elif defined(__IOS__)
#include "IOS/PlatformIOSProcess.h"
#elif defined(__ANDROID__)
#include "Android/PlatformAndroidProcess.h"
#elif defined(__LINUX__)
#include "Linux/PlatformLinuxProcess.h"
#elif defined(__UNIX__)
#include "Posix/PlatformPosixProcess.h"
#endif
