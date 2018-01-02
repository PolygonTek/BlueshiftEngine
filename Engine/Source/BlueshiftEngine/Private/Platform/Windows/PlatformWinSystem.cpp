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
#include "Platform/Windows/PlatformWinSystem.h"
#include <mmsystem.h>

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

BE_NAMESPACE_END
