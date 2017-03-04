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

BE_NAMESPACE_BEGIN

//--------------------------------------------------------------------------------------------------
// cpuid
//--------------------------------------------------------------------------------------------------
enum CpuId {
    CPUID_NONE                  = 0x00000,
    CPUID_UNSUPPORTED           = 0x00001,  ///< unsupported (386/486)
    CPUID_GENERIC               = 0x00002,  ///< unrecognized processor
    CPUID_INTEL                 = 0x00004,  ///< Intel
    CPUID_AMD                   = 0x00008,  ///< AMD
    CPUID_HTT                   = 0x00010,  ///< Hyper-Threading Technology
    CPUID_CMOV                  = 0x00020,  ///< Conditional Move (CMOV) and fast floating point comparison (FCOMI) instructions
    CPUID_FTZ                   = 0x00040,  ///< Flush-To-Zero mode (denormal results are flushed to zero)
    CPUID_DAZ                   = 0x00080,  ///< Denormals-Are-Zero mode (denormal source operands are set to zero)
    CPUID_MMX                   = 0x00100,  ///< Multi Media Extensions
    CPUID_3DNOW                 = 0x00200,  ///< 3DNow!
    CPUID_SSE                   = 0x00400,  ///< Streaming SIMD Extensions
    CPUID_SSE2                  = 0x00800,  ///< Streaming SIMD Extensions 2
    CPUID_SSE3                  = 0x01000,  ///< Streaming SIMD Extentions 3 aka Prescott's New Instructions
    CPUID_SSE4                  = 0x02000,  ///< Streaming SIMD Extensions 4.1
    CPUID_SSE42                 = 0x04000,  ///< Streaming SIMD Extensions 4.2
    CPUID_AVX                   = 0x08000,  ///< Advanced Vector Extensions
    CPUID_AVX2                  = 0x10000,  ///< Advanced Vector Extensions 2
    CPUID_NEON                  = 0x20000   ///< ARM Neon
};

struct CpuInfo {
    char            vendorString[16];
    char            brandString[48];
    int             steppingID;
    int             model;
    int             family;
    int             processorType;
    int             extendedModel;
    int             extendedFamily;
    int             brandIndex;
    int             CLFLUSHCacheLineSize;
    int             numLogicalProcessors;
    int             APICPhysicalID;
    int             cpuid;
};

void                DetectCpu();

const CpuInfo *     GetCpuInfo();

BE_NAMESPACE_END
