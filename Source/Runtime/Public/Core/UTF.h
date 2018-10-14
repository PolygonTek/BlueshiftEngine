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

/*
-------------------------------------------------------------------------------

    UTF-8

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class UTF8 {
public:
    enum Encoding {
        UTF8_PURE_ASCII,        ///< no characters with values > 127
        UTF8_ENCODED_BOM,       ///< characters > 128 encoded with UTF8, with a byte-order-marker at the beginning
        UTF8_ENCODED_NO_BOM,    ///< characters > 128 encoded with UTF8, but no byte-order-marker at the beginning
        UTF8_INVALID,           ///< has values > 127 but isn't valid UTF8 
        UTF8_INVALID_BOM        ///< has a byte-order-marker at the beginning, but isn't valid UTF8 -- it's messed up
    };

                        /// Encode Unicode character to UTF8. Pointer will be incremented.
    static void         Encode(char *&dest, uint32_t unicodeChar);
                        /// Decode Unicode character from UTF8. Pointer will be incremented.
    static uint32_t     Decode(const char *&src);

                        /// Returns number of characters in UTF8 content.
    static int          Length(const byte *s);

                        /// Returns Unicode character with then given index. Index will be incremented.
    static uint32_t     Char(const char *s, int &idx) { return Char((byte *)s, idx); }
                        /// Returns Unicode character with then given index. Index will be incremented.
    static uint32_t     Char(const byte *s, int &idx);

    static bool         IsValid(const uint8_t *s, const int maxLen, Encoding &encoding);
    static bool         IsValid(const char *s, const int maxLen, Encoding &encoding) { return IsValid((const uint8_t *)s, maxLen, encoding); }
    static bool         IsValid(const uint8_t *s, const int maxLen) { Encoding encoding; return IsValid(s, maxLen, encoding); }
    static bool         IsValid(const char *s, const int maxLen) { return IsValid((const uint8_t *)s, maxLen); }
};

/*
-------------------------------------------------------------------------------

    UTF-16

-------------------------------------------------------------------------------
*/

class UTF16 {
public:
    /// Encode Unicode character to UTF16. Pointer will be incremented.
    static void         Encode(wchar_t *&dest, uint32_t unicodeChar);
    /// Decode Unicode character from UTF16. Pointer will be incremented.
    static uint32_t     Decode(const wchar_t *&src);
};

BE_NAMESPACE_END
