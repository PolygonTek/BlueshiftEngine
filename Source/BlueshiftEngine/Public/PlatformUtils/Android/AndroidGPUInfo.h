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

class AndroidGPUInfo {
public:
    enum class Processor {
        Unknown,
        ARM_Mali,
        ARM_MaliT,
        ARM_MaliG,
        Broadcom_VideoCore,
        Intel_HDGraphics,
        Intel_Gen,
        Nvidia_Tegra,
        PowerVR_SGX,
        PowerVR_RogueG,     // Series 6, Series 6XE
        PowerVR_RogueGX,    // Series 6XT
        PowerVR_RogueGE,    // Series 7XE, Series 8XE, Series 9XE
        PowerVR_RogueGT,    // Series 7XT, Series 8XT
        PowerVR_RogueGM,    // Series 9XM
        Qualcomm_Adreno,
        Vivante_GC
    };

    Processor processor;
    int model;

    static AndroidGPUInfo GetFromOpenGLRendererString(const char *glrendererString);
};

BE_NAMESPACE_END
