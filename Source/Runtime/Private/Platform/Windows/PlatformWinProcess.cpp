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
#include "Core/Str.h"
#include "Platform/PlatformProcess.h"
#include "Platform/Windows/PlatformWinProcess.h"
#include "Platform/Windows/PlatformWinUtils.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <direct.h>
#include <io.h>
#include <sys/utime.h>

#ifndef __MRC__
#include <sys/types.h>
#include <sys/stat.h>
#endif

BE_NAMESPACE_BEGIN

// get the full path to the running executable
const char *PlatformWinProcess::ExecutableFileName() {
    static char filename[1024] = "";

    if (!filename[0]) {
        wchar_t wFilename[1024];
        GetModuleFileNameW(nullptr, wFilename, COUNT_OF(filename));

        PlatformWinUtils::UCS2ToUTF8(wFilename, filename, COUNT_OF(filename));
    }
    return filename;
}

const char *PlatformWinProcess::ComputerName() {
    static char computeName[256] = "";

    if (!computeName[0]) {
        wchar_t wComputerName[256];
        DWORD size = COUNT_OF(wComputerName);
        GetComputerNameW(wComputerName, &size);

        PlatformWinUtils::UCS2ToUTF8(wComputerName, computeName, COUNT_OF(computeName));
    }
    return computeName;
}

const char *PlatformWinProcess::UserName() {
    static char userName[256] = "";

    if (!userName[0]) {
        wchar_t wUserName[256];
        DWORD size = COUNT_OF(wUserName);
        GetUserNameW(wUserName, &size);

        PlatformWinUtils::UCS2ToUTF8(wUserName, userName, COUNT_OF(userName));
    }
    return userName;
}

Str PlatformWinProcess::GetLastErrorText() {
    TCHAR *errorText;
    DWORD errCode = GetLastError();

    if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, errCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&errorText, 0, nullptr)) {
        return "Internal error while looking up an error code";
    }

    Str result = Str::UTF8StrFromWCharString(errorText);
    LocalFree(errorText);
    return result;
}

SharedLib PlatformWinProcess::OpenLibrary(const char *filename) {
    HMODULE handle = LoadLibraryA(filename);
    if (!handle) {
        char str[512];
        Str::snPrintf(str, sizeof(str), "%s.dll", filename);
        handle = LoadLibraryA(str);
        // So you can see what the error is in the debugger...
        if (!handle) {
            Str lastErrorText = PlatformWinProcess::GetLastErrorText();
            BE_WARNLOG("Failed to LoadLibrary : %s", lastErrorText.c_str());
        }
    }

    return reinterpret_cast<SharedLib>(handle);
}

void *PlatformWinProcess::GetSymbol(SharedLib lib, const char *sym) {
    return GetProcAddress(reinterpret_cast<HMODULE>(lib), sym);
}

void PlatformWinProcess::CloseLibrary(SharedLib lib) {
    if (lib) {
        FreeLibrary(reinterpret_cast<HMODULE>(lib));
    }
}

ProcessHandle PlatformWinProcess::CreateProccess(const char *appPath, const char *args, const char *workingPath) {
    SECURITY_ATTRIBUTES secAttr;
    secAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    secAttr.lpSecurityDescriptor = nullptr;
    secAttr.bInheritHandle = true;

    HANDLE hStdOutRead;
    HANDLE hStdOutWrite;
    CreatePipe(&hStdOutRead, &hStdOutWrite, &secAttr, 0);
    SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);

    HANDLE hStdInRead;
    HANDLE hStdInWrite;
    CreatePipe(&hStdInRead, &hStdInWrite, &secAttr, 0);
    SetHandleInformation(hStdInWrite, HANDLE_FLAG_INHERIT, 0);

    DWORD creationFlags = NORMAL_PRIORITY_CLASS;
    creationFlags |= DETACHED_PROCESS;

    STARTUPINFO si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = FALSE;
    si.hStdInput = hStdInRead;
    si.hStdOutput = hStdOutWrite;
    si.hStdError = hStdOutWrite;

    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));

	char expandedAppPath[1024] = "";

	if (appPath && appPath[0]) {
		ExpandEnvironmentStringsA(appPath, expandedAppPath, COUNT_OF(expandedAppPath));
	}
	
	char commandLine[32768];
    Str::snPrintf(commandLine, COUNT_OF(commandLine), "%s %s", expandedAppPath, args);

    wchar_t wCommandLine[32768];
    PlatformWinUtils::UTF8ToUCS2(commandLine, wCommandLine, COUNT_OF(wCommandLine));


	wchar_t wExpandedWorkingPath[1024] = L"";
	if (workingPath && workingPath[0]) {
		char expandedWorkingPath[1024];
		ExpandEnvironmentStringsA(workingPath, expandedWorkingPath, COUNT_OF(expandedWorkingPath));
		PlatformWinUtils::UTF8ToUCS2(expandedWorkingPath, wExpandedWorkingPath, COUNT_OF(wExpandedWorkingPath));
	}
	
	if (!CreateProcessW(nullptr, wCommandLine, &secAttr, &secAttr, TRUE, creationFlags, nullptr, wExpandedWorkingPath[0] ? wExpandedWorkingPath : nullptr, &si, &pi)) {
        Str lastErrorText = PlatformWinProcess::GetLastErrorText();
        BE_WARNLOG("Failed to CreateProcess : %s", lastErrorText.c_str());
        return ProcessHandle();
    }

    ::CloseHandle(pi.hThread); // thread handle is no longer needed so we close
    ProcessHandle processHandle = ProcessHandle(pi.hProcess);
    processHandle.hStdOutRead = hStdOutRead;
    processHandle.hStdOutWrite = hStdOutWrite;
    processHandle.hStdInRead = hStdInRead;
    processHandle.hStdInWrite = hStdInWrite;
    return processHandle;
}

bool PlatformWinProcess::IsProccessRunning(ProcessHandle &processHandle) {
    if (::WaitForSingleObject(processHandle.handle, 0) != WAIT_TIMEOUT) {
        return false;
    }
    return true;
}

void PlatformWinProcess::WaitForProccess(ProcessHandle &processHandle) {
    ::WaitForSingleObject(processHandle.handle, INFINITE);
}

void PlatformWinProcess::TerminateProccess(ProcessHandle &processHandle) {
    ::TerminateProcess(processHandle.handle, 0);
    processHandle.Close();
}

bool PlatformWinProcess::GetProcessExitCode(ProcessHandle &processHandle, int32_t *exitCode) {
    return ::GetExitCodeProcess(processHandle.handle, (::DWORD *)exitCode) && *((DWORD *)exitCode) != STILL_ACTIVE;
}

bool PlatformWinProcess::ReadProcessOutput(ProcessHandle &processHandle, int bufferLength, char *buffer) {
    DWORD bytesRead = 0;
    DWORD bytesAvail = 0;
    DWORD bytesLeft = 0;

    BOOL ok = PeekNamedPipe(processHandle.hStdOutRead, nullptr, 0, nullptr, &bytesAvail, &bytesLeft);
    if (ok && bytesAvail != 0) {
        // The read operation will block until there is data to read
        ok = ReadFile(processHandle.hStdOutRead, buffer, bufferLength - 3, &bytesRead, nullptr);
        if (ok && bytesRead > 0) {
            buffer[bytesRead] = '\0';

            int length = 0;
            for (int i = 0; buffer[i] != '\0'; i++) {
                if (buffer[i] != '\r') { // remove '\r' from the buffer
                    buffer[length++] = buffer[i];
                }
            }
            buffer[length++] = '\0';
        }

        return true;
    }

    return false;
}

void PlatformWinProcess::Sleep(float seconds) {
    ::Sleep((DWORD)(seconds * 1000.0));
}

BE_NAMESPACE_END
