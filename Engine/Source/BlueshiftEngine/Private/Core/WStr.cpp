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
#include "Core/WStr.h"
#include "Containers/StrArray.h"

BE_NAMESPACE_BEGIN

WStr WStr::empty;

void WStr::ReAllocate(int amount, bool keepOld) {
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
    wchar_t *newBuffer = stringDataAllocator.Alloc(alloced);
#else
    wchar_t *newBuffer = new wchar_t[alloced];
#endif
    if (keepOld && data && len > 0) {
        data[len] = L'\0';
        wcscpy(newBuffer, data);
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

void WStr::FreeData() {
    if (data && data != baseBuffer) {
#ifdef USE_STRING_DATA_ALLOCATOR
        stringDataAllocator.Free(data);
#else
        delete [] data;
#endif
        data = baseBuffer;
    }
}

WStr &WStr::operator=(const wchar_t *text) {
    if (!text) {
        // safe behaviour if nullptr
        EnsureAlloced(1, false);
        data[0] = L'\0';
        len = 0;
        return *this;
    }

    if (text == data) {
        return *this; // copying same thing
    }

    // check if we're aliasing
    if (text >= data && text <= data + len) {
        int diff = (int)(text - data);

        assert(wcslen(text) < (unsigned)len);

        int i;
        for (i = 0; text[i]; i++) {
            data[i] = text[i];
        }

        data[i] = L'\0';

        len -= diff;

        return *this;
    }

    int l = (int)wcslen(text);
    EnsureAlloced(l + 1, false);
    wcscpy(data, text);
    len = l;

    return *this;
}

const wchar_t *WStr::Mid(int start, int len, WStr &result) const {
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

WStr WStr::Mid(int start, int len) const {
    WStr result;

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

WStr WStr::Commafy() const {
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
    
    WStr result;
    result.EnsureAlloced(len + 1 + (digits - 1) / 3);

    int index = 0;
    for (i = 0; i < len; i++) {
        if (i > start && i < end) {
            if (digits % 3) {
                result.data[index++] = L',';
            }
        }

        result.data[index++] = data[i];
        digits--;
    }

    result.data[index] = L'\0';

    return result;
}

WStr WStr::NumberedName(int *number) const {
    int index = len - 1;
    while (index >= 0) {
        if (!WStr::CharIsNumeric(data[index])) {
            break;
        }

        index--;
    }

    if (index < len - 1) {
        *number = (int)wcstol(&data[index + 1], nullptr, 10);
    } else {
        *number = 0;
    }

    // skip whitespaces
    while (index >= 0) {
        if (data[index] != L' ' && data[index] != L'\t') {
            break;
        }

        index--;
    }

    WStr basename = Left(index + 1);
    return basename;
}

int BE_CDECL WStr::sPrintf(const wchar_t *fmt, ...) {
    va_list argptr;
    wchar_t buffer[32000];	// big, but small enough to fit in PPC stack
    va_start(argptr, fmt);
    len = WStr::vsnPrintf(buffer, COUNT_OF(buffer) - 1, fmt, argptr);
    va_end(argptr);

    EnsureAlloced(len + 1, false);
    WStr::Copynz(data, buffer, len + 1);

    return len;
}

WStr WStr::FormatBytes(int bytes) {
    WStr s;
    if (bytes < 1024) {
        s.sPrintf(L"%i bytes", bytes);
    } else if (bytes < 1024 * 1024) {
        s.sPrintf(L"%.1f Kb", bytes / (float)(1024));
    } else if (bytes < 1024 * 1024 * 1024) {
        s.sPrintf(L"%.1f Mb", bytes / (float)(1024 * 1024));
    } else {
        s.sPrintf(L"%.1f Gb", bytes / (float)(1024 * 1024 * 1024));
    }

    return s;
}

/*
-------------------------------------------------------------------------------

filename methods

-------------------------------------------------------------------------------
*/

int WStr::FileNameHash() const {
    long hash = 0;
    for (int i = 0; data[i] != L'\0'; i++) {
        wchar_t letter = WStr::ToLower(data[i]);
        if (letter == '.') {
            break; // don't include extension
        }

        if (letter ==L'\\') {
            letter = L'/';
        }

        hash += (long)(letter) * (i + 119);
    }
    hash &= (FileNameHashSize - 1);
    return (int)hash;
}

WStr &WStr::BackSlashesToSlashes() {
    for (int i = 0; i < len; i++) {
        if (data[i] == L'\\')
            data[i] = L'/';
    }
    return *this;
}

WStr &WStr::SlashesToBackSlashes() {
    for (int i = 0; i < len; i++) {
        if (data[i] == L'/') {
            data[i] = L'\\';
        }
    }
    return *this;
}

const WStr WStr::ToRelativePath(const wchar_t *basePath) const {
    WStr a_path = data;
    WStr base_path = basePath;

    a_path.CleanPath(PATHSEPERATOR_WCHAR);
    base_path.CleanPath(PATHSEPERATOR_WCHAR);

    WStrArray a_path_list;
    WStrArray base_path_list;
    SplitStringIntoList(a_path_list, a_path, PATHSEPERATOR_WSTR);
    SplitStringIntoList(base_path_list, base_path, PATHSEPERATOR_WSTR);

    int idx;
    for (idx = 0; idx < a_path_list.Count() && idx < base_path_list.Count(); idx++) {
        if (a_path_list[idx] != base_path_list[idx]) {
            break;
        }
    }
    assert(idx > 0);

    WStr result;
    for (int i = idx; i < base_path_list.Count(); i++) {
        if (!result.IsEmpty()) {
            result += PATHSEPERATOR_WSTR;
        }
        result += L"..";
    }

    for (int i = idx; i < a_path_list.Count(); i++) {
        if (!result.IsEmpty()) {
            result += PATHSEPERATOR_WSTR;
        }
        result += a_path_list[i];
    }

    return result;
}

const WStr WStr::ToAbsolutePath(const wchar_t *basePath) const {
    WStr result = basePath;
    result.AppendPath(data);
    result.CleanPath(PATHSEPERATOR_WCHAR);
    return result;
}

WStr &WStr::StripFileExtension() {
    for (int i = len - 1; i > 0; i--) {
        if (data[i] == L'/' || data[i] == L'\\') {
            break;
        }

        if (data[i] == L'.') {
            data[i] = L'\0';
            len = i;
            break;
        }
    }
    return *this;
}

WStr &WStr::StripFileName() {
    int pos = Length() - 1;
    while ((pos > 0) && ((*this)[pos] != L'/') && ((*this)[pos] != L'\\')) {
        pos--;
    }

    if (pos < 0) {
        pos = 0;
    }

    Truncate(pos);
    return *this;
}

WStr &WStr::StripPath() {
    int pos = Length();
    while ((pos > 0) && ((*this)[pos - 1] != L'/') && ((*this)[pos - 1] != L'\\')) {
        pos--;
    }

    *this = Right(Length() - pos);
    return *this;
}

WStr &WStr::SetFileExtension(const wchar_t *extension) {
    StripFileExtension();
    if (extension[0] && extension[0] != L'.') {
        Append(L'.');
    }

    Append(extension);
    return *this;
}

WStr &WStr::DefaultFileExtension(const wchar_t *extension) {
    // 확장자가 있다면 그냥 리턴
    for (int i = len-1; i >= 0; i--) {
        if (data[i] == L'.') {
            return *this;
        }
    }

    if (*extension != L'.') {
        Append(L'.');
    }

    Append(extension);
    return *this;
}

WStr &WStr::DefaultPath(const wchar_t *basepath) {
    if (((*this)[0] == L'/') || ((*this)[0] == L'\\')) {
        // absolute path location
        return *this;
    }

    *this = basepath + *this;
    return *this;
}

WStr &WStr::AppendPath(const wchar_t *text, wchar_t pathSeparator) {
    if (len > 0 && text && text[0]) {
        if (data[len - 1] != L'/' && data[len - 1] != L'\\') {
            Append(pathSeparator);
        }
    }

    if (text[0] == L'/' || text[0] == L'\\') {
        text++;
    }
    
    Append(text);
    return *this;
}

WStr &WStr::CleanPath(wchar_t pathSeparator) {
    WStr a_path = data;

    ConvertPathSeperator(a_path.data, L'/');

    WStrArray a_path_list;
    SplitStringIntoList(a_path_list, a_path, L"/");

    for (int i = 0; i < a_path_list.Count(); i++) {
        if (a_path_list[i] == L".") {
            a_path_list.RemoveIndex(i);

            i--;
        } else if (i > 0 && a_path_list[i - 1] != L".." && a_path_list[i] == L"..") {
            a_path_list.RemoveIndex(i);
            a_path_list.RemoveIndex(i - 1);
            
            i -= 2;
        }
    }

    WStr result;
    for (int i = 0; i < a_path_list.Count(); i++) {
        if (i > 0) {
            result += pathSeparator;
        }
        result += a_path_list[i];
    }

    *this = result;
    return *this;
}

void WStr::ExtractFilePath(WStr &dest) const {
    //
    // back up until a \ or the start
    //
    int len = Length();
    if (len == 0) {
        dest.Clear();
        return;
    }

    int pos = len;
    while ((pos > 0) && ((*this)[pos - 1] != L'/') && ((*this)[pos - 1] != L'\\')) {
        pos--;
    }

    Left(pos, dest);
}

void WStr::ExtractFileName(WStr &dest) const {
    //
    // back up until a \ or the start
    //
    int len = Length();
    if (len == 0) {
        dest.Clear();
        return;
    }

    int pos = len - 1;
    while ((pos > 0) && ((*this)[pos - 1] != L'/') && ((*this)[pos - 1] != L'\\')) {
        pos--;
    }

    Right(len - pos, dest);
}

void WStr::ExtractFileBase(WStr &dest) const {
    //
    // back up until a \ or the start
    //
    int len = Length();
    if (len == 0) {
        dest.Clear();
        return;
    }

    int pos = len - 1;
    while ((pos > 0) && ((*this)[pos - 1] != L'/') && ((*this)[pos - 1] != L'\\')) {
        pos--;
    }

    int start = pos;
    while ((pos < len) && ((*this)[pos] != L'.')) {
        pos++;
    }

    Mid(start, pos - start, dest);
}

void WStr::ExtractFileExtension(WStr &dest) const {
    //
    // back up until a . or the start
    //
    int len = Length();
    if (len == 0) {
        dest.Clear();
        return;
    }

    int pos = len - 1;
    while ((pos > 0) && ((*this)[pos - 1] != L'.')) {
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

wchar_t * methods to replace library functions

-------------------------------------------------------------------------------
*/

int WStr::Cmp(const wchar_t *s1, const wchar_t *s2) {
    int	c1,  c2;

    do {
        c1 = *s1++;
        c2 = *s2++;

        int d = c1 - c2;
        if (d) {
            return d < 0 ? -1 : 1;
        }
    } while (c1);

    return 0;   // strings are equal
}

int WStr::Cmpn(const wchar_t *s1, const wchar_t *s2, int n) {
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
            return  d < 0 ? -1 : 1;
        }
    } while (c1);

    return 0;   // strings are equal
}

int WStr::Icmp(const wchar_t *s1, const wchar_t *s2) {
    int	c1,  c2;

    do {
        c1 = *s1++;
        c2 = *s2++;

        int d = c1 - c2;

        while (d) {
            if (c1 >= L'A' && c1 <= L'Z') {
                d += (L'a' - L'A');
                if (!d) {
                    break;
                }
            }

            if (c2 >= L'A' && c2 <= L'Z') {
                d -= (L'a' - L'A');
                if (!d) {
                    break;
                }
            }
            return d < 0 ? -1 : 1;
        }
    } while (c1);

    return 0;   // strings are equal
}

int WStr::Icmpn(const wchar_t *s1, const wchar_t *s2, int n) {
    int	c1,  c2;

    assert(n >= 0);

    do {
        c1 = *s1++;
        c2 = *s2++;

        if (!n--)
            return 0;   // strings are equal until end point

        int d = c1 - c2;

        while (d) {
            if (c1 >= L'A' && c1 <= L'Z') {
                d += (L'a' - L'A');
                if (!d) {
                    break;
                }
            }

            if (c2 >= L'A' && c2 <= L'Z') {
                d -= (L'a' - L'A');
                if (!d) {
                    break;
                }
            }

            return d < 0 ? -1 : 1;
        }
    } while (c1);

    return 0;   // strings are equal
}

int WStr::IcmpPath(const wchar_t *s1, const wchar_t *s2) {
    int c1, c2;

    do {
        c1 = *s1++;
        c2 = *s2++;

        int d = c1 - c2;
        while (d) {
            if (c1 <= L'Z' && c1 >= L'A') {
                d += (L'a' - L'A');
                if (!d) {
                    break;
                }
            }

            if (c1 == L'\\') {
                d += (L'/' - L'\\');
                if (!d) {
                    break;
                }
            }

            if (c2 <= L'Z' && c2 >= L'A') {
                d -= (L'a' - L'A');
                if (!d) {
                    break;
                }
            }

            if (c2 == L'\\') {
                d -= (L'/' - L'\\');
                if (!d) {
                    break;
                }
            }

            // make sure folders come first
            while (c1) {
                if (c1 == L'/' || c1 == L'\\') {
                    break;
                }
                c1 = *s1++;
            }

            while (c2) {
                if (c2 == L'/' || c2 == L'\\') {
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

int WStr::IcmpnPath(const wchar_t *s1, const wchar_t *s2, int n) {
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
            if (c1 <= L'Z' && c1 >= L'A') {
                d += (L'a' - L'A');
                if (!d) {
                    break;
                }
            }

            if (c1 == L'\\') {
                d += (L'/' - L'\\');
                if (!d) {
                    break;
                }
            }

            if (c2 <= L'Z' && c2 >= L'A') {
                d -= (L'a' - L'A');
                if (!d) {
                    break;
                }
            }

            if (c2 == L'\\') {
                d -= (L'/' - L'\\');
                if (!d) {
                    break;
                }
            }

            // make sure folders come first
            while (c1) {
                if (c1 == L'/' || c1 == L'\\') {
                    break;
                }
                c1 = *s1++;
            }

            while (c2) {
                if (c2 == L'/' || c2 == L'\\') {
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
typedef wchar_t nat_char;

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

int WStr::CmpNat(const wchar_t *s1, const wchar_t *s2) {
    return strnatcmp0(s1, s2, 0);
}

/* Compare, recognizing numeric string and ignoring case. */
int WStr::IcmpNat(const wchar_t *s1, const wchar_t *s2) {
    return strnatcmp0(s1, s2, 1);
}

float WStr::FuzzyScore(const wchar_t *s1, const wchar_t *s2, float fuzziness) {
    const static float ACRONYM_BONUS = 0.8f;
    const static float CONSECUTIVE_BONUS = 0.6f;
    const static float START_OF_STR_BONUS = 0.1f;
    const static float SAMECASE_BONUS = 0.1f;
    const static float MATCH_BONUS = 0.1f;

    // If the original string is equal to the abbreviation, perfect match.
    if (!WStr::Cmp(s1, s2)) {
        return 1.0f;
    }

    size_t s2_len = WStr::Length(s2);

    // If the comparison string is empty, perfectly bad match.
    if (s2_len == 0) {
        return 0.0f;
    }

    // Create a copy of original string, so that we can manipulate it. */
    const wchar_t *s1_ptr = s1;

    float score = 0.0f;
    int start_of_string_bonus = 0;
    wchar_t c_cases[] = L"  ";
    float fuzzies = 1.0f;

    // Walk through abbreviation and add up scores.
    for (size_t i = 0; i < s2_len; ++i) {
        // Find the first case-insensitive match of a character.
        int c = s2[i];
        c_cases[0] = WStr::ToUpper(c);
        c_cases[1] = WStr::ToUpper(c);
        size_t c_index = wcscspn(s1_ptr, c_cases);

        // Set base score for any matching char.
        if (c_index == WStr::Length(s1_ptr)) {
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
    score = ((score * (s2_len / (float)WStr::Length(s1))) + score) / 2;
    score /= fuzzies;

    if (start_of_string_bonus && (score + START_OF_STR_BONUS < 1)) {
        score += START_OF_STR_BONUS;
    }

    return score;
}

// Safe strncpy that ensures a trailing zero
void WStr::Copynz(wchar_t *dest, const wchar_t *src, int destsize) {
    if (!src) {
        BE_WARNLOG(L"WStr::Copynz: nullptr src\n");
        return;
    }

    if (destsize < 1) {
        BE_WARNLOG(L"WStr::Copynz: destsize < 1\n"); 
        return;
    }

    wcsncpy(dest, src, destsize - 1);
    dest[destsize - 1] = 0;
}

// never goes past bounds or leaves without a terminating 0
void WStr::Append(wchar_t *dest, int size, const wchar_t *src) {
    int l1 = (int)wcslen(dest);
    if (l1 >= size) {
        BE_ERRLOG(L"WStr::Append: already overflowed\n");
    }

    WStr::Copynz(dest + l1, src, size - l1);
}

// returns -1 if not found otherwise the index of the wchar_t
int WStr::FindChar(const wchar_t *str, const wchar_t c, int start, int end) {
    if (end == -1) {
        end = (int)wcslen(str) - 1;
    }

    for (int i = start; i <= end; i++) {
        if (str[i] == c) {
            return i;
        }
    }
    return -1;
}

// returns -1 if not found otherwise the index of the wchar_t
int WStr::FindLastChar(const wchar_t *str, const wchar_t c, int start, int end) {
    if (end == -1) {
        end = (int)wcslen(str) - 1;
    }

    for (int i = end; i >= start; i--) {
        if (str[i] == c) {
            return i;
        }
    }
    return -1;
}

// returns -1 if not found otherwise the index of the text
int WStr::FindText(const wchar_t *str, const wchar_t *text, bool caseSensitive, int start, int end) {
    if (end == -1) {
        end = (int)wcslen(str);
    }

    int l = end - (int)wcslen(text);
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
int WStr::FindLastText(const wchar_t *str, const wchar_t *text, bool caseSensitive, int start, int end) {
    if (end == -1) {
        end = (int)wcslen(str);
    }

    int idx = -1;
    int l = end - (int)wcslen(text);
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

bool WStr::Filter(const wchar_t *filter, const wchar_t *name, bool caseSensitive) {
    WStr namestr;
    WStr buf;

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
                            if (::toupper(*name) >= ::toupper(*filter) && ::toupper(*name) <= ::toupper(*(filter+2)))
                                found = true;
                        }
                        filter += 3;
                    } else {
                        if (caseSensitive) {
                            if (*filter == *name)
                                found = true;
                        } else {
                            if (::toupper(*filter) == ::toupper(*name))
                                found = true;
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

    if (buf.Length() && *name != L'\0') {
        return false;
    }
    return true;
}

bool WStr::CheckExtension(const wchar_t *name, const wchar_t *ext) {
    const wchar_t *s1 = name + Length(name) - 1;
    const wchar_t *s2 = ext + Length(ext) - 1;

    do {
        int c1 = *s1--;
        int c2 = *s2--;

        int d = c1 - c2;
        while (d) {
            if (c1 <= L'Z' && c1 >= L'A') {
                d += (L'a' - L'A');
                if (!d) {
                    break;
                }
            }

            if (c2 <= L'Z' && c2 >= L'A') {
                d -= (L'a' - L'A');
                if (!d) {
                    break;
                }
            }

            return false;
        }
    } while (s1 >= name && s2 >= ext);

    return (s1 >= name);
}

void WStr::ConvertPathSeperator(wchar_t *path, wchar_t separator) {
    for (int i = 0; path[i]; i++) {
        if (path[i] == L'/' || path[i] == L'\\') {
            path[i] = separator;
        }
    }
}

const wchar_t *WStr::IntegerArrayToString(const int *arr, const int length) {
    static int index = 0;
    static wchar_t str[4][16384];   // in case called by nested functions
    
    // use an array of string so that multiple calls won't collide
    wchar_t *s = str[index];
    index = (index + 1) & 3;

    int n = 0;
    for (int i = 0; i < length; i++) {
        n += WStr::snPrintf(s + n, sizeof(str[0]) - n, L"%i", arr[i]);
    }
    return s;
}
const wchar_t *WStr::FloatArrayToString(const float *arr, const int length, const int precision) {
    static int index = 0;
    static wchar_t str[4][16384];   // in case called by nested functions	

    // use an array of string so that multiple calls won't collide
    wchar_t *s = str[index];
    index = (index + 1) & 3;

    wchar_t format[16];
    WStr::snPrintf(format, sizeof(format), L"%%.%df", precision);
    int n = WStr::snPrintf(s, sizeof(str[0]), format, arr[0]);
    if (precision > 0) {
        while (n > 0 && s[n-1] == L'0') s[--n] = L'\0';
        while (n > 0 && s[n-1] == L'.') s[--n] = L'\0';
    }

    WStr::snPrintf(format, sizeof(format), L" %%.%df", precision);
    for (int i = 1; i < length; i++) {
        n += WStr::snPrintf(s + n, sizeof(str[0]) - n, format, arr[i]);
        if (precision > 0) {
            while (n > 0 && s[n-1] == L'0') s[--n] = L'\0';
            while (n > 0 && s[n-1] == L'.') s[--n] = L'\0';
        }
    }
    return s;
}

int BE_CDECL WStr::snPrintf(wchar_t *dest, int size, const wchar_t *fmt, ...) {
    va_list argptr;
    wchar_t buffer[32000];  // big, but small enough to fit in PPC stack
    va_start(argptr, fmt);
    int len = ::vswprintf(buffer, COUNT_OF(buffer), fmt, argptr);
    va_end(argptr);

    if (len >= COUNT_OF(buffer)) {
        BE_FATALERROR(L"WStr::snPrintf: overflowed buffer\n");
    }

    if (len >= size) {
        BE_WARNLOG(L"WStr::snPrintf: overflow of %i in %i\n", len, size);
        len = size;
    }

    WStr::Copynz(dest, buffer, size);
    return len;
}

// vsnprintf portability:
// 
// C99 standard: vsnprintf returns the number of wchar_tacters (excluding the trailing
// '\0') which would have been written to the final string if enough space had been available
// snprintf and vsnprintf do not write more than size bytes (including the trailing '\0')
// 
// win32: _vsnprintf returns the number of wchar_tacters written, not including the terminating null wchar_tacter,
// or a negative value if an output error occurs. If the number of wchar_tacters to write exceeds count, then count 
// wchar_tacters are written and -1 is returned and no trailing '\0' is added.
// 
// WStr::vsnPrintf: always appends a trailing '\0', returns number of wchar_tacters written (not including terminal \0)
// or returns -1 on failure or if the buffer would be overflowed.
int BE_CDECL WStr::vsnPrintf(wchar_t *dest, int size, const wchar_t *fmt, va_list argptr) {
#ifdef __WIN32__
    #undef _vsnwprintf
    int ret = _vsnwprintf(dest, size - 1, fmt, argptr);
    #define _vsnwprintf	use_cWStr_vsnPrintf
#else
    #undef vsnprintf
    int ret = vswprintf(dest, size, fmt, argptr);
    #define vsnprintf use_cWStr_vsnPrintf
#endif
    dest[size-1] = L'\0';
    if (ret < 0 || ret >= size) {
        return -1;
    }
    return ret;
}

Str WStr::ToStr(const wchar_t *wcstr) {
    size_t size = wcstombs(nullptr, wcstr, 0); // multi-byte string 으로 변환후의 크기를 얻는다
    char *str = (char *)_alloca(size + 1); // nullptr 문자도 포함해서 변환
    wcstombs(str, wcstr, size + 1);
    return Str(str);
}

// Sets the value of the string using a printf interface.
int BE_CDECL swprintf(WStr &string, const wchar_t *fmt, ...) {
    va_list argptr;
    wchar_t buffer[32000];
    va_start(argptr, fmt);
    int l = WStr::vsnPrintf(buffer, sizeof(buffer)-1, fmt, argptr);
    va_end(argptr);

    buffer[COUNT_OF(buffer) - 1] = L'\0';

    string = buffer;
    return l;
}

// Sets the value of the string using a vprintf interface.
int BE_CDECL vswprintf(WStr &string, const wchar_t *fmt, va_list argptr) {
    wchar_t buffer[32000];
    int l = WStr::vsnPrintf(buffer, sizeof(buffer)-1, fmt, argptr);

    buffer[COUNT_OF(buffer) - 1] = L'\0';

    string = buffer;
    return l;
}

// does a varargs printf into a temp buffer
// NOTE: not thread safe
#define VA_BUF_LEN 16384
wchar_t BE_CDECL *wva(const wchar_t *fmt, ...) {
    va_list argptr;
    static int index = 0;
    static wchar_t string[4][VA_BUF_LEN];   // 재귀 호출이 4번까지는 가능하다.
    wchar_t *buf = string[index];
    index = (index + 1) & 3;

    va_start(argptr, fmt);
    WStr::vsnPrintf(buf, VA_BUF_LEN, fmt, argptr);
    va_end(argptr);

    return buf;
}

wchar_t *towcs(const char *mbs) {
    static int index = 0;
    static wchar_t wcs[4][4096];
    wchar_t *buf = wcs[index];
    index = (index + 1) & 3;
    mbstowcs(buf, mbs, sizeof(wcs));
    return buf;
}

char *tombs(const wchar_t *wcs) {
    static int index = 0;
    static char mbs[4][4096];
    char *buf = mbs[index];
    index = (index + 1) & 3;
    wcstombs(buf, wcs, sizeof(mbs));
    return buf;
}

BE_NAMESPACE_END
