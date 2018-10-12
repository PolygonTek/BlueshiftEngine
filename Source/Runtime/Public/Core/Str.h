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

    String

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class WStr;

/// String class.
class BE_API Str {
public:
    enum UTF8Encoding {
        UTF8_PURE_ASCII,        ///< no characters with values > 127
        UTF8_ENCODED_BOM,       ///< characters > 128 encoded with UTF8, with a byte-order-marker at the beginning
        UTF8_ENCODED_NO_BOM,    ///< characters > 128 encoded with UTF8, but no byte-order-marker at the beginning
        UTF8_INVALID,           ///< has values > 127 but isn't valid UTF8 
        UTF8_INVALID_BOM        ///< has a byte-order-marker at the beginning, but isn't valuid UTF8 -- it's messed up
    };    

    /// Constructs empty string.
    Str();
    /// Constructs from another string.
    Str(const Str &str);
    /// Constructs from rvalue string.
    Str(Str &&str) noexcept;
    /// Constructs from a null-terminated C string.
    Str(const char *text);
    /// Constructs from a null-terminated wide character string.
    Str(const wchar_t *text);

#if __OBJC__
    /// Constructs from a Objective-C NSString pointer.
    Str(const NSString *nsstr) : Str() {
        // get the length of the NSString
        int l = nsstr ? (int)[nsstr length] : 0;
        EnsureAlloced(l + 1, false);
        
        if (l > 0) {
            memcpy(data, [nsstr cStringUsingEncoding:NSUTF8StringEncoding], l);
            data[l] = '\0';
            len = l;
        }
    }
#endif

#ifdef QSTRING_H
    /// Constructs from a QString.
    Str(const QString &qstr) : Str() {
        const QByteArray bytes = qstr.toLatin1(); // qstr.toUtf8();
        int l = bytes.length();
        EnsureAlloced(l + 1, false);
        strcpy(data, (const char *)bytes.constData());
        len = l;
    }
#endif

#ifdef __ANDROID__
    /// Constructs from a jstring
    Str(JNIEnv *env, jstring javaString) : Str() {
        const char *raw = env->GetStringUTFChars(javaString, 0);
        jsize l = env->GetStringLength(javaString);
        EnsureAlloced(l + 1, false);
        strcpy(data, raw);
        env->ReleaseStringUTFChars(javaString, raw);
    }
#endif

    /// Constructs from a bool.
    explicit Str(const bool b);
    /// Constructs from a character.
    explicit Str(const char c);
    /// Constructs from an integer.
    explicit Str(const int i);
    /// Constructs from an 64 bits integer.
    explicit Str(const int64_t i);
    /// Constructs from an unsigned integer.
    explicit Str(const unsigned u);
    /// Constructs from an unsigned 64 bits integer.
    explicit Str(const uint64_t u);
    /// Constructs from a float.
    explicit Str(const float f);
    /// Constructs from a double.
    explicit Str(const double d);
    /// Assign a string.
    Str &operator=(const Str &rhs);
    /// Move a string.
    Str &operator=(Str &&rhs) noexcept;
    /// Assign a null-terminated C string.
    Str &operator=(const char *rhs);
    /// Destructs.
    ~Str();

                        /// Return the const C string.
    const char *        c_str() const { return data; }
                        /// Return the const C string.
                        operator const char *() const { return data; }
                        /// Return the C string.
                        operator char *() const { return data; }

                        /// Return const char at index.
    const char &        operator[](int index) const;
                        /// Return char at index.
    char &              operator[](int index);
                        /// Return char at index.
    char &              At(int index) { return (*this)[index]; }

                        /// Returns length
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
    friend bool         operator==(const Str &lhs, const Str &rhs) { return !Str::Cmp(lhs.data, rhs.data); }
                        /// Test for equality of two strings.
    friend bool         operator==(const Str &lhs, const char *rhs) { return !Str::Cmp(lhs.data, rhs); }
                        /// Test for equality of two strings.
    friend bool         operator==(const char *lhs, const Str &rhs) { return !Str::Cmp(lhs, rhs.data); }
                        /// Test for inequality of two strings.
    friend bool         operator!=(const Str &lhs, const Str &rhs) { return !!Str::Cmp(lhs.data, rhs.data); }
                        /// Test for inequality of two strings.
    friend bool         operator!=(const Str &lhs, const char *rhs) { return !!Str::Cmp(lhs.data, rhs); }
                        /// Test for inequality of two strings.
    friend bool         operator!=(const char *lhs, const Str &rhs) { return !!Str::Cmp(lhs, rhs.data); }
                        /// Test if string is less than another string.
    friend bool         operator<(const Str &lhs, const Str &rhs) { return Str::Cmp(lhs.data, rhs.data) < 0; }
                        /// Test if string is less than another string.
    friend bool         operator<(const Str &lhs, const char *rhs) { return Str::Cmp(lhs.data, rhs) < 0; }
                        /// Test if string is less than another string.
    friend bool         operator<(const char *lhs, const Str &rhs) { return Str::Cmp(lhs, rhs.data) < 0; }
                        /// Test if string is greater than another string.
    friend bool         operator>(const Str &lhs, const Str &rhs) { return Str::Cmp(lhs.data, rhs.data) > 0; }
                        /// Test if string is greater than another string.
    friend bool         operator>(const Str &lhs, const char *rhs) { return Str::Cmp(lhs.data, rhs) > 0; }
                        /// Test if string is greater than another string.
    friend bool         operator>(const char *lhs, const Str &rhs) { return Str::Cmp(lhs, rhs.data) > 0; }

                        /// Returns case sensitive comparison result with a string.
    int                 Cmp(const char *text) const;
                        /// Returns case sensitive comparison result with a string up to num characters.
    int                 Cmpn(const char *text, int n) const;
                        /// Returns case sensitive natural order comparison result with a string.
    int                 CmpNat(const char *text) const;

                        /// Returns case insensitive comparison result with a string.
    int                 Icmp(const char *text) const;
                        /// Returns case insensitive comparison result with a string up to num characters.
    int                 Icmpn(const char *text, int n) const;
                        /// Returns case insensitive path comparison result with a string.
    int                 IcmpPath(const char *text) const;
                        /// Returns case insensitive path comparison result with a string up to num characters.
    int                 IcmpnPath(const char *text, int n) const;
                        /// Returns case insensitive natural order comparison result with a string.
    int                 IcmpNat(const char *text) const;

                        /// string ranking algorithm that produced a number between 0 and 1 representing the similarity between two strings.
    float               FuzzyScore(const char *text, float fuzziness = 0.0f) const;

                        /// Add-assign a string.
    Str &               operator+=(const Str &rhs);
                        /// Add-assign a C string.
    Str &               operator+=(const char *rhs);
                        /// Add-assign (concatenate as string) a float.
    Str &               operator+=(const float rhs);
                        /// Add-assign a character.
    Str &               operator+=(const char rhs);
                        /// Add-assign (concatenate as string) an integer.
    Str &               operator+=(const int rhs);
                        /// Add-assign (concatenate as string) an unsigned integer.
    Str &               operator+=(const unsigned rhs);
                        /// Add-assign (concatenate as string) a bool.
    Str &               operator+=(const bool rhs);
    
                        /// Returns a string which is the result of concatenating lhs and rhs.
    friend Str          operator+(const Str &lhs, const Str &rhs);
                        /// Returns a string which is the result of concatenating lhs and rhs.
    friend Str          operator+(const Str &lhs, const char *rhs);
                        /// Returns a string which is the result of concatenating lhs and rhs.
    friend Str          operator+(const char *lhs, const Str &rhs);
                        /// Returns a string which is the result of concatenating the string s and the character ch.
    friend Str          operator+(const Str &lhs, const char rhs);

                        /// Make string in lowercase.
    void                ToLower();
                        /// Make string in uppercase.
    void                ToUpper();
                        /// Is this string contains only numeric characters ?
    bool                IsNumeric();
                        /// Is this string contains only alphabetical characters ?
    bool                IsAlpha();
                        /// Append a character.
    void                Append(const char ch);
                        /// Append a string.
    void                Append(const Str &text);
                        /// Append a C string.
    void                Append(const char *text);
                        /// Append a C string with the given length.
    void                Append(const char *text, int len);
                        /// Insert a character at the given index.
    void                Insert(const char a, int index);
                        /// Insert a C string at the given index.
    void                Insert(const char *text, int index);
                        /// Truncates the string at the given index.
                        /// If the specified position index is beyond the end of the string, nothing happens.
    void                Truncate(int position);
                        /// Removes n characters from the end of the string.
    void                Chop(int n);

                        /// Fills with a characters with the given length
    void                Fill(const char ch, int newLen);

                        /// Calculate number of characters in UTF8 content.
    int                 UTF8Length() { return UTF8Length((byte *)data); }
    uint32_t            UTF8Char(int &idx) { return UTF8Char((byte *)data, idx); }
    void                AppendUTF8Char(uint32_t c);
    void                ConvertToUTF8();

                        /// Returns index to the first occurrence of a character, or -1 if not found.
    int                 Find(const char ch, int start = 0, int end = -1) const;
                        /// Returns index to the first occurrence of a string, or -1 if not found.
    int                 Find(const char *text, bool caseSensitive = true, int start = 0, int end = -1) const;
                        /// Returns index to the last occurrence of a character, or -1 if not found.
    int                 FindLast(const char c, int start = 0, int end = -1) const;
                        /// Returns index to the last occurrence of a string, or -1 if not found.
    int                 FindLast(const char *text, bool caseSensitive = true, int start = 0, int end = -1) const;

                        /// Returns true if the string conforms the given filter.
                        /// Several metacharacter may be used in the filter.
                        ///
                        /// *          match any string of zero or more characters
                        /// ?          match any single character
                        /// [abc...]   match any of the enclosed characters; a hyphen can
                        ///            be used to specify a range (e.g. a-z, A-Z, 0-9)
    bool                Filter(const char *filter, bool caseSensitive = true) const;
                        /// Strip whole quoted string to unquoted string
    Str &               StripQuotes();

                        /// Replaces 'n' characters beginning at index position with the string after and returns a reference to this string.
    Str &               Replace(int index, int n, const char *after);
                        /// Replaces first occurrence of the string 'before' with the string 'after' and returns a reference to this string.
    Str &               Replace(const char *before, const char *after, bool caseSensitive = true);
                        /// Replaces every occurrence of the string 'before' with the string 'after' and returns a reference to this string.
    Str &               ReplaceAll(const char *before, const char *after, bool caseSensitive = true);

                        /// Stores a substring that contains the 'n' leftmost characters of the string.
    const char *        Left(int n, Str &result) const;
                        /// Stores a substring that contains the n rightmost characters of the string.
    const char *        Right(int n, Str &result) const;
                        /// Stores a string that contains 'n' characters of this string, starting at the specified 'position' index.
    const char *        Mid(int position, int n, Str &result) const;

                        /// Returns a substring that contains the 'n' leftmost characters of the string.
    Str                 Left(int n) const;
                        /// Returns a substring that contains the n rightmost characters of the string.
    Str                 Right(int n) const;
                        /// Returns a string that contains 'n' characters of this string, starting at the specified 'position' index.
    Str                 Mid(int start, int n) const;

                        /// Returns comma added string. ex) 10000 -> 10,000
    Str                 Commafy() const;

    Str                 NumberedName(int *number) const;
    
                        /// Set formatted string.
    int BE_CDECL        sPrintf(const char *formatString, ...);
    
                        /// Hash key for the filename (skips extension)
    int                 FileNameHash() const;
                        /// Converts '\' to '/'
    Str &               BackSlashesToSlashes();
                        /// Converts '/' to '\'
    Str &               SlashesToBackSlashes();
                        /// Converts absolute path to relative path.
    const Str           ToRelativePath(const char *basePath) const;
                        /// Converts relative path to absolute path.
    const Str           ToAbsolutePath(const char *basePath) const;
                        /// Removes any file extension.
    Str &               StripFileExtension();
                        /// Removes the filename from a path.
    Str &               StripFileName();
                        /// Removes the path from the filename.
    Str &               StripPath();
                        /// Sets the given file extension.
    Str &               SetFileExtension(const char *extension);
                        /// If there's no file extension use the default.
    Str &               DefaultFileExtension(const char *extension);
                        /// If there's no path use the default.
    Str &               DefaultPath(const char *basepath);
                        /// Appends path string separated by directory separator.
    Str &               AppendPath(const char *path, char pathSeparator = PATHSEPERATOR_CHAR);
                        /// Removes all multiple directory separators and resolves any "."s or ".."s.
    Str &               CleanPath(char pathSeparator = PATHSEPERATOR_CHAR);
                        /// Copy the file path to another string.
    void                ExtractFilePath(Str &dest) const;
                        /// Copy the filename to another string.
    void                ExtractFileName(Str &dest) const;
                        /// Copy the filename minus the extension to another string.
    void                ExtractFileBase(Str &dest) const;
                        /// Copy the file extension to another string.
    void                ExtractFileExtension(Str &dest) const;
                        /// Test if extension matched with.
    bool                CheckExtension(const char *extension) const;

    static int          Length(const char *s);
    static int          LineCount(const char *s);
    static char *       ToLower(char *s);
    static char *       ToUpper(char *s);
    static bool         IsNumeric(const char *s);
    static bool         IsAlpha(const char *s);
    static int32_t      ToI32(const char *s);
    static uint32_t     ToUI32(const char *s);
    static int64_t      ToI64(const char *s);
    static uint64_t     ToUI64(const char *s);
    static Str          FormatBytes(int bytes);

    static int          Cmp(const char *s1, const char *s2);
    static int          Cmpn(const char *s1, const char *s2, int n);
    static int          Icmp(const char *s1, const char *s2);
    static int          Icmpn(const char *s1, const char *s2, int n);
    static int          IcmpPath(const char *s1, const char *s2);
    static int          IcmpnPath(const char *s1, const char *s2, int n);
    static int          CmpNat(const char *s1, const char *s2);
    static int          IcmpNat(const char *s1, const char *s2);

    static float        FuzzyScore(const char *s1, const char *s2, float fuzziness = 0.0f);

    static void         Append(char *dest, int size, const char *src);
    static void         Copynz(char *dest, const char *src, int destsize);
    static int          FindChar(const char *str, const char c, int start = 0, int end = -1);
    static int          FindLastChar(const char *str, const char c, int start = 0, int end = -1);
    static int          FindText(const char *str, const char *text, bool caseSensitive = true, int start = 0, int end = -1);
    static int          FindLastText(const char *str, const char *text, bool caseSensitive = true, int start = 0, int end = -1);
    static bool         Filter(const char *filter, const char *name, bool caseSensitive = true);
    static bool         CheckExtension(const char *name, const char *ext);
    static void         ConvertPathSeperator(char *path, char separator);

    static const char * IntegerArrayToString(const int *array, const int length);
    static const char * FloatArrayToString(const float *array, const int length, const int precision);

    static int BE_CDECL snPrintf(char *dest, int size, const char *fmt, ...);
    static int BE_CDECL vsnPrintf(char *dest, int size, const char *fmt, va_list argptr);
    
    friend int BE_CDECL sprintf(Str &dest, const char *fmt, ...);
    friend int BE_CDECL vsprintf(Str &dest, const char *fmt, va_list ap);

    static int          UTF8Length(const byte *s);
    static uint32_t     UTF8Char(const char *s, int &idx) { return UTF8Char((byte *)s, idx); }
    static uint32_t     UTF8Char(const byte *s, int &idx);

    static bool         IsValidUTF8(const uint8_t *s, const int maxLen, UTF8Encoding &encoding);
    static bool         IsValidUTF8(const char *s, const int maxLen, UTF8Encoding &encoding) { return IsValidUTF8((const uint8_t *)s, maxLen, encoding); }
    static bool         IsValidUTF8(const uint8_t *s, const int maxLen) { UTF8Encoding encoding; return IsValidUTF8(s, maxLen, encoding); }
    static bool         IsValidUTF8(const char *s, const int maxLen) { return IsValidUTF8((const uint8_t *)s, maxLen); }

    static int          Hash(const char *string);
    static int          Hash(const char *string, int length);
    static int          IHash(const char *string);
    static int          IHash(const char *string, int length);

    static char         ToLower(int c);
    static char         ToUpper(int c);
    static bool         CharIsNumeric(int c);
    static bool         CharIsAlpha(int c);
    static bool         CharIsLower(int c);
    static bool         CharIsUpper(int c);
    static bool         CharIsNewLine(int c);
    static bool         CharIsTab(int c);
    static int          ColorIndex(int c);

                        /// Convert string to WStr
    static WStr         ToWStr(const char *str);
#if __OBJC__
                        /// Convert Str to Objective-C NSString
    NSString *          ToNSString() const {
        return [[NSString alloc] initWithBytes:data length:Length() encoding:NSUTF8StringEncoding];
    }
#endif

#ifdef QSTRING_H
                        /// Convert Str to QString
    QString             ToQString() const {
        return QString::fromLatin1(data, len);
    }
#endif

#ifdef __ANDROID__
    /// Convert Str to jstring
    jstring             ToJavaString(JNIEnv *env) const {
        jstring javaString = env->NewStringUTF(data);
        return javaString;
    }
#endif

    void                ReAllocate(int amount, bool keepOld);
    void                FreeData();

    static Str          empty;

private:
                        /// Ensures string data buffer is large enough.
    void                EnsureAlloced(int amount, bool keepOld = true);

    static constexpr int BaseLength = 20;
    static constexpr int AllocGranularity = 32;
    static constexpr int FileNameHashSize = 1024;
 
    char *              data;                       ///< Data pointer
    size_t              alloced;                    ///< Allocated data size
    int                 len;                        ///< String length
    char                baseBuffer[BaseLength];     ///< Default base buffer
};

char * BE_CDECL         va(const char *format, ...);

BE_INLINE void Str::EnsureAlloced(int amount, bool keepOld) {
    if (amount > alloced) {
        ReAllocate(amount, keepOld);
    }
}

BE_INLINE Str::Str() {
    len = 0;
    alloced = BaseLength;
    data = baseBuffer;
    data[0] = '\0';
}

BE_INLINE Str::Str(const bool b) : Str() {
    EnsureAlloced(2, false);
    data[0] = b ? '1' : '0';
    data[1] = '\0';
    len = 1;
}

BE_INLINE Str::Str(const char c) : Str() {
    EnsureAlloced(2, false);
    data[0] = c;
    data[1] = '\0';
    len = 1;
}

BE_INLINE Str::Str(const Str &str) : Str() {
    int l = str.Length();
    EnsureAlloced(l + 1, false);
    strcpy(data, str.data);
    len = l;
}

BE_INLINE Str::Str(Str &&str) noexcept : Str() {
    // just call move assignment operator
    *this = std::move(str);
}

BE_INLINE Str::Str(const char *text) : Str() {
    if (text) {
        int l = (int)strlen(text);
        EnsureAlloced(l + 1, false);
        strcpy(data, text);
        len = l;
    }
}

BE_INLINE Str::Str(const wchar_t *text) : Str() {
    if (text) {
        int l = (int)wcstombs(nullptr, text, 0);
        EnsureAlloced(l + 1, false);
        wcstombs(data, text, l + 1);
        len = l;
    }
}

BE_INLINE Str::Str(const int i) : Str() {
    char text[64];
    int l = Str::snPrintf(text, sizeof(text), "%i", i);
    EnsureAlloced(l + 1, false);
    strcpy(data, text);
    len = l;
}

BE_INLINE Str::Str(const int64_t i) : Str() {
    char text[64];
    int l = Str::snPrintf(text, sizeof(text), "%" PRIi64, i);
    EnsureAlloced(l + 1, false);
    strcpy(data, text);
    len = l;
}

BE_INLINE Str::Str(const unsigned u) : Str() {
    char text[64];
    int l = Str::snPrintf(text, sizeof(text), "%u", u);
    EnsureAlloced(l + 1, false);
    strcpy(data, text);
    len = l;
}

BE_INLINE Str::Str(const uint64_t u) : Str() {
    char text[64];
    int l = Str::snPrintf(text, sizeof(text), "%" PRIu64, u);
    EnsureAlloced(l + 1, false);
    strcpy(data, text);
    len = l;
}

BE_INLINE Str::Str(const float f) : Str() {
    char text[64];
    int l = Str::snPrintf(text, sizeof(text), "%f", f);
    EnsureAlloced(l + 1, false);
    strcpy(data, text);
    len = l;
}

BE_INLINE Str::Str(const double d) : Str() {
    char text[64];
    int l = Str::snPrintf(text, sizeof(text), "%lf", d);
    EnsureAlloced(l + 1, false);
    strcpy(data, text);
    len = l;
}

BE_INLINE Str &Str::operator=(const Str &rhs) {
    int l = rhs.Length();
    EnsureAlloced(l + 1, false);
    strcpy(data, rhs.data);
    len = l;
    return *this;
}

BE_INLINE Str &Str::operator=(Str &&rhs) noexcept {
    BE1::Swap(len, rhs.len);
    BE1::Swap(alloced, rhs.alloced);
    BE1::Swap(data, rhs.data);
    if (data == rhs.baseBuffer) {
        strcpy(baseBuffer, rhs.baseBuffer);
        data = baseBuffer;
    }
    if (rhs.data == baseBuffer) {
        rhs.data = rhs.baseBuffer;
    }
    return *this;
}

BE_INLINE Str::~Str() {
    FreeData();
}

BE_INLINE size_t Str::Size() const {
    return sizeof(*this) + Allocated();
}

BE_INLINE const char &Str::operator[](int index) const {
    assert((index >= 0) && (index <= len));
    return data[index];
}

BE_INLINE char &Str::operator[](int index) {
    assert((index >= 0) && (index <= len));
    return data[index];
}

BE_INLINE Str &Str::operator+=(const Str &rhs) {
    Append(rhs);
    return *this;
}

BE_INLINE Str &Str::operator+=(const char *rhs) {
    Append(rhs);
    return *this;
}

BE_INLINE Str &Str::operator+=(const float rhs) {
    char text[64];

    Str::snPrintf(text, sizeof(text), "%f", rhs);
    Append(text);
    return *this;
}

BE_INLINE Str &Str::operator+=(const char rhs) {
    Append(rhs);
    return *this;
}

BE_INLINE Str &Str::operator+=(const int rhs) {
    char text[64];

    Str::snPrintf(text, sizeof(text), "%i", rhs);
    Append(text);
    return *this;
}

BE_INLINE Str &Str::operator+=(const unsigned rhs) {
    char text[64];

    Str::snPrintf(text, sizeof(text), "%u", rhs); 
    Append(text);
    return *this;
}

BE_INLINE Str &Str::operator+=(const bool rhs) {
    Append(rhs ? "true" : "false");
    return *this;
}

BE_INLINE Str operator+(const Str &lhs, const Str &rhs) {
    Str result(lhs);

    result.Append(rhs);
    return result;
}

BE_INLINE Str operator+(const Str &lhs, const char *rhs) {
    Str result(lhs);

    result.Append(rhs);
    return result;
}

BE_INLINE Str operator+(const char *lhs, const Str &rhs) {
    Str result(lhs);

    result.Append(rhs);
    return result;
}

BE_INLINE Str operator+(const Str &lhs, const float rhs) {
    char text[64];
    Str result(lhs);
    
    Str::snPrintf(text, sizeof(text), "%f", rhs);
    result.Append(text);
    return result;
}

BE_INLINE Str operator+(const Str &lhs, const int rhs) {
    char text[64];
    Str result(lhs);
    
    Str::snPrintf(text, sizeof(text), "%i", rhs);
    result.Append(text);
    return result;
}

BE_INLINE Str operator+(const Str &lhs, const unsigned rhs) {
    char text[64];
    Str result(lhs);
    
    Str::snPrintf(text, sizeof(text), "%u", rhs);
    result.Append(text);
    return result;
}

BE_INLINE Str operator+(const Str &lhs, const bool rhs) {
    Str result(lhs);
    
    result.Append(rhs ? "true" : "false");
    return result;
}

BE_INLINE Str operator+(const Str &lhs, const char rhs) {
    Str result(lhs);

    result.Append(rhs);
    return result;
}

BE_INLINE int Str::Cmp(const char *text) const {
    assert(text);
    return Str::Cmp(data, text);
}

BE_INLINE int Str::Cmpn(const char *text, int n) const {
    assert(text);
    return Str::Cmpn(data, text, n);
}

BE_INLINE int Str::CmpNat(const char *text) const {
    assert(text);
    return Str::CmpNat(data, text);
}

BE_INLINE int Str::Icmp(const char *text) const {
    assert(text);
    return Str::Icmp(data, text);
}

BE_INLINE int Str::Icmpn(const char *text, int n) const {
    assert(text);
    return Str::Icmpn(data, text, n);
}

BE_INLINE int Str::IcmpPath(const char *text) const {
    assert(text);
    return Str::IcmpPath(data, text);
}

BE_INLINE int Str::IcmpnPath(const char *text, int n) const {
    assert(text);
    return Str::IcmpnPath(data, text, n);
}

BE_INLINE int Str::IcmpNat(const char *text) const {
    assert(text);
    return Str::IcmpNat(data, text);
}

BE_INLINE float Str::FuzzyScore(const char *text, float fuzziness) const {
    assert(text);
    return Str::FuzzyScore(data, text, fuzziness);
}

BE_INLINE size_t Str::Allocated() const {
    if (data != baseBuffer) {
        return alloced;
    }    
    return 0;
}

BE_INLINE void Str::Clear() {
    EnsureAlloced(1);
    data[0] = '\0';
    len = 0;
}

BE_INLINE bool Str::IsEmpty() const {
    return (data[0] == 0);
}

BE_INLINE void Str::ToLower() {
    for (int i = 0; data[i]; i++) {
        if (CharIsUpper(data[i])) {
            data[i] += ('a' - 'A');
        }
    }
}

BE_INLINE void Str::ToUpper() {
    for (int i = 0; data[i]; i++) {
        if (CharIsLower(data[i])) {
            data[i] -= ('a' - 'A');
        }
    }
}

BE_INLINE bool Str::IsNumeric() {
    return Str::IsNumeric(data);
}

BE_INLINE bool Str::IsAlpha() {
    return Str::IsAlpha(data);
}

BE_INLINE void Str::Append(const char a) {
    EnsureAlloced(len + 2);
    data[len] = a;
    len++;
    data[len] = '\0';
}

BE_INLINE void Str::Append(const Str &text) {
    int newLen = len + text.Length();
    EnsureAlloced(newLen + 1);
    for (int i = 0; i < text.len; i++) {
        data[len + i] = text[i];
    }

    len = newLen;
    data[len] = '\0';
}

BE_INLINE void Str::Append(const char *text) {
    if (text) {
        int newLen = len + (int)strlen(text);
        EnsureAlloced(newLen + 1);
        for (int i = 0; text[i]; i++) {
            data[len + i] = text[i];
        }
    
        len = newLen;
        data[len] = '\0';
    }
}

BE_INLINE void Str::Append(const char *text, int l) {
    if (text && l) {
        int newLen = len + l;
        EnsureAlloced(newLen + 1);
        for (int i = 0; text[i] && i < l; i++) {
            data[len + i] = text[i];
        }
        
        len = newLen;
        data[len] = '\0';
    }
}
   
BE_INLINE void Str::Insert(const char a, int index) {
    Clamp(index, 0, len);

    int l = 1;
    EnsureAlloced(len + l + 1);
    for (int i = len; i >= index; i--) {
        data[i+l] = data[i];
    }

    data[index] = a;
    len++;
}

BE_INLINE void Str::Insert(const char *text, int index) {
    Clamp(index, 0, len);

    int l = (int)strlen(text);
    EnsureAlloced(len + l + 1);
    for (int i = len; i >= index; i--) {
        data[i+l] = data[i];
    }

    for (int i = 0; i < l; i++) {
        data[index+i] = text[i];
    }

    len += l;
}

BE_INLINE void Str::Truncate(int position) {
    if (len <= position) {
        return;
    }

    data[position] = 0;
    len = position;
}

BE_INLINE void Str::Chop(int n) {
    if (n >= len) {
        Clear();
        return;
    }

    data[len - n] = 0;
    len = n;
}

BE_INLINE void Str::Fill(const char ch, int newLen) {
    EnsureAlloced(newLen + 1);
    len = newLen;
    memset(data, ch, len);
    data[len] = 0;
}

BE_INLINE void Str::ConvertToUTF8() {
    Str temp(*this);
    Clear();
    for (int index = 0; index < temp.Length(); ++index) {
        AppendUTF8Char(temp[index]);
    }
}

BE_INLINE int Str::Find(const char ch, int start, int end) const {
    if (end == -1) {
        end = len; 
    }

    return Str::FindChar(data, ch, start, end);
}

BE_INLINE int Str::Find(const char *text, bool caseSensitive, int start, int end) const {
    if (end == -1) {
        end = len;
    }

    return Str::FindText(data, text, caseSensitive, start, end);
}

BE_INLINE int Str::FindLast(const char ch, int start, int end) const {
    if (end == -1) {
        end = len;
    }

    return Str::FindLastChar(data, ch, start, end);
}

BE_INLINE int Str::FindLast(const char *text, bool caseSensitive, int start, int end) const {
    if (end == -1) {
        end = len;
    }

    return Str::FindLastText(data, text, caseSensitive, start, end);
}

BE_INLINE bool Str::Filter(const char *filter, bool caseSensitive) const {
    return Str::Filter(filter, data, caseSensitive);
}

BE_INLINE const char *Str::Left(int len, Str &result) const {
    return Mid(0, len, result);
}

BE_INLINE const char *Str::Right(int len, Str &result) const {
    if (len >= Length()) {
        result = *this;
        return result;
    }
    return Mid(Length() - len, len, result);
}

BE_INLINE Str Str::Left(int len) const {
    return Mid(0, len);
}

BE_INLINE Str Str::Right(int len) const {
    if (len >= Length()) {
        return *this;
    }
    return Mid(Length() - len, len);
}

BE_INLINE Str &Str::StripQuotes() {
    if (data[0] != '\"') {
        return *this;
    }
    
    // Remove the trailing quote first
    if (data[len - 1] == '\"') {
        data[len - 1] = '\0';
        len--;
    }

    // Strip the leading quote now
    len--;	
    memmove(&data[0], &data[1], len);
    data[len] = '\0';
    
    return *this;
}

BE_INLINE Str &Str::Replace(int index, int length, const char *after) {
    if (index + length > len) {
        return *this;
    }

    *this = Left(index) + Str(after) + Right(len - (index + length));
    return *this;
}

BE_INLINE Str &Str::Replace(const char *before, const char *after, bool caseSensitive) {
    int s = Find(before, caseSensitive);
    if (s == -1) {
        return *this;
    }

    *this = Left(s) + Str(after) + Right(len - (s + Length(before)));
    return *this;
}

BE_INLINE Str &Str::ReplaceAll(const char *before, const char *after, bool caseSensitive) {
    int afterLen = Length(after);
    int findStart = 0;
    
    int s = Find(before, caseSensitive, findStart);
    while (s >= 0) {
        *this = Left(s) + Str(after) + Right(len - (s + Length(before)));

        findStart = s + afterLen;

        s = Find(before, caseSensitive, findStart);
    }

    return *this;
}

//--------------------------------------------------------------------------------------------------
//
// char * methods to replace library functions
//
//--------------------------------------------------------------------------------------------------

BE_INLINE int Str::Length(const char *s) {
    int i;
    for (i = 0; s[i]; i++) {}
    return i;
}

BE_INLINE int Str::LineCount(const char *s) {
    int count = 1;
    for (int i = 0; s[i]; i++) {
        if (s[i] == '\n') {
            count++;
        }
    }
    return count;
}

BE_INLINE char *Str::ToLower(char *s) {
    for (int i = 0; s[i]; i++) {
        if (CharIsUpper(s[i])) {
            s[i] += ('a' - 'A');
        }
    }
    return s;
}

BE_INLINE char *Str::ToUpper(char *s) {
    for (int i = 0; s[i]; i++) {
        if (CharIsLower(s[i])) {
            s[i] -= ('a' - 'A');
        }
    }
    return s;
}

BE_INLINE bool Str::IsNumeric(const char *s) {
    if (*s == '-') {
        s++;
    }

    bool dot = false;
    for (int i = 0; s[i]; i++) {
        if (!CharIsNumeric(s[i])) {
            if ((s[i] == '.') && !dot) {
                dot = true;
                continue;
            }
            return false;
        }
    }

    return true;
}

BE_INLINE bool Str::IsAlpha(const char *s) {
    for (int i = 0; s[i]; i++) {
        if (!CharIsAlpha(s[i])) {
            return false;
        }
    }

    return true;
}

BE_INLINE int32_t Str::ToI32(const char *s) {
    return (int32_t)strtol(s, nullptr, 10);
}

BE_INLINE uint32_t Str::ToUI32(const char *s) {
    return (uint32_t)strtoul(s, nullptr, 10);
}

BE_INLINE int64_t Str::ToI64(const char *s) {
    return strtoll(s, nullptr, 10);
}

BE_INLINE uint64_t Str::ToUI64(const char *s) {
    return strtoull(s, nullptr, 10);
}

BE_INLINE bool Str::CheckExtension(const char *ext) const {
    return Str::CheckExtension(data, ext);
}

BE_INLINE int Str::Hash(const char *string) {
    int hash = 0;
    for (int i = 0; *string != '\0'; i++) {
        hash += (*string++) * (i + 119);
    }
    return hash;
}

BE_INLINE int Str::Hash(const char *string, int length) {
    int hash = 0;
    for (int i = 0; i < length; i++) {
        hash += (*string++) * (i + 119);
    }
    return hash;
}

BE_INLINE int Str::IHash(const char *string) {
    int hash = 0;
    for (int i = 0; *string != '\0'; i++) {
        hash += ToLower(*string++) * (i + 119);
    }
    return hash;
}

BE_INLINE int Str::IHash(const char *string, int length) {
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

BE_INLINE char Str::ToLower(int c) {
    if (c >= 'A' && c <= 'Z') {
        return (c - 'A' + 'a');
    }
    return c;
}

BE_INLINE char Str::ToUpper(int c) {
    if (c >= 'a' && c <= 'z') {
        return (c - 'a' + 'A');
    }
    return c;
}

BE_INLINE bool Str::CharIsNumeric(int c) {
    return (c <= '9' && c >= '0');
}

BE_INLINE bool Str::CharIsAlpha(int c) {
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

BE_INLINE bool Str::CharIsLower(int c) {
    return (c >= 'a' && c <= 'z');
}

BE_INLINE bool Str::CharIsUpper(int c) {
    return (c >= 'A' && c <= 'Z');
}

BE_INLINE bool Str::CharIsNewLine(int c) {
    return (c == '\n' || c == '\r');
}

BE_INLINE bool Str::CharIsTab(int c) {
    return (c == '\t');
}

BE_INLINE int Str::ColorIndex(int c) {
    return (c & 15);
}

BE_NAMESPACE_END
