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

#define USE_BASE_PLATFORM_APPLE_SYSTEM
#include "../Apple/PlatformAppleSystem.h"

BE_NAMESPACE_BEGIN

class BE_API PlatformMacOSSystem : public PlatformAppleSystem {
public:
    static void             GetEnvVar(const char *variableName, char *result, uint32_t resultLength);
    static void             SetEnvVar(const char *variableName, const char *value);

    static const char *     UserDir();
    static const char *     UserDocumentDir();
    static const char *     UserAppDataDir();
    static const char *     UserTempDir();

    static int32_t          NumCPUCores();
    static int32_t          NumCPUCoresIncludingHyperthreads();
};

typedef PlatformMacOSSystem PlatformSystem;

BE_NAMESPACE_END
