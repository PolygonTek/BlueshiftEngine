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
#include "Core/Str.h"
#include "Containers/StrArray.h"

BE_NAMESPACE_BEGIN

Str Str::empty;

void Str::ReAllocate(int amount, bool keepOld) {
    assert(amount > 0);

    int newSize;
    int mod = amount % AllocGranularity;
    if (!mod) {
        newSize = amount;
    } else {
        newSize = amount + AllocGranularity - mod;
    }

    alloced = newSize;

#ifdef USE_STRING_DATA_ALLOCATOR
    char *newBuffer = stringDataAllocator.Alloc(alloced);
#else
    char *newBuffer = new char[alloced];
#endif
    if (keepOld && data && len > 0) {
        data[len] = '\0';
        strcpy(newBuffer, data);
    }

    if (data && data != baseBuffer) {
#ifdef USE_STRING_DATA_ALLOCATOR
        stringDataAllocator.Free(data);
#else
        delete [] data;
#endif
    }

    data = newBuffer;
}

void Str::FreeData() {
    if (data && data != baseBuffer) {
#ifdef USE_STRING_DATA_ALLOCATOR
        stringDataAllocator.Free(data);
#else
        delete [] data;
#endif
        data = baseBuffer;
    }
}

Str &Str::operator=(const char *text) {
    if (!text) {
        // safe behaviour if nullptr
        EnsureAlloced(1, false);
        data[0] = '\0';
        len = 0;
        return *this;
    }

    if (text == data) {
        return *this; // copying same thing
    }

    // check if we're aliasing
    if (text >= data && text <= data + len) {
        int diff = (int)(text - data);

        assert(strlen(text) < (unsigned)len);

        int i;
        for (i = 0; text[i]; i++) {
            data[i] = text[i];
        }

        data[i] = '\0';

        len -= diff;

        return *this;
    }

    int l = (int)strlen(text);
    EnsureAlloced(l + 1, false);
    strcpy(data, text);
    len = l;

    return *this;
}

const char *Str::Mid(int start, int len, Str &result) const {
    result.Clear();

    int i = Length();
    if (i == 0 || len <= 0 || start >= i) {
        return nullptr;
    }

    if (start + len >= i) {
        len = i - start;
    }

    result.Append(&data[start], len);
    return result;
}

Str Str::Mid(int start, int len) const {
    Str result;

    int i = Length();
    if (i == 0 || len <= 0 || start >= i) {
        return result;
    }

    if (start + len >= i) {
        len = i - start;
    }

    result.Append(&data[start], len);
    return result;
}

Str Str::Commafy() const {
    int start = -1;
    int end = -1;
    int digits = 0;
    int i;

    for (i = 0; i < len; i++) {
        if (CharIsNumeric(data[i])) {
            if (start < 0) {
                start = i;
            }
            digits++;
        } else if (start >= 0) {
            break;
        }
    }

    end = i;
    
    Str result;
    result.EnsureAlloced(len + 1 + (digits - 1) / 3);

    int index = 0;
    for (i = 0; i < len; i++) {
        if (i > start && i < end) {
            if (digits % 3 == 0) {
                result.data[index++] = ',';
            }
        }

        result.data[index++] = data[i];
        digits--;
    }

    result.data[index] = '\0';

    return result;
}

Str Str::NumberedName(int *number) const {
    int index = len - 1;
    while (index >= 0) {
        if (!Str::CharIsNumeric(data[index])) {
            break;
        }

        index--;
    }

    if (index < len - 1) {
        *number = (int)atoi(&data[index + 1]);
    } else {
        *number = 0;
    }

    // skip whitespaces
    while (index >= 0) {
        if (data[index] != ' ' && data[index] != '\t') {
            break;
        }

        index--;
    }

    Str basename = Left(index + 1);
    return basename;
}

int BE_CDECL Str::sPrintf(const char *fmt, ...) {
    va_list argptr;
    char buffer[32000];	// big, but small enough to fit in PPC stack
    va_start(argptr, fmt);
    len = Str::vsnPrintf(buffer, COUNT_OF(buffer) - 1, fmt, argptr);
    va_end(argptr);

    EnsureAlloced(len + 1, false);
    Str::Copynz(data, buffer, len + 1);

    return len;
}

Str Str::FormatBytes(int bytes) {
    Str s;
    if (bytes < 1024) {
        s.sPrintf("%i bytes", bytes);
    } else if (bytes < 1024 * 1024) {
        s.sPrintf("%.1f Kb", bytes / (float)(1024));
    } else if (bytes < 1024 * 1024 * 1024) {
        s.sPrintf("%.1f Mb", bytes / (float)(1024 * 1024));
    } else {
        s.sPrintf("%.1f Gb", bytes / (float)(1024 * 1024 * 1024));
    }

    return s;
}

bool Str::IsValidUTF8(const uint8_t *s, const int maxLen, UTF8Encoding &encoding) {
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
    UTF8Encoding utf8Type = UTF8_ENCODED_NO_BOM;
    if (maxLen > 3 && s[0] == 0xEF && s[1] == 0xBB && s[2] == 0xBF) {
        utf8Type = UTF8_ENCODED_BOM;
    }

    for (int i = 0; s[i] != '\0' && i < maxLen; i++) {
        int numBytes = Local::GetNumEncodedUTF8Bytes(s[i]);
        if (numBytes == 1) {
            continue;   // just low ASCII
        } else if (numBytes == 2) {
            // 2 byte encoding - the next byte must begin with bit pattern 10
            if (!Local::RemainingCharsAreUTF8FollowingBytes(s, i, maxLen, 1)) {
                return false;
            }
            // skip over UTF-8 character
            i += 1;
            encoding = utf8Type;
        } else if (numBytes == 3) {
            // 3 byte encoding - the next 2 bytes must begin with bit pattern 10
            if (!Local::RemainingCharsAreUTF8FollowingBytes(s, i, maxLen, 2)) {
                return false;
            }
            // skip over UTF-8 character
            i += 2;
            encoding = utf8Type;
        } else if (numBytes == 4) {
            // 4 byte encoding - the next 3 bytes must begin with bit pattern 10
            if (!Local::RemainingCharsAreUTF8FollowingBytes(s, i, maxLen, 3)) {
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

int Str::UTF8Length(const byte *s) {
    int mbLen = 0;
    int charLen = 0;
    while (s[mbLen] != '\0') {
        uint32_t cindex;
        cindex = s[mbLen];
        if (cindex < 0x80) {
            mbLen++;
        } else {
            int trailing = 0;
            if (cindex >= 0xc0) {
                static const byte trailingBytes[64] = {
                    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
                };
                trailing = trailingBytes[cindex - 0xc0];
            }
            mbLen += trailing + 1;
        }
        charLen++;
    }
    return charLen;
}

void Str::AppendUTF8Char(uint32_t c) {
    if (c < 0x80) {
        Append((char)c);
    } else if (c < 0x800) { // 11 bits
        Append((char)(0xC0 | (c >> 6)));
        Append((char)(0x80 | (c & 0x3F)));
    } else if (c < 0x10000) { // 16 bits
        Append((char)(0xE0 | (c >> 12)));
        Append((char)(0x80 | ((c >> 6) & 0x3F)));
        Append((char)(0x80 | (c & 0x3F)));
    } else if (c < 0x200000) {	// 21 bits
        Append((char)(0xF0 | (c >> 18)));
        Append((char)(0x80 | ((c >> 12) & 0x3F)));
        Append((char)(0x80 | ((c >> 6) & 0x3F)));
        Append((char)(0x80 | (c & 0x3F)));
    } else {
        // UTF-8 can encode up to 6 bytes. Why don't we support that?
        // This is an invalid Unicode character
        Append('?');
    }
}

uint32_t Str::UTF8Char(const byte *s, int &idx) {
    if (idx >= 0) {
        while (s[idx] != '\0') {
            uint32_t cindex = s[idx];
            if (cindex < 0x80) {
                idx++;
                return cindex;
            }
            int trailing = 0;
            if (cindex >= 0xc0) {
                static const byte trailingBytes[64] = {
                    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
                };
                trailing = trailingBytes[cindex - 0xc0];
            }
            static const uint32_t trailingMask[6] = { 0x0000007f, 0x0000001f, 0x0000000f, 0x00000007, 0x00000003, 0x00000001 };
            cindex &= trailingMask[trailing];
            while (trailing-- > 0) {
                cindex <<= 6;
                cindex += s[++idx] & 0x0000003f;
            }
            idx++;
            return cindex;
        }
    }
    idx++;
    return 0;   // return a null terminator if out of range
}

/*
-------------------------------------------------------------------------------

    filename methods

-------------------------------------------------------------------------------
*/

int Str::FileNameHash() const {
    long hash = 0;
    for (int i = 0; data[i] != '\0'; i++) {
        char letter = Str::ToLower(data[i]);
        if (letter == '.') {
            break; // don't include extension
        }
        
        if (letter =='\\') {
            letter = '/';
        }
        
        hash += (long)(letter) * (i + 119);
    }
    hash &= (FileNameHashSize - 1);
    return (int)hash;
}

Str &Str::BackSlashesToSlashes() {
    for (int i = 0; i < len; i++) {
        if (data[i] == '\\') {
            data[i] = '/';
        }
    }
    return *this;
}

Str &Str::SlashesToBackSlashes() {
    for (int i = 0; i < len; i++) {
        if (data[i] == '/') {
            data[i] = '\\';
        }
    }
    return *this;
}

const Str Str::ToRelativePath(const char *basePath) const {
    Str a_path = data;
    Str base_path = basePath;

    a_path.CleanPath(PATHSEPERATOR_CHAR);
    base_path.CleanPath(PATHSEPERATOR_CHAR);

    StrArray a_path_list;
    StrArray base_path_list;
    SplitStringIntoList(a_path_list, a_path, PATHSEPERATOR_STR);
    SplitStringIntoList(base_path_list, base_path, PATHSEPERATOR_STR);
    
    int idx;
    for (idx = 0; idx < a_path_list.Count() && idx < base_path_list.Count(); idx++) {
        if (a_path_list[idx] != base_path_list[idx]) {
            break;
        }
    }

    if (idx == 0) {
        // already a path is relative path.
        return a_path;
    }
    assert(idx > 0);

    Str result;
    for (int i = idx; i < base_path_list.Count(); i++) {
        if (!result.IsEmpty()) {
            result += PATHSEPERATOR_STR;
        }
        result += "..";
    }

    for (int i = idx; i < a_path_list.Count(); i++) {
        if (!result.IsEmpty()) {
            result += PATHSEPERATOR_STR;
        }
        result += a_path_list[i];
    }

    return result;
}

const Str Str::ToAbsolutePath(const char *basePath) const {
    Str result = basePath;
    result.AppendPath(data);
    result.CleanPath(PATHSEPERATOR_CHAR);
    return result;
}

Str &Str::StripFileExtension() {
    for (int i = len - 1; i > 0; i--) {
        if (data[i] == '/' || data[i] == '\\') {
            break;
        }

        if (data[i] == '.') {
            data[i] = '\0';
            len = i;
            break;
        }
    }
    return *this;
}

Str &Str::StripFileName() {
    int pos = Length() - 1;
    while ((pos > 0) && ((*this)[pos] != '/') && ((*this)[pos] != '\\')) {
        pos--;
    }
    
    if (pos < 0) {
        pos = 0;
    }
    
    Truncate(pos);
    return *this;
}

Str &Str::StripPath() {
    int pos = Length();
    while ((pos > 0) && ((*this)[pos - 1] != '/') && ((*this)[pos - 1] != '\\')) {
        pos--;
    }

    *this = Right(Length() - pos);
    return *this;
}

Str &Str::SetFileExtension(const char *extension) {
    StripFileExtension();
    if (extension[0] && extension[0] != '.') {
        Append('.');
    }
    
    Append(extension);
    return *this;
}

Str &Str::DefaultFileExtension(const char *extension) {
    // 확장자가 있다면 그냥 리턴
    for (int i = len - 1; i >= 0; i--) {
        if (data[i] == '.') {
            return *this;
        }
    }

    if (*extension != '.') {
        Append('.');
    }

    Append(extension);
    return *this;
}

Str &Str::DefaultPath(const char *basepath) {
    if (((*this)[0] == '/') || ((*this)[0] == '\\')) {
        // absolute path location
        return *this;
    }

    *this = basepath + *this;
    return *this;
}

Str &Str::AppendPath(const char *text, char pathSeparator) {
    if (len > 0 && text && text[0]) {
        if (data[len - 1] != '/' && data[len - 1] != '\\') {
            Append(pathSeparator);
        }
    }

    if (text[0] == L'/' || text[0] == L'\\') {
        text++;
    }
    
    Append(text);
    return *this;
}

Str &Str::CleanPath(char pathSeparator) {
    Str a_path = data;

    ConvertPathSeperator(a_path.data, '/');

    StrArray a_path_list;
    SplitStringIntoList(a_path_list, a_path, "/");

    for (int i = 0; i < a_path_list.Count(); i++) {
        if (a_path_list[i] == ".") {
            a_path_list.RemoveIndex(i);

            i--;
        } else if (i > 0 && a_path_list[i - 1] != ".." && a_path_list[i] == "..") {
            a_path_list.RemoveIndex(i);
            a_path_list.RemoveIndex(i - 1);
            
            i -= 2;
        }
    }

    Str result;
    for (int i = 0; i < a_path_list.Count(); i++) {
        if (i > 0) {
            result += pathSeparator;
        }
        result += a_path_list[i];
    }

    *this = result;
    return *this;
}

void Str::ExtractFilePath(Str &dest) const {
    //
    // back up until a \ or the start
    //
    int len = Length();
    if (len == 0) {
        dest.Clear();
        return;
    }

    int pos = len;
    while ((pos > 0) && ((*this)[pos - 1] != '/') && ((*this)[pos - 1] != '\\')) {
        pos--;
    }
    
    Left(pos, dest);
}

void Str::ExtractFileName(Str &dest) const {
    //
    // back up until a \ or the start
    //
    int len = Length();
    if (len == 0) {
        dest.Clear();
        return;
    }

    int pos = len - 1;
    while ((pos > 0) && ((*this)[pos - 1] != '/') && ((*this)[pos - 1] != '\\')) {
        pos--;
    }

    Right(len - pos, dest);
}

void Str::ExtractFileBase(Str &dest) const {
    //
    // back up until a \ or the start
    //
    int len = Length();
    if (len == 0) {
        dest.Clear();
        return;
    }

    int pos = len - 1;
    while ((pos > 0) && ((*this)[pos - 1] != '/') && ((*this)[pos - 1] != '\\')) {
        pos--;
    }

    int start = pos;
    while ((pos < len) && ((*this)[pos] != '.')) {
        pos++;
    }

    Mid(start, pos - start, dest);
}

void Str::ExtractFileExtension(Str &dest) const {
    //
    // back up until a . or the start
    //
    int len = Length();
    if (len == 0) {
        dest.Clear();
        return;
    }

    int pos = len - 1;
    while ((pos > 0) && ((*this)[pos - 1] != '.')) {
        pos--;
    }

    if (!pos) { // no extension
        dest.Clear();
    } else {
        Right(len - pos, dest);
    }
}

/*
-------------------------------------------------------------------------------

    char * methods to replace library functions

-------------------------------------------------------------------------------
*/

int Str::Cmp(const char *s1, const char *s2) {
    int	c1,  c2;
    
    do {
        c1 = *s1++;
        c2 = *s2++;
        
        int d = c1 - c2;
        if (d) {
            return d;
        }
    } while (c2);
    
    return 0;   // strings are equal
}

int Str::Cmpn(const char *s1, const char *s2, int n) {
    int	c1,  c2;

    assert(n >= 0);
    
    do {
        c1 = *s1++;
        c2 = *s2++;

        if (!n--) {
            return 0;   // strings are equal until end point
        }
        
        int d = c1 - c2;
        if (d) {
            return d < 0 ? -1 : 1;
        }
    } while (c1);
    
    return 0;   // strings are equal
}
           
int Str::Icmp(const char *s1, const char *s2) {
    int	c1,  c2;

    do {
        c1 = *s1++;
        c2 = *s2++;

        int d = c1 - c2;
        
        while (d) {
            if (c1 >= 'A' && c1 <= 'Z') {
                d += ('a' - 'A');
                if (!d) {
                    break;
                }
            }

            if (c2 >= 'A' && c2 <= 'Z') {
                d -= ('a' - 'A');
                if (!d) {
                    break;
                }
            }
            return d < 0 ? -1 : 1;
        }
    } while (c1);
    
    return 0;   // strings are equal
}

int Str::Icmpn(const char *s1, const char *s2, int n) {
    int	c1,  c2;

    assert(n >= 0);
    
    do {
        c1 = *s1++;
        c2 = *s2++;

        if (!n--) {
            return 0;   // strings are equal until end point
        }

        int d = c1 - c2;
        
        while (d) {
            if (c1 >= 'A' && c1 <= 'Z') {
                d += ('a' - 'A');
                if (!d) {
                    break;
                }
            }

            if (c2 >= 'A' && c2 <= 'Z') {
                d -= ('a' - 'A');
                if (!d) {
                    break;
                }
            }
            return d < 0 ? -1 : 1;
        }
    } while (c1);
    
    return 0;   // strings are equal
}

int Str::IcmpPath(const char *s1, const char *s2) {
    int c1, c2;

    do {
        c1 = *s1++;
        c2 = *s2++;

        int d = c1 - c2;
        while (d) {
            if (c1 <= 'Z' && c1 >= 'A') {
                d += ('a' - 'A');
                if (!d) {
                    break;
                }
            }

            if (c1 == '\\') {
                d += ('/' - '\\');
                if (!d) {
                    break;
                }
            }

            if (c2 <= 'Z' && c2 >= 'A') {
                d -= ('a' - 'A');
                if (!d) {
                    break;
                }
            }

            if (c2 == '\\') {
                d -= ('/' - '\\');
                if (!d) {
                    break;
                }
            }

            // make sure folders come first
            while (c1) {
                if (c1 == '/' || c1 == '\\') {
                    break;
                }
                c1 = *s1++;
            }

            while (c2) {
                if (c2 == '/' || c2 == '\\') {
                    break;
                }
                c2 = *s2++;
            }

            if (c1 && !c2) {
                return 1;
            } else if (!c1 && c2) {
                return -1;
            }

            // same folder depth so use the regular compare
            return d < 0 ? -1 : 1;
        }
    } while (c1);

    return 0;
}

int Str::IcmpnPath(const char *s1, const char *s2, int n) {
    int c1, c2;

    assert(n >= 0);

    do {
        c1 = *s1++;
        c2 = *s2++;

        if (!n--) {
            return 0;       // strings are equal until end point
        }

        int d = c1 - c2;
        while (d) {
            if (c1 <= 'Z' && c1 >= 'A') {
                d += ('a' - 'A');
                if (!d) {
                    break;
                }
            }

            if (c1 == '\\') {
                d += ('/' - '\\'); 
                if (!d) {
                    break;
                }
            }

            if (c2 <= 'Z' && c2 >= 'A') {
                d -= ('a' - 'A');
                if (!d) {
                    break;
                }
            }

            if (c2 == '\\') {
                d -= ('/' - '\\');
                if (!d) {
                    break;
                }
            }

            // make sure folders come first
            while (c1) {
                if (c1 == '/' || c1 == '\\') {
                    break;
                }
                c1 = *s1++;
            }

            while (c2) {
                if (c2 == '/' || c2 == '\\') {
                    break;
                }
                c2 = *s2++;
            }

            if (c1 && !c2) {
                return 1;
            } else if (!c1 && c2) {
                return -1;
            }

            // same folder depth so use the regular compare
            return d < 0 ? -1 : 1;
        }
    } while (c1);

    return 0;
}

// natsort (zlib license) - http://sourcefrog.net/projects/natsort/

/* CUSTOMIZATION SECTION
 *
 * You can change this typedef, but must then also change the inline
 * functions in strnatcmp.c */
typedef char nat_char;

/* These are defined as macros to make it easier to adapt this code to
 * different characters types or comparison functions. */
static inline int nat_isdigit(nat_char a) {
    return isdigit((unsigned char) a);
}

static inline int nat_isspace(nat_char a) {
    return isspace((unsigned char) a);
}

static inline nat_char nat_toupper(nat_char a) {
    return toupper((unsigned char) a);
}

static int compare_right(nat_char const *a, nat_char const *b) {
    int bias = 0;

    /* The longest run of digits wins.  That aside, the greatest
       value wins, but we can't know that it will until we've scanned
       both numbers to know that they have the same magnitude, so we
       remember it in BIAS. */
    for (;; a++, b++) {
        if (!nat_isdigit(*a)  &&  !nat_isdigit(*b))
            return bias;
        else if (!nat_isdigit(*a))
            return -1;
        else if (!nat_isdigit(*b))
            return +1;
        else if (*a < *b) {
            if (!bias)
                bias = -1;
        } else if (*a > *b) {
            if (!bias)
                bias = +1;
        } else if (!*a  &&  !*b)
            return bias;
    }

    return 0;
}

static int compare_left(nat_char const *a, nat_char const *b) {
    /* Compare two left-aligned numbers: the first to have a
       different value wins. */
    for (;; a++, b++) {
        if (!nat_isdigit(*a)  &&  !nat_isdigit(*b))
            return 0;
        else if (!nat_isdigit(*a))
            return -1;
        else if (!nat_isdigit(*b))
            return +1;
        else if (*a < *b)
            return -1;
        else if (*a > *b)
            return +1;
    }

    return 0;
}

static int strnatcmp0(nat_char const *a, nat_char const *b, int fold_case) {
    int ai, bi;
    nat_char ca, cb;
    int fractional, result;

    assert(a && b);
    ai = bi = 0;
    while (1) {
        ca = a[ai]; cb = b[bi];

        /* skip over leading spaces or zeros */
        while (nat_isspace(ca))
            ca = a[++ai];

        while (nat_isspace(cb))
            cb = b[++bi];

        /* process run of digits */
        if (nat_isdigit(ca)  &&  nat_isdigit(cb)) {
            fractional = (ca == '0' || cb == '0');

            if (fractional) {
                if ((result = compare_left(a+ai, b+bi)) != 0)
                    return result;
            } else {
                if ((result = compare_right(a+ai, b+bi)) != 0)
                    return result;
            }
        }

        if (!ca && !cb) {
            /* The strings compare the same.  Perhaps the caller
               will want to call strcmp to break the tie. */
            return 0;
        }

        if (fold_case) {
            ca = nat_toupper(ca);
            cb = nat_toupper(cb);
        }

        if (ca < cb)
            return -1;
        else if (ca > cb)
            return +1;

        ++ai; ++bi;
    }
}

int Str::CmpNat(const char *s1, const char *s2) {
    return strnatcmp0(s1, s2, 0);
}

/* Compare, recognizing numeric string and ignoring case. */
int Str::IcmpNat(const char *s1, const char *s2) {
    return strnatcmp0(s1, s2, 1);
}

float Str::FuzzyScore(const char *s1, const char *s2, float fuzziness) {
    const static float ACRONYM_BONUS = 0.8f;
    const static float CONSECUTIVE_BONUS = 0.6f;
    const static float START_OF_STR_BONUS = 0.1f;
    const static float SAMECASE_BONUS = 0.1f;
    const static float MATCH_BONUS = 0.1f;

    // If the original string is equal to the abbreviation, perfect match.
    if (!Str::Cmp(s1, s2)) {
        return 1.0f;
    }

    size_t s2_len = Str::Length(s2);

    // If the comparison string is empty, perfectly bad match.
    if (s2_len == 0) {
        return 0.0f;
    }
    
    // Create a copy of original string, so that we can manipulate it. */
    const char *s1_ptr = s1;
    
    float score = 0.0f;
    int start_of_string_bonus = 0;
    char c_cases[] = "  ";    
    float fuzzies = 1.0f;
    
    // Walk through abbreviation and add up scores.
    for (size_t i = 0; i < s2_len; ++i) {
        // Find the first case-insensitive match of a character.
        int c = s2[i];
        c_cases[0] = Str::ToUpper(c);
        c_cases[1] = Str::ToUpper(c);
        size_t c_index = strcspn(s1_ptr, c_cases);
        
        // Set base score for any matching char.
        if (c_index == Str::Length(s1_ptr)) {
            if (fuzziness > 0.0f) {
                fuzzies += 1.0f - fuzziness;
                continue;
            }
            return 0.0f;
        } else {
            score += MATCH_BONUS;
        }

        // Same-case bonus.
        if (s1_ptr[c_index] == c) {
            score += SAMECASE_BONUS;
        }
        
        // Consecutive letter & start-of-string bonus.
        if (c_index == 0) {
            // Increase the score when matching first character of the remainder of the string.
            score += CONSECUTIVE_BONUS;
            if (i == 0) {
                // If match is the first char of the string & the first char
                // of abbreviation, add a start-of-string match bonus.
                start_of_string_bonus = 1;
            }
        } else if (s1_ptr[c_index - 1] == ' ') {
            // Acronym Bonus
            // Weighing Logic: Typing the first character of an acronym is as
            // if you preceded it with two perfect character matches.
            score += ACRONYM_BONUS;
        }
        
        // Left trim the already matched part of the string. 
        // (Forces sequential matching.)
        s1_ptr += c_index + 1;
    }
    
    score /= (float)s2_len;
    
    // Reduce penalty for longer strings.
    score = ((score * (s2_len / (float)Str::Length(s1))) + score) / 2;
    score /= fuzzies;
    
    if (start_of_string_bonus && (score + START_OF_STR_BONUS < 1)) {
        score += START_OF_STR_BONUS;
    }
    
    return score;
}

// Safe strncpy that ensures a trailing zero
void Str::Copynz(char *dest, const char *src, int destsize) {
    if (!src) {
        BE_WARNLOG(L"Str::Copynz: nullptr src\n");
        return;
    }

    if (destsize < 1) {
        BE_WARNLOG(L"Str::Copynz: destsize < 1\n");
        return;
    }

    strncpy(dest, src, destsize-1);
    dest[destsize-1] = 0;
}

// never goes past bounds or leaves without a terminating 0
void Str::Append(char *dest, int size, const char *src) {
    int l1 = (int)strlen(dest);
    if (l1 >= size) {
        BE_ERRLOG(L"Str::Append: already overflowed\n");
    }
    
    Str::Copynz(dest + l1, src, size - l1);
}

// returns -1 if not found otherwise the index of the char
int Str::FindChar(const char *str, const char c, int start, int end) {
    if (end == -1) {
        end = (int)strlen(str) - 1;
    }
    
    for (int i = start; i <= end; i++) {
        if (str[i] == c) {
            return i;
        }
    }
    return -1;
}

// returns -1 if not found otherwise the index of the char
int Str::FindLastChar(const char *str, const char c, int start, int end) {
    if (end == -1) {
        end = (int)strlen(str) - 1;
    }

    for (int i = end; i >= start; i--) {
        if (str[i] == c) {
            return i;
        }
    }
    return -1;
}

// returns -1 if not found otherwise the index of the text
int Str::FindText(const char *str, const char *text, bool caseSensitive, int start, int end) {
    if (end == -1) {
        end = (int)strlen(str);
    }
    
    int l = end - (int)strlen(text);
    for (int i = start; i <= l; i++) {
        int j;
        if (caseSensitive) {
            for (j = 0; text[j]; j++) {
                if (str[i+j] != text[j]) {
                    break;
                }
            }
        } else {
            for (j = 0; text[j]; j++) {
                if (::toupper(str[i+j]) != ::toupper(text[j])) {
                    break;
                }
            }
        }

        if (!text[j]) {
            return i;
        }
    }
    return -1;
}

// returns -1 if not found otherwise the index of the text
int Str::FindLastText(const char *str, const char *text, bool caseSensitive, int start, int end) {
    if (end == -1) {
        end = (int)strlen(str);
    }

    int idx = -1;
    int l = end - (int)strlen(text);
    for (int i = start; i <= l; i++) {
        int j;
        if (caseSensitive) {
            for (j = 0; text[j]; j++) {
                if (str[i + j] != text[j]) {
                    break;
                }
            }
        } else {
            for (j = 0; text[j]; j++) {
                if (::toupper(str[i + j]) != ::toupper(text[j])) {
                    break;
                }
            }
        }

        if (!text[j]) {
            idx = i;
        }
    }
    return idx;
}

bool Str::Filter(const char *filter, const char *name, bool caseSensitive) {
    Str namestr;
    Str buf;

    while (*filter) {
        if (*filter == '*') {
            filter++;
            buf.Clear();
            for (int i = 0; *filter; i++) {
                if (*filter == '*' || *filter == '?' || (*filter == '[' && *(filter+1) != '[')) {
                    break;
                }
                
                buf += *filter;
                if (*filter == '[') {
                    filter++;
                }
            
                filter++;
            }

            if (buf.Length()) {
                namestr = name;
                int index = namestr.Find(buf.c_str(), caseSensitive);
                if (index == -1) {
                    return false;
                }
                
                name += index + buf.Length();
            }
        } else if (*filter == '?') {
            filter++;
            name++;
        } else if (*filter == '[') {
            if (*(filter+1) == '[') {
                if (*name != '[') {
                    return false;
                }
                
                filter += 2;
                name++;
            } else {
                filter++;
                bool found = false;
                while (*filter && !found) {
                    if (*filter == ']' && *(filter+1) != ']') {
                        break;
                    }
                
                    if (*(filter+1) == '-' && *(filter+2) && (*(filter+2) != ']' || *(filter+3) == ']')) {
                        if (caseSensitive) {
                            if (*name >= *filter && *name <= *(filter+2)) {
                                found = true;
                            }
                        } else {
                            if (::toupper(*name) >= ::toupper(*filter) && ::toupper(*name) <= ::toupper(*(filter+2))) {
                                found = true;
                            }
                        }
                        filter += 3;
                    } else {
                        if (caseSensitive) {
                            if (*filter == *name) {
                                found = true;
                            }
                        } else {
                            if (::toupper(*filter) == ::toupper(*name)) {
                                found = true;
                            }
                        }
                        filter++;
                    }
                }

                if (!found) {
                    return false;
                }

                while (*filter) {
                    if (*filter == ']' && *(filter+1) != ']') {
                        break;
                    }
                    filter++;
                }
                filter++;
                name++;
            }
        } else {
            if (caseSensitive) {
                if (*filter != *name) {
                    return false;
                }
            } else {
                if (::toupper(*filter) != ::toupper(*name)) {
                    return false;
                }
            }
            filter++;
            name++;
        }
    }

    if (buf.Length() && *name != '\0') {
        return false;
    }
    return true;
}

bool Str::CheckExtension(const char *name, const char *ext) {
    const char *s1 = name + Length(name) - 1;
    const char *s2 = ext + Length(ext) - 1;

    do {
        int c1 = *s1--;
        int c2 = *s2--;

        int d = c1 - c2;
        while (d) {
            if (c1 <= 'Z' && c1 >= 'A') {
                d += ('a' - 'A');
                if (!d) {
                    break;
                }
            }

            if (c2 <= 'Z' && c2 >= 'A') {
                d -= ('a' - 'A');
                if (!d) {
                    break;
                }
            }

            return false;
        }
    } while (s1 >= name && s2 >= ext);

    return (s1 >= name);
}

void Str::ConvertPathSeperator(char *path, char separator) {
    for (int i = 0; path[i]; i++) {
        if (path[i] == '/' || path[i] == '\\') {
            path[i] = separator;
        }
    }
}

const char *Str::IntegerArrayToString(const int *arr, const int length) {
    static int index = 0;
    static char str[4][16384];  // in case called by nested functions
    
    // use an array of string so that multiple calls won't collide
    char *s = str[index];
    index = (index + 1) & 3;

    int n = Str::snPrintf(s, sizeof(str[0]), "%i", arr[0]);
    for (int i = 1; i < length; i++) {
        n += Str::snPrintf(s + n, sizeof(str[0]) - n, " %i", arr[i]);
    }
    return s;
}

const char *Str::FloatArrayToString(const float *arr, const int length, const int precision) {
    static int index = 0;
    static char str[4][16384];  // in case called by nested functions
    
    // use an array of string so that multiple calls won't collide
    char *s = str[index];
    index = (index + 1) & 3;

    char format[16];
    Str::snPrintf(format, sizeof(format), "%%.%if", precision);
    int n = Str::snPrintf(s, sizeof(str[0]), format, arr[0]);
    if (precision > 0) {
        while (n > 0 && s[n-1] == '0') s[--n] = '\0';
        while (n > 0 && s[n-1] == '.') s[--n] = '\0';
    }
    Str::snPrintf(format, sizeof(format), " %%.%if", precision);
    for (int i = 1; i < length; i++) {
        n += Str::snPrintf(s + n, sizeof(str[0]) - n, format, arr[i]);
        if (precision > 0) {
            while (n > 0 && s[n-1] == '0') s[--n] = '\0';
            while (n > 0 && s[n-1] == '.') s[--n] = '\0';
        }
    }
    return s;
}

int BE_CDECL Str::snPrintf(char *dest, int size, const char *fmt, ...) {
    va_list argptr;
    char buffer[32000];	// big, but small enough to fit in PPC stack
    va_start(argptr, fmt);
    int len = ::vsprintf(buffer, fmt, argptr);
    va_end(argptr);

    if (len >= COUNT_OF(buffer)) {
        BE_FATALERROR(L"Str::snPrintf: overflowed buffer\n");	
    }

    if (len >= size) {
        BE_WARNLOG(L"Str::snPrintf: overflow of %i in %i\n", len, size);
        len = size;
    }

    Str::Copynz(dest, buffer, size);
    return len;
}

// vsnprintf portability:
// 
// C99 standard: vsnprintf returns the number of characters (excluding the trailing '\0') 
// which would have been written to the final string if enough space had been available
// snprintf and vsnprintf do not write more than size bytes (including the trailing '\0')
// 
// win32: _vsnprintf returns the number of characters written, not including the terminating null character,
// or a negative value if an output error occurs. If the number of characters to write exceeds count, then count 
// characters are written and -1 is returned and no trailing '\0' is added.
// 
// Str::vsnPrintf: always appends a trailing '\0', returns number of characters written (not including terminal \0)
// or returns -1 on failure or if the buffer would be overflowed.
int BE_CDECL Str::vsnPrintf(char *dest, int size, const char *fmt, va_list argptr) {
#ifdef __WIN32__
    #undef _vsnprintf
    int ret = _vsnprintf(dest, size - 1, fmt, argptr);
    #define _vsnprintf	use_cStr_vsnPrintf
#else
    #undef vsnprintf
    int ret = vsnprintf(dest, size, fmt, argptr);
    #define vsnprintf	use_cStr_vsnPrintf
#endif
    dest[size-1] = '\0';
    if (ret < 0 || ret >= size) {
        return -1;
    }
    return ret;
}

WStr Str::ToWStr(const char *str) {
    size_t count = mbstowcs(nullptr, str, 0); // wide character string 의 개수를 얻는다
    wchar_t *wcstr = (wchar_t *)_alloca((count + 1) * sizeof(wchar_t)); // nullptr 문자도 포함해서 wchar 로 변환
    mbstowcs(wcstr, str, count + 1);
    return WStr(wcstr);
}

// Sets the value of the string using a printf interface.
int BE_CDECL sprintf(Str &string, const char *fmt, ...) {
    va_list argptr;
    char buffer[32000];
    va_start(argptr, fmt);
    int l = Str::vsnPrintf(buffer, sizeof(buffer)-1, fmt, argptr);
    va_end(argptr);
    buffer[sizeof(buffer)-1] = '\0';

    string = buffer;
    return l;
}

// Sets the value of the string using a vprintf interface.
int BE_CDECL vsprintf(Str &string, const char *fmt, va_list argptr) {
    char buffer[32000];
    int l = Str::vsnPrintf(buffer, sizeof(buffer)-1, fmt, argptr);
    buffer[sizeof(buffer)-1] = '\0';
    
    string = buffer;
    return l;
}

// does a varargs printf into a temp buffer
// NOTE: not thread safe
#define VA_BUF_LEN 16384
char BE_CDECL *va(const char *fmt, ...) {
    va_list argptr;
    static int index = 0;
    static char string[4][VA_BUF_LEN];  // 재귀 호출이 4번까지는 가능하다.
    char *buf = string[index];
    index = (index + 1) & 3;

    va_start(argptr, fmt);
    ::vsprintf(buf, fmt, argptr);
    va_end(argptr);

    return buf;
}

BE_NAMESPACE_END
