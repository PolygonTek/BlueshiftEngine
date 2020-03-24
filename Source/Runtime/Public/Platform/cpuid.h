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
    CPUID_NONE                  = 0,
    CPUID_UNSUPPORTED           = BIT(0),   ///< unsupported (386/486)
    CPUID_GENERIC               = BIT(1),   ///< unrecognized processor
    CPUID_INTEL                 = BIT(2),   ///< Intel
    CPUID_AMD                   = BIT(3),   ///< AMD
    CPUID_ARM                   = BIT(4),   ///< ARM
    CPUID_HTT                   = BIT(5),   ///< Hyper-Threading Technology
    CPUID_CMOV                  = BIT(6),   ///< Conditional Move (CMOV) and fast floating point comparison (FCOMI) instructions
    CPUID_FTZ                   = BIT(7),   ///< Flush-To-Zero mode (denormal results are flushed to zero)
    CPUID_DAZ                   = BIT(8),   ///< Denormals-Are-Zero mode (denormal source operands are set to zero)
    CPUID_MMX                   = BIT(9),   ///< Multi Media Extensions
    CPUID_3DNOW                 = BIT(10),  ///< 3DNow!
    CPUID_SSE                   = BIT(11),  ///< Streaming SIMD Extensions
    CPUID_SSE2                  = BIT(12),  ///< Streaming SIMD Extensions 2
    CPUID_SSE3                  = BIT(13),  ///< Streaming SIMD Extentions 3 aka Prescott's New Instructions
    CPUID_SSSE3                 = BIT(14),  ///< Supplemental Streaming SIMD Extentions 3
    CPUID_SSE4_1                = BIT(15),  ///< Streaming SIMD Extensions 4.1
    CPUID_SSE4_2                = BIT(16),  ///< Streaming SIMD Extensions 4.2
    CPUID_POPCNT                = BIT(17),  ///< Population Count instruction
    CPUID_AVX                   = BIT(18),  ///< Advanced Vector Extensions
    CPUID_AVX2                  = BIT(19),  ///< Advanced Vector Extensions 2
    CPUID_AVX512F               = BIT(20),  ///< AVX-512 Foundation
    CPUID_AVX512CD              = BIT(21),  ///< AVX-512 Conflict Detection Instructions
    CPUID_AVX512ER              = BIT(22),  ///< AVX-512 Exponential and Reciprocal Instructions
    CPUID_AVX512PF              = BIT(23),  ///< AVX-512 Prefetch Instructions
    CPUID_NEON                  = BIT(24),  ///< ARM Neon
    CPUID_NEON_FMA              = BIT(25)   ///< ARM Neon FMA
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
