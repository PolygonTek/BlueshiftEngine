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
#include "Platform/IOS/PlatformIOSSystem.h"

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

BE_NAMESPACE_END
