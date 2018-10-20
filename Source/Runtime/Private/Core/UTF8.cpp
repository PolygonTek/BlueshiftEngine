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

static const byte trailingBytes[64] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4,
    5, 5, 5, 5
};

static const uint32_t trailingMask[6] = { 0x0000007f, 0x0000001f, 0x0000000f, 0x00000007, 0x00000003, 0x00000001 };

bool UTF8::IsValid(const char *s, const int maxLen, Encoding &encoding) {
    struct Local {
        static int GetNumEncodedUTF8Bytes(const uint8_t c) {
            if (c < 0x80) { // 0xxxxxxx
                return 1;
            } else if ((c >> 5) == 0x06) { // 110xxxxx
                                           // 2 byte encoding - the next byte must begin with
                return 2;
            } else if ((c >> 4) == 0x0E) { // 1110xxxx
                                           // 3 byte encoding
                return 3;
            } else if ((c >> 3) == 0x1E) { // 11110xxx
                                           // 4 byte encoding
                return 4;
            }
            // this isnt' a valid UTF-8 precursor character
            return 0;
        }
        static bool RemainingCharsAreUTF8FollowingBytes(const uint8_t *s, const int curChar, const int maxLen, const int num) {
            if (maxLen - curChar < num) {
                return false;
            }
            for (int i = curChar + 1; i <= curChar + num; i++) {
                if (s[i] == '\0') {
                    return false;
                }
                if ((s[i] >> 6) != 0x02) {
                    return false;
                }
            }
            return true;
        }
    };

    // check for byte-order-marker
    encoding = UTF8_PURE_ASCII;
    Encoding utf8Type = UTF8_ENCODED_NO_BOM;
    if (maxLen > 3 && s[0] == 0xEF && s[1] == 0xBB && s[2] == 0xBF) {
        utf8Type = UTF8_ENCODED_BOM;
    }

    for (int i = 0; s[i] != '\0' && i < maxLen; i++) {
        int numBytes = Local::GetNumEncodedUTF8Bytes(s[i]);
        if (numBytes == 1) {
            continue;   // just low ASCII
        } else if (numBytes == 2) {
            // 2 byte encoding - the next byte must begin with bit pattern 10
            if (!Local::RemainingCharsAreUTF8FollowingBytes((const uint8_t *)s, i, maxLen, 1)) {
                return false;
            }
            // skip over UTF-8 character
            i += 1;
            encoding = utf8Type;
        } else if (numBytes == 3) {
            // 3 byte encoding - the next 2 bytes must begin with bit pattern 10
            if (!Local::RemainingCharsAreUTF8FollowingBytes((const uint8_t *)s, i, maxLen, 2)) {
                return false;
            }
            // skip over UTF-8 character
            i += 2;
            encoding = utf8Type;
        } else if (numBytes == 4) {
            // 4 byte encoding - the next 3 bytes must begin with bit pattern 10
            if (!Local::RemainingCharsAreUTF8FollowingBytes((const uint8_t *)s, i, maxLen, 3)) {
                return false;
            }
            // skip over UTF-8 character
            i += 3;
            encoding = utf8Type;
        } else {
            // this isnt' a valid UTF-8 character
            if (utf8Type == UTF8_ENCODED_BOM) {
                encoding = UTF8_INVALID_BOM;
            } else {
                encoding = UTF8_INVALID;
            }
            return false;
        }
    }
    return true;
}

void UTF8::Encode(char *&dest, char32_t unicodeChar) {
    if (unicodeChar < 0x80) {
        *dest++ = (char)unicodeChar;
    } else if (unicodeChar < 0x800) { // 11 bits
        dest[0] = (char)(0xC0 | (unicodeChar >> 6));
        dest[1] = (char)(0x80 | (unicodeChar & 0x3F));
        dest += 2;
    } else if (unicodeChar < 0x10000) { // 16 bits
        dest[0] = (char)(0xE0 | (unicodeChar >> 12));
        dest[1] = (char)(0x80 | ((unicodeChar >> 6) & 0x3F));
        dest[2] = (char)(0x80 | (unicodeChar & 0x3F));
        dest += 3;
    } else if (unicodeChar < 0x200000) { // 21 bits
        dest[0] = (char)(0xF0 | (unicodeChar >> 18));
        dest[1] = (char)(0x80 | ((unicodeChar >> 12) & 0x3F));
        dest[2] = (char)(0x80 | ((unicodeChar >> 6) & 0x3F));
        dest[3] = (char)(0x80 | (unicodeChar & 0x3F));
        dest += 4;
    } else if (unicodeChar < 0x4000000) { // 26 bits
        dest[0] = (char)(0xF8 | (unicodeChar >> 24));
        dest[1] = (char)(0x80 | ((unicodeChar >> 18) & 0x3F));
        dest[2] = (char)(0x80 | ((unicodeChar >> 12) & 0x3F));
        dest[3] = (char)(0x80 | ((unicodeChar >> 6) & 0x3F));
        dest[4] = (char)(0x80 | (unicodeChar & 0x3F));
        dest += 5;
    } else { // 31 bits
        dest[0] = (char)(0xFC | (unicodeChar >> 30));
        dest[1] = (char)(0x80 | ((unicodeChar >> 24) & 0x3F));
        dest[2] = (char)(0x80 | ((unicodeChar >> 18) & 0x3F));
        dest[3] = (char)(0x80 | ((unicodeChar >> 12) & 0x3F));
        dest[4] = (char)(0x80 | ((unicodeChar >> 6) & 0x3F));
        dest[5] = (char)(0x80 | (unicodeChar & 0x3F));
        dest += 6;
    }
}

#define GET_NEXT_CONTINUATION_BYTE(ptr) *ptr; if ((byte)*ptr < 0x80 || (byte)*ptr >= 0xC0) return '?'; else ++ptr;

char32_t UTF8::Decode(const char *&src) {
    if (!src) {
        return 0;
    }

    byte char1 = *src++;

    // Check if we are in the middle of a UTF8 character
    if (char1 >= 0x80 && char1 < 0xC0) {
        while ((byte)*src >= 0x80 && (byte)*src < 0xC0) {
            ++src;
        }
        return '?';
    }

    if (char1 < 0x80) {
        return char1;
    } else if (char1 < 0xE0) {
        byte char2 = GET_NEXT_CONTINUATION_BYTE(src);
        return (char32_t)((char2 & 0x3F) | ((char1 & 0x1F) << 6));
    } else if (char1 < 0xF0) {
        byte char2 = GET_NEXT_CONTINUATION_BYTE(src);
        byte char3 = GET_NEXT_CONTINUATION_BYTE(src);
        return (char32_t)((char3 & 0x3F) | ((char2 & 0x3F) << 6) | ((char1 & 0xF) << 12));
    } else if (char1 < 0xF8) {
        byte char2 = GET_NEXT_CONTINUATION_BYTE(src);
        byte char3 = GET_NEXT_CONTINUATION_BYTE(src);
        byte char4 = GET_NEXT_CONTINUATION_BYTE(src);
        return (char32_t)((char4 & 0x3F) | ((char3 & 0x3F) << 6) | ((char2 & 0x3F) << 12) | ((char1 & 0x7) << 18));
    } else if (char1 < 0xFC) {
        byte char2 = GET_NEXT_CONTINUATION_BYTE(src);
        byte char3 = GET_NEXT_CONTINUATION_BYTE(src);
        byte char4 = GET_NEXT_CONTINUATION_BYTE(src);
        byte char5 = GET_NEXT_CONTINUATION_BYTE(src);
        return (char32_t)((char5 & 0x3F) | ((char4 & 0x3F) << 6) | ((char3 & 0x3F) << 12) | ((char2 & 0x3F) << 18) | ((char1 & 0x3) << 24));
    } else {
        byte char2 = GET_NEXT_CONTINUATION_BYTE(src);
        byte char3 = GET_NEXT_CONTINUATION_BYTE(src);
        byte char4 = GET_NEXT_CONTINUATION_BYTE(src);
        byte char5 = GET_NEXT_CONTINUATION_BYTE(src);
        byte char6 = GET_NEXT_CONTINUATION_BYTE(src);
        return (char32_t)((char6 & 0x3F) | ((char5 & 0x3F) << 6) | ((char4 & 0x3F) << 12) | ((char3 & 0x3F) << 18) | ((char2 & 0x3F) << 24) | ((char1 & 0x1) << 30));
    }
}

int UTF8::Length(const char *s) {
    int mbLen = 0;
    int charLen = 0;

    while (s[mbLen] != '\0') {
        uint32_t cindex = (byte)s[mbLen];

        if (cindex < 0x80) {
            mbLen++;
        } else {
            int trailing = 0;
            if (cindex >= 0xc0) {
                trailing = trailingBytes[cindex - 0xc0];
            }
            mbLen += trailing + 1;
        }
        charLen++;
    }
    return charLen;
}

char32_t UTF8::CharAdvance(const char *s, int &offset) {
    if (offset >= 0) {
        uint32_t cindex = (byte)s[offset];

        if (cindex != '\0') {
            if (cindex < 0x80) {
                offset++;
                return cindex;
            }
            int trailing = 0;
            if (cindex >= 0xc0) {
                trailing = trailingBytes[cindex - 0xc0];
            }
            cindex &= trailingMask[trailing];
            while (trailing-- > 0) {
                cindex <<= 6;
                cindex += ((byte)s[++offset]) & 0x0000003f;
            }
            offset++;
            return cindex;
        }
    }
    offset++;
    return 0;   // return a null terminator if out of range
}

char32_t UTF8::CharPrevious(const char *s, int &offset) {
    if (Previous(s, offset)) {
        return Char(s, offset);
    }
    return 0;
}

bool UTF8::Advance(const char *s, int &offset) {
    if (offset >= 0) {
        uint32_t cindex = (byte)s[offset];

        if (cindex != '\0') {
            if (cindex < 0x80) {
                offset++;
                return true;
            }
            int trailing = 0;
            if (cindex >= 0xc0) {
                trailing = trailingBytes[cindex - 0xc0];
            }
            offset += trailing + 1;
            return true;
        }
    }
    return false;
}

bool UTF8::Previous(const char *s, int &offset) {
    if (offset <= 0) {
        return false;
    }

    while ((((byte)s[--offset]) >> 6) == 2) {
        if (offset < 0) {
            return false;
        }
    }
    return true;
}

BE_NAMESPACE_END
