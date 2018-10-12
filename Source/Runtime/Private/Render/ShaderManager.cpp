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
#include "Render/Render.h"
#include "RenderInternal.h"
#include "Core/Cmds.h"

BE_NAMESPACE_BEGIN

struct engineShader_t {
    const char *filename;
};

static const engineShader_t originalShaderList[] = {
    { "Shaders/drawArrayTexture" },
    { "Shaders/Simple" },
    { "Shaders/selectionId" },
    { "Shaders/depth" },
    { "Shaders/constantColor" },
    { "Shaders/vertexColor" },
    { "Shaders/objectMotionBlur" },

    { "Shaders/StandardSpec" },
    { "Shaders/Standard" },
    { "Shaders/Phong" },

    { "Shaders/skyboxCubemap" },
    { "Shaders/skyboxSixSided" },

    { "Shaders/fogLight" },
    { "Shaders/blendLight" },
    
    { "Shaders/postObjectMotionBlur" },
    { "Shaders/postCameraMotionBlur" },
    { "Shaders/passThru" },
    { "Shaders/passThruColor" },
    { "Shaders/downscale2x2" },
    { "Shaders/downscale4x4" },
    { "Shaders/downscale4x4LogLum" },
    { "Shaders/downscale4x4ExpLum" },
    { "Shaders/blur5x" },
    { "Shaders/blur7x" },
    { "Shaders/blur15x" },
    { "Shaders/blurBilinear3x" },
    { "Shaders/blurBilinear4x" },
    { "Shaders/blurBilinear8x" },
    { "Shaders/blurAlphaMaskedBilinear8x" },
    { "Shaders/kawaseBlur" },
    { "Shaders/radialBlur" },
    { "Shaders/aoBlur" },
    { "Shaders/colorTransform" },
    { "Shaders/gammaCorrection" },
    { "Shaders/linearizeDepth" },
    { "Shaders/copyDownscaledCocToAlpha" },
    { "Shaders/copyColorAndCoc" },
    { "Shaders/applyDOF" },
    { "Shaders/SunShaftsMaskGen" },
    { "Shaders/SunShaftsGen" },
    { "Shaders/SunShaftsDisplay" },
    { "Shaders/luminanceAdaptation" },
    { "Shaders/brightFilter" },
    { "Shaders/HDRFinal" },
    { "Shaders/chromaShift" },
    { "Shaders/SSAO" },
    { "Shaders/generateHOM" },
    { "Shaders/queryHOM" }
};

ShaderManager       shaderManager;

Shader *            ShaderManager::originalShaders[MaxPredefinedOriginalShaders];

Shader *            ShaderManager::drawArrayTextureShader;
Shader *            ShaderManager::simpleShader;
Shader *            ShaderManager::selectionIdShader;
Shader *            ShaderManager::depthShader;
Shader *            ShaderManager::constantColorShader;
Shader *            ShaderManager::vertexColorShader;
Shader *            ShaderManager::objectMotionBlurShader;
Shader *            ShaderManager::standardDefaultShader;
Shader *            ShaderManager::standardDefaultAmbientLitShader;
Shader *            ShaderManager::standardDefaultDirectLitShader;
Shader *            ShaderManager::standardDefaultAmbientLitDirectLitShader;
Shader *            ShaderManager::skyboxCubemapShader;
Shader *            ShaderManager::skyboxSixSidedShader;
Shader *            ShaderManager::fogLightShader;
Shader *            ShaderManager::blendLightShader;
Shader *            ShaderManager::postObjectMotionBlurShader;
Shader *            ShaderManager::postCameraMotionBlurShader;
Shader *            ShaderManager::postPassThruShader;
Shader *            ShaderManager::postPassThruColorShader;
Shader *            ShaderManager::downscale2x2Shader;
Shader *            ShaderManager::downscale4x4Shader;
Shader *            ShaderManager::downscale4x4LogLumShader;
Shader *            ShaderManager::downscale4x4ExpLumShader;
Shader *            ShaderManager::blur5xShader;
Shader *            ShaderManager::blur7xShader;
Shader *            ShaderManager::blur15xShader;
Shader *            ShaderManager::blurBilinear3xShader;
Shader *            ShaderManager::blurBilinear4xShader;
Shader *            ShaderManager::blurBilinear8xShader;
Shader *            ShaderManager::blurAlphaMaskedBilinear8xShader;
Shader *            ShaderManager::kawaseBlurShader;
Shader *            ShaderManager::radialBlurShader;
Shader *            ShaderManager::aoBlurShader;
Shader *            ShaderManager::postColorTransformShader;
Shader *            ShaderManager::postGammaCorrectionShader;
Shader *            ShaderManager::linearizeDepthShader;
Shader *            ShaderManager::copyDownscaledCocToAlphaShader;
Shader *            ShaderManager::copyColorAndCocShader;
Shader *            ShaderManager::applyDofShader;
Shader *            ShaderManager::sunShaftsMaskGenShader;
Shader *            ShaderManager::sunShaftsGenShader;
Shader *            ShaderManager::sunShaftsDisplayShader;
Shader *            ShaderManager::brightFilterShader;
Shader *            ShaderManager::luminanceAdaptationShader;
Shader *            ShaderManager::hdrFinalShader;
Shader *            ShaderManager::chromaShiftShader;
Shader *            ShaderManager::ssaoShader;
Shader *            ShaderManager::generateHomShader;
Shader *            ShaderManager::queryHomShader;

void ShaderManager::Init() {
    cmdSystem.AddCommand(L"listShaders", Cmd_ListShaders);
    cmdSystem.AddCommand(L"reloadShader", Cmd_ReloadShader);

    shaderHashMap.Init(1024, 128, 128);

    InitGlobalDefines();

    LoadEngineShaders();

    InstantiateEngineShaders();

    //defaultShader = AllocShader("_defaultShader", DefaultShaderGuid);
    //defaultShader->Create(va("{ }", DefaultShaderGuid));
    //defaultShader->permanence = true;
}

void ShaderManager::Shutdown() {
    cmdSystem.RemoveCommand(L"listShaders");
    cmdSystem.RemoveCommand(L"reloadShader");

    for (int i = 0; i < shaderHashMap.Count(); i++) {
        const auto *entry = shaderManager.shaderHashMap.GetByIndex(i);
        Shader *shader = entry->second;
        
        shader->Purge();
    }

    shaderHashMap.DeleteContents(true);

    globalHeaderList.Clear();
}

void ShaderManager::InitGlobalDefines() {
    if (textureManager.texture_useNormalCompression.GetBool()) {
        if (rhi.SupportsTextureCompressionLATC()) {
            shaderManager.AddGlobalHeader("#define LATC_NORMAL\n");
        } else if (rhi.SupportsTextureCompressionETC2()) {
            //shaderManager.AddGlobalHeader("#define ETC2_NORMAL\n");
        } else if (rhi.SupportsTextureCompressionS3TC()) {
            shaderManager.AddGlobalHeader("#define DXT5_XGBR_NORMAL\n");
        }
    }

#ifdef COMPRESSED_VERTEX_NORMAL_TANGENTS
    shaderManager.AddGlobalHeader("#define COMPRESSED_VERTEX_NORMAL_TANGENTS\n");
#endif

    if (r_usePostProcessing.GetBool()) {
        if (r_motionBlur.GetInteger() == 2) {
            shaderManager.AddGlobalHeader("#define OBJECT_MOTION_BLUR\n");
        }

        if (r_SSAO_quality.GetInteger() > 0) {
            shaderManager.AddGlobalHeader("#define HIGH_QUALITY_SSAO\n");
        }
    }

    if (renderGlobal.instancingMethod == Mesh::InstancedArraysInstancing) {
        shaderManager.AddGlobalHeader("#define INSTANCED_ARRAY\n");
        shaderManager.AddGlobalHeader(va("#define INSTANCE_DATA_SIZE %i\n", renderGlobal.instanceBufferOffsetAlignment));
        shaderManager.AddGlobalHeader(va("#define MAX_INSTANCE_COUNT %i\n", r_maxInstancingCount.GetInteger()));
    } else if (renderGlobal.instancingMethod == Mesh::UniformBufferInstancing) {
        shaderManager.AddGlobalHeader(va("#define INSTANCE_DATA_SIZE %i\n", renderGlobal.instanceBufferOffsetAlignment));
        shaderManager.AddGlobalHeader(va("#define MAX_INSTANCE_COUNT %i\n", Min(r_maxInstancingCount.GetInteger(), rhi.HWLimit().maxUniformBlockSize / renderGlobal.instanceBufferOffsetAlignment)));
    }

    if (renderGlobal.skinningMethod == SkinningJointCache::VertexTextureFetchSkinning) {
        shaderManager.AddGlobalHeader("#define VTF_SKINNING\n");
    }

    if (renderGlobal.vtUpdateMethod == BufferCacheManager::TboUpdate) {
        shaderManager.AddGlobalHeader("#define USE_BUFFER_TEXTURE\n");
    }

    shaderManager.AddGlobalHeader(va("#define USE_SRGB_TEXTURE %i\n", TextureManager::texture_sRGB.GetBool() ? 1 : 0));

    if (r_shadows.GetInteger() == 1) {
        shaderManager.AddGlobalHeader("#define USE_SHADOW_MAP\n");
    }

    shaderManager.AddGlobalHeader(va("#define SHADOW_MAP_QUALITY %i\n", r_shadowMapQuality.GetInteger()));
    
    int maxShaderJoints = (rhi.HWLimit().maxVertexUniformComponents - 256) / (4 * 3);
    shaderManager.AddGlobalHeader(va("#define MAX_SHADER_JOINTSX3 %i\n", maxShaderJoints * 3));

    shaderManager.AddGlobalHeader(va("#define CSM_COUNT %i\n", r_CSM_count.GetInteger()));
    shaderManager.AddGlobalHeader(va("#define CASCADE_SELECTION_METHOD %i\n", r_CSM_selectionMethod.GetInteger()));

    if (r_CSM_blend.GetBool()) {
        shaderManager.AddGlobalHeader("#define BLEND_CASCADE\n");
    }

    if (r_showShadows.GetInteger() == 1) {
        shaderManager.AddGlobalHeader("#define DEBUG_CASCADE_SHADOW_MAP\n");
    }
}

void ShaderManager::LoadEngineShaders() {
    for (int i = 0; i < COUNT_OF(originalShaderList); i++) {
        Str filename = originalShaderList[i].filename;

        Shader *shader = AllocShader(filename);
        if (!shader->Load(filename)) {
            DestroyShader(shader);
            BE_FATALERROR(L"Failed to create shader '%hs'", filename.c_str());
        }

        originalShaders[i] = shader;
    }
}

void ShaderManager::InstantiateEngineShaders() {
    Array<Shader::Define> defineArray;

    drawArrayTextureShader = originalShaders[DrawArrayTextureShader]->InstantiateShader(Array<Shader::Define>());

    simpleShader = originalShaders[SimpleShader]->InstantiateShader(Array<Shader::Define>());

    selectionIdShader = originalShaders[SelectionIdShader]->InstantiateShader(Array<Shader::Define>());

    depthShader = originalShaders[DepthShader]->InstantiateShader(Array<Shader::Define>());

    constantColorShader = originalShaders[ConstantColorShader]->InstantiateShader(Array<Shader::Define>());
    vertexColorShader = originalShaders[VertexColorShader]->InstantiateShader(Array<Shader::Define>());

    defineArray.Clear();
    defineArray.Append(Shader::Define("_ALBEDO", 1));
    defineArray.Append(Shader::Define("_SPECULAR", 0));
    defineArray.Append(Shader::Define("_GLOSS", 0));
    defineArray.Append(Shader::Define("_NORMAL", 0));
    defineArray.Append(Shader::Define("_PARALLAX", 0));
    defineArray.Append(Shader::Define("_OCCLUSION", 0));
    defineArray.Append(Shader::Define("_EMISSION", 0));
    standardDefaultShader = originalShaders[StandardSpecShader]->InstantiateShader(defineArray);

    defineArray.Clear();
    defineArray.Append(Shader::Define("_ALBEDO", 1));
    defineArray.Append(Shader::Define("_SPECULAR", 0));
    defineArray.Append(Shader::Define("_GLOSS", 0));
    defineArray.Append(Shader::Define("_NORMAL", 0));
    defineArray.Append(Shader::Define("_PARALLAX", 0));
    defineArray.Append(Shader::Define("_OCCLUSION", 0));
    defineArray.Append(Shader::Define("_EMISSION", 0));
    standardDefaultAmbientLitShader = originalShaders[StandardSpecShader]->ambientLitVersion->InstantiateShader(defineArray);

    defineArray.Clear();
    defineArray.Append(Shader::Define("_ALBEDO", 1));
    defineArray.Append(Shader::Define("_SPECULAR", 0));
    defineArray.Append(Shader::Define("_GLOSS", 0));
    defineArray.Append(Shader::Define("_NORMAL", 0));
    defineArray.Append(Shader::Define("_PARALLAX", 0));
    defineArray.Append(Shader::Define("_OCCLUSION", 0));
    defineArray.Append(Shader::Define("_EMISSION", 0));
    standardDefaultDirectLitShader = originalShaders[StandardSpecShader]->directLitVersion->InstantiateShader(defineArray);

    defineArray.Clear();
    defineArray.Append(Shader::Define("_ALBEDO", 1));
    defineArray.Append(Shader::Define("_SPECULAR", 0));
    defineArray.Append(Shader::Define("_GLOSS", 0));
    defineArray.Append(Shader::Define("_NORMAL", 0));
    defineArray.Append(Shader::Define("_PARALLAX", 0));
    defineArray.Append(Shader::Define("_OCCLUSION", 0));
    defineArray.Append(Shader::Define("_EMISSION", 0));
    standardDefaultAmbientLitDirectLitShader = originalShaders[StandardSpecShader]->ambientLitDirectLitVersion->InstantiateShader(defineArray);

    objectMotionBlurShader = originalShaders[ObjectMotionBlurShader]->InstantiateShader(Array<Shader::Define>());

    skyboxCubemapShader = originalShaders[SkyboxCubemapShader]->InstantiateShader(Array<Shader::Define>());

    skyboxSixSidedShader = originalShaders[SkyboxSixSidedShader]->InstantiateShader(Array<Shader::Define>());

    fogLightShader = originalShaders[FogLightShader]->InstantiateShader(Array<Shader::Define>());
    blendLightShader = originalShaders[BlendLightShader]->InstantiateShader(Array<Shader::Define>());

    postObjectMotionBlurShader = originalShaders[PostObjectMotionBlurShader]->InstantiateShader(Array<Shader::Define>());
    postCameraMotionBlurShader = originalShaders[PostCameraMotionBlurShader]->InstantiateShader(Array<Shader::Define>());
    postPassThruShader = originalShaders[PostPassThruShader]->InstantiateShader(Array<Shader::Define>());
    postPassThruColorShader = originalShaders[PostPassThruColorShader]->InstantiateShader(Array<Shader::Define>());

    downscale2x2Shader = originalShaders[Downscale2x2Shader]->InstantiateShader(Array<Shader::Define>());
    downscale4x4Shader = originalShaders[Downscale4x4Shader]->InstantiateShader(Array<Shader::Define>());
    downscale4x4LogLumShader = originalShaders[Downscale4x4LogLumShader]->InstantiateShader(Array<Shader::Define>());
    downscale4x4ExpLumShader = originalShaders[Downscale4x4ExpLumShader]->InstantiateShader(Array<Shader::Define>());

    blur5xShader = originalShaders[Blur5xShader]->InstantiateShader(Array<Shader::Define>());
    blur7xShader = originalShaders[Blur7xShader]->InstantiateShader(Array<Shader::Define>());
    blur15xShader = originalShaders[Blur15xShader]->InstantiateShader(Array<Shader::Define>());
    blurBilinear3xShader = originalShaders[BlurBilinear3xShader]->InstantiateShader(Array<Shader::Define>());
    blurBilinear4xShader = originalShaders[BlurBilinear4xShader]->InstantiateShader(Array<Shader::Define>());
    blurBilinear8xShader = originalShaders[BlurBilinear8xShader]->InstantiateShader(Array<Shader::Define>());
    blurAlphaMaskedBilinear8xShader = originalShaders[BlurAlphaMaskedBilinear8xShader]->InstantiateShader(Array<Shader::Define>());
    kawaseBlurShader = originalShaders[KawaseBlurShader]->InstantiateShader(Array<Shader::Define>());
    radialBlurShader = originalShaders[RadialBlurShader]->InstantiateShader(Array<Shader::Define>());
    aoBlurShader = originalShaders[AoBlurShader]->InstantiateShader(Array<Shader::Define>());

    postColorTransformShader = originalShaders[PostColorTransformShader]->InstantiateShader(Array<Shader::Define>());
    postGammaCorrectionShader = originalShaders[PostGammaCorrectionShader]->InstantiateShader(Array<Shader::Define>());

    linearizeDepthShader = originalShaders[LinearizeDepthShader]->InstantiateShader(Array<Shader::Define>());

    copyDownscaledCocToAlphaShader = originalShaders[CopyDownscaledCocToAlphaShader]->InstantiateShader(Array<Shader::Define>());
    copyColorAndCocShader = originalShaders[CopyColorAndCocShader]->InstantiateShader(Array<Shader::Define>());

    applyDofShader = originalShaders[ApplyDofShader]->InstantiateShader(Array<Shader::Define>());

    sunShaftsMaskGenShader = originalShaders[SunShaftsMaskGenShader]->InstantiateShader(Array<Shader::Define>());
    sunShaftsGenShader = originalShaders[SunShaftsGenShader]->InstantiateShader(Array<Shader::Define>());
    sunShaftsDisplayShader = originalShaders[SunShaftsDisplayShader]->InstantiateShader(Array<Shader::Define>());

    brightFilterShader = originalShaders[BrightFilterShader]->InstantiateShader(Array<Shader::Define>());
    luminanceAdaptationShader = originalShaders[LuminanceAdaptationShader]->InstantiateShader(Array<Shader::Define>());
    hdrFinalShader = originalShaders[HdrFinalShader]->InstantiateShader(Array<Shader::Define>());

    chromaShiftShader = originalShaders[ChromaShiftShader]->InstantiateShader(Array<Shader::Define>());

    ssaoShader = originalShaders[SsaoShader]->InstantiateShader(Array<Shader::Define>());

    generateHomShader = originalShaders[GenerateHomShader]->InstantiateShader(Array<Shader::Define>());
    queryHomShader = originalShaders[QueryHomShader]->InstantiateShader(Array<Shader::Define>());
}

void ShaderManager::ReloadShaders() {
    for (int i = 0; i < shaderHashMap.Count(); i++) {
        const auto *entry = shaderManager.shaderHashMap.GetByIndex(i);
        Shader *shader = entry->second;

        if (shader->IsOriginalShader()) {
            shader->Reload();
        }
    }
}

void ShaderManager::ReloadLitSurfaceShaders() {
    for (int i = 0; i < shaderHashMap.Count(); i++) {
        const auto *entry = shaderManager.shaderHashMap.GetByIndex(i);
        Shader *shader = entry->second;

        if (shader->IsOriginalShader()) {
            if (shader->flags & Shader::LitSurface) {
                shader->Reload();
            }
        }
    }
}

void ShaderManager::DestroyUnusedShaders() {
    Array<Shader *> removeArray;

    for (int i = 0; i < shaderHashMap.Count(); i++) {
        const auto *entry = shaderHashMap.GetByIndex(i);
        Shader *shader = entry->second;

        if (shader && !shader->permanence && shader->refCount == 0) {
            removeArray.Append(shader);
        }
    }

    for (int i = 0; i < removeArray.Count(); i++) {
        DestroyShader(removeArray[i]);
    }
}

void ShaderManager::DestroyShader(Shader *shader) {
    if (shader->refCount > 1) {
        BE_WARNLOG(L"ShaderManager::DestroyShader: shader '%hs' has %i reference count\n", shader->hashName.c_str(), shader->refCount);
    }

    shaderHashMap.Remove(shader->hashName);

    delete shader;
}

Shader *ShaderManager::AllocShader(const char *hashName) {
    if (shaderHashMap.Get(hashName)) {
        BE_FATALERROR(L"%hs shader already allocated", hashName);
    }

    Shader *shader = new Shader;
    shader->hashName = hashName;
    shader->hashName.StripFileExtension(); // Remove '.shader'
    shader->name = hashName;
    shader->name.StripPath();
    shader->refCount = 1;

    shaderHashMap.Set(shader->hashName, shader);

    return shader;
}

Shader *ShaderManager::FindShader(const char *hashName) const {
    const auto *entry = shaderHashMap.Get(hashName);
    if (entry) {
        return entry->second;
    }

    return nullptr;
}

Shader *ShaderManager::GetShader(const char *hashName) {
    if (!hashName || !hashName[0]) {
        return nullptr;
    }

    Shader *shader = FindShader(hashName);
    if (shader) {
        shader->refCount++;
        return shader;
    }

    shader = AllocShader(hashName);
    if (!shader->Load(hashName)) {
        DestroyShader(shader);
        return nullptr;
    }

    return shader;
}

void ShaderManager::RenameShader(Shader *shader, const Str &newName) {
    if (shader->originalShader) {
        shader = shader->originalShader;
    }

    const auto *entry = shaderHashMap.Get(shader->hashName);
    if (entry) {
        shaderHashMap.Remove(shader->hashName);

        shader->hashName = newName;
        shader->name = newName;
        shader->name.StripPath();
        shader->name.StripFileExtension();

        shaderHashMap.Set(newName, shader);

        for (int i = 0; i < shader->instantiatedShaders.Count(); i++) {
            Shader *instantiatedShader = shader->instantiatedShaders[i];

            shaderHashMap.Remove(instantiatedShader->hashName);

            Str mangledName;
            Shader::MangleNameWithDefineList("@" + newName, instantiatedShader->defineArray, mangledName);

            instantiatedShader->hashName = mangledName;
            instantiatedShader->name = mangledName;
            instantiatedShader->name.StripPath();
            instantiatedShader->name.StripFileExtension();

            shaderHashMap.Set(newName, instantiatedShader);
        }
    }
}

void ShaderManager::ReleaseShader(Shader *shader, bool immediateDestroy) {
    if (shader->permanence) {
        return;
    }

    if (shader->refCount > 0) {
        shader->refCount--;
    }

    if (immediateDestroy && shader->refCount == 0) {
        DestroyShader(shader);
    }
}

void ShaderManager::PrecacheShader(const char *name) {
    Shader *shader = GetShader(name);
    ReleaseShader(shader);
}

bool ShaderManager::FindGlobalHeader(const char *text) const {
    for (int i = 0; i < globalHeaderList.Count(); i++) {
        if (!globalHeaderList[i].Cmpn(text, Str::Length(text))) {
            return true;
        }
    }
    return false;
}

void ShaderManager::AddGlobalHeader(const char *text) {
    globalHeaderList.Append(Str(text));
}

void ShaderManager::RemoveGlobalHeader(const char *text) {
    int removeIndex = -1;

    for (int i = 0; i < globalHeaderList.Count(); i++) {
        if (!globalHeaderList[i].Cmpn(text, Str::Length(text))) {
            removeIndex = i;
            break;
        }
    }

    if (removeIndex >= 0) {
        globalHeaderList.RemoveIndex(removeIndex);
    }
}

void ShaderManager::Cmd_ListShaders(const CmdArgs &args) {
    int count = 0;

    BE_LOG(L"NUM. REF. TYPE NAME\n");

    for (int i = 0; i < shaderManager.shaderHashMap.Count(); i++) {
        const auto *entry = shaderManager.shaderHashMap.GetByIndex(i);
        Shader *shader = entry->second;

        BE_LOG(L"%4d %4d %hs\n", i, shader->refCount, shader->hashName.c_str());

        count++;
    }

    BE_LOG(L"%i total shaders\n", count);
}

void ShaderManager::Cmd_ReloadShader(const CmdArgs &args) {
    if (args.Argc() != 2) {
        BE_LOG(L"reloadShader <filename>\n");
        return;
    }

    if (!WStr::Icmp(args.Argv(1), L"all")) {
        int count = shaderManager.shaderHashMap.Count();

        for (int i = 0; i < count; i++) {
            const auto *entry = shaderManager.shaderHashMap.GetByIndex(i);
            Shader *shader = entry->second;
            if (shader->IsOriginalShader()) {
                shader->Reload();
            }
        }
    } else {
        Shader *shader = shaderManager.FindShader(WStr::ToStr(args.Argv(1)));
        if (!shader) {
            BE_WARNLOG(L"Couldn't find shader to reload \"%ls\"\n", args.Argv(1));
            return;
        }

        shader->Reload();
    }
}

BE_NAMESPACE_END
