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
#include "SIMD/SIMD.h"

BE_NAMESPACE_BEGIN

SIMDProcessor *     simdGeneric = nullptr;     // pointer to generic SIMD implementation
SIMDProcessor *     simdProcessor = nullptr;

void SIMD::Init(bool forceGeneric) {
    simdGeneric = new SIMD_Generic;
        
    int cpuid = GetCpuInfo()->cpuid;

    if (forceGeneric) {
        simdProcessor = simdGeneric;
#ifdef HAVE_X86_AVX_INTRIN
    } else if ((cpuid & CPUID_MMX) && (cpuid & CPUID_SSE) && (cpuid & CPUID_SSE2) && (cpuid & CPUID_SSE3) && (cpuid & CPUID_SSSE3) && 
        (cpuid & CPUID_SSE4_1) && (cpuid & CPUID_SSE4_2) && (cpuid & CPUID_AVX)) {
        simdProcessor = new SIMD_8(CPUID_AVX);
#endif
#ifdef HAVE_X86_SSE_INTRIN
    } else if ((cpuid & CPUID_MMX) && (cpuid & CPUID_SSE) && (cpuid & CPUID_SSE2) && (cpuid & CPUID_SSE3) && (cpuid & CPUID_SSSE3)) {
        simdProcessor = new SIMD_4(CPUID_SSSE3);
#endif
#ifdef HAVE_ARM_NEON_INTRIN
    } else if ((cpuid & CPUID_NEON)) {
        simdProcessor = new SIMD_4(CPUID_NEON);
#endif
    } else {
        simdProcessor = simdGeneric;
    }

    BE_LOG("using %s for SIMD processing\n", simdProcessor->GetName());

#ifdef HAVE_X86_SSE_INTRIN
    if (cpuid & CPUID_FTZ) {
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        BE_LOG("enabled Flush-To-Zero mode\n");
    }
    if (cpuid & CPUID_DAZ) {
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
        BE_LOG("enabled Denormals-Are-Zero mode\n");
    }
#elif defined(HAVE_ARM_NEON_INTRIN)
    static constexpr uint64_t FP_FZ = 1 << 24;
    uint64_t val;
    asm volatile("mrs %0, fpcr" : "=r" (val));
    val |= FP_FZ;
    asm volatile("msr fpcr, %0" : /* no output */ : "r" (val));
#endif
}

void SIMD::Shutdown() {
    if (simdProcessor != simdGeneric) {
        delete simdProcessor;
        simdProcessor = nullptr;
    }
    
    delete simdGeneric;
    simdGeneric = nullptr;
}

BE_NAMESPACE_END
