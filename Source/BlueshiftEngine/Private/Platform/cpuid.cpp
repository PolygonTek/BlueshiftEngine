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
#include "Platform/Intrinsics.h"
#include "Platform/cpuid.h"

BE_NAMESPACE_BEGIN

#define VENDOR_INTEL    "GenuineIntel"
#define VENDOR_AMD      "AuthenticAMD"

// Reference: http://msdn.microsoft.com/en-us/library/hskdteyh(v=vs.100).aspx
static CpuInfo cpuInfo;

const CpuInfo *GetCpuInfo() {
    return &cpuInfo;
}

void DetectCpu() {
    memset(&cpuInfo, 0, sizeof(cpuInfo));
    
#if defined(__X86__)
    int info[4] = { -1, };
    __cpuid(info, 0);
    //unsigned int nIds = info[0];
    *((int *)(cpuInfo.vendorString)) = info[1];
    *((int *)(cpuInfo.vendorString + 4)) = info[3];
    *((int *)(cpuInfo.vendorString + 8)) = info[2];

    __cpuid(info, 1);
    cpuInfo.steppingID = info[0] & 0xf;
    cpuInfo.model = (info[0] >> 4) & 0xf;
    cpuInfo.family = (info[0] >> 8) & 0xf;
    cpuInfo.processorType = (info[0] >> 12) & 0x3;
    cpuInfo.extendedModel = (info[0] >> 16) & 0xf;
    cpuInfo.extendedFamily = (info[0] >> 20) & 0xff;
    cpuInfo.brandIndex = info[1] & 0xff;
    cpuInfo.CLFLUSHCacheLineSize = ((info[1] >> 8) & 0xff) * 8;
    cpuInfo.numLogicalProcessors = ((info[1] >> 16) & 0xff);
    cpuInfo.APICPhysicalID = (info[1] >> 24) & 0xff;

    cpuInfo.cpuid = CPUID_NONE;

    if (info[3] & BIT(23)) {
        cpuInfo.cpuid |= CPUID_MMX;
    }

    if (info[3] & BIT(15)) {
        cpuInfo.cpuid |= CPUID_CMOV;
    }

    if (info[3] & BIT(28)) {
        cpuInfo.cpuid |= CPUID_HTT;
    }

    if (info[3] & BIT(25)) {
        cpuInfo.cpuid |= CPUID_SSE;
        cpuInfo.cpuid |= CPUID_FTZ;
                
        if (info[3] & BIT(26)) {
            cpuInfo.cpuid |= CPUID_SSE2;
        }
    }

    if (info[2] & BIT(0)) {
        cpuInfo.cpuid |= CPUID_SSE3;
        cpuInfo.cpuid |= CPUID_DAZ;		

        if (info[2] & BIT(19)) {
            cpuInfo.cpuid |= CPUID_SSE4;

            if (info[2] & BIT(20)) {
                cpuInfo.cpuid |= CPUID_SSE42;
            }
        }
    }

    // If Visual Studio 2010 SP1 or later
#if (_MSC_FULL_VER >= 160040219)
    // Checking for AVX requires 3 things:
    // 1) CPUID indicates that the OS uses XSAVE and XRSTORE
    //     instructions (allowing saving YMM registers on context
    //     switch)
    // 2) CPUID indicates support for AVX
    // 3) XGETBV indicates the AVX registers will be saved and
    //     restored on context switch
    //
    // Note that XGETBV is only available on 686 or later CPUs, so
    // the instruction needs to be conditionally run.
    bool osUsesXSAVE_XRSTORE = info[2] & BIT(27) || false;
    bool cpuAVXSupport = info[2] & BIT(28) || false;

    if (osUsesXSAVE_XRSTORE && cpuAVXSupport) {
        // Check if the OS will save the YMM registers
        unsigned long long xcrFeatureMask = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
        if (xcrFeatureMask & 0x6) {
            cpuInfo.cpuid |= CPUID_AVX;
        }
    }
#endif

    if (!strncmp(cpuInfo.vendorString, VENDOR_INTEL, 12)) {
        cpuInfo.cpuid |= CPUID_INTEL;

        if (cpuInfo.family <= 4) {
            // unsupported 386/486 processor
            cpuInfo.cpuid |= CPUID_UNSUPPORTED;
        }
    } else if (!strncmp(cpuInfo.vendorString, VENDOR_AMD, 12)) {
        cpuInfo.cpuid |= CPUID_AMD;

        __cpuid(info, 0x80000000);
        unsigned int nExtIds = info[0];
        if (nExtIds > 0) {
            __cpuid(info, 0x80000001);

            if (info[3] & BIT(31)) {
                cpuInfo.cpuid |= CPUID_3DNOW;
            }

            __cpuid(info, 0x80000002);
            *((int *)(cpuInfo.brandString)) = info[0];
            *((int *)(cpuInfo.brandString + 4)) = info[1];
            *((int *)(cpuInfo.brandString + 8)) = info[2];
            *((int *)(cpuInfo.brandString + 12)) = info[3];

            __cpuid(info, 0x80000003);
            *((int *)(cpuInfo.brandString + 16)) = info[0];
            *((int *)(cpuInfo.brandString + 20)) = info[1];
            *((int *)(cpuInfo.brandString + 24)) = info[2];
            *((int *)(cpuInfo.brandString + 28)) = info[3];

            __cpuid(info, 0x80000004);
            *((int *)(cpuInfo.brandString + 32)) = info[0];
            *((int *)(cpuInfo.brandString + 36)) = info[1];
            *((int *)(cpuInfo.brandString + 40)) = info[2];
            *((int *)(cpuInfo.brandString + 44)) = info[3];
        }

        if (cpuInfo.family <= 4) {
            // unsupported 386/486 processor
            cpuInfo.cpuid |= CPUID_UNSUPPORTED;
        }
    } else {
        cpuInfo.cpuid |= CPUID_GENERIC;
    }
#else
    cpuInfo.cpuid |= CPUID_GENERIC;
#endif
}

BE_NAMESPACE_END
