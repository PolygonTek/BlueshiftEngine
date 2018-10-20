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
#include "Core/UTF.h"

BE_NAMESPACE_BEGIN

void UTF16::Encode(wchar_t *&dest, char32_t unicodeChar) {
    if (unicodeChar < 0x10000) {
        *dest++ = unicodeChar;
    } else {
        unicodeChar -= 0x10000;
        *dest++ = 0xD800 | ((unicodeChar >> 10) & 0x3FF);
        *dest++ = 0xDC00 | (unicodeChar & 0x3FF);
    }
}

char32_t UTF16::Decode(const wchar_t *&src) {
    if (!src) {
        return 0;
    }

    uint16_t word1 = *src++;

    // Check if we are at a low surrogate
    if (word1 >= 0xDC00 && word1 < 0xE000) {
        while (*src >= 0xDC00 && *src < 0xE000) {
            ++src;
        }
        return '?';
    }

    if (word1 < 0xD800 || word1 >= 0xE000) {
        return word1;
    } else {
        uint16_t word2 = *src++;
        if (word2 < 0xDC00 || word2 >= 0xE000) {
            --src;
            return '?';
        } else {
            return (((word1 & 0x3FF) << 10) | (word2 & 0x3FF)) + 0x10000;
        }
    }
}

BE_NAMESPACE_END
