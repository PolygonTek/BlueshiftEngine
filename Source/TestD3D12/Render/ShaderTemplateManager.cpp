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

ShaderTemplateManager       shaderTemplateManager;

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

    fullscreenImageShaderTemplate = GetShaderTemplate("Source/TestD3D12/Shaders/FullscreenImage");
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

    CreateFFTShaderTemplates(computeShaderModel, baseDir);
    CreateTransposeShaderTemplates(computeShaderModel, baseDir);
    CreateBloomShaderTemplates(computeShaderModel, baseDir);
    CreateToneMappingShaderTemplates(computeShaderModel, baseDir);
}

void ShaderTemplateManager::CreateFFTShaderTemplates(RHI::ShaderModel shaderModel, const char *baseDir) {
    fftComplex256ShaderTemplate = AllocShaderTemplate("_fftComplex256Shader");
    const BE1::Str fftComplex256ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTComplex256.hlsl", "CSMain");
    if (!fftComplex256ShaderTemplate->Create(fftComplex256ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT complex 256 shader template");
    }
    fftComplex256ShaderTemplate->AddPermutation();

    fftComplex512ShaderTemplate = AllocShaderTemplate("_fftComplex512Shader");
    const BE1::Str fftComplex512ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTComplex512.hlsl", "CSMain");
    if (!fftComplex512ShaderTemplate->Create(fftComplex512ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT complex 512 shader template");
    }
    fftComplex512ShaderTemplate->AddPermutation();

    fftComplex1024ShaderTemplate = AllocShaderTemplate("_fftComplex1024Shader");
    const BE1::Str fftComplex1024ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTComplex1024.hlsl", "CSMain");
    if (!fftComplex1024ShaderTemplate->Create(fftComplex1024ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT complex 1024 shader template");
    }
    fftComplex1024ShaderTemplate->AddPermutation();

    fftComplex2048ShaderTemplate = AllocShaderTemplate("_fftComplex2048Shader");
    const BE1::Str fftComplex2048ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTComplex2048.hlsl", "CSMain");
    if (!fftComplex2048ShaderTemplate->Create(fftComplex2048ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT complex 2048 shader template");
    }
    fftComplex2048ShaderTemplate->AddPermutation();

    fftReal512ShaderTemplate = AllocShaderTemplate("_fftReal512Shader");
    const BE1::Str fftReal512ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTReal512.hlsl", "CSMain");
    if (!fftReal512ShaderTemplate->Create(fftReal512ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT real 512 shader template");
    }
    fftReal512ShaderTemplate->AddPermutation();

    fftReal1024ShaderTemplate = AllocShaderTemplate("_fftReal1024Shader");
    const BE1::Str fftReal1024ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTReal1024.hlsl", "CSMain");
    if (!fftReal1024ShaderTemplate->Create(fftReal1024ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT real 1024 shader template");
    }
    fftReal1024ShaderTemplate->AddPermutation();

    fftReal2048ShaderTemplate = AllocShaderTemplate("_fftReal2048Shader");
    const BE1::Str fftReal2048ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTReal2048.hlsl", "CSMain");
    if (!fftReal2048ShaderTemplate->Create(fftReal2048ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT real 2048 shader template");
    }
    fftReal2048ShaderTemplate->AddPermutation();

    fftDualReal256ShaderTemplate = AllocShaderTemplate("_fftDualReal256Shader");
    const BE1::Str fftDualReal256ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTDualReal256.hlsl", "CSMain");
    if (!fftDualReal256ShaderTemplate->Create(fftDualReal256ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT dual real 256 shader template");
    }
    fftDualReal256ShaderTemplate->AddPermutation();

    fftDualReal512ShaderTemplate = AllocShaderTemplate("_fftDualReal512Shader");
    const BE1::Str fftDualReal512ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTDualReal512.hlsl", "CSMain");
    if (!fftDualReal512ShaderTemplate->Create(fftDualReal512ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT dual real 512 shader template");
    }
    fftDualReal512ShaderTemplate->AddPermutation();

    fftDualReal1024ShaderTemplate = AllocShaderTemplate("_fftDualReal1024Shader");
    const BE1::Str fftDualReal1024ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTDualReal1024.hlsl", "CSMain");
    if (!fftDualReal1024ShaderTemplate->Create(fftDualReal1024ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT dual real 1024 shader template");
    }
    fftDualReal1024ShaderTemplate->AddPermutation();

    fftDualReal2048ShaderTemplate = AllocShaderTemplate("_fftDualReal2048Shader");
    const BE1::Str fftDualReal2048ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/FFTDualReal2048.hlsl", "CSMain");
    if (!fftDualReal2048ShaderTemplate->Create(fftDualReal2048ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create FFT dual real 2048 shader template");
    }
    fftDualReal2048ShaderTemplate->AddPermutation();

    ifftReal512ShaderTemplate = AllocShaderTemplate("_ifftReal512Shader");
    const BE1::Str ifftReal512ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/IFFTReal512.hlsl", "CSMain");
    if (!ifftReal512ShaderTemplate->Create(ifftReal512ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT real 512 shader template");
    }
    ifftReal512ShaderTemplate->AddPermutation();

    ifftReal1024ShaderTemplate = AllocShaderTemplate("_ifftReal1024Shader");
    const BE1::Str ifftReal1024ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/IFFTReal1024.hlsl", "CSMain");
    if (!ifftReal1024ShaderTemplate->Create(ifftReal1024ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT real 1024 shader template");
    }
    ifftReal1024ShaderTemplate->AddPermutation();

    ifftReal2048ShaderTemplate = AllocShaderTemplate("_ifftReal2048Shader");
    const BE1::Str ifftReal2048ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/IFFTReal2048.hlsl", "CSMain");
    if (!ifftReal2048ShaderTemplate->Create(ifftReal2048ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT real 2048 shader template");
    }
    ifftReal2048ShaderTemplate->AddPermutation();

    ifftDualReal256ShaderTemplate = AllocShaderTemplate("_ifftDualReal256Shader");
    const BE1::Str ifftDualReal256ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/IFFTDualReal256.hlsl", "CSMain");
    if (!ifftDualReal256ShaderTemplate->Create(ifftDualReal256ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT dual real 256 shader template");
    }
    ifftDualReal256ShaderTemplate->AddPermutation();

    ifftDualReal512ShaderTemplate = AllocShaderTemplate("_ifftDualReal512Shader");
    const BE1::Str ifftDualReal512ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/IFFTDualReal512.hlsl", "CSMain");
    if (!ifftDualReal512ShaderTemplate->Create(ifftDualReal512ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT dual real 512 shader template");
    }
    ifftDualReal512ShaderTemplate->AddPermutation();

    ifftDualReal1024ShaderTemplate = AllocShaderTemplate("_ifftDualReal1024Shader");
    const BE1::Str ifftDualReal1024ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/IFFTDualReal1024.hlsl", "CSMain");
    if (!ifftDualReal1024ShaderTemplate->Create(ifftDualReal1024ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT dual real 1024 shader template");
    }
    ifftDualReal1024ShaderTemplate->AddPermutation();

    ifftDualReal2048ShaderTemplate = AllocShaderTemplate("_ifftDualReal2048Shader");
    const BE1::Str ifftDualReal2048ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "FFT/IFFTDualReal2048.hlsl", "CSMain");
    if (!ifftDualReal2048ShaderTemplate->Create(ifftDualReal2048ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create IFFT dual real 2048 shader template");
    }
    ifftDualReal2048ShaderTemplate->AddPermutation();

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
}

void ShaderTemplateManager::CreateTransposeShaderTemplates(RHI::ShaderModel shaderModel, const char *baseDir) {
    transposeComplex256ShaderTemplate = AllocShaderTemplate("_transposeComplex256Shader");
    const BE1::Str transposeComplex256ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "Transpose/TransposeComplex256.hlsl", "CSMain");
    if (!transposeComplex256ShaderTemplate->Create(transposeComplex256ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create transpose complex 256 shader template");
    }
    transposeComplex256ShaderTemplate->AddPermutation();

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

    transposeComplex2048ShaderTemplate = AllocShaderTemplate("_transposeComplex2048Shader");
    const BE1::Str transposeComplex2048ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "Transpose/TransposeComplex2048.hlsl", "CSMain");
    if (!transposeComplex2048ShaderTemplate->Create(transposeComplex2048ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create transpose complex 2048 shader template");
    }
    transposeComplex2048ShaderTemplate->AddPermutation();

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

    transposeComplex1024x2048ShaderTemplate = AllocShaderTemplate("_transposeComplex1024x2048Shader");
    const BE1::Str transposeComplex1024x2048ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "Transpose/TransposeComplex1024x2048.hlsl", "CSMain");
    if (!transposeComplex1024x2048ShaderTemplate->Create(transposeComplex1024x2048ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create transpose complex 1024x2048 shader template");
    }
    transposeComplex1024x2048ShaderTemplate->AddPermutation();

    transposeComplex2048x1024ShaderTemplate = AllocShaderTemplate("_transposeComplex2048x1024Shader");
    const BE1::Str transposeComplex2048x1024ShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "Transpose/TransposeComplex2048x1024.hlsl", "CSMain");
    if (!transposeComplex2048x1024ShaderTemplate->Create(transposeComplex2048x1024ShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create transpose complex 2048x1024 shader template");
    }
    transposeComplex2048x1024ShaderTemplate->AddPermutation();
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

    convolutionBloomAperturePrepareShaderTemplate = AllocShaderTemplate("_convolutionBloomAperturePrepareShader");
    const BE1::Str convolutionBloomAperturePrepareShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "Bloom/ConvolutionBloomAperturePrepare.hlsl", "CSMain");
    if (!convolutionBloomAperturePrepareShaderTemplate->Create(convolutionBloomAperturePrepareShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create convolution bloom aperture prepare shader template");
    }
    convolutionBloomAperturePrepareShaderTemplate->AddPermutation();

    convolutionBloomReduceApertureEnergyShaderTemplate = AllocShaderTemplate("_convolutionBloomReduceApertureEnergyShader");
    const BE1::Str convolutionBloomReduceApertureEnergyShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "Bloom/ConvolutionBloomReduceApertureEnergy.hlsl", "CSMain");
    if (!convolutionBloomReduceApertureEnergyShaderTemplate->Create(convolutionBloomReduceApertureEnergyShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create convolution bloom reduce aperture energy shader template");
    }
    convolutionBloomReduceApertureEnergyShaderTemplate->AddPermutation();

    convolutionBloomResolveDiffractionPSFShaderTemplate = AllocShaderTemplate("_convolutionBloomResolveDiffractionPSFShader");
    const BE1::Str convolutionBloomResolveDiffractionPSFShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "Bloom/ConvolutionBloomResolveDiffractionPSF.hlsl", "CSMain");
    if (!convolutionBloomResolveDiffractionPSFShaderTemplate->Create(convolutionBloomResolveDiffractionPSFShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create convolution bloom resolve diffraction PSF shader template");
    }
    convolutionBloomResolveDiffractionPSFShaderTemplate->AddPermutation();

    convolutionBloomApplyDispersionShaderTemplate = AllocShaderTemplate("_convolutionBloomApplyDispersionShader");
    const BE1::Str convolutionBloomApplyDispersionShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "Bloom/ConvolutionBloomApplyDispersion.hlsl", "CSMain");
    if (!convolutionBloomApplyDispersionShaderTemplate->Create(convolutionBloomApplyDispersionShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create convolution bloom apply dispersion shader template");
    }
    convolutionBloomApplyDispersionShaderTemplate->AddPermutation();
}

void ShaderTemplateManager::CreateToneMappingShaderTemplates(RHI::ShaderModel shaderModel, const char *baseDir) {
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

    toneMappingShaderTemplate = AllocShaderTemplate("_toneMappingShader");
    const BE1::Str toneMappingShaderTemplateText = CreateComputeShaderTemplateText(shaderModel, "ToneMapping/ToneMapping.hlsl", "CSMain");
    if (!toneMappingShaderTemplate->Create(toneMappingShaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to create tone mapping shader template");
    }
    toneMappingShaderTemplate->AddPermutation();
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
    if (fftComplex256ShaderTemplate) {
        fftComplex256ShaderTemplate.reset();
    }
    if (fftComplex512ShaderTemplate) {
        fftComplex512ShaderTemplate.reset();
    }
    if (fftComplex1024ShaderTemplate) {
        fftComplex1024ShaderTemplate.reset();
    }
    if (fftComplex2048ShaderTemplate) {
        fftComplex2048ShaderTemplate.reset();
    }
    if (fftReal512ShaderTemplate) {
        fftReal512ShaderTemplate.reset();
    }
    if (fftReal1024ShaderTemplate) {
        fftReal1024ShaderTemplate.reset();
    }
    if (fftReal2048ShaderTemplate) {
        fftReal2048ShaderTemplate.reset();
    }
    if (ifftReal512ShaderTemplate) {
        ifftReal512ShaderTemplate.reset();
    }
    if (ifftReal1024ShaderTemplate) {
        ifftReal1024ShaderTemplate.reset();
    }
    if (ifftReal2048ShaderTemplate) {
        ifftReal2048ShaderTemplate.reset();
    }
    if (fftDualReal256ShaderTemplate) {
        fftDualReal256ShaderTemplate.reset();
    }
    if (fftDualReal512ShaderTemplate) {
        fftDualReal512ShaderTemplate.reset();
    }
    if (fftDualReal1024ShaderTemplate) {
        fftDualReal1024ShaderTemplate.reset();
    }
    if (fftDualReal2048ShaderTemplate) {
        fftDualReal2048ShaderTemplate.reset();
    }
    if (ifftDualReal256ShaderTemplate) {
        ifftDualReal256ShaderTemplate.reset();
    }
    if (ifftDualReal512ShaderTemplate) {
        ifftDualReal512ShaderTemplate.reset();
    }
    if (ifftDualReal1024ShaderTemplate) {
        ifftDualReal1024ShaderTemplate.reset();
    }
    if (ifftDualReal2048ShaderTemplate) {
        ifftDualReal2048ShaderTemplate.reset();
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
    if (transposeComplex256ShaderTemplate) {
        transposeComplex256ShaderTemplate.reset();
    }
    if (transposeComplex512ShaderTemplate) {
        transposeComplex512ShaderTemplate.reset();
    }
    if (transposeComplex1024ShaderTemplate) {
        transposeComplex1024ShaderTemplate.reset();
    }
    if (transposeComplex2048ShaderTemplate) {
        transposeComplex2048ShaderTemplate.reset();
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
    if (transposeComplex1024x2048ShaderTemplate) {
        transposeComplex1024x2048ShaderTemplate.reset();
    }
    if (transposeComplex2048x1024ShaderTemplate) {
        transposeComplex2048x1024ShaderTemplate.reset();
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
    if (convolutionBloomResolveShaderTemplate) {
        convolutionBloomResolveShaderTemplate.reset();
    }
    if (convolutionBloomAperturePrepareShaderTemplate) {
        convolutionBloomAperturePrepareShaderTemplate.reset();
    }
    if (convolutionBloomReduceApertureEnergyShaderTemplate) {
        convolutionBloomReduceApertureEnergyShaderTemplate.reset();
    }
    if (convolutionBloomResolveDiffractionPSFShaderTemplate) {
        convolutionBloomResolveDiffractionPSFShaderTemplate.reset();
    }
    if (convolutionBloomApplyDispersionShaderTemplate) {
        convolutionBloomApplyDispersionShaderTemplate.reset();
    }
    if (luminancePass1ShaderTemplate) {
        luminancePass1ShaderTemplate.reset();
    }
    if (luminancePass2ShaderTemplate) {
        luminancePass2ShaderTemplate.reset();
    }
    if (toneMappingShaderTemplate) {
        toneMappingShaderTemplate.reset();
    }
}
