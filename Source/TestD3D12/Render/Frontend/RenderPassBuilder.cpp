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
#include "RenderPassBuilder.h"
#include "Render/RenderContext.h"
#include "Render/RenderFrame.h"
#include "Render/RenderInternal.h"
#include "Render/Material/Material.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"
#include "Render/ShaderPipeline/VertexInputLayoutFactory.h"

// float 값을 작은 수에서 큰 수 순으로 정렬할 수 있도록 uint32_t 값을 계산한다.
static uint32_t FloatToAscendingSortKey(float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return (bits & 0x80000000u) ? ~bits : (bits ^ 0x80000000u);
}

// float 값을 큰 수에서 작은 수 순으로 정렬할 수 있도록 uint32_t 값을 계산한다.
static uint32_t FloatToDescendingSortKey(float value) {
    return FloatToAscendingSortKey(value) ^ 0xFFFFFFFFu;
}

static uint64_t BuildOpaqueSortKey(const FrameSurf *frameSurf, RHI::PipelineState *pso) {
    //--------------------------------------------------------------
    // 0xFFFF000000000000 (16비트, 0~65535)  : PSO sort ID
    // 0x0000FFFF00000000 (16비트, 0~65535)  : subMesh index
    // 0x00000000F0000000 ( 4비트, 0~15)     : LOD level
    // 0x000000000FFFF000 (16비트, 0~65535)  : shader material index
    // 0x0000000000000FFF (12비트, 0~4095)   : rough view depth
    //--------------------------------------------------------------
    const uint64_t psoSortId = pso ? (pso->sortId & 0xFFFF) : 0;
    const uint64_t subMeshIndex = frameSurf->subMesh ? (frameSurf->subMesh->GetID() & 0xFFFF) : 0;
    const uint64_t lodLevel = frameSurf->lodLevel & 0xF;
    const uint64_t shaderMaterialIndex = frameSurf->shaderMaterialIndex & 0xFFFF;
    const uint64_t roughDepth = (FloatToDescendingSortKey(frameSurf->viewZ) >> 20) & 0xFFF;

    return (psoSortId << 48) | (subMeshIndex << 32) | (lodLevel << 28) | (shaderMaterialIndex << 12) | roughDepth;
}

static uint64_t BuildDepthSortKey(const FrameSurf *frameSurf) {
    //--------------------------------------------------------------
    // 0x8000000000000000 ( 1비트, 0~1)          : alpha-test bucket
    // 0x7FFFFFFF00000000 (31비트, 0~2147483647) : shader material index (alpha-test only)
    // 0x00000000FFFFFFFF (32비트, 0~4294967295) : near-to-far view depth
    //--------------------------------------------------------------
    const bool alphaTest = frameSurf->baseFeatureSet.HasFeature(ShaderFeature::Perforated);
    const uint64_t alphaTestBucket = alphaTest ? 1ull : 0ull;
    const uint64_t shaderMaterialIndex = alphaTest ? (frameSurf->shaderMaterialIndex & 0x7FFFFFFFull) : 0;
    const uint64_t depthKey = static_cast<uint64_t>(FloatToDescendingSortKey(frameSurf->viewZ));

    return (alphaTestBucket << 63) | (shaderMaterialIndex << 32) | depthKey;
}

static uint64_t BuildTransparentSortKey(const FrameSurf *frameSurf) {
    // viewZ 는 전방이 음수를 나타내므로, 먼 물체에서 가까운 물체 순으로 정렬하도록 한다.
    return FloatToAscendingSortKey(frameSurf->viewZ);
}

static ShaderFeatureSet BuildSurfaceFeatureSet(const FrameSurf *frameSurf, const RenderPass *renderPass, bool directLighting, bool indirectLighting) {
    ShaderFeatureSet featureSet = frameSurf->baseFeatureSet | renderPass->featureSet;
    const SharedPtr<ShaderTemplate> &shaderTemplate = frameSurf->material->GetShaderTemplate();
    if (!shaderTemplate) {
        return featureSet;
    }

    if (directLighting && shaderTemplate->SupportsFeature(ShaderFeature::DirectLighting)) {
        featureSet.EnableFeature(ShaderFeature::DirectLighting);
    }
    if (indirectLighting && shaderTemplate->SupportsFeature(ShaderFeature::IndirectLighting)) {
        featureSet.EnableFeature(ShaderFeature::IndirectLighting);
    }
    return featureSet;
}

static bool IsTransparentSSRReceiver(const FrameSurf *frameSurf) {
    const SharedPtr<ShaderTemplate> &shaderTemplate = frameSurf->material->GetShaderTemplate();
    return shaderTemplate &&
        shaderTemplate->SupportsFeature(ShaderFeature::ApplySSR) &&
        shaderTemplate->SupportsFeature(ShaderFeature::SurfacePrepass) &&
        shaderTemplate->SupportsFeature(ShaderFeature::TransparentSSRPrepass) &&
        shaderTemplate->SupportsFeature(ShaderFeature::OutputNormal) &&
        shaderTemplate->SupportsFeature(ShaderFeature::OutputVelocity) &&
        shaderTemplate->SupportsFeature(ShaderFeature::OutputRoughness);
}

void RenderPassBuilder::AllocRenderPassDrawSurfs(RenderFrame *frame, FrameCamera *frameCamera, uint32_t maxDrawSurfs) {
    assert(!frameCamera->renderPassDrawSurfs);
    assert(!frameCamera->renderPassDrawSurfStorage);

    frameCamera->maxRenderPassDrawSurfs = maxDrawSurfs;
    frameCamera->numRenderPassDrawSurfs = 0;
    frameCamera->renderPassDrawSurfs = (const RenderPassDrawSurf **)frame->MemAlloc(maxDrawSurfs * sizeof(RenderPassDrawSurf *));
    frameCamera->renderPassDrawSurfStorage = (RenderPassDrawSurf *)frame->MemAlloc(maxDrawSurfs * sizeof(RenderPassDrawSurf));
}

RenderPass *RenderPassBuilder::AllocRenderPass(FrameCamera *frameCamera, RenderPassType type, ShaderFeatureSet passFeatureSet, VertexInputLayoutId passBaseInputLayoutId, uint32_t maxDrawSurfs) {
    assert(frameCamera->numRenderPasses < FrameCamera::MaxRenderPasses);
    assert(frameCamera->numRenderPassDrawSurfs + maxDrawSurfs <= frameCamera->maxRenderPassDrawSurfs);

    if (maxDrawSurfs <= 0) {
        return nullptr;
    }

    RenderPass *renderPass = &frameCamera->renderPasses[frameCamera->numRenderPasses++];
    renderPass->type = type;
    renderPass->featureSet = passFeatureSet;
    renderPass->baseInputLayoutId = passBaseInputLayoutId;

    renderPass->graphicsPipelineDesc = {};
    renderPass->graphicsPipelineDesc.inputLayout = &VertexInputLayoutFactory::Get(passBaseInputLayoutId);

    renderPass->maxDrawSurfs = maxDrawSurfs;
    renderPass->numDrawSurfs = 0;
    renderPass->drawSurfs = frameCamera->renderPassDrawSurfs + frameCamera->numRenderPassDrawSurfs;
    renderPass->drawSurfStorage = frameCamera->renderPassDrawSurfStorage + frameCamera->numRenderPassDrawSurfs;
    frameCamera->numRenderPassDrawSurfs += maxDrawSurfs;

    return renderPass;
}

static RHI::DepthStencilStateType GetDepthState(const RHI::ProjectionPolicy &projectionPolicy, bool write) {
    if (projectionPolicy.reverseZ) {
        return write ? RHI::DepthStencilStateType::GEqualWrite : RHI::DepthStencilStateType::GEqualNoWrite;
    }
    return write ? RHI::DepthStencilStateType::LEqualWrite : RHI::DepthStencilStateType::LEqualNoWrite;
}

void RenderPassBuilder::BuildCascadedShadow(RenderFrame *frame, FrameCascadedShadow *cascadedShadow, uint32_t cascadeIndex, const FrameSurf *const *frameSurfs, uint32_t numFrameSurfs, bool allowAsyncPSOBuild) {
    PROFILER_CPU_SCOPED_EVENT("RenderPassBuilder::BuildCascadedShadow");

    assert(cascadedShadow);
    assert(cascadedShadow->depthTexture);
    assert(cascadeIndex < cascadedShadow->cascadeCount);

    CSMCascade &cascade = cascadedShadow->cascades[cascadeIndex];
    assert(cascade.depthDSV.IsValid());

    RenderPass *shadowPass = &cascade.renderPass;
    assert(!shadowPass->drawSurfs && !shadowPass->drawSurfStorage);

    GraphicsPipelineDesc &pipelineDesc = shadowPass->graphicsPipelineDesc;
    pipelineDesc = {};
    pipelineDesc.depthStencilStateType = RHI::DepthStencilStateType::GEqualWrite;
    pipelineDesc.renderDest.renderTargetCount = 0;
    pipelineDesc.renderDest.depthStencilFormat = RenderContext::activeContext->GetCSMDepthFormat();
    pipelineDesc.renderDest.sampleCount = cascadedShadow->depthTexture->GetSampleCount();

    // CSM 패스에서만 depth clipping 을 꺼 near 밖 caster 를 reverse-Z 의 depth 1 에 clamp 한다 (shadow pancaking).
    pipelineDesc.depthClipEnabled = false;

    // 각 cascade 는 독립된 view/DSV/draw 목록과 해당 프레임에 캡처한 bias 를 사용한다.
    const RenderFrameSettings &frameSettings = frame->GetRenderFrameSettings();
    pipelineDesc.depthBiasOverride = true;
    // 양수 설정은 caster 를 광원에서 멀리 민다. reverse-Z 에서는 depth 를 줄여 같은 방향을 유지한다.
    pipelineDesc.constantDepthBias = -frameSettings.csmConstantDepthBias[cascadeIndex];
    pipelineDesc.slopeScaledDepthBias = -frameSettings.csmSlopeScaledDepthBias[cascadeIndex];

    BuildShadow(frame, shadowPass, static_cast<uint8_t>(1u << cascadeIndex), frameSurfs, numFrameSurfs, allowAsyncPSOBuild);
}

void RenderPassBuilder::BuildLocalShadow(RenderFrame *frame, FrameShadowAtlas *shadowAtlas, uint32_t shadowViewIndex, bool allowAsyncPSOBuild) {
    PROFILER_CPU_SCOPED_EVENT("RenderPassBuilder::BuildLocalShadow");

    assert(shadowAtlas);
    assert(shadowAtlas->shadowTexture);
    assert(shadowAtlas->shadowDSV.IsValid());
    assert(shadowViewIndex < shadowAtlas->numShadowViews);

    FrameLocalShadowView &shadowView = shadowAtlas->shadowViews[shadowViewIndex];
    assert(shadowView.shadowIndex < shadowAtlas->numShadows);
    const FrameLight *shadowLight = shadowAtlas->shadows[shadowView.shadowIndex].light;
    assert(shadowLight);

    RenderPass *shadowPass = &shadowView.renderPass;
    assert(!shadowPass->drawSurfs && !shadowPass->drawSurfStorage);

    GraphicsPipelineDesc &pipelineDesc = shadowPass->graphicsPipelineDesc;
    pipelineDesc = {};
    pipelineDesc.depthStencilStateType = RHI::DepthStencilStateType::GEqualWrite;
    pipelineDesc.renderDest.renderTargetCount = 0;
    // ShadowAtlas 의 DSV 포맷. Typeless texture 의 리소스 포맷을 사용하지 않는다.
    pipelineDesc.renderDest.depthStencilFormat = RHI::Format::D32_FLOAT;
    pipelineDesc.renderDest.sampleCount = shadowAtlas->shadowTexture->GetSampleCount();

    // Local shadow 는 유한한 perspective 투영의 near/far 범위로 depth 를 clip 한다.
    pipelineDesc.depthClipEnabled = true;
    pipelineDesc.depthBiasOverride = true;
    // 양수 설정값은 caster 를 광원에서 멀어지게 한다. Reverse-Z 에서는 depth 를 줄인다.
    pipelineDesc.constantDepthBias = -shadowLight->shadowOffsetUnits;
    pipelineDesc.slopeScaledDepthBias = -shadowLight->shadowOffsetFactor;

    // View 별 caster/submesh 컬링을 마친 목록은 CSM mask 와 무관하게 그대로 사용한다.
    BuildShadow(frame, shadowPass, 0, shadowView.shadowSurfs, shadowView.numShadowSurfs, allowAsyncPSOBuild);
}

void RenderPassBuilder::BuildShadow(RenderFrame *frame, RenderPass *shadowPass, uint8_t shadowCascadeMask, const FrameSurf *const *frameSurfs, uint32_t numFrameSurfs, bool allowAsyncPSOBuild) {
    PROFILER_CPU_SCOPED_EVENT("RenderPassBuilder::BuildShadow");

    assert(shadowPass);
    assert(!shadowPass->drawSurfs && !shadowPass->drawSurfStorage);
    assert(frameSurfs || numFrameSurfs == 0);
    assert(shadowPass->graphicsPipelineDesc.renderDest.renderTargetCount == 0);
    assert(shadowPass->graphicsPipelineDesc.renderDest.depthStencilFormat != RHI::Format::Unknown);

    shadowPass->type = RenderPassType::Shadow;
    shadowPass->baseInputLayoutId = VertexInputLayoutId::XyzStColor;
    // Depth 대상과 bias 등 호출자가 준비한 설정은 유지한다.
    shadowPass->graphicsPipelineDesc.inputLayout = &VertexInputLayoutFactory::Get(shadowPass->baseInputLayoutId);

    // Local shadow 는 이미 선별된 목록의 크기를 사용하여 별도의 count 순회를 생략한다.
    uint32_t numShadowDrawSurfs = numFrameSurfs;
    if (shadowCascadeMask != 0) {
        numShadowDrawSurfs = 0;
        for (uint32_t i = 0; i < numFrameSurfs; ++i) {
            if ((frameSurfs[i]->shadowCascadeMask & shadowCascadeMask) != 0) {
                numShadowDrawSurfs++;
            }
        }
    }

    // Caster 가 없는 패스도 depth clear 에 사용할 수 있도록 설정은 유지하고 draw 메모리만 생략한다.
    if (numShadowDrawSurfs == 0) {
        return;
    }

    shadowPass->maxDrawSurfs = numShadowDrawSurfs;
    shadowPass->drawSurfs = (const RenderPassDrawSurf **)frame->MemAlloc(numShadowDrawSurfs * sizeof(RenderPassDrawSurf *));
    shadowPass->drawSurfStorage = (RenderPassDrawSurf *)frame->MemAlloc(numShadowDrawSurfs * sizeof(RenderPassDrawSurf));

    // ShadowViewCB 에 광원 행렬과 surface table 을 전달하는 전용 셰이더를 사용한다.
    shadowPSOResolver.Begin(shadowPass, BE1::Min(numShadowDrawSurfs, 24u), shaderTemplateManager.shadowDepthShaderTemplate);

    for (uint32_t i = 0; i < numFrameSurfs; ++i) {
        const FrameSurf *frameSurf = frameSurfs[i];
        if (shadowCascadeMask != 0 && (frameSurf->shadowCascadeMask & shadowCascadeMask) == 0) {
            continue;
        }

        const RenderPassDepthPSOResolver::ResolveResult rr = shadowPSOResolver.Resolve(frameSurf, allowAsyncPSOBuild);
        AddRenderPassDrawSurf(shadowPass, frameSurf, BuildOpaqueSortKey(frameSurf, rr.pso), rr.inputLayoutId, rr.pso);
    }

    SortRenderPassDrawSurfs(frame, shadowPass);
}

void RenderPassBuilder::BuildMain(RenderFrame *frame, FrameCamera *frameCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderPassBuilder::BuildMain");

    if (frameCamera->numFrameSurfs == 0) {
        return;
    }

    // Forward lighting 에서 불투명 표면을 렌더링하려면 AO 가 필요하다.
    // AO, velocity 또는 roughness 출력이 필요하면 depth prepass 를 강제로 켠다.
    const bool useDepthPrepass = frame->GetRenderFrameSettings().depthPrepass || frameCamera->applyAO || frameCamera->needVelocityOutput || frameCamera->needRoughnessOutput;

    int numDepthDrawSurfs = 0;
    int numOpaqueDrawSurfs = 0;
    int numSkyboxDrawSurfs = 0;
    int numBlendDrawSurfs = 0;
    int numTransparentSSRDrawSurfs = 0;

    for (int i = 0; i < frameCamera->numFrameSurfs; ++i) {
        const FrameSurf *frameSurf = frameCamera->frameSurfs[i];

        if (!BE1::HasFlag(frameSurf->flags, FrameSurf::Flag::CameraVisible)) {
            continue;
        }

        if (BE1::HasFlag(frameSurf->flags, FrameSurf::Flag::Skybox)) {
            numSkyboxDrawSurfs++;
        } else if (frameSurf->material->GetRenderingMode() == Material::RenderingMode::Blend) {
            numBlendDrawSurfs++;
            if (frameCamera->traceTransparentSSR && IsTransparentSSRReceiver(frameSurf)) {
                numTransparentSSRDrawSurfs++;
            }
        } else {
            if (useDepthPrepass) {
                numDepthDrawSurfs++;
            }
            numOpaqueDrawSurfs++;
        }
    }

    // Shadow 전용 surface 만 있다면 메인 패스용 메모리를 할당하지 않는다.
    const uint32_t numMainDrawSurfs = numDepthDrawSurfs + numOpaqueDrawSurfs + numSkyboxDrawSurfs + numBlendDrawSurfs + numTransparentSSRDrawSurfs;
    if (numMainDrawSurfs == 0) {
        return;
    }

    const bool allowAsyncPSOBuild = !BE1::HasFlag(frameCamera->flags, RenderCamera::Flag::ForceSyncPassPSOBuild);
    const bool directLighting = frameCamera->directionalLightVisible || frameCamera->numFrameLocalLights > 0;
    const bool indirectLighting = frameCamera->envLightDiffuseIrradianceCubeMap && frameCamera->envLightSpecularCubeMap;

    // RenderPass 에서 필요한 모든 RenderPassDrawSurf 를 미리 할당한다.
    AllocRenderPassDrawSurfs(frame, frameCamera, numMainDrawSurfs);

    // AllocRenderPass 호출 순서대로 백엔드에서 RenderPass 가 렌더링된다.
    // 1. DepthPrepass RenderPass 를 할당한다.
    ShaderFeatureSet depthPassFeatureSet;
    const bool useSurfacePrepass = frameCamera->needRoughnessOutput;
    if (useSurfacePrepass) {
        depthPassFeatureSet.EnableFeature(ShaderFeature::SurfacePrepass);
        depthPassFeatureSet.EnableFeature(ShaderFeature::OutputRoughness);
    }
    if (frameCamera->needNormalOutput) {
        depthPassFeatureSet.EnableFeature(ShaderFeature::OutputNormal);
    }

    if (frameCamera->needVelocityOutput) {
        depthPassFeatureSet.EnableFeature(ShaderFeature::OutputVelocity);
    }

    const VertexInputLayoutId depthInputLayout = useSurfacePrepass ? VertexInputLayoutId::XyzStColorNormalTangent : (frameCamera->needNormalOutput ? VertexInputLayoutId::XyzStColorNormal : VertexInputLayoutId::XyzStColor);
    RenderPass *depthPrepass = AllocRenderPass(frameCamera, RenderPassType::DepthPrepass, depthPassFeatureSet, depthInputLayout, numDepthDrawSurfs);
    if (depthPrepass) {
        depthPrepass->graphicsPipelineDesc.depthStencilStateType = GetDepthState(frameCamera->projectionPolicy, true);
        RHI::RenderDest &renderDest = depthPrepass->graphicsPipelineDesc.renderDest;
        renderDest = RHI::RenderDest();
        renderDest.renderTargetCount = 0;

        if (frameCamera->needNormalOutput || frameCamera->needRoughnessOutput) {
            renderDest.renderTargetFormats[renderDest.renderTargetCount++] = frameCamera->normalRoughnessTexture->GetFormat();
        }

        if (frameCamera->needVelocityOutput) {
            renderDest.renderTargetFormats[renderDest.renderTargetCount++] = RHI::Format::R16G16_FLOAT;
        }

        renderDest.depthStencilFormat = RenderContext::activeContext->GetMainRTDepthFormat();
        renderDest.sampleCount = frameCamera->depthTexture->GetSampleCount();

        if (useSurfacePrepass) {
            surfacePrepassPSOResolver.Begin(depthPrepass, BE1::Min(numDepthDrawSurfs, 1024));
        } else {
            depthPrepassPSOResolver.Begin(depthPrepass, BE1::Min(numDepthDrawSurfs, 16), shaderTemplateManager.depthPrepassShaderTemplate);
        }
    }

    ShaderFeatureSet opaquePassFeatureSet;
    if (frameCamera->useSSR) {
        opaquePassFeatureSet.EnableFeature(ShaderFeature::ApplySSR);
    }

    // 2. Opaque RenderPass 를 할당한다.
    RenderPass *opaquePass = AllocRenderPass(frameCamera, RenderPassType::Opaque, opaquePassFeatureSet, VertexInputLayoutId::XyzStColorNormalTangent, numOpaqueDrawSurfs);
    if (opaquePass) {
        opaquePass->graphicsPipelineDesc.depthStencilStateType = GetDepthState(frameCamera->projectionPolicy, !useDepthPrepass);
        RenderContext::activeContext->GetMainRenderDest(opaquePass->graphicsPipelineDesc.renderDest, false, false, false);

        opaquePSOResolver.Begin(opaquePass, BE1::Min(numOpaqueDrawSurfs, 1024));
    }

    // 3. Skybox RenderPass 를 할당한다.
    RenderPass *skyboxPass = AllocRenderPass(frameCamera, RenderPassType::Skybox, {}, VertexInputLayoutId::XyzStColor, numSkyboxDrawSurfs);
    if (skyboxPass) {
        skyboxPass->graphicsPipelineDesc.depthStencilStateType = GetDepthState(frameCamera->projectionPolicy, false);
        RenderContext::activeContext->GetMainRenderDest(skyboxPass->graphicsPipelineDesc.renderDest, false, false, false);

        skyboxPSOResolver.Begin(skyboxPass, BE1::Min(numSkyboxDrawSurfs, 1));
    }

    // 4. Transparent SSR receiver prepass 를 할당한다. 별도 depth 로 가장 가까운 한 layer 만 보존한다.
    ShaderFeatureSet transparentSSRPrepassFeatureSet;
    transparentSSRPrepassFeatureSet.EnableFeature(ShaderFeature::SurfacePrepass);
    transparentSSRPrepassFeatureSet.EnableFeature(ShaderFeature::TransparentSSRPrepass);
    transparentSSRPrepassFeatureSet.EnableFeature(ShaderFeature::OutputNormal);
    transparentSSRPrepassFeatureSet.EnableFeature(ShaderFeature::OutputVelocity);
    transparentSSRPrepassFeatureSet.EnableFeature(ShaderFeature::OutputRoughness);
    RenderPass *transparentSSRPrepass = AllocRenderPass(frameCamera, RenderPassType::TransparentSSRPrepass, transparentSSRPrepassFeatureSet, VertexInputLayoutId::XyzStColorNormalTangent, numTransparentSSRDrawSurfs);
    if (transparentSSRPrepass) {
        transparentSSRPrepass->graphicsPipelineDesc.depthStencilStateType = GetDepthState(frameCamera->projectionPolicy, true);
        RHI::RenderDest &renderDest = transparentSSRPrepass->graphicsPipelineDesc.renderDest;
        renderDest.renderTargetCount = 2;
        renderDest.renderTargetFormats[0] = frameCamera->transparentSSRNormalRoughnessTexture->GetFormat();
        renderDest.renderTargetFormats[1] = frameCamera->transparentSSRVelocityTexture->GetFormat();
        renderDest.depthStencilFormat = RenderContext::activeContext->GetMainRTDepthFormat();
        renderDest.sampleCount = 1;
        transparentSSRPrepassPSOResolver.Begin(transparentSSRPrepass, BE1::Min(numTransparentSSRDrawSurfs, 256));
    }

    // 5. Blend RenderPass 를 할당한다.
    ShaderFeatureSet blendPassFeatureSet;
    if (frameCamera->traceTransparentSSR) {
        blendPassFeatureSet.EnableFeature(ShaderFeature::ApplySSR);
    }
    RenderPass *blendPass = AllocRenderPass(frameCamera, RenderPassType::Transparent, blendPassFeatureSet, VertexInputLayoutId::XyzStColorNormalTangent, numBlendDrawSurfs);
    if (blendPass) {
        blendPass->graphicsPipelineDesc.depthStencilStateType = GetDepthState(frameCamera->projectionPolicy, false);
        RenderContext::activeContext->GetMainRenderDest(blendPass->graphicsPipelineDesc.renderDest, false, false, frameCamera->taaResolve);

        blendPSOResolver.Begin(blendPass, BE1::Min(numBlendDrawSurfs, 256));
    }

    // 카메라에 보이는 모든 surfaces 를 순회하면서, 각 render pass 에 visible surfaces 를 추가한다.
    for (int i = 0; i < frameCamera->numFrameSurfs; ++i) {
        const FrameSurf *frameSurf = frameCamera->frameSurfs[i];

        if (!BE1::HasFlag(frameSurf->flags, FrameSurf::Flag::CameraVisible)) {
            continue;
        }

        // 각 surface 의 특성 (material, featureSet, maxVertexWeights) 에 따라 PSO, inputLayoutId 를 얻고, RenderPassDrawSurf 를 추가한다.
        if (BE1::HasFlag(frameSurf->flags, FrameSurf::Flag::Skybox)) {
            if (skyboxPass) {
                const RenderPassPSOResolver::ResolveResult rr = skyboxPSOResolver.Resolve(frameSurf, frameSurf->baseFeatureSet | skyboxPass->featureSet, allowAsyncPSOBuild);
                AddRenderPassDrawSurf(skyboxPass, frameSurf, 0, rr.inputLayoutId, rr.pso);
            }
        } else if (frameSurf->material->GetRenderingMode() == Material::RenderingMode::Blend) {
            if (transparentSSRPrepass && IsTransparentSSRReceiver(frameSurf)) {
                const ShaderFeatureSet featureSet = frameSurf->baseFeatureSet | transparentSSRPrepass->featureSet;
                const RenderPassPSOResolver::ResolveResult rr = transparentSSRPrepassPSOResolver.Resolve(frameSurf, featureSet, allowAsyncPSOBuild);
                AddRenderPassDrawSurf(transparentSSRPrepass, frameSurf, BuildDepthSortKey(frameSurf), rr.inputLayoutId, rr.pso);
            }
            if (blendPass) {
                const ShaderFeatureSet featureSet = BuildSurfaceFeatureSet(frameSurf, blendPass, directLighting, indirectLighting);
                const RenderPassPSOResolver::ResolveResult rr = blendPSOResolver.Resolve(frameSurf, featureSet, allowAsyncPSOBuild);
                AddRenderPassDrawSurf(blendPass, frameSurf, BuildTransparentSortKey(frameSurf), rr.inputLayoutId, rr.pso);
            }
        } else {
            if (depthPrepass) {
                if (useSurfacePrepass) {
                    const ShaderFeatureSet featureSet = frameSurf->baseFeatureSet | depthPrepass->featureSet;
                    const RenderPassPSOResolver::ResolveResult rr = surfacePrepassPSOResolver.Resolve(frameSurf, featureSet, allowAsyncPSOBuild);
                    AddRenderPassDrawSurf(depthPrepass, frameSurf, BuildDepthSortKey(frameSurf), rr.inputLayoutId, rr.pso);
                } else {
                    const RenderPassDepthPSOResolver::ResolveResult rr = depthPrepassPSOResolver.Resolve(frameSurf, allowAsyncPSOBuild);
                    AddRenderPassDrawSurf(depthPrepass, frameSurf, BuildDepthSortKey(frameSurf), rr.inputLayoutId, rr.pso);
                }
            }
            if (opaquePass) {
                const ShaderFeatureSet featureSet = BuildSurfaceFeatureSet(frameSurf, opaquePass, directLighting, indirectLighting);
                const RenderPassPSOResolver::ResolveResult rr = opaquePSOResolver.Resolve(frameSurf, featureSet, allowAsyncPSOBuild);
                AddRenderPassDrawSurf(opaquePass, frameSurf, BuildOpaqueSortKey(frameSurf, rr.pso), rr.inputLayoutId, rr.pso);
            }
        }
    }

    // 필요한 경우 각 render pass 마다 surface 들을 sorting 한다.
    if (depthPrepass) {
        SortRenderPassDrawSurfs(frame, depthPrepass);
    }
    if (opaquePass) {
        SortRenderPassDrawSurfs(frame, opaquePass);
    }
    if (transparentSSRPrepass) {
        SortRenderPassDrawSurfs(frame, transparentSSRPrepass);
    }
    if (blendPass) {
        SortRenderPassDrawSurfs(frame, blendPass);
    }
}

void RenderPassBuilder::BuildGUI(RenderFrame *frame, FrameCamera *frameCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderPassBuilder::BuildGUI");

    if (frameCamera->numFrameSurfs == 0) {
        return;
    }

    AllocRenderPassDrawSurfs(frame, frameCamera, frameCamera->numFrameSurfs);

    RenderPass *guiPass = AllocRenderPass(frameCamera, RenderPassType::Gui, {}, VertexInputLayoutId::XyzStColor, frameCamera->numFrameSurfs);
    RenderContext::activeContext->GetSwapChainRenderDest(guiPass->graphicsPipelineDesc.renderDest);
    guiPass->graphicsPipelineDesc.depthStencilStateType = RHI::DepthStencilStateType::Disabled;

    guiPSOResolver.Begin(guiPass, BE1::Min(frameCamera->numFrameSurfs, 16u));

    for (uint32_t i = 0; i < frameCamera->numFrameSurfs; ++i) {
        const FrameSurf *frameSurf = frameCamera->frameSurfs[i];
        const ShaderFeatureSet featureSet = frameSurf->baseFeatureSet | guiPass->featureSet;
        const RenderPassPSOResolver::ResolveResult rr = guiPSOResolver.Resolve(frameSurf, featureSet);

        AddRenderPassDrawSurf(guiPass, frameSurf, i, rr.inputLayoutId, rr.pso);
    }
}

void RenderPassBuilder::AddRenderPassDrawSurf(RenderPass *renderPass, const FrameSurf *frameSurf, uint64_t sortKey, VertexInputLayoutId inputLayoutId, RHI::PipelineState *pso) {
    assert(renderPass->numDrawSurfs < renderPass->maxDrawSurfs);

    const uint32_t drawSurfIndex = renderPass->numDrawSurfs++;
    RenderPassDrawSurf *passDrawSurf = &renderPass->drawSurfStorage[drawSurfIndex];
    passDrawSurf->frameSurf = frameSurf;
    passDrawSurf->sortKey = sortKey;
    passDrawSurf->inputLayoutId = inputLayoutId;
    passDrawSurf->pso = pso;

    renderPass->drawSurfs[drawSurfIndex] = passDrawSurf;
}

void RenderPassBuilder::SortRenderPassDrawSurfs(RenderFrame *frame, RenderPass *renderPass) {
    PROFILER_CPU_SCOPED_EVENT("RenderPassBuilder::SortRenderPassDrawSurfs");

    if (renderPass->numDrawSurfs == 0) {
        return;
    }

    if (renderPass->numDrawSurfs < 128) {
        std::sort(renderPass->drawSurfs, renderPass->drawSurfs + renderPass->numDrawSurfs, [](const RenderPassDrawSurf *lhs, const RenderPassDrawSurf *rhs) {
            return lhs->sortKey < rhs->sortKey;
        });
        return;
    }

    const RenderPassDrawSurf **tempDrawSurfs = (const RenderPassDrawSurf **)frame->MemAlloc(renderPass->numDrawSurfs * sizeof(tempDrawSurfs[0]));
    RadixSortDrawSurfs(renderPass->drawSurfs, renderPass->numDrawSurfs, tempDrawSurfs);
}

void RenderPassBuilder::RadixSortDrawSurfs(const RenderPassDrawSurf **drawSurfs, uint32_t numDrawSurfs, const RenderPassDrawSurf **tempDrawSurfs) {
    constexpr uint32_t NumBuckets = 256;
    constexpr uint32_t NumPasses = 8;

    const RenderPassDrawSurf **src = drawSurfs;
    const RenderPassDrawSurf **dst = tempDrawSurfs;

    for (uint32_t pass = 0; pass < NumPasses; pass++) {
        const uint32_t shift = pass * 8;

        uint32_t counts[NumBuckets] = {};

        for (uint32_t i = 0; i < numDrawSurfs; i++) {
            const uint32_t bucket = (src[i]->sortKey >> shift) & 0xFF;
            counts[bucket]++;
        }

        uint32_t offsets[NumBuckets];
        uint32_t sum = 0;

        for (uint32_t i = 0; i < NumBuckets; i++) {
            offsets[i] = sum;
            sum += counts[i];
        }

        for (uint32_t i = 0; i < numDrawSurfs; i++) {
            const RenderPassDrawSurf *drawSurf = src[i];
            const uint32_t bucket = (drawSurf->sortKey >> shift) & 0xFF;
            dst[offsets[bucket]++] = drawSurf;
        }

        const RenderPassDrawSurf **swap = src;
        src = dst;
        dst = swap;
    }

    // 8 passes 라 짝수 번 swap 되므로 보통 src == drawSurfs 이지만,
    // pass 수가 바뀌어도 안전하게 유지.
    if (src != drawSurfs) {
        memcpy(drawSurfs, src, numDrawSurfs * sizeof(drawSurfs[0]));
    }
}
