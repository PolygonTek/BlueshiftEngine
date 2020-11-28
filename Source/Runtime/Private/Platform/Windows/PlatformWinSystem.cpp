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
#include "Platform/PlatformSystem.h"
#include "Platform/Windows/PlatformWinUtils.h"
#include <mmsystem.h>
#include <shlobj.h>

BE_NAMESPACE_BEGIN

void PlatformWinSystem::GetEnvVar(const char *variableName, char *result, uint32_t resultLength) {
    wchar_t wVariableName[64];
    wchar_t wValue[4096];

    PlatformWinUtils::UTF8ToUCS2(variableName, wVariableName, COUNT_OF(wVariableName));

    DWORD err = ::GetEnvironmentVariableW(wVariableName, wValue, COUNT_OF(wValue));
    if (err <= 0) {
        *result = 0;
        return;
    }
    PlatformWinUtils::UCS2ToUTF8(wValue, result, resultLength);
}

void PlatformWinSystem::SetEnvVar(const char *variableName, const char *value) {
    wchar_t wVariableName[64];
    wchar_t wValue[4096];

    PlatformWinUtils::UTF8ToUCS2(variableName, wVariableName, COUNT_OF(wVariableName));
    PlatformWinUtils::UTF8ToUCS2(value, wValue, COUNT_OF(wValue));

    BOOL err = ::SetEnvironmentVariableW(wVariableName, wValue);
    if (err == 0) {
        BE_WARNLOG("Failed to SetEnvironmentVariable: %s to %s", variableName, value);
    }
}

const char *PlatformWinSystem::UserDir() {
    static char userDir[1024] = "";

    const char *userProfile = getenv("USERPROFILE");
    if (userProfile) {
        strcpy(userDir, userProfile);
    } else {
        const char *homeDrive = getenv("HOMEDRIVE");
        const char *homePath = getenv("HOMEPATH");

        strcpy(userDir, homeDrive);
        strcat(userDir, homePath);
    }

    return userDir;
}

const char *PlatformWinSystem::UserDocumentDir() {
    static char userDocumentDir[1024] = "";

    if (!userDocumentDir[0]) {
        wchar_t wUserDocumentDir[1024];

        // Get the My Documents directory
        if (!SUCCEEDED(::SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, wUserDocumentDir))) {
            return nullptr;
        }
        PlatformWinUtils::UCS2ToUTF8(wUserDocumentDir, userDocumentDir, COUNT_OF(userDocumentDir));
    }
    return userDocumentDir;
}

const char *PlatformWinSystem::UserAppDataDir() {
    static char userAppDataDir[1024] = "";

    if (!userAppDataDir[0]) {
        wchar_t wUserAppDataDir[1024];

        if (!SUCCEEDED(::SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, wUserAppDataDir))) {
            return nullptr;
        }
        PlatformWinUtils::UCS2ToUTF8(wUserAppDataDir, userAppDataDir, COUNT_OF(userAppDataDir));
    }
    return userAppDataDir;
}

const char *PlatformWinSystem::UserTempDir() {
    static char userTempDir[1024] = "";

    if (!userTempDir[0]) {
        wchar_t wTempPath[MAX_PATH];
        ZeroMemory(wTempPath, sizeof(wTempPath[0]) * MAX_PATH);
        ::GetTempPathW(MAX_PATH, wTempPath);

        wchar_t wFullTempPath[MAX_PATH];
        ZeroMemory(wFullTempPath, sizeof(wFullTempPath[0]) * MAX_PATH);
        ::GetLongPathNameW(wTempPath, wFullTempPath, MAX_PATH);

        PlatformWinUtils::UCS2ToUTF8(wFullTempPath, userTempDir, COUNT_OF(userTempDir));
    }
    return userTempDir;
}

int32_t PlatformWinSystem::NumCPUCores() {
    static int32_t numCores = 0;
    if (numCores == 0) {
        // Get only physical cores
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION *infoBuffer = NULL;
        DWORD bufferSize = 0;

        // Get the size of the buffer to hold processor information.
        if (!GetLogicalProcessorInformation(infoBuffer, &bufferSize)) {
            return 0;
        }

        // Allocate the buffer to hold the processor infoBuffer.
        infoBuffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(bufferSize);

        // Get the actual information.
        if (!GetLogicalProcessorInformation(infoBuffer, &bufferSize)) {
            return 0;
        }

        // Count physical cores
        const int32_t InfoCount = (int32_t)(bufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
        for (int32_t Index = 0; Index < InfoCount; ++Index) {
            SYSTEM_LOGICAL_PROCESSOR_INFORMATION* Info = &infoBuffer[Index];
            if (Info->Relationship == RelationProcessorCore) {
                numCores++;
            }
        }
        free(infoBuffer);
    }
    return numCores;
}

int32_t PlatformWinSystem::NumCPUCoresIncludingHyperthreads() {
#if (_WIN32_WINNT >= 0x0601)
    static int32_t numCores = 0;
    if (numCores == 0) {
        static int32_t numGroups = GetActiveProcessorGroupCount();
        for (int32_t i = 0; i < numGroups; i++) {
            numCores += GetActiveProcessorCount(i);
        }
    }
    return numCores;
#else
    static int32_t numCores = 0;
    if (numCores == 0) {
        // Get the number of logical processors, including hyperthreaded ones.
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        numCores = (int32_t)si.dwNumberOfProcessors;
    }
    return numCores;
#endif
}

bool PlatformWinSystem::IsDebuggerPresent() {
    return ::IsDebuggerPresent();
}

void PlatformWinSystem::DebugBreak() {
    if (IsDebuggerPresent()) {
        __debugbreak();
    }
}

BE_NAMESPACE_END
