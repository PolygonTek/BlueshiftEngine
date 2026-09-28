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

void RenderWorld::AddFrameSurf(RenderFrame *frame, FrameCamera *frameCamera, FrameLight *frameLight, FrameObject *frameObject, const Material *material, SubMesh *subMesh, FrameSurfFlag flags, uint8_t shadowCascadeMask, int lodLevel) {
    if (frameCamera->numFrameSurfs + 1 > frameCamera->maxFrameSurfs) {
        BE_WARNLOG("RenderWorld::AddFrameSurf: not enough available visible surfaces\n");
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
        BE_WARNLOG("RenderWorld::AddFrameSurf: failed to prepare static submesh buffers\n");
        return;
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

void RenderWorld::SetupPyramidBloom(RenderFrame *frame, RenderCamera *renderCamera, FrameCamera *frameCamera) {
    BloomResources &resources = renderCamera->GetBloomResources();
    if (resources.GetBloomMipCount() < 2) {
        return;
    }

    frameCamera->bloom.bloomDownTexture = resources.GetBloomDownTexture();
    frameCamera->bloom.bloomUpTexture = resources.GetBloomUpTexture();
    frameCamera->bloom.bloomMipCount = resources.GetBloomMipCount();

    for (uint32_t mipLevel = 0; mipLevel < RenderBloomDownMaxLevelCount; ++mipLevel) {
        frameCamera->bloom.bloomMipWidths[mipLevel] = resources.GetBloomMipWidths()[mipLevel];
        frameCamera->bloom.bloomMipHeights[mipLevel] = resources.GetBloomMipHeights()[mipLevel];
        frameCamera->bloom.bloomDownMipSRVs[mipLevel] = resources.GetBloomDownMipSRVs()[mipLevel];
        frameCamera->bloom.bloomDownMipUAVs[mipLevel] = resources.GetBloomDownMipUAVs()[mipLevel];
    }
    for (uint32_t mipLevel = 0; mipLevel < RenderBloomUpMaxLevelCount; ++mipLevel) {
        frameCamera->bloom.bloomUpMipSRVs[mipLevel] = resources.GetBloomUpMipSRVs()[mipLevel];
        frameCamera->bloom.bloomUpMipUAVs[mipLevel] = resources.GetBloomUpMipUAVs()[mipLevel];
    }

    frameCamera->bloom.outputTexture = resources.GetBloomUpTexture();
    frameCamera->bloom.outputViewID = resources.GetBloomUpMipSRVs()[0];
}

void RenderWorld::SetupConvolutionBloom(RenderFrame *frame, RenderCamera *renderCamera, FrameCamera *frameCamera) {
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

    frameCamera->bloom.fftBloomSize = fftBloomSize;
    frameCamera->bloom.fftBloomScreenSpatialBuffer = resources.GetFFTBloomScreenSpatialBuffer();
    frameCamera->bloom.fftBloomScreenSpectrumBuffer = resources.GetFFTBloomScreenSpectrumBuffer();
    frameCamera->bloom.fftBloomKernelSourceTexture = updateFFTBloomKernelBuffer ? fftBloomKernelSourceRHITexture : nullptr;
    frameCamera->bloom.fftBloomKernelSpatialBuffer = resources.GetFFTBloomKernelSpatialBuffer();
    frameCamera->bloom.fftBloomKernelSpectrumBuffer = resources.GetFFTBloomKernelSpectrumBuffer();
    frameCamera->bloom.fftBloomWorkBuffer = resources.GetFFTBloomWorkBuffer();
    frameCamera->bloom.fftBloomConvolutionOutputTexture = resources.GetFFTBloomConvolutionOutputTexture();
    frameCamera->bloom.fftBloomConvolutionOutputWidth = resources.GetFFTBloomConvolutionOutputWidth();
    frameCamera->bloom.fftBloomConvolutionOutputHeight = resources.GetFFTBloomConvolutionOutputHeight();
    frameCamera->bloom.fftBloomConvolutionOutputMipCount = resources.GetFFTBloomConvolutionOutputMipCount();
    for (uint32_t mipLevel = 0; mipLevel < RenderConvolutionBloomMaxMipLevelCount; ++mipLevel) {
        frameCamera->bloom.fftBloomConvolutionOutputMipSRVs[mipLevel] = resources.GetFFTBloomConvolutionOutputMipSRVs()[mipLevel];
        frameCamera->bloom.fftBloomConvolutionOutputMipUAVs[mipLevel] = resources.GetFFTBloomConvolutionOutputMipUAVs()[mipLevel];
    }
    frameCamera->bloom.fftBloomKernelRotation = resources.GetFFTBloomKernelRotation();
    frameCamera->bloom.fftBloomKernelScale = resources.GetFFTBloomKernelScale();
    frameCamera->bloom.fftBloomPaddingTrim = resources.GetFFTBloomPaddingTrim();
    frameCamera->bloom.fftBloomKernelNormalizedExtents = resources.GetFFTBloomKernelNormalizedExtents();
    frameCamera->bloom.fftBloomMappedContentRect = resources.GetFFTBloomMappedContentRect();
    frameCamera->bloom.fftBloomMappedKernelScale = resources.GetFFTBloomMappedKernelScale();
    frameCamera->bloom.fftBloomKernelInverseAverageLuminance = resources.GetFFTBloomKernelInverseAverageLuminance();
    frameCamera->bloom.fftBloomKernelDirty = updateFFTBloomKernelBuffer;

    if (updateFFTBloomKernelBuffer) {
        resources.MarkConvolutionBloomKernelClean();
    }

    frameCamera->bloom.outputTexture = resources.GetFFTBloomConvolutionOutputTexture();
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
    FrameCamera *frameCamera = frame->AllocFrameCamera();
    if (!frameCamera) {
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

    frameCamera->maxFrameSurfs = FrameCamera::MaxFrameSurfs;
    frameCamera->frameSurfs = (const FrameSurf **)frame->MemAlloc(frameCamera->maxFrameSurfs * sizeof(FrameSurf *));

    frameCamera->numRenderPasses = 0;
    frameCamera->renderPasses = (RenderPass *)frame->MemAlloc(FrameCamera::MaxRenderPasses * sizeof(RenderPass));

    frameCamera->origin = cameraDesc.origin;
    frameCamera->axis = cameraDesc.axis;

    if (hasEnvironmentLight) {
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
    frameCamera->projectionZ = BE1::Vec2(renderCamera->projMatrix[2][2], renderCamera->projMatrix[2][3]);
    frameCamera->projectionOrthogonal = cameraDesc.orthogonal;
    frameCamera->projectionInfiniteZFar = cameraDesc.infiniteZFar;

    if (frameCamera->projectionOrthogonal) {
        frameCamera->projectionXY = BE1::Vec4(renderCamera->projMatrix[0][0], renderCamera->projMatrix[1][1], renderCamera->projMatrix[0][3], renderCamera->projMatrix[1][3]);
        frameCamera->viewBox = renderCamera->GetBox();
    } else {
        frameCamera->projectionXY = BE1::Vec4(renderCamera->projMatrix[0][0], renderCamera->projMatrix[1][1], -renderCamera->projMatrix[0][2], -renderCamera->projMatrix[1][2]);
        frameCamera->viewFrustum = renderCamera->GetFrustum();
    }

    if (frameCamera->projectionPolicy.IsZNegativeOneToOne()) {
        frameCamera->ndcDepthScale = 2.0f;
        frameCamera->ndcDepthBias = -1.0f;
    } else {
        frameCamera->ndcDepthScale = 1.0f;
        frameCamera->ndcDepthBias = 0.0f;
    }

    frameCamera->flags = cameraDesc.flags;
    frameCamera->needNormalOutput = frameSettings.ssr;
    frameCamera->needVelocityOutput = useTAA;

    frameCamera->colorTexture = renderCamera->GetColorTexture();
    frameCamera->depthTexture = renderCamera->GetDepthTexture();
    frameCamera->normalTexture = renderCamera->GetNormalTexture();
    frameCamera->velocityTexture = renderCamera->GetVelocityTexture();

    frameCamera->resolvedColorTexture = renderCamera->GetResolvedColorTexture();
    frameCamera->resolvedDepthTexture = renderCamera->GetResolvedDepthTexture();
    frameCamera->resolvedNormalTexture = renderCamera->GetResolvedNormalTexture();
    frameCamera->resolvedVelocityTexture = renderCamera->GetResolvedVelocityTexture();

    frameCamera->sceneSnapshotDownsamplePSO = useHalfResSnapshot ? sceneSnapshotDownsamplePSO : nullptr;
    frameCamera->snapshotColorTexture = renderCamera->GetSnapshotColorTexture();
    frameCamera->snapshotDepthTexture = renderCamera->GetSnapshotDepthTexture();

    FrameScreenSpaceAO &screenSpaceAO = frameCamera->ao;
    screenSpaceAO.textureSet = renderCamera->screenSpaceAOResources.GetTextureSet();
    screenSpaceAO.depthTexture = useMSAA ? frameCamera->resolvedDepthTexture : frameCamera->depthTexture;
    screenSpaceAO.normalTexture = useMSAA ? frameCamera->resolvedNormalTexture : frameCamera->normalTexture;
    screenSpaceAO.projectionPolicy = frameCamera->projectionPolicy;
    screenSpaceAO.mode = frameSettings.aoMode;
    screenSpaceAO.temporalNoise = frameSettings.aoTemporalNoise && !debugAO;

    frameCamera->applyAO = useAO && ScreenSpaceAO::HasRequiredResources(screenSpaceAO) && (!useMSAA || frameCamera->normalTexture);
    frameCamera->needDepthNormalOutput = frameCamera->applyAO;

    frameCamera->generateScreenSpaceDepthPyramid = requestScreenSpaceDepthPyramidGeneration;

    if (frameCamera->generateScreenSpaceDepthPyramid) {
        FrameScreenSpaceDepthPyramid &depthPyramid = frameCamera->screenSpaceDepthPyramid;
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

    frameCamera->traceSSR = requestSSRTrace;
    frameCamera->ssrTraceTexture = renderCamera->GetSSRTraceTexture();

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
        // 초기 LUT upload request 는 backend frame 실행 전에 처리되므로 Pending 상태의 RHI 포인터도 프레임에 전달할 수 있다.
        Texture *colorGradingLUTTexture = frame->RetainTexture(renderSystem->GetColorGradingLUTTexture());
        frameCamera->colorGradingLUTTexture = colorGradingLUTTexture ? colorGradingLUTTexture->GetRHITexture() : nullptr;

        Texture *bloomDirtMaskTexture = frame->RetainTexture(renderCamera->GetBloomResources().GetBloomDirtMaskTexture());
        frameCamera->bloom.bloomDirtMaskTexture = bloomDirtMaskTexture ? bloomDirtMaskTexture->GetRHITexture() : nullptr;

        if (bloomMode == RenderBloomMode::Pyramid) {
            SetupPyramidBloom(frame, renderCamera, frameCamera);
        } else if (bloomMode == RenderBloomMode::Convolution) {
            SetupConvolutionBloom(frame, renderCamera, frameCamera);
        }
    }

    frameCamera->worldAABB.Clear();
    frameCamera->is2D = false;
    frameCamera->viewMatrix = renderCamera->viewMatrix;

    const BE1::Vec2 jitterToNDCScale(2.0f / static_cast<float>(renderWidth), -2.0f / static_cast<float>(renderHeight));

    // Rasterization 은 jittered 행렬을 사용하고, velocity 계산용 unjittered 행렬은 backend 에서 jitter 를 제거해 복원한다.
    frameCamera->viewProjMatrix = renderCamera->jitteredViewProjMatrix;
    frameCamera->invViewProjMatrix = renderCamera->jitteredInvViewProjMatrix;
    frameCamera->jitterNDC = renderCamera->currentJitter * jitterToNDCScale;

    const bool usePreviousMatrix = temporalHistoryEnabled && renderCamera->previousMatrixValid && !renderCamera->temporalHistoryInvalidated;

    if (usePreviousMatrix) {
        frameCamera->prevViewProjMatrix = renderCamera->previousJitteredViewProjMatrix;
        frameCamera->prevInvViewProjMatrix = renderCamera->previousJitteredInvViewProjMatrix;
        frameCamera->prevJitterNDC = renderCamera->previousJitter * jitterToNDCScale;
    } else {
        frameCamera->prevViewProjMatrix = renderCamera->jitteredViewProjMatrix;
        frameCamera->prevInvViewProjMatrix = renderCamera->jitteredInvViewProjMatrix;
        frameCamera->prevJitterNDC = frameCamera->jitterNDC;
    }

    frameCamera->taaResolve = useTAA;

    // freeze 가 되면 prevViewProjMatrix 를 유지하기 위해서 hiZCullViewProjMatrix 에 복사한다.
    if (!frameSettings.hiZOcclusionCullingFreeze || !hiZResources.IsHistoryValid()) {
        renderCamera->hiZCullViewProjMatrix = frameCamera->prevViewProjMatrix;
    }
    frameCamera->hiZCullViewProjMatrix = renderCamera->hiZCullViewProjMatrix;

    DrawCamera(frame, renderCamera, frameCamera);

    if (temporalHistoryEnabled) {
        renderCamera->previousViewProjMatrix = renderCamera->viewProjMatrix;
        renderCamera->previousInvViewProjMatrix = renderCamera->invViewProjMatrix;
        renderCamera->previousJitteredViewProjMatrix = renderCamera->jitteredViewProjMatrix;
        renderCamera->previousJitteredInvViewProjMatrix = renderCamera->jitteredInvViewProjMatrix;
        renderCamera->previousMatrixValid = true;
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

void RenderWorld::DrawCamera(RenderFrame *frame, const RenderCamera *renderCamera, FrameCamera *frameCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::DrawCamera");

    // RenderObject 중에서 컬링한 오브젝트를 프레임 데이터에 FrameObject 로 등록한다.
    FindFrameObjects(frame, renderCamera, frameCamera);

    // RenderLight 중에서 컬링한 라이트를 프레임 데이터에 FrameLight 로 등록한다.
    FindFrameLights(frame, renderCamera, frameCamera);

    // 우선순위에 따라 local shadow 타일을 할당하고 카메라 전용 snapshot 에 등록한다.
    SetupShadowAtlas(frame, frameCamera);

    // 각 CSM split 의 receiver 영역으로 directional shadow 투영을 준비하고 caster 를 수집한다.
    SetupCascadedShadow(frame, renderCamera, frameCamera);

    // frameObjects 중에서 mesh surface 들을 frameCamera->frameSurfs 에 추가한다.
    AddMeshes(frame, renderCamera, frameCamera);

    // visibleLights 들로 GPU light culling 데이터를 구성한다.
    BuildLightCullingData(frame, renderCamera, frameCamera);

    // frameObjects 중에서 text mesh 를 frameCamera->frameSurfs 에 추가한다.
    AddTextMeshes(frame, renderCamera, frameCamera);

    // Add drawing surface of skybox.
    AddSkyBoxMeshes(frame, renderCamera, frameCamera);

    // Ocean surface 와 query 정보를 프레임 데이터에 등록한다.
    if (hasOceanSurface) {
        frame->RegisterFrameOceanSurface(oceanSurface.desc.simulation, oceanSurface.desc.surface, oceanSurface.revision, oceanSurface.queries.Ptr(), static_cast<uint32_t>(oceanSurface.queries.Count()));
    }

    // CSM 렌더 패스를 구성한다.
    if (frameCamera->frameCascadedShadow) {
        const bool allowAsyncPSOBuild = !BE1::HasFlag(frameCamera->flags, RenderCamera::Flag::ForceSyncPassPSOBuild);
        for (uint32_t cascadeIndex = 0; cascadeIndex < frameCamera->frameCascadedShadow->cascadeCount; cascadeIndex++) {
            renderPassBuilder.BuildCascadedShadow(frame, frameCamera->frameCascadedShadow, cascadeIndex, frameCamera->frameSurfs, frameCamera->numFrameSurfs, allowAsyncPSOBuild);
        }
    }

    // 메인 렌더 패스를 구성한다.
    renderPassBuilder.BuildMain(frame, frameCamera);

    // Post-Opaque 단계에서 그릴 ocean surface pipeline 을 준비한다.
    PrepareOceanSurfacePipeline(frameCamera);

    // 카메라의 debug draw 데이터를 구성한다.
    BuildDebugDraw(frame, frameCamera);

    // 렌더링 커맨드에 frameCamera 를 기록한다.
    frame->CmdDrawCamera(frameCamera);

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

void RenderWorld::SetupCascadedShadow(RenderFrame *frame, const RenderCamera *renderCamera, FrameCamera *frameCamera) {
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
    const uint32_t shadowMapBorderTexels = CalcShadowMapBorderTexels(shadowMapResolution, frameSettings.shadowMapQuality);
    constexpr float referenceUsableResolution = static_cast<float>(RenderFrameSettings::ShadowMapFilterReferenceResolution - 2 * RenderFrameSettings::ShadowMapFilterReferenceBorderTexels);
    const float usableShadowResolution = static_cast<float>(shadowMapResolution - 2 * shadowMapBorderTexels);
    const float filterResolutionScale = usableShadowResolution / referenceUsableResolution;

    FrameCascadedShadow *cascadedShadow = new (frame->MemAlloc(sizeof(FrameCascadedShadow))) FrameCascadedShadow();
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

    frameCamera->frameCascadedShadow = cascadedShadow;
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
    if (frameCamera->needNormalOutput) {
        featureSet.EnableFeature(ShaderFeature::OutputNormal);
    }
    if (frameCamera->needVelocityOutput) {
        featureSet.EnableFeature(ShaderFeature::OutputVelocity);
    }
    GraphicsPipelineDesc graphicsPipelineDesc;
    graphicsPipelineDesc.inputLayout = nullptr;
    graphicsPipelineDesc.rasterizerStateType = surfaceSettings.wireframe ? RHI::RasterizerStateType::Wire : RHI::RasterizerStateType::SolidTwoSided;
    graphicsPipelineDesc.depthStencilStateType = frameCamera->projectionPolicy.reverseZ ? RHI::DepthStencilStateType::GEqualWrite : RHI::DepthStencilStateType::LEqualWrite;
    graphicsPipelineDesc.blendStateType = RHI::BlendStateType::Disabled;
    graphicsPipelineDesc.primitiveTopology = RHI::PrimitiveTopology::TriangleList;
    RenderContext::activeContext->GetMainRenderDest(graphicsPipelineDesc.renderDest, frameCamera->needNormalOutput, frameCamera->needVelocityOutput);

    const ShaderPermutationKey permutationKey = shaderTemplate->RegisterPermutation({}, featureSet);
    const bool allowAsyncPSOBuild = !BE1::HasFlag(frameCamera->flags, RenderCamera::Flag::ForceSyncPassPSOBuild);
    const SharedPtr<ShaderPipeline> shaderPipeline = shaderPipelineCache.GetGraphicsPipeline(shaderTemplate, permutationKey, graphicsPipelineDesc, allowAsyncPSOBuild);
    frameCamera->oceanSurfacePSO = ShaderPipeline::GetReadyPSO(shaderPipeline);
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

    if (renderLight->desc.type == RenderLight::Type::Point) {
        frameLight->viewProjScaleBiasMatrix[0] = renderLight->viewProjScaleBiasMatrix[0];
        frameLight->viewProjScaleBiasMatrix[1] = renderLight->viewProjScaleBiasMatrix[1];
    } else if (renderLight->desc.type == RenderLight::Type::Spot) {
        frameLight->viewProjScaleBiasMatrix = renderLight->viewProjScaleBiasMatrix;

        FrameSpotLightData *spotLightData = frame->AllocFrameSpotLightData();
        spotLightData->fallOffMatrix = renderLight->fallOffMatrix;
        frameLight->spotLightData = spotLightData;

        frameLight->axis = renderLight->desc.axis;
    } else if (renderLight->desc.type == RenderLight::Type::BoxDir) {
        frameLight->viewProjScaleBiasMatrix = renderLight->viewProjScaleBiasMatrix;
        frameLight->axis = renderLight->desc.axis;
    } else if (renderLight->desc.type == RenderLight::Type::Directional) {
        frameLight->axis = renderLight->desc.axis;
    }
    frameLight->origin = renderLight->desc.origin;
    frameLight->size = renderLight->desc.size;
    frameLight->lightColor = renderLight->desc.linearColor * renderLight->desc.intensity;
    frameLight->zNear = renderLight->desc.zNear;
    frameLight->fallOffExponent = renderLight->desc.fallOffExponent;
    frameLight->shadowOffsetFactor = renderLight->desc.shadowOffsetFactor;
    frameLight->shadowOffsetUnits = renderLight->desc.shadowOffsetUnits;
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
    const bool staticOnly = BE1::HasFlag(cameraDesc.flags, RenderCamera::Flag::StaticOnly);

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

        if (staticOnly && !BE1::HasFlag(renderObject->desc.flags, RenderObject::Flag::Static)) {
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

void RenderWorld::UpdateSkinningJointCaches(RenderFrame *frame, const FrameSkinnedMesh **objects, uint32_t numObjects, uint32_t numSkinningJoints) {
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

void RenderWorld::UpdateSkinningJointCachesWithoutTask(const FrameSkinnedMesh **objects, uint32_t numObjects) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::UpdateSkinningJointCachesWithoutTask");

    for (uint32_t i = 0; i < numObjects; ++i) {
        const FrameSkinnedMesh *frameSkinnedMesh = objects[i];
        frameSkinnedMesh->meshInstance->UpdateSkinningJointCache(frameSkinnedMesh->skeleton);
    }
}

void RenderWorld::UpdateSkinningJointCachesByTask(RenderWorld::UpdateSkinningJointCachesTaskDesc *taskDesc) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::UpdateSkinningJointCachesByTask");

    for (uint32_t i = 0; i < taskDesc->numSkinnedMeshObjects; ++i) {
        const FrameSkinnedMesh *frameSkinnedMesh = taskDesc->skinnedMeshObjects[i];
        frameSkinnedMesh->meshInstance->UpdateSkinningJointCache(frameSkinnedMesh->skeleton);
    }
}

void RenderWorld::UpdateSkinningJointCachesByTaskFunction(void *data) {
    RenderWorld::UpdateSkinningJointCachesTaskDesc *taskDesc = reinterpret_cast<RenderWorld::UpdateSkinningJointCachesTaskDesc *>(data);
    taskDesc->renderWorld->UpdateSkinningJointCachesByTask(taskDesc);
}

void RenderWorld::UpdateSkinningJointCachesWithTask(const FrameSkinnedMesh **objects, uint32_t numObjects, uint32_t numSkinningJoints, uint32_t numTasks) {
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

int RenderWorld::CountSkinningUploadJoints(const RenderFrame::ChunkRange<FrameSkinnedMesh> &frameSkinnedMeshes, bool needVelocityOutput, uint64_t currentFrameSerial) const {
    uint32_t numTotalUploadJoints = 0;

    for (const FrameSkinnedMesh &frameSkinnedMesh : frameSkinnedMeshes) {
        if (!frameSkinnedMesh.space->IsRenderable()) {
            continue;
        }

        const SkinningJointCache *skinningJointCache = frameSkinnedMesh.meshInstance->GetSkinningJointCache();
        if (!skinningJointCache) {
            continue;
        }

        // 캐시가 최신 상태가 아니라면 업로드를 하지 않는다.
        if (!skinningJointCache->IsUpToDate(frameSkinnedMesh.skeleton, frameSkinnedMesh.meshInstance->GetJointMatsSerial())) {
            continue;
        }

        const uint32_t numJoints = static_cast<uint32_t>(skinningJointCache->NumJoints());
        const bool needPreviousPose = needVelocityOutput && BE1::HasFlag(frameSkinnedMesh.space->flags, FrameObject::Flag::CameraVisible);
        const uint32_t numPoses = needPreviousPose && skinningJointCache->HasPreviousPose(currentFrameSerial) ? 2u : 1u;
        const uint32_t numUploadJoints = numJoints * numPoses;

        if (numUploadJoints > (UINT32_MAX / sizeof(ShaderSkinningJoint) - numTotalUploadJoints)) {
            return -1;
        }

        numTotalUploadJoints += numUploadJoints;
    }
    return static_cast<int>(numTotalUploadJoints);
}

void RenderWorld::BuildSkinningJointBuffer(RenderFrame *frame, FrameCamera *frameCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::BuildSkinningJointBuffer");

    if (frame->NumFrameSkinnedMeshes() == frameCamera->firstFrameSkinnedMeshIndex) {
        return;
    }

    skinningJointCacheUpdateObjects.SetCount(0, false);

    uint32_t numTotalJoints = 0;

    const auto frameSkinnedMeshes = frame->GetFrameSkinnedMeshes(frameCamera->firstFrameSkinnedMeshIndex);
    const uint64_t currentFrameSerial = frame->GetFrameSerial();

    for (const FrameSkinnedMesh &frameSkinnedMesh : frameSkinnedMeshes) {
        if (!frameSkinnedMesh.space->IsRenderable()) {
            continue;
        }

        if (!frameSkinnedMesh.skeleton->IsReady() || !frameSkinnedMesh.meshInstance->EnsureCurrent()) {
            continue;
        }

        SkinningJointCache *skinningJointCache = frameSkinnedMesh.meshInstance->GetSkinningJointCache();
        if (!skinningJointCache) {
            continue;
        }

        // 캐시의 joint pose 나 skeleton 이 변경되었다면 업데이트를 예약한다.
        // 캐시 업데이트가 예약된 상태이거나 이미 업데이트된 상태라면 건너뛴다.
        if (!skinningJointCache->PrepareUpdate(frameSkinnedMesh.skeleton, frameSkinnedMesh.meshInstance->GetJointMatsSerial(), currentFrameSerial)) {
            continue;
        }

        skinningJointCacheUpdateObjects.Alloc() = &frameSkinnedMesh;

        numTotalJoints += skinningJointCache->NumJoints();
    }

    // 앞선 카메라나 프레임에서 이미 갱신된 캐시도 현재 카메라의 jointBuffer 에 업로드해야 한다.
    UpdateSkinningJointCaches(frame, skinningJointCacheUpdateObjects.Ptr(), skinningJointCacheUpdateObjects.Count(), numTotalJoints);

    // jointBuffer 에 업로드할 전체 joint pose 개수를 계산한다. 카메라 velocity 에 필요한 이전 pose 만 포함한다.
    const int numTotalUploadJoints = CountSkinningUploadJoints(frameSkinnedMeshes, frameCamera->needVelocityOutput, currentFrameSerial);
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
    for (FrameSkinnedMesh &frameSkinnedMesh : frameSkinnedMeshes) {
        if (!frameSkinnedMesh.space->IsRenderable()) {
            continue;
        }

        const SkinningJointCache *skinningJointCache = frameSkinnedMesh.meshInstance->GetSkinningJointCache();
        if (!skinningJointCache) {
            continue;
        }

        if (!skinningJointCache->IsUpToDate(frameSkinnedMesh.skeleton, frameSkinnedMesh.meshInstance->GetJointMatsSerial())) {
            continue;
        }

        const uint32_t numJoints = static_cast<uint32_t>(skinningJointCache->NumJoints());
        const uint32_t byteSize = numJoints * sizeof(ShaderSkinningJoint);

        BE1::simdProcessor->Memcpy(dst + jointOffset, skinningJointCache->GetSkinningJoints(), byteSize);
        frameSkinnedMesh.skinningJointOffset = jointOffset;
        jointOffset += numJoints;

        frameSkinnedMesh.prevSkinningJointOffset = frameSkinnedMesh.skinningJointOffset;

        const bool needPreviousPose = frameCamera->needVelocityOutput && BE1::HasFlag(frameSkinnedMesh.space->flags, FrameObject::Flag::CameraVisible);
        if (needPreviousPose && skinningJointCache->HasPreviousPose(currentFrameSerial)) {
            BE1::simdProcessor->Memcpy(dst + jointOffset, skinningJointCache->GetPrevSkinningJoints(), byteSize);
            frameSkinnedMesh.prevSkinningJointOffset = jointOffset;
            jointOffset += numJoints;
        }
    }

    assert(jointOffset == static_cast<uint32_t>(numTotalUploadJoints));
    frameCamera->skinningJointBuffer = jointBuffer;
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

static BE_INLINE void CullMeshSurface(const BE1::AABB &localAABB, const BE1::Mat3x4 &worldMatrix, const RenderCamera *renderCamera, const FrameCascadedShadow *cascadedShadow, bool cullCamera, bool &surfaceCameraVisible, uint8_t &surfaceShadowCascadeMask) {
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

void RenderWorld::AddMeshes(RenderFrame *frame, const RenderCamera *renderCamera, FrameCamera *frameCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::AddMeshes");

    // 현재 카메라에서 새로 등록된 FrameObject 가 없으면 메시 처리를 생략한다.
    if (frame->NumFrameObjects() == frameCamera->firstFrameObjectIndex) {
        return;
    }

    BuildSkinningJointBuffer(frame, frameCamera);

    const bool skipViewCulling = frame->GetRenderFrameSettings().skipViewCulling;
    const FrameCascadedShadow *cascadedShadow = frameCamera->frameCascadedShadow;

    for (FrameObject &frameObject : frame->GetFrameObjects(frameCamera->firstFrameObjectIndex)) {
        const bool objectCameraVisible = BE1::HasFlag(frameObject.flags, FrameObject::Flag::CameraVisible);
        const uint8_t objectShadowCascadeMask = cascadedShadow ? frameObject.shadowCascadeMask : 0;
        if (!objectCameraVisible && objectShadowCascadeMask == 0) {
            continue;
        }

        MeshInstance *meshInstance = frameObject.meshInstance;
        if (!meshInstance || !meshInstance->EnsureCurrent()) {
            continue;
        }

        const bool skinned = meshInstance->IsSkinned();

        // Pose 준비에 실패한 skinned mesh 를 bind pose 상태로 등록하지 않는다.
        if (skinned && (!frameObject.skinnedMesh ||
            frameObject.skinnedMesh->skinningJointOffset == ShaderSkinningJointOffsetInvalid)) {
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
            if (meshSurf->materialIndex >= 0 && static_cast<uint32_t>(meshSurf->materialIndex) < frameObject.numMaterials) {
                material = frameObject.GetMaterials()[meshSurf->materialIndex];
            }

            const bool castsShadow = !BE1::HasFlag(material->GetFlags(), Material::Flag::NoShadow) && material->GetRenderingMode() != Material::RenderingMode::Blend;

            bool surfaceCameraVisible = objectCameraVisible;
            uint8_t surfaceShadowCascadeMask = castsShadow ? objectShadowCascadeMask : 0;
            if (!surfaceCameraVisible && surfaceShadowCascadeMask == 0) {
                continue;
            }

            if (cullSurfaces && (cullCameraSurfaces || surfaceShadowCascadeMask != 0)) {
                CullMeshSurface(meshSurf->subMesh->GetBoundingBox(), frameObject.worldMatrix, renderCamera, cascadedShadow, cullCameraSurfaces, surfaceCameraVisible, surfaceShadowCascadeMask);
            }

            if (!surfaceCameraVisible && surfaceShadowCascadeMask == 0) {
                continue;
            }

            const FrameSurf::Flag flags = surfaceCameraVisible ? FrameSurf::Flag::CameraVisible : FrameSurf::Flag::None;

            // 카메라에 보이는 surface 는 카메라 기준, shadow 전용 surface 는 가장 정밀한 관련 cascade 기준 LOD 를 사용한다.
            // 한 surface 가 여러 cascade 에 투영되어도 같은 LOD 를 공유하여 blend 영역의 caster silhouette 를 일치시킨다.
            const int lodLevel = SelectMeshLOD(meshSurf->subMesh, frameObject.meshLOD, frameObject.squaredLODErrorLimit);
            AddFrameSurf(frame, frameCamera, nullptr, &frameObject, material, meshSurf->subMesh, flags, surfaceShadowCascadeMask, lodLevel);
        }
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

            AddFrameSurf(frame, frameCamera, nullptr, frameObject, guiSurf->material, subMesh, FrameSurf::Flag::CameraVisible, 0);
        }
    }
}

void RenderWorld::AddSkyBoxMeshes(RenderFrame *frame, const RenderCamera *renderCamera, FrameCamera *frameCamera) {
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

    FrameObject *frameObject = frame->AllocFrameObject();
    if (!frameObject) {
        return;
    }

    frameObject->worldMatrix.SetTRS(renderCamera->GetDesc().origin, BE1::Mat3::identity, BE1::Vec3(10, 10, 10));
    frameObject->prevWorldMatrix = frameObject->worldMatrix;
    frameObject->flags |= FrameObject::Flag::CameraVisible;
    Material *material = frame->RetainMaterial(skyboxMaterial);

    AddFrameSurf(frame, frameCamera, nullptr, frameObject, material, meshSurf->subMesh, FrameSurf::Flag::CameraVisible | FrameSurf::Flag::SkipSelection | FrameSurf::Flag::Skybox, 0);
}

void RenderWorld::FindFrameLights(RenderFrame *frame, const RenderCamera *renderCamera, FrameCamera *frameCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::FindFrameLights");

    localShadowCandidates.SetCount(0, false);

    const RenderFrameSettings &frameSettings = frame->GetRenderFrameSettings();
    const bool skipViewCulling = frameSettings.skipViewCulling;
    const bool localShadowsEnabled = frameSettings.shadows && frameSettings.shadowAtlasMaxViews > 0;

    const RenderCamera::Desc &cameraDesc = renderCamera->GetDesc();
    const bool staticOnly = BE1::HasFlag(cameraDesc.flags, RenderCamera::Flag::StaticOnly);
    const BE1::Vec3 &viewZAxis = cameraDesc.axis[EngineAxis::Forward];
    const float viewZOffset = -viewZAxis.Dot(cameraDesc.origin);

    // 현재 카메라의 local light 참조가 시작되는 프레임 공용 ShaderLightRef 테이블의 오프셋을 기록한다.
    frameCamera->lightCulling.lightRefTableOffset = frame->NumFrameLocalLights();

    // RenderWorld 에 등록된 renderLights 개수만큼 미리 frameCamera->frameLocalLights 를 할당
    frameCamera->maxFrameLocalLights = static_cast<uint32_t>(renderLights.Count());
    if (frameCamera->maxFrameLocalLights > 0) {
        frameCamera->frameLocalLights = (const FrameLight **)frame->MemAlloc(frameCamera->maxFrameLocalLights * sizeof(frameCamera->frameLocalLights[0]));
    }

    // 등록된 모든 RenderLight 들을 프레임 데이터에 FrameLight 로 등록한다.
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

            const uint32_t projectedSize = ComputeLightTileRange(frameCamera, screenClipRect, frameLight->packedTileMin, frameLight->packedTileMax);
            if (localShadowsEnabled && BE1::HasFlag(frameLight->flags, RenderLight::Flag::CastShadows)) {
                if (projectedSize > 0 && (frameLight->type == RenderLight::Type::Spot || frameLight->type == RenderLight::Type::Point)) {
                    // 광원 영향 영역으로 요청 해상도를 근사한다.
                    const uint32_t clampedSize = BE1::Clamp(projectedSize, RenderFrameSettings::ShadowAtlasMinTileResolution, frameSettings.shadowAtlasMaxTileResolution);
                    frameLight->shadowRequestedResolution = BE1::Math::RoundUpPowerOfTwo(clampedSize);
                    localShadowCandidates.Alloc() = frameLight;
                }
            }

            frameCamera->frameLocalLights[frameCamera->numFrameLocalLights++] = frameLight;
        }

        renderLight->visibleFrameIndex = frameIndex;
    }
}

void RenderWorld::SetupShadowAtlas(RenderFrame *frame, FrameCamera *frameCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::SetupShadowAtlas");

    ShadowAtlas &shadowAtlas = RenderContext::activeContext->GetShadowAtlas();

    // 현재는 카메라마다 다시 배치한다. 이전 카메라의 좌표는 snapshot 에 복사되어 allocator 와 독립적이다.
    if (shadowAtlas.GetAllocedTileCount() > 0) {
        shadowAtlas.ResetAllocations();
    }

    frameCamera->frameShadowAtlas = frame->AllocFrameShadowAtlas(shadowAtlas);
    if (!frameCamera->frameShadowAtlas || localShadowCandidates.Count() == 0) {
        return;
    }

    FrameShadowAtlas &frameShadowAtlas = *frameCamera->frameShadowAtlas;

    if (localShadowCandidates.Count() > 1) {
        // 요청 해상도가 큰 광원부터 처리한다. 같으면 가까운 영향 영역, 그다음 등록 인덱스 순으로 고정한다.
        // 조명용 frameLocalLights 는 이후 Z-bin 순으로 정렬되므로 shadow 후보 목록만 정렬한다.
        std::sort(localShadowCandidates.Ptr(), localShadowCandidates.Ptr() + localShadowCandidates.Count(), [](const FrameLight *lhs, const FrameLight *rhs) {
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

    for (FrameLight *light : localShadowCandidates) {
        // Point 는 후보에 포함하되, 6 면의 투영/할당 연결 전까지 view 예산을 소비하지 않는다.
        if (light->type != RenderLight::Type::Spot) {
            continue;
        }

        if (frameShadowAtlas.numShadowViews >= frameShadowAtlas.maxShadowViews) {
            break;
        }

        ShadowAtlas::Tile tile;
        // 리소스 재설정이 실패해 이전 atlas 를 사용하는 경우에도 실제 allocator 의 최대 해상도를 따른다.
        const uint32_t requestedResolution = BE1::Min(light->shadowRequestedResolution, shadowAtlas.GetMaxTileResolution());
        if (!shadowAtlas.Allocate(requestedResolution, 1, &tile)) {
            continue;
        }

        FrameLocalShadow *shadow = frame->AllocFrameLocalShadow(frameShadowAtlas, *light);
        if (!shadow) {
            // Snapshot 등록 실패 시 타일과 view 예산을 남기지 않는다.
            const bool freed = shadowAtlas.Free(tile.handle);
            assert(freed);
            (void)freed;
            break;
        }

        shadow->requestedResolution = light->shadowRequestedResolution;
        shadow->resolution = tile.resolution;

        frameShadowAtlas.shadowViews[shadow->firstView].tileRect = BE1::Rect(tile.x, tile.y, tile.resolution, tile.resolution);
    }
}

static BE_INLINE uint16_t PackLightTileCoord(uint32_t x, uint32_t y) {
    assert(x <= FrameLight::TileCoordMask);
    assert(y <= FrameLight::TileCoordMask);
    return static_cast<uint16_t>(x | (y << FrameLight::TileCoordBitCount));
}

// Light-culling 타일 범위를 계산하고, 렌더 픽셀 기준 광원 화면 영역의 긴 변 길이를 반환한다.
uint32_t RenderWorld::ComputeLightTileRange(const FrameCamera *frameCamera, const BE1::Rect &screenClipRect, uint16_t &packedTileMin, uint16_t &packedTileMax) {
    if (frameCamera->outputRect.w <= 0 || frameCamera->outputRect.h <= 0 || frameCamera->renderWidth == 0 || frameCamera->renderHeight == 0) {
        assert(0);
        packedTileMin = 0;
        packedTileMax = 0;
        return 0;
    }

    const int32_t outputMinX = frameCamera->outputRect.x;
    const int32_t outputMinY = frameCamera->outputRect.y;
    const int32_t outputMaxX = outputMinX + frameCamera->outputRect.w;
    const int32_t outputMaxY = outputMinY + frameCamera->outputRect.h;

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

    const uint32_t outputWidth = static_cast<uint32_t>(frameCamera->outputRect.w);
    const uint32_t outputHeight = static_cast<uint32_t>(frameCamera->outputRect.h);
    const uint32_t renderWidth = frameCamera->renderWidth;
    const uint32_t renderHeight = frameCamera->renderHeight;

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

void RenderWorld::BuildLightCullingData(RenderFrame *frame, const RenderCamera *renderCamera, FrameCamera *frameCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::BuildLightCullingData");

    // 현재 카메라의 visible lights 를 minmax z 에 따라 정렬하고, z-bins 를 작성한다.
    BuildLightZBins(frame, renderCamera, frameCamera);

    if (frameCamera->lightCulling.zBins) {
        // 현재 카메라의 Z-bin 이 시작되는 프레임 공용 light Z-bin 테이블을 예약한다.
        frameCamera->lightCulling.zBinTableOffset = frame->ReserveLightCullingZBins();
    }

    if (frameCamera->numFrameLocalLights > 0) {
        assert(frameCamera->renderWidth > 0 && frameCamera->renderHeight > 0);

        // 렌더링 화면에 필요한 타일 개수
        frameCamera->lightCulling.tileCountX = (frameCamera->renderWidth + ShaderLightTileSize - 1) / ShaderLightTileSize;
        frameCamera->lightCulling.tileCountY = (frameCamera->renderHeight + ShaderLightTileSize - 1) / ShaderLightTileSize;

        // 각 타일이 필요한 비트 마스크 워드의 개수
        frameCamera->lightCulling.wordsPerTile = (frameCamera->numFrameLocalLights + ShaderLightWordBits - 1) / ShaderLightWordBits;

        // 전체 타일에 필요한 비트 마스크 워드의 개수
        const uint64_t numLightCullingWords = static_cast<uint64_t>(frameCamera->lightCulling.tileCountX) * frameCamera->lightCulling.tileCountY * frameCamera->lightCulling.wordsPerTile;

        if (numLightCullingWords <= UINT32_MAX) {
            // 현재 카메라의 타일 마스크 워드가 시작되는 프레임 공용 테이블을 예약한다.
            frameCamera->lightCulling.tileWordTableOffset = frame->ReserveLightCullingWords(static_cast<uint32_t>(numLightCullingWords));
        }
    }
}

static BE_INLINE uint32_t PackLightZBin(uint32_t minLightIndex, uint32_t maxLightIndex) {
    assert(minLightIndex <= ShaderLightZBinIndexMask);
    assert(maxLightIndex <= ShaderLightZBinIndexMask);
    return (maxLightIndex << 16) | minLightIndex;
}

void RenderWorld::BuildLightZBins(RenderFrame *frame, const RenderCamera *renderCamera, FrameCamera *frameCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::BuildLightZBins");

    const RenderCamera::Desc &cameraDesc = renderCamera->GetDesc();

    const uint32_t numFrameLocalLights = frameCamera->numFrameLocalLights;
    if (numFrameLocalLights == 0) {
        return;
    }

    if (numFrameLocalLights > 1) {
        // viewZMin, viewZMax 순으로 오름차순 정렬하고, 두 값이 같으면 index 가 작은 라이트를 먼저 배치해 순서를 고정한다.
        std::sort(frameCamera->frameLocalLights, frameCamera->frameLocalLights + numFrameLocalLights, [](const FrameLight *lhs, const FrameLight *rhs) {
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
    if (numFrameLocalLights > ShaderLightZBinMaxLightCount) {
        return;
    }

    if (cameraDesc.zFar <= cameraDesc.zNear) {
        return;
    }

    const float viewZRange = cameraDesc.zFar - cameraDesc.zNear;
    const float zBinSize = viewZRange / static_cast<float>(ShaderLightZBinCount);

    // linear view Z 를 Z-bin index 로 변환하기 위한 scale 과 bias 를 미리 계산한다.
    frameCamera->lightCulling.zBinScale = static_cast<float>(ShaderLightZBinCount) / viewZRange;
    frameCamera->lightCulling.zBinBias = -cameraDesc.zNear * frameCamera->lightCulling.zBinScale;
    frameCamera->lightCulling.zBins = (uint32_t *)frame->MemAlloc(ShaderLightZBinCount * sizeof(frameCamera->lightCulling.zBins[0]));

    uint32_t startLightIndex = 0;
    uint32_t endLightIndex = 0;

    // 각 Z-bin 에 라이트 범위 [startLightIndex, endLightIndex) 를 패킹한다. (endLightIndex 의 라이트는 포함되지 않는다)
    for (uint32_t zBinIndex = 0; zBinIndex < ShaderLightZBinCount; ++zBinIndex) {
        const float currentZBinMin = cameraDesc.zNear + static_cast<float>(zBinIndex) * zBinSize;
        const float currentZBinMax = zBinIndex + 1 == ShaderLightZBinCount ? cameraDesc.zFar : currentZBinMin + zBinSize;

        while (startLightIndex < numFrameLocalLights && frameCamera->frameLocalLights[startLightIndex]->viewZMax < currentZBinMin) {
            ++startLightIndex;
        }
        while (endLightIndex < numFrameLocalLights && frameCamera->frameLocalLights[endLightIndex]->viewZMin <= currentZBinMax) {
            ++endLightIndex;
        }

        if (startLightIndex < endLightIndex) {
            frameCamera->lightCulling.zBins[zBinIndex] = PackLightZBin(startLightIndex, endLightIndex - 1);
        } else {
            frameCamera->lightCulling.zBins[zBinIndex] = ShaderLightZBinEmpty;
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
    RenderContext::activeContext->GetMainRenderDest(graphicsPipelineDesc.renderDest, frameCamera->needNormalOutput, frameCamera->needVelocityOutput);

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

        const SharedPtr<ShaderPipeline> shaderPipeline = shaderPipelineCache.GetGraphicsPipeline(shaderTemplateManager.debugDrawShaderTemplate, ShaderPermutationKey(), graphicsPipelineDesc);
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
    frameCamera->viewProjMatrix.SetOrthoRH(0, screenWidth, screenHeight, 0, -1.0, 1.0, true); // Y 를 뒤집는다.
    frameCamera->invViewProjMatrix = frameCamera->viewProjMatrix.Inverse();
    frameCamera->projectionXY = BE1::Vec4(frameCamera->viewProjMatrix[0][0], frameCamera->viewProjMatrix[1][1], frameCamera->viewProjMatrix[0][3], frameCamera->viewProjMatrix[1][3]);
    frameCamera->projectionZ = BE1::Vec2(frameCamera->viewProjMatrix[2][2], frameCamera->viewProjMatrix[2][3]);
    frameCamera->projectionOrthogonal = true;
    frameCamera->projectionInfiniteZFar = false;

    if (frameCamera->projectionPolicy.IsZNegativeOneToOne()) {
        frameCamera->ndcDepthScale = 2.0f;
        frameCamera->ndcDepthBias = -1.0f;
    } else {
        frameCamera->ndcDepthScale = 1.0f;
        frameCamera->ndcDepthBias = 0.0f;
    }

    frameCamera->prevViewProjMatrix = frameCamera->viewProjMatrix;
    frameCamera->prevInvViewProjMatrix = frameCamera->invViewProjMatrix;
    frameCamera->jitterNDC = BE1::Vec2::zero;
    frameCamera->prevJitterNDC = BE1::Vec2::zero;
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

        AddFrameSurf(frame, frameCamera, nullptr, frameObject, guiSurf->material, subMesh, FrameSurf::Flag::None, 0);
    }

    guiMesh.Clear();

    renderPassBuilder.BuildGUI(frame, frameCamera);

    frame->CmdDrawCamera(frameCamera);

    viewCount++;
}
