#pragma once

#define USE_BASE_PLATFORM_APPLE_PROCESS
#include "../Apple/PlatformAppleProcess.h"

OBJC_CLASS(NSTask);
OBJC_CLASS(NSPipe);

BE_NAMESPACE_BEGIN

struct ProcessHandle {
    ProcessHandle(void *task = NULL) {
        this->task = task;
        this->stdOutPipe = NULL;
    }
    
    bool IsValid() const { return task != NULL; }
    
    void Close();
    
    void *task;
    void *stdOutPipe;
};

class BE_API PlatformMacOSProcess : public PlatformAppleProcess {
public:
    static const wchar_t *      ComputerName();
    static const wchar_t *      UserName();

    static ProcessHandle        CreateProccess(const wchar_t *appPath, const wchar_t *args, const wchar_t *workingPath);
    static bool                 IsProccessRunning(ProcessHandle &processHandle);
    static void                 WaitForProccess(ProcessHandle &processHandle);
    static void                 TerminateProccess(ProcessHandle &processHandle);
    static bool                 GetProcessExitCode(ProcessHandle &processHandle, int32_t *exitCode);
    static bool                 ReadProcessOutput(ProcessHandle &processHandle, int bufferLength, char *buffer);
};

typedef PlatformMacOSProcess    PlatformProcess;

BE_NAMESPACE_END