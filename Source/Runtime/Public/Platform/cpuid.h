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
    CPUID_NONE                  = 0x00000000,
    CPUID_UNSUPPORTED           = 0x00000001,  ///< unsupported (386/486)
    CPUID_GENERIC               = 0x00000002,  ///< unrecognized processor
    CPUID_INTEL                 = 0x00000004,  ///< Intel
    CPUID_AMD                   = 0x00000008,  ///< AMD
    CPUID_ARM                   = 0x00000010,  ///< ARM
    CPUID_HTT                   = 0x00000100,  ///< Hyper-Threading Technology
    CPUID_CMOV                  = 0x00000200,  ///< Conditional Move (CMOV) and fast floating point comparison (FCOMI) instructions
    CPUID_FTZ                   = 0x00000400,  ///< Flush-To-Zero mode (denormal results are flushed to zero)
    CPUID_DAZ                   = 0x00000800,  ///< Denormals-Are-Zero mode (denormal source operands are set to zero)
    CPUID_MMX                   = 0x00001000,  ///< Multi Media Extensions
    CPUID_3DNOW                 = 0x00002000,  ///< 3DNow!
    CPUID_SSE                   = 0x00004000,  ///< Streaming SIMD Extensions
    CPUID_SSE2                  = 0x00008000,  ///< Streaming SIMD Extensions 2
    CPUID_SSE3                  = 0x00010000,  ///< Streaming SIMD Extentions 3 aka Prescott's New Instructions
    CPUID_SSSE3                 = 0x00020000,  ///< Supplemental Streaming SIMD Extentions 3
    CPUID_SSE4_1                = 0x00040000,  ///< Streaming SIMD Extensions 4.1
    CPUID_SSE4_2                = 0x00080000,  ///< Streaming SIMD Extensions 4.2
    CPUID_AVX                   = 0x00100000,  ///< Advanced Vector Extensions
    CPUID_AVX2                  = 0x00200000,  ///< Advanced Vector Extensions 2
    CPUID_NEON                  = 0x00400000   ///< ARM Neon
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
