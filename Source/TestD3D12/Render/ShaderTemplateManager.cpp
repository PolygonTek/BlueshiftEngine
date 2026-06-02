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
    globalDefineModCount = 0;

    CreateBuiltInShaderTemplates();
}

void ShaderTemplateManager::Shutdown() {
    for (std::shared_ptr<ShaderTemplate> &shaderTemplate : builtInShaderTemplates) {
        shaderTemplate.reset();
    }

    shaderTemplateHashMap.Clear();

    {
        BE1::ScopedLock lock(globalDefinesMutex);
        globalDefines.Clear();
        globalDefineModCount = 0;
    }

    BE1::PlatformMutex::Destroy(globalDefinesMutex);
    globalDefinesMutex = nullptr;
}

void ShaderTemplateManager::UpdateFrame() {
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

std::shared_ptr<ShaderTemplate> ShaderTemplateManager::GetBuiltInShaderTemplate(ShaderTemplateId id) const {
    assert(to_int(id) >= 0 && to_int(id) < COUNT_OF(builtInShaderTemplates));
    return builtInShaderTemplates[to_int(id)];
}

uint32_t ShaderTemplateManager::CopyGlobalDefines(BE1::StrArray &outDefines) const {
    BE1::ScopedLock lock(globalDefinesMutex);

    outDefines = globalDefines;
    return globalDefineModCount;
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

void ShaderTemplateManager::InvalidateShaderTemplateCaches() {
    for (int i = 0; i < shaderTemplateHashMap.Count(); i++) {
        const auto *entry = shaderTemplateHashMap.GetByIndex(i);
        assert(entry);

        std::shared_ptr<ShaderTemplate> shaderTemplate = entry->second.lock();
        if (!shaderTemplate) {
            continue;
        }

        shaderTemplate->ClearPermutationShaders();
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

void ShaderTemplateManager::CreateBuiltInShaderTemplates() {
    std::shared_ptr<ShaderTemplate> fullScreenImage = AllocShaderTemplate("_fullScreenImageShader");
    fullScreenImage->AddStage(RHI::ShaderStage::Vertex, RHI::ShaderModel::SM_6_0, "Source/TestD3D12/Shaders/FullScreenTriangle.hlsl", "VSMain");
    fullScreenImage->AddStage(RHI::ShaderStage::Fragment, RHI::ShaderModel::SM_6_0, "Source/TestD3D12/Shaders/FullScreenTriangle.hlsl", "PSMain");
    fullScreenImage->AddPermutation({});
    builtInShaderTemplates[to_int(ShaderTemplateId::FullScreenImage)] = fullScreenImage;

    std::shared_ptr<ShaderTemplate> unlit = AllocShaderTemplate("_unlitShader");
    unlit->AddStage(RHI::ShaderStage::Vertex, RHI::ShaderModel::SM_6_0, "Source/TestD3D12/Shaders/Unlit.hlsl", "VSMain");
    unlit->AddStage(RHI::ShaderStage::Fragment, RHI::ShaderModel::SM_6_0, "Source/TestD3D12/Shaders/Unlit.hlsl", "PSMain");
    unlit->AddPermutation({});
    unlit->AddPermutation({ "USE_INSTANCING=1" });
    builtInShaderTemplates[to_int(ShaderTemplateId::Unlit)] = unlit;

    std::shared_ptr<ShaderTemplate> lit = AllocShaderTemplate("_litShader");
    lit->AddStage(RHI::ShaderStage::Vertex, RHI::ShaderModel::SM_6_0, "Source/TestD3D12/Shaders/Lit.hlsl", "VSMain");
    lit->AddStage(RHI::ShaderStage::Fragment, RHI::ShaderModel::SM_6_0, "Source/TestD3D12/Shaders/Lit.hlsl", "PSMain");
    lit->AddPermutation({});
    lit->AddPermutation({ "USE_INSTANCING=1" });
    builtInShaderTemplates[to_int(ShaderTemplateId::Lit)] = lit;
}
