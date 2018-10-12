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
#include "Platform/PlatformTime.h"
#include "Core/Str.h"
#include "Core/Guid.h"

BE_NAMESPACE_BEGIN

Guid Guid::zero = Guid();

Guid Guid::CreateGuid() {
    static uint16_t incrementCounter = 0;
    uint32_t sequentialBits = static_cast<uint32_t>(incrementCounter++);
    
    int32_t year, month, dayOfWeek, day, hour, min, sec, msec;
    PlatformTime::LocalTime(year, month, dayOfWeek, day, hour, min, sec, msec);
    
    const uint32_t cycles = (uint32_t)PlatformTime::Cycles();
    const uint32_t randomBits = rand();

    return Guid(randomBits | (sequentialBits << 16), (day - 1) | (hour << 8) | (month << 16) | (sec << 24), msec | (min << 16), (year - 1600) ^ cycles);
}

bool Guid::SetFromString(const char *s) {
    Str guidStr(s);
    Str normalizedGuidStr;

    if (guidStr.Length() == 32) { 
        // RawValues
        normalizedGuidStr += guidStr.Mid(0, 8);
        normalizedGuidStr += guidStr.Mid(8, 8);
        normalizedGuidStr += guidStr.Mid(16, 8);
        normalizedGuidStr += guidStr.Mid(24, 8);
    } else if (guidStr.Length() == 36) {
        if (guidStr[8] != '-' ||
            guidStr[13] != '-' ||
            guidStr[18] != '-' ||
            guidStr[23] != '-') {
            return false;
        }
        // DigitsWithHyphens
        normalizedGuidStr += guidStr.Mid(0, 8);
        normalizedGuidStr += guidStr.Mid(9, 4);
        normalizedGuidStr += guidStr.Mid(14, 4);
        normalizedGuidStr += guidStr.Mid(19, 4);
        normalizedGuidStr += guidStr.Mid(24, 12);
    } else if (guidStr.Length() == 38) {
        if (guidStr[0] == '{') {
            // DigitsWithHyphensInBraces
            if (guidStr[9] != '-' ||
                guidStr[14] != '-' ||
                guidStr[19] != '-' ||
                guidStr[24] != '-' ||
                guidStr[37] != '}') {
                return false;
            }
            normalizedGuidStr += guidStr.Mid(1, 8);
            normalizedGuidStr += guidStr.Mid(10, 4);
            normalizedGuidStr += guidStr.Mid(15, 4);
            normalizedGuidStr += guidStr.Mid(20, 4);
            normalizedGuidStr += guidStr.Mid(25, 12);
        } else if (guidStr[0] == '(') {
            // DigitsWithHyphensInParentheses
            if (guidStr[9] != '-' ||
                guidStr[14] != '-' ||
                guidStr[19] != '-' ||
                guidStr[24] != '-' ||
                guidStr[37] != ')') {
                return false;
            }
            normalizedGuidStr += guidStr.Mid(1, 8);
            normalizedGuidStr += guidStr.Mid(10, 4);
            normalizedGuidStr += guidStr.Mid(15, 4);
            normalizedGuidStr += guidStr.Mid(20, 4);
            normalizedGuidStr += guidStr.Mid(25, 12);
        }
    } else {
        return false;
    }

    uint32_t a = (uint32_t)strtoul(normalizedGuidStr.Mid(0, 8).c_str(), nullptr, 16);
    uint32_t b = (uint32_t)strtoul(normalizedGuidStr.Mid(8, 8).c_str(), nullptr, 16);
    uint32_t c = (uint32_t)strtoul(normalizedGuidStr.Mid(16, 8).c_str(), nullptr, 16);
    uint32_t d = (uint32_t)strtoul(normalizedGuidStr.Mid(24, 8).c_str(), nullptr, 16);
    Set(a, b, c, d);

    return true;
}

Guid Guid::FromString(const char *s) {
    Guid guid;
    if (!guid.SetFromString(s)) {
        return Guid();
    }
    return guid;
}

const char *Guid::ToString(Format format) const {
    switch (format) {
    case DigitsWithHyphens:
        return va("%08X-%04X-%04X-%04X-%04X%08X", u1, u2 >> 16, u2 & 0xFFFF, u3 >> 16, u3 & 0xFFFF, u4);
    case DigitsWithHyphensInBraces:
        return va("{%08X-%04X-%04X-%04X-%04X%08X}", u1, u2 >> 16, u2 & 0xFFFF, u3 >> 16, u3 & 0xFFFF, u4);
    case DigitsWithHyphensInParentheses:
        return va("(%08X-%04X-%04X-%04X-%04X%08X)", u1, u2 >> 16, u2 & 0xFFFF, u3 >> 16, u3 & 0xFFFF, u4);
    case Digits:
    default:
        return va("%08X%08X%08X%08X", u1, u2, u3, u4);
    }
}

BE_NAMESPACE_END