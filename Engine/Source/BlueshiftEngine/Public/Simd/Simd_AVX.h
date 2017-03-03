#pragma once

//#include "AVX/avx.h"

BE_NAMESPACE_BEGIN

typedef sseb simd8b;
typedef ssef simd8f;
typedef ssei simd8i;

class SIMD_AVX : public SIMD_SSE4 {
public:
    SIMD_AVX() { cpuid = CPUID_AVX; }

    virtual const char * BE_FASTCALL    GetName() const { return "SSE4 & AVX"; }
};

BE_NAMESPACE_END
