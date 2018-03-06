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
#include "Core/Str.h"
#include "PlatformUtils/Android/AndroidGPUInfo.h"

BE_NAMESPACE_BEGIN

AndroidGPUInfo AndroidGPUInfo::GetFromOpenGLRendererString(const char *renderer) {
    AndroidGPUInfo gpuInfo;
    gpuInfo.processor = Processor::Unknown;
    gpuInfo.model = 0;

    if (renderer && renderer[0]) {
        switch (renderer[0]) {
        case 'A':
            if (sscanf(renderer, "Adreno (TM) %" SCNu32, &gpuInfo.model) == 1 && gpuInfo.model != 0) {
                gpuInfo.processor = Processor::Qualcomm_Adreno;
            }
            break;
        case 'G':
            if (sscanf(renderer, "GC%" SCNu32" core", &gpuInfo.model) == 1 && gpuInfo.model != 0) {
                gpuInfo.processor = Processor::Vivante_GC;
            }
            break;
        case 'I':
            if (strcmp(renderer, "Immersion.16") == 0) {
                /* Vivante GC4000 in early HiSilicon chipsets */
                gpuInfo.processor = Processor::Vivante_GC;
                gpuInfo.model = 4000;
            } else if (strcmp(renderer, "Intel(R) HD Graphics for BayTrail") == 0) {
                gpuInfo.processor = Processor::Intel_Gen;
                gpuInfo.model = 7;
            } else if (strcmp(renderer, "Intel(R) HD Graphics for Atom(TM) x5/x7") == 0) {
                gpuInfo.processor = Processor::Intel_Gen;
                gpuInfo.model = 8;
            }
            break;
        case 'M':
            if (sscanf(renderer, "Mali-%" SCNu32, &gpuInfo.model) == 1 && gpuInfo.model != 0) {
                gpuInfo.processor = Processor::ARM_Mali;
            } else if (sscanf(renderer, "Mali-T%" SCNu32, &gpuInfo.model) == 1 && gpuInfo.model != 0) {
                gpuInfo.processor = Processor::ARM_MaliT;
            } else if (sscanf(renderer, "Mali-G%" SCNu32, &gpuInfo.model) == 1 && gpuInfo.model != 0) {
                gpuInfo.processor = Processor::ARM_MaliG;
            } else if (sscanf(renderer, "Mesa DRI Intel(R) HD Graphics %" SCNu32, &gpuInfo.model) == 1 && gpuInfo.model != 0) {
                gpuInfo.processor = Processor::Intel_HDGraphics;
            } else if (strcmp(renderer, "Mesa DRI Intel(R) Sandybridge Mobile x86/MMX/SSE2") == 0) {
                gpuInfo.processor = Processor::Intel_Gen;
                gpuInfo.model = 6;
            }
            break;
        case 'N':
            if (strcmp(renderer, "NVIDIA Tegra 3") == 0) {
                gpuInfo.processor = Processor::Nvidia_Tegra;
                gpuInfo.model = 3;
            } else if (strcmp(renderer, "NVIDIA Tegra") == 0) {
                gpuInfo.processor = Processor::Nvidia_Tegra;
                gpuInfo.model = 0;
            }
            break;
        case 'P':
            if (strcmp(renderer, "PXA1928 GPU") == 0) {
                /* Vivante GC5000 in PXA1928 chipset */
                gpuInfo.processor = Processor::Vivante_GC;
                gpuInfo.model = 5000;
            } else if (sscanf(renderer, "PowerVR SGX %" SCNu32, &gpuInfo.model) == 1 && gpuInfo.model != 0) {
                gpuInfo.processor = Processor::PowerVR_SGX;
            } else if (sscanf(renderer, "PowerVR Rogue G%" SCNu32, &gpuInfo.model) == 1 && gpuInfo.model != 0) {
                gpuInfo.processor = Processor::PowerVR_RogueG;
            } else if (sscanf(renderer, "PowerVR Rogue GE%" SCNu32, &gpuInfo.model) == 1 && gpuInfo.model != 0) {
                gpuInfo.processor = Processor::PowerVR_RogueGE;
            } else if (sscanf(renderer, "PowerVR Rogue GM%" SCNu32, &gpuInfo.model) == 1 && gpuInfo.model != 0) {
                gpuInfo.processor = Processor::PowerVR_RogueGM;
            } else if (sscanf(renderer, "PowerVR Rogue GT%" SCNu32, &gpuInfo.model) == 1 && gpuInfo.model != 0) {
                gpuInfo.processor = Processor::PowerVR_RogueGT;
            } else if (sscanf(renderer, "PowerVR Rogue GX%" SCNu32, &gpuInfo.model) == 1 && gpuInfo.model != 0) {
                gpuInfo.processor = Processor::PowerVR_RogueGX;
            } else if (strcmp(renderer, "PowerVR Rogue Han") == 0) {
                gpuInfo.processor = Processor::PowerVR_RogueG;
                gpuInfo.model = 6200;
            } else if (strcmp(renderer, "PowerVR Rogue Hood") == 0) {
                gpuInfo.processor = Processor::PowerVR_RogueG;
                gpuInfo.model = 6430;
            } else if (strcmp(renderer, "PowerVR Rogue Marlowe") == 0) {
                gpuInfo.processor = Processor::PowerVR_RogueGT;
                gpuInfo.model = 7400;
            }
            break;
        case 'S':
            if (strcmp(renderer, "Sapphire 650") == 0) {
                /* ARM Mali-T720 in some MT8163 chipsets */
                gpuInfo.processor = Processor::ARM_MaliT;
                gpuInfo.model = 720;
            }
            break;
        case 'V':
            if (strcmp(renderer, "VideoCore IV HW") == 0) {
                gpuInfo.processor = Processor::Broadcom_VideoCore;
                gpuInfo.model = 4;
            } else if (sscanf(renderer, "Vivante GC%" SCNu32, &gpuInfo.model) == 2 && gpuInfo.model != 0) {
                gpuInfo.processor = Processor::Vivante_GC;
            }
            break;
        }
    }
    return gpuInfo;
}

BE_NAMESPACE_END