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
#include "Core/Heap.h"
#include "Render/Render.h"
#include "RenderInternal.h"
#include "RBackEnd.h"
#include "Render/Font.h"
#include "Core/Cmds.h"
#include "IO/FileSystem.h"
#include "Platform/PlatformTime.h"
#include "Profiler/Profiler.h"

BE_NAMESPACE_BEGIN

RenderGlobal    renderGlobal;
RenderSystem    renderSystem;

void RenderSystem::InitRHI(void *windowHandle) {
    RHI::Settings settings;
    settings.colorBits = cvarSystem.GetCVarInteger("r_colorBits");
    settings.alphaBits = settings.colorBits == 32 ? 8 : 0;
    settings.depthBits = cvarSystem.GetCVarInteger("r_depthBits");
    settings.stencilBits = cvarSystem.GetCVarInteger("r_stencilBits");
    settings.multiSamples = cvarSystem.GetCVarInteger("r_multiSamples");

    // Initialize OpenGL renderer.
    rhi.Init(windowHandle, &settings);
}

void RenderSystem::Init() {
    cmdSystem.AddCommand("screenshot", Cmd_ScreenShot);
    cmdSystem.AddCommand("genDFGSumGGX", Cmd_GenerateDFGSumGGX);

    // Save current gamma ramp table.
    rhi.GetGammaRamp(savedGammaRamp);

    if (r_fastSkinning.GetInteger() == 2 && rhi.HWLimit().maxVertexTextureImageUnits > 0) {
        renderGlobal.skinningMethod = SkinningJointCache::SkinningMethod::VertexTextureFetch;
    } else if (r_fastSkinning.GetInteger() >= 1) {
        renderGlobal.skinningMethod = SkinningJointCache::SkinningMethod::VertexShader;
    } else {
        renderGlobal.skinningMethod = SkinningJointCache::SkinningMethod::Cpu;
    }

    if (r_vertexTextureUpdate.GetInteger() == 2 && rhi.SupportsTextureBuffer()) {
        renderGlobal.vertexTextureMethod = BufferCacheManager::VertexTextureMethod::Tbo;
    } else if (r_vertexTextureUpdate.GetInteger() >= 1 && rhi.SupportsPixelBuffer()) {
        renderGlobal.vertexTextureMethod = BufferCacheManager::VertexTextureMethod::Pbo;
    } else {
        renderGlobal.vertexTextureMethod = BufferCacheManager::VertexTextureMethod::DirectCopy;
    }

    if (r_instancing.GetInteger() == 2 && rhi.SupportsInstancedArrays() && rhi.SupportsMultiDrawIndirect()) {
        renderGlobal.instancingMethod = Mesh::InstancingMethod::InstancedArrays;
        renderGlobal.instanceBufferOffsetAlignment = 64;
    } else if (r_instancing.GetInteger() >= 1) {
        renderGlobal.instancingMethod = Mesh::InstancingMethod::UniformBuffer;
        renderGlobal.instanceBufferOffsetAlignment = rhi.HWLimit().uniformBufferOffsetAlignment;
    } else {
        renderGlobal.instancingMethod = Mesh::InstancingMethod::NoInstancing;
        renderGlobal.instanceBufferOffsetAlignment = 0;
    }

    renderGlobal.instanceBufferData = Mem_Alloc16(8192 * renderGlobal.instanceBufferOffsetAlignment);
    memset(renderGlobal.instanceBufferData, 0, sizeof(8192 * renderGlobal.instanceBufferOffsetAlignment));

    textureManager.Init();

    fontManager.Init();

    shaderManager.Init();

    materialManager.Init();

    //skinManager.Init();

    bufferCacheManager.Init();

    VertexFormat::Init();

    skeletonManager.Init();

    meshManager.Init();

    particleSystemManager.Init();

    animManager.Init();

    RB_Init();

    frameData.Init();

    primaryWorld = nullptr;

    currentContext = nullptr;
    mainContext = nullptr;

    r_gamma.SetModified();
    r_swapInterval.SetModified();

    initialized = true;
}

void RenderSystem::Shutdown() {
    cmdSystem.RemoveCommand("screenshot");
    cmdSystem.RemoveCommand("genDFGSumGGX");

    frameData.Shutdown();

    bufferCacheManager.Shutdown();

    RB_Shutdown();

    animManager.Shutdown();

    particleSystemManager.Shutdown();

    meshManager.Shutdown();

    skeletonManager.Shutdown();

    VertexFormat::Shutdown();

    //skinManager.Shutdown();

    materialManager.Shutdown();

    fontManager.Shutdown();

    shaderManager.Shutdown();

    textureManager.Shutdown();

    Mem_AlignedFree(renderGlobal.instanceBufferData);

    rhi.Shutdown();

    initialized = false;
}

bool RenderSystem::IsFullscreen() const {
    return rhi.IsFullscreen();
}

void RenderSystem::SetGamma(double gamma) {
    unsigned short ramp[768];

    Clamp(gamma, 0.5, 3.0);
    double one_gamma = 1.0 / gamma;

    double div = (double)(1.0 / 255.0);

    for (int i = 0; i < 256; i++) {
        unsigned int value = (int)(65535 * pow((double)(i + 0.5) * div, one_gamma));
        Clamp(value, (unsigned int)0, (unsigned int)65535);

        ramp[i] = ramp[i + 256] = ramp[i + 512] = (unsigned short)value;
    }

    rhi.SetGammaRamp(ramp);
}

void RenderSystem::RestoreGamma() {
    rhi.SetGammaRamp(savedGammaRamp);
}

RenderContext *RenderSystem::AllocRenderContext(bool isMainContext) {
    RenderContext *rc = new RenderContext;
    renderContexts.Append(rc);

    if (isMainContext) {
        mainContext = rc;
    }
    return rc;
}

void RenderSystem::FreeRenderContext(RenderContext *rc) {
    if (mainContext == rc) {
        mainContext = nullptr;
    }

    renderContexts.Remove(rc);
    delete rc;
}

void RenderSystem::BeginCommands(RenderContext *renderContext) {
    BE_PROFILE_CPU_SCOPE_STATIC("RenderSystem::BeginCommands");

    renderSystem.currentContext = renderContext;

    rhi.SetContext(renderContext->GetContextHandle());

#ifdef ENABLE_IMGUI
    rhi.ImGuiBeginFrame(renderContext->GetContextHandle());
#endif

    bufferCacheManager.BeginWrite();

    BeginContextRenderCommand *cmd = (BeginContextRenderCommand *)renderSystem.GetCommandBuffer(sizeof(BeginContextRenderCommand));
    cmd->commandId = RenderCommand::BeginContext;
    cmd->renderContext = renderContext;
}

void RenderSystem::EndCommands() {
    BE_PROFILE_CPU_SCOPE_STATIC("RenderSystem::EndCommands");

    bufferCacheManager.BeginBackEnd();

    renderSystem.IssueCommands();

    bufferCacheManager.EndWrite();

    frameData.ToggleFrame();

#ifdef ENABLE_IMGUI
    rhi.ImGuiEndFrame();
#endif

    renderSystem.currentContext = nullptr;
}

RenderWorld *RenderSystem::AllocRenderWorld() {
    RenderWorld *renderWorld = new RenderWorld;
    renderWorld->index = renderWorldCount++;

    renderWorld->AddDistantEnvProbe();

    renderWorld->SetSkyboxMaterial(materialManager.defaultSkyboxMaterial);

    primaryWorld = renderWorld;

    return renderWorld;
}

void RenderSystem::FreeRenderWorld(RenderWorld *renderWorld) {
    if (renderWorld) {
        delete renderWorld;
        renderWorld = nullptr;
    }
}

void *RenderSystem::GetCommandBuffer(int bytes) {
    RenderCommandBuffer *cmds = frameData.GetCommands();

    if (cmds->used + bytes + sizeof(RenderCommand) >= RenderCommandBufferSize) {
        BE_WARNLOG("RenderSystem::GetCommandBuffer: not enough command buffer space\n");
        return nullptr;
    }

    cmds->used += bytes;

    return cmds->data + cmds->used - bytes;
}

void RenderSystem::CmdDrawCamera(const VisCamera *camera) {
    DrawCameraRenderCommand *cmd = (DrawCameraRenderCommand *)GetCommandBuffer(sizeof(DrawCameraRenderCommand));
    if (!cmd) {
        return;
    }

    cmd->commandId      = RenderCommand::DrawCamera;
    cmd->camera         = *camera;
}

void RenderSystem::CmdScreenshot(int x, int y, int width, int height, const char *filename) {
    ScreenShotRenderCommand *cmd = (ScreenShotRenderCommand *)GetCommandBuffer(sizeof(ScreenShotRenderCommand));
    if (!cmd) {
        return;
    }

    cmd->commandId      = RenderCommand::ScreenShot;
    cmd->x              = x;
    cmd->y              = y;
    cmd->width          = width;
    cmd->height         = height;
    Str::Copynz(cmd->filename, filename, COUNT_OF(cmd->filename));
}

void RenderSystem::IssueCommands() {
    BE_PROFILE_CPU_SCOPE_STATIC("RenderSystem::IssueCommands");

    RenderCommandBuffer *cmds = frameData.GetCommands();
    // Add an end-of-list command.
    *(int *)(cmds->data + cmds->used) = RenderCommand::End;

    // Clear it out, in case this is a sync and not a buffer flip.
    cmds->used = 0;

    if (!r_skipBackEnd.GetBool()) {
        RB_Execute(cmds->data);
    }
}

void RenderSystem::RecreateScreenMapRT() {
    for (int i = 0; i < renderContexts.Count(); i++) {
        RenderContext *rc = renderContexts[i];
        rc->FreeScreenMapRT();
        rc->InitScreenMapRT();
    }
}

void RenderSystem::RecreateHDRMapRT() {
    for (int i = 0; i < renderContexts.Count(); i++) {
        RenderContext *rc = renderContexts[i];
        rc->FreeHdrMapRT();
        rc->InitHdrMapRT();
    }
}

void RenderSystem::RecreateShadowMapRT() {
    for (int i = 0; i < renderContexts.Count(); i++) {
        RenderContext *rc = renderContexts[i];
        rc->FreeShadowMapRT();
        rc->InitShadowMapRT();
    }
}

void RenderSystem::CheckModifiedCVars() {
    if (r_gamma.IsModified()) {
        r_gamma.ClearModified();

        SetGamma(r_gamma.GetFloat());
    }
    
    if (TextureManager::texture_filter.IsModified()) {
        TextureManager::texture_filter.ClearModified();

        textureManager.SetFilter(TextureManager::texture_filter.GetString());
    }

    if (TextureManager::texture_anisotropy.IsModified()) {
        TextureManager::texture_anisotropy.ClearModified();

        textureManager.SetAnisotropy(TextureManager::texture_anisotropy.GetFloat());
    }

    if (TextureManager::texture_lodBias.IsModified()) {
        TextureManager::texture_lodBias.ClearModified();

        textureManager.SetLodBias(TextureManager::texture_lodBias.GetFloat());
    }

    if (r_swapInterval.IsModified()) {
        r_swapInterval.ClearModified();

        rhi.SwapInterval(r_swapInterval.GetInteger());
    }

    if (r_useDeferredLighting.IsModified()) {
        r_useDeferredLighting.ClearModified();

        bool foundDefine = shaderManager.FindGlobalHeader("#define USE_DEFERRED_LIGHTING\n");

        if (r_useDeferredLighting.GetBool()) {
            if (!foundDefine) {
                shaderManager.AddGlobalHeader("#define USE_DEFERRED_LIGHTING\n");
                shaderManager.ReloadShaders();
                RecreateScreenMapRT();
            }
        } else {
            if (foundDefine) {
                shaderManager.RemoveGlobalHeader("#define USE_DEFERRED_LIGHTING\n");
                shaderManager.ReloadShaders();
                RecreateScreenMapRT();
            }
        }
    }

    if (r_usePostProcessing.IsModified()) {
        r_usePostProcessing.ClearModified();

        r_HDR.SetModified();
        r_DOF.SetModified();
        r_SSAO.SetModified();
        r_sunShafts.SetModified();
        r_motionBlur.SetModified();
    }

    if (r_HDR.IsModified()) {
        r_HDR.ClearModified();

        /*if (r_usePostProcessing.GetBool() && r_HDR.GetInteger() == 1) {
            if (!shaderManager.FindGlobalHeader("#define LOGLUV_HDR\n")) {
                shaderManager.AddGlobalHeader("#define LOGLUV_HDR\n");
                shaderManager.ReloadShaders();
            }
        } else {
            if (shaderManager.FindGlobalHeader("#define LOGLUV_HDR\n")) {
                shaderManager.RemoveGlobalHeader("#define LOGLUV_HDR\n");
                shaderManager.ReloadShaders();
            }
        }*/

        RecreateScreenMapRT();
        RecreateHDRMapRT();
    }

    if (r_HDR_toneMapOp.IsModified()) {
        r_HDR_toneMapOp.ClearModified();

        shaderManager.RemoveGlobalHeader(va("#define TONE_MAPPING_OPERATOR"));

        shaderManager.AddGlobalHeader(va("#define TONE_MAPPING_OPERATOR %i\n", r_HDR_toneMapOp.GetInteger()));
        
        shaderManager.originalShaders[ShaderManager::HdrFinalShader]->Reload();
    }

    if (r_motionBlur.IsModified()) {
        r_motionBlur.ClearModified();

        bool foundDefine = shaderManager.FindGlobalHeader("#define OBJECT_MOTION_BLUR\n");
        
        if (r_usePostProcessing.GetBool() && (r_motionBlur.GetInteger() & 2)) {
            if (!foundDefine) {
                shaderManager.AddGlobalHeader("#define OBJECT_MOTION_BLUR\n");
                shaderManager.ReloadShaders();
                meshManager.ReinstantiateSkinnedMeshes();
                RecreateScreenMapRT();
            }
        } else {
            if (foundDefine) {
                shaderManager.RemoveGlobalHeader("#define OBJECT_MOTION_BLUR\n");
                shaderManager.ReloadShaders();
                meshManager.ReinstantiateSkinnedMeshes();
                RecreateScreenMapRT();
            }
        }
    }

    if (r_SSAO_quality.IsModified()) {
        r_SSAO_quality.ClearModified();

        bool foundDefine = shaderManager.FindGlobalHeader("#define HIGH_QUALITY_SSAO\n");
        
        if (r_usePostProcessing.GetBool() && r_SSAO_quality.GetInteger() > 0) {
            if (!foundDefine) {
                shaderManager.AddGlobalHeader("#define HIGH_QUALITY_SSAO\n");
                shaderManager.ReloadShaders();
            }
        } else {
            if (foundDefine) {
                shaderManager.RemoveGlobalHeader("#define HIGH_QUALITY_SSAO\n");
                shaderManager.ReloadShaders();
            }
        }
    }

    if (r_probeBoxProjection.IsModified()) {
        r_probeBoxProjection.ClearModified();

        bool foundDefine = shaderManager.FindGlobalHeader("#define SPECULAR_PROBE_BOX_PROJECTION\n");

        if (r_probeBoxProjection.GetBool()) {
            if (!foundDefine) {
                shaderManager.AddGlobalHeader("#define SPECULAR_PROBE_BOX_PROJECTION\n");
                shaderManager.ReloadLitSurfaceShaders();
            }
        } else {
            if (foundDefine) {
                shaderManager.RemoveGlobalHeader("#define SPECULAR_PROBE_BOX_PROJECTION\n");
                shaderManager.ReloadLitSurfaceShaders();
            }
        }
    }

    if (r_probeBlending.IsModified()) {
        r_probeBlending.ClearModified();

        bool foundDefine = shaderManager.FindGlobalHeader("#define PROBE_BLENDING\n");

        if (r_probeBlending.GetBool()) {
            if (!foundDefine) {
                shaderManager.AddGlobalHeader("#define PROBE_BLENDING\n");
                shaderManager.ReloadLitSurfaceShaders();
            }
        } else {
            if (foundDefine) {
                shaderManager.RemoveGlobalHeader("#define PROBE_BLENDING\n");
                shaderManager.ReloadLitSurfaceShaders();
            }
        }
    }

    if (r_specularEnergyCompensation.IsModified()) {
        r_specularEnergyCompensation.ClearModified();

        bool foundDefine = shaderManager.FindGlobalHeader("#define USE_MULTIPLE_SCATTERING_COMPENSATION\n");

        if (r_specularEnergyCompensation.GetBool()) {
            if (!foundDefine) {
                shaderManager.AddGlobalHeader("#define USE_MULTIPLE_SCATTERING_COMPENSATION\n");
                shaderManager.ReloadLitSurfaceShaders();
            }
        } else {
            if (foundDefine) {
                shaderManager.RemoveGlobalHeader("#define USE_MULTIPLE_SCATTERING_COMPENSATION\n");
                shaderManager.ReloadLitSurfaceShaders();
            }
        }
    }

    if (r_shadowMapSize.IsModified()) {
        r_shadowMapSize.ClearModified();
        RecreateShadowMapRT();
    }

    if (r_shadowCubeMapFloat.IsModified()) {
        r_shadowCubeMapFloat.ClearModified();
        RecreateShadowMapRT();
    }

    if (r_shadowCubeMapSize.IsModified()) {
        r_shadowCubeMapSize.ClearModified();
        RecreateShadowMapRT();
    }

    if (r_shadows.IsModified()) {
        r_shadows.ClearModified();

        bool foundDefine = shaderManager.FindGlobalHeader("#define USE_SHADOW_MAP\n");

        if (r_shadows.GetInteger() == 1) {
            if (!foundDefine) {
                shaderManager.AddGlobalHeader("#define USE_SHADOW_MAP\n");
                shaderManager.ReloadLitSurfaceShaders();
                RecreateShadowMapRT();
            }
        } else {
            if (foundDefine) {
                shaderManager.RemoveGlobalHeader("#define USE_SHADOW_MAP\n");
                shaderManager.ReloadLitSurfaceShaders();
            }
        }
    }

    if (r_showShadows.IsModified()) {
        r_showShadows.ClearModified();

        bool foundDefine = shaderManager.FindGlobalHeader("#define DEBUG_CASCADE_SHADOW_MAP\n");

        if (r_showShadows.GetInteger() == 1) {
            if (!foundDefine) {
                shaderManager.AddGlobalHeader("#define DEBUG_CASCADE_SHADOW_MAP\n");
                shaderManager.ReloadLitSurfaceShaders();
            }
        } else {
            if (foundDefine) {
                shaderManager.RemoveGlobalHeader("#define DEBUG_CASCADE_SHADOW_MAP\n");
                shaderManager.ReloadLitSurfaceShaders();
            }
        }
    }

    if (r_CSM_count.IsModified()) {
        r_CSM_count.ClearModified();

        shaderManager.RemoveGlobalHeader(va("#define CSM_COUNT "));
        
        shaderManager.AddGlobalHeader(va("#define CSM_COUNT %i\n", r_CSM_count.GetInteger()));
        shaderManager.ReloadLitSurfaceShaders();

        RecreateShadowMapRT();
    }

    if (r_CSM_selectionMethod.IsModified()) {
        r_CSM_selectionMethod.ClearModified();

        shaderManager.RemoveGlobalHeader(va("#define CASCADE_SELECTION_METHOD "));
        
        shaderManager.AddGlobalHeader(va("#define CASCADE_SELECTION_METHOD %i\n", r_CSM_selectionMethod.GetInteger()));
        shaderManager.ReloadLitSurfaceShaders();

        RecreateShadowMapRT();
    }

    if (r_CSM_blend.IsModified()) {
        r_CSM_blend.ClearModified();

        bool foundDefine = shaderManager.FindGlobalHeader("#define CASCADE_BLENDING\n");

        if (r_CSM_blend.GetBool()) {
            if (!foundDefine) {
                shaderManager.AddGlobalHeader("#define CASCADE_BLENDING\n");
                shaderManager.ReloadLitSurfaceShaders();
            }
        } else {
            if (foundDefine) {
                shaderManager.RemoveGlobalHeader("#define CASCADE_BLENDING\n");
                shaderManager.ReloadLitSurfaceShaders();
            }
        }
    }

    if (r_shadowMapQuality.IsModified()) {
        r_shadowMapQuality.ClearModified();
        
        shaderManager.RemoveGlobalHeader(va("#define SHADOW_MAP_QUALITY "));
        
        if (r_shadowMapQuality.GetInteger() >= 0) {
            shaderManager.AddGlobalHeader(va("#define SHADOW_MAP_QUALITY %i\n", r_shadowMapQuality.GetInteger()));
            shaderManager.ReloadLitSurfaceShaders();
        }
    }
}

void RenderSystem::UpdateEnvProbes() {
    BE_PROFILE_CPU_SCOPE_STATIC("RenderSystem::UpdateEnvProbes");

    // Needs any render context to render environment cubemap
    if (!renderSystem.renderContexts.Count()) {
        return;
    }

    for (int i = 0; i < envProbeJobs.Count(); ) {
        EnvProbeJob *job = &envProbeJobs[i];

        if (job->Refresh(job->envProbe->GetTimeSlicing())) {
            envProbeJobs.RemoveIndexFast(i);
        } else {
            i++;
        }
    }
}

void RenderSystem::ScheduleToRefreshEnvProbe(RenderWorld *renderWorld, int probeHandle) {
    for (int i = 0; i < envProbeJobs.Count(); i++) {
        const EnvProbeJob *job = &envProbeJobs[i];

        if (job->GetRenderWorld() == renderWorld && job->GetEnvProbe() == renderWorld->GetEnvProbe(probeHandle)) {
            return;
        }
    }

    EnvProbeJob job;
    job.renderWorld = renderWorld;
    job.envProbe = renderWorld->GetEnvProbe(probeHandle);
    job.specularProbeCubemapMaxLevel = Math::Log(2, job.envProbe->GetSize());

    envProbeJobs.Append(job);
}

void RenderSystem::ForceToRefreshEnvProbe(RenderWorld *renderWorld, int probeHandle) {
    EnvProbeJob job;
    job.renderWorld = renderWorld;
    job.envProbe = renderWorld->GetEnvProbe(probeHandle);
    job.specularProbeCubemapMaxLevel = Math::Log(2, job.envProbe->GetSize());

    job.envProbe->bounces = 0;

    int numPasses = r_probeBakeBounces.GetInteger();
    while (numPasses--) {
        job.Refresh(EnvProbe::TimeSlicing::NoTimeSlicing);

        job.diffuseProbeCubemapComputed = false;
        job.specularProbeCubemapComputedLevel = -1;
        job.specularProbeCubemapComputedLevel0Face = -1;
    }
}

void RenderSystem::CaptureScreenRT(RenderWorld *renderWorld, int layerMask, 
    bool colorClear, const Color4 &clearColor, const Vec3 &origin, const Mat3 &axis, float fov, int width, int height, RenderTarget *targetRT) {
    RenderCamera renderCamera;
    RenderCamera::State cameraDef;

    memset(&cameraDef, 0, sizeof(cameraDef));
    cameraDef.flags = RenderCamera::Flag::TexturedMode | RenderCamera::Flag::NoSubViews | RenderCamera::Flag::SkipDebugDraw | RenderCamera::Flag::SkipPostProcess;
    cameraDef.clearMethod = colorClear ? RenderCamera::ClearMethod::Color : RenderCamera::ClearMethod::Skybox;
    cameraDef.clearColor = clearColor;
    cameraDef.layerMask = layerMask;
    cameraDef.origin = origin;
    cameraDef.axis = axis;
    cameraDef.renderRect.Set(0, 0, width, height);

    AABB worldAABB = renderWorld->GetStaticAABB();
    if (!worldAABB.IsCleared()) {
        Vec3 v;
        worldAABB.GetFarthestVertexFromDir(axis[0], v);
        cameraDef.zFar = Max(MeterToUnit(100.0f), origin.Distance(v));
    } else {
        cameraDef.zFar = MeterToUnit(100.0f);
    }
    cameraDef.zNear = CmToUnit(5.0f);

    RenderCamera::ComputeFov(fov, 1.25f, (float)width / height, &cameraDef.fovX, &cameraDef.fovY);

    // Use any render context.
    RenderContext *renderContext = renderSystem.renderContexts[0];

    renderCamera.Update(&cameraDef);

    targetRT->Begin();

    renderSystem.BeginCommands(renderContext);

    renderWorld->RenderScene(&renderCamera);

    renderSystem.EndCommands();

    targetRT->End();
}

Texture *RenderSystem::CaptureScreenTexture(RenderWorld *renderWorld, int layerMask, 
    bool colorClear, const Color4 &clearColor, const Vec3 &origin, const Mat3 &axis, float fov, bool useHDR, int width, int height) {
    Texture *screenTexture = new Texture;
    screenTexture->CreateEmpty(RHI::TextureType::Texture2D, width, height, 1, 1, 1, 
        useHDR ? Image::Format::RGB_11F_11F_10F : Image::Format::RGB_8_8_8,
        Texture::Flag::Clamp | Texture::Flag::NoMipmaps | Texture::Flag::HighQuality);

    RenderTarget *screenRT = RenderTarget::Create(screenTexture, nullptr, RHI::RenderTargetFlag::HasDepthBuffer);

    CaptureScreenRT(renderWorld, layerMask, colorClear, clearColor, origin, axis, fov, width, height, screenRT);

    RenderTarget::Delete(screenRT);

    return screenTexture;
}

void RenderSystem::CaptureScreenImage(RenderWorld *renderWorld, int layerMask, 
    bool colorClear, const Color4 &clearColor, const Vec3 &origin, const Mat3 &axis, float fov, bool useHDR, int width, int height, Image &screenImage) {
    Texture *screenTexture = CaptureScreenTexture(renderWorld, layerMask, colorClear, clearColor, origin, axis, fov, useHDR, width, height);

    Image::GammaSpace::Enum gammaSpace = Image::NeedFloatConversion(screenTexture->GetFormat()) ? Image::GammaSpace::Linear : Image::GammaSpace::sRGB;

    screenImage.Create2D(screenTexture->GetWidth(), screenTexture->GetHeight(), 1, screenTexture->GetFormat(), gammaSpace, nullptr, 0);

    screenTexture->Bind();
    screenTexture->GetTexels2D(0, screenTexture->GetFormat(), screenImage.GetPixels(0));
}

void RenderSystem::CaptureEnvCubeFaceRT(RenderWorld *renderWorld, int layerMask, int staticMask, 
    bool colorClear, const Color4 &clearColor,
    const Vec3 &origin, float zNear, float zFar, RenderTarget *targetCubeRT, int faceIndex) {
    //int t0 = PlatformTime::Milliseconds();

    RenderCamera renderCamera;
    RenderCamera::State cameraDef;

    memset(&cameraDef, 0, sizeof(cameraDef));
    cameraDef.flags = RenderCamera::Flag::TexturedMode | RenderCamera::Flag::NoSubViews | RenderCamera::Flag::SkipDebugDraw | RenderCamera::Flag::SkipPostProcess;
    cameraDef.clearMethod = colorClear ? RenderCamera::ClearMethod::Color : RenderCamera::ClearMethod::Skybox;
    cameraDef.clearColor = clearColor;
    cameraDef.layerMask = layerMask;
    cameraDef.staticMask = staticMask;
    cameraDef.zNear = zNear;
    cameraDef.zFar = zFar;
    cameraDef.origin = origin;
    cameraDef.orthogonal = false;
    cameraDef.fovX = 90;
    cameraDef.fovY = 90;
    cameraDef.renderRect.Set(0, 0, targetCubeRT->GetWidth(), targetCubeRT->GetHeight());

    if (staticMask) {
        cameraDef.flags |= RenderCamera::Flag::StaticOnly;
    }

    // Use any render context
    RenderContext *renderContext = renderSystem.renderContexts[0];

    R_EnvCubeMapFaceToEngineAxis((RHI::CubeMapFace::Enum)faceIndex, cameraDef.axis);

    renderCamera.Update(&cameraDef);

    targetCubeRT->Begin(0, faceIndex);

    renderSystem.BeginCommands(renderContext);

    renderWorld->RenderScene(&renderCamera);

    renderSystem.EndCommands();

    targetCubeRT->End();

    //int t1 = PlatformTime::Milliseconds();
    //BE_LOG("CaptureEnvCubeFaceRT(%i) takes %ims\n", faceIndex, t1 - t0);
}

void RenderSystem::CaptureEnvCubeRT(RenderWorld *renderWorld, int layerMask, int staticMask, 
    bool colorClear, const Color4 &clearColor, const Vec3 &origin, float zNear, float zFar, RenderTarget *targetCubeRT) {
    for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
        CaptureEnvCubeFaceRT(renderWorld, layerMask, staticMask, colorClear, clearColor, origin, zNear, zFar, targetCubeRT, faceIndex);
    }
}

Texture *RenderSystem::CaptureEnvCubeTexture(RenderWorld *renderWorld, int layerMask, int staticMask, 
    bool colorClear, const Color4 &clearColor, const Vec3 &origin, float zNear, float zFar, bool useHDR, int size) {
    Texture *envCubeTexture = new Texture;
    envCubeTexture->CreateEmpty(RHI::TextureType::TextureCubeMap, size, size, 1, 1, 1, 
        useHDR ? Image::Format::RGB_11F_11F_10F : Image::Format::RGB_8_8_8,
        Texture::Flag::Clamp | Texture::Flag::NoMipmaps | Texture::Flag::HighQuality);

    RenderTarget *envCubeRT = RenderTarget::Create(envCubeTexture, nullptr, RHI::RenderTargetFlag::HasDepthBuffer);

    CaptureEnvCubeRT(renderWorld, layerMask, staticMask, colorClear, clearColor, origin, zNear, zFar, envCubeRT);

    RenderTarget::Delete(envCubeRT);

    return envCubeTexture;
}

void RenderSystem::CaptureEnvCubeImage(RenderWorld *renderWorld, int layerMask, int staticMask, bool colorClear, const Color4 &clearColor,
    const Vec3 &origin, float zNear, float zFar, bool useHDR, int size, Image &envCubeImage) {
    Texture *envCubeTexture = CaptureEnvCubeTexture(renderWorld, layerMask, staticMask, colorClear, clearColor, origin, zNear, zFar, useHDR, size);

    Texture::GetCubeImageFromCubeTexture(envCubeTexture, 1, envCubeImage);

    delete envCubeTexture;
}

void RenderSystem::GenerateSHConvolvIrradianceEnvCubeRT(const Texture *envCubeTexture, RenderTarget *targetCubeRT) const {
    //-------------------------------------------------------------------------------
    // Create 4-by-4 envmap sized block weight map for each faces
    //------------------------------------------------------------------------------- 
    int envMapSize = envCubeTexture->GetWidth();
    Texture *weightTextures[6];

    float *weightData = (float *)Mem_Alloc(envMapSize * 4 * envMapSize * 4 * sizeof(float));
    float invSize = 1.0f / (envMapSize - 1);

    for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
        for (int y = 0; y < envMapSize; y++) {
            for (int x = 0; x < envMapSize; x++) {
                float s = (x + 0.5f) * invSize;
                float t = (y + 0.5f) * invSize;

                // Gets sample direction for each faces.
                Vec3 dir = Image::FaceToCubeMapCoords((Image::CubeMapFace::Enum)faceIndex, s, t).Normalized();

                // 9 terms are required for order 3 SH basis functions.
                float basisEval[16] = { 0, };
                // Evaluates the 9 SH basis functions Ylm with the given direction.
                SphericalHarmonics::EvalBasis(3, dir, basisEval);

                // Solid angle of the cubemap texel.
                float dw = Image::CubeMapTexelSolidAngle(x, y, envMapSize);

                // Precalculates 9 terms (basisEval * dw) for each envmap pixel in the 4-by-4 envmap sized block texture for each faces.
                for (int j = 0; j < 4; j++) {
                    for (int i = 0; i < 4; i++) {
                        int offset = (((j * envMapSize + y) * envMapSize) << 2) + i * envMapSize + x;

                        weightData[offset] = basisEval[(j << 2) + i] * dw;
                    }
                }
            }
        }

        weightTextures[faceIndex] = new Texture;
        weightTextures[faceIndex]->Create(RHI::TextureType::Texture2D,
            Image(envMapSize * 4, envMapSize * 4, 1, 1, 1, Image::Format::L_32F, Image::GammaSpace::Linear, (byte *)weightData, 0),
            Texture::Flag::Clamp | Texture::Flag::Nearest | Texture::Flag::NoMipmaps | Texture::Flag::HighQuality);
    }

    Mem_Free(weightData);

    //-------------------------------------------------------------------------------
    // SH projection of (Li * dw) and create 9 coefficents in a single 4x4 texture
    //-------------------------------------------------------------------------------
    Shader *weightedSHProjShader = shaderManager.GetShader("Shaders/WeightedSHProj")->InstantiateShader(Array<Shader::Define>());

    Image image;
    image.Create2D(4, 4, 1, Image::Format::RGB_32F_32F_32F, Image::GammaSpace::Linear, nullptr, 0);
    Texture *incidentCoeffTexture = new Texture;
    incidentCoeffTexture->Create(RHI::TextureType::Texture2D, image, Texture::Flag::Clamp | Texture::Flag::Nearest | Texture::Flag::NoMipmaps | Texture::Flag::HighQuality);

    RenderTarget *incidentCoeffRT = RenderTarget::Create(incidentCoeffTexture, nullptr, 0);

    incidentCoeffRT->Begin();

    rhi.SetViewport(Rect(0, 0, 4, 4));
    rhi.SetStateBits(RHI::ColorWrite);
    rhi.SetCullFace(RHI::CullType::None);

    weightedSHProjShader->Bind();
    weightedSHProjShader->SetTextureArray("weightMap", 6, (const Texture **)weightTextures);
    weightedSHProjShader->SetTexture("radianceCubeMap", envCubeTexture);
    weightedSHProjShader->SetConstant1i("radianceCubeMapSize", envCubeTexture->GetWidth());
    weightedSHProjShader->SetConstant1f("radianceScale", 1.0f);

    RB_DrawClipRect(0, 0, 1.0f, 1.0f);

    //rhi.ReadPixels(0, 0, 4, 4, Image::RGB_32F_32F_32F, image.GetPixels());

    incidentCoeffRT->End();

    shaderManager.ReleaseShader(weightedSHProjShader);
    shaderManager.ReleaseShader(weightedSHProjShader->GetOriginalShader());

    for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
        SAFE_DELETE(weightTextures[faceIndex]);
    }

    //-------------------------------------------------------------------------------
    // SH convolution
    //-------------------------------------------------------------------------------
    Shader *genDiffuseCubeMapSHConvolv = shaderManager.GetShader("Shaders/GenIrradianceEnvCubeMapSHConvolv")->InstantiateShader(Array<Shader::Define>());

    int size = targetCubeRT->GetWidth();

    // Precompute ZH coefficients * sqrt(4PI/(2l + 1)) of Lambert diffuse spherical function cos(theta) / PI
    // which function is rotationally symmetric so only 3 terms are needed.
    float al[3];
    al[0] = SphericalHarmonics::Lambert_Al_Evaluator(0); // 1
    al[1] = SphericalHarmonics::Lambert_Al_Evaluator(1); // 2/3
    al[2] = SphericalHarmonics::Lambert_Al_Evaluator(2); // 1/4

    for (int faceIndex = RHI::CubeMapFace::PositiveX; faceIndex <= RHI::CubeMapFace::NegativeZ; faceIndex++) {
        targetCubeRT->Begin(0, faceIndex);

        rhi.SetViewport(Rect(0, 0, size, size));
        rhi.SetStateBits(RHI::ColorWrite);
        rhi.SetCullFace(RHI::CullType::None);

        genDiffuseCubeMapSHConvolv->Bind();
        genDiffuseCubeMapSHConvolv->SetTexture("incidentCoeffMap", incidentCoeffRT->ColorTexture());
        genDiffuseCubeMapSHConvolv->SetConstant1i("targetCubeMapSize", size);
        genDiffuseCubeMapSHConvolv->SetConstant1i("targetCubeMapFace", faceIndex);
        genDiffuseCubeMapSHConvolv->SetConstantArray1f("lambertCoeff", COUNT_OF(al), al);

        RB_DrawClipRect(0, 0, 1.0f, 1.0f);

        targetCubeRT->End();
    }

    SAFE_DELETE(incidentCoeffTexture);
    RenderTarget::Delete(incidentCoeffRT);

    shaderManager.ReleaseShader(genDiffuseCubeMapSHConvolv);
    shaderManager.ReleaseShader(genDiffuseCubeMapSHConvolv->GetOriginalShader());
}

void RenderSystem::GenerateIrradianceEnvCubeRT(const Texture *envCubeTexture, RenderTarget *targetCubeRT) const {
    //int t0 = PlatformTime::Milliseconds();

    Shader *genDiffuseCubeMapShader = shaderManager.GetShader("Shaders/GenIrradianceEnvCubeMap")->InstantiateShader(Array<Shader::Define>());

    int size = targetCubeRT->GetWidth();

    for (int faceIndex = RHI::CubeMapFace::PositiveX; faceIndex <= RHI::CubeMapFace::NegativeZ; faceIndex++) {
        targetCubeRT->Begin(0, faceIndex);

        rhi.SetViewport(Rect(0, 0, size, size));
        rhi.SetStateBits(RHI::ColorWrite);
        rhi.SetCullFace(RHI::CullType::None);

        genDiffuseCubeMapShader->Bind();
        genDiffuseCubeMapShader->SetTexture("radianceCubeMap", envCubeTexture);
        genDiffuseCubeMapShader->SetConstant1i("radianceCubeMapSize", envCubeTexture->GetWidth());
        genDiffuseCubeMapShader->SetConstant1i("targetCubeMapSize", size);
        genDiffuseCubeMapShader->SetConstant1i("targetCubeMapFace", faceIndex);

        RB_DrawClipRect(0, 0, 1.0f, 1.0f);

        targetCubeRT->End();
    }

    shaderManager.ReleaseShader(genDiffuseCubeMapShader);
    shaderManager.ReleaseShader(genDiffuseCubeMapShader->GetOriginalShader());

    //int t1 = PlatformTime::Milliseconds();
    //BE_LOG("GenerateIrradianceEnvCubeRT() takes %ims\n", t1 - t0);
}

void RenderSystem::GeneratePhongSpecularLDSumRT(const Texture *envCubeTexture, int maxSpecularPower, RenderTarget *targetCubeRT) const {
    Shader *genLDSumPhongSpecularShader = shaderManager.GetShader("Shaders/GenLDSumPhongSpecular")->InstantiateShader(Array<Shader::Define>());

    int size = targetCubeRT->GetWidth();

    int numMipLevels = Math::Log(2, size) + 1;

    // power drop range [maxSpecularPower, 2]
    float powerDropOnMip = Math::Pow(maxSpecularPower, -1.0f / numMipLevels);

    for (int faceIndex = RHI::CubeMapFace::PositiveX; faceIndex <= RHI::CubeMapFace::NegativeZ; faceIndex++) {
        float specularPower = maxSpecularPower;

        for (int mipLevel = 0; mipLevel < numMipLevels; mipLevel++) {
            int mipSize = size >> mipLevel;

            targetCubeRT->Begin(mipLevel, faceIndex);

            rhi.SetViewport(Rect(0, 0, mipSize, mipSize));
            rhi.SetStateBits(RHI::ColorWrite);
            rhi.SetCullFace(RHI::CullType::None);

            genLDSumPhongSpecularShader->Bind();
            genLDSumPhongSpecularShader->SetTexture("radianceCubeMap", envCubeTexture);
            genLDSumPhongSpecularShader->SetConstant1i("radianceCubeMapSize", envCubeTexture->GetWidth());
            genLDSumPhongSpecularShader->SetConstant1i("targetCubeMapSize", mipSize);
            genLDSumPhongSpecularShader->SetConstant1i("targetCubeMapFace", faceIndex);
            genLDSumPhongSpecularShader->SetConstant1f("specularPower", specularPower);

            RB_DrawClipRect(0, 0, 1.0f, 1.0f);

            targetCubeRT->End();

            specularPower *= powerDropOnMip;
        }
    }

    shaderManager.ReleaseShader(genLDSumPhongSpecularShader);
    shaderManager.ReleaseShader(genLDSumPhongSpecularShader->GetOriginalShader());
}

void RenderSystem::GenerateGGXLDSumRTFirstLevel(const Texture *envCubeTexture, RenderTarget *targetCubeRT) const {
    Shader *passThruCubeFaceShader = shaderManager.GetShader("Shaders/PassThruCubeFace")->InstantiateShader(Array<Shader::Define>());

    int size = targetCubeRT->GetWidth();

    // We can skip complex calculation for mipLevel 0 for perfect specular mirror.
    for (int faceIndex = RHI::CubeMapFace::PositiveX; faceIndex <= RHI::CubeMapFace::NegativeZ; faceIndex++) {
        targetCubeRT->Begin(0, faceIndex);

        rhi.SetViewport(Rect(0, 0, size, size));
        rhi.SetStateBits(RHI::ColorWrite);
        rhi.SetCullFace(RHI::CullType::None);

        passThruCubeFaceShader->Bind();
        passThruCubeFaceShader->SetTexture("cubemap", envCubeTexture);
        passThruCubeFaceShader->SetConstant1i("targetCubeMapSize", size);
        passThruCubeFaceShader->SetConstant1i("targetCubeMapFace", faceIndex);

        RB_DrawClipRect(0, 0, 1.0f, 1.0f);

        targetCubeRT->End();
    }

    shaderManager.ReleaseShader(passThruCubeFaceShader);
    shaderManager.ReleaseShader(passThruCubeFaceShader->GetOriginalShader());
}

void RenderSystem::GenerateGGXLDSumRTRestLevel(const Texture *envCubeTexture, RenderTarget *targetCubeRT, int numMipLevels, int mipLevel) const {
    Shader *genLDSumGGXShader = shaderManager.GetShader("Shaders/GenLDSumGGX")->InstantiateShader(Array<Shader::Define>());

    int size = targetCubeRT->GetWidth();

    for (int faceIndex = RHI::CubeMapFace::PositiveX; faceIndex <= RHI::CubeMapFace::NegativeZ; faceIndex++) {
        int mipSize = size >> mipLevel;

        float roughness = (float)mipLevel / (numMipLevels - 1);

        targetCubeRT->Begin(mipLevel, faceIndex);

        rhi.SetViewport(Rect(0, 0, mipSize, mipSize));
        rhi.SetStateBits(RHI::ColorWrite);
        rhi.SetCullFace(RHI::CullType::None);

        genLDSumGGXShader->Bind();
        genLDSumGGXShader->SetTexture("radianceCubeMap", envCubeTexture);
        genLDSumGGXShader->SetConstant1i("radianceCubeMapSize", envCubeTexture->GetWidth());
        genLDSumGGXShader->SetConstant1i("targetCubeMapSize", mipSize);
        genLDSumGGXShader->SetConstant1i("targetCubeMapFace", faceIndex);
        genLDSumGGXShader->SetConstant1f("roughness", roughness);

        RB_DrawClipRect(0, 0, 1.0f, 1.0f);

        targetCubeRT->End();
    }

    shaderManager.ReleaseShader(genLDSumGGXShader);
    shaderManager.ReleaseShader(genLDSumGGXShader->GetOriginalShader());
}

void RenderSystem::GenerateGGXLDSumRTLevel(const Texture *envCubeTexture, RenderTarget *targetCubeRT, int numMipLevels, int mipLevel) const {
    //int t0 = PlatformTime::Milliseconds();

    if (mipLevel == 0) {
        GenerateGGXLDSumRTFirstLevel(envCubeTexture, targetCubeRT);
    } else {
        GenerateGGXLDSumRTRestLevel(envCubeTexture, targetCubeRT, numMipLevels, mipLevel);
    }

    //int t1 = PlatformTime::Milliseconds();
    //BE_LOG("GenerateGGXLDSumRTLevel(%i/%i) takes %ims\n", mipLevel, numMipLevels, t1 - t0);
}

void RenderSystem::GenerateGGXLDSumRT(const Texture *envCubeTexture, RenderTarget *targetCubeRT) const {
    int size = targetCubeRT->GetWidth();
    int numMipLevels = Math::Log(2, size) + 1;

    for (int mipLevel = 0; mipLevel < numMipLevels; mipLevel++) {
        GenerateGGXLDSumRTLevel(envCubeTexture, targetCubeRT, numMipLevels, mipLevel);
    }
}

void RenderSystem::GenerateGGXDFGSumImage(int size, Image &integrationImage) const {
    Shader *genDFGSumGGXShader = shaderManager.GetShader("Shaders/GenDFGSumGGX")->InstantiateShader(Array<Shader::Define>());

    Texture *integrationLutTexture = new Texture;
    integrationLutTexture->CreateEmpty(RHI::TextureType::Texture2D, size, size, 1, 1, 1, Image::Format::RG_16F_16F,
        Texture::Flag::Clamp | Texture::Flag::Nearest | Texture::Flag::NoMipmaps | Texture::Flag::HighQuality);

    RenderTarget *integrationLutRT = RenderTarget::Create(integrationLutTexture, nullptr, 0);

    integrationLutRT->Begin(0, 0);

    rhi.SetViewport(Rect(0, 0, size, size));
    rhi.SetStateBits(RHI::ColorWrite);
    rhi.SetCullFace(RHI::CullType::None);

    genDFGSumGGXShader->Bind();

    RB_DrawClipRect(0, 0, 1.0f, 1.0f);

    integrationImage.Create2D(size, size, 1, Image::Format::RG_16F_16F, Image::GammaSpace::Linear, nullptr, 0);

    rhi.ReadPixels(0, 0, size, size, Image::Format::RG_16F_16F, integrationImage.GetPixels());

    integrationLutRT->End();

    SAFE_DELETE(integrationLutTexture);

    RenderTarget::Delete(integrationLutRT);

    shaderManager.ReleaseShader(genDFGSumGGXShader);
    shaderManager.ReleaseShader(genDFGSumGGXShader->GetOriginalShader());
}

void RenderSystem::WriteGGXDFGSum(const char *filename, int size) const {
    Image integrationImage;
    GenerateGGXDFGSumImage(size, integrationImage);

    char path[256];
    Str::snPrintf(path, sizeof(path), "%s.dds", filename);
    integrationImage.WriteDDS(path);

    BE_LOG("Generated GGX integration LUT to \"%s\"\n", path);
}

void RenderSystem::TakeScreenShot(const char *filename, RenderWorld *renderWorld, int layerMask, const Vec3 &origin, const Mat3 &axis, float fov, bool useHDR, int width, int height) {
    Image screenImage;
    CaptureScreenImage(renderWorld, layerMask, false, Color4::black, origin, axis, fov, useHDR, width, height, screenImage);

    char path[256];
    Str::snPrintf(path, sizeof(path), "%s.png", filename);
    screenImage.WritePNG(path);

    BE_LOG("Screenshot saved to \"%s\"\n", path);
}

void RenderSystem::TakeEnvShot(const char *filename, RenderWorld *renderWorld, int layerMask, int staticMask, const Vec3 &origin, bool useHDR, int size) {
    Image envCubeImage;
    CaptureEnvCubeImage(renderWorld, layerMask, staticMask, false, Color4::black, origin, CmToUnit(5.0f), MeterToUnit(100.0f), useHDR, size, envCubeImage);

    char path[256];
    Str::snPrintf(path, sizeof(path), "%s.dds", filename);
    //envCubeImage.ConvertFormatSelf(Image::RGB_11F_11F_10F, false, Image::HighQuality);
    envCubeImage.WriteDDS(path);

    BE_LOG("Environment cubemap snapshot saved to \"%s\"\n", path);
}

void RenderSystem::TakeIrradianceEnvShot(const char *filename, RenderWorld *renderWorld, int layerMask, int staticMask, const Vec3 &origin, bool useHDR, int size, int envSize) {
    Texture *envCubeTexture = CaptureEnvCubeTexture(renderWorld, layerMask, staticMask, false, Color4::black, origin, CmToUnit(5.0f), MeterToUnit(100.0f), useHDR, envSize);

    Texture *irradianceEnvCubeTexture = new Texture;
    irradianceEnvCubeTexture->CreateEmpty(RHI::TextureType::TextureCubeMap, size, size, 1, 1, 1, 
        useHDR ? Image::Format::RGB_11F_11F_10F : Image::Format::RGB_8_8_8,
        Texture::Flag::Clamp | Texture::Flag::Nearest | Texture::Flag::NoMipmaps | Texture::Flag::HighQuality);
    RenderTarget *irradianceEnvCubeRT = RenderTarget::Create(irradianceEnvCubeTexture, nullptr, 0);
#if 1
    GenerateIrradianceEnvCubeRT(envCubeTexture, irradianceEnvCubeRT);
#else
    GenerateSHConvolvIrradianceEnvCubeRT(envCubeTexture, irradianceEnvCubeRT);
#endif

    RenderTarget::Delete(irradianceEnvCubeRT);

    delete envCubeTexture;

    Image irradianceEnvCubeImage;
    Texture::GetCubeImageFromCubeTexture(irradianceEnvCubeTexture, 1, irradianceEnvCubeImage);

    delete irradianceEnvCubeTexture;

    char path[256];
    Str::snPrintf(path, sizeof(path), "%s.dds", filename);
    //irradianceEnvCubeImage.ConvertFormatSelf(Image::RGB_11F_11F_10F, Image::GammaSpace::DontCare, false, Image::HighQuality);
    irradianceEnvCubeImage.WriteDDS(path);

    BE_LOG("Generated diffuse irradiance cubemap to \"%s\"\n", path);
}

void RenderSystem::TakePrefilteredEnvShot(const char *filename, RenderWorld *renderWorld, int layerMask, int staticMask, const Vec3 &origin, bool useHDR, int size, int envSize) {
    Texture *envCubeTexture = CaptureEnvCubeTexture(renderWorld, layerMask, staticMask, false, Color4::black, origin, CmToUnit(5.0f), MeterToUnit(100.0f), useHDR, envSize);

    int numMipLevels = Math::Log(2, size) + 1;

    Texture *prefilteredCubeTexture = new Texture;
    prefilteredCubeTexture->CreateEmpty(RHI::TextureType::TextureCubeMap, size, size, 1, 1, numMipLevels,
        useHDR ? Image::Format::RGB_11F_11F_10F : Image::Format::RGB_8_8_8,
        Texture::Flag::Clamp | Texture::Flag::Nearest | Texture::Flag::NoMipmaps | Texture::Flag::HighQuality);
    RenderTarget *prefilteredCubeRT = RenderTarget::Create(prefilteredCubeTexture, nullptr, 0);
#if 1
    GenerateGGXLDSumRT(envCubeTexture, prefilteredCubeRT);
#else
    GeneratePhongSpecularLDSumRT(envCubeTexture, 2048, prefilteredCubeRT);
#endif

    RenderTarget::Delete(prefilteredCubeRT);

    delete envCubeTexture;

    Image prefilteredCubeImage;
    Texture::GetCubeImageFromCubeTexture(prefilteredCubeTexture, numMipLevels, prefilteredCubeImage);

    delete prefilteredCubeTexture;

    char path[256];
    Str::snPrintf(path, sizeof(path), "%s.dds", filename);
    //prefilteredCubeImage.ConvertFormatSelf(Image::RGB_11F_11F_10F, Image::GammaSpace::DontCare, false, Image::HighQuality);
    prefilteredCubeImage.WriteDDS(path);

    BE_LOG("Generated specular prefiltered cubemap to \"%s\"\n", path);
}

//--------------------------------------------------------------------------------------------------

void RenderSystem::Cmd_GenerateDFGSumGGX(const CmdArgs &args) {
    Str path = PlatformFile::ExecutablePath();
    path.AppendPath("../../../Data/EngineTextures");
    path.CleanPath();

    if (args.Argc() > 1) {
        path.AppendPath(args.Argv(1));
    } else {
        path.AppendPath("DFGSumGGX");
    }

    const int size = 512;

    renderSystem.WriteGGXDFGSum(path, size);
}

void RenderSystem::Cmd_ScreenShot(const CmdArgs &args) {
    char path[1024];

    Str documentDir = fileSystem.GetUserDocumentDir();
    
    if (args.Argc() > 1) {
        Str::snPrintf(path, sizeof(path), "%s/Screenshots/%s", documentDir.c_str(), args.Argv(1));
    } else {
        char filename[16];
        strcpy(filename, "shot000.png");

        int index;
        for (index = 0; index <= 999; index++) {
            filename[4] = '0' + index / 100;
            filename[5] = '0' + (index % 100) / 10;
            filename[6] = '0' + index % 10;
            Str::snPrintf(path, sizeof(path), "%s/Screenshots/%s", documentDir.c_str(), filename);
            
            if (!fileSystem.FileExists(path)) {
                break;
            }
        }

        if (index == 1000) {
            BE_WARNLOG("too many screenshot exist\n");
            return;
        }
    }

    renderSystem.CmdScreenshot(0, 0, renderSystem.currentContext->GetDeviceWidth(), renderSystem.currentContext->GetDeviceHeight(), path);
}

BE_NAMESPACE_END
