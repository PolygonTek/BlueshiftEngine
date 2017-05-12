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
#include "Simd/Simd.h"
#include "Simd/Simd_Generic.h"

#if defined(__X86__)

#include "Simd/Simd_SSE4.h"
#include "Simd/Simd_AVX.h"

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
