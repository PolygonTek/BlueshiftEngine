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
#include "ShaderTemplateManager.h"
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
        shaderPipelineCache.RebuildAllPipelines();
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

void ShaderTemplateManager::CreateEngineShaderTemplates() {
    fullscreenImageShaderTemplate = GetShaderTemplate("Source/TestD3D12/Shaders/FullscreenImage");
    if (!fullscreenImageShaderTemplate) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to load fullscreen image shader template");
    }

    debugDrawShaderTemplate = GetShaderTemplate("Source/TestD3D12/Shaders/DebugDraw");
    if (!debugDrawShaderTemplate) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to load debug draw shader template");
    }

    depthShaderTemplate = GetShaderTemplate("Source/TestD3D12/Shaders/Depth");
    if (!depthShaderTemplate) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to load depth shader template");
    }

    unlitShaderTemplate = GetShaderTemplate("Source/TestD3D12/Shaders/Unlit");
    if (!unlitShaderTemplate) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to load unlit shader template");
    }

    litShaderTemplate = GetShaderTemplate("Source/TestD3D12/Shaders/Lit");
    if (!litShaderTemplate) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to load lit shader template");
    }
}

void ShaderTemplateManager::DestroyEngineShaderTemplates() {
    if (fullscreenImageShaderTemplate) {
        fullscreenImageShaderTemplate.reset();
    }
    if (debugDrawShaderTemplate) {
        debugDrawShaderTemplate.reset();
    }
    if (depthShaderTemplate) {
        depthShaderTemplate.reset();
    }
    if (unlitShaderTemplate) {
        unlitShaderTemplate.reset();
    }
    if (litShaderTemplate) {
        litShaderTemplate.reset();
    }
}
