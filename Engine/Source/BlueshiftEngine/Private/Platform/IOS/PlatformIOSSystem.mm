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
#include <sys/sysctl.h>

BE_NAMESPACE_BEGIN

void PlatformIOSSystem::GetEnvVar(const char *varName, char *result, uint32_t resultLength) {
    char *ret = getenv(varName);
    if (ret) {
        strncpy(result, ret, resultLength);
    } else {
        *result = 0;
    }
}

void PlatformIOSSystem::SetEnvVar(const char *varName, const char *value) {
}

const char *PlatformIOSSystem::UserDir() {
    static char path[1024] = "";
    if (!path[0]) {
        NSString *userDir = [NSSearchPathForDirectoriesInDomains(NSUserDirectory, NSUserDomainMask, YES) objectAtIndex:0];
        strcpy(path, (const char *)[userDir cStringUsingEncoding:NSUTF8StringEncoding]);
    }
    return path;
}

const char *PlatformIOSSystem::UserDocumentDir() {
    static char path[1024] = "";
    if (!path[0]) {
        NSString *documentDir = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex: 0];
        strcpy(path, (const char *)[documentDir cStringUsingEncoding:NSUTF8StringEncoding]);
    }
    return path;
}

const char *PlatformIOSSystem::UserAppDataDir() {
    static char path[1024] = "";
    if (!path[0]) {
        NSString *libraryDir = [NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES) objectAtIndex: 0];
        strcpy(path, (const char *)[libraryDir cStringUsingEncoding:NSUTF8StringEncoding]);
    }
    return path;
}

const char *PlatformIOSSystem::UserTempDir() {
    static char path[1024] = "";
    if (!path[0]) {
        NSString *tempDir = NSTemporaryDirectory();
        strcpy(path, (const char *)[tempDir cStringUsingEncoding:NSUTF8StringEncoding]);
    }
    return path;
}

int32_t PlatformIOSSystem::NumCPUCores() {
    static int32_t numCores = -1;
    if (numCores == -1) {
        size_t size = sizeof(int32_t);
        if (sysctlbyname("hw.ncpu", &numCores, &size, nullptr, 0) != 0) {
            numCores = 1;
        }
    }
    return numCores;
}

BE_NAMESPACE_END
