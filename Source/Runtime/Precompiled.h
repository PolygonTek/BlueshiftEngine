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

#define BE_NAME "Blueshift Engine"

#include "Version.h"

#define BE_STRINGIZE(x) #x
#define BE_CONCAT_VERSION(a, b, c) BE_STRINGIZE(a) "." BE_STRINGIZE(b) "." BE_STRINGIZE(c)
#define BE_VERSION BE_CONCAT_VERSION(BE_VERSION_MAJOR, BE_VERSION_MINOR, BE_VERSION_PATCH)

//----------------------------------------------------------------------------------------------
// Platform detection defines
//----------------------------------------------------------------------------------------------

// Detect x86 32 bit platform.
#if defined(__i386__) || defined(_M_IX86)
    #if !defined(__X86__)
        #define __X86__
    #endif
    #if !defined(__X86_32__)
        #define __X86_32_
    #endif
#endif

// Detect x86 64 bit platform.
#if defined(__x86_64__) || defined(__ia64__) || defined(_M_X64)
    #if !defined(__X86__)
        #define __X86__
    #endif
    #if !defined(__X86_64__)
        #define __X86_64__
    #endif
#endif

// Detect ARM 32 bit platform.
#if defined(__arm__) || defined(_M_ARM)
    #if !defined(__ARM__)
        #define __ARM__
    #endif
#endif

// Detect ARM 64 bit platform.
#if defined(__arm64__) || defined(__aarch64__) || defined(_M_ARM64)
    #if !defined(__ARM__)
        #define __ARM__
    #endif
    #if !defined(__ARM64__)
        #define __ARM64__
    #endif
#endif

// Detect Linux platform.
#if defined(linux) || defined(__linux__) || defined(__LINUX__)
    #if !defined(__LINUX__)
        #define __LINUX__
    #endif
    #if !defined(__UNIX__)
        #define __UNIX__
    #endif
#endif

// Detect FreeBSD platform.
#if defined(__FreeBSD__) || defined(__FREEBSD__)
    #if !defined(__FREEBSD__)
        #define __FREEBSD__
    #endif
    #if !defined(__UNIX__)
        #define __UNIX__
    #endif
#endif

// Detect Windows 95/98/NT/2000/XP/Vista/7 platform.
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)) && !defined(__CYGWIN__)
    #if !defined(__WIN32__)
        #define __WIN32__
    #endif
#endif

// Detect Cygwin platform.
#if defined(__CYGWIN__)
    #if !defined(__UNIX__)
        #define __UNIX__
    #endif
#endif

// Detect MacOS X & iOS/iOS simulator platform.
#if defined(__APPLE__)
    #define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0 // conflict with boost_1_58_0
    #include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR == 1
        #if !defined(__IOS_SIMULATOR__)
            #define __IOS_SIMULATOR__
            #define __IOS__
        #endif
    #elif TARGET_OS_IPHONE == 1
        #if !defined(__IOS__)
            #define __IOS__
        #endif
    #elif TARGET_OS_MAC == 1
        #if !defined(__MACOSX__)
            #define __MACOSX__
        #endif
    #endif
    #if !defined(__UNIX__)
        #define __UNIX__
    #endif
#endif

// Detect Android platform.
#if defined(ANDROID) || defined(__ANDROID__)
    #if !defined(__ANDROID__)
        #define __ANDROID__
    #endif
#endif

// Try to detect other Unix systems.
#if defined(__unix__) || defined (unix) || defined(__unix) || defined(_unix)
    #if !defined(__UNIX__)
        #define __UNIX__
    #endif
#endif

//----------------------------------------------------------------------------------------------
// Common platform
//----------------------------------------------------------------------------------------------

// We don't need this because we don't use character literal in other language.
//#pragma setlocale ("kor") 

// C runtime header files.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <assert.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <time.h>
#include <inttypes.h>
#include <locale.h>

// C++ runtime header files.
#include <cstddef>
#include <typeinfo>
#include <type_traits>
#include <iostream>
#include <algorithm>
#include <functional>
#include <initializer_list>
#include <utility>
#include <cmath>
#include <memory>
#include <string>
#include <limits>
#include <exception>
#include <atomic>
#include <thread>

#define BE1 BE1
#define BE_NAMESPACE_BEGIN namespace BE1 {
#define BE_NAMESPACE_END }

#define BE_STATIC_LINK

#if defined(BE_STATIC_LINK) || !defined(__WIN32__)
    #define BE_API
#elif defined(BE_EXPORTS)
    #define BE_API __declspec (dllexport)
#else 
    #define BE_API __declspec (dllimport)
#endif

#if defined(__WIN32__) && !defined(__X86_64__)
    #define BE_FASTCALL __fastcall
#else
    #define BE_FASTCALL
#endif

#define CONCAT(a, b)                _CONCAT_IMPL(a, b)
#define _CONCAT_IMPL(a, b)          a ## b

#define OVERLOADED_MACRO(m, ...)    _OVR(m, COUNT_ARGS(__VA_ARGS__)) (__VA_ARGS__)
#define _OVR(macro, num)            _OVR_EXPAND(macro, num)
#define _OVR_EXPAND(macro, num)     macro##_ARG##num

#define COUNT_ARGS(...)             _ARG_PATTERN_MATCH(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1)
#define _ARG_PATTERN_MATCH(_1, _2, _3, _4, _5, _6, _7, _8, n, ...) n

#define COUNT_OF(a)                 ((int)(sizeof(a) / sizeof((a)[0])))

#define OFFSET_OF(type, member)     ((intptr_t)&((type *)0)->member)

#ifndef BIT
    #define BIT(num)                (1 << (num))
#endif

#define ADD_BIT(x, num)             (x |= BIT(num))
#define SUB_BIT(x, num)             (x &= ~BIT(num))

#define MAKE_FOURCC(a, b, c, d)     (((d) << 24) + ((c) << 16) + ((b) << 8) + (a))

#define SWAP_WORD(w)                ((((w) & 0xff) << 8) | (((w) & 0xff00) >> 8))
#define SWAP_LONG(l)                ((((l) & 0xff) << 24) | (((l) & 0xff00) << 8) | (((l) & 0xff0000) >> 8) | (((l) & 0xff000000) >> 24))

#define RANDOM_FLOAT(l, h)          ((l) + (((rand() & 0x7ffe) / ((float)0x7fff)) * (float)((h) - (l))))
#define RANDOM_INT(l, h)            ((l) + ((((rand() * (int)((h) - (l) + 1)) - 1) / (RAND_MAX))))

#define SAFE_DELETE(p)              if (p) { delete p; p = nullptr; }
#define SAFE_DELETE_ARRAY(p)        if (p) { delete[] p; p = nullptr; }

#ifndef FLT_INFINITY
    #define FLT_INFINITY            std::numeric_limits<float>::infinity()
#endif

// Useful macro for forward declaration of Objective-C class in C/C++.
#ifdef __OBJC__
    #define OBJC_CLASS(name) @class name
#else
    #define OBJC_CLASS(name) typedef struct objc_object name
#endif

typedef uint8_t     byte;   // 8 bits
typedef uint16_t    word;   // 16 bits
typedef uint32_t    dword;  // 32 bits
typedef uint64_t    qword;  // 64 bits

#ifdef _DEBUG

#define assert_8_byte_aligned(pointer)      assert( ( ((uintptr_t)(pointer)) &  7 ) == 0 )
#define assert_16_byte_aligned(pointer)     assert( ( ((uintptr_t)(pointer)) & 15 ) == 0 )
#define assert_32_byte_aligned(pointer)     assert( ( ((uintptr_t)(pointer)) & 31 ) == 0 )
#define assert_64_byte_aligned(pointer)     assert( ( ((uintptr_t)(pointer)) & 63 ) == 0 )

#else

#define assert_8_byte_aligned(pointer)
#define assert_16_byte_aligned(pointer)
#define assert_32_byte_aligned(pointer)
#define assert_64_byte_aligned(pointer)

#endif

#ifdef _MSC_VER
    #if (_MSC_VER >= 1800) // VS2013
        #define __alignas_is_defined 1
    #endif
    #if (_MSC_VER >= 1900) // VS2015
        #define __alignof_is_defined 1
    #endif
#else
    #include <stdalign.h>   // __alignas/of_is_defined directly from the implementation
#endif

#ifdef __alignas_is_defined
    #define ALIGN_AS(x) alignas(x)
#else
    #pragma message("C++11 alignas unsupported :( Falling back to compiler attributes")
    #ifdef __GNUG__
        #define ALIGN_AS(x) __attribute__ ((aligned(x)))
    #elif defined(_MSC_VER)
        #define ALIGN_AS(x) __declspec(align(x))
    #else
        #error Unknown compiler, unknown alignment attribute!
    #endif
#endif

#ifdef __alignof_is_defined
    #define ALIGN_OF(x) alignof(x)
#else
    #pragma message("C++11 alignof unsupported :( Falling back to compiler attributes")
    #ifdef __GNUG__
        #define ALIGN_OF(x) __alignof__ (x)
    #elif defined(_MSC_VER)
        #define ALIGN_OF(x) __alignof(x)
    #else
        #error Unknown compiler, unknown alignment attribute!
    #endif
#endif

#define ALIGN_AS16 ALIGN_AS(16)
#define ALIGN_AS32 ALIGN_AS(32)

template <typename T>
inline T *address_of(T &&in) {
    return &in;
}

template <bool... B>
struct static_all_of;

// Do recursion if the first argument is true.
template <bool... Tail>
struct static_all_of<true, Tail...> : static_all_of<Tail...> {};

// End recursion if first argument is false.
template <bool... Tail>
struct static_all_of<false, Tail...> : std::false_type {};

// End recursion if no more arguments need to be processed.
template <> struct static_all_of<> : std::true_type {};

template <typename T, typename... Ts>
struct is_same_all : static_all_of<std::is_same<typename std::decay<Ts>::type, T>::value...> {};

template <typename T, typename... Ts>
struct is_assignable_all : static_all_of<std::is_assignable<T, Ts>::value...> {};

template <typename T1, typename T2>
inline const ptrdiff_t offset_of(T1 T2::*member) {
    static char obj_dummy[sizeof(T2)];
    const T2 *obj = reinterpret_cast<T2 *>(obj_dummy);
    return ptrdiff_t(intptr_t(&(obj->*member)) - intptr_t(obj));
}

template <typename T, std::size_t N>
constexpr std::size_t count_of(T (&)[N]) {
    return N;
}

template <typename T>
struct return_type;

template <typename R, typename... Args>
struct return_type<R(*)(Args...)> { using type = R; };

template <typename R, typename C, typename... Args>
struct return_type<R(C:: *)(Args...)> { using type = R; };

template <typename R, typename C, typename... Args>
struct return_type<R(C:: *)(Args...) const> { using type = R; };

template <typename R, typename C, typename... Args>
struct return_type<R(C:: *)(Args...) volatile> { using type = R; };

template <typename R, typename C, typename... Args>
struct return_type<R(C:: *)(Args...) const volatile> { using type = R; };

template <typename T>
using return_type_t = typename return_type<T>::type;

//----------------------------------------------------------------------------------------------
// Win32
//----------------------------------------------------------------------------------------------

#ifdef __WIN32__

#if defined(_MSC_VER)
#pragma warning (disable: 4018)     // signed/unsigned mismatch
#pragma warning (disable: 4221)     // ignore object file does not define any previously undefined public symbols
#pragma warning (disable: 4244)     // disable conversion warnings (double -> float)
#pragma warning (disable: 4267)     // conversion from 'size_t' to 'int', possible loss of data
#pragma warning (disable: 4305)     // truncation from const double to float
#pragma warning (disable: 4819)     // The file contains a character that cannot be represented in the current code page
#pragma warning (disable: 4996)     // This function or variable may be unsafe
#pragma warning (disable: 26812)    // Prefer 'enum class' over 'enum' (Enum.3)
#pragma warning (disable: 26495)    // Variable '%variable%' is uninitialized. Always initialize a member variable (type.6).
#endif

#define BE_CDECL

#define BE_FORCE_INLINE             __forceinline
#define BE_INLINE                   inline

#define BE_DEPRECATED               __declspec(deprecated("This is deprecated and will be removed in a future version."))

#define CURRENT_FUNC                __FUNCTION__

#define PATHSEPERATOR_STR           "\\"
#define PATHSEPERATOR_CHAR          '\\'

#define PATHSEPERATOR_WSTR          L"\\"
#define PATHSEPERATOR_WCHAR         L'\\'

#define _alloca16(x)                ((void *)((((intptr_t)_alloca((x) + 15)) + 15) & ~15))
#define _alloca32(x)                ((void *)((((intptr_t)_alloca((x) + 31)) + 31) & ~31))

#if !defined(__thread)
#define __thread                    __declspec(thread)
#endif

#define strtoll                     _strtoi64
#define strtoull                    _strtoui64

#define wcstoll                     _wcstoi64
#define wcstoull                    _wcstoui64

#ifdef _DEBUG
//#define _CRTDBG_MAP_ALLOC
#endif

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#include <SDKDDKVer.h>

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _CRT_NON_CONFORMING_SWPRINTFS
#define _CRT_NON_CONFORMING_SWPRINTFS
#endif

// Exclude rarely-used stuff from Windows headers.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

//----------------------------------------------------------------------------------------------
// Definition for stripping unused code from Windows headers.
//----------------------------------------------------------------------------------------------
#define NOGDICAPMASKS
//#define NOVIRTUALKEYCODES
//#define NOWINMESSAGES
//#define NOWINSTYLES
//#define NOSYSMETRICS
//#define NOMENUS
//#define NOICONS
#define NOKEYSTATES
//#define NOSYSCOMMANDS
#define NORASTEROPS
//#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
//#define NOCOLOR
//#define NOCTLMGR
#define NODRAWTEXT
//#define NOGDI
#define NOKERNEL
//#define NOUSER
//#define NONLS
//#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
//#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
//#define NOWH
//#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

// Windows specific headers
#include <windows.h>
#include <intrin.h>

#undef Yield

#endif

//----------------------------------------------------------------------------------------------
// Unix-like system
//----------------------------------------------------------------------------------------------

#ifdef __UNIX__

#define BE_CDECL                    // __attribute__((__cdecl__)) //__cdecl

#define BE_FORCE_INLINE             inline //__attribute__((always_inline))
#define BE_INLINE                   inline

#define BE_DEPRECATED               __attribute__((deprecated("This is deprecated and will be removed in a future version.")))

#define CURRENT_FUNC                __func__

#define PATHSEPERATOR_STR           "/"
#define PATHSEPERATOR_CHAR          '/'

#define PATHSEPERATOR_WSTR          L"/"
#define PATHSEPERATOR_WCHAR         L'/'

#define _alloca                     alloca
#define _alloca16(x)                ((void *)((((intptr_t)alloca((x) + 15)) + 15) & ~15))
#define _alloca32(x)                ((void *)((((intptr_t)alloca((x) + 31)) + 31) & ~31))

#define TCHAR                       char
#endif // __UNIX__

//----------------------------------------------------------------------------------------------
// iOS & macOS
//----------------------------------------------------------------------------------------------

#ifdef __APPLE__

#include <CoreFoundation/CoreFoundation.h>

#ifdef __IOS__
#undef BE_CDECL
#define BE_CDECL
#ifdef __OBJC__
#import <UIKit/UIKit.h>
#endif // __OBJC__
#endif // __IOS__

#ifdef __MACOSX__
#include <Carbon/Carbon.h>
#ifdef __OBJC__
#import <AppKit/AppKit.h>
#endif // __OBJC__
#endif // __MACOSX__

BE_FORCE_INLINE void CFStringToString(CFStringRef cfstring, char *string) {
    const size_t length = CFStringGetLength(cfstring);
    CFRange range = CFRangeMake(0, length);
    CFStringGetBytes(cfstring, range, kCFStringEncodingUTF8, '?', false, (UInt8 *)string, length * sizeof(string[0]) + 1, nullptr);
    string[length] = 0;
}

BE_FORCE_INLINE CFStringRef StringToCFString(const char *string) {
    const size_t length = strlen(string);
    CFStringRef cfstring = CFStringCreateWithBytes(kCFAllocatorDefault, (UInt8 *)string, length * sizeof(string[0]), kCFStringEncodingUTF8, false);
    return cfstring;
}

BE_FORCE_INLINE void CFStringToWideString(CFStringRef cfstring, wchar_t *string) {
    const size_t length = CFStringGetLength(cfstring);
    CFRange range = CFRangeMake(0, length);
    CFStringGetBytes(cfstring, range, kCFStringEncodingUTF32LE, '?', false, (UInt8 *)string, length * sizeof(string[0]) + 1, nullptr);
    string[length] = 0;
}

BE_FORCE_INLINE CFStringRef WideStringToCFString(const wchar_t *string) {
    const size_t length = wcslen(string);
    CFStringRef cfstring = CFStringCreateWithBytes(kCFAllocatorDefault, (UInt8 *)string, length * sizeof(string[0]), kCFStringEncodingUTF32LE, false);
    return cfstring;
}

#endif // __APPLE__

//----------------------------------------------------------------------------------------------
// Android
//----------------------------------------------------------------------------------------------

#ifdef __ANDROID__

#include <jni.h>

#endif // __ANDROID__

//----------------------------------------------------------------------------------------------

// For Intel processors FMA were introduced with AVX2 by Intel Haswell.
// And all AMD processors support FMA/FMA3 if AVX2 is present.
#if !defined(__FMA__) && defined(__AVX2__)
    #define __FMA__
#endif

#if (defined(__SSE__) && defined(__SSE2__) && defined(__SSE3__) && defined(__SSSE3__)) || defined(__NEON__)
    #define ENABLE_SIMD_INTRIN
    //#define ENABLE_SIMD_INTRIN_IN_DEBUG
#endif

#if defined(_DEBUG) && !defined(ENABLE_SIMD_INTRIN_IN_DEBUG)
    #undef ENABLE_SIMD_INTRIN
#endif

#ifdef ENABLE_SIMD_INTRIN
    #if defined(__SSE__) && defined(__SSE2__) && defined(__SSE3__) && defined(__SSSE3__)
        #define HAVE_X86_SSE_INTRIN
    #elif defined(__NEON__)
        #define HAVE_ARM_NEON_INTRIN
    #endif

    #if defined(__AVX__)
        #define HAVE_X86_AVX_INTRIN
    #endif

    #if defined(HAVE_X86_SSE_INTRIN) || defined(HAVE_ARM_NEON_INTRIN)
        #define ENABLE_SIMD4_INTRIN
    #endif

    #if defined(HAVE_X86_AVX_INTRIN)
        #define ENABLE_SIMD8_INTRIN
    #endif
#endif

typedef void (*streamOutFunc_t)(int level, const char *msg);

BE_NAMESPACE_BEGIN

enum {
    MaxAbsolutePath = 256,
    MaxRelativePath = 128
};

struct LogLevel {
    enum Enum {
        Normal,
        Dev,
        Warning,
        Error
    };
};

struct ErrorLevel {
    enum Enum {
        Restart,
        Fatal
    };
};

BE_FORCE_INLINE constexpr int64_t BE_API    MakeQWord(dword l, dword h) { return (qword)(l & 0xffffffff) | (((qword)(h & 0xffffffff)) << 32); }
BE_FORCE_INLINE constexpr int32_t BE_API    MakeDWord(word l, word h) { return (dword)(l & 0xffff) | (((dword)(h & 0xffff)) << 16); }
BE_FORCE_INLINE constexpr int32_t BE_API    MakeWord(byte l, byte h) { return (word)(l & 0xff) | (((word)(h & 0xff)) << 8); }

BE_FORCE_INLINE constexpr int16_t BE_API    LowDWord(qword q) { return (q & 0xffffffff); }
BE_FORCE_INLINE constexpr int16_t BE_API    HighDWord(qword q) { return ((q >> 32) & 0xffffffff); }
BE_FORCE_INLINE constexpr int16_t BE_API    LowWord(dword d) { return (d & 0xffff); }
BE_FORCE_INLINE constexpr int16_t BE_API    HighWord(dword d) { return ((d >> 16) & 0xffff); }
BE_FORCE_INLINE constexpr int16_t BE_API    LowByte(word w) { return (w & 0xff); }
BE_FORCE_INLINE constexpr int16_t BE_API    HighByte(word w) { return ((w >> 8) & 0xff); }

BE_FORCE_INLINE constexpr bool BE_API       IsPowerOf2(int x) { return x && ((x & (x - 1)) == 0); }

/// Tests if the value is aligned.
template <typename T>
BE_FORCE_INLINE constexpr bool IsAligned(const T &x, int n) { return IsPowerOf2(n) ? ((x & (n - 1)) == 0) : ((x % n) == 0); }

/// Returns aligned up value by n.
template <typename T>
BE_FORCE_INLINE constexpr T AlignUp(const T &x, int n) { return IsPowerOf2(n) ? ((x + n - 1) & (~(n - 1))) : ((x + n - 1) - (x + n - 1) % n); }

/// Returns aligned down value by n. 
template <typename T>
BE_FORCE_INLINE constexpr T AlignDown(const T &x, int n) { return IsPowerOf2(n) ? (x & (~(n - 1))) : (x - x % n); }

/// Returns the smaller of two values.
template <typename T>
BE_FORCE_INLINE constexpr T Min(const T &x, const T &y) { return (x < y) ? x : y; }
/// Returns the larger of two values.
template <typename T>
BE_FORCE_INLINE constexpr T Max(const T &x, const T &y) { return (x > y) ? x : y; }

/// Returns the smaller index of two values.
template <typename T>
BE_FORCE_INLINE constexpr int MinIndex(const T &x, const T &y) { return (x < y) ? 0 : 1; }
/// Returns the larger index of two values.
template <typename T>
BE_FORCE_INLINE constexpr int MaxIndex(const T &x, const T &y) { return (x > y) ? 0 : 1; }

/// Returns the smaller of three values.
template <typename T>
BE_FORCE_INLINE constexpr T Min3(const T &x, const T &y, const T &z) { return (x < y) ? ((x < z) ? x : z) : ((y < z) ? y : z); }
/// Returns the larger of three values.
template <typename T>
BE_FORCE_INLINE constexpr T Max3(const T &x, const T &y, const T &z) { return (x > y) ? ((x > z) ? x : z) : ((y > z) ? y : z); }
/// Returns the median of three values.
template <typename T>
BE_FORCE_INLINE constexpr T Median(const T &x, const T &y, const T &z) { return (x > y) ? ((x > z) ? Max(y, z) : x) : ((y > z) ? Max(x, z) : y); }

/// Returns the smaller index of three values.
template <typename T>
BE_FORCE_INLINE constexpr int Min3Index(const T &x, const T &y, const T &z) { return (x < y) ? ((x < z) ? 0 : 2) : ((y < z) ? 1 : 2); }
/// Returns the larger index of three values.
template <typename T>
BE_FORCE_INLINE constexpr int Max3Index(const T &x, const T &y, const T &z) { return (x > y) ? ((x > z) ? 0 : 2) : ((y > z) ? 1 : 2); }
/// Returns the median index of three values.
template <typename T>
BE_FORCE_INLINE constexpr int MedianIndex(const T &x, const T &y, const T &z) { return (x > y) ? ((x > z) ? MaxIndex(y, z) : 0) : ((y > z) ? MaxIndex(x, z) : 1); }

/// Swaps two values.
template <typename T> 
BE_FORCE_INLINE void Swap(T &a, T &b) noexcept { T c = std::move(a); a = std::move(b); b = std::move(c); }

/// Clamps a number to a range.
template <typename T>
BE_FORCE_INLINE void Clamp(T &v, const T &min, const T &max) { v = (v > max) ? max : (v < min ? min : v); }
/// Clamps a number to the range [0, 1].
template <typename T>
BE_FORCE_INLINE void Clamp01(T &v) { Clamp(v, T(0), T(1)); }

/// Returns the clamped number to a range.
template <typename T>
BE_FORCE_INLINE constexpr T Clamp(const T &v, const T &min, const T&max) { return (v > max) ? max : (v < min ? min : v); }
/// Returns the clamped number to the range [0, 1].
template <typename T>
BE_FORCE_INLINE constexpr T Clamp01(const T &v) { return Clamp(v, T(0), T(1)); }

/// Returns remainder of the division operation x / y.
template <typename T>
BE_FORCE_INLINE constexpr T Mod(const T &x, const T &y) { return std::fmod((T)x, (T)y); }
template <>
BE_FORCE_INLINE constexpr int Mod(const int &x, const int &y) { return x % y; }

/// Wraps a number to a range.
template <typename T>
BE_FORCE_INLINE void Wrap(T &v, const T &min, const T &max) {
    if (v > max) v = min + Mod(v - min, max - min);
    if (v < min) v = max - Mod(min - v, max - min);
}

/// Returns the wrapped number to a range.
template <typename T>
BE_FORCE_INLINE constexpr T Wrap(const T &v, const T &min, const T &max) {
    if (v > max) return min + Mod(v - min, max - min);
    if (v < min) return max - Mod(min - v, max - min);
    return v;
}

BE_FORCE_INLINE signed char ClampChar(int i) {
    if (i < -128) return -128;
    if (i > 127) return 127;
    return i;
}

BE_FORCE_INLINE unsigned char ClampByte(int i) {
    if (i < 0) return 0;
    if (i > 255) return 255;
    return i;
}

BE_FORCE_INLINE signed short ClampShort(int i) {
    if (i < -32768) return -32768;
    if (i > 32767) return 32767;
    return i;
}

BE_FORCE_INLINE int ClampInt(int min, int max, int value) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

BE_FORCE_INLINE float ClampFloat(float min, float max, float value) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

template <typename T>
BE_FORCE_INLINE constexpr T UnitToMeter(T x) { return x * 1.0f; }

template <typename T>
BE_FORCE_INLINE constexpr T MeterToUnit(T x) { return x / UnitToMeter(1.0f); }

template <typename T>
BE_FORCE_INLINE constexpr T UnitToCm(T x) { return UnitToMeter(x) * 100.0f; }

template <typename T>
BE_FORCE_INLINE constexpr T CmToUnit(T x) { return MeterToUnit(x * 0.01f); }

template <typename T>
BE_FORCE_INLINE constexpr T UnitToMm(T x) { return UnitToMeter(x) * 1000.0f; }

template <typename T>
BE_FORCE_INLINE constexpr T MmToUnit(T x) { return MeterToUnit(x * 0.001f); }

template <typename T>
BE_FORCE_INLINE constexpr T UnitToKm(T x) { return UnitToMeter(x) * 0.001f; }

template <typename T>
BE_FORCE_INLINE constexpr T KmToUnit(T x) { return MeterToUnit(x * 1000.0f); }

void BE_CDECL BE_API Log(int logLevel, const char *msg, ...);
void BE_CDECL BE_API Error(int errLevel, const char *msg, ...);
void BE_CDECL BE_API Assert(bool expr);

#define BE_LOG(...) do { \
    BE1::Log(BE1::LogLevel::Normal, __VA_ARGS__); \
} while (0)

#define BE_DLOG(...) do { \
    BE1::Log(BE1::LogLevel::Dev, __VA_ARGS__); \
} while (0)

#define BE_WARNLOG(...) do { \
    BE1::Log(BE1::LogLevel::Warning, __VA_ARGS__); \
} while (0)

#define BE_ERRLOG(...) do { \
    BE1::Log(BE1::LogLevel::Error, __VA_ARGS__); \
} while (0)

#define BE_FATALERROR(...) do { \
    BE1::Error(BE1::ErrorLevel::Fatal, __VA_ARGS__); \
} while (0)

#define BE_ASSERT BE1::Assert

#if defined _DEBUG && !defined BE_DEBUG
#define BE_DEBUG
#endif

BE_NAMESPACE_END
