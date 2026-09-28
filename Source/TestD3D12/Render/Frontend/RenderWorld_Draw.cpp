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
#include "Render/RenderContext.h"
#include "Render/RenderSystem.h"
#include "RenderCamera.h"
#include "Render/RenderInternal.h"
#include "Render/RenderUtils.h"
#include "Render/Animation/Skeleton.h"
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
static_assert(static_cast<uint32_t>(RenderLight::Type::BoxDir) == ShaderLightTypeBoxDir);

static BE_INLINE bool IsRenderLayerVisible(uint32_t layerMask, uint8_t layer) {
    return layer < 32 && (layerMask & BIT(layer)) != 0;
}

static uint32_t CalcShadowMapBorderTexels(uint32_t resolution, uint32_t shadowMapQuality) {
    constexpr uint32_t fixedKernelMaxOffsetTexels = 1;
    constexpr uint32_t fixedKernelBorderTexels = fixedKernelMaxOffsetTexels + 1; // bilinear comparison footprint 를 위해 1 texel border 를 추가한다.
    assert(resolution > 2 * fixedKernelBorderTexels);

    // Q0~Q2 의 fixed kernel offset 의 최대 크기는 1 이다.
    if (shadowMapQuality < 3) {
        return fixedKernelBorderTexels;
    }

    constexpr float usableResolution = static_cast<float>(RenderFrameSettings::ShadowMapFilterReferenceResolution - 2 * RenderFrameSettings::ShadowMapFilterReferenceBorderTexels);
    constexpr float maxFilterRadiusScale = RenderFrameSettings::ShadowMapFilterRadiusMax / usableResolution;
    constexpr float maxPoissonKernelRadiusScale = RenderFrameSettings::ShadowMapPoissonKernelMaxRadius * maxFilterRadiusScale;

    // effective radius 가 border 로 줄어드는 관계까지 풀어서 필요한 최소 border 를 계산한다.
    const float requiredBorder = (maxPoissonKernelRadiusScale * resolution + 1.0f) / (1.0f + 2.0f * maxPoissonKernelRadiusScale);

    return BE1::Max(fixedKernelBorderTexels, static_cast<uint32_t>(BE1::Math::Ceil(requiredBorder)));
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

void RenderWorld::AddVisSurf(RenderFrame *frame, VisCamera *visCamera, VisLight *visLight, VisObject *visObject, const Material *material, SubMesh *subMesh, VisSurfFlag flags, uint8_t shadowCascadeMask, int lodLevel) {
    if (visCamera->numVisSurfs + 1 > visCamera->maxVisSurfs) {
        BE_WARNLOG("RenderWorld::AddVisSurf: not enough available visible surfaces\n");
        return;
    }

    assert(material);
    assert(subMesh);
    assert(lodLevel >= 0 && lodLevel < subMesh->NumLODs());

    const RHI::IndexBuffer *indexBuffer = lodLevel == 0 ? subMesh->indexBuffer : (subMesh->lodIndexBuffers ? subMesh->lodIndexBuffers[lodLevel - 1] : nullptr);
    if (!subMesh->vertexBuffer || !indexBuffer || (subMesh->vertWeights && !subMesh->vertexWeightBuffer)) {
        indexBuffer = subMesh->UploadStaticDataToGPU(lodLevel);
    }

    if (!subMesh->vertexBuffer || !indexBuffer || (subMesh->vertWeights && !subMesh->vertexWeightBuffer)) {
        BE_WARNLOG("RenderWorld::AddVisSurf: failed to prepare static submesh buffers\n");
        return;
    }

    // 카메라 뷰 공간 z 깊이를 미리 계산한다 (불필요한 x, y 계산 생략)
#if 1
    const auto &viewRow2 = visCamera->viewMatrix[2];
    const float viewZ = viewRow2[0] * visObject->worldMatrix[0][3] + viewRow2[1] * visObject->worldMatrix[1][3] + viewRow2[2] * visObject->worldMatrix[2][3] + viewRow2[3];
#else
    const BE1::Vec3 localCenter = subMesh->GetBoundingBox().Center();
    const BE1::Vec3 worldCenter = visObject->worldMatrix.TransformPos(localCenter);
    const auto &viewRow2 = visCamera->viewMatrix[2];
    const flaot viewZ = viewRow2[0] * worldCenter.x + viewRow2[1] * worldCenter.y + viewRow2[2] * worldCenter.z + viewRow2[3];
#endif

    const VisSkinnedMesh *visSkinnedMesh = visObject->skinnedMesh;

    VisSurf *drawSurf = (VisSurf *)frame->MemAlloc(sizeof(VisSurf));
    drawSurf->index = visCamera->numVisSurfs;
    drawSurf->space = visObject;
    drawSurf->subMesh = subMesh;
    drawSurf->indexBuffer = indexBuffer;
    drawSurf->numIndexes = subMesh->NumIndexes(lodLevel);
    drawSurf->lodLevel = lodLevel;
    drawSurf->viewZ = viewZ;
    drawSurf->material = material;
    drawSurf->baseFeatureSet = BuildBaseFeatureSet(material, subMesh, visSkinnedMesh && visSkinnedMesh->skinningJointOffset != ShaderSkinningJointOffsetInvalid);
    drawSurf->shaderMaterialIndex = frame->RegisterShaderMaterial(material);
    drawSurf->flags = flags;
    drawSurf->shadowCascadeMask = shadowCascadeMask;

    visCamera->visSurfs[visCamera->numVisSurfs++] = drawSurf;
}

void RenderWorld::SetSkyboxMaterial(SharedPtr<Material> skyboxMaterial) {
    assert(!frameActive);

    if (this->skyboxMaterial != skyboxMaterial && hasEnvironmentLight && !envLight.GetDesc().radianceCubeMap) {
        envLight.MarkRegenerationNeeded();
        renderSystem->GetProbeBakeSystem().CancelEnvironmentLightProbeBake(this);
    }

    this->skyboxMaterial = skyboxMaterial;
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

void RenderWorld::SetupPyramidBloom(RenderFrame *frame, RenderCamera *renderCamera, VisCamera *visCamera) {
    BloomResources &resources = renderCamera->GetBloomResources();
    if (resources.GetBloomMipCount() < 2) {
        return;
    }

    visCamera->bloom.bloomDownTexture = resources.GetBloomDownTexture();
    visCamera->bloom.bloomUpTexture = resources.GetBloomUpTexture();
    visCamera->bloom.bloomMipCount = resources.GetBloomMipCount();

    for (uint32_t mipLevel = 0; mipLevel < RenderBloomDownMaxLevelCount; ++mipLevel) {
        visCamera->bloom.bloomMipWidths[mipLevel] = resources.GetBloomMipWidths()[mipLevel];
        visCamera->bloom.bloomMipHeights[mipLevel] = resources.GetBloomMipHeights()[mipLevel];
        visCamera->bloom.bloomDownMipSRVs[mipLevel] = resources.GetBloomDownMipSRVs()[mipLevel];
        visCamera->bloom.bloomDownMipUAVs[mipLevel] = resources.GetBloomDownMipUAVs()[mipLevel];
    }
    for (uint32_t mipLevel = 0; mipLevel < RenderBloomUpMaxLevelCount; ++mipLevel) {
        visCamera->bloom.bloomUpMipSRVs[mipLevel] = resources.GetBloomUpMipSRVs()[mipLevel];
        visCamera->bloom.bloomUpMipUAVs[mipLevel] = resources.GetBloomUpMipUAVs()[mipLevel];
    }

    visCamera->bloom.outputTexture = resources.GetBloomUpTexture();
    visCamera->bloom.outputViewID = resources.GetBloomUpMipSRVs()[0];
}

void RenderWorld::SetupConvolutionBloom(RenderFrame *frame, RenderCamera *renderCamera, VisCamera *visCamera) {
    BloomResources &resources = renderCamera->GetBloomResources();
    if (resources.GetFFTBloomSize() == 0 ||
        !resources.GetFFTBloomConvolutionOutputTexture() ||
        resources.GetFFTBloomConvolutionOutputWidth() == 0 ||
        resources.GetFFTBloomConvolutionOutputHeight() == 0) {
        return;
    }

    const RenderFrameSettings &frameSettings = frame->GetRenderFrameSettings();
    resources.SetConvolutionBloomKernelRotation(frameSettings.convolutionBloomKernelRotation);
    resources.SetConvolutionBloomKernelScale(frameSettings.convolutionBloomKernelScale);
    resources.SetConvolutionBloomPaddingTrim(frameSettings.convolutionBloomPaddingTrim);

    const uint32_t fftBloomSize = resources.GetFFTBloomSize();
    const bool updateFFTBloomKernelBuffer = resources.IsFFTBloomKernelDirty();

    Texture *fftBloomKernelSourceTexture = nullptr;
    RHI::Texture *fftBloomKernelSourceRHITexture = nullptr;
    if (updateFFTBloomKernelBuffer) {
        fftBloomKernelSourceTexture = frame->RetainTexture(resources.GetFFTBloomKernelTexture());
        if (!fftBloomKernelSourceTexture || !fftBloomKernelSourceTexture->IsReadyForRendering()) {
            return;
        }
        fftBloomKernelSourceRHITexture = fftBloomKernelSourceTexture->GetRHITexture();

        resources.UpdateConvolutionBloomMapping();
    }

    visCamera->bloom.fftBloomSize = fftBloomSize;
    visCamera->bloom.fftBloomScreenSpatialBuffer = resources.GetFFTBloomScreenSpatialBuffer();
    visCamera->bloom.fftBloomScreenSpectrumBuffer = resources.GetFFTBloomScreenSpectrumBuffer();
    visCamera->bloom.fftBloomKernelSourceTexture = updateFFTBloomKernelBuffer ? fftBloomKernelSourceRHITexture : nullptr;
    visCamera->bloom.fftBloomKernelSpatialBuffer = resources.GetFFTBloomKernelSpatialBuffer();
    visCamera->bloom.fftBloomKernelSpectrumBuffer = resources.GetFFTBloomKernelSpectrumBuffer();
    visCamera->bloom.fftBloomWorkBuffer = resources.GetFFTBloomWorkBuffer();
    visCamera->bloom.fftBloomConvolutionOutputTexture = resources.GetFFTBloomConvolutionOutputTexture();
    visCamera->bloom.fftBloomConvolutionOutputWidth = resources.GetFFTBloomConvolutionOutputWidth();
    visCamera->bloom.fftBloomConvolutionOutputHeight = resources.GetFFTBloomConvolutionOutputHeight();
    visCamera->bloom.fftBloomConvolutionOutputMipCount = resources.GetFFTBloomConvolutionOutputMipCount();
    for (uint32_t mipLevel = 0; mipLevel < RenderConvolutionBloomMaxMipLevelCount; ++mipLevel) {
        visCamera->bloom.fftBloomConvolutionOutputMipSRVs[mipLevel] = resources.GetFFTBloomConvolutionOutputMipSRVs()[mipLevel];
        visCamera->bloom.fftBloomConvolutionOutputMipUAVs[mipLevel] = resources.GetFFTBloomConvolutionOutputMipUAVs()[mipLevel];
    }
    visCamera->bloom.fftBloomKernelRotation = resources.GetFFTBloomKernelRotation();
    visCamera->bloom.fftBloomKernelScale = resources.GetFFTBloomKernelScale();
    visCamera->bloom.fftBloomPaddingTrim = resources.GetFFTBloomPaddingTrim();
    visCamera->bloom.fftBloomKernelNormalizedExtents = resources.GetFFTBloomKernelNormalizedExtents();
    visCamera->bloom.fftBloomMappedContentRect = resources.GetFFTBloomMappedContentRect();
    visCamera->bloom.fftBloomMappedKernelScale = resources.GetFFTBloomMappedKernelScale();
    visCamera->bloom.fftBloomKernelInverseAverageLuminance = resources.GetFFTBloomKernelInverseAverageLuminance();
    visCamera->bloom.fftBloomKernelDirty = updateFFTBloomKernelBuffer;

    if (updateFFTBloomKernelBuffer) {
        resources.MarkConvolutionBloomKernelClean();
    }

    visCamera->bloom.outputTexture = resources.GetFFTBloomConvolutionOutputTexture();
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

    const SharedPtr<ShaderPipeline> imagePipeline = shaderPipelineCache.GetGraphicsPipeline(shaderTemplateManager.fullscreenImageShaderTemplate, ShaderPermutationKey(), graphicsPipelineDesc, false);
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
    const SharedPtr<ShaderPipeline> downsamplePipeline = shaderPipelineCache.GetGraphicsPipeline(shaderTemplateManager.sceneSnapshotDownsampleShaderTemplate, ShaderPermutationKey(), graphicsPipelineDesc, false);
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

    // VisLight 는 축당 8 비트 타일 좌표를 저장하므로 최대 256 개 타일까지 지원한다. 즉, MaxRenderDimension x MaxRenderDimension 보다 큰 해상도는 지원하지 않는다.
    if (renderWidth > VisLight::MaxRenderDimension || renderHeight > VisLight::MaxRenderDimension) {
        BE_WARNLOG("RenderWorld::RenderScene: requested render size %ux%u exceeds the per-axis limit of %u\n", renderWidth, renderHeight, VisLight::MaxRenderDimension);
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
    const bool useTAA = temporalHistoryEnabled && frameSettings.taa && !skipPostProcess && !debugAO && frameSettings.shadowDebugMode != ShaderShadowDebugModeVisibility;
    const bool needNormalTexture = useAO || frameSettings.ssr;
    const bool needVelocityTexture = useTAA;
    const bool needColorHistory = useTAA;
    const bool needDepthHistory = useTAA;
    const bool needHiZCullResources = !BE1::HasFlag(cameraDesc.flags, RenderCamera::Flag::DisableHiZCulling) && frameSettings.hiZOcclusionCulling;
    const bool needHiZTexture = needHiZCullResources;
    const bool needHiZDebugPreview = needHiZTexture && frameSettings.hiZDebugPreview;

    const RenderBloomMode bloomMode = frameSettings.bloomMode;

    // 장면 렌더링에 필요한 컬러, 깊이, 노멀, velocity 텍스처를 준비한다.
    if (!renderCamera->EnsureRenderTextures(renderWidth, renderHeight, renderContext->GetMainRTColorFormat(), renderContext->GetMainRTDepthFormat(), renderContext->GetMainRTSampleCount(), needDepthHistory, needNormalTexture, needVelocityTexture, renderContext->GetBaseClearColor())) {
        return;
    }

    // 이번 프레임에 사용할 카메라 슬롯을 확보한다.
    VisCamera *visCamera = frame->AllocVisCamera();
    if (!visCamera) {
        return;
    }

    // SSR 에 사용할 화면 공간 깊이 피라미드 리소스를 준비한다.
    ScreenSpaceDepthPyramidResources &depthPyramidResources = renderCamera->GetScreenSpaceDepthPyramidResources();
    depthPyramidResources.Ensure(renderWidth, renderHeight, frameSettings.ssr);

    // SSR 추적 결과를 저장할 반해상도 텍스처를 준비한다.
    renderCamera->EnsureSSRTraceResources(renderWidth, renderHeight, frameSettings.ssr);

    // MSAA 깊이를 resolve 할 AO/SSR 공용 텍스처를 준비한다.
    renderCamera->EnsureResolvedDepthTexture(renderWidth, renderHeight, renderContext->GetMainRTSampleCount(), useAO || frameSettings.ssr);

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

    const bool canUseHiZ = renderCamera->GetDepthTexture() && hiZResources.GetTexture();
    const bool requestHiZOcclusionCulling = canUseHiZ && needHiZCullResources && hiZResources.GetVisibleInstanceBuffer() && hiZResources.GetIndirectArgsBuffer();
    const bool requestHiZDebugPreview = canUseHiZ && needHiZDebugPreview && hiZResources.GetDebugPreviewTexture();
    const bool requestHiZGeneration = requestHiZOcclusionCulling;

    const bool hasSSRInputs = renderCamera->GetNormalTexture() && (!useMSAA || (renderCamera->GetResolvedDepthTexture() && renderCamera->GetResolvedNormalTexture()));
    const bool requestScreenSpaceDepthPyramidGeneration = frameSettings.ssr && hasSSRInputs && renderCamera->GetDepthTexture() && depthPyramidResources.GetTexture();
    const bool requestSSRTrace = requestScreenSpaceDepthPyramidGeneration && renderCamera->GetSSRTraceTexture();

    visCamera->maxVisSurfs = VisCamera::MaxVisSurfs;
    visCamera->visSurfs = (const VisSurf **)frame->MemAlloc(visCamera->maxVisSurfs * sizeof(VisSurf *));

    visCamera->numRenderPasses = 0;
    visCamera->renderPasses = (RenderPass *)frame->MemAlloc(VisCamera::MaxRenderPasses * sizeof(RenderPass));

    visCamera->origin = cameraDesc.origin;
    visCamera->axis = cameraDesc.axis;

    if (hasEnvironmentLight) {
        const EnvironmentLight::Desc &envLightDesc = envLight.GetDesc();
        visCamera->envLightAxis = envLightDesc.axis;
        visCamera->envLightColor = envLightDesc.linearColor;
        visCamera->envLightIntensity = envLightDesc.intensity;

        Texture *diffuseIrradianceCubeMap = frame->RetainTexture(envLight.GetDiffuseIrradianceCubeMap());
        RHI::Texture *rhiDiffuseIrradianceCubeMap = diffuseIrradianceCubeMap ? diffuseIrradianceCubeMap->GetReadyRHITexture() : nullptr;
        if (rhiDiffuseIrradianceCubeMap && rhiDiffuseIrradianceCubeMap->type == RHI::TextureType::TextureCube) {
            visCamera->envLightDiffuseIrradianceCubeMap = rhiDiffuseIrradianceCubeMap;
        }

        Texture *specularCubeMap = frame->RetainTexture(envLight.GetSpecularPrefilteredCubeMap());
        RHI::Texture *rhiSpecularCubeMap = specularCubeMap ? specularCubeMap->GetReadyRHITexture() : nullptr;
        if (rhiSpecularCubeMap && rhiSpecularCubeMap->type == RHI::TextureType::TextureCube) {
            visCamera->envLightSpecularCubeMap = rhiSpecularCubeMap;
            visCamera->envLightSpecularMipCount = rhiSpecularCubeMap->GetMipLevelCount();
        }
    }

    visCamera->outputRect = outputRect;
    visCamera->outputTexture = cameraDesc.outputTexture;
    visCamera->outputViewID = cameraDesc.outputViewID;
    visCamera->outputBeforeState = cameraDesc.outputBeforeState;
    visCamera->outputAfterState = cameraDesc.outputAfterState;
    visCamera->outputBlitPSO = outputBlitPSO;
    visCamera->renderWidth = renderWidth;
    visCamera->renderHeight = renderHeight;
    visCamera->clearMethod = cameraDesc.clearMethod;
    visCamera->clearColorMode = cameraDesc.clearColorMode;
    visCamera->clearColor = cameraDesc.clearColor;
    visCamera->projectionPolicy = cameraDesc.projectionPolicy;
    visCamera->projectionZ = BE1::Vec2(renderCamera->projMatrix[2][2], renderCamera->projMatrix[2][3]);
    visCamera->projectionOrthogonal = cameraDesc.orthogonal;
    visCamera->projectionInfiniteZFar = cameraDesc.infiniteZFar;

    if (visCamera->projectionOrthogonal) {
        visCamera->projectionXY = BE1::Vec4(renderCamera->projMatrix[0][0], renderCamera->projMatrix[1][1], renderCamera->projMatrix[0][3], renderCamera->projMatrix[1][3]);
        visCamera->viewBox = renderCamera->GetBox();
    } else {
        visCamera->projectionXY = BE1::Vec4(renderCamera->projMatrix[0][0], renderCamera->projMatrix[1][1], -renderCamera->projMatrix[0][2], -renderCamera->projMatrix[1][2]);
        visCamera->viewFrustum = renderCamera->GetFrustum();
    }

    if (visCamera->projectionPolicy.IsZNegativeOneToOne()) {
        visCamera->ndcDepthScale = 2.0f;
        visCamera->ndcDepthBias = -1.0f;
    } else {
        visCamera->ndcDepthScale = 1.0f;
        visCamera->ndcDepthBias = 0.0f;
    }

    visCamera->flags = cameraDesc.flags;
    visCamera->needNormalOutput = frameSettings.ssr;
    visCamera->needVelocityOutput = useTAA;

    visCamera->colorTexture = renderCamera->GetColorTexture();
    visCamera->depthTexture = renderCamera->GetDepthTexture();
    visCamera->normalTexture = renderCamera->GetNormalTexture();
    visCamera->velocityTexture = renderCamera->GetVelocityTexture();

    visCamera->resolvedColorTexture = renderCamera->GetResolvedColorTexture();
    visCamera->resolvedDepthTexture = renderCamera->GetResolvedDepthTexture();
    visCamera->resolvedNormalTexture = renderCamera->GetResolvedNormalTexture();
    visCamera->resolvedVelocityTexture = renderCamera->GetResolvedVelocityTexture();

    visCamera->sceneSnapshotDownsamplePSO = useHalfResSnapshot ? sceneSnapshotDownsamplePSO : nullptr;
    visCamera->snapshotColorTexture = renderCamera->GetSnapshotColorTexture();
    visCamera->snapshotDepthTexture = renderCamera->GetSnapshotDepthTexture();

    ScreenSpaceAOFrameData &screenSpaceAO = visCamera->ao;
    screenSpaceAO.textureSet = renderCamera->screenSpaceAOResources.GetTextureSet();
    screenSpaceAO.depthTexture = useMSAA ? visCamera->resolvedDepthTexture : visCamera->depthTexture;
    screenSpaceAO.normalTexture = useMSAA ? visCamera->resolvedNormalTexture : visCamera->normalTexture;
    screenSpaceAO.projectionPolicy = visCamera->projectionPolicy;
    screenSpaceAO.mode = frameSettings.aoMode;
    screenSpaceAO.temporalNoise = frameSettings.aoTemporalNoise && !debugAO;

    visCamera->applyAO = useAO && ScreenSpaceAO::HasRequiredResources(screenSpaceAO) && (!useMSAA || visCamera->normalTexture);
    visCamera->needDepthNormalOutput = visCamera->applyAO;

    visCamera->generateScreenSpaceDepthPyramid = requestScreenSpaceDepthPyramidGeneration;

    if (visCamera->generateScreenSpaceDepthPyramid) {
        ScreenSpaceDepthPyramidFrameData &depthPyramid = visCamera->screenSpaceDepthPyramid;
        depthPyramid.texture = depthPyramidResources.GetTexture();
        depthPyramid.mipCount = depthPyramidResources.GetMipCount();
        depthPyramid.validWidth = depthPyramidResources.GetValidWidth();
        depthPyramid.validHeight = depthPyramidResources.GetValidHeight();

        for (uint32_t mipLevel = 0; mipLevel < RenderScreenSpaceDepthPyramidMaxLevelCount; ++mipLevel) {
            depthPyramid.mipSRVs[mipLevel] = depthPyramidResources.GetMipSRVs()[mipLevel];
            depthPyramid.mipUAVs[mipLevel] = depthPyramidResources.GetMipUAVs()[mipLevel];
            depthPyramid.mipWidths[mipLevel] = depthPyramidResources.GetMipWidths()[mipLevel];
            depthPyramid.mipHeights[mipLevel] = depthPyramidResources.GetMipHeights()[mipLevel];
        }
    }

    visCamera->traceSSR = requestSSRTrace;
    visCamera->ssrTraceTexture = renderCamera->GetSSRTraceTexture();

    visCamera->generateHiZ = requestHiZGeneration;
    if (visCamera->generateHiZ) {
        visCamera->hiZ.texture = hiZResources.GetTexture();
        visCamera->hiZ.mipCount = hiZResources.GetMipCount();

        for (uint32_t mipLevel = 0; mipLevel < RenderHiZMaxLevelCount; ++mipLevel) {
            visCamera->hiZ.mipSRVs[mipLevel] = hiZResources.GetMipSRVs()[mipLevel];
            visCamera->hiZ.mipUAVs[mipLevel] = hiZResources.GetMipUAVs()[mipLevel];
            visCamera->hiZ.mipWidths[mipLevel] = hiZResources.GetMipWidths()[mipLevel];
            visCamera->hiZ.mipHeights[mipLevel] = hiZResources.GetMipHeights()[mipLevel];
        }

        visCamera->hiZOcclusionCulling = hiZResources.IsHistoryValid() && requestHiZOcclusionCulling;
        if (visCamera->hiZOcclusionCulling) {
            visCamera->hiZ.visibleInstanceBuffer = hiZResources.GetVisibleInstanceBuffer();
            visCamera->hiZ.indirectArgsBuffer = hiZResources.GetIndirectArgsBuffer();
        }

        visCamera->hiZDebugPreview = requestHiZDebugPreview;
        if (visCamera->hiZDebugPreview) {
            visCamera->hiZ.debugPreviewTexture = hiZResources.GetDebugPreviewTexture();
            visCamera->hiZ.debugLinearDepthScale = cameraDesc.zFar > 0.0f ? 1.0f / cameraDesc.zFar : 1.0f;
        }
    }

    visCamera->postProcessColorTexture = renderCamera->GetPostProcessColorTexture();
    visCamera->colorHistoryTextures[0] = renderCamera->GetColorHistoryTexture(0);
    visCamera->colorHistoryTextures[1] = renderCamera->GetColorHistoryTexture(1);
    visCamera->depthHistoryTextures[0] = renderCamera->GetDepthHistoryTexture(0);
    visCamera->depthHistoryTextures[1] = renderCamera->GetDepthHistoryTexture(1);
    visCamera->currentHistoryTextureIndex = renderCamera->GetCurrentHistoryTextureIndex();
    visCamera->historyValid = renderCamera->IsHistoryValid();

    visCamera->luminanceBuffer = renderCamera->GetLuminanceBuffer();
    visCamera->bloom.renderWidth = renderWidth;
    visCamera->bloom.renderHeight = renderHeight;

    if (!skipPostProcess) {
        // 초기 LUT upload request 는 backend frame 실행 전에 처리되므로 Pending 상태의 RHI 포인터도 프레임에 전달할 수 있다.
        Texture *colorGradingLUTTexture = frame->RetainTexture(renderSystem->GetColorGradingLUTTexture());
        visCamera->colorGradingLUTTexture = colorGradingLUTTexture ? colorGradingLUTTexture->GetRHITexture() : nullptr;

        Texture *bloomDirtMaskTexture = frame->RetainTexture(renderCamera->GetBloomResources().GetBloomDirtMaskTexture());
        visCamera->bloom.bloomDirtMaskTexture = bloomDirtMaskTexture ? bloomDirtMaskTexture->GetRHITexture() : nullptr;

        if (bloomMode == RenderBloomMode::Pyramid) {
            SetupPyramidBloom(frame, renderCamera, visCamera);
        } else if (bloomMode == RenderBloomMode::Convolution) {
            SetupConvolutionBloom(frame, renderCamera, visCamera);
        }
    }

    visCamera->worldAABB.Clear();
    visCamera->is2D = false;
    visCamera->viewMatrix = renderCamera->viewMatrix;

    const BE1::Vec2 jitterToNDCScale(2.0f / static_cast<float>(renderWidth), -2.0f / static_cast<float>(renderHeight));

    // Rasterization 은 jittered 행렬을 사용하고, velocity 계산용 unjittered 행렬은 backend 에서 jitter 를 제거해 복원한다.
    visCamera->viewProjMatrix = renderCamera->jitteredViewProjMatrix;
    visCamera->invViewProjMatrix = renderCamera->jitteredInvViewProjMatrix;
    visCamera->jitterNDC = renderCamera->currentJitter * jitterToNDCScale;

    const bool usePreviousMatrix = temporalHistoryEnabled && renderCamera->previousMatrixValid && !renderCamera->temporalHistoryInvalidated;

    if (usePreviousMatrix) {
        visCamera->prevViewProjMatrix = renderCamera->previousJitteredViewProjMatrix;
        visCamera->prevInvViewProjMatrix = renderCamera->previousJitteredInvViewProjMatrix;
        visCamera->prevJitterNDC = renderCamera->previousJitter * jitterToNDCScale;
    } else {
        visCamera->prevViewProjMatrix = renderCamera->jitteredViewProjMatrix;
        visCamera->prevInvViewProjMatrix = renderCamera->jitteredInvViewProjMatrix;
        visCamera->prevJitterNDC = visCamera->jitterNDC;
    }

    visCamera->taaResolve = useTAA;

    // freeze 가 되면 prevViewProjMatrix 를 유지하기 위해서 hiZCullViewProjMatrix 에 복사한다.
    if (!frameSettings.hiZOcclusionCullingFreeze || !hiZResources.IsHistoryValid()) {
        renderCamera->hiZCullViewProjMatrix = visCamera->prevViewProjMatrix;
    }
    visCamera->hiZCullViewProjMatrix = renderCamera->hiZCullViewProjMatrix;

    DrawCamera(frame, renderCamera, visCamera);

    if (temporalHistoryEnabled) {
        renderCamera->previousViewProjMatrix = renderCamera->viewProjMatrix;
        renderCamera->previousInvViewProjMatrix = renderCamera->invViewProjMatrix;
        renderCamera->previousJitteredViewProjMatrix = renderCamera->jitteredViewProjMatrix;
        renderCamera->previousJitteredInvViewProjMatrix = renderCamera->jitteredInvViewProjMatrix;
        renderCamera->previousMatrixValid = true;
        renderCamera->temporalHistoryInvalidated = false;

        if (visCamera->taaResolve) {
            renderCamera->historyValid = true;
            renderCamera->currentHistoryTextureIndex ^= 1;
        } else {
            renderCamera->historyValid = false;
        }
    }

    if (!frameSettings.hiZOcclusionCullingFreeze) {
        hiZResources.SetHistoryValid(visCamera->generateHiZ);
    }
}

void RenderWorld::DrawCamera(RenderFrame *frame, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::DrawCamera");

    // RenderObject 중에서 컬링한 오브젝트를 프레임 데이터에 VisObject 로 등록한다.
    FindVisObjects(frame, renderCamera, visCamera);

    // RenderLight 중에서 컬링한 라이트를 프레임 데이터에 VisLight 로 등록한다.
    FindVisLights(frame, renderCamera, visCamera);

    // 광원별 요청/할당은 이후 이 카메라 전용 snapshot 에 등록한다.
    visCamera->localShadowAtlas = frame->AllocVisLocalShadowAtlas(RenderContext::activeContext->GetShadowAtlas());

    // 각 CSM split 의 receiver 영역으로 directional shadow 투영을 준비하고 caster 를 수집한다.
    SetupCascadedShadow(frame, renderCamera, visCamera);

    // visObjects 중에서 mesh surface 들을 visCamera->visSurfs 에 추가한다.
    AddMeshes(frame, renderCamera, visCamera);

    // visibleLights 들로 GPU light culling 데이터를 구성한다.
    BuildLightCullingData(frame, renderCamera, visCamera);

    // visObjects 중에서 text mesh 를 visCamera->visSurfs 에 추가한다.
    AddTextMeshes(frame, renderCamera, visCamera);

    // Add drawing surface of skybox.
    AddSkyBoxMeshes(frame, renderCamera, visCamera);

    // Ocean surface 와 query 정보를 프레임 데이터에 등록한다.
    if (hasOceanSurface) {
        frame->RegisterVisOceanSurface(oceanSurface.desc.simulation, oceanSurface.desc.surface, oceanSurface.revision, oceanSurface.queries.Ptr(), static_cast<uint32_t>(oceanSurface.queries.Count()));
    }

    // CSM 렌더 패스를 구성한다.
    if (visCamera->cascadedShadow) {
        const bool allowAsyncPSOBuild = !BE1::HasFlag(visCamera->flags, RenderCamera::Flag::ForceSyncPassPSOBuild);
        for (uint32_t cascadeIndex = 0; cascadeIndex < visCamera->cascadedShadow->cascadeCount; cascadeIndex++) {
            renderPassBuilder.BuildCascadedShadow(frame, visCamera->cascadedShadow, cascadeIndex, visCamera->visSurfs, visCamera->numVisSurfs, allowAsyncPSOBuild);
        }
    }

    // 메인 렌더 패스를 구성한다.
    renderPassBuilder.BuildMain(frame, visCamera);

    // Post-Opaque 단계에서 그릴 ocean surface pipeline 을 준비한다.
    PrepareOceanSurfacePipeline(visCamera);

    // 카메라의 debug draw 데이터를 구성한다.
    BuildDebugDraw(frame, visCamera);

    // 렌더링 커맨드에 visCamera 를 기록한다.
    frame->CmdDrawCamera(visCamera);

    viewCount++;
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
    const float depthMin = distForward - depthExtent;
    const float invDepthRange = 0.5f / depthExtent;

    // world-space sphere center 로 OBB 를 만든다. 광원 near 밖의 caster 는 depth clamp 로 보존한다.
    cascade.shadowMapBounds = BE1::OBB(shadowCenter, BE1::Vec3(depthExtent, shadowMapExtent, shadowMapExtent), lightAxis);

    // receiver sphere 만으로 view-projection matrix 를 완성한다.
    const float invExtent = 1.0f / shadowMapExtent;
    cascade.viewProjMatrix[0] = BE1::Vec4(lightAxis[EngineAxis::Left] * -invExtent, distLeft * invExtent);
    cascade.viewProjMatrix[1] = BE1::Vec4(lightAxis[EngineAxis::Up] * invExtent, -distUp * invExtent);
    cascade.viewProjMatrix[2] = BE1::Vec4(lightAxis[EngineAxis::Forward] * invDepthRange, -depthMin * invDepthRange);
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

void RenderWorld::SetupCascadedShadow(RenderFrame *frame, const RenderCamera *renderCamera, VisCamera *visCamera) {
    if (!visCamera->directionalLightVisible) {
        return;
    }

    const VisLight *directionalLight = frame->GetPrimaryDirectionalLight();
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
    const uint32_t shadowMapBorderTexels = CalcShadowMapBorderTexels(shadowMapResolution, frameSettings.shadowMapQuality);
    constexpr float referenceUsableResolution = static_cast<float>(RenderFrameSettings::ShadowMapFilterReferenceResolution - 2 * RenderFrameSettings::ShadowMapFilterReferenceBorderTexels);
    const float usableShadowResolution = static_cast<float>(shadowMapResolution - 2 * shadowMapBorderTexels);
    const float filterResolutionScale = usableShadowResolution / referenceUsableResolution;

    VisCascadedShadow *cascadedShadow = new (frame->MemAlloc(sizeof(VisCascadedShadow))) VisCascadedShadow();
    cascadedShadow->cascadeCount = frameSettings.csmCascadeCount;

    // cascade 별 distance 를 계산한다.
    ComputeCSMSplitDistances(cameraDesc.zNear, csmMaxDistance, frameSettings.csmSplitLambda, cascadedShadow->cascadeCount, cameraDesc.orthogonal, cascadedShadow->splitDistances);

    // 모든 cascade 의 caster shadow volume 검사에 사용할 전체 CSM 카메라 view planes 를 구한다.
    renderCamera->CalcSlicedViewVolume(cameraDesc.zNear, csmMaxDistance, nullptr, cascadedShadow->viewVolumePlanes);

    // 각 cascade 의 receiver 범위로 projection 과 caster 컬링 경계를 함께 정한다.
    for (uint32_t cascadeIndex = 0; cascadeIndex < cascadedShadow->cascadeCount; cascadeIndex++) {
        // blending 중인 cascade 는 이전 receiver 구간까지 포함하여 map coverage overlap 을 보장한다.
        const float cascadeNear = (cascadeIndex == 0 || frameSettings.csmBlendFraction <= 0.0f) ? cascadedShadow->splitDistances[cascadeIndex] : cascadedShadow->splitDistances[cascadeIndex - 1];
        const float cascadeFar = cascadedShadow->splitDistances[cascadeIndex + 1];

        BE1::Sphere cascadeSphere;
        renderCamera->CalcSlicedViewVolume(cascadeNear, cascadeFar, &cascadeSphere);

        CSMCascade &cascade = cascadedShadow->cascades[cascadeIndex];
        if (!CalcCSMCascadeProjection(cascade, directionalLight->axis, cascadeSphere, shadowMapResolution, shadowMapBorderTexels)) {
            return;
        }

        cascade.depthDSV = renderContext->GetCSMDepthSliceDSV(cascadeIndex);

        // worldToShadowCoordMatrix * worldPos = shadowCoord (UV.xy, depth.z).
        // near 밖 caster 는 depth clamp 로 처리하므로 caster 수집 후 projection 을 바꾸지 않는다.
        cascadedShadow->shaderData.worldToShadowCoordMatrices[cascadeIndex] = BuildWorldToShadowCoordMatrix(cascade.viewProjMatrix);
    }

    float cascadeFilterRadii[RenderFrameSettings::CSMCascadeCount] = { 1.0f, 1.0f, 1.0f, 1.0f };
    const float cascade0WorldUnitsPerTexel = cascadedShadow->cascades[0].worldUnitsPerTexel;
    const float cascade0FilterRadius = frameSettings.shadowMapFilterRadius * filterResolutionScale;
    const float minFilterRadius = RenderFrameSettings::ShadowMapFilterRadiusMin * filterResolutionScale;
    const float worldFilterRadius = cascade0FilterRadius * cascade0WorldUnitsPerTexel;

    for (uint32_t cascadeIndex = 0; cascadeIndex < cascadedShadow->cascadeCount; cascadeIndex++) {
        const float worldUnitsPerTexel = cascadedShadow->cascades[cascadeIndex].worldUnitsPerTexel;
        cascadeFilterRadii[cascadeIndex] = BE1::Clamp(worldFilterRadius / worldUnitsPerTexel, minFilterRadius, cascade0FilterRadius);
    }

    // RenderObject 를 한 번 순회하여 caster 와 cascade mask 를 수집한다.
    FindDirectionalShadowCasters(frame, renderCamera, cascadedShadow);

    cascadedShadow->depthTexture = depthTexture;

    cascadedShadow->shaderData.textureIndex = depthTexture->GetBindlessIndex(RHI::ViewType::SRV, renderContext->GetCSMDepthSRV());
    cascadedShadow->shaderData.cascadeCount = cascadedShadow->cascadeCount;
    cascadedShadow->shaderData.maxDistance = csmMaxDistance;

    cascadedShadow->shaderData.cascadeFilterRadii = float4(cascadeFilterRadii[0], cascadeFilterRadii[1], cascadeFilterRadii[2], cascadeFilterRadii[3]);

    const float texelSize = 1.0f / shadowMapResolution;
    cascadedShadow->shaderData.texelSize = float2(texelSize, texelSize);
    cascadedShadow->shaderData.normalizedBorder = shadowMapBorderTexels * texelSize;
    const float usableMapWidth = 1.0f - 2.0f * cascadedShadow->shaderData.normalizedBorder;
    const float cascadeBlendWidth = usableMapWidth * frameSettings.csmBlendFraction;
    cascadedShadow->shaderData.invBlendWidth = cascadeBlendWidth > 0.0f ? 1.0f / cascadeBlendWidth : 0.0f;

    const float farFadeRange = (csmMaxDistance - cameraDesc.zNear) * frameSettings.csmFadeFraction;
    cascadedShadow->shaderData.farFadeStart = csmMaxDistance - farFadeRange;
    cascadedShadow->shaderData.invFarFadeRange = 1.0f / farFadeRange;

    visCamera->cascadedShadow = cascadedShadow;
}

static float CalcDirectionalShadowSquaredLODErrorLimit(const VisCascadedShadow *cascadedShadow, uint8_t shadowCascadeMask, const BE1::Mat3x4 &worldMatrix, float shadowTexelErrorLimit) {
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

void RenderWorld::FindDirectionalShadowCasters(RenderFrame *frame, const RenderCamera *renderCamera, const VisCascadedShadow *cascadedShadow) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::FindDirectionalShadowCasters");

    const RenderCamera::Desc &cameraDesc = renderCamera->GetDesc();
    const bool staticOnly = BE1::HasFlag(cameraDesc.flags, RenderCamera::Flag::StaticOnly);
    const float meshLODPixelError = frame->GetRenderFrameSettings().meshLODPixelError;

    for (RenderObject *renderObject : renderObjects) {
        if (!renderObject || !renderObject->desc.meshInstance || !BE1::HasFlag(renderObject->desc.flags, RenderObject::Flag::CastShadows)) {
            continue;
        }

        if (!IsRenderLayerVisible(cameraDesc.layerMask, renderObject->desc.layer)) {
            continue;
        }

        if (staticOnly && !BE1::HasFlag(renderObject->desc.flags, RenderObject::Flag::Static)) {
            continue;
        }

        uint8_t shadowCascadeMask = 0;
        for (uint32_t cascadeIndex = 0; cascadeIndex < cascadedShadow->cascadeCount; cascadeIndex++) {
            const CSMCascade &cascade = cascadedShadow->cascades[cascadeIndex];

            if (!CullShadowCaster(cascadedShadow->viewVolumePlanes, cascade.shadowMapBounds, renderObject->worldAABB)) {
                shadowCascadeMask |= (uint8_t)BIT(cascadeIndex);
            }
        }

        // 오브젝트가 모든 캐스캐이드에서 컬링되면 shadow-map 에 그릴 필요가 없다.
        if (shadowCascadeMask == 0) {
            continue;
        }

        VisObject *visObject = RegisterVisObject(frame, renderObject);
        if (!visObject) {
            continue;
        }

        // 카메라에 안 보이는 caster 는 shadow 에 현재 world matrix 만 필요하다.
        // 공용 ShaderSurface 가 prevWorldMatrix 도 업로드하므로 현재 행렬로 초기화한다.
        if (!BE1::HasFlag(visObject->flags, VisObject::Flag::CameraVisible)) {
            visObject->worldMatrix = renderObject->GetWorldMatrix();
            visObject->prevWorldMatrix = visObject->worldMatrix;

            if (visObject->meshLOD < 0 && meshLODPixelError > 0.0f) {
                const Mesh *mesh = visObject->meshInstance->GetSourceMesh().get();
                if (mesh && mesh->HasLODs()) {
                    // 카메라 픽셀과 같은 허용 오차를 가장 정밀한 관련 cascade 의 shadow-map texel 단위로 적용한다.
                    visObject->squaredLODErrorLimit = CalcDirectionalShadowSquaredLODErrorLimit(cascadedShadow, shadowCascadeMask, visObject->worldMatrix, meshLODPixelError);
                }
            }
        }

        visObject->shadowCascadeMask |= shadowCascadeMask;
    }
}

void RenderWorld::PrepareOceanSurfacePipeline(VisCamera *visCamera) {
    if (!hasOceanSurface || BE1::HasFlag(visCamera->flags, RenderCamera::Flag::StaticOnly)) {
        return;
    }

    const GpuOceanSurface::Settings &surfaceSettings = oceanSurface.desc.surface;

    const SharedPtr<ShaderTemplate> &shaderTemplate = renderSystem->GetGpuOceanSurface().GetShaderTemplate();
    if (!shaderTemplate) {
        return;
    }

    ShaderFeatureSet featureSet;
    if (visCamera->needNormalOutput) {
        featureSet.EnableFeature(ShaderFeature::OutputNormal);
    }
    if (visCamera->needVelocityOutput) {
        featureSet.EnableFeature(ShaderFeature::OutputVelocity);
    }
    GraphicsPipelineDesc graphicsPipelineDesc;
    graphicsPipelineDesc.inputLayout = nullptr;
    graphicsPipelineDesc.rasterizerStateType = surfaceSettings.wireframe ? RHI::RasterizerStateType::Wire : RHI::RasterizerStateType::SolidTwoSided;
    graphicsPipelineDesc.depthStencilStateType = visCamera->projectionPolicy.reverseZ ? RHI::DepthStencilStateType::GEqualWrite : RHI::DepthStencilStateType::LEqualWrite;
    graphicsPipelineDesc.blendStateType = RHI::BlendStateType::Disabled;
    graphicsPipelineDesc.primitiveTopology = RHI::PrimitiveTopology::TriangleList;
    RenderContext::activeContext->GetMainRenderDest(graphicsPipelineDesc.renderDest, visCamera->needNormalOutput, visCamera->needVelocityOutput);

    const ShaderPermutationKey permutationKey = shaderTemplate->RegisterPermutation({}, featureSet);
    const bool allowAsyncPSOBuild = !BE1::HasFlag(visCamera->flags, RenderCamera::Flag::ForceSyncPassPSOBuild);
    const SharedPtr<ShaderPipeline> shaderPipeline = shaderPipelineCache.GetGraphicsPipeline(shaderTemplate, permutationKey, graphicsPipelineDesc, allowAsyncPSOBuild);
    visCamera->oceanSurfacePSO = ShaderPipeline::GetReadyPSO(shaderPipeline);
}

VisLight *RenderWorld::RegisterVisLight(RenderFrame *frame, RenderLight *renderLight) {
    assert(frameActive);

    if (IsVisLightRegisteredInCurrentView(renderLight)) {
        return nullptr;
    }

    VisLight *visLight = frame->AllocVisLight(renderLight->desc.type);
    if (!visLight) {
        return nullptr;
    }

    renderLight->viewCount = viewCount;

    if (renderLight->desc.type == RenderLight::Type::Point) {
        visLight->viewProjScaleBiasMatrix[0] = renderLight->viewProjScaleBiasMatrix[0];
        visLight->viewProjScaleBiasMatrix[1] = renderLight->viewProjScaleBiasMatrix[1];
    } else if (renderLight->desc.type == RenderLight::Type::Spot) {
        visLight->viewProjScaleBiasMatrix = renderLight->viewProjScaleBiasMatrix;

        VisSpotLightData *spotLightData = frame->AllocVisSpotLightData();
        spotLightData->fallOffMatrix = renderLight->fallOffMatrix;
        visLight->spotLightData = spotLightData;

        visLight->axis = renderLight->desc.axis;
    } else if (renderLight->desc.type == RenderLight::Type::BoxDir) {
        visLight->viewProjScaleBiasMatrix = renderLight->viewProjScaleBiasMatrix;
        visLight->axis = renderLight->desc.axis;
    } else if (renderLight->desc.type == RenderLight::Type::Directional) {
        visLight->axis = renderLight->desc.axis;
    }
    visLight->origin = renderLight->desc.origin;
    visLight->size = renderLight->desc.size;
    visLight->lightColor = renderLight->desc.linearColor * renderLight->desc.intensity;
    visLight->zNear = renderLight->desc.zNear;
    visLight->fallOffExponent = renderLight->desc.fallOffExponent;
    visLight->shadowOffsetFactor = renderLight->desc.shadowOffsetFactor;
    visLight->shadowOffsetUnits = renderLight->desc.shadowOffsetUnits;
    visLight->type = renderLight->desc.type;
    visLight->flags = renderLight->desc.flags;

    return visLight;
}

VisObject *RenderWorld::RegisterVisObject(RenderFrame *frame, RenderObject *renderObject) {
    assert(frameActive);

    if (IsVisObjectRegisteredInCurrentView(renderObject)) {
        return renderObject->visObject;
    }

    VisObject *visObject = frame->AllocVisObject();
    if (!visObject) {
        return nullptr;
    }

    renderObject->viewCount = viewCount;
    renderObject->visObject = visObject;
    visObject->meshLOD = frame->GetRenderFrameSettings().meshLOD ? renderObject->GetMeshLOD() : 0;

    if (renderObject->desc.meshInstance) {
        visObject->meshInstance = frame->RetainMeshInstance(renderObject->desc.meshInstance);
    }

    if (visObject->meshInstance && visObject->meshInstance->IsSkinned() && renderObject->desc.skeleton) {
        VisSkinnedMesh *visSkinnedMesh = frame->AllocVisSkinnedMesh();
        if (visSkinnedMesh) {
            visSkinnedMesh->space = visObject;
            visSkinnedMesh->meshInstance = visObject->meshInstance;
            visSkinnedMesh->skeleton = frame->RetainSkeleton(renderObject->desc.skeleton);
            visObject->skinnedMesh = visSkinnedMesh;
        }
    }

    const int numMaterials = renderObject->desc.materials.Count();
    visObject->numMaterials = static_cast<uint16_t>(numMaterials);
    if (numMaterials > 0) {
        const Material **materials;
        if (numMaterials > COUNT_OF(visObject->baseMaterials)) {
            materials = (const Material **)frame->MemAlloc(numMaterials * sizeof(materials[0]));
            visObject->materials = materials;
        } else {
            materials = visObject->baseMaterials;
        }
        for (int i = 0; i < numMaterials; ++i) {
            materials[i] = frame->RetainMaterial(renderObject->desc.materials[i]);
        }
    }

    if (renderObject->desc.text && !renderObject->desc.text->text.IsEmpty()) {
        RenderObject::TextDesc &textDesc = *(renderObject->desc.text);
        VisText *visText = frame->AllocVisText();
        if (visText) {
            visText->space = visObject;
            if (textDesc.font) {
                visText->font = frame->RetainFont(textDesc.font);
            }

            int size = textDesc.text.Length() + 1;
            char *textMemPtr = (char *)frame->MemAlloc(size);
            BE1::Str::Copynz(textMemPtr, textDesc.text.c_str(), size);
            visText->text = textMemPtr;
            visText->textRect = textDesc.textRect;
            visText->textShadowOffset = textDesc.textShadowOffset;
            visText->textShadowColor = textDesc.textShadowColor;
            visText->textParams = textDesc.textParams;
            visText->textScale = textDesc.textScale;
            visText->textLineSpacing = textDesc.textLineSpacing;
        }
    }

    return visObject;
}

void RenderWorld::SetVisObjectWorldMatrices(const RenderObject *renderObject, VisObject *visObject) {
    visObject->worldMatrix = renderObject->GetWorldMatrix();

    // 바로 직전 프레임에 prevWorldMatrix 를 세팅했었다면, 움직이던 오브젝트이므로 prevWorldMatrix 를 그대로 사용한다.
    const bool hasContinuousPrevWorldMatrix = frameIndex > 0 && renderObject->prevWorldMatrixFrameIndex == frameIndex - 1;

    visObject->prevWorldMatrix = hasContinuousPrevWorldMatrix ? renderObject->prevWorldMatrix : renderObject->desc.worldMatrix;
}

void RenderWorld::FindVisObjects(RenderFrame *frame, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::FindVisObjects");

    const bool skipViewCulling = frame->GetRenderFrameSettings().skipViewCulling;
    const float meshLODPixelError = frame->GetRenderFrameSettings().meshLODPixelError;
    const RenderCamera::Desc &cameraDesc = renderCamera->GetDesc();
    const bool staticOnly = BE1::HasFlag(cameraDesc.flags, RenderCamera::Flag::StaticOnly);

    // 같은 프레임 데이터로 RenderScene 을 여러 번 호출하면 앞선 카메라의 VisObject, VisSkinnedMesh, VisText 가 누적되어 있다.
    // 현재 카메라에서 새로 등록할 각 레코드의 범위를 구분하기 위해 시작 인덱스를 기록한다.
    visCamera->firstVisObjectIndex = frame->NumVisObjects();
    visCamera->firstVisSkinnedMeshIndex = frame->NumVisSkinnedMeshes();
    visCamera->firstVisTextIndex = frame->NumVisTexts();

    visCamera->worldAABB.Clear();

    // 등록된 모든 RenderObject 들을 개별 Culling 해서 프레임 데이터에 VisObject 로 등록한다.
    for (RenderObject *renderObject : renderObjects) {
        if (!renderObject) {
            continue;
        }

        if (!IsRenderLayerVisible(cameraDesc.layerMask, renderObject->desc.layer)) {
            continue;
        }

        if (staticOnly && !BE1::HasFlag(renderObject->desc.flags, RenderObject::Flag::Static)) {
            continue;
        }

        if (!skipViewCulling) {
            if (renderCamera->Cull(renderObject->worldAABB)) {
                continue;
            }
        }

        VisObject *visObject = RegisterVisObject(frame, renderObject);
        if (!visObject) {
            continue;
        }

        renderObject->visibleFrameIndex = frameIndex;

        SetVisObjectWorldMatrices(renderObject, visObject);
        visObject->flags |= VisObject::Flag::CameraVisible;

        if (visObject->meshInstance && visObject->meshLOD < 0 && meshLODPixelError > 0.0f) {
            const Mesh *mesh = visObject->meshInstance->GetSourceMesh().get();
            if (mesh && mesh->HasLODs()) {
                // 같은 카메라/객체의 SubMesh 들이 사용할 허용 오차를 한 번 계산한다.
                visObject->squaredLODErrorLimit = renderCamera->CalcLocalLODErrorLimitSqr(visObject->worldMatrix, renderObject->GetWorldAABB(), meshLODPixelError);
            }
        }

        visCamera->worldAABB.AddAABB(renderObject->worldAABB);
    }
}

void RenderWorld::UpdateSkinningJointCaches(RenderFrame *frame, const VisSkinnedMesh **objects, uint32_t numObjects, uint32_t numSkinningJoints) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::UpdateSkinningJointCaches");

    if (numObjects == 0 || numSkinningJoints == 0) {
        return;
    }

#ifdef USE_RENDER_TASK
    static constexpr uint32_t MaxSkinningJointsPerTask = 8192;

    uint32_t numTasks = (numSkinningJoints + MaxSkinningJointsPerTask - 1) / MaxSkinningJointsPerTask;
    numTasks = BE1::Min(frame->NumRenderTaskThreads(), numTasks);

    if (numTasks > 1) {
        UpdateSkinningJointCachesWithTask(objects, numObjects, numSkinningJoints, numTasks);
    } else {
        UpdateSkinningJointCachesWithoutTask(objects, numObjects);
    }
#else
    UpdateSkinningJointCachesWithoutTask(objects, numObjects);
#endif
}

void RenderWorld::UpdateSkinningJointCachesWithoutTask(const VisSkinnedMesh **objects, uint32_t numObjects) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::UpdateSkinningJointCachesWithoutTask");

    for (uint32_t i = 0; i < numObjects; ++i) {
        const VisSkinnedMesh *visSkinnedMesh = objects[i];
        visSkinnedMesh->meshInstance->UpdateSkinningJointCache(visSkinnedMesh->skeleton);
    }
}

void RenderWorld::UpdateSkinningJointCachesByTask(RenderWorld::UpdateSkinningJointCachesTaskDesc *taskDesc) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::UpdateSkinningJointCachesByTask");

    for (uint32_t i = 0; i < taskDesc->numSkinnedMeshObjects; ++i) {
        const VisSkinnedMesh *visSkinnedMesh = taskDesc->skinnedMeshObjects[i];
        visSkinnedMesh->meshInstance->UpdateSkinningJointCache(visSkinnedMesh->skeleton);
    }
}

void RenderWorld::UpdateSkinningJointCachesByTaskFunction(void *data) {
    RenderWorld::UpdateSkinningJointCachesTaskDesc *taskDesc = reinterpret_cast<RenderWorld::UpdateSkinningJointCachesTaskDesc *>(data);
    taskDesc->renderWorld->UpdateSkinningJointCachesByTask(taskDesc);
}

void RenderWorld::UpdateSkinningJointCachesWithTask(const VisSkinnedMesh **objects, uint32_t numObjects, uint32_t numSkinningJoints, uint32_t numTasks) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::UpdateSkinningJointCachesWithTask");

    const uint32_t targetJointsPerTask = (numSkinningJoints + numTasks - 1) / numTasks;
    uint32_t firstIndex = 0;

    updateSkinningJointCachesTaskDescs.Reserve(numTasks);
    updateSkinningJointCachesTaskDescs.SetCount(0, false);

    while (firstIndex < numObjects) {
        uint32_t endIndex = firstIndex;
        uint32_t numTaskJoints = 0;
        const bool lastTask = updateSkinningJointCachesTaskDescs.Count() + 1 == static_cast<int>(numTasks);
        do {
            numTaskJoints += objects[endIndex++]->meshInstance->NumJointMats();
        } while (endIndex < numObjects && (lastTask || numTaskJoints < targetJointsPerTask));

        UpdateSkinningJointCachesTaskDesc &taskDesc = updateSkinningJointCachesTaskDescs.Alloc();
        taskDesc.renderWorld = this;
        taskDesc.skinnedMeshObjects = objects + firstIndex;
        taskDesc.numSkinnedMeshObjects = endIndex - firstIndex;
        firstIndex = endIndex;
    }

    for (int taskIndex = 0; taskIndex < updateSkinningJointCachesTaskDescs.Count(); ++taskIndex) {
        UpdateSkinningJointCachesTaskDesc &taskDesc = updateSkinningJointCachesTaskDescs[taskIndex];
        if (!BE1::Engine::taskManager->AddTask(RenderWorld::UpdateSkinningJointCachesByTaskFunction, &taskDesc, renderWorldGroupId, false)) {
            UpdateSkinningJointCachesByTask(&taskDesc);
        }
    }

    BE1::Engine::taskManager->WaitFinish(renderWorldGroupId, true);
}

int RenderWorld::CountSkinningUploadJoints(const RenderFrame::ChunkRange<VisSkinnedMesh> &visSkinnedMeshes, bool needVelocityOutput, uint64_t currentFrameSerial) const {
    uint32_t numTotalUploadJoints = 0;

    for (const VisSkinnedMesh &visSkinnedMesh : visSkinnedMeshes) {
        if (!visSkinnedMesh.space->IsRenderable()) {
            continue;
        }

        const SkinningJointCache *skinningJointCache = visSkinnedMesh.meshInstance->GetSkinningJointCache();
        if (!skinningJointCache) {
            continue;
        }

        // 캐시가 최신 상태가 아니라면 업로드를 하지 않는다.
        if (!skinningJointCache->IsUpToDate(visSkinnedMesh.skeleton, visSkinnedMesh.meshInstance->GetJointMatsSerial())) {
            continue;
        }

        const uint32_t numJoints = static_cast<uint32_t>(skinningJointCache->NumJoints());
        const bool needPreviousPose = needVelocityOutput && BE1::HasFlag(visSkinnedMesh.space->flags, VisObject::Flag::CameraVisible);
        const uint32_t numPoses = needPreviousPose && skinningJointCache->HasPreviousPose(currentFrameSerial) ? 2u : 1u;
        const uint32_t numUploadJoints = numJoints * numPoses;

        if (numUploadJoints > (UINT32_MAX / sizeof(ShaderSkinningJoint) - numTotalUploadJoints)) {
            return -1;
        }

        numTotalUploadJoints += numUploadJoints;
    }
    return static_cast<int>(numTotalUploadJoints);
}

void RenderWorld::BuildSkinningJointBuffer(RenderFrame *frame, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::BuildSkinningJointBuffer");

    if (frame->NumVisSkinnedMeshes() == visCamera->firstVisSkinnedMeshIndex) {
        return;
    }

    skinningJointCacheUpdateObjects.SetCount(0, false);

    uint32_t numTotalJoints = 0;

    const auto visSkinnedMeshes = frame->GetVisSkinnedMeshes(visCamera->firstVisSkinnedMeshIndex);
    const uint64_t currentFrameSerial = frame->GetFrameSerial();

    for (const VisSkinnedMesh &visSkinnedMesh : visSkinnedMeshes) {
        if (!visSkinnedMesh.space->IsRenderable()) {
            continue;
        }

        if (!visSkinnedMesh.skeleton->IsReady() || !visSkinnedMesh.meshInstance->EnsureCurrent()) {
            continue;
        }

        SkinningJointCache *skinningJointCache = visSkinnedMesh.meshInstance->GetSkinningJointCache();
        if (!skinningJointCache) {
            continue;
        }

        // 캐시의 joint pose 나 skeleton 이 변경되었다면 업데이트를 예약한다.
        // 캐시 업데이트가 예약된 상태이거나 이미 업데이트된 상태라면 건너뛴다.
        if (!skinningJointCache->PrepareUpdate(visSkinnedMesh.skeleton, visSkinnedMesh.meshInstance->GetJointMatsSerial(), currentFrameSerial)) {
            continue;
        }

        skinningJointCacheUpdateObjects.Alloc() = &visSkinnedMesh;

        numTotalJoints += skinningJointCache->NumJoints();
    }

    // 앞선 카메라나 프레임에서 이미 갱신된 캐시도 현재 카메라의 jointBuffer 에 업로드해야 한다.
    UpdateSkinningJointCaches(frame, skinningJointCacheUpdateObjects.Ptr(), skinningJointCacheUpdateObjects.Count(), numTotalJoints);

    // jointBuffer 에 업로드할 전체 joint pose 개수를 계산한다. 카메라 velocity 에 필요한 이전 pose 만 포함한다.
    const int numTotalUploadJoints = CountSkinningUploadJoints(visSkinnedMeshes, visCamera->needVelocityOutput, currentFrameSerial);
    if (numTotalUploadJoints < 0) {
        BE_WARNLOG("RenderWorld::BuildSkinningJointBuffer: too many skinning joints\n");
        return;
    }

    if (numTotalUploadJoints == 0) {
        return;
    }

    // 전체 joint pose 를 한 번에 업로드하기 위해서 업로드 버퍼를 할당한다.
    RHI::Buffer *jointBuffer = frame->GetThreadData(0)->AllocStructuredBuffer(sizeof(ShaderSkinningJoint), static_cast<uint32_t>(numTotalUploadJoints));
    if (!jointBuffer) {
        return;
    }

    // 업로드 버퍼에는 메인 스레드에서 연속으로 복사한다.
    BE1::Mat3x4 *dst = static_cast<BE1::Mat3x4 *>(jointBuffer->mappedPtr);
    uint32_t jointOffset = 0;
    for (VisSkinnedMesh &visSkinnedMesh : visSkinnedMeshes) {
        if (!visSkinnedMesh.space->IsRenderable()) {
            continue;
        }

        const SkinningJointCache *skinningJointCache = visSkinnedMesh.meshInstance->GetSkinningJointCache();
        if (!skinningJointCache) {
            continue;
        }

        if (!skinningJointCache->IsUpToDate(visSkinnedMesh.skeleton, visSkinnedMesh.meshInstance->GetJointMatsSerial())) {
            continue;
        }

        const uint32_t numJoints = static_cast<uint32_t>(skinningJointCache->NumJoints());
        const uint32_t byteSize = numJoints * sizeof(ShaderSkinningJoint);

        BE1::simdProcessor->Memcpy(dst + jointOffset, skinningJointCache->GetSkinningJoints(), byteSize);
        visSkinnedMesh.skinningJointOffset = jointOffset;
        jointOffset += numJoints;

        visSkinnedMesh.prevSkinningJointOffset = visSkinnedMesh.skinningJointOffset;

        const bool needPreviousPose = visCamera->needVelocityOutput && BE1::HasFlag(visSkinnedMesh.space->flags, VisObject::Flag::CameraVisible);
        if (needPreviousPose && skinningJointCache->HasPreviousPose(currentFrameSerial)) {
            BE1::simdProcessor->Memcpy(dst + jointOffset, skinningJointCache->GetPrevSkinningJoints(), byteSize);
            visSkinnedMesh.prevSkinningJointOffset = jointOffset;
            jointOffset += numJoints;
        }
    }

    assert(jointOffset == static_cast<uint32_t>(numTotalUploadJoints));
    visCamera->skinningJointBuffer = jointBuffer;
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

static BE_INLINE void CullMeshSurface(const BE1::AABB &localAABB, const BE1::Mat3x4 &worldMatrix, const RenderCamera *renderCamera, const VisCascadedShadow *cascadedShadow, bool cullCamera, bool &surfaceCameraVisible, uint8_t &surfaceShadowCascadeMask) {
    if (localAABB.IsCleared()) {
        return;
    }

    // Static submesh 의 world bounds 는 카메라와 shadow 컬링에 한 번만 계산하여 공유한다.
    BE1::AABB surfaceWorldAABB;
    surfaceWorldAABB.SetFromTransformedAABB(localAABB, worldMatrix);

    if (cullCamera) {
        surfaceCameraVisible = !renderCamera->Cull(surfaceWorldAABB);
    }

    if (surfaceShadowCascadeMask == 0) {
        return;
    }

    for (uint32_t cascadeIndex = 0; cascadeIndex < cascadedShadow->cascadeCount; ++cascadeIndex) {
        const uint8_t cascadeBit = static_cast<uint8_t>(BIT(cascadeIndex));
        if ((surfaceShadowCascadeMask & cascadeBit) == 0) {
            continue;
        }

        const CSMCascade &cascade = cascadedShadow->cascades[cascadeIndex];
        if (CullShadowCaster(cascadedShadow->viewVolumePlanes, cascade.shadowMapBounds, surfaceWorldAABB)) {
            surfaceShadowCascadeMask &= ~cascadeBit;
        }
    }
}

void RenderWorld::AddMeshes(RenderFrame *frame, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::AddMeshes");

    // 현재 카메라에서 새로 등록된 VisObject 가 없으면 메시 처리를 생략한다.
    if (frame->NumVisObjects() == visCamera->firstVisObjectIndex) {
        return;
    }

    BuildSkinningJointBuffer(frame, visCamera);

    const bool skipViewCulling = frame->GetRenderFrameSettings().skipViewCulling;
    const VisCascadedShadow *cascadedShadow = visCamera->cascadedShadow;

    for (VisObject &visObject : frame->GetVisObjects(visCamera->firstVisObjectIndex)) {
        const bool objectCameraVisible = BE1::HasFlag(visObject.flags, VisObject::Flag::CameraVisible);
        const uint8_t objectShadowCascadeMask = cascadedShadow ? visObject.shadowCascadeMask : 0;
        if (!objectCameraVisible && objectShadowCascadeMask == 0) {
            continue;
        }

        MeshInstance *meshInstance = visObject.meshInstance;
        if (!meshInstance || !meshInstance->EnsureCurrent()) {
            continue;
        }

        const bool skinned = meshInstance->IsSkinned();

        // Pose 준비에 실패한 skinned mesh 를 bind pose 상태로 등록하지 않는다.
        if (skinned && (!visObject.skinnedMesh ||
            visObject.skinnedMesh->skinningJointOffset == ShaderSkinningJointOffsetInvalid)) {
            continue;
        }

        const int numSurfaces = meshInstance->NumSurfaces();

        // Skinned submesh 의 bounds 는 bind pose 기준이므로 오브젝트 수집 단계의 bounds 판정을 유지한다.
        const bool cullSurfaces = numSurfaces > 1 && !skinned;
        const bool cullCameraSurfaces = objectCameraVisible && !skipViewCulling;

        for (int meshSurfIndex = 0; meshSurfIndex < numSurfaces; ++meshSurfIndex) {
            const Mesh::Surface *meshSurf = meshInstance->GetSurface(meshSurfIndex);
            if (!meshSurf || !meshSurf->subMesh) {
                continue;
            }

            const Material *material = materialManager.defaultLitMaterial.get();
            if (meshSurf->materialIndex >= 0 && static_cast<uint32_t>(meshSurf->materialIndex) < visObject.numMaterials) {
                material = visObject.GetMaterials()[meshSurf->materialIndex];
            }

            const bool castsShadow = !BE1::HasFlag(material->GetFlags(), Material::Flag::NoShadow) && material->GetRenderingMode() != Material::RenderingMode::Blend;

            bool surfaceCameraVisible = objectCameraVisible;
            uint8_t surfaceShadowCascadeMask = castsShadow ? objectShadowCascadeMask : 0;
            if (!surfaceCameraVisible && surfaceShadowCascadeMask == 0) {
                continue;
            }

            if (cullSurfaces && (cullCameraSurfaces || surfaceShadowCascadeMask != 0)) {
                CullMeshSurface(meshSurf->subMesh->GetBoundingBox(), visObject.worldMatrix, renderCamera, cascadedShadow, cullCameraSurfaces, surfaceCameraVisible, surfaceShadowCascadeMask);
            }

            if (!surfaceCameraVisible && surfaceShadowCascadeMask == 0) {
                continue;
            }

            const VisSurfFlag flags = surfaceCameraVisible ? VisSurfFlag::CameraVisible : VisSurfFlag::None;

            // 카메라에 보이는 surface 는 카메라 기준, shadow 전용 surface 는 가장 정밀한 관련 cascade 기준 LOD 를 사용한다.
            // 한 surface 가 여러 cascade 에 투영되어도 같은 LOD 를 공유하여 blend 영역의 caster silhouette 를 일치시킨다.
            const int lodLevel = SelectMeshLOD(meshSurf->subMesh, visObject.meshLOD, visObject.squaredLODErrorLimit);
            AddVisSurf(frame, visCamera, nullptr, &visObject, material, meshSurf->subMesh, flags, surfaceShadowCascadeMask, lodLevel);
        }
    }
}

void RenderWorld::AddTextMeshes(RenderFrame *frame, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::AddTextMeshes");

    // 이전 카메라에서 등록한 VisText 는 제외하고 현재 카메라의 시작 위치부터 순회한다.
    for (VisText &visText : frame->GetVisTexts(visCamera->firstVisTextIndex)) {
        VisObject *visObject = visText.space;
        if (!BE1::HasFlag(visObject->flags, VisObject::Flag::CameraVisible)) {
            continue;
        }

        textMesh.Clear();

        if (visText.textRect.IsEmpty()) {
            textMesh.DrawText3D(frame->GetThreadData(0), (RenderObject::TextDrawMode)visText.textParams.bits.textDrawMode,
                (RenderObject::TextAnchor)visText.textParams.bits.textAnchor, (RenderObject::TextHorzAlignment)visText.textParams.bits.textHorzAlignment,
                visText.textLineSpacing, visText.textScale, BE1::Color4::white.ToUInt32(), visText.textShadowColor, visText.textShadowOffset, visText.font, visText.text);
        } else {
            textMesh.DrawText3D(frame->GetThreadData(0), (RenderObject::TextDrawMode)visText.textParams.bits.textDrawMode, visText.textRect,
                (RenderObject::TextHorzAlignment)visText.textParams.bits.textHorzAlignment, (RenderObject::TextVertAlignment)visText.textParams.bits.textVertAlignment,
                (RenderObject::TextHorzOverflow)visText.textParams.bits.textHorzOverflow, (RenderObject::TextVertOverflow)visText.textParams.bits.textVertOverflow,
                visText.textLineSpacing, visText.textScale, BE1::Color4::white.ToUInt32(), visText.textShadowColor, visText.textShadowOffset, visText.font, visText.text);
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

            AddVisSurf(frame, visCamera, nullptr, visObject, guiSurf->material, subMesh, VisSurfFlag::CameraVisible, 0);
        }
    }
}

void RenderWorld::AddSkyBoxMeshes(RenderFrame *frame, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::AddSkyBoxMeshes");

    if (renderCamera->GetDesc().clearMethod != RenderCamera::ClearMethod::Skybox) {
        return;
    }

    if (!skyboxMaterial || !meshManager.defaultBoxMesh || !meshManager.defaultBoxMesh->IsReady()) {
        return;
    }

    const Mesh::Surface *meshSurf = meshManager.defaultBoxMesh->GetSurface(0);
    if (!meshSurf || !meshSurf->subMesh) {
        return;
    }

    VisObject *visObject = frame->AllocVisObject();
    if (!visObject) {
        return;
    }

    visObject->worldMatrix.SetTRS(renderCamera->GetDesc().origin, BE1::Mat3::identity, BE1::Vec3(10, 10, 10));
    visObject->prevWorldMatrix = visObject->worldMatrix;
    visObject->flags |= VisObject::Flag::CameraVisible;
    Material *material = frame->RetainMaterial(skyboxMaterial);

    AddVisSurf(frame, visCamera, nullptr, visObject, material, meshSurf->subMesh, VisSurfFlag::CameraVisible | VisSurfFlag::SkipSelection | VisSurfFlag::Skybox, 0);
}

void RenderWorld::FindVisLights(RenderFrame *frame, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::FindVisLights");

    const RenderFrameSettings &frameSettings = frame->GetRenderFrameSettings();
    const bool skipViewCulling = frameSettings.skipViewCulling;
    const bool localShadowsEnabled = frameSettings.shadows && frameSettings.shadowAtlasMaxViews > 0;

    const RenderCamera::Desc &cameraDesc = renderCamera->GetDesc();
    const bool staticOnly = BE1::HasFlag(cameraDesc.flags, RenderCamera::Flag::StaticOnly);
    const BE1::Vec3 &viewZAxis = cameraDesc.axis[EngineAxis::Forward];
    const float viewZOffset = -viewZAxis.Dot(cameraDesc.origin);

    // 현재 카메라의 local light 참조가 시작되는 프레임 공용 ShaderLightRef 테이블의 오프셋을 기록한다.
    visCamera->lightCulling.lightRefTableOffset = frame->NumVisLocalLights();

    // RenderWorld 에 등록된 renderLights 개수만큼 미리 visCamera->visLocalLights 를 할당
    visCamera->maxVisLocalLights = static_cast<uint32_t>(renderLights.Count());
    if (visCamera->maxVisLocalLights > 0) {
        visCamera->visLocalLights = (const VisLight **)frame->MemAlloc(visCamera->maxVisLocalLights * sizeof(visCamera->visLocalLights[0]));
    }

    // 등록된 모든 RenderLight 들을 프레임 데이터에 VisLight 로 등록한다.
    for (RenderLight *renderLight : renderLights) {
        if (!renderLight) {
            continue;
        }

        if (!IsRenderLayerVisible(cameraDesc.layerMask, renderLight->desc.layer)) {
            continue;
        }

        if (staticOnly && !BE1::HasFlag(renderLight->desc.flags, RenderLight::Flag::Static)) {
            continue;
        }

        if (renderLight->desc.type == RenderLight::Type::Directional) {
            visCamera->directionalLightVisible = true;

            // Directional light 는 모든 카메라가 공유하는 프레임 단위 Primary light 하나만 등록한다.
            if (frame->GetPrimaryDirectionalLight()) {
                continue;
            }

            VisLight *visLight = RegisterVisLight(frame, renderLight);
            if (!visLight) {
                continue;
            }
        } else {
            if (!skipViewCulling) {
                if (renderLight->Cull(*renderCamera)) {
                    continue;
                }

                if (!renderLight->IsIntersectAABB(visCamera->worldAABB)) {
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

            VisLight *visLight = RegisterVisLight(frame, renderLight);
            if (!visLight) {
                continue;
            }

            visLight->viewZMin = viewZMin;
            visLight->viewZMax = viewZMax;

            const uint32_t projectedSize = ComputeLightTileRange(visCamera, screenClipRect, visLight->packedTileMin, visLight->packedTileMax);
            if (localShadowsEnabled && BE1::HasFlag(visLight->flags, RenderLight::Flag::CastShadows)) {
                if (projectedSize > 0 && (visLight->type == RenderLight::Type::Spot || visLight->type == RenderLight::Type::Point)) {
                    // 광원 영향 영역으로 요청 해상도를 근사한다.
                    const uint32_t clampedSize = BE1::Clamp(projectedSize, RenderFrameSettings::ShadowAtlasMinTileResolution, frameSettings.shadowAtlasMaxTileResolution);
                    visLight->shadowRequestedResolution = BE1::Math::RoundUpPowerOfTwo(clampedSize);
                }
            }

            visCamera->visLocalLights[visCamera->numVisLocalLights++] = visLight;
        }

        renderLight->visibleFrameIndex = frameIndex;
    }
}

static BE_INLINE uint16_t PackLightTileCoord(uint32_t x, uint32_t y) {
    assert(x <= VisLight::TileCoordMask);
    assert(y <= VisLight::TileCoordMask);
    return static_cast<uint16_t>(x | (y << VisLight::TileCoordBitCount));
}

// Light-culling 타일 범위를 계산하고, 렌더 픽셀 기준 광원 화면 영역의 긴 변 길이를 반환한다.
uint32_t RenderWorld::ComputeLightTileRange(const VisCamera *visCamera, const BE1::Rect &screenClipRect, uint16_t &packedTileMin, uint16_t &packedTileMax) {
    if (visCamera->outputRect.w <= 0 || visCamera->outputRect.h <= 0 || visCamera->renderWidth == 0 || visCamera->renderHeight == 0) {
        assert(0);
        packedTileMin = 0;
        packedTileMax = 0;
        return 0;
    }

    const int32_t outputMinX = visCamera->outputRect.x;
    const int32_t outputMinY = visCamera->outputRect.y;
    const int32_t outputMaxX = outputMinX + visCamera->outputRect.w;
    const int32_t outputMaxY = outputMinY + visCamera->outputRect.h;

    int32_t clipMinX = screenClipRect.x;
    int32_t clipMinY = screenClipRect.y;
    int32_t clipMaxX = static_cast<int32_t>(screenClipRect.x) + screenClipRect.w;
    int32_t clipMaxY = static_cast<int32_t>(screenClipRect.y) + screenClipRect.h;

    // 1 픽셀보다 작은 투영 영역은 정수 Rect 에서 크기가 0 이 될 수 있으므로 최소 한 픽셀을 보수적으로 포함한다.
    if (clipMinX >= clipMaxX && screenClipRect.w == 0) {
        clipMinX = BE1::Clamp<int32_t>(screenClipRect.x, outputMinX, outputMaxX - 1);
        clipMaxX = clipMinX + 1;
    }
    if (clipMinY >= clipMaxY && screenClipRect.h == 0) {
        clipMinY = BE1::Clamp<int32_t>(screenClipRect.y, outputMinY, outputMaxY - 1);
        clipMaxY = clipMinY + 1;
    }

    if (clipMinX >= clipMaxX || clipMinY >= clipMaxY) {
        assert(0);
        packedTileMin = 0;
        packedTileMax = 0;
        return 0;
    }

    const uint32_t outputWidth = static_cast<uint32_t>(visCamera->outputRect.w);
    const uint32_t outputHeight = static_cast<uint32_t>(visCamera->outputRect.h);
    const uint32_t renderWidth = visCamera->renderWidth;
    const uint32_t renderHeight = visCamera->renderHeight;

    // screenClipRect 는 outputRect 좌표계이므로, 동적 해상도를 반영한 렌더 타겟 좌표계로 변환한다.
    const uint32_t renderMinX = static_cast<uint32_t>(clipMinX - outputMinX) * renderWidth / outputWidth;
    const uint32_t renderMinY = static_cast<uint32_t>(clipMinY - outputMinY) * renderHeight / outputHeight;
    const uint32_t renderMaxX = (static_cast<uint32_t>(clipMaxX - outputMinX) * renderWidth + outputWidth - 1) / outputWidth;
    const uint32_t renderMaxY = (static_cast<uint32_t>(clipMaxY - outputMinY) * renderHeight + outputHeight - 1) / outputHeight;

    // 렌더 타겟 좌표계 기준으로 타일 좌표 (MinX, MinY, MaxX, MaxY) 를 구한다.
    const uint32_t tileMinX = static_cast<uint32_t>(renderMinX / ShaderLightTileSize);
    const uint32_t tileMinY = static_cast<uint32_t>(renderMinY / ShaderLightTileSize);
    const uint32_t tileMaxX = static_cast<uint32_t>((renderMaxX - 1) / ShaderLightTileSize);
    const uint32_t tileMaxY = static_cast<uint32_t>((renderMaxY - 1) / ShaderLightTileSize);

    packedTileMin = PackLightTileCoord(tileMinX, tileMinY);
    packedTileMax = PackLightTileCoord(tileMaxX, tileMaxY);

    // 타일 단위로 양자화하기 전의 크기를 shadow 요청 해상도 계산에서도 공유한다.
    return BE1::Max(renderMaxX - renderMinX, renderMaxY - renderMinY);
}

void RenderWorld::BuildLightCullingData(RenderFrame *frame, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::BuildLightCullingData");

    // 현재 카메라의 visible lights 를 minmax z 에 따라 정렬하고, z-bins 를 작성한다.
    BuildLightZBins(frame, renderCamera, visCamera);

    if (visCamera->lightCulling.zBins) {
        // 현재 카메라의 Z-bin 이 시작되는 프레임 공용 light Z-bin 테이블을 예약한다.
        visCamera->lightCulling.zBinTableOffset = frame->ReserveLightCullingZBins();
    }

    if (visCamera->numVisLocalLights > 0) {
        assert(visCamera->renderWidth > 0 && visCamera->renderHeight > 0);

        // 렌더링 화면에 필요한 타일 개수
        visCamera->lightCulling.tileCountX = (visCamera->renderWidth + ShaderLightTileSize - 1) / ShaderLightTileSize;
        visCamera->lightCulling.tileCountY = (visCamera->renderHeight + ShaderLightTileSize - 1) / ShaderLightTileSize;

        // 각 타일이 필요한 비트 마스크 워드의 개수
        visCamera->lightCulling.wordsPerTile = (visCamera->numVisLocalLights + ShaderLightWordBits - 1) / ShaderLightWordBits;

        // 전체 타일에 필요한 비트 마스크 워드의 개수
        const uint64_t numLightCullingWords = static_cast<uint64_t>(visCamera->lightCulling.tileCountX) * visCamera->lightCulling.tileCountY * visCamera->lightCulling.wordsPerTile;

        if (numLightCullingWords <= UINT32_MAX) {
            // 현재 카메라의 타일 마스크 워드가 시작되는 프레임 공용 테이블을 예약한다.
            visCamera->lightCulling.tileWordTableOffset = frame->ReserveLightCullingWords(static_cast<uint32_t>(numLightCullingWords));
        }
    }
}

static BE_INLINE uint32_t PackLightZBin(uint32_t minLightIndex, uint32_t maxLightIndex) {
    assert(minLightIndex <= ShaderLightZBinIndexMask);
    assert(maxLightIndex <= ShaderLightZBinIndexMask);
    return (maxLightIndex << 16) | minLightIndex;
}

void RenderWorld::BuildLightZBins(RenderFrame *frame, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::BuildLightZBins");

    const RenderCamera::Desc &cameraDesc = renderCamera->GetDesc();

    const uint32_t numVisLocalLights = visCamera->numVisLocalLights;
    if (numVisLocalLights == 0) {
        return;
    }

    if (numVisLocalLights > 1) {
        // viewZMin, viewZMax 순으로 오름차순 정렬하고, 두 값이 같으면 index 가 작은 라이트를 먼저 배치해 순서를 고정한다.
        std::sort(visCamera->visLocalLights, visCamera->visLocalLights + numVisLocalLights, [](const VisLight *lhs, const VisLight *rhs) {
            if (lhs->viewZMin < rhs->viewZMin) {
                return true;
            }
            if (lhs->viewZMin > rhs->viewZMin) {
                return false;
            }
            if (lhs->viewZMax < rhs->viewZMax) {
                return true;
            }
            if (lhs->viewZMax > rhs->viewZMax) {
                return false;
            }
            return lhs->index < rhs->index;
        });
    }

    // Z-bin light index 는 16 비트로 패킹된다. 오버플로가 발생하면 라이트를 누락하지 않고 GPU 경로에서
    // 폴백할 수 있도록 정렬된 라이트 목록은 유지하고 lightCulling.zBins 는 nullptr 로 둔다.
    if (numVisLocalLights > ShaderLightZBinMaxLightCount) {
        return;
    }

    if (cameraDesc.zFar <= cameraDesc.zNear) {
        return;
    }

    const float viewZRange = cameraDesc.zFar - cameraDesc.zNear;
    const float zBinSize = viewZRange / static_cast<float>(ShaderLightZBinCount);

    // linear view Z 를 Z-bin index 로 변환하기 위한 scale 과 bias 를 미리 계산한다.
    visCamera->lightCulling.zBinScale = static_cast<float>(ShaderLightZBinCount) / viewZRange;
    visCamera->lightCulling.zBinBias = -cameraDesc.zNear * visCamera->lightCulling.zBinScale;
    visCamera->lightCulling.zBins = (uint32_t *)frame->MemAlloc(ShaderLightZBinCount * sizeof(visCamera->lightCulling.zBins[0]));

    uint32_t startLightIndex = 0;
    uint32_t endLightIndex = 0;

    // 각 Z-bin 에 라이트 범위 [startLightIndex, endLightIndex) 를 패킹한다. (endLightIndex 의 라이트는 포함되지 않는다)
    for (uint32_t zBinIndex = 0; zBinIndex < ShaderLightZBinCount; ++zBinIndex) {
        const float currentZBinMin = cameraDesc.zNear + static_cast<float>(zBinIndex) * zBinSize;
        const float currentZBinMax = zBinIndex + 1 == ShaderLightZBinCount ? cameraDesc.zFar : currentZBinMin + zBinSize;

        while (startLightIndex < numVisLocalLights && visCamera->visLocalLights[startLightIndex]->viewZMax < currentZBinMin) {
            ++startLightIndex;
        }
        while (endLightIndex < numVisLocalLights && visCamera->visLocalLights[endLightIndex]->viewZMin <= currentZBinMax) {
            ++endLightIndex;
        }

        if (startLightIndex < endLightIndex) {
            visCamera->lightCulling.zBins[zBinIndex] = PackLightZBin(startLightIndex, endLightIndex - 1);
        } else {
            visCamera->lightCulling.zBins[zBinIndex] = ShaderLightZBinEmpty;
        }
    }
}

void RenderWorld::BuildDebugDraw(RenderFrame *frame, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::BuildDebugDraw");

    if (BE1::HasFlag(visCamera->flags, RenderCamera::Flag::SkipDebugDraw)) {
        return;
    }

    visCamera->showDebugLights = frame->GetRenderFrameSettings().showDebugLights && visCamera->numVisLocalLights > 0;

    // 현재 debug primitive 저장소의 프레임 snapshot 을 카메라에 연결한다.
    visCamera->debugDrawData = BuildDebugDrawData(frame);
    PrepareDebugDrawPipelines(visCamera);
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

void RenderWorld::PrepareDebugDrawPipelines(VisCamera *visCamera) {
    const DebugDrawData *drawData = visCamera->debugDrawData;
    if (!drawData && !visCamera->showDebugLights) {
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

    if (visCamera->showDebugLights) {
        requiredPipelines[static_cast<int>(DebugDrawPipelineId::DepthTestedLines)] = true;
    }

    GraphicsPipelineDesc graphicsPipelineDesc;
    graphicsPipelineDesc.inputLayout = &VertexInputLayoutFactory::Get(VertexInputLayoutId::XyzColor);
    graphicsPipelineDesc.blendStateType = RHI::BlendStateType::AlphaBlend;
    RenderContext::activeContext->GetMainRenderDest(graphicsPipelineDesc.renderDest, visCamera->needNormalOutput, visCamera->needVelocityOutput);

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
            graphicsPipelineDesc.depthStencilStateType = visCamera->projectionPolicy.reverseZ ? RHI::DepthStencilStateType::GEqualNoWrite : RHI::DepthStencilStateType::LEqualNoWrite;
        }

        const SharedPtr<ShaderPipeline> shaderPipeline = shaderPipelineCache.GetGraphicsPipeline(shaderTemplateManager.debugDrawShaderTemplate, ShaderPermutationKey(), graphicsPipelineDesc);
        visCamera->debugDrawPSOs[pipelineIndex] = ShaderPipeline::GetReadyPSO(shaderPipeline);
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

    // guiMesh 를 기반으로 GUI VisCamera 를 구성하여, 렌더링 커맨드에 DrawCamera 명령을 기록한다.
    DrawGUICamera(frame, screenWidth, screenHeight, guiMesh);
}

void RenderWorld::DrawGUICamera(RenderFrame *frame, uint32_t screenWidth, uint32_t screenHeight, GuiMesh &guiMesh) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::DrawGUICamera");

    VisCamera *visCamera = frame->AllocVisCamera();
    if (!visCamera) {
        return;
    }

    visCamera->maxVisSurfs = guiMesh.NumSurfaces();
    visCamera->visSurfs = (const VisSurf **)frame->MemAlloc(visCamera->maxVisSurfs * sizeof(VisSurf *));
    visCamera->numRenderPasses = 0;
    visCamera->renderPasses = (RenderPass *)frame->MemAlloc(VisCamera::MaxRenderPasses * sizeof(RenderPass));
    visCamera->origin = BE1::Vec3::origin;
    visCamera->axis = BE1::Mat3::identity;
    visCamera->outputRect = BE1::Rect(0, 0, screenWidth, screenHeight);
    visCamera->renderWidth = screenWidth;
    visCamera->renderHeight = screenHeight;
    visCamera->clearMethod = RenderCamera::ClearMethod::NoClear;
    visCamera->projectionPolicy = RHI::renderer->GetProjectionPolicy();
    visCamera->flags = RenderCamera::Flag::SkipPostProcess;
    //visCamera->time = BE1::PlatformTime::Milliseconds();
    //visCamera->orthogonal = true;
    //visCamera->zNear = 0.0f;
    //visCamera->zFar = 1.0f;
    visCamera->viewMatrix = BE1::Mat4::identity;
    visCamera->viewProjMatrix.SetOrthoRH(0, screenWidth, screenHeight, 0, -1.0, 1.0, true); // Y 를 뒤집는다.
    visCamera->invViewProjMatrix = visCamera->viewProjMatrix.Inverse();
    visCamera->projectionXY = BE1::Vec4(visCamera->viewProjMatrix[0][0], visCamera->viewProjMatrix[1][1], visCamera->viewProjMatrix[0][3], visCamera->viewProjMatrix[1][3]);
    visCamera->projectionZ = BE1::Vec2(visCamera->viewProjMatrix[2][2], visCamera->viewProjMatrix[2][3]);
    visCamera->projectionOrthogonal = true;
    visCamera->projectionInfiniteZFar = false;

    if (visCamera->projectionPolicy.IsZNegativeOneToOne()) {
        visCamera->ndcDepthScale = 2.0f;
        visCamera->ndcDepthBias = -1.0f;
    } else {
        visCamera->ndcDepthScale = 1.0f;
        visCamera->ndcDepthBias = 0.0f;
    }

    visCamera->prevViewProjMatrix = visCamera->viewProjMatrix;
    visCamera->prevInvViewProjMatrix = visCamera->invViewProjMatrix;
    visCamera->jitterNDC = BE1::Vec2::zero;
    visCamera->prevJitterNDC = BE1::Vec2::zero;
    visCamera->hiZCullViewProjMatrix = visCamera->viewProjMatrix;
    visCamera->worldAABB.Clear();
    visCamera->is2D = true;

    VisObject *visObject = frame->AllocVisObject();
    if (!visObject) {
        return;
    }

    visObject->worldMatrix.SetIdentity();
    visObject->prevWorldMatrix.SetIdentity();

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

        AddVisSurf(frame, visCamera, nullptr, visObject, guiSurf->material, subMesh, VisSurfFlag::None, 0);
    }

    guiMesh.Clear();

    renderPassBuilder.BuildGUI(frame, visCamera);

    frame->CmdDrawCamera(visCamera);

    viewCount++;
}
