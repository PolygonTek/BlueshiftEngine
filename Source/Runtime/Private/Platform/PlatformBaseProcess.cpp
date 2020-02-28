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

#include "Precompiled.h"
#include "Platform/PlatformProcess.h"

BE_NAMESPACE_BEGIN

const char *PlatformBaseProcess::PlatformName() {
#if defined(__LINUX__) && !defined(__X86_64__)
    return "Linux-x86";
#elif defined(__LINUX__) && defined(__X86_64__)
    return "Linux-x64";
#elif defined(__FREEBSD__) && !defined(__X86_64__)
    return "FreeBSD-x86";
#elif defined(__FREEBSD__) && defined(__X86_64__)
    return "FreeBSD-x64";
#elif defined(__CYGWIN__) && !defined(__X86_64__)
    return "Cygwin-x86";
#elif defined(__CYGWIN__) && defined(__X86_64__)
    return "Cygwin-x64";
#elif defined(__WIN32__) && !defined(__X86_64__)
    return "Windows-x86";
#elif defined(__WIN32__) && defined(__X86_64__)
    return "Windows-x64";
#elif defined(__MACOSX__) && !defined(__X86_64__)
    return "macOS-x86";
#elif defined(__MACOSX__) && defined(__X86_64__)
    return "macOS-x64";
#elif defined(__IOS__) && defined(__ARM64__)
    return "iOS-arm64";
#elif defined(__IOS__) && defined(__ARM__)
    return "iOS-arm";
#elif defined(__ANDROID__) && defined(__ARM64__)
    return "Android-arm64";
#elif defined(__ANDROID__) && defined(__ARM__)
    return "Android-arm";
#elif defined(__UNIX__) && !defined(__X86_64__)
    return "Unix-x86";
#elif defined(__UNIX__) && defined(__X86_64__)
    return "Unix-x64";
#else
    return "Unknown";
#endif
}

SharedLib PlatformBaseProcess::OpenLibrary(const char *filename) {
    return nullptr;
}

void *PlatformBaseProcess::GetSymbol(SharedLib lib, const char *sym) {
    return nullptr;
}

void PlatformBaseProcess::CloseLibrary(SharedLib lib) {
}

const char *PlatformBaseProcess::ExecutableFileName() {
    return "";
}

const char *PlatformBaseProcess::ComputerName() {
    return "";
}

const char *PlatformBaseProcess::UserName() {
    return "";
}

ProcessHandle PlatformBaseProcess::CreateProccess(const char *appPath, const char *args, const char *workingPath) {
    return ProcessHandle();
}

bool PlatformBaseProcess::ExecuteProcess(const char *appPath, const char *args, const char *workingPath) {
    ProcessHandle processHandle = PlatformProcess::CreateProccess(appPath, args, workingPath);
    if (!processHandle.IsValid()) {
        return false;
    }

    int32_t exitCode;
    const int bufferSize = 32768;
    char *buffer = new char[bufferSize];
    bool firstOutput = true;

    while (1) {
        bool exit = PlatformProcess::GetProcessExitCode(processHandle, &exitCode);
        PlatformProcess::Sleep(0.01f);

        if (PlatformProcess::ReadProcessOutput(processHandle, bufferSize, buffer)) {
            if (firstOutput) {
                BE_LOG("Executing %s:\n", appPath);
                firstOutput = false;
            }
            BE_LOG("%s", buffer);
        }

        if (exit) {
            break;
        }
    }

    BE_LOG("\n");

    delete[] buffer;

    processHandle.Close();

    return true;
}

bool PlatformBaseProcess::IsProccessRunning(ProcessHandle &processHandle) {
    return false;
}

void PlatformBaseProcess::WaitForProccess(ProcessHandle &processHandle) {
}

void PlatformBaseProcess::TerminateProccess(ProcessHandle &processHandle) {
}

bool PlatformBaseProcess::GetProcessExitCode(ProcessHandle &processHandle, int32_t *exitCode) {
    return false;
}

bool PlatformBaseProcess::ReadProcessOutput(ProcessHandle &processHandle, int bufferLength, char *buffer) {
    return false;
}

void PlatformBaseProcess::Sleep(float seconds) {
}

BE_NAMESPACE_END
