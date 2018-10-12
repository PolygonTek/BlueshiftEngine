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
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

renderGlobal_t      renderGlobal;
RenderSystem        renderSystem;

void RenderSystem::Init(void *windowHandle, const RHI::Settings *settings) {
    cmdSystem.AddCommand(L"screenshot", Cmd_ScreenShot);

    // Initialize OpenGL renderer
    rhi.Init(windowHandle, settings);

    // Save current gamma ramp table
    rhi.GetGammaRamp(savedGammaRamp);

    if (r_fastSkinning.GetInteger() == 2 && rhi.HWLimit().maxVertexTextureImageUnits > 0) {
        renderGlobal.skinningMethod = SkinningJointCache::VertexTextureFetchSkinning;
    } else if (r_fastSkinning.GetInteger() == 1) {
        renderGlobal.skinningMethod = SkinningJointCache::VertexShaderSkinning;
    } else {
        renderGlobal.skinningMethod = SkinningJointCache::CpuSkinning;
    }

    if (r_vertexTextureUpdate.GetInteger() == 2 && rhi.SupportsTextureBufferObject()) {
        renderGlobal.vtUpdateMethod = BufferCacheManager::TboUpdate;
    } else if (r_vertexTextureUpdate.GetInteger() == 1 && rhi.SupportsPixelBufferObject()) {
        renderGlobal.vtUpdateMethod = BufferCacheManager::PboUpdate;
    } else {
        renderGlobal.vtUpdateMethod = BufferCacheManager::DirectCopyUpdate;
    }

    if (r_instancing.GetInteger() == 2 && rhi.SupportsInstancedArrays() && rhi.SupportsMultiDrawIndirect()) {
        renderGlobal.instancingMethod = Mesh::InstancedArraysInstancing;
        renderGlobal.instanceBufferOffsetAlignment = 64;
    } else if (r_instancing.GetInteger() == 1) {
        renderGlobal.instancingMethod = Mesh::UniformBufferInstancing;
        renderGlobal.instanceBufferOffsetAlignment = rhi.HWLimit().uniformBufferOffsetAlignment;
    } else {
        renderGlobal.instancingMethod = Mesh::NoInstancing;
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
    cmdSystem.RemoveCommand(L"screenshot");

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

RenderWorld *RenderSystem::AllocRenderWorld() {
    RenderWorld *renderWorld = new RenderWorld;

    this->primaryWorld = renderWorld;
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
        BE_WARNLOG(L"RenderSystem::GetCommandBuffer: not enough command buffer space\n");
        return nullptr;
    }

    cmds->used += bytes;

    return cmds->data + cmds->used - bytes;
}

void RenderSystem::CmdDrawView(const VisibleView *visView) {
    DrawViewRenderCommand *cmd = (DrawViewRenderCommand *)GetCommandBuffer(sizeof(DrawViewRenderCommand));
    if (!cmd) {
        return;
    }

    cmd->commandId      = DrawViewCommand;
    cmd->view           = *visView;
}

void RenderSystem::CmdScreenshot(int x, int y, int width, int height, const char *filename) {
    ScreenShotRenderCommand *cmd = (ScreenShotRenderCommand *)GetCommandBuffer(sizeof(ScreenShotRenderCommand));
    if (!cmd) {
        return;
    }

    cmd->commandId      = ScreenShotCommand;
    cmd->x              = x;
    cmd->y              = y;
    cmd->width          = width;
    cmd->height         = height;
    Str::Copynz(cmd->filename, filename, COUNT_OF(cmd->filename));
}

void RenderSystem::IssueCommands() {
    RenderCommandBuffer *cmds = frameData.GetCommands();
    // add an end-of-list command
    *(int *)(cmds->data + cmds->used) = EndOfCommand;

    // clear it out, in case this is a sync and not a buffer flip
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

        textureManager.SetFilter(WStr::ToStr(TextureManager::texture_filter.GetString()));
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

        if (r_useDeferredLighting.GetBool()) {
            if (!shaderManager.FindGlobalHeader("#define USE_DEFERRED_LIGHTING\n")) {
                shaderManager.AddGlobalHeader("#define USE_DEFERRED_LIGHTING\n");
                shaderManager.ReloadShaders();

                RecreateScreenMapRT();
            }
        } else {
            if (shaderManager.FindGlobalHeader("#define USE_DEFERRED_LIGHTING\n")) {
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

    if (r_motionBlur.IsModified()) {
        r_motionBlur.ClearModified();
        
        if (r_usePostProcessing.GetBool() && (r_motionBlur.GetInteger() & 2)) {
            if (!shaderManager.FindGlobalHeader("#define OBJECT_MOTION_BLUR\n")) {
                shaderManager.AddGlobalHeader("#define OBJECT_MOTION_BLUR\n");
                shaderManager.ReloadShaders();

                meshManager.ReinstantiateSkinnedMeshes();

                RecreateScreenMapRT();
            }
        } else {
            if (shaderManager.FindGlobalHeader("#define OBJECT_MOTION_BLUR\n")) {
                shaderManager.RemoveGlobalHeader("#define OBJECT_MOTION_BLUR\n");
                shaderManager.ReloadShaders();

                meshManager.ReinstantiateSkinnedMeshes();

                RecreateScreenMapRT();
            }
        }
    }

    if (r_SSAO_quality.IsModified()) {
        r_SSAO_quality.ClearModified();
        
        if (r_usePostProcessing.GetBool() && r_SSAO_quality.GetInteger() > 0) {
            if (!shaderManager.FindGlobalHeader("#define HIGH_QUALITY_SSAO\n")) {
                shaderManager.AddGlobalHeader("#define HIGH_QUALITY_SSAO\n");
                shaderManager.ReloadShaders();
            }
        } else {
            if (shaderManager.FindGlobalHeader("#define HIGH_QUALITY_SSAO\n")) {
                shaderManager.RemoveGlobalHeader("#define HIGH_QUALITY_SSAO\n");
                shaderManager.ReloadShaders();
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

        if (r_shadows.GetInteger() == 1) {
            if (!shaderManager.FindGlobalHeader("#define USE_SHADOW_MAP\n")) {
                shaderManager.AddGlobalHeader("#define USE_SHADOW_MAP\n");
                shaderManager.ReloadLitSurfaceShaders();

                RecreateShadowMapRT();
            }
        } else {
            if (shaderManager.FindGlobalHeader("#define USE_SHADOW_MAP\n")) {
                shaderManager.RemoveGlobalHeader("#define USE_SHADOW_MAP\n");
                shaderManager.ReloadLitSurfaceShaders();
            }
        }
    }

    if (r_showShadows.IsModified()) {
        r_showShadows.ClearModified();

        if (r_showShadows.GetInteger() == 1) {
            if (!shaderManager.FindGlobalHeader("#define DEBUG_CASCADE_SHADOW_MAP\n")) {
                shaderManager.AddGlobalHeader("#define DEBUG_CASCADE_SHADOW_MAP\n");

                shaderManager.ReloadLitSurfaceShaders();
            }
        } else {
            if (shaderManager.FindGlobalHeader("#define DEBUG_CASCADE_SHADOW_MAP\n")) {
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

        if (r_CSM_blend.GetBool()) {
            if (!shaderManager.FindGlobalHeader("#define BLEND_CASCADE\n")) {
                shaderManager.AddGlobalHeader("#define BLEND_CASCADE\n");
                shaderManager.ReloadLitSurfaceShaders();
            }
        } else {
            if (shaderManager.FindGlobalHeader("#define BLEND_CASCADE\n")) {
                shaderManager.RemoveGlobalHeader("#define BLEND_CASCADE\n");
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

//--------------------------------------------------------------------------------------------------

void RenderSystem::Cmd_ScreenShot(const CmdArgs &args) {
    char path[1024];

    Str documentDir = fileSystem.GetDocumentDir();
    
    if (args.Argc() > 1) {
        Str::snPrintf(path, sizeof(path), "%s/Screenshots/%ls", documentDir.c_str(), args.Argv(1));
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
            BE_WARNLOG(L"too many screenshot exist\n");
            return;
        }
    }

    renderSystem.CmdScreenshot(0, 0, renderSystem.currentContext->GetDeviceWidth(), renderSystem.currentContext->GetDeviceHeight(), path);
}

BE_NAMESPACE_END
