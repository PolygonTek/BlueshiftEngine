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

#if defined(__ANDROID__)
#include <cpu-features.h>
#endif

BE_NAMESPACE_BEGIN

#define VENDOR_INTEL    "GenuineIntel"
#define VENDOR_AMD      "AuthenticAMD"

static CpuInfo cpuInfo;

const CpuInfo *GetCpuInfo() {
    return &cpuInfo;
}

void DetectCpu() {
    memset(&cpuInfo, 0, sizeof(cpuInfo));

#if defined(__WIN32__) || (defined(__X86__) && defined(__UNIX__))
    // __cpuid() stores the supported features and CPU information in cpuinfo,
    // an array of four 32-bit integers that is filled with the values of the EAX, EBX, ECX, and EDX registers.
    int cpuinfo_0[4];
    __cpuid(cpuinfo_0, 0);
    // Maximum value for the InfoType parameter that will return valid information.
    //unsigned int infoTypeNum = cpuinfo[0];
    // Identification string (part 1)
    *((int *)(cpuInfo.vendorString)) = cpuinfo_0[1];
    // Identification string (part 2)
    *((int *)(cpuInfo.vendorString + 4)) = cpuinfo_0[3];
    // Identification string (part 3)
    *((int *)(cpuInfo.vendorString + 8)) = cpuinfo_0[2];

    int cpuinfo_1[4];
    __cpuid(cpuinfo_1, 1);
    cpuInfo.steppingID = cpuinfo_1[0] & 0xf;
    cpuInfo.model = (cpuinfo_1[0] >> 4) & 0xf;
    cpuInfo.family = (cpuinfo_1[0] >> 8) & 0xf;
    cpuInfo.processorType = (cpuinfo_1[0] >> 12) & 0x3;
    cpuInfo.extendedModel = (cpuinfo_1[0] >> 16) & 0xf;
    cpuInfo.extendedFamily = (cpuinfo_1[0] >> 20) & 0xff;
    cpuInfo.brandIndex = cpuinfo_1[1] & 0xff;
    cpuInfo.CLFLUSHCacheLineSize = ((cpuinfo_1[1] >> 8) & 0xff) * 8;
    cpuInfo.numLogicalProcessors = ((cpuinfo_1[1] >> 16) & 0xff);
    cpuInfo.APICPhysicalID = (cpuinfo_1[1] >> 24) & 0xff;

    cpuInfo.cpuid = CPUID_NONE;

    if (cpuinfo_1[3] & BIT(23)) {
        cpuInfo.cpuid |= CPUID_MMX;
    }

    if (cpuinfo_1[3] & BIT(15)) {
        cpuInfo.cpuid |= CPUID_CMOV;
    }

    if (cpuinfo_1[3] & BIT(28)) {
        cpuInfo.cpuid |= CPUID_HTT;
    }

    if (cpuinfo_1[3] & BIT(25)) {
        cpuInfo.cpuid |= CPUID_SSE;
        cpuInfo.cpuid |= CPUID_FTZ;

        if (cpuinfo_1[3] & BIT(26)) {
            cpuInfo.cpuid |= CPUID_SSE2;
        }
    }

    if (cpuinfo_1[2] & BIT(0)) {
        cpuInfo.cpuid |= CPUID_SSE3;
        cpuInfo.cpuid |= CPUID_DAZ;

        if (cpuinfo_1[2] & BIT(9)) {
            cpuInfo.cpuid |= CPUID_SSSE3;

            if (cpuinfo_1[2] & BIT(19)) {
                cpuInfo.cpuid |= CPUID_SSE4_1;

                if (cpuinfo_1[2] & BIT(20)) {
                    cpuInfo.cpuid |= CPUID_SSE4_2;

                    if (cpuinfo_1[2] & BIT(23)) {
                        cpuInfo.cpuid |= CPUID_POPCNT;
                    }
                }
            }
        }
    }

    // Checking for AVX requires 3 things:
    // 1) CPUID indicates that the OS uses XSAVE and XRSTORE instructions (allowing saving YMM registers on context switch).
    // 2) CPUID indicates support for AVX.
    // 3) XGETBV indicates the AVX registers will be saved and restored on context switch.
    //
    // Note that XGETBV is only available on 686 or later CPUs, so the instruction needs to be conditionally run.
    bool osUsesXSAVE_XRSTORE = cpuinfo_1[2] & BIT(27) || false;
    bool cpuAVXSupport = cpuinfo_1[2] & BIT(28) || false;
    //bool cpuAVX2Support = cpuinfo_1[1] & BIT(5) || false;

    if (osUsesXSAVE_XRSTORE && cpuAVXSupport) {
        // Check if the OS will save the YMM registers.
        unsigned long long xcrFeatureMask = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
        if (xcrFeatureMask & 0x6) {
            cpuInfo.cpuid |= CPUID_AVX;
        }
    }

    if (cpuInfo.cpuid & CPUID_AVX) {
        int cpuinfo_7[4];
        __cpuid(cpuinfo_7, 7);

        if (cpuinfo_7[1] & BIT(5)) {
            cpuInfo.cpuid |= CPUID_AVX2;

            if (cpuinfo_7[1] & BIT(16)) {
                cpuInfo.cpuid |= CPUID_AVX512F;
            }

            if (cpuinfo_7[1] & BIT(28)) {
                cpuInfo.cpuid |= CPUID_AVX512CD;
            }

            if (cpuinfo_7[1] & BIT(27)) {
                cpuInfo.cpuid |= CPUID_AVX512ER;
            }

            if (cpuinfo_7[1] & BIT(26)) {
                cpuInfo.cpuid |= CPUID_AVX512PF;
            }
        }
    }
        
    if (!strncmp(cpuInfo.vendorString, VENDOR_INTEL, 12)) {
        cpuInfo.cpuid |= CPUID_INTEL;

        if (cpuInfo.family <= 4) {
            // Unsupported 386/486 processor.
            cpuInfo.cpuid |= CPUID_UNSUPPORTED;
        }
    } else if (!strncmp(cpuInfo.vendorString, VENDOR_AMD, 12)) {
        cpuInfo.cpuid |= CPUID_AMD;

        int cpuinfo_ext0[4];
        __cpuid(cpuinfo_ext0, 0x80000000);
        unsigned int nExtIds = cpuinfo_ext0[0];
        if (nExtIds > 0) {
            int cpuinfo_ext1[4];
            __cpuid(cpuinfo_ext1, 0x80000001);

            if (cpuinfo_ext1[3] & BIT(31)) {
                cpuInfo.cpuid |= CPUID_3DNOW;
            }

            int cpuinfo_ext2[4];
            __cpuid(cpuinfo_ext2, 0x80000002);
            *((int *)(cpuInfo.brandString)) = cpuinfo_ext2[0];
            *((int *)(cpuInfo.brandString + 4)) = cpuinfo_ext2[1];
            *((int *)(cpuInfo.brandString + 8)) = cpuinfo_ext2[2];
            *((int *)(cpuInfo.brandString + 12)) = cpuinfo_ext2[3];

            int cpuinfo_ext3[4];
            __cpuid(cpuinfo_ext3, 0x80000003);
            *((int *)(cpuInfo.brandString + 16)) = cpuinfo_ext3[0];
            *((int *)(cpuInfo.brandString + 20)) = cpuinfo_ext3[1];
            *((int *)(cpuInfo.brandString + 24)) = cpuinfo_ext3[2];
            *((int *)(cpuInfo.brandString + 28)) = cpuinfo_ext3[3];

            int cpuinfo_ext4[4];
            __cpuid(cpuinfo_ext4, 0x80000004);
            *((int *)(cpuInfo.brandString + 32)) = cpuinfo_ext4[0];
            *((int *)(cpuInfo.brandString + 36)) = cpuinfo_ext4[1];
            *((int *)(cpuInfo.brandString + 40)) = cpuinfo_ext4[2];
            *((int *)(cpuInfo.brandString + 44)) = cpuinfo_ext4[3];
        }

        if (cpuInfo.family <= 4) {
            // Unsupported 386/486 processor.
            cpuInfo.cpuid |= CPUID_UNSUPPORTED;
        }
    } else {
        cpuInfo.cpuid |= CPUID_GENERIC;
    }
#elif defined(__ANDROID__)
    AndroidCpuFamily family = android_getCpuFamily();
    uint64_t features = android_getCpuFeatures();

    if (family == ANDROID_CPU_FAMILY_ARM || family == ANDROID_CPU_FAMILY_ARM64) {
        cpuInfo.cpuid |= CPUID_ARM;

        if (features & ANDROID_CPU_ARM_FEATURE_NEON) {
            cpuInfo.cpuid |= CPUID_NEON;
        }
        if (features & ANDROID_CPU_ARM_FEATURE_NEON_FMA) {
            cpuInfo.cpuid |= CPUID_NEON_FMA;
        }
    } else if (family == ANDROID_CPU_FAMILY_X86 || family == ANDROID_CPU_FAMILY_X86_64) {
        cpuInfo.cpuid |= CPUID_GENERIC;

        if (features & ANDROID_CPU_X86_FEATURE_SSSE3) {
            cpuInfo.cpuid |= (CPUID_MMX | CPUID_SSE | CPUID_SSE2 | CPUID_SSE3);
        }
        if (features & ANDROID_CPU_X86_FEATURE_SSE4_1) {
            cpuInfo.cpuid |= CPUID_SSE4_1;
        }
        if (features & ANDROID_CPU_X86_FEATURE_SSE4_2) {
            cpuInfo.cpuid |= CPUID_SSE4_2;
        }
        if (features & ANDROID_CPU_X86_FEATURE_POPCNT) {
            cpuInfo.cpuid |= CPUID_POPCNT;
        }
        if (features & ANDROID_CPU_X86_FEATURE_AVX) {
            cpuInfo.cpuid |= CPUID_AVX;
        }
        if (features & ANDROID_CPU_X86_FEATURE_AVX2) {
            cpuInfo.cpuid |= CPUID_AVX2;
        }
    } else {
        cpuInfo.cpuid |= CPUID_UNSUPPORTED;
    }
#elif defined(__IOS__)
    #if defined(__ARM__) && defined(__NEON__)
        cpuInfo.cpuid |= CPUID_ARM;
        cpuInfo.cpuid |= CPUID_NEON;
    #endif
#else
    cpuInfo.cpuid |= CPUID_UNSUPPORTED;
#endif
}

BE_NAMESPACE_END
