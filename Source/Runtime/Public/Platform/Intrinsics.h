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

#ifdef __WIN32__

BE_FORCE_INLINE int32_t atomic_add(volatile int32_t *p, int32_t v) {
    return _InterlockedExchangeAdd((volatile long *)p, v);
}

BE_FORCE_INLINE int32_t atomic_xchg(volatile int32_t *p, int32_t v) {
    return _InterlockedExchange((volatile long *)p, v);
}

BE_FORCE_INLINE int32_t atomic_cmpxchg(volatile int32_t *p, int32_t v, int32_t c) {
    return _InterlockedCompareExchange((volatile long *)p, v, c);
}

#if defined(__X86_64__) || defined(__ARM64__)

BE_FORCE_INLINE int64_t atomic_add(volatile int64_t *p, int64_t v) {
    return _InterlockedExchangeAdd64(p, v);
}

BE_FORCE_INLINE int64_t atomic_xchg(volatile int64_t *p, int64_t v) {
    return _InterlockedExchange64(p, v);
}

BE_FORCE_INLINE int64_t atomic_cmpxchg(volatile int64_t *p, int64_t v, int64_t c) {
    return _InterlockedCompareExchange64(p, v, c);
}

#endif // !defined(__X86_64__) && !defined(__ARM64__)

#pragma intrinsic(_BitScanReverse)
#pragma intrinsic(_BitScanForward)

// Computes the base 2 logarithm for an integer value that is greater than 0.
// The result is rounded down to the nearest integer.
BE_INLINE uint32_t FloorLog2(uint32_t value) {
    // Use BSR to return the log2 of the integer
    DWORD log2;
    if (_BitScanReverse(&log2, value) != 0) {
        return log2;
    }
    return 0;
}

// Counts the number of leading zeros in the bit representation of the value
BE_INLINE uint32_t CountLeadingZeros(uint32_t value) {
    // Use BSR to return the log2 of the integer
    DWORD log2;
    if (_BitScanReverse(&log2, value) != 0) {
        return 31 - log2;
    }
    return 32;
}

// Counts the number of trailing zeros in the bit representation of the value.
BE_INLINE uint32_t CountTrailingZeros(uint32_t value) {
    if (value == 0) {
        return 32;
    }
    uint32_t bitIndex;    // 0-based, where the LSB is 0 and MSB is 31
    _BitScanForward((::DWORD *)&bitIndex, value);   // Scans from LSB to MSB
    return bitIndex;
}

#endif

////////////////////////////////////////////////////////////////////////////////
/// Unix Platform
////////////////////////////////////////////////////////////////////////////////

#ifdef __UNIX__

#if defined(__X86__)

BE_FORCE_INLINE void __cpuid(int out[4], int op) {
    asm volatile ("cpuid" : "=a"(out[0]), "=b"(out[1]), "=c"(out[2]), "=d"(out[3]) : "a"(op));
}

BE_FORCE_INLINE uint64_t read_tsc()  {
    uint32_t high, low;
    asm volatile ("rdtsc" : "=d"(high), "=a"(low));
    return (((uint64_t)high) << 32) + (uint64_t)low;
}

#endif // defined(__X86__)

BE_FORCE_INLINE int32_t atomic_add(int32_t volatile *value, int32_t input) {
    return __sync_fetch_and_add(value, input);
}

BE_FORCE_INLINE int32_t atomic_xchg(int32_t volatile *value, int32_t input) {
    return __sync_lock_test_and_set(value, input);
}

BE_FORCE_INLINE int32_t atomic_cmpxchg(int32_t volatile *value, int32_t input, int32_t comparand) {
    return __sync_val_compare_and_swap(value, comparand, input);
}

#if defined(__X86_64__) || defined(__ARM64__)

BE_FORCE_INLINE int64_t atomic_add(int64_t volatile *value, int64_t input) {
    return __sync_fetch_and_add(value, input);
}

BE_FORCE_INLINE int64_t atomic_xchg(int64_t volatile *value, int64_t input) {
    return __sync_lock_test_and_set(value, input);
}

BE_FORCE_INLINE int64_t atomic_cmpxchg(int64_t volatile *value, int64_t input, int64_t comparand) {
    return __sync_val_compare_and_swap(value, comparand, input);
}

#endif // !defined(__X86_64__) && !defined(__ARM64__)

BE_INLINE uint32_t FloorLog2(uint32_t value) {
    uint32_t pos = 0;
    if (value >= 1 << 16) { value >>= 16; pos += 16; }
    if (value >= 1 << 8) { value >>= 8; pos += 8; }
    if (value >= 1 << 4) { value >>= 4; pos += 4; }
    if (value >= 1 << 2) { value >>= 2; pos += 2; }
    if (value >= 1 << 1) { pos += 1; }
    return (value == 0) ? 0 : pos;
}

BE_INLINE uint32_t CountLeadingZeros(uint32_t value) {
    if (value == 0) {
        return 32;
    }
    return __builtin_clz(value);
}

BE_INLINE uint32_t CountTrailingZeros(uint32_t value) {
    if (value == 0) {
        return 32;
    }
    return __builtin_ctz(value);
}

#endif // __UNIX__
