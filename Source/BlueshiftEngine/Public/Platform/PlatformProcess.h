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

    static bool                 ExecuteProcess(const wchar_t *appPath, const wchar_t *args, const wchar_t *workingPath);

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

#if defined(__IOS__)
#include "IOS/PlatformIOSProcess.h"
#elif defined(__ANDROID__)
#include "Android/PlatformAndroidProcess.h"
#elif defined(__MACOSX__)
#include "MacOS/PlatformMacOSProcess.h"
#elif defined(__LINUX__)
#include "Linux/PlatformLinuxProcess.h"
#elif defined(__UNIX__)
#include "Posix/PlatformPosixProcess.h"
#elif defined(__WIN32__)
#include "Windows/PlatformWinProcess.h"
#endif
