// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "ShaderTemplate.h"
#include "ShaderPipelineCache.h"

ShaderTemplateManager    shaderTemplateManager;

static int GetDefineNameLength(const char *define) {
    int length = 0;
    while (define[length] && define[length] != '=') {
        length++;
    }
    return length;
}

static int GetDefineNameLength(const BE1::Str &define) {
    const int equalIndex = define.Find('=');
    return equalIndex >= 0 ? equalIndex : define.Length();
}

static bool HasDefine(const BE1::StrArray &defines, const char *name, int nameLength) {
    for (const BE1::Str &define : defines) {
        const int defineNameLength = GetDefineNameLength(define);
        if (defineNameLength == nameLength && !BE1::Str::Cmpn(define.c_str(), name, nameLength)) {
            return true;
        }
    }

    return false;
}

void ShaderTemplateManager::Init() {
    shaderTemplateHashMap.Init(128, 128, 128);

    expiredShaderTemplateHashMapCursor = 0;

    globalDefinesMutex = BE1::PlatformMutex::Create();
    globalDefines.Clear();

    InitGlobalDefines();

    globalDefineModCount = 0;
    lastInvalidatedGlobalDefineModCount = 0;

    CreateEngineShaderTemplates();
}

void ShaderTemplateManager::Shutdown() {
    DestroyEngineShaderTemplates();

    shaderTemplateHashMap.Clear();
    shaderTemplateReloaded.Clear();

    {
        BE1::ScopedLock lock(globalDefinesMutex);
        globalDefines.Clear();
        globalDefineModCount = 0;
        lastInvalidatedGlobalDefineModCount = 0;
    }

    BE1::PlatformMutex::Destroy(globalDefinesMutex);
    globalDefinesMutex = nullptr;
}

void ShaderTemplateManager::UpdateFrame() {
    uint32_t globalDefineModCountSnapshot;
    {
        BE1::ScopedLock lock(globalDefinesMutex);
        globalDefineModCountSnapshot = globalDefineModCount;
    }

    // 글로벌 디파인이 변경되었다면 모든 캐시된 파이프라인을 재빌드한다.
    if (lastInvalidatedGlobalDefineModCount != globalDefineModCountSnapshot) {
        shaderPipelineCache.RebuildAll();
        lastInvalidatedGlobalDefineModCount = globalDefineModCountSnapshot;
    }

    CleanExpiredShaderTemplates();
}

SharedPtr<ShaderTemplate> ShaderTemplateManager::AllocShaderTemplate(const char *hashName) {
    if (!hashName || !hashName[0]) {
        BE_FATALERROR("ShaderTemplateManager::AllocShaderTemplate: empty shader template name");
    }
    if (const auto *entry = shaderTemplateHashMap.Get(hashName)) {
        if (!entry->second.expired()) {
            BE_FATALERROR("ShaderTemplateManager::AllocShaderTemplate: %s shader template already allocated", hashName);
        }
        shaderTemplateHashMap.Remove(hashName);
    }

    SharedPtr<ShaderTemplate> shaderTemplate = MakeShared<ShaderTemplate>();
    shaderTemplate->hashName = hashName;
    shaderTemplate->name = hashName;
    shaderTemplate->name.StripPath();
    shaderTemplate->name.StripFileExtension();
    shaderTemplateHashMap.Set(shaderTemplate->hashName, shaderTemplate);

    return shaderTemplate;
}

SharedPtr<ShaderTemplate> ShaderTemplateManager::FindShaderTemplate(const char *hashName) const {
    if (!hashName || !hashName[0]) {
        return nullptr;
    }

    const auto *entry = shaderTemplateHashMap.Get(hashName);
    if (!entry) {
        return nullptr;
    }

    return entry->second.lock();
}

SharedPtr<ShaderTemplate> ShaderTemplateManager::GetShaderTemplate(const char *hashName) {
    if (!hashName || !hashName[0]) {
        return nullptr;
    }

    SharedPtr<ShaderTemplate> shaderTemplate = FindShaderTemplate(hashName);
    if (shaderTemplate) {
        return shaderTemplate;
    }

    shaderTemplate = AllocShaderTemplate(hashName);
    bool loaded = shaderTemplate->Load(hashName);
    if (!loaded) {
        shaderTemplateHashMap.Remove(shaderTemplate->hashName);
        return nullptr;
    }

    return shaderTemplate;
}

void ShaderTemplateManager::InitGlobalDefines() {
#ifdef COMPRESSED_VERTEX_NORMAL_TANGENTS
    AddGlobalDefine("COMPRESSED_VERTEX_NORMAL_TANGENTS");
#endif

    AddGlobalDefine(BE1::va("CSM_COUNT=%i", 4));
    AddGlobalDefine(BE1::va("CASCADE_SELECTION_METHOD=%i", 1));
}

bool ShaderTemplateManager::FindGlobalDefine(const char *define) const {
    if (!define || !define[0]) {
        return false;
    }

    const int defineNameLength = GetDefineNameLength(define);

    BE1::ScopedLock lock(globalDefinesMutex);

    return HasDefine(globalDefines, define, defineNameLength);
}

void ShaderTemplateManager::AddGlobalDefine(const char *define) {
    if (!define || !define[0]) {
        return;
    }

    const int defineNameLength = GetDefineNameLength(define);

    BE1::ScopedLock lock(globalDefinesMutex);

    if (HasDefine(globalDefines, define, defineNameLength)) {
        BE_WARNLOG("ShaderTemplateManager::AddGlobalDefine: duplicated global define '%s'\n", define);
        return;
    }

    globalDefines.Append(define);
    globalDefineModCount++;
}

void ShaderTemplateManager::RemoveGlobalDefine(const char *define) {
    if (!define || !define[0]) {
        return;
    }

    const int defineNameLength = GetDefineNameLength(define);

    BE1::ScopedLock lock(globalDefinesMutex);

    const int numGlobalDefines = globalDefines.Count();
    for (int i = 0; i < numGlobalDefines; i++) {
        const int globalDefineNameLength = GetDefineNameLength(globalDefines[i]);

        if (globalDefineNameLength == defineNameLength && !BE1::Str::Cmpn(globalDefines[i].c_str(), define, defineNameLength)) {
            globalDefines.RemoveIndex(i);
            globalDefineModCount++;
            return;
        }
    }
}

uint32_t ShaderTemplateManager::CopyGlobalDefines(BE1::StrArray &outDefines) const {
    BE1::ScopedLock lock(globalDefinesMutex);

    outDefines = globalDefines;
    return globalDefineModCount;
}

void ShaderTemplateManager::CleanExpiredShaderTemplates() {
    constexpr int ExpiredShaderTemplateHashMapScanBudget = 32;
    int scansLeft = ExpiredShaderTemplateHashMapScanBudget;

    while (scansLeft-- > 0 && shaderTemplateHashMap.Count() > 0) {
        if (expiredShaderTemplateHashMapCursor >= shaderTemplateHashMap.Count()) {
            expiredShaderTemplateHashMapCursor = 0;
        }

        const auto *entry = shaderTemplateHashMap.GetByIndex(expiredShaderTemplateHashMapCursor);
        assert(entry);

        if (entry->second.expired()) {
            shaderTemplateHashMap.Remove(entry->first);
            continue;
        }

        expiredShaderTemplateHashMapCursor++;
    }
}

void ShaderTemplateManager::CreateEngineShaderTemplates() {
#define ENGINE_SHADER_TEMPLATE(...) #__VA_ARGS__

    static constexpr const char *fullScreenImageShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            vertex SM_6_0 "FullScreenTriangle.hlsl" "VSMain"
            fragment SM_6_0 "FullScreenTriangle.hlsl" "PSMain"
        }
    }
    );

    static constexpr const char *toneMappingShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "ToneMapping.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *luminancePass1ShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "LuminancePass1.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *luminancePass2ShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "LuminancePass2.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *bloomPrefilterShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "BloomPrefilter.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *convolutionBloomPrefilterShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "ConvolutionBloomPrefilter.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *convolutionBloomKernelPrepareShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "ConvolutionBloomKernelPrepare.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *convolutionBloomMultiplySpectrumShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "ConvolutionBloomMultiplySpectrum.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *convolutionBloomResolveShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "ConvolutionBloomResolve.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *fftDualReal512FirstStageShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "FFT/FFTDualReal512FirstStage_Radix8.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *fftComplex256ShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "FFT/FFTComplex256_Radix4.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *fftComplex512ShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "FFT/FFTComplex512_Radix8.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *fftComplex1024ShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "FFT/FFTComplex1024_Radix8.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *fftComplex1024LastStageShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "FFT/FFTComplex1024LastStage_Radix2.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *fftReal512FirstStageShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "FFT/FFTReal512FirstStage_Radix4.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *fftReal512LastStageShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "FFT/FFTReal512LastStage_Radix4.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *fftReal1024FirstStageShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "FFT/FFTReal1024FirstStage_Radix8.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *fftReal1024LastStageShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "FFT/FFTReal1024LastStage_Radix8.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *fftDualReal512LastStageShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "FFT/FFTDualReal512LastStage_Radix8.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *fftDualReal1024FirstStageShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "FFT/FFTDualReal1024FirstStage_Radix8.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *fftDualReal1024LastStageShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "FFT/FFTDualReal1024LastStage_Radix2.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *ifftReal512FirstStageShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "FFT/IFFTReal512FirstStage_Radix4.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *ifftReal512LastStageShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "FFT/IFFTReal512LastStage_Radix4.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *ifftReal1024FirstStageShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "FFT/IFFTReal1024FirstStage_Radix8.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *ifftReal1024LastStageShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "FFT/IFFTReal1024LastStage_Radix8.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *ifftDualReal512FirstStageShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "FFT/IFFTDualReal512FirstStage_Radix8.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *ifftDualReal512LastStageShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "FFT/IFFTDualReal512LastStage_Radix8.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *ifftDualReal1024FirstStageShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "FFT/IFFTDualReal1024FirstStage_Radix8.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *ifftDualReal1024LastStageShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "FFT/IFFTDualReal1024LastStage_Radix2.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *transposeComplex256x512ShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "Transpose/TransposeComplex256x512.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *transposeComplex512x256ShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "Transpose/TransposeComplex512x256.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *transposeComplex512x1024ShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "Transpose/TransposeComplex512x1024.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *transposeComplex1024x512ShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "Transpose/TransposeComplex1024x512.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *transposeComplex512ShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "Transpose/TransposeComplex512.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *transposeComplex1024ShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "Transpose/TransposeComplex1024.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *bloomDownsampleShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "BloomDownsample.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *bloomUpsampleShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            compute SM_6_0 "BloomUpsample.hlsl" "CSMain"
        }
    }
    );

    static constexpr const char *unlitShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            vertex SM_6_0 "Unlit.hlsl" "VSMain"
            fragment SM_6_0 "Unlit.hlsl" "PSMain"
        }

        properties {
            baseColorTexture("Base Color") : texture "2D" = "_whiteTexture"
        }

        features {
            gpuSkinningWeight1 {
                GPU_SKINNING=1
                GPU_SKINNING_WEIGHTS=1
            }

            gpuSkinningWeight4 {
                GPU_SKINNING=1
                GPU_SKINNING_WEIGHTS=4
            }

            gpuSkinningWeight8 {
                GPU_SKINNING=1
                GPU_SKINNING_WEIGHTS=8
            }
        }
    }
    );

    static constexpr const char *litShaderTemplateText = ENGINE_SHADER_TEMPLATE(
    {
        stages {
            vertex SM_6_0 "Lit.hlsl" "VSMain"
            fragment SM_6_0 "Lit.hlsl" "PSMain"
        }

        properties {
            baseColorTexture("Base Color") : texture "2D" = "_whiteTexture"
        }

        features {
            gpuSkinningWeight1 {
                GPU_SKINNING=1
                GPU_SKINNING_WEIGHTS=1
            }

            gpuSkinningWeight4 {
                GPU_SKINNING=1
                GPU_SKINNING_WEIGHTS=4
            }

            gpuSkinningWeight8 {
                GPU_SKINNING=1
                GPU_SKINNING_WEIGHTS=8
            }
        }
    }
    );

#undef ENGINE_SHADER_TEMPLATE

    const char *baseDir = "Source/TestD3D12/Shaders";

    fullscreenImageShaderTemplate = AllocShaderTemplate("_fullScreenImageShader");
    if (!fullscreenImageShaderTemplate->Create(fullScreenImageShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create fullscreen image shader template");
    }
    fullscreenImageShaderTemplate->AddPermutation();

    toneMappingShaderTemplate = AllocShaderTemplate("_toneMappingShader");
    if (!toneMappingShaderTemplate->Create(toneMappingShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create tone mapping shader template");
    }
    toneMappingShaderTemplate->AddPermutation();

    luminancePass1ShaderTemplate = AllocShaderTemplate("_luminancePass1Shader");
    if (!luminancePass1ShaderTemplate->Create(luminancePass1ShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create luminance pass 1 shader template");
    }
    luminancePass1ShaderTemplate->AddPermutation();

    luminancePass2ShaderTemplate = AllocShaderTemplate("_luminancePass2Shader");
    if (!luminancePass2ShaderTemplate->Create(luminancePass2ShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create luminance pass 2 shader template");
    }
    luminancePass2ShaderTemplate->AddPermutation();

    bloomPrefilterShaderTemplate = AllocShaderTemplate("_bloomPrefilterShader");
    if (!bloomPrefilterShaderTemplate->Create(bloomPrefilterShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create bloom prefilter shader template");
    }
    bloomPrefilterShaderTemplate->AddPermutation();

    bloomDownsampleShaderTemplate = AllocShaderTemplate("_bloomDownsampleShader");
    if (!bloomDownsampleShaderTemplate->Create(bloomDownsampleShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create bloom downsample shader template");
    }
    bloomDownsampleShaderTemplate->AddPermutation();

    bloomUpsampleShaderTemplate = AllocShaderTemplate("_bloomUpsampleShader");
    if (!bloomUpsampleShaderTemplate->Create(bloomUpsampleShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create bloom upsample shader template");
    }
    bloomUpsampleShaderTemplate->AddPermutation();

    convolutionBloomPrefilterShaderTemplate = AllocShaderTemplate("_convolutionBloomPrefilterShader");
    if (!convolutionBloomPrefilterShaderTemplate->Create(convolutionBloomPrefilterShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create convolution bloom prefilter shader template");
    }
    convolutionBloomPrefilterShaderTemplate->AddPermutation();

    convolutionBloomKernelPrepareShaderTemplate = AllocShaderTemplate("_convolutionBloomKernelPrepareShader");
    if (!convolutionBloomKernelPrepareShaderTemplate->Create(convolutionBloomKernelPrepareShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create convolution bloom kernel prepare shader template");
    }
    convolutionBloomKernelPrepareShaderTemplate->AddPermutation();

    convolutionBloomMultiplySpectrumShaderTemplate = AllocShaderTemplate("_convolutionBloomMultiplySpectrumShader");
    if (!convolutionBloomMultiplySpectrumShaderTemplate->Create(convolutionBloomMultiplySpectrumShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create convolution bloom multiply spectrum shader template");
    }
    convolutionBloomMultiplySpectrumShaderTemplate->AddPermutation();

    convolutionBloomResolveShaderTemplate = AllocShaderTemplate("_convolutionBloomResolveShader");
    if (!convolutionBloomResolveShaderTemplate->Create(convolutionBloomResolveShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create convolution bloom resolve shader template");
    }
    convolutionBloomResolveShaderTemplate->AddPermutation();

    fftComplex256ShaderTemplate = AllocShaderTemplate("_fftComplex256Shader");
    if (!fftComplex256ShaderTemplate->Create(fftComplex256ShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT complex 256 shader template");
    }
    fftComplex256ShaderTemplate->AddPermutation();

    fftComplex512ShaderTemplate = AllocShaderTemplate("_fftComplex512Shader");
    if (!fftComplex512ShaderTemplate->Create(fftComplex512ShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT512 shader template");
    }
    fftComplex512ShaderTemplate->AddPermutation();

    fftComplex1024ShaderTemplate = AllocShaderTemplate("_fftComplex1024Shader");
    if (!fftComplex1024ShaderTemplate->Create(fftComplex1024ShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT1024 shader template");
    }
    fftComplex1024ShaderTemplate->AddPermutation();

    fftComplex1024LastStageShaderTemplate = AllocShaderTemplate("_fftComplex1024LastStageShader");
    if (!fftComplex1024LastStageShaderTemplate->Create(fftComplex1024LastStageShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT1024 last stage shader template");
    }
    fftComplex1024LastStageShaderTemplate->AddPermutation();

    fftReal512FirstStageShaderTemplate = AllocShaderTemplate("_fftReal512FirstStageShader");
    if (!fftReal512FirstStageShaderTemplate->Create(fftReal512FirstStageShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT real 512 first stage shader template");
    }
    fftReal512FirstStageShaderTemplate->AddPermutation();

    fftReal512LastStageShaderTemplate = AllocShaderTemplate("_fftReal512LastStageShader");
    if (!fftReal512LastStageShaderTemplate->Create(fftReal512LastStageShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT real 512 last stage shader template");
    }
    fftReal512LastStageShaderTemplate->AddPermutation();

    fftReal1024FirstStageShaderTemplate = AllocShaderTemplate("_fftReal1024FirstStageShader");
    if (!fftReal1024FirstStageShaderTemplate->Create(fftReal1024FirstStageShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT real 1024 first stage shader template");
    }
    fftReal1024FirstStageShaderTemplate->AddPermutation();

    fftReal1024LastStageShaderTemplate = AllocShaderTemplate("_fftReal1024LastStageShader");
    if (!fftReal1024LastStageShaderTemplate->Create(fftReal1024LastStageShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT real 1024 last stage shader template");
    }
    fftReal1024LastStageShaderTemplate->AddPermutation();

    fftDualReal512FirstStageShaderTemplate = AllocShaderTemplate("_fftDualReal512FirstStageShader");
    if (!fftDualReal512FirstStageShaderTemplate->Create(fftDualReal512FirstStageShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT dual real 512 first stage shader template");
    }
    fftDualReal512FirstStageShaderTemplate->AddPermutation();

    fftDualReal512LastStageShaderTemplate = AllocShaderTemplate("_fftDualReal512LastStageShader");
    if (!fftDualReal512LastStageShaderTemplate->Create(fftDualReal512LastStageShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT dual real 512 last stage shader template");
    }
    fftDualReal512LastStageShaderTemplate->AddPermutation();

    fftDualReal1024FirstStageShaderTemplate = AllocShaderTemplate("_fftDualReal1024FirstStageShader");
    if (!fftDualReal1024FirstStageShaderTemplate->Create(fftDualReal1024FirstStageShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT dual real 1024 first stage shader template");
    }
    fftDualReal1024FirstStageShaderTemplate->AddPermutation();

    fftDualReal1024LastStageShaderTemplate = AllocShaderTemplate("_fftDualReal1024LastStageShader");
    if (!fftDualReal1024LastStageShaderTemplate->Create(fftDualReal1024LastStageShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT dual real 1024 last stage shader template");
    }
    fftDualReal1024LastStageShaderTemplate->AddPermutation();

    ifftReal512FirstStageShaderTemplate = AllocShaderTemplate("_ifftReal512FirstStageShader");
    if (!ifftReal512FirstStageShaderTemplate->Create(ifftReal512FirstStageShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT real 512 first stage shader template");
    }
    ifftReal512FirstStageShaderTemplate->AddPermutation();

    ifftReal512LastStageShaderTemplate = AllocShaderTemplate("_ifftReal512LastStageShader");
    if (!ifftReal512LastStageShaderTemplate->Create(ifftReal512LastStageShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT real 512 last stage shader template");
    }
    ifftReal512LastStageShaderTemplate->AddPermutation();

    ifftReal1024FirstStageShaderTemplate = AllocShaderTemplate("_ifftReal1024FirstStageShader");
    if (!ifftReal1024FirstStageShaderTemplate->Create(ifftReal1024FirstStageShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT real 1024 first stage shader template");
    }
    ifftReal1024FirstStageShaderTemplate->AddPermutation();

    ifftReal1024LastStageShaderTemplate = AllocShaderTemplate("_ifftReal1024LastStageShader");
    if (!ifftReal1024LastStageShaderTemplate->Create(ifftReal1024LastStageShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT real 1024 last stage shader template");
    }
    ifftReal1024LastStageShaderTemplate->AddPermutation();

    ifftDualReal512FirstStageShaderTemplate = AllocShaderTemplate("_ifftDualReal512FirstStageShader");
    if (!ifftDualReal512FirstStageShaderTemplate->Create(ifftDualReal512FirstStageShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT dual real 512 first stage shader template");
    }
    ifftDualReal512FirstStageShaderTemplate->AddPermutation();

    ifftDualReal512LastStageShaderTemplate = AllocShaderTemplate("_ifftDualReal512LastStageShader");
    if (!ifftDualReal512LastStageShaderTemplate->Create(ifftDualReal512LastStageShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT dual real 512 last stage shader template");
    }
    ifftDualReal512LastStageShaderTemplate->AddPermutation();

    ifftDualReal1024FirstStageShaderTemplate = AllocShaderTemplate("_ifftDualReal1024FirstStageShader");
    if (!ifftDualReal1024FirstStageShaderTemplate->Create(ifftDualReal1024FirstStageShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT dual real 1024 first stage shader template");
    }
    ifftDualReal1024FirstStageShaderTemplate->AddPermutation();

    ifftDualReal1024LastStageShaderTemplate = AllocShaderTemplate("_ifftDualReal1024LastStageShader");
    if (!ifftDualReal1024LastStageShaderTemplate->Create(ifftDualReal1024LastStageShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT dual real 1024 last stage shader template");
    }
    ifftDualReal1024LastStageShaderTemplate->AddPermutation();

    transposeComplex256x512ShaderTemplate = AllocShaderTemplate("_transposeComplex256x512Shader");
    if (!transposeComplex256x512ShaderTemplate->Create(transposeComplex256x512ShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create transpose complex 256x512 shader template");
    }
    transposeComplex256x512ShaderTemplate->AddPermutation();

    transposeComplex512x256ShaderTemplate = AllocShaderTemplate("_transposeComplex512x256Shader");
    if (!transposeComplex512x256ShaderTemplate->Create(transposeComplex512x256ShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create transpose complex 512x256 shader template");
    }
    transposeComplex512x256ShaderTemplate->AddPermutation();

    transposeComplex512x1024ShaderTemplate = AllocShaderTemplate("_transposeComplex512x1024Shader");
    if (!transposeComplex512x1024ShaderTemplate->Create(transposeComplex512x1024ShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create transpose complex 512x1024 shader template");
    }
    transposeComplex512x1024ShaderTemplate->AddPermutation();

    transposeComplex1024x512ShaderTemplate = AllocShaderTemplate("_transposeComplex1024x512Shader");
    if (!transposeComplex1024x512ShaderTemplate->Create(transposeComplex1024x512ShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create transpose complex 1024x512 shader template");
    }
    transposeComplex1024x512ShaderTemplate->AddPermutation();

    transposeComplex512ShaderTemplate = AllocShaderTemplate("_transposeComplex512Shader");
    if (!transposeComplex512ShaderTemplate->Create(transposeComplex512ShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create transpose complex 512 shader template");
    }
    transposeComplex512ShaderTemplate->AddPermutation();

    transposeComplex1024ShaderTemplate = AllocShaderTemplate("_transposeComplex1024Shader");
    if (!transposeComplex1024ShaderTemplate->Create(transposeComplex1024ShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create transpose complex 1024 shader template");
    }
    transposeComplex1024ShaderTemplate->AddPermutation();

    unlitShaderTemplate = AllocShaderTemplate("_unlitShader");
    if (!unlitShaderTemplate->Create(unlitShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create unlit shader template");
    }

    litShaderTemplate = AllocShaderTemplate("_litShader");
    if (!litShaderTemplate->Create(litShaderTemplateText, baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create lit shader template");
    }
}

void ShaderTemplateManager::DestroyEngineShaderTemplates() {
    if (fullscreenImageShaderTemplate) {
        fullscreenImageShaderTemplate.reset();
    }
    if (toneMappingShaderTemplate) {
        toneMappingShaderTemplate.reset();
    }
    if (luminancePass1ShaderTemplate) {
        luminancePass1ShaderTemplate.reset();
    }
    if (luminancePass2ShaderTemplate) {
        luminancePass2ShaderTemplate.reset();
    }
    if (bloomPrefilterShaderTemplate) {
        bloomPrefilterShaderTemplate.reset();
    }
    if (bloomDownsampleShaderTemplate) {
        bloomDownsampleShaderTemplate.reset();
    }
    if (bloomUpsampleShaderTemplate) {
        bloomUpsampleShaderTemplate.reset();
    }
    if (convolutionBloomPrefilterShaderTemplate) {
        convolutionBloomPrefilterShaderTemplate.reset();
    }
    if (convolutionBloomKernelPrepareShaderTemplate) {
        convolutionBloomKernelPrepareShaderTemplate.reset();
    }
    if (convolutionBloomMultiplySpectrumShaderTemplate) {
        convolutionBloomMultiplySpectrumShaderTemplate.reset();
    }
    if (convolutionBloomResolveShaderTemplate) {
        convolutionBloomResolveShaderTemplate.reset();
    }
    if (fftComplex256ShaderTemplate) {
        fftComplex256ShaderTemplate.reset();
    }
    if (fftComplex512ShaderTemplate) {
        fftComplex512ShaderTemplate.reset();
    }
    if (fftComplex1024ShaderTemplate) {
        fftComplex1024ShaderTemplate.reset();
    }
    if (fftComplex1024LastStageShaderTemplate) {
        fftComplex1024LastStageShaderTemplate.reset();
    }
    if (fftReal512FirstStageShaderTemplate) {
        fftReal512FirstStageShaderTemplate.reset();
    }
    if (fftReal512LastStageShaderTemplate) {
        fftReal512LastStageShaderTemplate.reset();
    }
    if (fftReal1024FirstStageShaderTemplate) {
        fftReal1024FirstStageShaderTemplate.reset();
    }
    if (fftReal1024LastStageShaderTemplate) {
        fftReal1024LastStageShaderTemplate.reset();
    }
    if (fftDualReal512FirstStageShaderTemplate) {
        fftDualReal512FirstStageShaderTemplate.reset();
    }
    if (fftDualReal512LastStageShaderTemplate) {
        fftDualReal512LastStageShaderTemplate.reset();
    }
    if (fftDualReal1024FirstStageShaderTemplate) {
        fftDualReal1024FirstStageShaderTemplate.reset();
    }
    if (fftDualReal1024LastStageShaderTemplate) {
        fftDualReal1024LastStageShaderTemplate.reset();
    }
    if (ifftReal512FirstStageShaderTemplate) {
        ifftReal512FirstStageShaderTemplate.reset();
    }
    if (ifftReal512LastStageShaderTemplate) {
        ifftReal512LastStageShaderTemplate.reset();
    }
    if (ifftReal1024FirstStageShaderTemplate) {
        ifftReal1024FirstStageShaderTemplate.reset();
    }
    if (ifftReal1024LastStageShaderTemplate) {
        ifftReal1024LastStageShaderTemplate.reset();
    }
    if (ifftDualReal512FirstStageShaderTemplate) {
        ifftDualReal512FirstStageShaderTemplate.reset();
    }
    if (ifftDualReal512LastStageShaderTemplate) {
        ifftDualReal512LastStageShaderTemplate.reset();
    }
    if (ifftDualReal1024FirstStageShaderTemplate) {
        ifftDualReal1024FirstStageShaderTemplate.reset();
    }
    if (ifftDualReal1024LastStageShaderTemplate) {
        ifftDualReal1024LastStageShaderTemplate.reset();
    }
    if (transposeComplex256x512ShaderTemplate) {
        transposeComplex256x512ShaderTemplate.reset();
    }
    if (transposeComplex512x256ShaderTemplate) {
        transposeComplex512x256ShaderTemplate.reset();
    }
    if (transposeComplex512x1024ShaderTemplate) {
        transposeComplex512x1024ShaderTemplate.reset();
    }
    if (transposeComplex1024x512ShaderTemplate) {
        transposeComplex1024x512ShaderTemplate.reset();
    }
    if (transposeComplex512ShaderTemplate) {
        transposeComplex512ShaderTemplate.reset();
    }
    if (transposeComplex1024ShaderTemplate) {
        transposeComplex1024ShaderTemplate.reset();
    }
    if (unlitShaderTemplate) {
        unlitShaderTemplate.reset();
    }
    if (litShaderTemplate) {
        litShaderTemplate.reset();
    }
}
