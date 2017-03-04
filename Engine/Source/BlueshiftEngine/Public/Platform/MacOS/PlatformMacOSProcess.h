// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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