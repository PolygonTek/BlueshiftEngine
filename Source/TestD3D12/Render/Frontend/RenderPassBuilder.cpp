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
#include "Render/RenderFrameData.h"
#include "Render/RenderInternal.h"
#include "Render/Material/Material.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"
#include "Render/ShaderPipeline/VertexInputLayoutFactory.h"

static uint64_t BuildOpaqueSortKey(const VisSurf *visSurf, RHI::PipelineState *pso) {
    //-----------------------------------------------------------
    // 0xFFFFF00000000000 (0~1048575)   : PSO sort index
    // 0x00000FFFFF000000 (0~1048575)   : subMesh index
    // 0x0000000000FF0000 (0~255)       : LOD level
    // 0x000000000000FFFF (0~65535)     : shader material index
    //-----------------------------------------------------------
    uint64_t psoIndex = pso ? (pso->sortIndex & 0xFFFFF) : 0;
    uint64_t subMeshIndex = visSurf->subMesh ? (visSurf->subMesh->GetID() & 0xFFFFF) : 0;
    uint64_t lodLevel = visSurf->lodLevel & 0xFF;
    uint64_t shaderMaterialIndex = visSurf->shaderMaterialIndex & 0xFFFF;

    // 비트 마스크는 정렬에만 사용한다. 실제 LOD 선택과 배치 비교에는 전체 값을 사용한다.
    return (psoIndex << 44) | (subMeshIndex << 24) | (lodLevel << 16) | shaderMaterialIndex;
}

static uint32_t BuildFloatSortKey(float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return (bits & 0x80000000u) ? ~bits : (bits ^ 0x80000000u);
}

static float GetViewCenterZ(const VisSurf *visSurf, const VisCamera *visCamera) {
    const BE1::Vec3 localCenter = visSurf->subMesh->GetBoundingBox().Center();
    const BE1::Vec3 worldCenter = visSurf->space->worldMatrix.TransformPos(localCenter);
    const BE1::Vec3 viewCenter = visCamera->viewMatrix.TransformPos(worldCenter);
    return viewCenter.z;
}

static uint64_t BuildDepthSortKey(const VisSurf *visSurf, const VisCamera *visCamera) {
    //-----------------------------------------------------------
    // 0x8000000000000000 (0~1)          : alpha-test bucket
    // 0x7FFFFFFF00000000 (0~2147483647) : shader material index (alpha-test only)
    // 0x00000000FFFFFFFF (0~4294967295) : near-to-far view depth
    //-----------------------------------------------------------
    const bool alphaTest = visSurf->baseFeatureSet.HasFeature(ShaderFeature::Perforated);
    const uint64_t alphaTestBucket = alphaTest ? 1ull : 0ull;
    const uint64_t shaderMaterialIndex = alphaTest ? (visSurf->shaderMaterialIndex & 0x7FFFFFFFull) : 0;
    const uint64_t depthKey = static_cast<uint64_t>(BuildFloatSortKey(GetViewCenterZ(visSurf, visCamera))) ^ 0xFFFFFFFFull;

    return (alphaTestBucket << 63) | (shaderMaterialIndex << 32) | depthKey;
}

static uint64_t BuildTransparentSortKey(const VisSurf *visSurf, const VisCamera *visCamera) {
    return BuildFloatSortKey(GetViewCenterZ(visSurf, visCamera));
}

static ShaderFeatureSet BuildSurfaceFeatureSet(const VisSurf *visSurf, const RenderPass *renderPass, bool directLighting, bool indirectLighting) {
    ShaderFeatureSet featureSet = visSurf->baseFeatureSet | renderPass->featureSet;
    const SharedPtr<ShaderTemplate> &shaderTemplate = visSurf->material->GetShaderTemplate();
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

void RenderPassBuilder::AllocRenderPassDrawSurfs(RenderFrameData *frameData, VisCamera *visCamera, uint32_t maxDrawSurfs) {
    assert(!visCamera->renderPassDrawSurfs);
    assert(!visCamera->renderPassDrawSurfStorage);

    visCamera->maxRenderPassDrawSurfs = maxDrawSurfs;
    visCamera->numRenderPassDrawSurfs = 0;
    visCamera->renderPassDrawSurfs = (const RenderPassDrawSurf **)frameData->MemAlloc(maxDrawSurfs * sizeof(RenderPassDrawSurf *));
    visCamera->renderPassDrawSurfStorage = (RenderPassDrawSurf *)frameData->MemAlloc(maxDrawSurfs * sizeof(RenderPassDrawSurf));
}

RenderPass *RenderPassBuilder::AllocRenderPass(VisCamera *visCamera, RenderPassType type, ShaderFeatureSet passFeatureSet, VertexInputLayoutId passBaseInputLayoutId, uint32_t maxDrawSurfs) {
    assert(visCamera->numRenderPasses < VisCamera::MaxRenderPasses);
    assert(visCamera->numRenderPassDrawSurfs + maxDrawSurfs <= visCamera->maxRenderPassDrawSurfs);

    if (maxDrawSurfs <= 0) {
        return nullptr;
    }

    RenderPass *renderPass = &visCamera->renderPasses[visCamera->numRenderPasses++];
    renderPass->type = type;
    renderPass->featureSet = passFeatureSet;
    renderPass->baseInputLayoutId = passBaseInputLayoutId;

    renderPass->graphicsPipelineDesc = {};
    renderPass->graphicsPipelineDesc.inputLayout = &VertexInputLayoutFactory::Get(passBaseInputLayoutId);

    renderPass->maxDrawSurfs = maxDrawSurfs;
    renderPass->numDrawSurfs = 0;
    renderPass->drawSurfs = visCamera->renderPassDrawSurfs + visCamera->numRenderPassDrawSurfs;
    renderPass->drawSurfStorage = visCamera->renderPassDrawSurfStorage + visCamera->numRenderPassDrawSurfs;
    visCamera->numRenderPassDrawSurfs += maxDrawSurfs;

    return renderPass;
}

static RHI::DepthStencilStateType GetDepthState(const RHI::ProjectionPolicy &projectionPolicy, bool write) {
    if (projectionPolicy.reverseZ) {
        return write ? RHI::DepthStencilStateType::GEqualWrite : RHI::DepthStencilStateType::GEqualNoWrite;
    }
    return write ? RHI::DepthStencilStateType::LEqualWrite : RHI::DepthStencilStateType::LEqualNoWrite;
}

void RenderPassBuilder::BuildCascadedShadow(RenderFrameData *frameData, VisCascadedShadow *cascadedShadow, uint32_t cascadeIndex, const VisSurf *const *visSurfs, uint32_t numVisSurfs, bool allowAsyncPSOBuild) {
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
    pipelineDesc.depthStencilStateType = RHI::DepthStencilStateType::LEqualWrite;
    pipelineDesc.renderDest.renderTargetCount = 0;
    pipelineDesc.renderDest.depthStencilFormat = RenderContext::activeContext->GetCSMDepthFormat();
    pipelineDesc.renderDest.sampleCount = cascadedShadow->depthTexture->GetSampleCount();

    // CSM 패스에서만 depth clipping 을 꺼 near 밖 caster 를 depth 0 에 clamp 한다 (shadow pancaking).
    pipelineDesc.depthClipEnabled = false;

    // 각 cascade 는 독립된 view/DSV/draw 목록과 해당 프레임에 캡처한 bias 를 사용한다.
    const RenderFrameSettings &settings = frameData->GetRenderFrameSettings();
    pipelineDesc.depthBiasOverride = true;
    pipelineDesc.constantDepthBias = settings.csmConstantDepthBias[cascadeIndex];
    pipelineDesc.slopeScaledDepthBias = settings.csmSlopeScaledDepthBias[cascadeIndex];

    BuildShadow(frameData, shadowPass, static_cast<uint8_t>(1u << cascadeIndex), visSurfs, numVisSurfs, allowAsyncPSOBuild);
}

void RenderPassBuilder::BuildShadow(RenderFrameData *frameData, RenderPass *shadowPass, uint8_t shadowCascadeMask, const VisSurf *const *visSurfs, uint32_t numVisSurfs, bool allowAsyncPSOBuild) {
    PROFILER_CPU_SCOPED_EVENT("RenderPassBuilder::BuildShadow");

    assert(shadowPass);
    assert(!shadowPass->drawSurfs && !shadowPass->drawSurfStorage);
    assert(visSurfs || numVisSurfs == 0);
    assert(shadowPass->graphicsPipelineDesc.renderDest.renderTargetCount == 0);
    assert(shadowPass->graphicsPipelineDesc.renderDest.depthStencilFormat != RHI::Format::Unknown);
    assert(shadowCascadeMask != 0);

    shadowPass->type = RenderPassType::Shadow;
    shadowPass->baseInputLayoutId = VertexInputLayoutId::XyzStColor;
    // Depth 대상과 bias 등 호출자가 준비한 설정은 유지한다.
    shadowPass->graphicsPipelineDesc.inputLayout = &VertexInputLayoutFactory::Get(shadowPass->baseInputLayoutId);

    uint32_t numShadowDrawSurfs = 0;
    for (uint32_t i = 0; i < numVisSurfs; ++i) {
        if ((visSurfs[i]->shadowCascadeMask & shadowCascadeMask) != 0) {
            numShadowDrawSurfs++;
        }
    }

    // Caster 가 없는 패스도 depth clear 에 사용할 수 있도록 설정은 유지하고 draw 메모리만 생략한다.
    if (numShadowDrawSurfs == 0) {
        return;
    }

    shadowPass->maxDrawSurfs = numShadowDrawSurfs;
    shadowPass->drawSurfs = (const RenderPassDrawSurf **)frameData->MemAlloc(numShadowDrawSurfs * sizeof(RenderPassDrawSurf *));
    shadowPass->drawSurfStorage = (RenderPassDrawSurf *)frameData->MemAlloc(numShadowDrawSurfs * sizeof(RenderPassDrawSurf));

    // ShadowViewCB 에 광원 행렬과 surface table 을 전달하는 전용 셰이더를 사용한다.
    shadowPSOResolver.Begin(shadowPass, BE1::Min(numShadowDrawSurfs, 24u), shaderTemplateManager.shadowDepthShaderTemplate);

    for (uint32_t i = 0; i < numVisSurfs; ++i) {
        const VisSurf *visSurf = visSurfs[i];
        if ((visSurf->shadowCascadeMask & shadowCascadeMask) == 0) {
            continue;
        }

        const RenderPassDepthPSOResolver::ResolveResult rr = shadowPSOResolver.Resolve(visSurf, allowAsyncPSOBuild);
        AddRenderPassDrawSurf(shadowPass, visSurf, BuildOpaqueSortKey(visSurf, rr.pso), rr.inputLayoutId, rr.pso);
    }

    SortRenderPassDrawSurfs(frameData, shadowPass);
}

void RenderPassBuilder::BuildMain(RenderFrameData *frameData, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderPassBuilder::BuildMain");

    if (visCamera->numVisSurfs == 0) {
        return;
    }

    // Forward lighting 에서 불투명 표면을 렌더링하려면 AO 가 필요하다.
    // SSAO 또는 velocity 가 필요하면 depth prepass 를 강제로 켠다.
    const bool useDepthPrepass = frameData->GetRenderFrameSettings().depthPrepass || visCamera->applySSAO || visCamera->needVelocityOutput;

    int numDepthDrawSurfs = 0;
    int numOpaqueDrawSurfs = 0;
    int numSkyboxDrawSurfs = 0;
    int numBlendDrawSurfs = 0;

    for (int i = 0; i < visCamera->numVisSurfs; ++i) {
        const VisSurf *visSurf = visCamera->visSurfs[i];

        if (!BE1::HasFlag(visSurf->flags, VisSurfFlag::CameraVisible)) {
            continue;
        }

        if (BE1::HasFlag(visSurf->flags, VisSurfFlag::Skybox)) {
            numSkyboxDrawSurfs++;
        } else if (visSurf->material->GetRenderingMode() == Material::RenderingMode::Blend) {
            numBlendDrawSurfs++;
        } else {
            if (useDepthPrepass) {
                numDepthDrawSurfs++;
            }
            numOpaqueDrawSurfs++;
        }
    }

    // Shadow 전용 surface 만 있다면 메인 패스용 메모리를 할당하지 않는다.
    const uint32_t numMainDrawSurfs = numDepthDrawSurfs + numOpaqueDrawSurfs + numSkyboxDrawSurfs + numBlendDrawSurfs;
    if (numMainDrawSurfs == 0) {
        return;
    }

    const bool allowAsyncPSOBuild = !BE1::HasFlag(visCamera->flags, RenderCamera::Flag::ForceSyncPassPSOBuild);
    const bool directLighting = visCamera->directionalLightVisible || visCamera->numVisLocalLights > 0;
    const bool indirectLighting = visCamera->envLightDiffuseIrradianceCubeMap && visCamera->envLightSpecularCubeMap;

    // RenderPass 에서 필요한 모든 RenderPassDrawSurf 를 미리 할당한다.
    AllocRenderPassDrawSurfs(frameData, visCamera, numMainDrawSurfs);

    // AllocRenderPass 호출 순서대로 백엔드에서 RenderPass 가 렌더링된다.
    // 1. DepthPrepass RenderPass 를 할당한다.
    ShaderFeatureSet depthPassFeatureSet;
    if (visCamera->needVelocityOutput) {
        depthPassFeatureSet.EnableFeature(ShaderFeature::OutputVelocity);
    }
    RenderPass *depthPass = AllocRenderPass(visCamera, RenderPassType::DepthPrepass, depthPassFeatureSet, VertexInputLayoutId::XyzStColor, numDepthDrawSurfs);
    if (depthPass) {
        depthPass->graphicsPipelineDesc.depthStencilStateType = GetDepthState(visCamera->projectionPolicy, true);
        RHI::RenderDest &renderDest = depthPass->graphicsPipelineDesc.renderDest;
        renderDest = RHI::RenderDest();
        renderDest.renderTargetCount = 0;
        if (visCamera->needVelocityOutput) {
            renderDest.renderTargetFormats[renderDest.renderTargetCount++] = RHI::Format::R16G16_FLOAT;
        }
        renderDest.depthStencilFormat = RenderContext::activeContext->GetMainRTDepthFormat();
        renderDest.sampleCount = visCamera->depthTexture->GetSampleCount();

        depthPSOResolver.Begin(depthPass, BE1::Min(numDepthDrawSurfs, 16), shaderTemplateManager.depthShaderTemplate);
    }

    ShaderFeatureSet opaquePassFeatureSet;
    if (visCamera->needNormalOutput) {
        opaquePassFeatureSet.EnableFeature(ShaderFeature::OutputNormal);
    }

    // 2. Opaque RenderPass 를 할당한다.
    RenderPass *opaquePass = AllocRenderPass(visCamera, RenderPassType::Opaque, opaquePassFeatureSet, VertexInputLayoutId::XyzStColorNormalTangent, numOpaqueDrawSurfs);
    if (opaquePass) {
        opaquePass->graphicsPipelineDesc.depthStencilStateType = GetDepthState(visCamera->projectionPolicy, !useDepthPrepass);
        RenderContext::activeContext->GetMainRenderDest(opaquePass->graphicsPipelineDesc.renderDest, visCamera->needNormalOutput, false);

        opaquePSOResolver.Begin(opaquePass, BE1::Min(numOpaqueDrawSurfs, 1024));
    }

    // 3. Skybox RenderPass 를 할당한다.
    RenderPass *skyboxPass = AllocRenderPass(visCamera, RenderPassType::Skybox, {}, VertexInputLayoutId::XyzStColor, numSkyboxDrawSurfs);
    if (skyboxPass) {
        skyboxPass->graphicsPipelineDesc.depthStencilStateType = GetDepthState(visCamera->projectionPolicy, false);
        RenderContext::activeContext->GetMainRenderDest(skyboxPass->graphicsPipelineDesc.renderDest, visCamera->needNormalOutput, false);

        skyboxPSOResolver.Begin(skyboxPass, BE1::Min(numSkyboxDrawSurfs, 1));
    }

    // 4. Blend RenderPass 를 할당한다.
    RenderPass *blendPass = AllocRenderPass(visCamera, RenderPassType::Transparent, {}, VertexInputLayoutId::XyzStColorNormalTangent, numBlendDrawSurfs);
    if (blendPass) {
        blendPass->graphicsPipelineDesc.depthStencilStateType = GetDepthState(visCamera->projectionPolicy, false);
        RenderContext::activeContext->GetMainRenderDest(blendPass->graphicsPipelineDesc.renderDest, visCamera->needNormalOutput, visCamera->needVelocityOutput);

        blendPSOResolver.Begin(blendPass, BE1::Min(numBlendDrawSurfs, 256));
    }

    // 카메라에 보이는 모든 surfaces 를 순회하면서, 각 render pass 에 visible surfaces 를 추가한다.
    for (int i = 0; i < visCamera->numVisSurfs; ++i) {
        const VisSurf *visSurf = visCamera->visSurfs[i];

        if (!BE1::HasFlag(visSurf->flags, VisSurfFlag::CameraVisible)) {
            continue;
        }

        // 각 surface 의 특성 (material, featureSet, maxVertexWeights) 에 따라 PSO, inputLayoutId 를 얻고, RenderPassDrawSurf 를 추가한다.
        if (BE1::HasFlag(visSurf->flags, VisSurfFlag::Skybox)) {
            if (skyboxPass) {
                const RenderPassPSOResolver::ResolveResult rr = skyboxPSOResolver.Resolve(visSurf, visSurf->baseFeatureSet | skyboxPass->featureSet, allowAsyncPSOBuild);
                AddRenderPassDrawSurf(skyboxPass, visSurf, 0, rr.inputLayoutId, rr.pso);
            }
        } else if (visSurf->material->GetRenderingMode() == Material::RenderingMode::Blend) {
            if (blendPass) {
                const ShaderFeatureSet featureSet = BuildSurfaceFeatureSet(visSurf, blendPass, directLighting, indirectLighting);
                const RenderPassPSOResolver::ResolveResult rr = blendPSOResolver.Resolve(visSurf, featureSet, allowAsyncPSOBuild);
                AddRenderPassDrawSurf(blendPass, visSurf, BuildTransparentSortKey(visSurf, visCamera), rr.inputLayoutId, rr.pso);
            }
        } else {
            if (depthPass) {
                const RenderPassDepthPSOResolver::ResolveResult rr = depthPSOResolver.Resolve(visSurf, allowAsyncPSOBuild);
                AddRenderPassDrawSurf(depthPass, visSurf, BuildDepthSortKey(visSurf, visCamera), rr.inputLayoutId, rr.pso);
            }
            if (opaquePass) {
                const ShaderFeatureSet featureSet = BuildSurfaceFeatureSet(visSurf, opaquePass, directLighting, indirectLighting);
                const RenderPassPSOResolver::ResolveResult rr = opaquePSOResolver.Resolve(visSurf, featureSet, allowAsyncPSOBuild);
                AddRenderPassDrawSurf(opaquePass, visSurf, BuildOpaqueSortKey(visSurf, rr.pso), rr.inputLayoutId, rr.pso);
            }
        }
    }

    // 필요한 경우 각 render pass 마다 surface 들을 sorting 한다.
    if (depthPass) {
        SortRenderPassDrawSurfs(frameData, depthPass);
    }
    if (opaquePass) {
        SortRenderPassDrawSurfs(frameData, opaquePass);
    }
    if (blendPass) {
        SortRenderPassDrawSurfs(frameData, blendPass);
    }
}

void RenderPassBuilder::BuildGUI(RenderFrameData *frameData, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderPassBuilder::BuildGUI");

    if (visCamera->numVisSurfs == 0) {
        return;
    }

    AllocRenderPassDrawSurfs(frameData, visCamera, visCamera->numVisSurfs);

    RenderPass *guiPass = AllocRenderPass(visCamera, RenderPassType::Gui, {}, VertexInputLayoutId::XyzStColor, visCamera->numVisSurfs);
    RenderContext::activeContext->GetSwapChainRenderDest(guiPass->graphicsPipelineDesc.renderDest);
    guiPass->graphicsPipelineDesc.depthStencilStateType = RHI::DepthStencilStateType::Disabled;

    guiPSOResolver.Begin(guiPass, BE1::Min(visCamera->numVisSurfs, 16u));

    for (uint32_t i = 0; i < visCamera->numVisSurfs; ++i) {
        const VisSurf *visSurf = visCamera->visSurfs[i];
        const ShaderFeatureSet featureSet = visSurf->baseFeatureSet | guiPass->featureSet;
        const RenderPassPSOResolver::ResolveResult rr = guiPSOResolver.Resolve(visSurf, featureSet);

        AddRenderPassDrawSurf(guiPass, visSurf, i, rr.inputLayoutId, rr.pso);
    }
}

void RenderPassBuilder::AddRenderPassDrawSurf(RenderPass *renderPass, const VisSurf *visSurf, uint64_t sortKey, VertexInputLayoutId inputLayoutId, RHI::PipelineState *pso) {
    assert(renderPass->numDrawSurfs < renderPass->maxDrawSurfs);

    const uint32_t drawSurfIndex = renderPass->numDrawSurfs++;
    RenderPassDrawSurf *passDrawSurf = &renderPass->drawSurfStorage[drawSurfIndex];
    passDrawSurf->visSurf = visSurf;
    passDrawSurf->sortKey = sortKey;
    passDrawSurf->inputLayoutId = inputLayoutId;
    passDrawSurf->pso = pso;

    renderPass->drawSurfs[drawSurfIndex] = passDrawSurf;
}

void RenderPassBuilder::SortRenderPassDrawSurfs(RenderFrameData *frameData, RenderPass *renderPass) {
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

    const RenderPassDrawSurf **tempDrawSurfs = (const RenderPassDrawSurf **)frameData->MemAlloc(renderPass->numDrawSurfs * sizeof(tempDrawSurfs[0]));
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
