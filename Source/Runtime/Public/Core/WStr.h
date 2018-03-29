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

    Wide Character String

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class Str;

/// Wide character string class.
class BE_API WStr {
public:
    /// Construct empty string.
    WStr();
    /// Construct from another string.
    WStr(const WStr &str);
    /// Construct from rvalue string
    WStr(WStr &&str);
    /// Construct from a null-terminated C string.
    WStr(const char *text);
    /// Construct from a null-terminated wide character array.
    WStr(const wchar_t *text);

#if __OBJC__
    /// Constructs from a Objective-C NSString pointer.
    WStr(const NSString *nsstr) : WStr() {
        // get the length of the NSString
        int l = nsstr ? (int)[nsstr length] : 0;
        EnsureAlloced(l + 1, false);
        
        if (l > 0) {
            // copy the NSString's cString data into the WStr
            if (sizeof(wchar_t) == 4) {
                memcpy(data, [nsstr cStringUsingEncoding:NSUTF32StringEncoding], l * sizeof(wchar_t));
            } else {
                memcpy(data, [nsstr cStringUsingEncoding:NSUTF16StringEncoding], l * sizeof(wchar_t));
            }
            // for non-UTF-8 encodings this may not be terminated with a nullptr!
            data[l] = '\0';
            len = l;
        }
    }
#endif

#ifdef QSTRING_H
    /// Constructs from a QString.
    WStr(const QString &qstr) : WStr() {
#if 1
        int l = qstr.length();
        EnsureAlloced(l + 1, false);
        qstr.toWCharArray(data);
        data[l] = '\0';
        len = l;
#else
        static QTextCodec *codec = QTextCodec::codecForName(sizeof(wchar_t) == 4 ? "UTF-32" : "UTF-16");
        static QTextEncoder *encoderWithoutBom = codec->makeEncoder(QTextCodec::IgnoreHeader);

        int l = qstr.length();
        EnsureAlloced(l + 1, false);
        QByteArray bytes = encoderWithoutBom->fromUnicode(qstr);
        wcscpy(data, (const wchar_t *)bytes.constData());
        len = l;
#endif
    }
#endif

#ifdef __ANDROID__
    /// Constructs from a jstring
    WStr(JNIEnv *env, jstring javaString) : WStr() {
        const jchar *raw = env->GetStringChars(javaString, 0);
        jsize l = env->GetStringLength(javaString);
        EnsureAlloced(l + 1, false);
        for (int i = 0; i < l; i++) {
            data[i] = (wchar_t)raw[i];
        }
        data[l] = 0;
        env->ReleaseStringChars(javaString, raw);
    }
#endif

    /// Construct from a bool.
    explicit WStr(const bool b);
    /// Construct from a character.
    explicit WStr(const wchar_t c);
    /// Construct from an integer.
    explicit WStr(const int i);
    /// Construct from an 64 bits integer.
    explicit WStr(const int64_t i);
    /// Construct from an unsigned integer.
    explicit WStr(const unsigned u);
    /// Construct from an unsigned 64 bits integer.
    explicit WStr(const uint64_t u);
    /// Construct from a float.
    explicit WStr(const float f);
    /// Construct from a double.
    explicit WStr(const double d);
    /// Assign a wide string.
    WStr &operator=(const WStr &a);
    /// Move a wide string.
    WStr &operator=(WStr &&a);
    /// Assign a null-terminated wide character C string.
    WStr &operator=(const wchar_t *text);
    /// Destructs.
    ~WStr();

                        /// Return the const wchar_t C string.
    const wchar_t *     c_str() const { return data; }
                        /// Return the const wchar_t C string.
                        operator const wchar_t *() const { return data; }
                        /// Return the wchar_t C string.
                        operator wchar_t *() const { return data; }

                        /// Return const wchar_t at index.
    const wchar_t &     operator[](int index) const;
                        /// Return wchar_t at index.
    wchar_t &           operator[](int index);
                        /// Return char at index.
    wchar_t &           At(int index) { return (*this)[index]; }

                        /// Returns length.
    int                 Length() const { return len; }
                        /// Returns total size of allocated memory.
    size_t              Allocated() const;
                        /// Returns total size of allocated memory including size of this type.
    size_t              Size() const;
                        /// Clear the string.
    void                Clear();
                        /// Return whether the string is empty.
    bool                IsEmpty() const;

                        /// Test for equality of two strings.
    friend bool         operator==(const WStr &lhs, const WStr &rhs) { return !WStr::Cmp(lhs.data, rhs.data); }
                        /// Test for equality of two strings.
    friend bool         operator==(const WStr &lhs, const wchar_t *rhs) { return !WStr::Cmp(lhs.data, rhs); }
                        /// Test for equality of two strings.
    friend bool         operator==(const wchar_t *lhs, const WStr &rhs) { return !WStr::Cmp(lhs, rhs.data); }
                        /// Test for inequality of two strings.
    friend bool         operator!=(const WStr &lhs, const WStr &rhs) { return !!WStr::Cmp(lhs.data, rhs.data); }
                        /// Test for inequality of two strings.
    friend bool         operator!=(const WStr &lhs, const wchar_t *rhs) { return !!WStr::Cmp(lhs.data, rhs); }
                        /// Test for inequality of two strings.
    friend bool         operator!=(const wchar_t *lhs, const WStr &rhs) { return !!WStr::Cmp(lhs, rhs.data); }
                        /// Test if string is less than another string.
    friend bool         operator<(const WStr &lhs, const WStr &rhs) { return WStr::Cmp(lhs.data, rhs.data) < 0; }
                        /// Test if string is less than another string.
    friend bool         operator<(const WStr &lhs, const wchar_t *rhs) { return WStr::Cmp(lhs.data, rhs) < 0; }
                        /// Test if string is less than another string.
    friend bool         operator<(const wchar_t *lhs, const WStr &rhs) { return WStr::Cmp(lhs, rhs.data) < 0; }
                        /// Test if string is greater than another string.
    friend bool         operator>(const WStr &lhs, const WStr &rhs) { return WStr::Cmp(lhs.data, rhs.data) > 0; }
                        /// Test if string is greater than another string.
    friend bool         operator>(const WStr &lhs, const wchar_t *rhs) { return WStr::Cmp(lhs.data, rhs) > 0; }
                        /// Test if string is greater than another string.
    friend bool         operator>(const wchar_t *lhs, const WStr &rhs) { return WStr::Cmp(lhs, rhs.data) > 0; }

                        /// Returns case sensitive comparison result with a string.
    int                 Cmp(const wchar_t *text) const;
                        /// Returns case sensitive comparison result with a string up to num characters.
    int                 Cmpn(const wchar_t *text, int n) const;
                        /// Returns case sensitive natural order comparison result with a string.
    int                 CmpNat(const wchar_t *text) const;
    
                        /// Returns case insensitive comparison result with a string.
    int                 Icmp(const wchar_t *text) const;
                        /// Returns case insensitive comparison result with a string up to num characters.
    int                 Icmpn(const wchar_t *text, int n) const;
                        /// Returns case insensitive path comparison result with a string.
    int                 IcmpPath(const wchar_t *text) const;
                        /// Returns case insensitive path comparison result with a string up to num characters.
    int                 IcmpnPath(const wchar_t *text, int n) const;
                        /// Returns case insensitive natural order comparison result with a string.
    int                 IcmpNat(const wchar_t *text) const;

                        /// String ranking algorithm that produced a number between 0 and 1 representing the similarity between two strings.
    float               FuzzyScore(const wchar_t *text, float fuzziness = 0.0f) const;

                        /// Add-assign a wide string.
    WStr &              operator+=(const WStr &a);
                        /// Add-assign a wide character C string.
    WStr &              operator+=(const wchar_t *a);
                        /// Add-assign (concatenate as string) a float.
    WStr &              operator+=(const float a);
                        /// Add-assign a wide character.
    WStr &              operator+=(const wchar_t a);
                        /// Add-assign (concatenate as string) an integer.
    WStr &              operator+=(const int a);
                        /// Add-assign (concatenate as string) an unsigned integer.
    WStr &              operator+=(const unsigned a);
                        /// Add-assign (concatenate as string) a bool.
    WStr &              operator+=(const bool a);

                        /// Returns a string which is the result of concatenating s1 and s2.
    friend WStr         operator+(const WStr &s1, const WStr &s2);
                        /// Returns a string which is the result of concatenating s1 and s2.
    friend WStr         operator+(const WStr &s1, const wchar_t *s2);
                        /// Returns a string which is the result of concatenating s1 and s2.
    friend WStr         operator+(const wchar_t *s1, const WStr &s2);
                        /// Returns a string which is the result of concatenating the string s and the character ch.
    friend WStr         operator+(const WStr &s, const wchar_t ch);

                        /// Make string in lowercase.
    void                ToLower();
                        /// Make string in uppercase.
    void                ToUpper();
                        /// Is this string contains only numeric characters ?
    bool                IsNumeric();
                        /// Is this string contains only alphabetical characters ?
    bool                IsAlpha();
                        /// Append a character.
    void                Append(const wchar_t ch);
                        /// Append a string.
    void                Append(const WStr &text);
                        /// Append a C string.
    void                Append(const wchar_t *text);
                        /// Append a C string with the given length.
    void                Append(const wchar_t *text, int len);
                        /// Insert a character at the given index.
    void                Insert(const wchar_t a, int index);
                        /// Insert a C string at the given index.
    void                Insert(const wchar_t *text, int index);
                        /// Truncates the string at the given index.
                        /// If the specified position index is beyond the end of the string, nothing happens.
    void                Truncate(int position);
                        /// Removes n characters from the end of the string.
    void                Chop(int n);

                        /// Fills with a characters with the given length
    void                Fill(const wchar_t ch, int newLen);

                        /// Returns index to the first occurrence of a character, or -1 if not found.
    int                 Find(const wchar_t ch, int start = 0, int end = -1) const;
                        /// Returns index to the first occurrence of a string, or -1 if not found.
    int                 Find(const wchar_t *text, bool caseSensitive = true, int start = 0, int end = -1) const;
                        /// Returns index to the last occurrence of a character, or -1 if not found.
    int                 FindLast(const wchar_t ch, int start = 0, int end = -1) const;
                        /// Returns index to the last occurrence of a string, or -1 if not found.
    int                 FindLast(const wchar_t *text, bool caseSensitive = true, int start = 0, int end = -1) const;

                        /// Returns true if the string conforms the given filter.
                        /// Several metacharacter may be used in the filter.
                        ///
                        /// *          match any string of zero or more characters
                        /// ?          match any single characters
                        /// [abc...]   match any of the enclosed characters; a hyphen can
                        ///            be used to specify a range (e.g. a-z, A-Z, 0-9)
    bool                Filter(const wchar_t *filter, bool caseSensitive = true) const;
                        /// Strip whole quoted string to unquoted string
    WStr &              StripQuotes();

                        /// Replaces 'n' characters beginning at index position with the string after and returns a reference to this string.
    WStr &              Replace(int index, int length, const wchar_t *after);
                        /// Replaces first occurrence of the string 'before' with the string 'after' and returns a reference to this string.
    WStr &              Replace(const wchar_t *before, const wchar_t *after, bool caseSensitive = true);
                        /// Replaces every occurrence of the string 'before' with the string 'after' and returns a reference to this string.
    WStr &              ReplaceAll(const wchar_t *before, const wchar_t *after, bool caseSensitive = true);

                        /// Stores a substring that contains the 'n' leftmost characters of the string.
    const wchar_t *     Left(int n, WStr &result) const;
                        /// Stores a substring that contains the n rightmost characters of the string.
    const wchar_t *     Right(int n, WStr &result) const;
                        /// Stores a string that contains 'n' characters of this string, starting at the specified 'position' index.
    const wchar_t *     Mid(int position, int n, WStr &result) const;

                        /// Returns a substring that contains the 'n' leftmost characters of the string.
    WStr                Left(int n) const;
                        /// Returns a substring that contains the n rightmost characters of the string.
    WStr                Right(int n) const;
                        /// Returns a string that contains 'n' characters of this string, starting at the specified 'position' index.
    WStr                Mid(int position, int n) const;

                        /// Returns comma added string. ex) 10000 -> 10,000
    WStr                Commafy() const;

    WStr                NumberedName(int *number) const;

                        /// Set formatted string.
    int	BE_CDECL        sPrintf(const wchar_t *formatString, ...);
    
                        /// hash key for the filename (skips extension)
    int                 FileNameHash() const;
                        /// Converts '\' to '/'
    WStr &              BackSlashesToSlashes();
                        /// Converts '/' to '\'
    WStr &              SlashesToBackSlashes();
                        /// Converts absolute path to relative path.
    const WStr          ToRelativePath(const wchar_t *basePath) const;
                        /// Converts relative path to absolute path.
    const WStr          ToAbsolutePath(const wchar_t *basePath) const;
                        /// Removes any file extension.
    WStr &              StripFileExtension();
                        /// Removes the filename from a path.
    WStr &              StripFileName();
                        /// Removes the path from the filename.
    WStr &              StripPath();
                        /// Sets the given file extension.
    WStr &              SetFileExtension(const wchar_t *extension);
                        /// If there's no file extension use the default
    WStr &              DefaultFileExtension(const wchar_t *extension);
                        /// If there's no path use the default.
    WStr &              DefaultPath(const wchar_t *basepath);
                        /// Appends path string separated by directory separator.
    WStr &              AppendPath(const wchar_t *text, wchar_t pathSeparator = PATHSEPERATOR_WCHAR);
                        /// Removes all multiple directory separators and resolves any "."s or ".."s.
    WStr &              CleanPath(wchar_t pathSeparator = PATHSEPERATOR_CHAR);
                        /// Copy the file path to another string.
    void                ExtractFilePath(WStr &dest) const;
                        /// Copy the filename to another string.
    void                ExtractFileName(WStr &dest) const;
                        /// Copy the filename minus the extension to another string.
    void                ExtractFileBase(WStr &dest) const;
                        /// Copy the file extension to another string.
    void                ExtractFileExtension(WStr &dest) const;
                        /// Test if extension matched with.
    bool                CheckExtension(const wchar_t *extension) const;

    static int          Length(const wchar_t *s);
    static int          LineCount(const wchar_t *s);
    static int32_t      ToI32(const wchar_t *s);
    static uint32_t     ToUI32(const wchar_t *s);
    static int64_t      ToI64(const wchar_t *s);
    static uint64_t     ToUI64(const wchar_t *s);
    static WStr         FormatBytes(int bytes);
    
    static int          Cmp(const wchar_t *s1, const wchar_t *s2);
    static int          Cmpn(const wchar_t *s1, const wchar_t *s2, int n);
    static int          Icmp(const wchar_t *s1, const wchar_t *s2);
    static int          Icmpn(const wchar_t *s1, const wchar_t *s2, int n);
    static int          IcmpPath(const wchar_t *s1, const wchar_t *s2);
    static int          IcmpnPath(const wchar_t *s1, const wchar_t *s2, int n);
    static int          CmpNat(const wchar_t *s1, const wchar_t *s2);
    static int          IcmpNat(const wchar_t *s1, const wchar_t *s2);

    static float        FuzzyScore(const wchar_t *s1, const wchar_t *s2, float fuzziness = 0.0f);

    static void         Append(wchar_t *dest, int size, const wchar_t *src);
    static void         Copynz(wchar_t *dest, const wchar_t *src, int destsize);
    static int          FindChar(const wchar_t *str, const wchar_t c, int start = 0, int end = -1);
    static int          FindLastChar(const wchar_t *str, const wchar_t c, int start = 0, int end = -1);
    static int          FindText(const wchar_t *str, const wchar_t *text, bool caseSensitive = true, int start = 0, int end = -1);
    static int          FindLastText(const wchar_t *str, const wchar_t *text, bool caseSensitive = true, int start = 0, int end = -1);
    static bool         Filter(const wchar_t *filter, const wchar_t *name, bool caseSensitive = true);
    static bool         CheckExtension(const wchar_t *name, const wchar_t *ext);
    static void         ConvertPathSeperator(wchar_t *path, wchar_t separator);

    static const wchar_t *IntegerArrayToString(const int *arr, const int length);
    static const wchar_t *FloatArrayToString(const float *arr, const int length, const int precision);

    static int BE_CDECL snPrintf(wchar_t *dest, int size, const wchar_t *fmt, ...);
    static int BE_CDECL vsnPrintf(wchar_t *dest, int size, const wchar_t *fmt, va_list argptr);
    
    friend int BE_CDECL swprintf(WStr &dest, const wchar_t *fmt, ...);
    friend int BE_CDECL vswprintf(WStr &dest, const wchar_t *fmt, va_list ap);

    static int          Hash(const wchar_t *string);
    static int          Hash(const wchar_t *string, int length);
    static int          IHash(const wchar_t *string);
    static int          IHash(const wchar_t *string, int length);

    static wchar_t      ToLower(int c);
    static wchar_t      ToUpper(int c);
    static bool         CharIsNumeric(int c);
    static bool         CharIsAlpha(int c);
    static bool         CharIsLower(int c);
    static bool         CharIsUpper(int c);
    static bool         CharIsNewLine(int c);
    static bool         CharIsTab(int c);
    static int          ColorIndex(int c);

                        /// Convert wide-character string to Str
    static Str          ToStr(const wchar_t *wcs);
#if __OBJC__
                        /// Convert WStr to Objective-C NSString
    NSString *          ToNSString() const {
        if (sizeof(wchar_t) == 4) {
            return [[NSString alloc] initWithBytes:data length:Length() * sizeof(wchar_t) encoding:NSUTF32LittleEndianStringEncoding];
        } else {
            return [[NSString alloc] initWithBytes:data length:Length() * sizeof(wchar_t) encoding:NSUTF16LittleEndianStringEncoding];
        }
    }
#endif

#ifdef QSTRING_H
                        /// Convert WStr to QString
    QString             ToQString() const {
        return QString::fromWCharArray(data, len);
    }
#endif

#ifdef __ANDROID__
                        /// Convert WStr to jstring
    jstring             ToJavaString(JNIEnv *env) const {
        jstring javaString;
        if (sizeof(wchar_t) == sizeof(jchar)) {
            javaString = env->NewString((jchar *)data, (jsize)len);
        } else {
            jchar *raw = (jchar *)malloc((len + 1) * sizeof(jchar));
            for (int i = 0; i < len; i++) {
                // This discards two bytes in data[i], but these should be 0 in UTF-16
                raw[i] = (jchar)data[i];
            }
            raw[len] = 0;
            javaString = env->NewString(raw, (jsize)len);
            free(raw);
        }
        return javaString;
    }
#endif

    void                ReAllocate(int amount, bool keepOld);
    void                FreeData();

    static WStr         empty;

private:
                        /// Ensures string data buffer is large enough.
    void                EnsureAlloced(int amount, bool keepOld = true);

    static constexpr int BaseLength = 20;
    static constexpr int AllocGranularity = 32;
    static constexpr int FileNameHashSize = 1024;

    wchar_t *           data;                       ///< String Data pointer
    size_t              alloced;                    ///< Allocated data size
    int                 len;                        ///< String length
    wchar_t             baseBuffer[BaseLength];     ///< Default base buffer
};

wchar_t * BE_CDECL      wva(const wchar_t *format, ...);
BE_INLINE const char *  tombs(const char *mbs) { return mbs; }
char *                  tombs(const wchar_t *wcs);
wchar_t *               towcs(const char *mbs);
BE_INLINE const wchar_t *towcs(const wchar_t *wcs) { return wcs; }

BE_INLINE const TCHAR * totcs(const char *mbs) {
#ifdef  UNICODE
    return towcs(mbs);
#else
    return mbs;
#endif
}

BE_INLINE const TCHAR * totcs(const wchar_t *wcs) {
#ifdef  UNICODE
    return wcs;
#else
    return tombs(wcs);
#endif
}

BE_INLINE void WStr::EnsureAlloced(int amount, bool keepOld) {
    if (amount > alloced) {
        ReAllocate(amount, keepOld);
    }
}

BE_INLINE WStr::WStr() {
    len = 0;
    alloced = BaseLength;
    data = baseBuffer;
    data[0] = L'\0';
}

BE_INLINE WStr::WStr(const bool b) : WStr() {
    EnsureAlloced(2, false);
    data[0] = b ? L'1' : L'0';
    data[1] = L'\0';
    len = 1;
}

BE_INLINE WStr::WStr(const wchar_t c) : WStr() {
    EnsureAlloced(2, false);
    data[0] = c;
    data[1] = L'\0';
    len = 1;
}

BE_INLINE WStr::WStr(const WStr &wstr) : WStr() {
    int l = wstr.Length();
    EnsureAlloced(l + 1, false);
    wcscpy(data, wstr.data);
    len = l;
}

BE_INLINE WStr::WStr(WStr &&wstr) : WStr() {
    // just call move assignment operator
    *this = std::move(wstr);
}

BE_INLINE WStr::WStr(const wchar_t *text) : WStr() {
    if (text) {
        int l = (int)wcslen(text);
        EnsureAlloced(l + 1, false);
        wcscpy(data, text);
        len = l;
    }
}

BE_INLINE WStr::WStr(const char *text) : WStr() {
    if (text) {
        int l = (int)mbstowcs(nullptr, text, 0);
        EnsureAlloced(l + 1, false);
        mbstowcs(data, text, l + 1);
        len = l;
    }
}

BE_INLINE WStr::WStr(const int i) : WStr() {
    wchar_t text[64];
    int l = WStr::snPrintf(text, COUNT_OF(text), L"%i", i);
    EnsureAlloced(l + 1, false);
    wcscpy(data, text);
    len = l;
}

BE_INLINE WStr::WStr(const int64_t i) : WStr() {
    wchar_t text[64];
    int l = WStr::snPrintf(text, COUNT_OF(text), L"%" PRIi64, i);
    EnsureAlloced(l + 1, false);
    wcscpy(data, text);
    len = l;
}

BE_INLINE WStr::WStr(const unsigned u) : WStr() {
    wchar_t text[64];
    int l = WStr::snPrintf(text, COUNT_OF(text), L"%u", u);
    EnsureAlloced(l + 1, false);
    wcscpy(data, text);
    len = l;
}

BE_INLINE WStr::WStr(const uint64_t u) : WStr() {
    wchar_t text[64];
    int l = WStr::snPrintf(text, COUNT_OF(text), L"%" PRIu64, u);
    EnsureAlloced(l + 1, false);
    wcscpy(data, text);
    len = l;
}

BE_INLINE WStr::WStr(const float f) : WStr() {
    wchar_t text[64];
    int l = WStr::snPrintf(text, COUNT_OF(text), L"%f", f);
    EnsureAlloced(l + 1, false);
    wcscpy(data, text);
    len = l;
}

BE_INLINE WStr::WStr(const double d) : WStr() {
    wchar_t text[64];
    int l = WStr::snPrintf(text, COUNT_OF(text), L"%lf", d);
    EnsureAlloced(l + 1, false);
    wcscpy(data, text);
    len = l;
}

BE_INLINE WStr &WStr::operator=(const WStr &a) {
    int l = a.Length();
    EnsureAlloced(l + 1, false);
    wcsncpy(data, a.data, l);
    data[l] = L'\0';
    len = l;
    return *this;
}

BE_INLINE WStr &WStr::operator=(WStr &&a) {
    BE1::Swap(len, a.len);
    BE1::Swap(alloced, a.alloced);
    BE1::Swap(data, a.data);
    if (data == a.baseBuffer) {
        wcscpy(baseBuffer, a.baseBuffer);
        data = baseBuffer;
    }
    if (a.data == baseBuffer) {
        a.data = a.baseBuffer;
    }
    return *this;
}

BE_INLINE WStr::~WStr() {
    FreeData();
}

BE_INLINE size_t WStr::Size() const {
    return sizeof(*this) + Allocated();
}

BE_INLINE const wchar_t &WStr::operator[](int index) const {
    assert((index >= 0) && (index <= len));
    return data[index];
}

BE_INLINE wchar_t &WStr::operator[](int index) {
    assert((index >= 0) && (index <= len));
    return data[index];
}

BE_INLINE WStr &WStr::operator+=(const WStr &a) {
    Append(a);
    return *this;
}

BE_INLINE WStr &WStr::operator+=(const wchar_t *a) {
    Append(a);
    return *this;
}

BE_INLINE WStr &WStr::operator+=(const float a) {
    wchar_t text[64];

    WStr::snPrintf(text, COUNT_OF(text), L"%f", a);
    Append(text);
    return *this;
}

BE_INLINE WStr &WStr::operator+=(const wchar_t a) {
    Append(a);
    return *this;
}

BE_INLINE WStr &WStr::operator+=(const int a) {
    wchar_t text[64];

    WStr::snPrintf(text, COUNT_OF(text), L"%i", a);
    Append(text);
    return *this;
}

BE_INLINE WStr &WStr::operator+=(const unsigned a) {
    wchar_t text[64];

    WStr::snPrintf(text, COUNT_OF(text), L"%u", a);
    Append(text);
    return *this;
}

BE_INLINE WStr &WStr::operator+=(const bool a) {
    Append(a ? L"true" : L"false");
    return *this;
}

BE_INLINE WStr operator+(const WStr &s1, const WStr &s2) {
    WStr result(s1);

    result.Append(s2);
    return result;
}

BE_INLINE WStr operator+(const WStr &s1, const wchar_t *s2) {
    WStr result(s1);

    result.Append(s2);
    return result;
}

BE_INLINE WStr operator+(const wchar_t *s1, const WStr &s2) {
    WStr result(s1);

    result.Append(s2);
    return result;
}

BE_INLINE WStr operator+(const WStr &s, const wchar_t ch) {
    WStr result(s);

    result.Append(ch);
    return result;
}

BE_INLINE int WStr::Cmp(const wchar_t *text) const {
    assert(text);
    return WStr::Cmp(data, text);
}

BE_INLINE int WStr::Cmpn(const wchar_t *text, int n) const {
    assert(text);
    return WStr::Cmpn(data, text, n);
}

BE_INLINE int WStr::CmpNat(const wchar_t *text) const {
    assert(text);
    return WStr::CmpNat(data, text);
}

BE_INLINE int WStr::Icmp(const wchar_t *text) const {
    assert(text);
    return WStr::Icmp(data, text);
}

BE_INLINE int WStr::Icmpn(const wchar_t *text, int n) const {
    assert(text);
    return WStr::Icmpn(data, text, n);
}

BE_INLINE int WStr::IcmpPath(const wchar_t *text) const {
    assert(text);
    return WStr::IcmpPath(data, text);
}

BE_INLINE int WStr::IcmpnPath(const wchar_t *text, int n) const {
    assert(text);
    return WStr::IcmpnPath(data, text, n);
}

BE_INLINE int WStr::IcmpNat(const wchar_t *text) const {
    assert(text);
    return WStr::IcmpNat(data, text);
}

BE_INLINE float WStr::FuzzyScore(const wchar_t *text, float fuzziness) const {
    assert(text);
    return WStr::FuzzyScore(data, text, fuzziness);
}

BE_INLINE size_t WStr::Allocated() const {
    if (data != baseBuffer) {
        return alloced;
    }
    return 0;
}

BE_INLINE void WStr::Clear() {
    EnsureAlloced(1);
    data[0] = L'\0';
    len = 0;
}

BE_INLINE bool WStr::IsEmpty() const {
    return (WStr::Cmp(data, L"") == 0);
}

BE_INLINE void WStr::Append(const wchar_t a) {
    EnsureAlloced(len + 2);
    data[len] = a;
    len++;
    data[len] = L'\0';
}

BE_INLINE void WStr::Append(const WStr &text) {
    int newLen;
    int i;

    newLen = len + text.Length();
    EnsureAlloced(newLen + 1);
    for (i = 0; i < text.len; i++)
        data[len + i] = text[i];

    len = newLen;
    data[len] = L'\0';
}

BE_INLINE void WStr::Append(const wchar_t *text) {
    int newLen;
    int i;

    if (text) {
        newLen = len + (int)wcslen(text);
        EnsureAlloced(newLen + 1);
        for (i = 0; text[i]; i++)
            data[len + i] = text[i];
    
        len = newLen;
        data[len] = L'\0';
    }
}

BE_INLINE void WStr::Append(const wchar_t *text, int l) {
    int newLen;
    int i;

    if (text && l) {
        newLen = len + l;
        EnsureAlloced(newLen + 1);
        for (i = 0; text[i] && i < l; i++)
            data[len + i] = text[i];
        
        len = newLen;
        data[len] = L'\0';
    }
}

BE_INLINE void WStr::Insert(const wchar_t a, int index) {
    Clamp(index, 0, len);
    
    int l = 1;
    EnsureAlloced(len + l + 1);
    for (int i = len; i >= index; i--)
        data[i+l] = data[i];

    data[index] = a;
    len++;
}

BE_INLINE void WStr::Insert(const wchar_t *text, int index) {
    Clamp(index, 0, len);

    int l = (int)wcslen(text);
    EnsureAlloced(len + l + 1);
    for (int i = len; i >= index; i--)
        data[i+l] = data[i];

    for (int i = 0; i < l; i++)
        data[index+i] = text[i];

    len += l;
}

BE_INLINE void WStr::Truncate(int position) {
    if (position >= len) {
        return;
    }

    data[position] = 0;
    len = position;
}

BE_INLINE void WStr::Chop(int n) {
    if (n >= len) {
        Clear();
        return;
    }

    data[len - n] = 0;
    len = n;
}

BE_INLINE void WStr::Fill(const wchar_t ch, int newLen) {
    EnsureAlloced(newLen + 1);
    len = newLen;
    wmemset(data, ch, len);
    data[len] = 0;
}

BE_INLINE int WStr::Find(const wchar_t c, int start, int end) const {
    if (end == -1) {
        end = len;
    }

    return WStr::FindChar(data, c, start, end);
}

BE_INLINE int WStr::Find(const wchar_t *text, bool caseSensitive, int start, int end) const {
    if (end == -1) {
        end = len;
    }

    return WStr::FindText(data, text, caseSensitive, start, end);
}

BE_INLINE int WStr::FindLast(const wchar_t ch, int start, int end) const {
    if (end == -1) {
        end = len;
    }

    return WStr::FindLastChar(data, ch, start, end);
}

BE_INLINE int WStr::FindLast(const wchar_t *text, bool caseSensitive, int start, int end) const {
    if (end == -1) {
        end = len;
    }

    return WStr::FindLastText(data, text, caseSensitive, start, end);
}

BE_INLINE bool WStr::Filter(const wchar_t *filter, bool caseSensitive) const {
    return WStr::Filter(filter, data, caseSensitive);
}

BE_INLINE const wchar_t *WStr::Left(int len, WStr &result) const {
    return Mid(0, len, result);
}

BE_INLINE const wchar_t *WStr::Right(int len, WStr &result) const {
    if (len >= Length()) {
        result = *this;
        return result;
    }
    return Mid(Length() - len, len, result);
}

BE_INLINE WStr WStr::Left(int len) const {
    return Mid(0, len);
}

BE_INLINE WStr WStr::Right(int len) const {
    if (len >= Length()) {
        return *this;
    }
    return Mid(Length() - len, len);
}

BE_INLINE WStr &WStr::StripQuotes() {
    if (data[0] != L'\"') {
        return *this;
    }
    
    // Remove the trailing quote first
    if (data[len - 1] == L'\"') {
        data[len - 1] = L'\0';
        len--;
    }

    // Strip the leading quote now
    len--;
    memmove(&data[0], &data[1], len * sizeof(wchar_t));
    data[len] = L'\0';
    
    return *this;
}

BE_INLINE WStr &WStr::Replace(int index, int length, const wchar_t *after) {
    if (index + length > len) {
        return *this;
    }

    *this = Left(index) + WStr(after) + Right(len - (index + length));
    return *this;
}

BE_INLINE WStr &WStr::Replace(const wchar_t *before, const wchar_t *after, bool caseSensitive) {
    int s = Find(before, caseSensitive);
    if (s == -1) {
        return *this;
    }

    *this = Left(s) + WStr(after) + Right(len - (s + Length(before)));
    return *this;
}

BE_INLINE WStr &WStr::ReplaceAll(const wchar_t *before, const wchar_t *after, bool caseSensitive) {
    int afterLen = Length(after);
    int findStart = 0;

    int s = Find(before, caseSensitive, findStart);
    while (s >= 0) {
        *this = Left(s) + WStr(after) + Right(len - (s + Length(before)));

        findStart = s + afterLen;

        s = Find(before, caseSensitive, findStart);
    }

    return *this;
}

//--------------------------------------------------------------------------------------------------
//
// wchar_t * methods to replace library functions
//
//--------------------------------------------------------------------------------------------------

BE_INLINE int WStr::Length(const wchar_t *s) {
    int i;
    for (i = 0; s[i]; i++) {}
    return i;
}

BE_INLINE int WStr::LineCount(const wchar_t *s) {
    int count = 1;
    for (int i = 0; s[i]; i++) {
        if (s[i] == L'\n') {
            count++;
        }
    }
    return count;
}

BE_INLINE int32_t WStr::ToI32(const wchar_t *s) {
    return (int32_t)wcstol(s, nullptr, 10);
}

BE_INLINE uint32_t WStr::ToUI32(const wchar_t *s) {
    return (uint32_t)wcstoul(s, nullptr, 10);
}

BE_INLINE int64_t WStr::ToI64(const wchar_t *s) {
#if !defined(__ANDROID__) || __ANDROID_API__ >= 21
    return wcstoll(s, nullptr, 10);
#else
    int64_t i;
    swscanf(s, L"%" SCNi64, &i);
    return i;
#endif
}

BE_INLINE uint64_t WStr::ToUI64(const wchar_t *s) {
#if !defined(__ANDROID__) || __ANDROID_API__ >= 21
    return wcstoull(s, nullptr, 10);
#else
    uint64_t u;
    swscanf(s, L"%" SCNu64, &u);
    return u;
#endif
}

BE_INLINE bool WStr::CheckExtension(const wchar_t *ext) const {
    return WStr::CheckExtension(data, ext);
}

BE_INLINE int WStr::Hash(const wchar_t *string) {
    int hash = 0;
    for (int i = 0; *string != L'\0'; i++) {
        hash += (*string++) * (i + 119);
    }
    return hash;
}

BE_INLINE int WStr::Hash(const wchar_t *string, int length) {
    int hash = 0;
    for (int i = 0; i < length; i++) {
        hash += (*string++) * (i + 119);
    }
    return hash;
}

BE_INLINE int WStr::IHash(const wchar_t *string) {
    int hash = 0;
    for (int i = 0; *string != L'\0'; i++) {
        hash += ToLower(*string++) * (i + 119);
    }
    return hash;
}

BE_INLINE int WStr::IHash(const wchar_t *string, int length) {
    int hash = 0;
    for (int i = 0; i < length; i++) {
        hash += ToLower(*string++) * (i + 119);
    }
    return hash;
}

//--------------------------------------------------------------------------------------------------
//
// character methods
//
//--------------------------------------------------------------------------------------------------

BE_INLINE wchar_t WStr::ToLower(int c) {
    if (c >= L'A' && c <= L'Z') {
        return (c - L'A' + L'a');
    }
    return c;
}

BE_INLINE wchar_t WStr::ToUpper(int c) {
    if (c >= L'a' && c <= L'z') {
        return (c - L'a' + L'A');
    }
    return c;
}

BE_INLINE bool WStr::CharIsNumeric(int c) {
    return (c <= L'9' && c >= L'0');
}

BE_INLINE bool WStr::CharIsAlpha(int c) {
    return ((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z'));
}

BE_INLINE bool WStr::CharIsLower(int c) {
    return (c >= L'a' && c <= L'z');
}

BE_INLINE bool WStr::CharIsUpper(int c) {
    return (c >= L'A' && c <= L'Z');
}

BE_INLINE bool WStr::CharIsNewLine(int c) {
    return (c == L'\n' || c == L'\r');
}

BE_INLINE bool WStr::CharIsTab(int c) {
    return (c == L'\t');
}

BE_INLINE int WStr::ColorIndex(int c) {
    return (c & 15);
}

BE_NAMESPACE_END
