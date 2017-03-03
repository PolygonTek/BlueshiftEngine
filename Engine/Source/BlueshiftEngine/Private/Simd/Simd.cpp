#include "Precompiled.h"
#include "SIMD/Simd.h"
#include "SIMD/Simd_Generic.h"

#if defined(__X86__)

#include "SIMD/Simd_SSE4.h"
#include "SIMD/Simd_AVX.h"

#endif

BE_NAMESPACE_BEGIN

SIMDProcessor *     simdGeneric = nullptr;     // pointer to generic SIMD implementation
SIMDProcessor *     simdProcessor = nullptr;

void SIMD::Init(bool forceGeneric) {
    simdGeneric = new SIMD_Generic;
        
    int cpuid = GetCpuInfo()->cpuid;

    if (forceGeneric) {
        simdProcessor = simdGeneric;
    } else {
#if defined(__X86__)
        if ((cpuid & CPUID_MMX) && 
            (cpuid & CPUID_SSE) && 
            (cpuid & CPUID_SSE2) && 
            (cpuid & CPUID_SSE3) && 
            (cpuid & CPUID_SSE4) && 
            (cpuid & CPUID_AVX)) {
            simdProcessor = new SIMD_AVX;
        } else if ((cpuid & CPUID_MMX) && 
            (cpuid & CPUID_SSE) && 
            (cpuid & CPUID_SSE2) && 
            (cpuid & CPUID_SSE3) && 
            (cpuid & CPUID_SSE4)) {
            simdProcessor = new SIMD_SSE4;
        } else {
            simdProcessor = simdGeneric;
        }
#elif defined(__ARM__)
        simdProcessor = simdGeneric;
#else
        simdProcessor = simdGeneric;
#endif
    }

    BE_LOG(L"using %hs for SIMD processing\n", simdProcessor->GetName());

#if defined(__X86__)
    if (cpuid & CPUID_FTZ) {
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        BE_LOG(L"enabled Flush-To-Zero mode\n");
    }

    if (cpuid & CPUID_DAZ) {
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);	
        BE_LOG(L"enabled Denormals-Are-Zero mode\n");
    }
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
