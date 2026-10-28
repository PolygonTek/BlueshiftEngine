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
#include "RenderWorld.h"
#include "LightCullingBuilder.h"
#include "Render/RenderContext.h"
#include "Render/RenderSystem.h"
#include "RenderCamera.h"
#include "Render/RenderInternal.h"
#include "Render/RenderUtils.h"
#include "Render/LocalShadowCasterCuller.h"
#include "Render/Material/MaterialManager.h"
#include "Render/Mesh/MeshManager.h"
#include "Render/ShaderPipeline/ShaderPipelineCache.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"
#include "Render/ShaderPipeline/VertexInputLayoutFactory.h"
#include "Render/Texture/Texture.h"
#include "Shaders/ShaderInterop.h"
#include "Math/Sphere.h"

static_assert(static_cast<uint32_t>(RenderLight::Type::Point) == ShaderLightTypePoint);
static_assert(static_cast<uint32_t>(RenderLight::Type::Spot) == ShaderLightTypeSpot);

static BE_INLINE bool IsRenderLayerVisible(uint32_t layerMask, uint8_t layer) {
    return layer < 32 && (layerMask & BIT(layer)) != 0;
}

static ShaderFeatureSet BuildBaseFeatureSet(const Material *material, const SubMesh *subMesh, bool skinning) {
    ShaderFeatureSet featureSet;

    if (material->GetRenderingMode() == Material::RenderingMode::AlphaCutoff) {
        featureSet.EnableFeature(ShaderFeature::Perforated);
    }

    int maxVertexWeights = skinning ? subMesh->MaxVertexWeights() : 0;
    if (maxVertexWeights == 8) {
        featureSet.EnableFeature(ShaderFeature::GpuSkinningWeight8);
    } else if (maxVertexWeights == 4) {
        featureSet.EnableFeature(ShaderFeature::GpuSkinningWeight4);
    } else if (maxVertexWeights == 1) {
        featureSet.EnableFeature(ShaderFeature::GpuSkinningWeight1);
    }

    return featureSet;
}

const FrameSurf *RenderWorld::AddFrameSurf(RenderFrame *frame, FrameCamera *frameCamera, FrameObject *frameObject, const Material *material, SubMesh *subMesh, FrameSurfFlag flags, uint8_t shadowCascadeMask, int lodLevel) {
    if (frameCamera->numFrameSurfs + 1 > frameCamera->maxFrameSurfs) {
        BE_WARNLOG("RenderWorld::AddFrameSurf: not enough available visible surfaces\n");
        return nullptr;
    }

    assert(material);
    assert(subMesh);
    assert(lodLevel >= 0 && lodLevel < subMesh->NumLODs());

    const RHI::IndexBuffer *indexBuffer = lodLevel == 0 ? subMesh->indexBuffer : (subMesh->lodIndexBuffers ? subMesh->lodIndexBuffers[lodLevel - 1] : nullptr);
    if (!subMesh->vertexBuffer || !indexBuffer || (subMesh->vertWeights && !subMesh->vertexWeightBuffer)) {
        indexBuffer = subMesh->UploadStaticDataToGPU(lodLevel);
    }

    if (!subMesh->vertexBuffer || !indexBuffer || (subMesh->vertWeights && !subMesh->vertexWeightBuffer)) {
        BE_WARNLOG("RenderWorld::AddFrameSurf: failed to prepare static submesh buffers\n");
        return nullptr;
    }

    // 카메라 뷰 공간 z 깊이를 미리 계산한다 (불필요한 x, y 계산 생략)
#if 1
    const auto &viewRow2 = frameCamera->viewMatrix[2];
    const float viewZ = viewRow2[0] * frameObject->worldMatrix[0][3] + viewRow2[1] * frameObject->worldMatrix[1][3] + viewRow2[2] * frameObject->worldMatrix[2][3] + viewRow2[3];
#else
    const BE1::Vec3 localCenter = subMesh->GetBoundingBox().Center();
    const BE1::Vec3 worldCenter = frameObject->worldMatrix.TransformPos(localCenter);
    const auto &viewRow2 = frameCamera->viewMatrix[2];
    const flaot viewZ = viewRow2[0] * worldCenter.x + viewRow2[1] * worldCenter.y + viewRow2[2] * worldCenter.z + viewRow2[3];
#endif

    const FrameSkinnedMesh *frameSkinnedMesh = frameObject->skinnedMesh;

    FrameSurf *drawSurf = (FrameSurf *)frame->MemAlloc(sizeof(FrameSurf));
    if (!drawSurf) {
        return nullptr;
    }
    drawSurf->index = frameCamera->numFrameSurfs;
    drawSurf->space = frameObject;
    drawSurf->subMesh = subMesh;
    drawSurf->indexBuffer = indexBuffer;
    drawSurf->numIndexes = subMesh->NumIndexes(lodLevel);
    drawSurf->lodLevel = lodLevel;
    drawSurf->viewZ = viewZ;
    drawSurf->material = material;
    drawSurf->baseFeatureSet = BuildBaseFeatureSet(material, subMesh, frameSkinnedMesh && frameSkinnedMesh->skinningJointOffset != ShaderSkinningJointOffsetInvalid);
    drawSurf->shaderMaterialIndex = frame->RegisterShaderMaterial(material);
    drawSurf->flags = flags;
    drawSurf->shadowCascadeMask = shadowCascadeMask;

    frameCamera->frameSurfs[frameCamera->numFrameSurfs++] = drawSurf;
    return drawSurf;
}

void RenderWorld::SetSkyboxMaterial(SharedPtr<Material> skyboxMaterial) {
    assert(!frameActive);

    if (this->skyboxMaterial != skyboxMaterial && hasEnvironmentLight && !envLight.GetDesc().radianceCubeMap) {
        envLight.MarkRegenerationNeeded();
        renderSystem->GetProbeBakeSystem().CancelEnvironmentLightProbeBake(this);
    }

    this->skyboxMaterial = std::move(skyboxMaterial);
}

void RenderWorld::SetSkyboxLighting(const BE1::Color3 &linearColor, float intensity) {
    assert(BE1::Engine::IsInMainThread());
    assert(!frameActive);

    /// 화면 표시용 sky 색과 밝기. 환경광 베이크 원본에는 적용하지 않는다.
    skyboxRadianceScale = linearColor * intensity;
}

void RenderWorld::SetSkyboxRotation(const BE1::Mat3 &axis) {
    assert(BE1::Engine::IsInMainThread());
    assert(!frameActive);

    skyboxAxis = axis;
}

static bool IsMaterialTextureReady(const Material *material, ShaderTemplate::StandardProperty property) {
    const SharedPtr<Texture> *texture = material->GetPropertyTexture(property);
    return texture && *texture && (*texture)->IsReadyForRendering();
}

RenderWorld::SkyboxState RenderWorld::GetSkyboxState() const {
    if (!skyboxMaterial) {
        return SkyboxState::None;
    }

    if (!skyboxMaterial->GetShaderTemplate() || !meshManager.defaultBoxMesh || !meshManager.defaultBoxMesh->IsReady()) {
        return SkyboxState::Pending;
    }

    const Mesh::Surface *meshSurface = meshManager.defaultBoxMesh->GetSurface(0);
    if (!meshSurface || !meshSurface->subMesh) {
        return SkyboxState::Pending;
    }

    if (skyboxMaterial->FindPropertyIndex(ShaderTemplate::StandardProperty::SkyCubeMap) >= 0) {
        return IsMaterialTextureReady(skyboxMaterial.get(), ShaderTemplate::StandardProperty::SkyCubeMap) ? SkyboxState::Ready : SkyboxState::Pending;
    }

    if (skyboxMaterial->FindPropertyIndex(ShaderTemplate::StandardProperty::SkyBackMap) >= 0) {
        static constexpr ShaderTemplate::StandardProperty SkyboxTextureProperties[] = {
            ShaderTemplate::StandardProperty::SkyBackMap,
            ShaderTemplate::StandardProperty::SkyFrontMap,
            ShaderTemplate::StandardProperty::SkyRightMap,
            ShaderTemplate::StandardProperty::SkyLeftMap,
            ShaderTemplate::StandardProperty::SkyTopMap,
            ShaderTemplate::StandardProperty::SkyDownMap
        };

        for (ShaderTemplate::StandardProperty property : SkyboxTextureProperties) {
            if (!IsMaterialTextureReady(skyboxMaterial.get(), property)) {
                return SkyboxState::Pending;
            }
        }
    }

    return SkyboxState::Ready;
}

static RHI::PipelineState *GetOutputBlitPSO(const RHI::Texture *outputTexture, RHI::ViewID outputViewID) {
    RHI::TextureViewInfo outputViewInfo;
    if (!outputTexture || !outputTexture->GetViewInfo(RHI::ViewType::RTV, outputViewID, outputViewInfo)) {
        return nullptr;
    }

    GraphicsPipelineDesc graphicsPipelineDesc;
    graphicsPipelineDesc.renderDest.renderTargetCount = 1;
    graphicsPipelineDesc.renderDest.renderTargetFormats[0] = outputViewInfo.format;
    graphicsPipelineDesc.renderDest.sampleCount = outputTexture->GetSampleCount();
    graphicsPipelineDesc.depthStencilStateType = RHI::DepthStencilStateType::Disabled;
    graphicsPipelineDesc.blendStateType = RHI::BlendStateType::Disabled;

    ShaderPipeline *imagePipeline = shaderPipelineCache.GetGraphicsPipelinePtr(shaderTemplateManager.fullscreenImageShaderTemplate, {}, graphicsPipelineDesc, false);
    return ShaderPipeline::GetReadyPSO(imagePipeline);
}

static RHI::PipelineState *GetSceneSnapshotDownsamplePSO(RHI::Format outputFormat) {
    if (!shaderTemplateManager.sceneSnapshotDownsampleShaderTemplate) {
        return nullptr;
    }

    GraphicsPipelineDesc graphicsPipelineDesc;
    graphicsPipelineDesc.renderDest.renderTargetCount = 2;
    graphicsPipelineDesc.renderDest.renderTargetFormats[0] = outputFormat;
    graphicsPipelineDesc.renderDest.renderTargetFormats[1] = RHI::Format::R32_FLOAT;
    graphicsPipelineDesc.renderDest.sampleCount = 1;
    graphicsPipelineDesc.depthStencilStateType = RHI::DepthStencilStateType::Disabled;
    graphicsPipelineDesc.blendStateType = RHI::BlendStateType::Disabled;
    graphicsPipelineDesc.primitiveTopology = RHI::PrimitiveTopology::TriangleList;

    // Half-resolution color/depth 가 같은 원본 texel 을 나타내도록 전용 point downsample pipeline 을 사용한다.
    ShaderPipeline *downsamplePipeline = shaderPipelineCache.GetGraphicsPipelinePtr(shaderTemplateManager.sceneSnapshotDownsampleShaderTemplate, {}, graphicsPipelineDesc, false);
    return ShaderPipeline::GetReadyPSO(downsamplePipeline);
}

void RenderWorld::RenderScene(RenderCamera *renderCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::RenderScene");

    assert(BE1::Engine::IsInMainThread());
    assert(frameActive);

    const RenderCamera::Desc &cameraDesc = renderCamera->desc;
    const BE1::Rect &outputRect = cameraDesc.outputRect;
    if (outputRect.w <= 0 || outputRect.h <= 0) {
        return;
    }

    RenderContext *renderContext = RenderContext::activeContext;
    RenderFrame *frame = renderContext->GetCurrentFrontendFrame();
    const RenderFrameSettings &frameSettings = frame->GetRenderFrameSettings();

    // 렌더 해상도가 지정되지 않았으면 출력 영역의 크기를 사용한다.
    const uint32_t renderWidth = cameraDesc.renderWidth != 0 ? cameraDesc.renderWidth : static_cast<uint32_t>(outputRect.w);
    const uint32_t renderHeight = cameraDesc.renderHeight != 0 ? cameraDesc.renderHeight : static_cast<uint32_t>(outputRect.h);

    // FrameLight 는 축당 8 비트 타일 좌표를 저장하므로 최대 256 개 타일까지 지원한다. 즉, MaxRenderDimension x MaxRenderDimension 보다 큰 해상도는 지원하지 않는다.
    if (renderWidth > FrameLight::MaxRenderDimension || renderHeight > FrameLight::MaxRenderDimension) {
        BE_WARNLOG("RenderWorld::RenderScene: requested render size %ux%u exceeds the per-axis limit of %u\n", renderWidth, renderHeight, FrameLight::MaxRenderDimension);
        return;
    }

    // 출력 텍스처가 지정되면, 최종 렌더링 결과를 해당 텍스처에 blit 할 PSO 를 준비한다.
    RHI::PipelineState *outputBlitPSO = nullptr;
    if (cameraDesc.outputTexture) {
        if (!cameraDesc.outputTexture->HasValidView(RHI::ViewType::RTV, cameraDesc.outputViewID)) {
            BE_WARNLOG("RenderWorld::RenderScene: output texture has no valid render target view\n");
            return;
        }

        outputBlitPSO = GetOutputBlitPSO(cameraDesc.outputTexture, cameraDesc.outputViewID);
        if (!outputBlitPSO) {
            BE_WARNLOG("RenderWorld::RenderScene: output blit pipeline is not ready\n");
            return;
        }
    }

    const bool skipPostProcess = frameSettings.skipPostProcess || BE1::HasFlag(cameraDesc.flags, RenderCamera::Flag::SkipPostProcess);
    const bool useAO = frameSettings.aoMode != RenderAOMode::Off;
    const bool debugAO = useAO && frameSettings.gtaoDebug != 0;
    const bool temporalHistoryEnabled = !BE1::HasFlag(cameraDesc.flags, RenderCamera::Flag::DisableTemporalHistory);
    const bool useEnvironmentLight = hasEnvironmentLight && envLight.GetDesc().enabled;
    // 환경광을 끄면 SSR 리소스와 히스토리도 기존 비활성화 경로에서 정리한다.
    const bool useSSR = useEnvironmentLight && temporalHistoryEnabled && frameSettings.ssr;
    const bool needsSSRTemporalAccumulation = frameSettings.ssrTemporal && frameSettings.ssrTemporalHistoryWeight > 0.0f && (frameSettings.ssrDebugMode == SSRDebugModeOff || frameSettings.ssrDebugMode == SSRDebugModeUpsampledReflection);
    const bool debugSSR = useSSR && frameSettings.ssrDebugMode != SSRDebugModeOff;
    const bool debugTransparentSSR = debugSSR && frameSettings.ssrTransparent && frameSettings.ssrDebugTransparent;
    const bool useTransparentSSR = useSSR && frameSettings.ssrTransparent && (!debugSSR || debugTransparentSSR);
    const bool useTAA = temporalHistoryEnabled && frameSettings.taa && !skipPostProcess && !debugAO && frameSettings.shadowDebugMode != ShaderShadowDebugModeVisibility && !debugSSR;
    const bool needNormalRoughnessTexture = useAO || useSSR;
    const bool needVelocityTexture = useTAA || useSSR;
    const bool needColorHistory = useTAA;
    const bool needDepthHistory = useTAA;
    const bool needHiZCullResources = !BE1::HasFlag(cameraDesc.flags, RenderCamera::Flag::DisableHiZCulling) && frameSettings.hiZOcclusionCulling;
    const bool needHiZTexture = needHiZCullResources;
    const bool needHiZDebugPreview = needHiZTexture && frameSettings.hiZDebugPreview;

    // 장면 렌더링에 필요한 컬러, 깊이, 노멀, roughness, velocity 텍스처를 준비한다.
    if (!renderCamera->EnsureRenderTextures(renderWidth, renderHeight, renderContext->GetMainRTColorFormat(), renderContext->GetMainRTDepthFormat(), renderContext->GetMainRTSampleCount(), needDepthHistory, needNormalRoughnessTexture, needVelocityTexture, renderContext->GetBaseClearColor())) {
        return;
    }

    // 이번 프레임에 사용할 카메라 슬롯을 확보한다.
    FrameCamera *frameCamera = frame->AllocFrameCamera();
    if (!frameCamera) {
        return;
    }

    // SSR 에 사용할 SSRHiZ 리소스를 준비한다.
    SSRHiZResources &ssrHiZResources = renderCamera->GetSSRHiZResources();
    const bool ssrHiZResourcesReady = ssrHiZResources.Ensure(renderWidth, renderHeight, useSSR);

    // SSR 은 이전 프레임의 full-resolution HDR radiance 와 논리적 mip 1 부터 시작하는 압축 pyramid 를 사용한다.
    SSRSceneColorHistoryResources &ssrSceneColorHistoryResources = renderCamera->GetSSRSceneColorHistoryResources();
    const bool ssrSceneColorHistoryResourcesReady = ssrSceneColorHistoryResources.Ensure(renderWidth, renderHeight, renderContext->GetMainRTColorFormat(), frameSettings.ssrResolutionMode == 0 && needsSSRTemporalAccumulation && !debugTransparentSSR, useSSR);

    // SSR 추적/resolve 용 선택 해상도 텍스처와 Lit 이 한 번만 읽을 full-resolution radiance 를 준비한다.
    const bool ssrResourcesReady = renderCamera->EnsureSSRResources(renderWidth, renderHeight, frameSettings.ssrResolutionMode, useSSR);
    if (!useTransparentSSR) {
        renderCamera->EnsureTransparentSSRResources(renderWidth, renderHeight, renderContext->GetMainRTDepthFormat(), frameSettings.ssrResolutionMode, false);
    }

    // MSAA 깊이를 resolve 할 AO/SSR 공용 텍스처를 준비한다.
    const bool resolvedDepthReady = renderCamera->EnsureResolvedDepthTexture(renderWidth, renderHeight, renderContext->GetMainRTSampleCount(), useAO || useSSR);

    // AO 계산과 필터링에 필요한 텍스처를 설정에 맞게 준비한다.
    renderCamera->screenSpaceAOResources.Ensure(renderWidth, renderHeight, frameSettings);

    // Ocean 굴절에 사용할 장면 컬러와 깊이 스냅샷 리소스를 준비한다.
    const bool useMSAA = renderCamera->GetColorTexture()->GetSampleCount() > 1;
    const bool needSceneSnapshot = hasOceanSurface && oceanSurface.desc.surface.refractionEnabled && !oceanSurface.desc.surface.wireframe && !BE1::HasFlag(cameraDesc.flags, RenderCamera::Flag::StaticOnly);
    const bool requestHalfResSnapshot = needSceneSnapshot && frameSettings.snapshotColorHalfRes && !useMSAA;
    RHI::PipelineState *sceneSnapshotDownsamplePSO = requestHalfResSnapshot ? GetSceneSnapshotDownsamplePSO(renderContext->GetMainRTColorFormat()) : nullptr;
    const bool useHalfResSnapshot = requestHalfResSnapshot && sceneSnapshotDownsamplePSO;
    renderCamera->EnsureSceneSnapshotResources(needSceneSnapshot, useHalfResSnapshot);

    // Hi-Z 오클루전 컬링과 디버그 표시에 필요한 리소스를 준비한다.
    HiZResources &hiZResources = renderCamera->GetHiZResources();
    hiZResources.Ensure(renderWidth, renderHeight, needHiZTexture, needHiZCullResources, needHiZDebugPreview);

    // 후처리와 TAA 히스토리에 필요한 리소스를 준비한다.
    renderCamera->EnsurePostProcessResources(skipPostProcess, renderContext->GetPostProcessColorFormat(), needColorHistory, needDepthHistory, frameSettings);

    // TAA 설정에 따라 카메라의 서브픽셀 jitter 와 jitter 적용 행렬을 갱신한다.
    const bool temporalJitterEnabled = useTAA && frameSettings.taaJitter;
    const uint32_t temporalJitterSampleCount = frameSettings.taaJitterSampleCount;
    renderCamera->UpdateTemporalJitter(temporalJitterEnabled, temporalJitterSampleCount);

    const bool canUseHiZ = hiZResources.GetTexture() != nullptr;
    const bool requestHiZOcclusionCulling = canUseHiZ && needHiZCullResources && hiZResources.GetVisibleInstanceBuffer() && hiZResources.GetIndirectArgsBuffer();
    const bool requestHiZDebugPreview = canUseHiZ && needHiZDebugPreview && hiZResources.GetDebugPreviewTexture();
    const bool requestHiZGeneration = requestHiZOcclusionCulling;
    const bool hasSSRInputs = !useMSAA || resolvedDepthReady;
    const bool requestSSRHiZGeneration = useSSR && hasSSRInputs && ssrHiZResourcesReady;
    const bool canTraceSSR = requestSSRHiZGeneration && ssrResourcesReady && ssrSceneColorHistoryResourcesReady && ssrSceneColorHistoryResources.IsValid();
    const bool requestSSRTrace = !debugTransparentSSR && canTraceSSR;
    const bool requestSSRTemporalAccumulation = requestSSRTrace && needsSSRTemporalAccumulation;
    const bool requestSSRSceneColorHistoryUpdate = useSSR && ssrSceneColorHistoryResourcesReady;
    const bool requestTransparentSSRTrace = useTransparentSSR && canTraceSSR;
    const bool requestTransparentSSRTemporalAccumulation = requestTransparentSSRTrace && needsSSRTemporalAccumulation;

    frameCamera->maxFrameSurfs = FrameCamera::MaxFrameSurfs;
    frameCamera->frameSurfs = (const FrameSurf **)frame->MemAlloc(frameCamera->maxFrameSurfs * sizeof(FrameSurf *));

    frameCamera->numRenderPasses = 0;
    frameCamera->renderPasses = (RenderPass *)frame->MemAlloc(FrameCamera::MaxRenderPasses * sizeof(RenderPass));

    frameCamera->origin = cameraDesc.origin;
    frameCamera->axis = cameraDesc.axis;
    frameCamera->skyboxRadianceScale = BE1::HasFlag(cameraDesc.flags, RenderCamera::Flag::SkipSkyboxLighting) ? BE1::Color3::white : skyboxRadianceScale;

    if (useEnvironmentLight) {
        const EnvironmentLight::Desc &envLightDesc = envLight.GetDesc();
        frameCamera->envLightAxis = envLightDesc.axis;
        frameCamera->envLightColor = envLightDesc.linearColor;
        frameCamera->envLightIntensity = envLightDesc.intensity;

        Texture *diffuseIrradianceCubeMap = frame->RetainTexture(envLight.GetDiffuseIrradianceCubeMap());
        RHI::Texture *rhiDiffuseIrradianceCubeMap = diffuseIrradianceCubeMap ? diffuseIrradianceCubeMap->GetReadyRHITexture() : nullptr;
        if (rhiDiffuseIrradianceCubeMap && rhiDiffuseIrradianceCubeMap->type == RHI::TextureType::TextureCube) {
            frameCamera->envLightDiffuseIrradianceCubeMap = rhiDiffuseIrradianceCubeMap;
        }

        Texture *specularCubeMap = frame->RetainTexture(envLight.GetSpecularPrefilteredCubeMap());
        RHI::Texture *rhiSpecularCubeMap = specularCubeMap ? specularCubeMap->GetReadyRHITexture() : nullptr;
        if (rhiSpecularCubeMap && rhiSpecularCubeMap->type == RHI::TextureType::TextureCube) {
            frameCamera->envLightSpecularCubeMap = rhiSpecularCubeMap;
            frameCamera->envLightSpecularMipCount = rhiSpecularCubeMap->GetMipLevelCount();
        }
    }

    frameCamera->outputRect = outputRect;
    frameCamera->outputTexture = cameraDesc.outputTexture;
    frameCamera->outputViewID = cameraDesc.outputViewID;
    frameCamera->outputBeforeState = cameraDesc.outputBeforeState;
    frameCamera->outputAfterState = cameraDesc.outputAfterState;
    frameCamera->outputBlitPSO = outputBlitPSO;
    frameCamera->renderWidth = renderWidth;
    frameCamera->renderHeight = renderHeight;
    frameCamera->clearMethod = cameraDesc.clearMethod;
    frameCamera->clearColorMode = cameraDesc.clearColorMode;
    frameCamera->clearColor = cameraDesc.clearColor;
    frameCamera->projectionPolicy = cameraDesc.projectionPolicy;
    frameCamera->projectionScaleBiasZ = BE1::Vec2(renderCamera->projMatrix[2][2], renderCamera->projMatrix[2][3]);
    frameCamera->projectionOrthogonal = cameraDesc.orthogonal;
    frameCamera->projectionInfiniteZFar = cameraDesc.infiniteZFar;

    if (frameCamera->projectionOrthogonal) {
        frameCamera->projectionScaleBiasXY = BE1::Vec4(renderCamera->projMatrix[0][0], renderCamera->projMatrix[1][1], renderCamera->projMatrix[0][3], renderCamera->projMatrix[1][3]);
        frameCamera->viewBox = renderCamera->GetBox();
    } else {
        frameCamera->projectionScaleBiasXY = BE1::Vec4(renderCamera->projMatrix[0][0], renderCamera->projMatrix[1][1], -renderCamera->projMatrix[0][2], -renderCamera->projMatrix[1][2]);
        frameCamera->viewFrustum = renderCamera->GetFrustum();
    }

    frameCamera->flags = cameraDesc.flags;
    frameCamera->useSSR = useSSR;
    frameCamera->needRoughnessOutput = useSSR;
    frameCamera->needVelocityOutput = needVelocityTexture;

    frameCamera->colorTexture = renderCamera->GetColorTexture();
    frameCamera->depthTexture = renderCamera->GetDepthTexture();
    frameCamera->normalRoughnessTexture = renderCamera->GetNormalRoughnessTexture();
    frameCamera->velocityTexture = renderCamera->GetVelocityTexture();
    frameCamera->taaReactiveMaskTexture = renderCamera->GetTAAReactiveMaskTexture();

    frameCamera->resolvedColorTexture = renderCamera->GetResolvedColorTexture();
    frameCamera->resolvedDepthTexture = renderCamera->GetResolvedDepthTexture();
    frameCamera->resolvedNormalRoughnessTexture = renderCamera->GetResolvedNormalRoughnessTexture();
    frameCamera->resolvedVelocityTexture = renderCamera->GetResolvedVelocityTexture();
    frameCamera->resolvedTAAReactiveMaskTexture = renderCamera->GetResolvedTAAReactiveMaskTexture();

    frameCamera->sceneSnapshotDownsamplePSO = useHalfResSnapshot ? sceneSnapshotDownsamplePSO : nullptr;
    frameCamera->snapshotColorTexture = renderCamera->GetSnapshotColorTexture();
    frameCamera->snapshotDepthTexture = renderCamera->GetSnapshotDepthTexture();

    FrameScreenSpaceAO &screenSpaceAO = frameCamera->ao;
    screenSpaceAO.textureSet = renderCamera->screenSpaceAOResources.GetTextureSet();
    screenSpaceAO.depthTexture = useMSAA ? frameCamera->resolvedDepthTexture : frameCamera->depthTexture;
    screenSpaceAO.normalRoughnessTexture = useMSAA ? frameCamera->resolvedNormalRoughnessTexture : frameCamera->normalRoughnessTexture;
    screenSpaceAO.projectionPolicy = frameCamera->projectionPolicy;
    screenSpaceAO.mode = frameSettings.aoMode;
    screenSpaceAO.temporalNoise = frameSettings.aoTemporalNoise && !debugAO;

    frameCamera->applyAO = useAO && ScreenSpaceAO::HasRequiredResources(screenSpaceAO, frameSettings);
    frameCamera->needNormalOutput = frameCamera->applyAO || useSSR;

    frameCamera->generateSSRHiZ = requestSSRHiZGeneration;

    if (frameCamera->generateSSRHiZ) {
        FrameSSRHiZ &ssrHiZ = frameCamera->ssrHiZ;
        ssrHiZ.texture = ssrHiZResources.GetTexture();
        ssrHiZ.mipCount = ssrHiZResources.GetMipCount();
        ssrHiZ.validWidth = ssrHiZResources.GetValidWidth();
        ssrHiZ.validHeight = ssrHiZResources.GetValidHeight();

        for (uint32_t mipLevel = 0; mipLevel < RenderSSRHiZMaxLevelCount; ++mipLevel) {
            ssrHiZ.mipSRVs[mipLevel] = ssrHiZResources.GetMipSRVs()[mipLevel];
            ssrHiZ.mipUAVs[mipLevel] = ssrHiZResources.GetMipUAVs()[mipLevel];
            ssrHiZ.mipWidths[mipLevel] = ssrHiZResources.GetMipWidths()[mipLevel];
            ssrHiZ.mipHeights[mipLevel] = ssrHiZResources.GetMipHeights()[mipLevel];
        }
    }

    frameCamera->updateSSRSceneColorHistory = requestSSRSceneColorHistoryUpdate;
    if (requestSSRSceneColorHistoryUpdate) {
        FrameSSRSceneColorHistory &history = frameCamera->ssrSceneColorHistory;
        history.radianceTexture = ssrSceneColorHistoryResources.GetRadianceTexture();
        history.radiancePyramidTexture = ssrSceneColorHistoryResources.GetRadiancePyramidTexture();
        history.depthExtremaTexture = ssrSceneColorHistoryResources.GetDepthExtremaTexture();
        history.fullResolutionDepthTexture = ssrSceneColorHistoryResources.GetFullResolutionDepthTexture();
        history.mipCount = ssrSceneColorHistoryResources.GetMipCount();
        history.valid = ssrSceneColorHistoryResources.IsValid();

        for (uint32_t mipLevel = 0; mipLevel < RenderSSRSceneColorHistoryMaxLevelCount; ++mipLevel) {
            history.mipSRVs[mipLevel] = ssrSceneColorHistoryResources.GetMipSRVs()[mipLevel];
            history.mipUAVs[mipLevel] = ssrSceneColorHistoryResources.GetMipUAVs()[mipLevel];
            history.mipWidths[mipLevel] = ssrSceneColorHistoryResources.GetMipWidths()[mipLevel];
            history.mipHeights[mipLevel] = ssrSceneColorHistoryResources.GetMipHeights()[mipLevel];
        }
    }

    frameCamera->traceSSR = requestSSRTrace;
    frameCamera->ssrHistoryRenderState = renderCamera->ssrHistoryRenderState; // SSR 히스토리의 유효성을 추적하기 위해 프레임 카메라와 렌더 카메라 간에 SSR 히스토리 렌더 상태를 공유한다.
    frameCamera->ssrHistorySerial = ++renderCamera->ssrHistorySerial; // 카메라 별로 증가시키는 시리얼 번호를 사용하여 SSR 히스토리의 유효성을 추적한다.
    frameCamera->ssrTraceTexture = renderCamera->GetSSRTraceTexture();
    frameCamera->ssrReceiverSurfaceTexture = renderCamera->GetSSRReceiverSurfaceTexture();
    frameCamera->ssrResolveReflectionTexture = renderCamera->GetSSRResolveReflectionTexture();
    frameCamera->accumulateSSR = requestSSRTemporalAccumulation;
    frameCamera->ssrTemporalHistoryValid = renderCamera->IsSSRTemporalHistoryValid();
    frameCamera->ssrPreviousTemporalReflectionTexture = renderCamera->GetPreviousSSRTemporalReflectionTexture();
    frameCamera->ssrCurrentTemporalReflectionTexture = renderCamera->GetCurrentSSRTemporalReflectionTexture();
    frameCamera->ssrUpsampledReflectionTexture = renderCamera->GetSSRUpsampledReflectionTexture();

    frameCamera->traceTransparentSSR = requestTransparentSSRTrace;
    frameCamera->accumulateTransparentSSR = requestTransparentSSRTemporalAccumulation;

    frameCamera->generateHiZ = requestHiZGeneration;
    if (frameCamera->generateHiZ) {
        frameCamera->hiZ.texture = hiZResources.GetTexture();
        frameCamera->hiZ.mipCount = hiZResources.GetMipCount();

        for (uint32_t mipLevel = 0; mipLevel < RenderHiZMaxLevelCount; ++mipLevel) {
            frameCamera->hiZ.mipSRVs[mipLevel] = hiZResources.GetMipSRVs()[mipLevel];
            frameCamera->hiZ.mipUAVs[mipLevel] = hiZResources.GetMipUAVs()[mipLevel];
            frameCamera->hiZ.mipWidths[mipLevel] = hiZResources.GetMipWidths()[mipLevel];
            frameCamera->hiZ.mipHeights[mipLevel] = hiZResources.GetMipHeights()[mipLevel];
        }

        frameCamera->hiZOcclusionCulling = hiZResources.IsHistoryValid() && requestHiZOcclusionCulling;
        if (frameCamera->hiZOcclusionCulling) {
            frameCamera->hiZ.visibleInstanceBuffer = hiZResources.GetVisibleInstanceBuffer();
            frameCamera->hiZ.indirectArgsBuffer = hiZResources.GetIndirectArgsBuffer();
        }

        frameCamera->hiZDebugPreview = requestHiZDebugPreview;
        if (frameCamera->hiZDebugPreview) {
            frameCamera->hiZ.debugPreviewTexture = hiZResources.GetDebugPreviewTexture();
            frameCamera->hiZ.debugLinearDepthScale = cameraDesc.zFar > 0.0f ? 1.0f / cameraDesc.zFar : 1.0f;
        }
    }

    frameCamera->postProcessColorTexture = renderCamera->GetPostProcessColorTexture();
    frameCamera->colorHistoryTextures[0] = renderCamera->GetColorHistoryTexture(0);
    frameCamera->colorHistoryTextures[1] = renderCamera->GetColorHistoryTexture(1);
    frameCamera->depthHistoryTextures[0] = renderCamera->GetDepthHistoryTexture(0);
    frameCamera->depthHistoryTextures[1] = renderCamera->GetDepthHistoryTexture(1);
    frameCamera->currentHistoryTextureIndex = renderCamera->GetCurrentHistoryTextureIndex();
    frameCamera->historyValid = renderCamera->IsHistoryValid();

    frameCamera->luminanceBuffer = renderCamera->GetLuminanceBuffer();
    frameCamera->bloom.renderWidth = renderWidth;
    frameCamera->bloom.renderHeight = renderHeight;

    if (!skipPostProcess) {
        // LUT 의 최초 업로드는 엔진 초기화에서 완료된다.
        Texture *colorGradingLUTTexture = frame->RetainTexture(renderSystem->GetColorGradingLUTTexture());
        frameCamera->colorGradingLUTTexture = colorGradingLUTTexture->GetRHITexture();

        renderCamera->GetBloomResources().PrepareFrame(*frame, frameCamera->bloom);
    }

    frameCamera->worldAABB.Clear();
    frameCamera->is2D = false;
    frameCamera->viewMatrix = renderCamera->viewMatrix;

    const BE1::Vec2 jitterToNDCScale(2.0f / static_cast<float>(renderWidth), -2.0f / static_cast<float>(renderHeight));

    // Rasterization 은 jittered 행렬을 사용하고, velocity 계산용 unjittered 행렬은 backend 에서 jitter 를 제거해 복원한다.
    frameCamera->viewProjMatrix = renderCamera->jitteredViewProjMatrix;
    frameCamera->invViewProjMatrix = renderCamera->jitteredInvViewProjMatrix;
    frameCamera->ndcJitter = renderCamera->currJitter * jitterToNDCScale;

    const bool usePreviousMatrix = temporalHistoryEnabled && renderCamera->prevMatrixValid && !renderCamera->temporalHistoryInvalidated;

    if (usePreviousMatrix) {
        frameCamera->prevViewProjMatrix = renderCamera->prevJitteredViewProjMatrix;
        frameCamera->prevInvViewProjMatrix = renderCamera->prevJitteredInvViewProjMatrix;
        frameCamera->prevNdcJitter = renderCamera->prevJitter * jitterToNDCScale;
    } else {
        frameCamera->prevViewProjMatrix = renderCamera->jitteredViewProjMatrix;
        frameCamera->prevInvViewProjMatrix = renderCamera->jitteredInvViewProjMatrix;
        frameCamera->prevNdcJitter = frameCamera->ndcJitter;
    }

    frameCamera->taaResolve = useTAA;

    // freeze 가 되면 prevViewProjMatrix 를 유지하기 위해서 hiZCullViewProjMatrix 에 복사한다.
    if (!frameSettings.hiZOcclusionCullingFreeze || !hiZResources.IsHistoryValid()) {
        renderCamera->hiZCullViewProjMatrix = frameCamera->prevViewProjMatrix;
    }
    frameCamera->hiZCullViewProjMatrix = renderCamera->hiZCullViewProjMatrix;

    DrawCamera(frame, renderCamera, frameCamera);

    // Reserve the next history slots here; the backend additionally validates the producing
    // view serial before reading them. A requested but skipped pass never becomes valid there.
    ssrSceneColorHistoryResources.SetValid(frameCamera->updateSSRSceneColorHistory);
    renderCamera->CommitSSRTemporalHistory(frameCamera->accumulateSSR);
    renderCamera->CommitTransparentSSRTemporalHistory(frameCamera->accumulateTransparentSSR);

    if (temporalHistoryEnabled) {
        renderCamera->prevViewProjMatrix = renderCamera->viewProjMatrix;
        renderCamera->prevInvViewProjMatrix = renderCamera->invViewProjMatrix;
        renderCamera->prevJitteredViewProjMatrix = renderCamera->jitteredViewProjMatrix;
        renderCamera->prevJitteredInvViewProjMatrix = renderCamera->jitteredInvViewProjMatrix;
        renderCamera->prevMatrixValid = true;
        renderCamera->temporalHistoryInvalidated = false;

        if (frameCamera->taaResolve) {
            renderCamera->historyValid = true;
            renderCamera->currentHistoryTextureIndex ^= 1;
        } else {
            renderCamera->historyValid = false;
        }
    }

    if (!frameSettings.hiZOcclusionCullingFreeze) {
        hiZResources.SetHistoryValid(frameCamera->generateHiZ);
    }
}

void RenderWorld::DrawCamera(RenderFrame *frame, RenderCamera *renderCamera, FrameCamera *frameCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::DrawCamera");

    // RenderObject 중에서 컬링한 오브젝트를 프레임 스냅샷에 FrameObject 로 등록한다.
    FindFrameObjects(frame, renderCamera, frameCamera);

    // RenderLight 중에서 컬링한 라이트를 프레임 스냅샷에 FrameLight 로 등록한다.
    FindFrameLights(frame, renderCamera, frameCamera);

    // 각 CSM split 의 receiver 영역으로 directional shadow 투영을 준비하고 caster 를 수집한다.
    SetupFrameCascadedShadow(frame, renderCamera, frameCamera);

    // local shadow 들을 우선순위에 따라 atlas 에 타일을 할당하고 atlas 는 frameCamera 에 등록한다.
    SetupFrameShadowAtlas(frame, renderCamera, frameCamera);

    // RenderObject 중에서 shadow caster 가 될 오브젝트들을 프레임 스냅샷에 FrameObject 로 등록한다.
    // 카메라에 보이진 않지만 shadow caster 가 될 수 있는 오브젝트도 공통 FrameObject 로 등록한다.
    FindLocalShadowCasters(frame, renderCamera, frameCamera);

    // frameObjects 중에서 mesh surface 들을 frameCamera->frameSurfs 에 추가한다.
    AddMeshes(frame, renderCamera, frameCamera);

    // visibleLights 들로 GPU light culling 데이터를 구성한다.
    const RenderCamera::Desc &cameraDesc = renderCamera->GetDesc();
    LightCullingBuilder::Build(frame, frameCamera, cameraDesc.zNear, cameraDesc.zFar);

    // frameObjects 중에서 text mesh 를 frameCamera->frameSurfs 에 추가한다.
    AddTextMeshes(frame, renderCamera, frameCamera);

    // Add drawing surface of skybox.
    AddSkyBoxMeshes(frame, renderCamera, frameCamera);

    // Transparent SSR receiver 를 확인하고 프레임 데이터를 준비한다.
    SetupFrameTransparentSSR(frame, renderCamera, frameCamera);

    // Ocean surface 와 query 정보를 프레임 데이터에 등록한다.
    RegisterFrameOceanSurface(frame);

    // 카메라의 렌더 패스를 구성한다.
    BuildCameraPasses(frame, frameCamera);

    // Post-Opaque 본 패스와 transparent SSR receiver prepass 에 사용할 ocean pipeline 을 준비한다.
    PrepareOceanSurfacePipeline(frameCamera);

    // 카메라의 debug draw 데이터를 구성한다.
    BuildDebugDraw(frame, frameCamera);

    // 렌더링 커맨드에 frameCamera 를 기록한다.
    frame->CmdDrawCamera(frameCamera);

    viewCount++;
}

void RenderWorld::SetupFrameTransparentSSR(RenderFrame *frame, RenderCamera *renderCamera, FrameCamera *frameCamera) {
    if (!frameCamera->traceTransparentSSR) {
        return;
    }

    bool hasReceiver = hasOceanSurface && !oceanSurface.desc.surface.wireframe && !BE1::HasFlag(frameCamera->flags, RenderCamera::Flag::StaticOnly);

    for (uint32_t i = 0; !hasReceiver && i < frameCamera->numFrameSurfs; ++i) {
        const FrameSurf *surf = frameCamera->frameSurfs[i];
        if (!BE1::HasFlag(surf->flags, FrameSurf::Flag::CameraVisible) || surf->material->GetRenderingMode() != Material::RenderingMode::Blend) {
            continue;
        }

        const SharedPtr<ShaderTemplate> &shader = surf->material->GetShaderTemplate();
        hasReceiver = shader &&
            shader->SupportsFeature(ShaderFeature::SurfacePrepass) &&
            shader->SupportsFeature(ShaderFeature::TransparentSSRPrepass) &&
            shader->SupportsFeature(ShaderFeature::OutputNormal) &&
            shader->SupportsFeature(ShaderFeature::OutputVelocity) &&
            shader->SupportsFeature(ShaderFeature::OutputRoughness);
    }

    bool transparentSSRResourcesReady = false;
    if (hasReceiver) {
        transparentSSRResourcesReady = renderCamera->EnsureTransparentSSRResources(frameCamera->renderWidth, frameCamera->renderHeight, RenderContext::activeContext->GetMainRTDepthFormat(), frame->GetRenderFrameSettings().ssrResolutionMode, true);

        frameCamera->transparentSSRDepthTexture = renderCamera->GetCurrentTransparentSSRDepthTexture();
        frameCamera->transparentSSRPreviousDepthTexture = renderCamera->GetPreviousTransparentSSRDepthTexture();
        frameCamera->transparentSSRNormalRoughnessTexture = renderCamera->GetTransparentSSRNormalRoughnessTexture();
        frameCamera->transparentSSRVelocityTexture = renderCamera->GetTransparentSSRVelocityTexture();
        frameCamera->transparentSSRPreviousTemporalReflectionTexture = renderCamera->GetPreviousTransparentSSRTemporalReflectionTexture();
        frameCamera->transparentSSRCurrentTemporalReflectionTexture = renderCamera->GetCurrentTransparentSSRTemporalReflectionTexture();
        frameCamera->transparentSSRUpsampledReflectionTexture = renderCamera->GetTransparentSSRUpsampledReflectionTexture();
        frameCamera->transparentSSRTemporalHistoryValid = renderCamera->IsTransparentSSRTemporalHistoryValid();
    }
    frameCamera->traceTransparentSSR &= transparentSSRResourcesReady;
    frameCamera->accumulateTransparentSSR &= transparentSSRResourcesReady;
}

void RenderWorld::BuildCameraPasses(RenderFrame *frame, FrameCamera *frameCamera) {
    const bool allowAsyncPSOBuild = !BE1::HasFlag(frameCamera->flags, RenderCamera::Flag::DisableAsyncPSOBuild);

    // CSM 렌더 패스를 구성한다.
    if (frameCamera->frameCascadedShadow) {
        for (uint32_t cascadeIndex = 0; cascadeIndex < frameCamera->frameCascadedShadow->cascadeCount; cascadeIndex++) {
            renderPassBuilder.BuildShadowCascadePass(frame, frameCamera->frameCascadedShadow, cascadeIndex, frameCamera->frameSurfs, frameCamera->numFrameSurfs, allowAsyncPSOBuild);
        }
    }

    // 갱신할 local shadow 의 view 들만 렌더 패스를 구성한다.
    FrameShadowAtlas *frameShadowAtlas = frameCamera->frameShadowAtlas;
    if (frameShadowAtlas) {
        for (uint32_t shadowIndex = 0; shadowIndex < frameShadowAtlas->numShadows; ++shadowIndex) {
            const FrameLocalShadow &shadow = frameShadowAtlas->shadows[shadowIndex];
            if (shadow.updateDepth) {
                const uint32_t endShadowView = shadow.firstShadowView + shadow.numShadowViews;
                for (uint32_t shadowViewIndex = shadow.firstShadowView; shadowViewIndex < endShadowView; ++shadowViewIndex) {
                    renderPassBuilder.BuildLocalShadowPass(frame, frameShadowAtlas, shadowViewIndex, allowAsyncPSOBuild);
                }
            }
        }
    }

    // 장면 렌더 패스를 구성한다.
    renderPassBuilder.BuildScenePasses(frame, frameCamera);
}

// Receiver sphere 로 cascade 투영 영역을 계산한다. near 밖 caster 는 depth clamp 로 처리한다.
static bool CalcCSMCascadeProjection(CSMCascade &cascade, const BE1::Mat3 &lightAxis, const BE1::Sphere &receiverSphere, uint32_t resolution, uint32_t borderTexels) {
    const float radius = receiverSphere.Radius();
    if (radius <= 0.0f) {
        return false;
    }

    if (resolution <= 2 * borderTexels) {
        return false;
    }

    const uint32_t usableResolution = resolution - 2 * borderTexels;
    cascade.worldUnitsPerTexel = 2.0f * radius / static_cast<float>(usableResolution);
    if (cascade.worldUnitsPerTexel <= 0.0f) {
        return false;
    }

    // sphere radius 가 차지하는 (border 를 포함한) shadow map XY 크기를 계산한다.
    const float shadowMapExtent = static_cast<float>(resolution) * cascade.worldUnitsPerTexel * 0.5f;

    const BE1::Vec3 &sphereCenter = receiverSphere.Center();

    // world-space sphere center 의 light-space 좌표를 구한다. XY 는 texel grid 에 snap 한다.
    const float distLeft = BE1::Math::Snap(lightAxis[EngineAxis::Left].Dot(sphereCenter), cascade.worldUnitsPerTexel);
    const float distUp = BE1::Math::Snap(lightAxis[EngineAxis::Up].Dot(sphereCenter), cascade.worldUnitsPerTexel);
    const float distForward = lightAxis[EngineAxis::Forward].Dot(sphereCenter);

    // snap 된 light-space 좌표를 다시 world-space 좌표로 변환한다.
    const BE1::Vec3 shadowCenter = lightAxis[EngineAxis::Forward] * distForward + lightAxis[EngineAxis::Left] * distLeft + lightAxis[EngineAxis::Up] * distUp;

    // sphere 경계가 부동소수점 오차로 잘리지 않도록 depth 범위에 여유를 둔다.
    // caster 출입이나 sphere 중심 이동이 depthRange 를 바꾸지 않도록 반경에서 직접 계산한다.
    const float depthPadding = BE1::Max(0.001f, 2.0f * radius * 0.0001f);
    const float depthExtent = radius + depthPadding;
    const float depthMax = distForward + depthExtent;
    const float invDepthRange = 0.5f / depthExtent;

    // world-space sphere center 로 OBB 를 만든다. 광원 near 밖의 caster 는 depth clamp 로 보존한다.
    cascade.shadowMapBounds = BE1::OBB(shadowCenter, BE1::Vec3(depthExtent, shadowMapExtent, shadowMapExtent), lightAxis);

    // receiver sphere 만으로 reverse-Z (near 1, far 0) view-projection matrix 를 완성한다.
    const float invExtent = 1.0f / shadowMapExtent;
    cascade.viewProjMatrix[0] = BE1::Vec4(lightAxis[EngineAxis::Left] * -invExtent, distLeft * invExtent);
    cascade.viewProjMatrix[1] = BE1::Vec4(lightAxis[EngineAxis::Up] * invExtent, -distUp * invExtent);
    cascade.viewProjMatrix[2] = BE1::Vec4(lightAxis[EngineAxis::Forward] * -invDepthRange, depthMax * invDepthRange);
    cascade.viewProjMatrix[3] = BE1::Vec4(0.0f, 0.0f, 0.0f, 1.0f);

    return true;
}

static float3x4 BuildWorldToShadowCoordMatrix(const BE1::Mat4 &viewProjMatrix) {
    float3x4 matrix;
    for (int column = 0; column < 4; column++) {
        // D3D texture UV: clip XY [-1, 1] -> [0, 1], Y 는 반전. Depth 는 이미 [0, 1] 이다.
        matrix.m[0][column] = 0.5f * (viewProjMatrix[0][column] + viewProjMatrix[3][column]);
        matrix.m[1][column] = 0.5f * (-viewProjMatrix[1][column] + viewProjMatrix[3][column]);
        matrix.m[2][column] = viewProjMatrix[2][column];
    }
    return matrix;
}

void RenderWorld::SetupFrameCascadedShadow(RenderFrame *frame, const RenderCamera *renderCamera, FrameCamera *frameCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::SetupFrameCascadedShadow");

    if (!frameCamera->directionalLightVisible) {
        return;
    }

    const FrameLight *directionalLight = frame->GetPrimaryDirectionalLight();
    if (!directionalLight || !BE1::HasFlag(directionalLight->flags, RenderLight::Flag::CastShadows)) {
        return;
    }

    const RenderFrameSettings &frameSettings = frame->GetRenderFrameSettings();
    const RenderCamera::Desc &cameraDesc = renderCamera->GetDesc();
    const float csmMaxDistance = BE1::Min(cameraDesc.zFar, frameSettings.csmMaxDistance); // CSM 최대 거리
    if (!frameSettings.shadows || csmMaxDistance <= cameraDesc.zNear) {
        return;
    }

    const RenderContext *renderContext = RenderContext::activeContext;
    RHI::Texture *depthTexture = renderContext->GetCSMDepthTexture();
    if (!depthTexture) {
        return;
    }

    const uint32_t shadowMapResolution = renderContext->GetCSMResolution();
    constexpr float referenceUsableResolution = static_cast<float>(RenderFrameSettings::ShadowMapReferenceResolution - 2 * RenderFrameSettings::ShadowMapReferenceBorderTexels);
    constexpr float filterRadiusScale = RenderFrameSettings::ShadowMapReferenceMaxRadius / referenceUsableResolution;
    const uint32_t shadowMapBorderTexels = frameSettings.shadowMapQuality < 3 ? 2 : CalcShadowMapBorderTexels(shadowMapResolution, filterRadiusScale);

    FrameCascadedShadow *frameCascadedShadow = new (frame->MemAlloc(sizeof(FrameCascadedShadow))) FrameCascadedShadow();
    frameCascadedShadow->cascadeCount = frameSettings.csmCascadeCount;

    // cascade 별 distance 를 계산한다.
    ComputeCSMSplitDistances(cameraDesc.zNear, csmMaxDistance, frameSettings.csmSplitLambda, frameCascadedShadow->cascadeCount, cameraDesc.orthogonal, frameCascadedShadow->splitDistances);

    // 모든 cascade 의 caster shadow volume 검사에 사용할 전체 CSM 카메라 view planes 를 구한다.
    renderCamera->CalcSlicedViewVolume(cameraDesc.zNear, csmMaxDistance, nullptr, frameCascadedShadow->viewPlanes);

    // 각 cascade 의 receiver 범위로 projection 과 caster 컬링 경계를 함께 정한다.
    for (uint32_t cascadeIndex = 0; cascadeIndex < frameCascadedShadow->cascadeCount; cascadeIndex++) {
        // blending 중인 cascade 는 이전 receiver 구간까지 포함하여 map coverage overlap 을 보장한다.
        const float cascadeNear = (cascadeIndex == 0 || frameSettings.csmBlendFraction <= 0.0f) ? frameCascadedShadow->splitDistances[cascadeIndex] : frameCascadedShadow->splitDistances[cascadeIndex - 1];
        const float cascadeFar = frameCascadedShadow->splitDistances[cascadeIndex + 1];

        BE1::Sphere cascadeSphere;
        renderCamera->CalcSlicedViewVolume(cascadeNear, cascadeFar, &cascadeSphere);

        CSMCascade &cascade = frameCascadedShadow->cascades[cascadeIndex];
        if (!CalcCSMCascadeProjection(cascade, directionalLight->axis, cascadeSphere, shadowMapResolution, shadowMapBorderTexels)) {
            return;
        }

        cascade.depthDSV = renderContext->GetCSMDepthSliceDSV(cascadeIndex);

        // worldToShadowCoordMatrix * worldPos = shadowCoord (UV.xy, depth.z).
        // near 밖 caster 는 depth clamp 로 처리하므로 caster 수집 후 projection 을 바꾸지 않는다.
        frameCascadedShadow->shaderData.worldToShadowCoordMatrices[cascadeIndex] = BuildWorldToShadowCoordMatrix(cascade.viewProjMatrix);
    }

    // csmFilterRadius 는 reference resolution 기준으로 되어 있으므로 shadow map resolution 에 맞추기 위해 scale 값을 계산한다.
    const float shadowMapUsableResolution = static_cast<float>(shadowMapResolution - 2 * shadowMapBorderTexels);
    const float filterResolutionScale = shadowMapUsableResolution / referenceUsableResolution;

    const float maxFilterRadius = frameSettings.csmFilterRadius * filterResolutionScale;
    const float cascade0FilterRadiusInWorld = maxFilterRadius * frameCascadedShadow->cascades[0].worldUnitsPerTexel;

    // 각 cascade 의 filter radius 를 cascade 0 와 동일한 월드 반경이 되도록 texel 단위로 계산한다.
    float cascadeFilterRadii[RenderFrameSettings::CSMCascadeMaxCount] = { 1.0f, 1.0f, 1.0f, 1.0f };

    for (uint32_t cascadeIndex = 0; cascadeIndex < frameCascadedShadow->cascadeCount; cascadeIndex++) {
        const float worldUnitsPerTexel = frameCascadedShadow->cascades[cascadeIndex].worldUnitsPerTexel;
        cascadeFilterRadii[cascadeIndex] = cascade0FilterRadiusInWorld / worldUnitsPerTexel;
    }

    // RenderObject 를 한 번 순회하여 caster 와 cascade mask 를 수집한다.
    FindDirectionalShadowCasters(frame, renderCamera, frameCascadedShadow);

    frameCascadedShadow->depthTexture = depthTexture;

    frameCascadedShadow->shaderData.textureIndex = depthTexture->GetBindlessIndex(RHI::ViewType::SRV, renderContext->GetCSMDepthSRV());
    frameCascadedShadow->shaderData.cascadeCount = frameCascadedShadow->cascadeCount;
    frameCascadedShadow->shaderData.maxDistance = csmMaxDistance;
    frameCascadedShadow->shaderData.cascadeFilterRadii = float4(cascadeFilterRadii);

    const float texelSize = 1.0f / shadowMapResolution;
    frameCascadedShadow->shaderData.texelSize = float2(texelSize, texelSize);
    frameCascadedShadow->shaderData.normalizedBorder = shadowMapBorderTexels * texelSize;
    const float usableMapWidth = 1.0f - 2.0f * frameCascadedShadow->shaderData.normalizedBorder;
    const float cascadeBlendWidth = usableMapWidth * frameSettings.csmBlendFraction;
    frameCascadedShadow->shaderData.invBlendWidth = cascadeBlendWidth > 0.0f ? 1.0f / cascadeBlendWidth : 0.0f;

    const float farFadeRange = (csmMaxDistance - cameraDesc.zNear) * frameSettings.csmFadeFraction;
    frameCascadedShadow->shaderData.farFadeStart = csmMaxDistance - farFadeRange;
    frameCascadedShadow->shaderData.invFarFadeRange = 1.0f / farFadeRange;

    frameCamera->frameCascadedShadow = frameCascadedShadow;
}

static float CalcDirectionalShadowSquaredLODErrorLimit(const FrameCascadedShadow *cascadedShadow, uint8_t shadowCascadeMask, const BE1::Mat3x4 &worldMatrix, float shadowTexelErrorLimit) {
    float finestWorldUnitsPerTexel = 0.0f;

    for (uint32_t cascadeIndex = 0; cascadeIndex < cascadedShadow->cascadeCount; cascadeIndex++) {
        const uint8_t cascadeBit = static_cast<uint8_t>(BIT(cascadeIndex));
        if ((shadowCascadeMask & cascadeBit) == 0) {
            continue;
        }

        const float worldUnitsPerTexel = cascadedShadow->cascades[cascadeIndex].worldUnitsPerTexel;
        if (finestWorldUnitsPerTexel == 0.0f || worldUnitsPerTexel < finestWorldUnitsPerTexel) {
            finestWorldUnitsPerTexel = worldUnitsPerTexel;
        }
    }

    if (finestWorldUnitsPerTexel <= 0.0f) {
        return 0.0f;
    }

    float cascadePixelsPerWorldUnit = 1.0f / finestWorldUnitsPerTexel;
    return CalcLocalLODErrorLimitSqr(worldMatrix, cascadePixelsPerWorldUnit, shadowTexelErrorLimit);
}

void RenderWorld::FindDirectionalShadowCasters(RenderFrame *frame, const RenderCamera *renderCamera, const FrameCascadedShadow *cascadedShadow) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::FindDirectionalShadowCasters");

    const RenderCamera::Desc &cameraDesc = renderCamera->GetDesc();
    const bool renderStaticOnly = BE1::HasFlag(cameraDesc.flags, RenderCamera::Flag::StaticOnly);
    const float meshLODPixelError = frame->GetRenderFrameSettings().meshLODPixelError;

    for (RenderObject *renderObject : renderObjects) {
        if (!renderObject || !renderObject->desc.meshInstance || !BE1::HasFlag(renderObject->desc.flags, RenderObject::Flag::CastShadows)) {
            continue;
        }

        if (!IsRenderLayerVisible(cameraDesc.layerMask, renderObject->desc.layer)) {
            continue;
        }

        if (renderStaticOnly && !BE1::HasFlag(renderObject->desc.flags, RenderObject::Flag::Static)) {
            continue;
        }

        uint8_t shadowCascadeMask = 0;
        for (uint32_t cascadeIndex = 0; cascadeIndex < cascadedShadow->cascadeCount; cascadeIndex++) {
            const CSMCascade &cascade = cascadedShadow->cascades[cascadeIndex];

            if (!CullShadowCaster(cascadedShadow->viewPlanes, cascade.shadowMapBounds, renderObject->worldAABB)) {
                shadowCascadeMask |= (uint8_t)BIT(cascadeIndex);
            }
        }

        // 오브젝트가 모든 캐스캐이드에서 컬링되면 shadow-map 에 그릴 필요가 없다.
        if (shadowCascadeMask == 0) {
            continue;
        }

        FrameObject *frameObject = RegisterFrameObject(frame, renderObject);
        if (!frameObject) {
            continue;
        }

        // 카메라에 안 보이는 caster 는 shadow 에 현재 world matrix 만 필요하다.
        // 공용 ShaderSurface 가 prevWorldMatrix 도 업로드하므로 현재 행렬로 초기화한다.
        if (!BE1::HasFlag(frameObject->flags, FrameObject::Flag::CameraVisible)) {
            frameObject->worldMatrix = renderObject->GetWorldMatrix();
            frameObject->prevWorldMatrix = frameObject->worldMatrix;

            if (frameObject->meshLOD < 0 && meshLODPixelError > 0.0f) {
                const Mesh *mesh = frameObject->meshInstance->GetSourceMesh().get();
                if (mesh && mesh->HasLODs()) {
                    // 카메라 픽셀과 같은 허용 오차를 가장 정밀한 관련 cascade 의 shadow-map texel 단위로 적용한다.
                    frameObject->squaredLODErrorLimit = CalcDirectionalShadowSquaredLODErrorLimit(cascadedShadow, shadowCascadeMask, frameObject->worldMatrix, meshLODPixelError);
                }
            }
        }

        frameObject->shadowCascadeMask |= shadowCascadeMask;
    }
}

static float CalcLocalShadowTexelsPerWorldUnit(const FrameLocalShadowView &shadowView, float minDepth) {
    const BE1::Frustum &frustum = shadowView.frustum;

    // 광원 near plane 에 걸치거나 뒤에 있는 caster 는 카메라 LOD 와 동일하게 LOD 0 를 사용한다.
    if (minDepth <= frustum.GetNearDistance()) {
        return 0.0f;
    }

    // border 만큼 확장된 frustum 과 실제 할당된 전체 tile 크기를 함께 사용한다.
    // 할당 실패로 낮아진 해상도도 반영하며 border 를 이중으로 차감하지 않는다.
    const float projectedTexels = 0.5f * frustum.GetFarDistance() * BE1::Max(shadowView.tileRect.w / frustum.GetLeft(), shadowView.tileRect.h / frustum.GetUp());
    return projectedTexels / minDepth;
}

void RenderWorld::FindLocalShadowCasters(RenderFrame *frame, const RenderCamera *renderCamera, FrameCamera *frameCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::FindLocalShadowCasters");

    // 현재 카메라에서 갱신할 shadow 의 caster 참조만 수집한다.
    localShadowCasterRefs.SetCount(0, false);

    FrameShadowAtlas *frameShadowAtlas = frameCamera->frameShadowAtlas;
    if (!frameShadowAtlas || frameShadowAtlas->numShadowViews == 0) {
        return;
    }

    // 모든 그림자가 기존 depth 를 사용한다면 caster 오브젝트 순회와 컬링 준비도 생략한다.
    bool hasUpdates = false;
    for (uint32_t shadowIndex = 0; shadowIndex < frameShadowAtlas->numShadows; ++shadowIndex) {
        if (frameShadowAtlas->shadows[shadowIndex].updateDepth) {
            hasUpdates = true;
            break;
        }
    }

    if (!hasUpdates) {
        return;
    }

    const RenderCamera::Desc &cameraDesc = renderCamera->GetDesc();
    const bool renderStaticOnly = BE1::HasFlag(cameraDesc.flags, RenderCamera::Flag::StaticOnly);

    const BE1::Plane *viewPlanes = renderCamera->GetViewPlanes();
    const float meshLODPixelError = frame->GetRenderFrameSettings().meshLODPixelError;

    // 전체 render objects 를 순회하면서 shadow caster 가 될 수 있는 오브젝트들은 frame object 로 등록한다.
    for (RenderObject *renderObject : renderObjects) {
        if (!renderObject || !renderObject->desc.meshInstance || !BE1::HasFlag(renderObject->desc.flags, RenderObject::Flag::CastShadows)) {
            continue;
        }

        if (!IsRenderLayerVisible(cameraDesc.layerMask, renderObject->desc.layer) ||
            (renderStaticOnly && !BE1::HasFlag(renderObject->desc.flags, RenderObject::Flag::Static))) {
            continue;
        }

        if (renderObject->worldAABB.IsCleared()) {
            continue;
        }

        FrameObject *frameObject = nullptr;
        bool calcShadowLOD = false;
        bool useFinestLOD = false;
        float finestTexelsPerWorldUnit = 0.0f;
        LocalShadowCasterCuller casterCuller(renderObject->worldAABB, viewPlanes);

        for (uint32_t shadowIndex = 0; shadowIndex < frameShadowAtlas->numShadows; ++shadowIndex) {
            const FrameLocalShadow &shadow = frameShadowAtlas->shadows[shadowIndex];
            if (!shadow.updateDepth) {
                continue;
            }

            const FrameLight &light = *shadow.light;
            const bool staticShadows = BE1::HasFlag(light.flags, RenderLight::Flag::StaticShadows);
            if (staticShadows && !BE1::HasFlag(renderObject->desc.flags, RenderObject::Flag::Static)) {
                // static shadows 는 static 오브젝트만 caster 로 삼는다.
                continue;
            }
            const float *viewPlaneToOriginDistances = staticShadows ? nullptr : shadow.viewPlaneToOriginDistances;

            // Point 라이트의 영향 범위는 광원당 한 번 검사하고, 통과한 경우에만 6면에 접근한다.
            if (light.type == RenderLight::Type::Point && casterCuller.CullPointLight(light.origin, shadow.squaredRadius)) {
                continue;
            }

            bool registrationFailed = false;
            const uint32_t endShadowView = shadow.firstShadowView + shadow.numShadowViews;
            for (uint32_t shadowViewIndex = shadow.firstShadowView; shadowViewIndex < endShadowView; ++shadowViewIndex) {
                FrameLocalShadowView &shadowView = frameShadowAtlas->shadowViews[shadowViewIndex];

                // shadow frustum 과 카메라 view volume 기준으로 shadow caster 를 컬링한다.
                // static shadows 일 경우 카메라 view volume 과 무관하게 shadow frustum 전체의 caster 를 수집한다.
                float minDepth;
                if (casterCuller.Cull(shadowView, viewPlaneToOriginDistances, minDepth)) {
                    continue;
                }

                if (!frameObject) {
                    const bool alreadyRegistered = IsFrameObjectRegisteredInCurrentView(renderObject);
                    frameObject = RegisterFrameObject(frame, renderObject);
                    if (!frameObject) {
                        registrationFailed = true;
                        break;
                    }

                    if (!alreadyRegistered) {
                        // shadow caster 전용 객체에는 motion history 가 필요 없지만 공통 surface 업로드를 위해 두 행렬을 초기화한다.
                        frameObject->worldMatrix = renderObject->GetWorldMatrix();
                        frameObject->prevWorldMatrix = frameObject->worldMatrix;
                    }

                    // 이미 공유 LOD 가 0 으로 결정되었거나 LOD 를 명시한 경우 추가 계산이 필요 없다.
                    if (frameObject->meshLOD < 0 && meshLODPixelError > 0.0f && (!frameObject->IsRenderable() || frameObject->squaredLODErrorLimit > 0.0f)) {
                        const Mesh *mesh = frameObject->meshInstance->GetSourceMesh().get();
                        calcShadowLOD = mesh && mesh->HasLODs();
                    }

                    frameObject->firstLocalShadowCasterRef = static_cast<uint32_t>(localShadowCasterRefs.Count());
                    frameObject->flags |= FrameObject::Flag::LocalShadowCaster;
                }

                if (calcShadowLOD && !useFinestLOD) {
                    const float texelsPerWorldUnit = CalcLocalShadowTexelsPerWorldUnit(shadowView, minDepth);
                    useFinestLOD = texelsPerWorldUnit <= 0.0f;
                    finestTexelsPerWorldUnit = BE1::Max(finestTexelsPerWorldUnit, texelsPerWorldUnit);
                }

                // localShadowCasterRefs 이 꽉차면 2배씩 용량을 증가시킨다.
                if (localShadowCasterRefs.Count() == localShadowCasterRefs.Capacity()) {
                    localShadowCasterRefs.Reserve(BE1::Max(256, localShadowCasterRefs.Capacity() * 2));
                }

                LocalShadowCasterRef &casterRef = localShadowCasterRefs.Alloc();
                casterRef.object = frameObject; // shadow caster object
                casterRef.shadowViewIndex = shadowViewIndex; // shadow atlas 의 shadow view index

                frameObject->numLocalShadowCasterRefs++;
            }

            // 등록 실패 시 이후 광원에서도 재시도하지 않고 다음 오브젝트로 넘어간다.
            if (registrationFailed) {
                break;
            }
        }

        if (calcShadowLOD) {
            // 모든 관련 shadow view 중 가장 정밀한 texel 밀도를 선택한 뒤 world scale 은 객체당 한 번만 적용한다.
            const float squaredShadowLODErrorLimit = useFinestLOD ? 0.0f : CalcLocalLODErrorLimitSqr(frameObject->worldMatrix, finestTexelsPerWorldUnit, meshLODPixelError);
            const bool hasSharedLOD = BE1::HasFlag(frameObject->flags, FrameObject::Flag::CameraVisible) || frameObject->shadowCascadeMask != 0;
            frameObject->squaredLODErrorLimit = hasSharedLOD ? BE1::Min(frameObject->squaredLODErrorLimit, squaredShadowLODErrorLimit) : squaredShadowLODErrorLimit;
        }
    }
}

void RenderWorld::RegisterFrameOceanSurface(RenderFrame *frame) {
    assert(frameActive);

    if (!hasOceanSurface) {
        return;
    }

    if (const FrameOceanSurface *frameOceanSurface = frame->GetFrameOceanSurface()) {
        assert(frameOceanSurface->settingsRevision == oceanSurface.revision); // 현재 한 frame 에서는 하나의 ocean simulation 만 지원한다.
        return;
    }

    FrameOceanSurface *frameOceanSurface = frame->AllocFrameOceanSurface(oceanSurface.desc.simulation, oceanSurface.desc.surface);
    if (!frameOceanSurface) {
        return;
    }
    frameOceanSurface->settingsRevision = oceanSurface.revision;

    const OceanSurfaceQueryInput *surfaceQueries = oceanSurface.queries.Ptr();
    const uint32_t surfaceQueryCount = static_cast<uint32_t>(oceanSurface.queries.Count());
    if (surfaceQueries && surfaceQueryCount > 0) {
        const int queryDataSize = static_cast<int>(surfaceQueryCount * sizeof(OceanSurfaceQueryInput));
        OceanSurfaceQueryInput *queryCopy = static_cast<OceanSurfaceQueryInput *>(frame->MemAlloc(queryDataSize));
        if (queryCopy) {
            BE1::simdProcessor->Memcpy(queryCopy, surfaceQueries, queryDataSize);

            frameOceanSurface->surfaceQueries = queryCopy;
            frameOceanSurface->surfaceQueryCount = surfaceQueryCount;
        }
    }
}

void RenderWorld::PrepareOceanSurfacePipeline(FrameCamera *frameCamera) {
    if (!hasOceanSurface || BE1::HasFlag(frameCamera->flags, RenderCamera::Flag::StaticOnly)) {
        return;
    }

    const GpuOceanSurface::Settings &surfaceSettings = oceanSurface.desc.surface;

    const SharedPtr<ShaderTemplate> &shaderTemplate = renderSystem->GetGpuOceanSurface().GetShaderTemplate();
    if (!shaderTemplate) {
        return;
    }

    ShaderFeatureSet featureSet;
    if (frameCamera->taaResolve) {
        featureSet.EnableFeature(ShaderFeature::OutputVelocity);
    }

    if (hasEnvironmentLight && envLight.GetDesc().enabled) {
        featureSet.EnableFeature(ShaderFeature::IndirectLighting);
    }

    GraphicsPipelineDesc graphicsPipelineDesc;
    graphicsPipelineDesc.inputLayout = nullptr;
    graphicsPipelineDesc.rasterizerStateType = surfaceSettings.wireframe ? RHI::RasterizerStateType::Wire : RHI::RasterizerStateType::SolidTwoSided;
    graphicsPipelineDesc.depthStencilStateType = frameCamera->projectionPolicy.reverseZ ? RHI::DepthStencilStateType::GEqualWrite : RHI::DepthStencilStateType::LEqualWrite;
    graphicsPipelineDesc.blendStateType = RHI::BlendStateType::Disabled;
    graphicsPipelineDesc.primitiveTopology = RHI::PrimitiveTopology::TriangleList;
    RenderContext::activeContext->GetMainRenderDest(graphicsPipelineDesc.renderDest, false, false, frameCamera->taaResolve);

    const ShaderPermutationKey permutationKey = shaderTemplate->RegisterPermutation({}, featureSet);
    const bool allowAsyncPSOBuild = !BE1::HasFlag(frameCamera->flags, RenderCamera::Flag::DisableAsyncPSOBuild);
    ShaderPipeline *shaderPipeline = shaderPipelineCache.GetGraphicsPipelinePtr(shaderTemplate, permutationKey, graphicsPipelineDesc, allowAsyncPSOBuild);
    frameCamera->oceanSurfacePSO = ShaderPipeline::GetReadyPSO(shaderPipeline);

    if (frameCamera->traceTransparentSSR) {
        ShaderFeatureSet prepassFeatureSet;
        prepassFeatureSet.EnableFeature(ShaderFeature::SurfacePrepass);
        prepassFeatureSet.EnableFeature(ShaderFeature::TransparentSSRPrepass);
        prepassFeatureSet.EnableFeature(ShaderFeature::OutputNormal);
        prepassFeatureSet.EnableFeature(ShaderFeature::OutputVelocity);
        prepassFeatureSet.EnableFeature(ShaderFeature::OutputRoughness);

        GraphicsPipelineDesc prepassPipelineDesc;
        prepassPipelineDesc.inputLayout = nullptr;
        prepassPipelineDesc.rasterizerStateType = surfaceSettings.wireframe ? RHI::RasterizerStateType::Wire : RHI::RasterizerStateType::SolidTwoSided;
        prepassPipelineDesc.depthStencilStateType = frameCamera->projectionPolicy.reverseZ ? RHI::DepthStencilStateType::GEqualWrite : RHI::DepthStencilStateType::LEqualWrite;
        prepassPipelineDesc.blendStateType = RHI::BlendStateType::Disabled;
        prepassPipelineDesc.primitiveTopology = RHI::PrimitiveTopology::TriangleList;
        prepassPipelineDesc.renderDest.renderTargetCount = 2;
        prepassPipelineDesc.renderDest.renderTargetFormats[0] = frameCamera->transparentSSRNormalRoughnessTexture->GetFormat();
        prepassPipelineDesc.renderDest.renderTargetFormats[1] = frameCamera->transparentSSRVelocityTexture->GetFormat();
        prepassPipelineDesc.renderDest.depthStencilFormat = RenderContext::activeContext->GetMainRTDepthFormat();
        prepassPipelineDesc.renderDest.sampleCount = 1;

        const ShaderPermutationKey prepassPermutationKey = shaderTemplate->RegisterPermutation({}, prepassFeatureSet);
        ShaderPipeline *prepassPipeline = shaderPipelineCache.GetGraphicsPipelinePtr(shaderTemplate, prepassPermutationKey, prepassPipelineDesc, allowAsyncPSOBuild);
        frameCamera->oceanSurfaceSSRPrepassPSO = ShaderPipeline::GetReadyPSO(prepassPipeline);
    }
}

FrameLight *RenderWorld::RegisterFrameLight(RenderFrame *frame, RenderLight *renderLight) {
    assert(frameActive);

    if (IsFrameLightRegisteredInCurrentView(renderLight)) {
        return nullptr;
    }

    FrameLight *frameLight = frame->AllocFrameLight(renderLight->desc.type);
    if (!frameLight) {
        return nullptr;
    }

    renderLight->viewCount = viewCount;
    frameLight->lightId = renderLight->lightId;
    frameLight->shadowRevision = renderLight->shadowRevision;

    if (renderLight->desc.type == RenderLight::Type::Point) {
        frameLight->viewProjScaleBiasMatrix[0] = renderLight->viewProjScaleBiasMatrix[0];
        frameLight->viewProjScaleBiasMatrix[1] = renderLight->viewProjScaleBiasMatrix[1];
    } else if (renderLight->desc.type == RenderLight::Type::Spot) {
        frameLight->spotCosCone = renderLight->spotCosCone;
        frameLight->spotInvCosConeDifference = renderLight->spotInvCosConeDifference;
        frameLight->spotTanOuterCone = renderLight->spotTanOuterCone;
        frameLight->axis = renderLight->desc.axis;
    } else if (renderLight->desc.type == RenderLight::Type::Directional) {
        frameLight->axis = renderLight->desc.axis;
    }
    frameLight->origin = renderLight->desc.origin;
    frameLight->range = renderLight->desc.range;
    frameLight->intensityRGB = renderLight->intensityRGB;
    frameLight->zNear = renderLight->desc.zNear;
    frameLight->shadowOffsetUnits = renderLight->desc.shadowOffsetUnits;
    frameLight->shadowOffsetFactor = renderLight->desc.shadowOffsetFactor;
    frameLight->type = renderLight->desc.type;
    frameLight->flags = renderLight->desc.flags;

    return frameLight;
}

FrameObject *RenderWorld::RegisterFrameObject(RenderFrame *frame, RenderObject *renderObject) {
    assert(frameActive);

    if (IsFrameObjectRegisteredInCurrentView(renderObject)) {
        return renderObject->frameObject;
    }

    FrameObject *frameObject = frame->AllocFrameObject();
    if (!frameObject) {
        return nullptr;
    }

    renderObject->viewCount = viewCount;
    renderObject->frameObject = frameObject;
    frameObject->meshLOD = frame->GetRenderFrameSettings().meshLOD ? renderObject->GetMeshLOD() : 0;
    if (renderObject->nonUniformScale) {
        frameObject->flags |= FrameObject::Flag::NonUniformScale;
    }

    if (renderObject->desc.meshInstance) {
        frameObject->meshInstance = frame->RetainMeshInstance(renderObject->desc.meshInstance);
    }

    if (frameObject->meshInstance && frameObject->meshInstance->IsSkinned() && renderObject->desc.skeleton) {
        FrameSkinnedMesh *frameSkinnedMesh = frame->AllocFrameSkinnedMesh();
        if (frameSkinnedMesh) {
            frameSkinnedMesh->space = frameObject;
            frameSkinnedMesh->meshInstance = frameObject->meshInstance;
            frameSkinnedMesh->skeleton = frame->RetainSkeleton(renderObject->desc.skeleton);
            frameObject->skinnedMesh = frameSkinnedMesh;
        }
    }

    const int numMaterials = renderObject->desc.materials.Count();
    frameObject->numMaterials = static_cast<uint16_t>(numMaterials);
    if (numMaterials > 0) {
        const Material **materials;
        if (numMaterials > COUNT_OF(frameObject->baseMaterials)) {
            materials = (const Material **)frame->MemAlloc(numMaterials * sizeof(materials[0]));
            frameObject->materials = materials;
        } else {
            materials = frameObject->baseMaterials;
        }
        for (int i = 0; i < numMaterials; ++i) {
            materials[i] = frame->RetainMaterial(renderObject->desc.materials[i]);
        }
    }

    if (renderObject->desc.text && !renderObject->desc.text->text.IsEmpty()) {
        RenderObject::TextDesc &textDesc = *(renderObject->desc.text);
        FrameText *frameText = frame->AllocFrameText();
        if (frameText) {
            frameText->space = frameObject;
            if (textDesc.font) {
                frameText->font = frame->RetainFont(textDesc.font);
            }

            int size = textDesc.text.Length() + 1;
            char *textMemPtr = (char *)frame->MemAlloc(size);
            BE1::Str::Copynz(textMemPtr, textDesc.text.c_str(), size);
            frameText->text = textMemPtr;
            frameText->textRect = textDesc.textRect;
            frameText->textShadowOffset = textDesc.textShadowOffset;
            frameText->textShadowColor = textDesc.textShadowColor;
            frameText->textParams = textDesc.textParams;
            frameText->textScale = textDesc.textScale;
            frameText->textLineSpacing = textDesc.textLineSpacing;
        }
    }

    return frameObject;
}

void RenderWorld::SetFrameObjectWorldMatrices(const RenderObject *renderObject, FrameObject *frameObject) {
    frameObject->worldMatrix = renderObject->GetWorldMatrix();

    // 바로 직전 프레임에 prevWorldMatrix 를 세팅했었다면, 움직이던 오브젝트이므로 prevWorldMatrix 를 그대로 사용한다.
    const bool hasContinuousPrevWorldMatrix = frameIndex > 0 && renderObject->prevWorldMatrixFrameIndex == frameIndex - 1;

    frameObject->prevWorldMatrix = hasContinuousPrevWorldMatrix ? renderObject->prevWorldMatrix : renderObject->desc.worldMatrix;
}

void RenderWorld::FindFrameObjects(RenderFrame *frame, const RenderCamera *renderCamera, FrameCamera *frameCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::FindFrameObjects");

    const bool skipViewCulling = frame->GetRenderFrameSettings().skipViewCulling;
    const float meshLODPixelError = frame->GetRenderFrameSettings().meshLODPixelError;
    const RenderCamera::Desc &cameraDesc = renderCamera->GetDesc();
    const bool renderStaticOnly = BE1::HasFlag(cameraDesc.flags, RenderCamera::Flag::StaticOnly);

    // 같은 프레임 데이터로 RenderScene 을 여러 번 호출하면 앞선 카메라의 FrameObject, FrameSkinnedMesh, FrameText 가 누적되어 있다.
    // 현재 카메라에서 새로 등록할 각 레코드의 범위를 구분하기 위해 시작 인덱스를 기록한다.
    frameCamera->firstFrameObjectIndex = frame->NumFrameObjects();
    frameCamera->firstFrameSkinnedMeshIndex = frame->NumFrameSkinnedMeshes();
    frameCamera->firstFrameTextIndex = frame->NumFrameTexts();

    frameCamera->worldAABB.Clear();

    // 등록된 모든 RenderObject 들을 개별 Culling 해서 프레임 데이터에 FrameObject 로 등록한다.
    for (RenderObject *renderObject : renderObjects) {
        if (!renderObject) {
            continue;
        }

        if (!IsRenderLayerVisible(cameraDesc.layerMask, renderObject->desc.layer)) {
            continue;
        }

        if (renderStaticOnly && !BE1::HasFlag(renderObject->desc.flags, RenderObject::Flag::Static)) {
            continue;
        }

        if (!skipViewCulling) {
            if (renderCamera->Cull(renderObject->worldAABB)) {
                continue;
            }
        }

        FrameObject *frameObject = RegisterFrameObject(frame, renderObject);
        if (!frameObject) {
            continue;
        }

        renderObject->visibleFrameIndex = frameIndex;

        SetFrameObjectWorldMatrices(renderObject, frameObject);
        frameObject->flags |= FrameObject::Flag::CameraVisible;

        if (frameObject->meshInstance && frameObject->meshLOD < 0 && meshLODPixelError > 0.0f) {
            const Mesh *mesh = frameObject->meshInstance->GetSourceMesh().get();
            if (mesh && mesh->HasLODs()) {
                // 같은 카메라/객체의 SubMesh 들이 사용할 허용 오차를 한 번 계산한다.
                frameObject->squaredLODErrorLimit = renderCamera->CalcLocalLODErrorLimitSqr(frameObject->worldMatrix, renderObject->GetWorldAABB(), meshLODPixelError);
            }
        }

        frameCamera->worldAABB.AddAABB(renderObject->worldAABB);
    }
}

static int SelectMeshLOD(const SubMesh *subMesh, int requestedLOD, float squaredLODErrorLimit) {
    const int numLODs = subMesh->NumLODs();
    if (requestedLOD == 0 || numLODs <= 1) {
        return 0;
    }

    if (requestedLOD > 0) {
        return BE1::Min(requestedLOD, numLODs - 1);
    }

    if (squaredLODErrorLimit <= 0.0f) {
        return 0;
    }

    for (int level = numLODs - 1; level > 0; level--) {
        const float error = subMesh->GetLODError(level);
        if (error * error <= squaredLODErrorLimit) {
            return level;
        }
    }
    return 0;
}

void RenderWorld::AddMeshes(RenderFrame *frame, const RenderCamera *renderCamera, FrameCamera *frameCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::AddMeshes");

    localShadowSurfRefs.SetCount(0, false);

    // 현재 카메라에서 새로 등록된 FrameObject 가 없으면 메시 처리를 생략한다.
    if (frame->NumFrameObjects() == frameCamera->firstFrameObjectIndex) {
        return;
    }

    skinningJointBufferBuilder.Build(frame, frameCamera);

    const bool skipViewCulling = frame->GetRenderFrameSettings().skipViewCulling;
    const BE1::Plane *viewPlanes = renderCamera->GetViewPlanes();
    const FrameCascadedShadow *frameCascadedShadow = frameCamera->frameCascadedShadow;
    FrameShadowAtlas *frameShadowAtlas = frameCamera->frameShadowAtlas;

    for (FrameObject &frameObject : frame->GetFrameObjects(frameCamera->firstFrameObjectIndex)) {
        const bool objectCameraVisible = BE1::HasFlag(frameObject.flags, FrameObject::Flag::CameraVisible);
        const uint8_t objectShadowCascadeMask = frameCascadedShadow ? frameObject.shadowCascadeMask : 0;
        const uint32_t objectLocalShadowViewCount = frameShadowAtlas && BE1::HasFlag(frameObject.flags, FrameObject::Flag::LocalShadowCaster) ? frameObject.numLocalShadowCasterRefs : 0;

        if (!objectCameraVisible && objectShadowCascadeMask == 0 && objectLocalShadowViewCount == 0) {
            continue;
        }

        MeshInstance *meshInstance = frameObject.meshInstance;
        if (!meshInstance || !meshInstance->EnsureCurrent()) {
            continue;
        }

        const bool skinned = meshInstance->IsSkinned();

        // pose 준비에 실패한 skinned mesh 를 bind pose 상태로 등록하지 않는다.
        if (skinned && (!frameObject.skinnedMesh || frameObject.skinnedMesh->skinningJointOffset == ShaderSkinningJointOffsetInvalid)) {
            continue;
        }

        const int numSurfaces = meshInstance->NumSurfaces();

        // skinned submesh 의 bounds 는 bind pose 기준이므로 오브젝트 수집 단계의 bounds 판정을 유지한다.
        const bool canCullSurfaces = numSurfaces > 1 && !skinned;
        const bool cullCameraSurfaces = canCullSurfaces && objectCameraVisible && !skipViewCulling;

        for (int meshSurfIndex = 0; meshSurfIndex < numSurfaces; ++meshSurfIndex) {
            const Mesh::Surface *meshSurf = meshInstance->GetSurface(meshSurfIndex);
            if (!meshSurf || !meshSurf->subMesh) {
                continue;
            }

            const Material *material = materialManager.defaultLitMaterial.get();
            if (meshSurf->materialIndex >= 0 && static_cast<uint32_t>(meshSurf->materialIndex) < frameObject.numMaterials) {
                if (const Material *surfaceMaterial = frameObject.GetMaterials()[meshSurf->materialIndex]) {
                    material = surfaceMaterial;
                }
            }

            const bool castsShadow = !BE1::HasFlag(material->GetFlags(), Material::Flag::NoShadow) && material->GetRenderingMode() != Material::RenderingMode::Blend;

            // 아래 가시성 값은 오브젝트 컬링 결과를 물려받은 렌더링 후보 상태이며, 아직 surface 단위 컬링 전이다.
            bool surfaceCameraVisible = objectCameraVisible;
            uint8_t surfaceShadowCascadeMask = castsShadow ? objectShadowCascadeMask : 0;
            const uint32_t surfaceLocalShadowViewCount = castsShadow ? objectLocalShadowViewCount : 0;
            bool surfaceShadowVisible = surfaceShadowCascadeMask != 0 || surfaceLocalShadowViewCount != 0;

            // camera 와 shadow 렌더링에 모두 필요 없는 오브젝트는 건너뛴다.
            if (!surfaceCameraVisible && !surfaceShadowVisible) {
                continue;
            }

            // surface 단위로 camera 와 CSM 컬링을 수행한다.
            BE1::AABB surfaceWorldAABB;
            bool hasSurfaceWorldBounds = false;
            if (cullCameraSurfaces || (canCullSurfaces && surfaceShadowVisible)) {
                const BE1::AABB &localAABB = meshSurf->subMesh->GetBoundingBox();
                if (!localAABB.IsCleared()) {
                    // static submesh 의 world bounds 는 카메라, CSM, 모든 local shadow 컬링에 한 번만 계산하여 공유한다.
                    surfaceWorldAABB.SetFromTransformedAABB(localAABB, frameObject.worldMatrix);
                    hasSurfaceWorldBounds = true;

                    if (!surfaceWorldAABB.IsCleared()) {
                        if (cullCameraSurfaces) {
                            surfaceCameraVisible = !renderCamera->Cull(surfaceWorldAABB);
                        }

                        if (surfaceShadowCascadeMask != 0) {
                            surfaceShadowCascadeMask = frameCascadedShadow->CullCascades(surfaceWorldAABB, surfaceShadowCascadeMask);
                        }
                    }
                }
            }

            // 이 surface 를 그릴 local shadow view 들의 참조를 수집한다.
            const int firstSurfaceRef = localShadowSurfRefs.Count();
            uint32_t testedShadowIndex = ~0u;
            bool outsidePointShadow = false;
            LocalShadowCasterCuller surfaceCuller(surfaceWorldAABB, viewPlanes);

            for (uint32_t viewRefIndex = 0; viewRefIndex < surfaceLocalShadowViewCount; ++viewRefIndex) {
                const LocalShadowCasterRef &casterRef = localShadowCasterRefs[frameObject.firstLocalShadowCasterRef + viewRefIndex];
                assert(casterRef.object == &frameObject);

                // local shadow view 에 대해 컬링을 수행한다.
                const FrameLocalShadowView &shadowView = frameShadowAtlas->shadowViews[casterRef.shadowViewIndex];
                assert(frameShadowAtlas->shadows[shadowView.shadowIndex].updateDepth);
                if (hasSurfaceWorldBounds) {
                    const FrameLocalShadow &shadow = frameShadowAtlas->shadows[shadowView.shadowIndex];
                    // 같은 광원의 참조는 연속된다. 오브젝트와 교차한 포인트 라이트 구라도 submesh 는 바깥일 수 있으므로 광원당 한 번 검사한다.
                    if (testedShadowIndex != shadowView.shadowIndex) {
                        testedShadowIndex = shadowView.shadowIndex;
                        const FrameLight &light = *shadow.light;
                        outsidePointShadow = light.type == RenderLight::Type::Point && surfaceCuller.CullPointLight(light.origin, shadow.squaredRadius);
                    }

                    float minDepth;
                    const float *viewPlaneToOriginDistances = BE1::HasFlag(shadow.light->flags, RenderLight::Flag::StaticShadows) ? nullptr : shadow.viewPlaneToOriginDistances;
                    if (outsidePointShadow || surfaceCuller.Cull(shadowView, viewPlaneToOriginDistances, minDepth)) {
                        continue;
                    }
                }

                // localShadowSurfRefs 이 꽉차면 2배씩 용량을 증가시킨다.
                if (localShadowSurfRefs.Count() == localShadowSurfRefs.Capacity()) {
                    localShadowSurfRefs.Reserve(BE1::Max(256, localShadowSurfRefs.Capacity() * 2));
                }

                LocalShadowSurfRef &surfRef = localShadowSurfRefs.Alloc();
                surfRef.surface = nullptr;
                surfRef.shadowViewIndex = casterRef.shadowViewIndex;
            }

            // surface 컬링 후 남은 CSM cascade 와 local shadow 참조로 최종 shadow 가시성을 갱신한다.
            surfaceShadowVisible = surfaceShadowCascadeMask != 0 || localShadowSurfRefs.Count() != firstSurfaceRef;

            // camera, CSM, local shadow 어디에도 필요없는 surface 는 건너뛴다.
            if (!surfaceCameraVisible && !surfaceShadowVisible) {
                continue;
            }

            const FrameSurf::Flag surfaceflags = surfaceCameraVisible ? FrameSurf::Flag::CameraVisible : FrameSurf::Flag::None;

            // camera 또는 CSM 기준 LOD 에 local shadow 의 texel 오차 제한을 함께 적용한다.
            // 한 surface 가 여러 shadow view 에 투영되어도 같은 LOD 를 공유하여 cascade blend 영역과 Point 면 경계에서 caster silhouette 를 일치시킨다.
            const int lodLevel = SelectMeshLOD(meshSurf->subMesh, frameObject.meshLOD, frameObject.squaredLODErrorLimit);
            const FrameSurf *frameSurf = AddFrameSurf(frame, frameCamera, &frameObject, material, meshSurf->subMesh, surfaceflags, surfaceShadowCascadeMask, lodLevel);
            if (!frameSurf) {
                // 공통 surface 생성 실패 시 어느 shadow view 에도 미완성 참조를 남기지 않는다.
                localShadowSurfRefs.SetCount(firstSurfaceRef, false);
                continue;
            }

            for (int surfRefIndex = firstSurfaceRef; surfRefIndex < localShadowSurfRefs.Count(); ++surfRefIndex) {
                LocalShadowSurfRef &surfRef = localShadowSurfRefs[surfRefIndex];
                surfRef.surface = frameSurf;
                frameShadowAtlas->shadowViews[surfRef.shadowViewIndex].numShadowSurfs++;
            }
        }
    }

    BuildLocalShadowSurfaceLists(frame, frameShadowAtlas);
}

void RenderWorld::BuildLocalShadowSurfaceLists(RenderFrame *frame, FrameShadowAtlas *frameShadowAtlas) {
    if (localShadowSurfRefs.Count() == 0) {
        return;
    }

    assert(frameShadowAtlas);

    // 갱신할 shadowView 에 들어갈 모든 shadow surface 포인터들을 한꺼번에 할당한다.
    const size_t size = sizeof(FrameSurf *) * static_cast<size_t>(localShadowSurfRefs.Count());
    const FrameSurf **surfs = static_cast<const FrameSurf **>(frame->MemAlloc(static_cast<int>(size)));
    if (!surfs) {
        for (uint32_t shadowViewIndex = 0; shadowViewIndex < frameShadowAtlas->numShadowViews; ++shadowViewIndex) {
            frameShadowAtlas->shadowViews[shadowViewIndex].numShadowSurfs = 0;
        }
        return;
    }

    for (uint32_t shadowViewIndex = 0; shadowViewIndex < frameShadowAtlas->numShadowViews; ++shadowViewIndex) {
        FrameLocalShadowView &shadowView = frameShadowAtlas->shadowViews[shadowViewIndex];
        if (shadowView.numShadowSurfs > 0) {
            shadowView.shadowSurfs = surfs;
            surfs += shadowView.numShadowSurfs;
            shadowView.numShadowSurfs = 0;
        }
    }

    for (const LocalShadowSurfRef &surfRef : localShadowSurfRefs) {
        FrameLocalShadowView &shadowView = frameShadowAtlas->shadowViews[surfRef.shadowViewIndex];
        shadowView.shadowSurfs[shadowView.numShadowSurfs++] = surfRef.surface;
    }
}

void RenderWorld::AddTextMeshes(RenderFrame *frame, const RenderCamera *renderCamera, FrameCamera *frameCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::AddTextMeshes");

    // 이전 카메라에서 등록한 FrameText 는 제외하고 현재 카메라의 시작 위치부터 순회한다.
    for (FrameText &frameText : frame->GetFrameTexts(frameCamera->firstFrameTextIndex)) {
        FrameObject *frameObject = frameText.space;
        if (!BE1::HasFlag(frameObject->flags, FrameObject::Flag::CameraVisible)) {
            continue;
        }

        textMesh.Clear();

        if (frameText.textRect.IsEmpty()) {
            textMesh.DrawText3D(frame->GetThreadData(0), (RenderObject::TextDrawMode)frameText.textParams.bits.textDrawMode,
                (RenderObject::TextAnchor)frameText.textParams.bits.textAnchor, (RenderObject::TextHorzAlignment)frameText.textParams.bits.textHorzAlignment,
                frameText.textLineSpacing, frameText.textScale, BE1::Color4::white.ToUInt32(), frameText.textShadowColor, frameText.textShadowOffset, frameText.font, frameText.text);
        } else {
            textMesh.DrawText3D(frame->GetThreadData(0), (RenderObject::TextDrawMode)frameText.textParams.bits.textDrawMode, frameText.textRect,
                (RenderObject::TextHorzAlignment)frameText.textParams.bits.textHorzAlignment, (RenderObject::TextVertAlignment)frameText.textParams.bits.textVertAlignment,
                (RenderObject::TextHorzOverflow)frameText.textParams.bits.textHorzOverflow, (RenderObject::TextVertOverflow)frameText.textParams.bits.textVertOverflow,
                frameText.textLineSpacing, frameText.textScale, BE1::Color4::white.ToUInt32(), frameText.textShadowColor, frameText.textShadowOffset, frameText.font, frameText.text);
        }

        textMesh.CacheIndexes(frame->GetThreadData(0));

        const int numSurfaces = textMesh.NumSurfaces();
        for (int surfaceIndex = 0; surfaceIndex < numSurfaces; surfaceIndex++) {
            const GuiMesh::Surface *guiSurf = textMesh.GetSurface(surfaceIndex);
            if (!guiSurf->numIndexes) {
                break;
            }

            SubMesh *subMesh = (SubMesh *)frame->MemAlloc(sizeof(SubMesh));
            new (subMesh) SubMesh();

            subMesh->numIndexes = guiSurf->numIndexes;
            subMesh->numVerts = guiSurf->numVerts;
            subMesh->vertexBuffer = guiSurf->vertexBuffer;
            subMesh->indexBuffer = guiSurf->indexBuffer;

            AddFrameSurf(frame, frameCamera, frameObject, guiSurf->material, subMesh, FrameSurf::Flag::CameraVisible, 0);
        }
    }
}

void RenderWorld::AddSkyBoxMeshes(RenderFrame *frame, const RenderCamera *renderCamera, FrameCamera *frameCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::AddSkyBoxMeshes");

    const RenderCamera::Desc &cameraDesc = renderCamera->GetDesc();
    if (cameraDesc.clearMethod != RenderCamera::ClearMethod::Skybox) {
        return;
    }

    if (!skyboxMaterial || !meshManager.defaultBoxMesh || !meshManager.defaultBoxMesh->IsReady()) {
        return;
    }

    const Mesh::Surface *meshSurf = meshManager.defaultBoxMesh->GetSurface(0);
    if (!meshSurf || !meshSurf->subMesh) {
        return;
    }

    FrameObject *frameObject = frame->AllocFrameObject();
    if (!frameObject) {
        return;
    }

    const BE1::Mat3 &axis = BE1::HasFlag(cameraDesc.flags, RenderCamera::Flag::SkipSkyboxLighting) ? BE1::Mat3::identity : skyboxAxis;
    frameObject->worldMatrix.SetTRS(cameraDesc.origin, axis, BE1::Vec3(1, 1, 1));
    frameObject->prevWorldMatrix = frameObject->worldMatrix;
    frameObject->flags |= FrameObject::Flag::CameraVisible;
    Material *material = frame->RetainMaterial(skyboxMaterial);

    AddFrameSurf(frame, frameCamera, frameObject, material, meshSurf->subMesh, FrameSurf::Flag::CameraVisible | FrameSurf::Flag::SkipSelection | FrameSurf::Flag::Skybox, 0);
}

void RenderWorld::FindFrameLights(RenderFrame *frame, RenderCamera *renderCamera, FrameCamera *frameCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::FindFrameLights");

    localShadowLightCandidates.SetCount(0, false);

    const RenderFrameSettings &frameSettings = frame->GetRenderFrameSettings();
    const bool skipViewCulling = frameSettings.skipViewCulling;
    const bool localShadowsEnabled = frameSettings.shadows && frameSettings.shadowAtlasMaxViews > 0;

    const RenderCamera::Desc &cameraDesc = renderCamera->GetDesc();
    const bool useShadowResolutionHistory = localShadowsEnabled && !BE1::HasFlag(cameraDesc.flags, RenderCamera::Flag::DisableTemporalHistory);
    if (useShadowResolutionHistory) {
        renderCamera->localShadowResolutionHistory.BeginView();
    } else {
        renderCamera->localShadowResolutionHistory.Reset();
    }
    const bool renderStaticOnly = BE1::HasFlag(cameraDesc.flags, RenderCamera::Flag::StaticOnly);
    const BE1::Vec3 &viewZAxis = cameraDesc.axis[EngineAxis::Forward];
    const float viewZOffset = -viewZAxis.Dot(cameraDesc.origin);

    // 현재 카메라의 local light 참조가 시작되는 프레임 공용 ShaderLightRef 테이블의 오프셋을 기록한다.
    frameCamera->lightCulling.lightRefTableOffset = frame->NumFrameLocalLights();

    // RenderWorld 에 등록된 renderLights 개수만큼 미리 frameCamera->frameLocalLights 를 할당
    frameCamera->maxFrameLocalLights = static_cast<uint32_t>(renderLights.Count());
    if (frameCamera->maxFrameLocalLights > 0) {
        frameCamera->frameLocalLights = (const FrameLight **)frame->MemAlloc(frameCamera->maxFrameLocalLights * sizeof(frameCamera->frameLocalLights[0]));
    }

    // 등록된 모든 RenderLight 들을 프레임 스냅샷에 FrameLight 로 등록한다.
    for (RenderLight *renderLight : renderLights) {
        if (!renderLight) {
            continue;
        }

        if (!IsRenderLayerVisible(cameraDesc.layerMask, renderLight->desc.layer)) {
            continue;
        }

        if (renderStaticOnly && !BE1::HasFlag(renderLight->desc.flags, RenderLight::Flag::Static)) {
            continue;
        }

        if (renderLight->desc.type == RenderLight::Type::Directional) {
            frameCamera->directionalLightVisible = true;

            // Directional light 는 모든 카메라가 공유하는 프레임 단위 Primary light 하나만 등록한다.
            if (frame->GetPrimaryDirectionalLight()) {
                continue;
            }

            FrameLight *frameLight = RegisterFrameLight(frame, renderLight);
            if (!frameLight) {
                continue;
            }
        } else {
            if (!skipViewCulling) {
                if (renderLight->Cull(*renderCamera)) {
                    continue;
                }

                if (!renderLight->IsIntersectAABB(frameCamera->worldAABB)) {
                    continue;
                }
            }

            // 라이트의 bounding volume 을 camera forward axis 에 투영해서 linear view Z min/max (화면 안쪽이 양수) 를 계산한다.
            float viewZMin;
            float viewZMax;
            renderLight->ProjectOnAxis(viewZAxis, viewZMin, viewZMax);
            viewZMin += viewZOffset;
            viewZMax += viewZOffset;

            // 라이트가 카메라 [zNear, zFar] 범위 밖에 있으면 제외
            if (viewZMax < cameraDesc.zNear || viewZMin > cameraDesc.zFar) {
                continue;
            }

            // 라이트의 clipRect 를 계산하고, 화면 밖이라면 제외
            BE1::Rect screenClipRect;
            if (!renderLight->ComputeScreenClipRect(*renderCamera, screenClipRect)) {
                continue;
            }

            FrameLight *frameLight = RegisterFrameLight(frame, renderLight);
            if (!frameLight) {
                continue;
            }

            frameLight->viewZMin = viewZMin;
            frameLight->viewZMax = viewZMax;

            // 광원의 light culling 타일 범위와 화면 투영 크기를 계산한다.
            const uint32_t projectedSize = LightCullingBuilder::ComputeTileRange(frameCamera, screenClipRect, frameLight->packedTileMin, frameLight->packedTileMax);
            frameLight->projectedSize = projectedSize;

            if (localShadowsEnabled && BE1::HasFlag(frameLight->flags, RenderLight::Flag::CastShadows)) {
                // 투영 영역의 긴 변이 최소 크기 미만이면 shadow light 후보에서 제외한다.
                if (projectedSize >= RenderFrameSettings::LocalShadowMinProjectedSize && (frameLight->type == RenderLight::Type::Spot || frameLight->type == RenderLight::Type::Point)) {
                    const float projectedSizeScale = frameLight->type == RenderLight::Type::Spot
                        ? frameSettings.shadowMapProjectedSizeScaleSpot : frameSettings.shadowMapProjectedSizeScalePoint;
                    const float scaledProjectedSize = projectedSize * projectedSizeScale;

                    if (useShadowResolutionHistory) {
                        // 광원 영향 영역에 해상도 배율을 적용해 요청 해상도를 근사하고, 카메라별 history 로 경계 부근의 반복 변경을 막는다.
                        // shadow atlas 의 공간 부족으로 축소된 실제 할당 해상도는 요청 history 에 반영하지 않는다.
                        frameLight->shadowRequestedResolution = renderCamera->localShadowResolutionHistory.Update(renderLight->index, renderLight->lightId, frameSettings.shadowAtlasMaxTileResolution, scaledProjectedSize);
                    } else {
                        frameLight->shadowRequestedResolution = ShadowResolutionHistory::SelectResolution(0, frameSettings.shadowAtlasMaxTileResolution, scaledProjectedSize);
                    }
                    localShadowLightCandidates.Alloc() = frameLight;
                }
            }

            frameCamera->frameLocalLights[frameCamera->numFrameLocalLights++] = frameLight;
        }

        renderLight->visibleFrameIndex = frameIndex;
    }
}

void RenderWorld::SetupFrameShadowAtlas(RenderFrame *frame, RenderCamera *renderCamera, FrameCamera *frameCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::SetupFrameShadowAtlas");

    const RenderFrameSettings &frameSettings = frame->GetRenderFrameSettings();
    if (!renderCamera->EnsureShadowAtlas(frameSettings, localShadowLightCandidates.Count() > 0)) {
        BE_WARNLOG("RenderWorld::SetupFrameShadowAtlas: failed to update camera shadow atlas\n");
        return;
    }
    ShadowAtlas &shadowAtlas = renderCamera->GetShadowAtlas();

    // 타일 할당은 광원별로 유지한다. 같은 카메라를 여러 번 제출해도 이전 호출의 좌표는 frame snapshot 에 보존된다.
    frameCamera->frameShadowAtlas = frame->AllocFrameShadowAtlas(shadowAtlas);
    if (localShadowLightCandidates.Count() == 0) {
        shadowAtlas.FreeUnusedEntries();
        return;
    }

    if (!frameCamera->frameShadowAtlas) {
        return;
    }

    FrameShadowAtlas *frameShadowAtlas = frameCamera->frameShadowAtlas;
    frameShadowAtlas->numCandidates = static_cast<uint32_t>(localShadowLightCandidates.Count());
    const BE1::Plane *viewPlanes = renderCamera->GetViewPlanes();
    const uint32_t shadowMapQuality = frameSettings.shadowMapQuality;
    const uint32_t frameCount = frame->GetFrameCount();
    const uint32_t casterLayerMask = renderCamera->GetDesc().layerMask;

    // Q3 최대 filter radius 를 기준으로 border 를 확보하여 radius 조절 시 projection 을 유지한다.
    constexpr float referenceUsableResolution = static_cast<float>(RenderFrameSettings::ShadowMapReferenceResolution - 2 * RenderFrameSettings::ShadowMapReferenceBorderTexels);
    constexpr float filterRadiusScale = RenderFrameSettings::ShadowMapReferenceMaxRadius / referenceUsableResolution;

    // 그림자 라이트 후보들을 shadow 요청 해상도가 크고, 카메라에 가까운 순으로 정렬한다.
    if (localShadowLightCandidates.Count() > 1) {
        std::sort(localShadowLightCandidates.Ptr(), localShadowLightCandidates.Ptr() + localShadowLightCandidates.Count(), [](const FrameLight *lhs, const FrameLight *rhs) {
            if (lhs->shadowRequestedResolution != rhs->shadowRequestedResolution) {
                return lhs->shadowRequestedResolution > rhs->shadowRequestedResolution;
            }
            const float lhsDistance = BE1::Max(0.0f, lhs->viewZMin);
            const float rhsDistance = BE1::Max(0.0f, rhs->viewZMin);
            if (lhsDistance != rhsDistance) {
                return lhsDistance < rhsDistance;
            }
            return lhs->index < rhs->index;
        });
    }

    // 새 타일 묶음을 할당 전에 재사용 가능한 타일 묶음을 keep 처리한다.
    // 면 수가 같으며 기존 요청 해상도 또는 실제 해상도가 새 요청 해상도와 일치하는 라이트의 타일 묶음만 유지한다.
    uint32_t reservedShadowViews = 0;
    for (const FrameLight *shadowLight : localShadowLightCandidates) {
        if (reservedShadowViews == frameShadowAtlas->maxShadowViews) {
            break;
        }

        const uint32_t shadowViewCount = shadowLight->GetShadowViewCount();
        if (shadowViewCount > frameShadowAtlas->maxShadowViews - reservedShadowViews) {
            continue;
        }

        reservedShadowViews += shadowViewCount;
        const uint32_t requestedResolution = BE1::Min(shadowLight->shadowRequestedResolution, shadowAtlas.GetMaxTileResolution());
        shadowAtlas.KeepEntry(shadowLight->lightId, requestedResolution, shadowViewCount);
    }
    shadowAtlas.FreeUnusedEntries();

    for (uint32_t candidateIndex = 0; candidateIndex < frameShadowAtlas->numCandidates; ++candidateIndex) {
        if (frameShadowAtlas->numShadowViews >= frameShadowAtlas->maxShadowViews) {
            // shadow view 예산을 초과했다면 남은 라이트 후보는 추가 순회 없이 제외된 라이트로 집계한다.
            frameShadowAtlas->numSkippedLights += frameShadowAtlas->numCandidates - candidateIndex;
            break;
        }

        FrameLight *shadowLight = localShadowLightCandidates[candidateIndex];
        const uint32_t shadowViewCount = shadowLight->GetShadowViewCount();
        assert(shadowViewCount > 0);
        if (shadowViewCount > frameShadowAtlas->maxShadowViews - frameShadowAtlas->numShadowViews) {
            frameShadowAtlas->numSkippedLights++;
            continue;
        }

        const uint32_t requestedResolution = BE1::Min(shadowLight->shadowRequestedResolution, shadowAtlas.GetMaxTileResolution());

        // 아틀라스에 shadow entry 가 존재하지 않는다면 새로 할당한다.
        ShadowAtlas::Entry *entry = shadowAtlas.FindEntry(shadowLight->lightId);
        const bool newAllocation = entry == nullptr;
        if (newAllocation) {
            // 포인트 라이트의 6면은 동일한 해상도로 함께 할당하며, 부족하면 allocator 가 해상도를 절반씩 낮춘다.
            entry = shadowAtlas.Alloc(shadowLight->lightId, requestedResolution, shadowViewCount);
            if (!entry) {
                frameShadowAtlas->numAllocationFailures++;
                continue;
            }
        }

        // 라이트의 shadow 와 shadowViews 를 할당한다.
        FrameLocalShadow *frameShadow = frame->AllocFrameLocalShadow(frameShadowAtlas, shadowLight);
        if (!frameShadow) {
            // 프레임 데이터 생성을 실패했다면 아틀라스에서도 제거한다.
            if (newAllocation) {
                const bool freed = shadowAtlas.Free(shadowLight->lightId);
                assert(freed);
                (void)freed;
            }
            break;
        }

        // 백엔드의 depth 기록 결과를 관리하는 상태를 entry 와 공유한다.
        frameShadow->renderState = entry->renderState;

        frameShadow->requestedResolution = shadowLight->shadowRequestedResolution;
        frameShadow->resolution = entry->tiles[0].resolution;
        if (frameShadow->resolution < requestedResolution) {
            frameShadowAtlas->numResolutionDowngrades++;
        }
        assert(frameShadow->numShadowViews == shadowViewCount);

        const bool staticShadows = BE1::HasFlag(shadowLight->flags, RenderLight::Flag::StaticShadows);
        const uint32_t borderTexels = shadowMapQuality < 3 ? 2 : CalcShadowMapBorderTexels(frameShadow->resolution, filterRadiusScale);

        // 라이트 설정/플래그, caster 레이어 또는 투영 border 가 바뀌면 이전 depth 를 재사용할 수 없다.
        // 신규/재할당 타일과 이전 기록 실패도 백엔드의 유효성 확인을 통과하지 못한다.
        frameShadow->hasValidDepth = entry->shadowRevision == shadowLight->shadowRevision && entry->casterLayerMask == casterLayerMask && entry->borderTexels == borderTexels && entry->renderState->HasValidDepth();

        // static shadows 는 유효한 depth 를 계속 유지한다. 무효화된 그림자는 load balancing 과 무관하게 즉시 갱신한다.
        frameShadow->updateDepth = !staticShadows || !frameShadow->hasValidDepth;
        if (!staticShadows && frameSettings.shadowAtlasLoadBalancing && frameShadow->hasValidDepth) {
            // 투영 크기가 기준 크기의 절반이 될 때마다 갱신 간격을 2배로 늘린다.
            uint32_t updateInterval = 1;
            uint32_t projectedSize = shadowLight->projectedSize;

            while (projectedSize < frameSettings.shadowMapUpdateMinProjectedSize && updateInterval < RenderFrameSettings::ShadowAtlasMaxUpdateInterval) {
                projectedSize <<= 1;
                updateInterval <<= 1;
            }

            // Light ID 로 갱신 프레임을 분산하며, 포인트 라이트 그림자의 6면은 함께 갱신한다.
            const uint32_t frameMask = updateInterval - 1;
            frameShadow->updateDepth = (frameCount & frameMask) == (shadowLight->lightId & frameMask);
        }

        // 갱신할 때만 실제 해상도와 현재 품질에 맞는 PCF/bilinear footprint 용 border 를 적용한다.
        if (frameShadow->updateDepth) {
            entry->shadowRevision = shadowLight->shadowRevision;
            entry->casterLayerMask = casterLayerMask;
            entry->borderTexels = borderTexels;
            frameShadow->hasValidDepth = false; // 갱신 결과는 백엔드에서 depth 기록 후 확정한다.
        }

        FrameLocalShadowView *shadowViews = &frameShadowAtlas->shadowViews[frameShadow->firstShadowView];
        for (uint32_t viewIndex = 0; viewIndex < shadowViewCount; ++viewIndex) {
            const ShadowAtlas::Tile &tile = entry->tiles[viewIndex];
            assert(tile.resolution == frameShadow->resolution);
            shadowViews[viewIndex].tileRect = BE1::Rect(tile.x, tile.y, tile.resolution, tile.resolution);
            shadowViews[viewIndex].borderTexels = entry->borderTexels;
        }

        if (frameShadow->updateDepth) {
            // 갱신할 그림자의 컬링 공통 계산은 카메라마다 한 번 준비한다.
            if (shadowLight->type == RenderLight::Type::Point) {
                const float lightRadius = shadowLight->range;
                frameShadow->squaredRadius = lightRadius * lightRadius;

                entry->pointOrigin = shadowLight->origin;
                entry->pointDepthScaleBias = shadowLight->CalcPointShadowProjections(shadowViews);
            } else {
                shadowLight->CalcSpotShadowProjection(shadowViews[0]);
                entry->spotViewProjMatrix = shadowViews[0].viewProjMatrix;
            }

            if (!staticShadows) {
                for (int planeIndex = 0; planeIndex < 6; ++planeIndex) {
                    frameShadow->viewPlaneToOriginDistances[planeIndex] = viewPlanes[planeIndex].Distance(shadowLight->origin);
                }
            }
        }

        // 스킵 프레임도 기존 depth 와 같은 좌표계로 샘플링한다. 컬링용 frustum/평면은 갱신할 때만 필요하다.
        if (shadowLight->type == RenderLight::Type::Point) {
            frameShadow->pointOrigin = entry->pointOrigin;
            frameShadow->pointDepthScaleBias = entry->pointDepthScaleBias;
        } else if (!frameShadow->updateDepth) {
            shadowViews[0].viewProjMatrix = entry->spotViewProjMatrix;
        }
    }
}

void RenderWorld::BuildDebugDraw(RenderFrame *frame, FrameCamera *frameCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::BuildDebugDraw");

    if (BE1::HasFlag(frameCamera->flags, RenderCamera::Flag::SkipDebugDraw)) {
        return;
    }

    frameCamera->showDebugLights = frame->GetRenderFrameSettings().showDebugLights && frameCamera->numFrameLocalLights > 0;

    // 현재 debug primitive 저장소의 프레임 snapshot 을 카메라에 연결한다.
    frameCamera->debugDrawData = BuildDebugDrawData(frame);
    PrepareDebugDrawPipelines(frameCamera);
}

const DebugDrawData *RenderWorld::BuildDebugDrawData(RenderFrame *frame) {
    static_assert(static_cast<uint32_t>(DebugDrawPipelineId::Count) == DebugDrawData::MaxBatches);
    if (debugPrimitiveVertexCount == 0) {
        assert(debugPrimitiveCount == 0);
        return nullptr;
    }

    RHI::VertexBuffer *vertexBuffer = frame->GetThreadData(0)->AllocVertex(sizeof(DebugDrawVertex), debugPrimitiveVertexCount);
    if (!vertexBuffer || !vertexBuffer->mappedPtr) {
        BE_WARNLOG("RenderWorld::BuildDebugDrawData: failed to allocate debug primitive vertex buffer\n");
        return nullptr;
    }

    DebugDrawData *newDebugDrawData = new (frame->MemAlloc(sizeof(DebugDrawData))) DebugDrawData();
    newDebugDrawData->vertexBuffer = vertexBuffer;
    newDebugDrawData->numVerts = debugPrimitiveVertexCount;

    DebugDrawVertex *dstVerts = static_cast<DebugDrawVertex *>(vertexBuffer->mappedPtr);
    uint32_t firstVertex = 0;

    // 각 bucket 으로 batch 를 만든다.
    for (uint32_t bucketIndex = 0; bucketIndex < DebugDrawData::MaxBatches; ++bucketIndex) {
        const DebugPrimitiveBucket &bucket = debugPrimitiveBuckets[bucketIndex];
        const uint32_t numBucketVerts = static_cast<uint32_t>(bucket.vertices.Count());
        if (numBucketVerts == 0) {
            continue;
        }

        BE1::simdProcessor->Memcpy(dstVerts + firstVertex, bucket.vertices.Ptr(), numBucketVerts * sizeof(DebugDrawVertex));

        DebugDrawBatch &batch = newDebugDrawData->batches[newDebugDrawData->numBatches++];
        batch.pipelineId = static_cast<DebugDrawPipelineId>(bucketIndex);
        batch.firstVertex = firstVertex;
        batch.numVerts = numBucketVerts;

        firstVertex += numBucketVerts;
    }

    assert(firstVertex == debugPrimitiveVertexCount);
    assert(newDebugDrawData->numBatches > 0);

    return newDebugDrawData;
}

void RenderWorld::PrepareDebugDrawPipelines(FrameCamera *frameCamera) {
    const DebugDrawData *drawData = frameCamera->debugDrawData;
    if (!drawData && !frameCamera->showDebugLights) {
        return;
    }

    struct DebugDrawPipelineState {
        RHI::PrimitiveTopology primitiveTopology;
        bool depthTest;
        bool twoSided;
    };

    static constexpr DebugDrawPipelineState pipelineStates[] = {
        { RHI::PrimitiveTopology::TriangleList, true,  false },
        { RHI::PrimitiveTopology::TriangleList, true,  true  },
        { RHI::PrimitiveTopology::LineList,     true,  false },
        { RHI::PrimitiveTopology::TriangleList, false, false },
        { RHI::PrimitiveTopology::TriangleList, false, true  },
        { RHI::PrimitiveTopology::LineList,     false, false }
    };
    static_assert(COUNT_OF(pipelineStates) == DebugDrawData::MaxBatches);

    // Debug primitives 별로 graphics pipeline 의 필요 여부를 requiredPipelines 에 표시한다.
    bool requiredPipelines[DebugDrawData::MaxBatches] = {};

    if (drawData) {
        for (uint32_t batchIndex = 0; batchIndex < drawData->numBatches; ++batchIndex) {
            const DebugDrawBatch &batch = drawData->batches[batchIndex];
            requiredPipelines[static_cast<int>(batch.pipelineId)] = true;
        }
    }

    if (frameCamera->showDebugLights) {
        requiredPipelines[static_cast<int>(DebugDrawPipelineId::DepthTestedLines)] = true;
    }

    GraphicsPipelineDesc graphicsPipelineDesc;
    graphicsPipelineDesc.inputLayout = &VertexInputLayoutFactory::Get(VertexInputLayoutId::XyzColor);
    graphicsPipelineDesc.blendStateType = RHI::BlendStateType::AlphaBlend;
    RenderContext::activeContext->GetMainRenderDest(graphicsPipelineDesc.renderDest, false, false, frameCamera->taaResolve);

    // 필요한 graphics pipeline 들을 준비한다.
    for (uint32_t pipelineIndex = 0; pipelineIndex < DebugDrawData::MaxBatches; ++pipelineIndex) {
        if (!requiredPipelines[pipelineIndex]) {
            continue;
        }

        const DebugDrawPipelineState &pipelineState = pipelineStates[pipelineIndex];
        const bool linePrimitive = pipelineState.primitiveTopology == RHI::PrimitiveTopology::LineList;

        graphicsPipelineDesc.primitiveTopology = pipelineState.primitiveTopology;
        graphicsPipelineDesc.rasterizerStateType = linePrimitive || pipelineState.twoSided ? RHI::RasterizerStateType::SolidTwoSided : RHI::RasterizerStateType::SolidFrontSided;
        if (!pipelineState.depthTest) {
            graphicsPipelineDesc.depthStencilStateType = RHI::DepthStencilStateType::Disabled;
        } else {
            graphicsPipelineDesc.depthStencilStateType = frameCamera->projectionPolicy.reverseZ ? RHI::DepthStencilStateType::GEqualNoWrite : RHI::DepthStencilStateType::LEqualNoWrite;
        }

        ShaderPipeline *shaderPipeline = shaderPipelineCache.GetGraphicsPipelinePtr(shaderTemplateManager.debugDrawShaderTemplate, {}, graphicsPipelineDesc);
        frameCamera->debugDrawPSOs[pipelineIndex] = ShaderPipeline::GetReadyPSO(shaderPipeline);
    }
}

void RenderWorld::RenderGUI(GuiMesh &guiMesh) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::RenderGUI");

    assert(BE1::Engine::IsInMainThread());
    assert(frameActive);

    if (guiMesh.NumSurfaces() == 0) {
        return;
    }

    RenderFrame *frame = RenderContext::activeContext->GetCurrentFrontendFrame();

    uint32_t screenWidth = RenderContext::activeContext->GetWidth();
    uint32_t screenHeight = RenderContext::activeContext->GetHeight();

    // guiMesh 를 기반으로 GUI FrameCamera 를 구성하여, 렌더링 커맨드에 DrawCamera 명령을 기록한다.
    DrawGUICamera(frame, screenWidth, screenHeight, guiMesh);
}

void RenderWorld::DrawGUICamera(RenderFrame *frame, uint32_t screenWidth, uint32_t screenHeight, GuiMesh &guiMesh) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::DrawGUICamera");

    FrameCamera *frameCamera = frame->AllocFrameCamera();
    if (!frameCamera) {
        return;
    }

    frameCamera->maxFrameSurfs = guiMesh.NumSurfaces();
    frameCamera->frameSurfs = (const FrameSurf **)frame->MemAlloc(frameCamera->maxFrameSurfs * sizeof(FrameSurf *));
    frameCamera->numRenderPasses = 0;
    frameCamera->renderPasses = (RenderPass *)frame->MemAlloc(FrameCamera::MaxRenderPasses * sizeof(RenderPass));
    frameCamera->origin = BE1::Vec3::origin;
    frameCamera->axis = BE1::Mat3::identity;
    frameCamera->outputRect = BE1::Rect(0, 0, screenWidth, screenHeight);
    frameCamera->renderWidth = screenWidth;
    frameCamera->renderHeight = screenHeight;
    frameCamera->clearMethod = RenderCamera::ClearMethod::NoClear;
    frameCamera->projectionPolicy = RHI::renderer->GetProjectionPolicy();
    frameCamera->flags = RenderCamera::Flag::SkipPostProcess;
    //frameCamera->time = BE1::PlatformTime::Milliseconds();
    //frameCamera->orthogonal = true;
    //frameCamera->zNear = 0.0f;
    //frameCamera->zFar = 1.0f;
    frameCamera->viewMatrix = BE1::Mat4::identity;
    frameCamera->viewProjMatrix.SetOrthoRH(0, screenWidth, screenHeight, 0, -1.0, 1.0, false, frameCamera->projectionPolicy.reverseZ); // Y 를 뒤집고 NDC Z 는 [0, 1] 을 사용한다.
    frameCamera->invViewProjMatrix = frameCamera->viewProjMatrix.Inverse();
    frameCamera->projectionScaleBiasXY = BE1::Vec4(frameCamera->viewProjMatrix[0][0], frameCamera->viewProjMatrix[1][1], frameCamera->viewProjMatrix[0][3], frameCamera->viewProjMatrix[1][3]);
    frameCamera->projectionScaleBiasZ = BE1::Vec2(frameCamera->viewProjMatrix[2][2], frameCamera->viewProjMatrix[2][3]);
    frameCamera->projectionOrthogonal = true;
    frameCamera->projectionInfiniteZFar = false;

    frameCamera->prevViewProjMatrix = frameCamera->viewProjMatrix;
    frameCamera->prevInvViewProjMatrix = frameCamera->invViewProjMatrix;
    frameCamera->ndcJitter = BE1::Vec2::zero;
    frameCamera->prevNdcJitter = BE1::Vec2::zero;
    frameCamera->hiZCullViewProjMatrix = frameCamera->viewProjMatrix;
    frameCamera->worldAABB.Clear();
    frameCamera->is2D = true;

    FrameObject *frameObject = frame->AllocFrameObject();
    if (!frameObject) {
        return;
    }

    frameObject->worldMatrix.SetIdentity();
    frameObject->prevWorldMatrix.SetIdentity();

    guiMesh.CacheIndexes(frame->GetThreadData(0));

    const int numSurfaces = guiMesh.NumSurfaces();
    for (int surfaceIndex = 0; surfaceIndex < numSurfaces; surfaceIndex++) {
        const GuiMesh::Surface *guiSurf = guiMesh.GetSurface(surfaceIndex);
        if (!guiSurf->numIndexes) {
            break;
        }

        SubMesh *subMesh = (SubMesh *)frame->MemAlloc(sizeof(SubMesh));
        new (subMesh) SubMesh();

        subMesh->numIndexes = guiSurf->numIndexes;
        subMesh->numVerts = guiSurf->numVerts;
        subMesh->vertexBuffer = guiSurf->vertexBuffer;
        subMesh->indexBuffer = guiSurf->indexBuffer;

        AddFrameSurf(frame, frameCamera, frameObject, guiSurf->material, subMesh, FrameSurf::Flag::None, 0);
    }

    guiMesh.Clear();

    renderPassBuilder.BuildGuiPass(frame, frameCamera);

    frame->CmdDrawCamera(frameCamera);

    viewCount++;
}
