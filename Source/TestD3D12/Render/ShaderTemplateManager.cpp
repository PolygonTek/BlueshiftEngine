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

static const char *ShaderModelToString(RHI::ShaderModel shaderModel) {
    switch (shaderModel) {
    case RHI::ShaderModel::SM_5_0: return "SM_5_0";
    case RHI::ShaderModel::SM_6_0: return "SM_6_0";
    case RHI::ShaderModel::SM_6_1: return "SM_6_1";
    case RHI::ShaderModel::SM_6_2: return "SM_6_2";
    case RHI::ShaderModel::SM_6_3: return "SM_6_3";
    case RHI::ShaderModel::SM_6_4: return "SM_6_4";
    case RHI::ShaderModel::SM_6_5: return "SM_6_5";
    case RHI::ShaderModel::SM_6_6: return "SM_6_6";
    case RHI::ShaderModel::SM_6_7: return "SM_6_7";
    default:
        BE_FATALERROR("ShaderTemplateManager::ShaderModelToString: invalid shader model");
        return "";
    }
}

static BE1::Str CreateComputeShaderTemplateText(RHI::ShaderModel shaderModel, const char *filename, const char *entryPoint) {
    BE1::Str text;
    text.sPrintf(
        "{\n"
        "    stages {\n"
        "        compute %s \"%s\" \"%s\"\n"
        "    }\n"
        "}\n",
        ShaderModelToString(shaderModel), filename, entryPoint);
    return text;
}

void ShaderTemplateManager::CreateEngineShaderTemplates() {
    const char *baseDir = "Source/TestD3D12/Shaders";

    fullscreenImageShaderTemplate = GetShaderTemplate("Source/TestD3D12/Shaders/FullScreenImage");
    if (!fullscreenImageShaderTemplate) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to load fullscreen image shader template");
    }
    fullscreenImageShaderTemplate->AddPermutation();

    unlitShaderTemplate = GetShaderTemplate("Source/TestD3D12/Shaders/Unlit");
    if (!unlitShaderTemplate) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to load unlit shader template");
    }

    litShaderTemplate = GetShaderTemplate("Source/TestD3D12/Shaders/Lit");
    if (!litShaderTemplate) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to load lit shader template");
    }

    const RHI::ShaderModel computeShaderModel = RHI::ShaderModel::SM_6_6;

    CreateToneMappingShaderTemplates(computeShaderModel, baseDir);
    CreateBloomShaderTemplates(computeShaderModel, baseDir);
    CreateFFTShaderTemplates(computeShaderModel, baseDir);
    CreateTransposeShaderTemplates(computeShaderModel, baseDir);
}

void ShaderTemplateManager::CreateToneMappingShaderTemplates(RHI::ShaderModel shaderModel, const char *baseDir) {
    toneMappingShaderTemplate = AllocShaderTemplate("_toneMappingShader");
    const BE1::Str toneMappingShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "ToneMapping/ToneMapping.hlsl", "CSMain");
    if (!toneMappingShaderTemplate->Create(toneMappingShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create tone mapping shader template");
    }
    toneMappingShaderTemplate->AddPermutation();

    luminancePass1ShaderTemplate = AllocShaderTemplate("_luminancePass1Shader");
    const BE1::Str luminancePass1ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "ToneMapping/LuminancePass1.hlsl", "CSMain");
    if (!luminancePass1ShaderTemplate->Create(luminancePass1ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create luminance pass 1 shader template");
    }
    luminancePass1ShaderTemplate->AddPermutation();

    luminancePass2ShaderTemplate = AllocShaderTemplate("_luminancePass2Shader");
    const BE1::Str luminancePass2ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "ToneMapping/LuminancePass2.hlsl", "CSMain");
    if (!luminancePass2ShaderTemplate->Create(luminancePass2ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create luminance pass 2 shader template");
    }
    luminancePass2ShaderTemplate->AddPermutation();
}

void ShaderTemplateManager::CreateBloomShaderTemplates(RHI::ShaderModel shaderModel, const char *baseDir) {
    bloomPrefilterShaderTemplate = AllocShaderTemplate("_bloomPrefilterShader");
    const BE1::Str bloomPrefilterShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "Bloom/BloomPrefilter.hlsl", "CSMain");
    if (!bloomPrefilterShaderTemplate->Create(bloomPrefilterShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create bloom prefilter shader template");
    }
    bloomPrefilterShaderTemplate->AddPermutation();

    bloomDownsampleShaderTemplate = AllocShaderTemplate("_bloomDownsampleShader");
    const BE1::Str bloomDownsampleShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "Bloom/BloomDownsample.hlsl", "CSMain");
    if (!bloomDownsampleShaderTemplate->Create(bloomDownsampleShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create bloom downsample shader template");
    }
    bloomDownsampleShaderTemplate->AddPermutation();

    bloomUpsampleShaderTemplate = AllocShaderTemplate("_bloomUpsampleShader");
    const BE1::Str bloomUpsampleShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "Bloom/BloomUpsample.hlsl", "CSMain");
    if (!bloomUpsampleShaderTemplate->Create(bloomUpsampleShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create bloom upsample shader template");
    }
    bloomUpsampleShaderTemplate->AddPermutation();

    convolutionBloomPrefilterShaderTemplate = AllocShaderTemplate("_convolutionBloomPrefilterShader");
    const BE1::Str convolutionBloomPrefilterShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "Bloom/ConvolutionBloomPrefilter.hlsl", "CSMain");
    if (!convolutionBloomPrefilterShaderTemplate->Create(convolutionBloomPrefilterShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create convolution bloom prefilter shader template");
    }
    convolutionBloomPrefilterShaderTemplate->AddPermutation();

    convolutionBloomKernelPrepareShaderTemplate = AllocShaderTemplate("_convolutionBloomKernelPrepareShader");
    const BE1::Str convolutionBloomKernelPrepareShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "Bloom/ConvolutionBloomKernelPrepare.hlsl", "CSMain");
    if (!convolutionBloomKernelPrepareShaderTemplate->Create(convolutionBloomKernelPrepareShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create convolution bloom kernel prepare shader template");
    }
    convolutionBloomKernelPrepareShaderTemplate->AddPermutation();

    convolutionBloomResolveShaderTemplate = AllocShaderTemplate("_convolutionBloomResolveShader");
    const BE1::Str convolutionBloomResolveShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "Bloom/ConvolutionBloomResolve.hlsl", "CSMain");
    if (!convolutionBloomResolveShaderTemplate->Create(convolutionBloomResolveShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create convolution bloom resolve shader template");
    }
    convolutionBloomResolveShaderTemplate->AddPermutation();
}

void ShaderTemplateManager::CreateFFTShaderTemplates(RHI::ShaderModel shaderModel, const char *baseDir) {
    multiplyComplexSpectrum2DShaderTemplate = AllocShaderTemplate("_multiplyComplexSpectrum2DShader");
    const BE1::Str multiplyComplexSpectrum2DShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/MultiplyComplexSpectrum2D.hlsl", "CSMain");
    if (!multiplyComplexSpectrum2DShaderTemplate->Create(multiplyComplexSpectrum2DShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create multiply complex spectrum 2D shader template");
    }
    multiplyComplexSpectrum2DShaderTemplate->AddPermutation();

    multiplyPackedRealSpectrum2DShaderTemplate = AllocShaderTemplate("_multiplyPackedRealSpectrum2DShader");
    const BE1::Str multiplyPackedRealSpectrum2DShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/MultiplyPackedRealSpectrum2D.hlsl", "CSMain");
    if (!multiplyPackedRealSpectrum2DShaderTemplate->Create(multiplyPackedRealSpectrum2DShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create multiply packed real spectrum 2D shader template");
    }
    multiplyPackedRealSpectrum2DShaderTemplate->AddPermutation();

    multiplyPackedDualRealSpectrum2DShaderTemplate = AllocShaderTemplate("_multiplyPackedDualRealSpectrum2DShader");
    const BE1::Str multiplyPackedDualRealSpectrum2DShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/MultiplyPackedDualRealSpectrum2D.hlsl", "CSMain");
    if (!multiplyPackedDualRealSpectrum2DShaderTemplate->Create(multiplyPackedDualRealSpectrum2DShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create multiply packed dual-real spectrum 2D shader template");
    }
    multiplyPackedDualRealSpectrum2DShaderTemplate->AddPermutation();

    fftComplex256ShaderTemplate = AllocShaderTemplate("_fftComplex256Shader");
    const BE1::Str fftComplex256ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTComplex256_Radix4.hlsl", "CSMain");
    if (!fftComplex256ShaderTemplate->Create(fftComplex256ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT complex 256 shader template");
    }
    fftComplex256ShaderTemplate->AddPermutation();

    fftComplex512ShaderTemplate = AllocShaderTemplate("_fftComplex512Shader");
    const BE1::Str fftComplex512ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTComplex512_Radix8.hlsl", "CSMain");
    if (!fftComplex512ShaderTemplate->Create(fftComplex512ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT512 shader template");
    }
    fftComplex512ShaderTemplate->AddPermutation();

    fftComplex1024ShaderTemplate = AllocShaderTemplate("_fftComplex1024Shader");
    const BE1::Str fftComplex1024ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTComplex1024_Radix8.hlsl", "CSMain");
    if (!fftComplex1024ShaderTemplate->Create(fftComplex1024ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT1024 shader template");
    }
    fftComplex1024ShaderTemplate->AddPermutation();

    fftComplex1024LastStageShaderTemplate = AllocShaderTemplate("_fftComplex1024LastStageShader");
    const BE1::Str fftComplex1024LastStageShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTComplex1024LastStage_Radix2.hlsl", "CSMain");
    if (!fftComplex1024LastStageShaderTemplate->Create(fftComplex1024LastStageShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT1024 last stage shader template");
    }
    fftComplex1024LastStageShaderTemplate->AddPermutation();

    fftReal512FirstStageShaderTemplate = AllocShaderTemplate("_fftReal512FirstStageShader");
    const BE1::Str fftReal512FirstStageShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTReal512FirstStage_Radix4.hlsl", "CSMain");
    if (!fftReal512FirstStageShaderTemplate->Create(fftReal512FirstStageShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT real 512 first stage shader template");
    }
    fftReal512FirstStageShaderTemplate->AddPermutation();

    fftReal512LastStageShaderTemplate = AllocShaderTemplate("_fftReal512LastStageShader");
    const BE1::Str fftReal512LastStageShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTReal512LastStage_Radix4.hlsl", "CSMain");
    if (!fftReal512LastStageShaderTemplate->Create(fftReal512LastStageShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT real 512 last stage shader template");
    }
    fftReal512LastStageShaderTemplate->AddPermutation();

    fftReal1024FirstStageShaderTemplate = AllocShaderTemplate("_fftReal1024FirstStageShader");
    const BE1::Str fftReal1024FirstStageShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTReal1024FirstStage_Radix8.hlsl", "CSMain");
    if (!fftReal1024FirstStageShaderTemplate->Create(fftReal1024FirstStageShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT real 1024 first stage shader template");
    }
    fftReal1024FirstStageShaderTemplate->AddPermutation();

    fftReal1024LastStageShaderTemplate = AllocShaderTemplate("_fftReal1024LastStageShader");
    const BE1::Str fftReal1024LastStageShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTReal1024LastStage_Radix8.hlsl", "CSMain");
    if (!fftReal1024LastStageShaderTemplate->Create(fftReal1024LastStageShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT real 1024 last stage shader template");
    }
    fftReal1024LastStageShaderTemplate->AddPermutation();

    fftDualReal256FirstStageShaderTemplate = AllocShaderTemplate("_fftDualReal256FirstStageShader");
    const BE1::Str fftDualReal256FirstStageShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTDualReal256FirstStage_Radix4.hlsl", "CSMain");
    if (!fftDualReal256FirstStageShaderTemplate->Create(fftDualReal256FirstStageShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT dual real 256 first stage shader template");
    }
    fftDualReal256FirstStageShaderTemplate->AddPermutation();

    fftDualReal256LastStageShaderTemplate = AllocShaderTemplate("_fftDualReal256LastStageShader");
    const BE1::Str fftDualReal256LastStageShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTDualReal256LastStage_Radix4.hlsl", "CSMain");
    if (!fftDualReal256LastStageShaderTemplate->Create(fftDualReal256LastStageShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT dual real 256 last stage shader template");
    }
    fftDualReal256LastStageShaderTemplate->AddPermutation();

    fftDualReal512FirstStageShaderTemplate = AllocShaderTemplate("_fftDualReal512FirstStageShader");
    const BE1::Str fftDualReal512FirstStageShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTDualReal512FirstStage_Radix8.hlsl", "CSMain");
    if (!fftDualReal512FirstStageShaderTemplate->Create(fftDualReal512FirstStageShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT dual real 512 first stage shader template");
    }
    fftDualReal512FirstStageShaderTemplate->AddPermutation();

    fftDualReal512LastStageShaderTemplate = AllocShaderTemplate("_fftDualReal512LastStageShader");
    const BE1::Str fftDualReal512LastStageShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTDualReal512LastStage_Radix8.hlsl", "CSMain");
    if (!fftDualReal512LastStageShaderTemplate->Create(fftDualReal512LastStageShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT dual real 512 last stage shader template");
    }
    fftDualReal512LastStageShaderTemplate->AddPermutation();

    fftDualReal1024FirstStageShaderTemplate = AllocShaderTemplate("_fftDualReal1024FirstStageShader");
    const BE1::Str fftDualReal1024FirstStageShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTDualReal1024FirstStage_Radix8.hlsl", "CSMain");
    if (!fftDualReal1024FirstStageShaderTemplate->Create(fftDualReal1024FirstStageShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT dual real 1024 first stage shader template");
    }
    fftDualReal1024FirstStageShaderTemplate->AddPermutation();

    fftDualReal1024LastStageShaderTemplate = AllocShaderTemplate("_fftDualReal1024LastStageShader");
    const BE1::Str fftDualReal1024LastStageShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTDualReal1024LastStage_Radix2.hlsl", "CSMain");
    if (!fftDualReal1024LastStageShaderTemplate->Create(fftDualReal1024LastStageShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT dual real 1024 last stage shader template");
    }
    fftDualReal1024LastStageShaderTemplate->AddPermutation();

    ifftReal512FirstStageShaderTemplate = AllocShaderTemplate("_ifftReal512FirstStageShader");
    const BE1::Str ifftReal512FirstStageShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/IFFTReal512FirstStage_Radix4.hlsl", "CSMain");
    if (!ifftReal512FirstStageShaderTemplate->Create(ifftReal512FirstStageShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT real 512 first stage shader template");
    }
    ifftReal512FirstStageShaderTemplate->AddPermutation();

    ifftReal512LastStageShaderTemplate = AllocShaderTemplate("_ifftReal512LastStageShader");
    const BE1::Str ifftReal512LastStageShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/IFFTReal512LastStage_Radix4.hlsl", "CSMain");
    if (!ifftReal512LastStageShaderTemplate->Create(ifftReal512LastStageShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT real 512 last stage shader template");
    }
    ifftReal512LastStageShaderTemplate->AddPermutation();

    ifftReal1024FirstStageShaderTemplate = AllocShaderTemplate("_ifftReal1024FirstStageShader");
    const BE1::Str ifftReal1024FirstStageShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/IFFTReal1024FirstStage_Radix8.hlsl", "CSMain");
    if (!ifftReal1024FirstStageShaderTemplate->Create(ifftReal1024FirstStageShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT real 1024 first stage shader template");
    }
    ifftReal1024FirstStageShaderTemplate->AddPermutation();

    ifftReal1024LastStageShaderTemplate = AllocShaderTemplate("_ifftReal1024LastStageShader");
    const BE1::Str ifftReal1024LastStageShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/IFFTReal1024LastStage_Radix8.hlsl", "CSMain");
    if (!ifftReal1024LastStageShaderTemplate->Create(ifftReal1024LastStageShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT real 1024 last stage shader template");
    }
    ifftReal1024LastStageShaderTemplate->AddPermutation();

    ifftDualReal256FirstStageShaderTemplate = AllocShaderTemplate("_ifftDualReal256FirstStageShader");
    const BE1::Str ifftDualReal256FirstStageShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/IFFTDualReal256FirstStage_Radix4.hlsl", "CSMain");
    if (!ifftDualReal256FirstStageShaderTemplate->Create(ifftDualReal256FirstStageShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT dual real 256 first stage shader template");
    }
    ifftDualReal256FirstStageShaderTemplate->AddPermutation();

    ifftDualReal256LastStageShaderTemplate = AllocShaderTemplate("_ifftDualReal256LastStageShader");
    const BE1::Str ifftDualReal256LastStageShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/IFFTDualReal256LastStage_Radix4.hlsl", "CSMain");
    if (!ifftDualReal256LastStageShaderTemplate->Create(ifftDualReal256LastStageShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT dual real 256 last stage shader template");
    }
    ifftDualReal256LastStageShaderTemplate->AddPermutation();

    ifftDualReal512FirstStageShaderTemplate = AllocShaderTemplate("_ifftDualReal512FirstStageShader");
    const BE1::Str ifftDualReal512FirstStageShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/IFFTDualReal512FirstStage_Radix8.hlsl", "CSMain");
    if (!ifftDualReal512FirstStageShaderTemplate->Create(ifftDualReal512FirstStageShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT dual real 512 first stage shader template");
    }
    ifftDualReal512FirstStageShaderTemplate->AddPermutation();

    ifftDualReal512LastStageShaderTemplate = AllocShaderTemplate("_ifftDualReal512LastStageShader");
    const BE1::Str ifftDualReal512LastStageShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/IFFTDualReal512LastStage_Radix8.hlsl", "CSMain");
    if (!ifftDualReal512LastStageShaderTemplate->Create(ifftDualReal512LastStageShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT dual real 512 last stage shader template");
    }
    ifftDualReal512LastStageShaderTemplate->AddPermutation();

    ifftDualReal1024FirstStageShaderTemplate = AllocShaderTemplate("_ifftDualReal1024FirstStageShader");
    const BE1::Str ifftDualReal1024FirstStageShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/IFFTDualReal1024FirstStage_Radix8.hlsl", "CSMain");
    if (!ifftDualReal1024FirstStageShaderTemplate->Create(ifftDualReal1024FirstStageShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT dual real 1024 first stage shader template");
    }
    ifftDualReal1024FirstStageShaderTemplate->AddPermutation();

    ifftDualReal1024LastStageShaderTemplate = AllocShaderTemplate("_ifftDualReal1024LastStageShader");
    const BE1::Str ifftDualReal1024LastStageShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/IFFTDualReal1024LastStage_Radix2.hlsl", "CSMain");
    if (!ifftDualReal1024LastStageShaderTemplate->Create(ifftDualReal1024LastStageShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT dual real 1024 last stage shader template");
    }
    ifftDualReal1024LastStageShaderTemplate->AddPermutation();
}

void ShaderTemplateManager::CreateTransposeShaderTemplates(RHI::ShaderModel shaderModel, const char *baseDir) {
    transposeComplex256ShaderTemplate = AllocShaderTemplate("_transposeComplex256Shader");
    const BE1::Str transposeComplex256ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "Transpose/TransposeComplex256.hlsl", "CSMain");
    if (!transposeComplex256ShaderTemplate->Create(transposeComplex256ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create transpose complex 256 shader template");
    }
    transposeComplex256ShaderTemplate->AddPermutation();

    transposeComplex256x512ShaderTemplate = AllocShaderTemplate("_transposeComplex256x512Shader");
    const BE1::Str transposeComplex256x512ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "Transpose/TransposeComplex256x512.hlsl", "CSMain");
    if (!transposeComplex256x512ShaderTemplate->Create(transposeComplex256x512ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create transpose complex 256x512 shader template");
    }
    transposeComplex256x512ShaderTemplate->AddPermutation();

    transposeComplex512x256ShaderTemplate = AllocShaderTemplate("_transposeComplex512x256Shader");
    const BE1::Str transposeComplex512x256ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "Transpose/TransposeComplex512x256.hlsl", "CSMain");
    if (!transposeComplex512x256ShaderTemplate->Create(transposeComplex512x256ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create transpose complex 512x256 shader template");
    }
    transposeComplex512x256ShaderTemplate->AddPermutation();

    transposeComplex512x1024ShaderTemplate = AllocShaderTemplate("_transposeComplex512x1024Shader");
    const BE1::Str transposeComplex512x1024ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "Transpose/TransposeComplex512x1024.hlsl", "CSMain");
    if (!transposeComplex512x1024ShaderTemplate->Create(transposeComplex512x1024ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create transpose complex 512x1024 shader template");
    }
    transposeComplex512x1024ShaderTemplate->AddPermutation();

    transposeComplex1024x512ShaderTemplate = AllocShaderTemplate("_transposeComplex1024x512Shader");
    const BE1::Str transposeComplex1024x512ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "Transpose/TransposeComplex1024x512.hlsl", "CSMain");
    if (!transposeComplex1024x512ShaderTemplate->Create(transposeComplex1024x512ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create transpose complex 1024x512 shader template");
    }
    transposeComplex1024x512ShaderTemplate->AddPermutation();

    transposeComplex512ShaderTemplate = AllocShaderTemplate("_transposeComplex512Shader");
    const BE1::Str transposeComplex512ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "Transpose/TransposeComplex512.hlsl", "CSMain");
    if (!transposeComplex512ShaderTemplate->Create(transposeComplex512ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create transpose complex 512 shader template");
    }
    transposeComplex512ShaderTemplate->AddPermutation();

    transposeComplex1024ShaderTemplate = AllocShaderTemplate("_transposeComplex1024Shader");
    const BE1::Str transposeComplex1024ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "Transpose/TransposeComplex1024.hlsl", "CSMain");
    if (!transposeComplex1024ShaderTemplate->Create(transposeComplex1024ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create transpose complex 1024 shader template");
    }
    transposeComplex1024ShaderTemplate->AddPermutation();
}

void ShaderTemplateManager::DestroyEngineShaderTemplates() {
    if (fullscreenImageShaderTemplate) {
        fullscreenImageShaderTemplate.reset();
    }
    if (unlitShaderTemplate) {
        unlitShaderTemplate.reset();
    }
    if (litShaderTemplate) {
        litShaderTemplate.reset();
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
    if (multiplyComplexSpectrum2DShaderTemplate) {
        multiplyComplexSpectrum2DShaderTemplate.reset();
    }
    if (multiplyPackedRealSpectrum2DShaderTemplate) {
        multiplyPackedRealSpectrum2DShaderTemplate.reset();
    }
    if (multiplyPackedDualRealSpectrum2DShaderTemplate) {
        multiplyPackedDualRealSpectrum2DShaderTemplate.reset();
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
    if (fftDualReal256FirstStageShaderTemplate) {
        fftDualReal256FirstStageShaderTemplate.reset();
    }
    if (fftDualReal256LastStageShaderTemplate) {
        fftDualReal256LastStageShaderTemplate.reset();
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
    if (ifftDualReal256FirstStageShaderTemplate) {
        ifftDualReal256FirstStageShaderTemplate.reset();
    }
    if (ifftDualReal256LastStageShaderTemplate) {
        ifftDualReal256LastStageShaderTemplate.reset();
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
    if (transposeComplex256ShaderTemplate) {
        transposeComplex256ShaderTemplate.reset();
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
}
