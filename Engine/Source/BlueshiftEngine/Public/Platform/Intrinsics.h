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

////////////////////////////////////////////////////////////////////////////////
/// Windows Platform
////////////////////////////////////////////////////////////////////////////////

#if defined(__WIN32__)

#include <intrin.h>

BE_FORCE_INLINE size_t read_tsc() {
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return (size_t)li.QuadPart;
}

BE_FORCE_INLINE uint64_t __rdpmc(int i) {
    return __readpmc(i);
}

BE_FORCE_INLINE int __bsf(int v) {
    unsigned long r = 0; 
    _BitScanForward(&r, v); 
    return r;
}

BE_FORCE_INLINE int __bsr(int v) {
    unsigned long r = 0; 
    _BitScanReverse(&r, v); 
    return r;
}

BE_FORCE_INLINE int __btc(int v, int i) {
    long r = v; 
    _bittestandcomplement(&r, i); 
    return r;
}

BE_FORCE_INLINE int __bts(int v, int i) {
    long r = v; 
    _bittestandset(&r, i); 
    return r;
}

BE_FORCE_INLINE int __btr(int v, int i) {
    long r = v; 
    _bittestandreset(&r, i); 
    return r;
}

#if (defined(__X86_64__) || defined(__ARM64__)) && !defined(__INTEL_COMPILER)

BE_FORCE_INLINE size_t __bsf(size_t v) {
    unsigned long r = 0; 
    _BitScanForward64(&r, v); 
    return r;
}

BE_FORCE_INLINE size_t __bsr(size_t v) {
    unsigned long r = 0; 
    _BitScanReverse64(&r, v); 
    return r;
}

BE_FORCE_INLINE size_t __btc(size_t v, size_t i) {
    __int64 r = v; 
    _bittestandcomplement64(&r, i); 
    return r;
}

BE_FORCE_INLINE size_t __bts(size_t v, size_t i) {
    __int64 r = v; 
    _bittestandset64(&r, i); 
    return r;
}

BE_FORCE_INLINE size_t __btr(size_t v, size_t i) {
    __int64 r = v; 
    _bittestandreset64(&r, i); 
    return r;
}

#endif // (defined(__X86_64__) || defined(__ARM64__)) && !defined(__INTEL_COMPILER)

#if defined(__X86_64__) || defined(__ARM64__)

typedef int64_t atomic_t;

BE_FORCE_INLINE int64_t atomic_add(volatile int64_t *m, const int64_t v) {
    return _InterlockedExchangeAdd64(m, v);
}

BE_FORCE_INLINE int64_t atomic_xchg(volatile int64_t *p, int64_t v) {
    return _InterlockedExchange64((volatile long long *)p, v);
}

BE_FORCE_INLINE int64_t atomic_cmpxchg(volatile int64_t *m, const int64_t v, const int64_t c) {
    return _InterlockedCompareExchange64(m, v, c);
}

#else // !defined(__X86_64__) && !defined(__ARM64__)

typedef int32_t atomic_t;

BE_FORCE_INLINE int32_t atomic_add(volatile int32_t *p, const int32_t v) {
    return _InterlockedExchangeAdd((volatile long *)p, v);
}

BE_FORCE_INLINE int32_t atomic_xchg(volatile int32_t *p, int32_t v) {
    return _InterlockedExchange((volatile long *)p, v);
}

BE_FORCE_INLINE int32_t atomic_cmpxchg(volatile int32_t *p, const int32_t v, const int32_t c) {
    return _InterlockedCompareExchange((volatile long *)p, v, c);
}

#endif // !defined(__X86_64__) && !defined(__ARM64__)

////////////////////////////////////////////////////////////////////////////////
/// Unix Platform
////////////////////////////////////////////////////////////////////////////////

#elif defined(__UNIX__)

#if defined(__X86__)

BE_FORCE_INLINE void __cpuid(int out[4], int op) {
    asm volatile ("cpuid" : "=a"(out[0]), "=b"(out[1]), "=c"(out[2]), "=d"(out[3]) : "a"(op));
}

BE_FORCE_INLINE uint64_t read_tsc()  {
    uint32_t high, low;
    asm volatile ("rdtsc" : "=d"(high), "=a"(low));
    return (((uint64_t)high) << 32) + (uint64_t)low;
}

BE_FORCE_INLINE uint64_t __rdpmc(int i) {
    uint32_t high, low;
    asm volatile ("rdpmc" : "=d"(high), "=a"(low) : "c"(i));
    return (((uint64_t)high) << 32) + (uint64_t)low;
}

BE_FORCE_INLINE unsigned int __popcnt(unsigned int in) {
    int r = 0; 
    asm ("popcnt %1,%0" : "=r"(r) : "r"(in)); 
    return r;
}

BE_FORCE_INLINE int __bsf(int v) {
    int r = 0; 
    asm ("bsf %1,%0" : "=r"(r) : "r"(v)); 
    return r;
}

BE_FORCE_INLINE int __bsr(int v) {
    int r = 0; 
    asm ("bsr %1,%0" : "=r"(r) : "r"(v)); 
    return r;
}

BE_FORCE_INLINE int __btc(int v, int i) {
    int r = 0; 
    asm ("btc %1,%0" : "=r"(r) : "r"(i), "0"(v) : "flags"); 
    return r;
}

BE_FORCE_INLINE int __bts(int v, int i) {
    int r = 0; 
    asm ("bts %1,%0" : "=r"(r) : "r"(i), "0"(v) : "flags"); 
    return r;
}

BE_FORCE_INLINE int __btr(int v, int i) {
    int r = 0; 
    asm ("btr %1,%0" : "=r"(r) : "r"(i), "0"(v) : "flags"); 
    return r;
}

BE_FORCE_INLINE size_t __bsf(size_t v) {
    size_t r = 0; 
    asm ("bsf %1,%0" : "=r"(r) : "r"(v)); 
    return r;
}

BE_FORCE_INLINE size_t __bsr(size_t v) {
    size_t r = 0; 
    asm ("bsr %1,%0" : "=r"(r) : "r"(v)); 
    return r;
}

BE_FORCE_INLINE size_t __btc(size_t v, size_t i) {
    size_t r = 0; 
    asm ("btc %1,%0" : "=r"(r) : "r"(i), "0"(v) : "flags"); 
    return r;
}

BE_FORCE_INLINE size_t __bts(size_t v, size_t i) {
    size_t r = 0; 
    asm ("bts %1,%0" : "=r"(r) : "r"(i), "0"(v) : "flags"); 
    return r;
}

BE_FORCE_INLINE size_t __btr(size_t v, size_t i) {
    size_t r = 0; 
    asm ("btr %1,%0" : "=r"(r) : "r"(i), "0"(v) : "flags"); 
    return r;
}

#endif // defined(__X86__)

#if defined(__X86_64__) || defined(__ARM64__)

typedef int64_t atomic_t;

BE_FORCE_INLINE int64_t atomic_add(int64_t volatile *value, int64_t input) {
    return __sync_fetch_and_add(value, input);
}

BE_FORCE_INLINE int64_t atomic_xchg(int64_t volatile *value, int64_t input) {
    return __sync_lock_test_and_set(value, input);
}

BE_FORCE_INLINE int64_t atomic_cmpxchg(int64_t volatile *value, const int64_t input, int64_t comparand) {
    return __sync_val_compare_and_swap(value, comparand, input);
}

#else // !defined(__X86_64__) && !defined(__ARM64__)

typedef int32_t atomic_t;

BE_FORCE_INLINE int32_t atomic_add(int32_t volatile *value, int32_t input) {
    return __sync_fetch_and_add(value, input);
}

BE_FORCE_INLINE int32_t atomic_xchg(int32_t volatile *value, int32_t input) {
    return __sync_lock_test_and_set(value, input);
}

BE_FORCE_INLINE int32_t atomic_cmpxchg(int32_t volatile *value, const int32_t input, int32_t comparand) {
    return __sync_val_compare_and_swap(value, comparand, input);
}

#endif // !defined(__X86_64__) && !defined(__ARM64__)

#endif // __UNIX__

////////////////////////////////////////////////////////////////////////////////
/// All Platforms
////////////////////////////////////////////////////////////////////////////////

#if defined(__X86_64__) || defined(__ARM64__)

template <typename T>
BE_FORCE_INLINE T *atomic_xchg(T *volatile *value, const T *input) {
    return (T *)atomic_xchg((int64_t *)value, (int64_t)input); 
}

template <typename T>
BE_FORCE_INLINE T *atomic_cmpxchg(T *volatile *value, const T *input, T *comparand) {  
    return (T *)atomic_cmpxchg((int64_t *)value, (int64_t)input, (int64_t)comparand); 
}

#else // !defined(__X86_64__) && !defined(__ARM64__)

template <typename T>
BE_FORCE_INLINE T *atomic_xchg(T *volatile *value, const T *input) {  
    return (T *)atomic_xchg((int32_t *)value, (int32_t)input); 
}

template <typename T>
BE_FORCE_INLINE T *atomic_cmpxchg(T *volatile *value, const T *input, T *comparand) {  
    return (T *)atomic_cmpxchg((int32_t *)value, (int32_t)input, (int32_t)comparand); 
}

#endif // !defined(__X86_64__) && !defined(__ARM64__)

#if defined(__X86__)

BE_FORCE_INLINE uint64_t rdtsc() {
    int dummy[4];
    __cpuid(dummy, 0);
    uint64_t clock = read_tsc();
    __cpuid(dummy, 0);
    return clock;
}

#endif // defined(__X86__)

BE_FORCE_INLINE int cast_f2i(float f) {
    union { float f; int i; } v; 
    v.f = f; 
    return v.i;
}

BE_FORCE_INLINE float cast_i2f(int i) {
    union { float f; int i; } v; 
    v.i = i; 
    return v.f;
}
