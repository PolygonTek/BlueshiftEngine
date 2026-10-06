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
#include "Render/RenderContext.h"
#include "Render/RenderCvars.h"
#include "ShaderTemplateManager.h"
#include "ShaderPipelineCache.h"

ShaderTemplateManager       shaderTemplateManager;

static constexpr char       SpecularEnergyCompensationDefine[] = "USE_MULTIPLE_SCATTERING_COMPENSATION=1";
static constexpr char       MultiBounceAODefine[] = "USE_MULTI_BOUNCE_AO=1";
static constexpr char       ShadowRotationTextureDefine[] = "USE_SHADOW_ROTATION_TEXTURE=1";

void ShaderTemplateManager::Init() {
    shaderTemplateHashMap.Init(128, 128, 128);

    expiredShaderTemplateHashMapCursor = 0;

    globalDefines.Clear();

    InitGlobalDefines();

    globalDefineModCount = 0;
    lastInvalidatedGlobalDefineModCount = 0;

    CreateEngineShaderTemplates();
}

void ShaderTemplateManager::Shutdown() {
    DestroyEngineShaderTemplates();

    shaderTemplateHashMap.Clear();
    ShaderTemplate::reloaded.Clear();

    {
        BE1::ScopedLock lock(globalDefinesMutex);
        globalDefines.Clear();
        globalDefineModCount = 0;
        lastInvalidatedGlobalDefineModCount = 0;
    }
}

void ShaderTemplateManager::UpdateFrame() {
    CleanExpiredShaderTemplates();
}

void ShaderTemplateManager::ProcessPendingGlobalDefineChanges() {
    assert(BE1::Engine::IsInMainThread());
    assert(RenderContext::activeContext == nullptr);

    if (r_specularEnergyCompensation2.IsModified()) {
        r_specularEnergyCompensation2.ClearModified();

        const bool defineExists = FindGlobalDefine(SpecularEnergyCompensationDefine);
        if (r_specularEnergyCompensation2.GetBool()) {
            if (!defineExists) {
                AddGlobalDefine(SpecularEnergyCompensationDefine);
            }
        } else if (defineExists) {
            RemoveGlobalDefine(SpecularEnergyCompensationDefine);
        }
    }

    if (r_aoMultiBounce.IsModified()) {
        r_aoMultiBounce.ClearModified();

        const bool defineExists = FindGlobalDefine(MultiBounceAODefine);
        if (r_aoMultiBounce.GetBool()) {
            if (!defineExists) {
                AddGlobalDefine(MultiBounceAODefine);
            }
        } else if (defineExists) {
            RemoveGlobalDefine(MultiBounceAODefine);
        }
    }

    if (r_shadowMapQuality2.IsModified()) {
        r_shadowMapQuality2.ClearModified();
        const int quality = r_shadowMapQuality2.GetInteger();
        if (shadowMapQuality != quality) {
            shadowMapQuality = quality;
            RemoveGlobalDefine("SHADOW_MAP_QUALITY");
            AddGlobalDefine(BE1::va("SHADOW_MAP_QUALITY=%i", shadowMapQuality));
        }
    }

    if (r_shadowMapRotationUseTexture.IsModified()) {
        r_shadowMapRotationUseTexture.ClearModified();

        const bool defineExists = FindGlobalDefine(ShadowRotationTextureDefine);
        if (r_shadowMapRotationUseTexture.GetBool()) {
            if (!defineExists) {
                AddGlobalDefine(ShadowRotationTextureDefine);
            }
        } else if (defineExists) {
            RemoveGlobalDefine(ShadowRotationTextureDefine);
        }
    }

    if (r_shadowDebug.IsModified()) {
        r_shadowDebug.ClearModified();
        const int debugMode = r_shadowDebug.GetInteger();
        if (shadowDebugMode != debugMode) {
            shadowDebugMode = debugMode;
            RemoveGlobalDefine("DEBUG_CASCADED_SHADOW_MAP");
            AddGlobalDefine(BE1::va("DEBUG_CASCADED_SHADOW_MAP=%i", shadowDebugMode));
        }
    }

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
}

SharedPtr<ShaderTemplate> ShaderTemplateManager::AllocShaderTemplate(const char *hashName) {
    if (!hashName || !hashName[0]) {
        BE_FATALERROR("ShaderTemplateManager::AllocShaderTemplate: empty shader template name");
    }
    if (const auto *entry = shaderTemplateHashMap.Get(hashName)) {
        if (!entry->second.expired()) {
            BE_FATALERROR("ShaderTemplateManager::AllocShaderTemplate: %s shader template already allocated", hashName);
        }
        shaderTemplateHashMap.RemoveFast(hashName);
    }

    SharedPtr<ShaderTemplate> shaderTemplate = MakeShared<ShaderTemplate>();
    shaderTemplate->hashName = hashName;
    shaderTemplate->name = hashName;
    shaderTemplate->name.StripPath();
    shaderTemplate->name.StripFileExtension();
    shaderTemplateHashMap.Set(shaderTemplate->hashName, shaderTemplate);

    return shaderTemplate;
}

SharedPtr<ShaderTemplate> ShaderTemplateManager::CreateComputeShaderTemplate(const char *hashName, RHI::ShaderModel shaderModel, const char *filename, const char *baseDir, const char *entryPoint, const char *baseDefines) {
    if (!filename || !filename[0] || !baseDir || !baseDir[0] || !entryPoint || !entryPoint[0]) {
        BE_FATALERROR("ShaderTemplateManager::CreateComputeShaderTemplate: invalid compute shader source");
    }

    BE1::Str shaderTemplateText;
    if (baseDefines && baseDefines[0]) {
        shaderTemplateText.sPrintf(
            "{\n"
            "    baseDefines {\n"
            "        %s\n"
            "    }\n"
            "    stages {\n"
            "        compute %s \"%s\" \"%s\"\n"
            "    }\n"
            "}\n",
            baseDefines, RHI::Renderer::ShaderModelName(shaderModel), filename, entryPoint);
    } else {
        shaderTemplateText.sPrintf(
            "{\n"
            "    stages {\n"
            "        compute %s \"%s\" \"%s\"\n"
            "    }\n"
            "}\n",
            RHI::Renderer::ShaderModelName(shaderModel), filename, entryPoint);
    }

    SharedPtr<ShaderTemplate> shaderTemplate = AllocShaderTemplate(hashName);
    if (!shaderTemplate->Create(shaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("ShaderTemplateManager::CreateComputeShaderTemplate: failed to create shader template '%s'", hashName);
    }
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
        shaderTemplateHashMap.RemoveFast(shaderTemplate->hashName);
        return nullptr;
    }

    return shaderTemplate;
}

void ShaderTemplateManager::InitGlobalDefines() {
#ifdef PACKED_VERTEX_TANGENT_FRAME
    AddGlobalDefine("PACKED_VERTEX_TANGENT_FRAME");
    #ifdef PACKED_TANGENT_FRAME_DIAMOND_ENCODING
        AddGlobalDefine("PACKED_TANGENT_FRAME_DIAMOND_ENCODING");
    #endif
#endif
    AddGlobalDefine(BE1::va("CSM_COUNT=%i", static_cast<int>(RenderContext::CSMCascadeMaxCount)));
    AddGlobalDefine(BE1::va("CASCADE_SELECTION_METHOD=%i", 1));

    shadowMapQuality = r_shadowMapQuality2.GetInteger();
    AddGlobalDefine(BE1::va("SHADOW_MAP_QUALITY=%i", shadowMapQuality));

    if (r_shadowMapRotationUseTexture.GetBool()) {
        AddGlobalDefine(ShadowRotationTextureDefine);
    }

    if (r_specularEnergyCompensation2.GetBool()) {
        AddGlobalDefine(SpecularEnergyCompensationDefine);
    }

    if (r_aoMultiBounce.GetBool()) {
        AddGlobalDefine(MultiBounceAODefine);
    }

    shadowDebugMode = r_shadowDebug.GetInteger();
    AddGlobalDefine(BE1::va("DEBUG_CASCADED_SHADOW_MAP=%i", shadowDebugMode));
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
            shaderTemplateHashMap.RemoveFast(entry->first);
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

    fullscreenImageLinearShaderTemplate = GetShaderTemplate("Source/TestD3D12/Shaders/FullscreenImageLinear");
    if (!fullscreenImageLinearShaderTemplate) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to load fullscreen linear image shader template");
    }

    sceneSnapshotDownsampleShaderTemplate = GetShaderTemplate("Source/TestD3D12/Shaders/SceneSnapshotDownsample");
    if (!sceneSnapshotDownsampleShaderTemplate) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to load scene snapshot downsample shader template");
    }

    hdr10OutputShaderTemplate = GetShaderTemplate("Source/TestD3D12/Shaders/HDROutput/HDR10Output");
    if (!hdr10OutputShaderTemplate) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to load HDR10 output shader template");
    }

    displayP3EDROutputShaderTemplate = GetShaderTemplate("Source/TestD3D12/Shaders/HDROutput/DisplayP3EDROutput");
    if (!displayP3EDROutputShaderTemplate) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to load Display P3 EDR output shader template");
    }

    debugDrawShaderTemplate = GetShaderTemplate("Source/TestD3D12/Shaders/DebugDraw");
    if (!debugDrawShaderTemplate) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to load debug draw shader template");
    }

    depthShaderTemplate = GetShaderTemplate("Source/TestD3D12/Shaders/Depth");
    if (!depthShaderTemplate) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to load depth shader template");
    }

    shadowDepthShaderTemplate = GetShaderTemplate("Source/TestD3D12/Shaders/ShadowDepth");
    if (!shadowDepthShaderTemplate) {
        BE_FATALERROR("ShaderTemplateManager::CreateEngineShaderTemplates: failed to load shadow depth shader template");
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
    if (fullscreenImageLinearShaderTemplate) {
        fullscreenImageLinearShaderTemplate.reset();
    }
    if (sceneSnapshotDownsampleShaderTemplate) {
        sceneSnapshotDownsampleShaderTemplate.reset();
    }
    if (hdr10OutputShaderTemplate) {
        hdr10OutputShaderTemplate.reset();
    }
    if (displayP3EDROutputShaderTemplate) {
        displayP3EDROutputShaderTemplate.reset();
    }
    if (debugDrawShaderTemplate) {
        debugDrawShaderTemplate.reset();
    }
    if (depthShaderTemplate) {
        depthShaderTemplate.reset();
    }
    if (shadowDepthShaderTemplate) {
        shadowDepthShaderTemplate.reset();
    }
    if (unlitShaderTemplate) {
        unlitShaderTemplate.reset();
    }
    if (litShaderTemplate) {
        litShaderTemplate.reset();
    }
}
