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
#include <mmsystem.h>
#include <shlobj.h>

BE_NAMESPACE_BEGIN

void PlatformWinSystem::GetEnvVar(const char *variableName, char *result, uint32_t resultLength) {
    DWORD err = ::GetEnvironmentVariableA(variableName, result, resultLength);
    if (err <= 0) {
        *result = 0;
    }
}

void PlatformWinSystem::SetEnvVar(const char *variableName, const char *value) {
    BOOL err = ::SetEnvironmentVariableA(variableName, value);
    if (err == 0) {
        BE_WARNLOG(L"Failed to SetEnvironmentVariable: %s to %s", variableName, value);
    }
}

const char *PlatformWinSystem::UserDir() {
    static char path[1024] = "";

    const char *userProfile = getenv("USERPROFILE");
    if (userProfile) {
        strcpy(path, userProfile);
    } else {
        const char *homeDrive = getenv("HOMEDRIVE");
        const char *homePath = getenv("HOMEPATH");
        strcpy(path, homeDrive);
        strcat(path, homePath);
    }

    return path;
}

const char *PlatformWinSystem::UserDocumentDir() {
    static char path[1024] = "";

    if (!path[0]) {
        // Get the My Documents directory
        if (!SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, path))) {
            return nullptr;
        }
    }
    return path;
}

const char *PlatformWinSystem::UserAppDataDir() {
    static char path[1024] = "";

    if (!path[0]) {
        if (!SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, path))) {
            return nullptr;
        }
    }
    return path;
}

const char *PlatformWinSystem::UserTempDir() {
    static char path[1024] = "";

    if (!path[0]) {
        char tempPath[MAX_PATH];
        ZeroMemory(tempPath, sizeof(tempPath[0]) * MAX_PATH);
        ::GetTempPathA(MAX_PATH, tempPath);

        char fullTempPath[MAX_PATH];
        ZeroMemory(fullTempPath, sizeof(fullTempPath[0]) * MAX_PATH);
        ::GetLongPathNameA(tempPath, fullTempPath, MAX_PATH);

        strcpy(path, fullTempPath);
    }
    return path;
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
    static int32_t numCores = 0;
    if (numCores == 0) {
        // Get the number of logical processors, including hyperthreaded ones.
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        numCores = (int32_t)si.dwNumberOfProcessors;
    }
    return numCores;
}

BE_NAMESPACE_END
