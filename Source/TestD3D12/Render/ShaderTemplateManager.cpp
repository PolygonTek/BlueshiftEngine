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

    if (lastInvalidatedGlobalDefineModCount != globalDefineModCountSnapshot) {
        InvalidateShaderTemplateCaches();
        lastInvalidatedGlobalDefineModCount = globalDefineModCountSnapshot;
    }

    CleanExpiredShaderTemplates();
}

std::shared_ptr<ShaderTemplate> ShaderTemplateManager::AllocShaderTemplate(const char *hashName) {
    if (!hashName || !hashName[0]) {
        BE_FATALERROR("ShaderTemplateManager::AllocShaderTemplate: empty shader template name");
    }
    if (const auto *entry = shaderTemplateHashMap.Get(hashName)) {
        if (!entry->second.expired()) {
            BE_FATALERROR("ShaderTemplateManager::AllocShaderTemplate: %s shader template already allocated", hashName);
        }
        shaderTemplateHashMap.Remove(hashName);
    }

    std::shared_ptr<ShaderTemplate> shaderTemplate = std::make_shared<ShaderTemplate>();
    shaderTemplate->hashName = hashName;
    shaderTemplate->name = hashName;
    shaderTemplate->name.StripPath();
    shaderTemplate->name.StripFileExtension();
    shaderTemplateHashMap.Set(shaderTemplate->hashName, shaderTemplate);

    return shaderTemplate;
}

std::shared_ptr<ShaderTemplate> ShaderTemplateManager::FindShaderTemplate(const char *hashName) const {
    if (!hashName || !hashName[0]) {
        return nullptr;
    }

    const auto *entry = shaderTemplateHashMap.Get(hashName);
    if (!entry) {
        return nullptr;
    }

    return entry->second.lock();
}

std::shared_ptr<ShaderTemplate> ShaderTemplateManager::GetShaderTemplate(const char *hashName) {
    if (!hashName || !hashName[0]) {
        return nullptr;
    }

    std::shared_ptr<ShaderTemplate> shaderTemplate = FindShaderTemplate(hashName);
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

    for (int i = 0; i < globalDefines.Count(); i++) {
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

void ShaderTemplateManager::InvalidateShaderTemplateCaches() {
    for (int i = 0; i < shaderTemplateHashMap.Count(); i++) {
        const auto *entry = shaderTemplateHashMap.GetByIndex(i);
        assert(entry);

        std::shared_ptr<ShaderTemplate> shaderTemplate = entry->second.lock();
        if (!shaderTemplate) {
            continue;
        }

        // 셰이더를 모두 삭제
        shaderTemplate->ClearPermutationShaders();

        // 캐시된 파이프라인을 모두 무효화 후 재생성 요청
        shaderTemplate->InvalidateCachedPipelines();
    }
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
    fullscreenImageShaderTemplate = AllocShaderTemplate("_fullScreenImageShader");
    fullscreenImageShaderTemplate->AddStage(RHI::ShaderStage::Vertex, RHI::ShaderModel::SM_6_0, "Source/TestD3D12/Shaders/FullScreenTriangle.hlsl", "VSMain");
    fullscreenImageShaderTemplate->AddStage(RHI::ShaderStage::Fragment, RHI::ShaderModel::SM_6_0, "Source/TestD3D12/Shaders/FullScreenTriangle.hlsl", "PSMain");
    fullscreenImageShaderTemplate->AddPermutation({});

    unlitShaderTemplate = AllocShaderTemplate("_unlitShader");
    unlitShaderTemplate->AddStage(RHI::ShaderStage::Vertex, RHI::ShaderModel::SM_6_0, "Source/TestD3D12/Shaders/Unlit.hlsl", "VSMain");
    unlitShaderTemplate->AddStage(RHI::ShaderStage::Fragment, RHI::ShaderModel::SM_6_0, "Source/TestD3D12/Shaders/Unlit.hlsl", "PSMain");
    unlitShaderTemplate->AddPermutation({});
    unlitShaderTemplate->AddPermutation({ "USE_INSTANCING=1" });

    litShaderTemplate = AllocShaderTemplate("_litShader");
    litShaderTemplate->AddStage(RHI::ShaderStage::Vertex, RHI::ShaderModel::SM_6_0, "Source/TestD3D12/Shaders/Lit.hlsl", "VSMain");
    litShaderTemplate->AddStage(RHI::ShaderStage::Fragment, RHI::ShaderModel::SM_6_0, "Source/TestD3D12/Shaders/Lit.hlsl", "PSMain");
    litShaderTemplate->AddPermutation({});
    litShaderTemplate->AddPermutation({ "USE_INSTANCING=1" });
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
}
