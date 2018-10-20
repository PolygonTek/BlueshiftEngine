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
#include "Platform/Windows/PlatformWinUtils.h"

BE_NAMESPACE_BEGIN

int PlatformWinUtils::UCS2ToUTF8(const wchar_t *src, char *dest, int destLen) {
    int srcLen = lstrlenW(src) + 1;

    if (destLen <= 0) {
        destLen = WideCharToMultiByte(CP_UTF8, 0, src, srcLen, nullptr, 0, nullptr, nullptr);
    }
    if (dest) {
        WideCharToMultiByte(CP_UTF8, 0, src, srcLen, dest, destLen, nullptr, nullptr);
    }
    return destLen;
}

int PlatformWinUtils::UTF8ToUCS2(const char *src, wchar_t *dest, int destLen) {
    int srcLen = strlen(src) + 1;

    if (destLen <= 0) {
        destLen = MultiByteToWideChar(CP_UTF8, 0, src, srcLen, nullptr, 0);
    }
    if (dest) {
        MultiByteToWideChar(CP_UTF8, 0, src, srcLen, dest, destLen);
    }
    return destLen;
}

BE_NAMESPACE_END
