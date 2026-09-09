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

static_assert(static_cast<uint32_t>(RenderLight::Type::Point) == ShaderLightTypePoint);
static_assert(static_cast<uint32_t>(RenderLight::Type::Spot) == ShaderLightTypeSpot);
static_assert(static_cast<uint32_t>(RenderLight::Type::BoxDir) == ShaderLightTypeBoxDir);

static BE_INLINE bool IsRenderLayerVisible(uint32_t layerMask, uint8_t layer) {
    return layer < 32 && (layerMask & BIT(layer)) != 0;
}

static const Material *GetVisObjectMaterial(const VisObject *visObject, int materialIndex) {
    if (materialIndex >= 0 && static_cast<uint32_t>(materialIndex) < visObject->numMaterials) {
        return visObject->GetMaterials()[materialIndex];
    }
    return materialManager.defaultLitMaterial.get();
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

void RenderWorld::AddVisSurf(RenderFrameData *frameData, VisCamera *visCamera, VisLight *visLight, VisObject *visObject, const Material *material, SubMesh *subMesh, VisSurfFlag flags, int lodLevel) {
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

    VisSurf *drawSurf = (VisSurf *)frameData->MemAlloc(sizeof(VisSurf));
    drawSurf->index = visCamera->numVisSurfs;
    drawSurf->space = visObject;
    drawSurf->subMesh = subMesh;
    drawSurf->indexBuffer = indexBuffer;
    drawSurf->numIndexes = subMesh->NumIndexes(lodLevel);
    drawSurf->lodLevel = lodLevel;
    drawSurf->material = material;
    const VisSkinnedMesh *visSkinnedMesh = visObject->skinnedMesh;
    drawSurf->baseFeatureSet = BuildBaseFeatureSet(material, subMesh, visSkinnedMesh && visSkinnedMesh->skinningJointOffset != ShaderSkinningJointOffsetInvalid);
    drawSurf->shaderMaterialIndex = frameData->RegisterShaderMaterial(material);
    drawSurf->flags = flags;

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

void RenderWorld::SetupPyramidBloom(RenderFrameData *frameData, RenderCamera *renderCamera, VisCamera *visCamera) {
    if (renderCamera->GetBloomMipCount() < 2) {
        return;
    }

    visCamera->bloomDownTexture = renderCamera->GetBloomDownTexture();
    visCamera->bloomUpTexture = renderCamera->GetBloomUpTexture();
    visCamera->bloomMipCount = renderCamera->GetBloomMipCount();

    for (uint32_t mipLevel = 0; mipLevel < RenderBloomDownMaxLevelCount; ++mipLevel) {
        visCamera->bloomMipWidths[mipLevel] = renderCamera->GetBloomMipWidths()[mipLevel];
        visCamera->bloomMipHeights[mipLevel] = renderCamera->GetBloomMipHeights()[mipLevel];
        visCamera->bloomDownMipSRVs[mipLevel] = renderCamera->GetBloomDownMipSRVs()[mipLevel];
        visCamera->bloomDownMipUAVs[mipLevel] = renderCamera->GetBloomDownMipUAVs()[mipLevel];
    }
    for (uint32_t mipLevel = 0; mipLevel < RenderBloomUpMaxLevelCount; ++mipLevel) {
        visCamera->bloomUpMipSRVs[mipLevel] = renderCamera->GetBloomUpMipSRVs()[mipLevel];
        visCamera->bloomUpMipUAVs[mipLevel] = renderCamera->GetBloomUpMipUAVs()[mipLevel];
    }
}

void RenderWorld::SetupConvolutionBloom(RenderFrameData *frameData, RenderCamera *renderCamera, VisCamera *visCamera) {
    if (renderCamera->GetFFTBloomSize() == 0 ||
        !renderCamera->GetFFTBloomConvolutionOutputTexture() ||
        renderCamera->GetFFTBloomConvolutionOutputWidth() == 0 ||
        renderCamera->GetFFTBloomConvolutionOutputHeight() == 0) {
        return;
    }

    const RenderFrameSettings &renderFrameSettings = frameData->GetRenderFrameSettings();
    renderCamera->SetConvolutionBloomKernelRotation(renderFrameSettings.convolutionBloomKernelRotation);
    renderCamera->SetConvolutionBloomKernelScale(renderFrameSettings.convolutionBloomKernelScale);
    renderCamera->SetConvolutionBloomPaddingTrim(renderFrameSettings.convolutionBloomPaddingTrim);

    const uint32_t fftBloomSize = renderCamera->GetFFTBloomSize();
    const bool updateFFTBloomKernelBuffer = renderCamera->IsFFTBloomKernelDirty();

    Texture *fftBloomKernelSourceTexture = nullptr;
    RHI::Texture *fftBloomKernelSourceRHITexture = nullptr;
    if (updateFFTBloomKernelBuffer) {
        fftBloomKernelSourceTexture = frameData->RetainTexture(renderCamera->GetFFTBloomKernelTexture());
        if (!fftBloomKernelSourceTexture || !fftBloomKernelSourceTexture->IsReadyForRendering()) {
            return;
        }
        fftBloomKernelSourceRHITexture = fftBloomKernelSourceTexture->GetRHITexture();

        renderCamera->UpdateConvolutionBloomMapping();
    }

    visCamera->fftBloomSize = fftBloomSize;
    visCamera->fftBloomScreenSpatialBuffer = renderCamera->GetFFTBloomScreenSpatialBuffer();
    visCamera->fftBloomScreenSpectrumBuffer = renderCamera->GetFFTBloomScreenSpectrumBuffer();
    visCamera->fftBloomKernelSourceTexture = updateFFTBloomKernelBuffer ? fftBloomKernelSourceRHITexture : nullptr;
    visCamera->fftBloomKernelSpatialBuffer = renderCamera->GetFFTBloomKernelSpatialBuffer();
    visCamera->fftBloomKernelSpectrumBuffer = renderCamera->GetFFTBloomKernelSpectrumBuffer();
    visCamera->fftBloomWorkBuffer = renderCamera->GetFFTBloomWorkBuffer();
    visCamera->fftBloomConvolutionOutputTexture = renderCamera->GetFFTBloomConvolutionOutputTexture();
    visCamera->fftBloomConvolutionOutputWidth = renderCamera->GetFFTBloomConvolutionOutputWidth();
    visCamera->fftBloomConvolutionOutputHeight = renderCamera->GetFFTBloomConvolutionOutputHeight();
    visCamera->fftBloomConvolutionOutputMipCount = renderCamera->GetFFTBloomConvolutionOutputMipCount();
    for (uint32_t mipLevel = 0; mipLevel < RenderConvolutionBloomMaxMipLevelCount; ++mipLevel) {
        visCamera->fftBloomConvolutionOutputMipSRVs[mipLevel] = renderCamera->GetFFTBloomConvolutionOutputMipSRVs()[mipLevel];
        visCamera->fftBloomConvolutionOutputMipUAVs[mipLevel] = renderCamera->GetFFTBloomConvolutionOutputMipUAVs()[mipLevel];
    }
    visCamera->fftBloomKernelRotation = renderCamera->GetFFTBloomKernelRotation();
    visCamera->fftBloomKernelScale = renderCamera->GetFFTBloomKernelScale();
    visCamera->fftBloomPaddingTrim = renderCamera->GetFFTBloomPaddingTrim();
    visCamera->fftBloomKernelNormalizedExtents = renderCamera->GetFFTBloomKernelNormalizedExtents();
    visCamera->fftBloomMappedContentRect = renderCamera->GetFFTBloomMappedContentRect();
    visCamera->fftBloomMappedKernelScale = renderCamera->GetFFTBloomMappedKernelScale();
    visCamera->fftBloomKernelInverseAverageLuminance = renderCamera->GetFFTBloomKernelInverseAverageLuminance();
    visCamera->fftBloomKernelDirty = updateFFTBloomKernelBuffer;

    if (updateFFTBloomKernelBuffer) {
        renderCamera->MarkConvolutionBloomKernelClean();
    }
}

static RHI::PipelineState *GetOutputImagePSO(const RHI::Texture *outputTexture, RHI::ViewID outputViewID) {
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

    RenderFrameData *frameData = RenderContext::activeContext->GetCurrentFrontendFrameData();
    RenderContext *renderContext = RenderContext::activeContext;
    const RenderFrameSettings &renderFrameSettings = frameData->GetRenderFrameSettings();
    const BE1::Rect &outputRect = renderCamera->desc.outputRect;
    // VisLight stores each tile coordinate in 8 bits; limit internal render targets to 256 tiles per axis.
    const uint32_t renderWidth = BE1::Min(renderCamera->desc.renderWidth > 0 ? renderCamera->desc.renderWidth : (outputRect.w > 0 ? static_cast<uint32_t>(outputRect.w) : 0), VisLight::MaxRenderDimension);
    const uint32_t renderHeight = BE1::Min(renderCamera->desc.renderHeight > 0 ? renderCamera->desc.renderHeight : (outputRect.h > 0 ? static_cast<uint32_t>(outputRect.h) : 0), VisLight::MaxRenderDimension);

    if (renderWidth <= 0 || renderHeight <= 0) {
        return;
    }

    RHI::PipelineState *outputImagePSO = nullptr;
    if (renderCamera->desc.outputTexture) {
        if (!renderCamera->desc.outputTexture->HasValidView(RHI::ViewType::RTV, renderCamera->desc.outputViewID)) {
            BE_WARNLOG("RenderWorld::RenderScene: output texture has no valid render target view\n");
            return;
        }

        outputImagePSO = GetOutputImagePSO(renderCamera->desc.outputTexture, renderCamera->desc.outputViewID);
        if (!outputImagePSO) {
            BE_WARNLOG("RenderWorld::RenderScene: output image pipeline is not ready\n");
            return;
        }
    }

    const bool skipPostProcess = renderFrameSettings.skipPostProcess || BE1::HasFlag(renderCamera->desc.flags, RenderCamera::Flag::SkipPostProcess);
    const bool temporalHistoryEnabled = !BE1::HasFlag(renderCamera->desc.flags, RenderCamera::Flag::DisableTemporalHistory);
    const bool useTAA = temporalHistoryEnabled && renderFrameSettings.taa && !skipPostProcess;
    const bool needColorHistory = useTAA;
    const bool needDepthHistory = useTAA;
    const bool needHiZCullResources = !BE1::HasFlag(renderCamera->desc.flags, RenderCamera::Flag::DisableHiZCulling) && renderFrameSettings.hiZOcclusionCulling;
    const bool needHiZTexture = needHiZCullResources;
    const bool needHiZDebugPreview = needHiZTexture && renderFrameSettings.hiZDebugPreview;
    const bool needSSR = renderFrameSettings.ssr && !BE1::HasFlag(renderCamera->desc.flags, RenderCamera::Flag::StaticOnly);

    const RenderBloomMode bloomMode = renderFrameSettings.bloomMode;

    if (!renderCamera->EnsureRenderTextures(renderWidth, renderHeight, renderContext->GetMainRTColorFormat(), renderContext->GetMainRTDepthFormat(), renderContext->GetMainRTSampleCount(), needDepthHistory, renderContext->GetBaseClearColor())) {
        return;
    }

    renderCamera->EnsureScreenSpaceDepthPyramidResources(renderWidth, renderHeight, needSSR);
    renderCamera->EnsureSSRTraceResources(renderWidth, renderHeight, renderContext->GetMainRTSampleCount(), needSSR);

    const bool needSceneSnapshot = hasOceanSurface && oceanSurface.desc.surface.refractionEnabled && !oceanSurface.desc.surface.wireframe && !BE1::HasFlag(renderCamera->desc.flags, RenderCamera::Flag::StaticOnly);
    const bool requestHalfResSnapshot = needSceneSnapshot && renderFrameSettings.snapshotColorHalfRes && !renderCamera->GetRTColorMSAATexture();
    RHI::PipelineState *sceneSnapshotDownsamplePSO = requestHalfResSnapshot ? GetSceneSnapshotDownsamplePSO(renderContext->GetMainRTColorFormat()) : nullptr;
    const bool useHalfResSnapshot = requestHalfResSnapshot && sceneSnapshotDownsamplePSO;
    renderCamera->EnsureSceneSnapshotResources(needSceneSnapshot, useHalfResSnapshot);

    renderCamera->EnsureHiZResources(renderWidth, renderHeight, needHiZTexture, needHiZCullResources, needHiZDebugPreview);
    renderCamera->EnsurePostProcessResources(skipPostProcess, renderContext->GetPostProcessColorFormat(), needColorHistory, needDepthHistory, renderFrameSettings);

    const bool temporalJitterEnabled = useTAA && renderFrameSettings.taaJitter;
    const uint32_t temporalJitterSampleCount = renderFrameSettings.taaJitterSampleCount;

    renderCamera->UpdateTemporalJitter(temporalJitterEnabled, temporalJitterSampleCount);

    const bool canUseHiZ = renderCamera->GetDepthTexture() && renderCamera->GetHiZTexture();
    const bool requestHiZOcclusionCulling = canUseHiZ && needHiZCullResources && renderCamera->GetHiZCullVisibleInstanceBuffer() && renderCamera->GetHiZCullIndirectArgsBuffer();
    const bool requestHiZDebugPreview = canUseHiZ && needHiZDebugPreview && renderCamera->GetHiZDebugPreviewTexture();
    const bool requestHiZGeneration = requestHiZOcclusionCulling;
    const bool useMSAA = renderCamera->GetRTColorMSAATexture() != nullptr;
    const bool hasSSRInputs = renderCamera->GetRTNormalTexture() && (!useMSAA || renderCamera->GetSSRResolvedDepthTexture());
    const bool requestScreenSpaceDepthPyramidGeneration = needSSR && hasSSRInputs && renderCamera->GetDepthTexture() && renderCamera->GetScreenSpaceDepthPyramidTexture();
    const bool requestSSRTrace = requestScreenSpaceDepthPyramidGeneration && renderCamera->GetSSRTraceTexture();

    // RenderCamera 정보를 기반으로 프레임 데이터에 필요한 VisCamera 를 구성한다.
    VisCamera *visCamera = frameData->AllocVisCamera();
    if (!visCamera) {
        return;
    }

    visCamera->maxVisSurfs = VisCamera::MaxVisSurfs;
    visCamera->visSurfs = (const VisSurf **)frameData->MemAlloc(visCamera->maxVisSurfs * sizeof(VisSurf *));
    visCamera->numRenderPasses = 0;
    visCamera->renderPasses = (RenderPass *)frameData->MemAlloc(VisCamera::MaxRenderPasses * sizeof(RenderPass));
    visCamera->origin = renderCamera->desc.origin;
    visCamera->axis = renderCamera->desc.axis;

    if (hasEnvironmentLight) {
        const EnvironmentLight::Desc &envLightDesc = envLight.GetDesc();
        visCamera->envLightAxis = envLightDesc.axis;
        visCamera->envLightColor = envLightDesc.linearColor;
        visCamera->envLightIntensity = envLightDesc.intensity;

        Texture *diffuseIrradianceCubeMap = frameData->RetainTexture(envLight.GetDiffuseIrradianceCubeMap());
        RHI::Texture *rhiDiffuseIrradianceCubeMap = diffuseIrradianceCubeMap ? diffuseIrradianceCubeMap->GetReadyRHITexture() : nullptr;
        if (rhiDiffuseIrradianceCubeMap && rhiDiffuseIrradianceCubeMap->type == RHI::TextureType::TextureCube) {
            visCamera->envLightDiffuseIrradianceCubeMap = rhiDiffuseIrradianceCubeMap;
        }

        Texture *specularCubeMap = frameData->RetainTexture(envLight.GetSpecularPrefilteredCubeMap());
        RHI::Texture *rhiSpecularCubeMap = specularCubeMap ? specularCubeMap->GetReadyRHITexture() : nullptr;
        if (rhiSpecularCubeMap && rhiSpecularCubeMap->type == RHI::TextureType::TextureCube) {
            visCamera->envLightSpecularCubeMap = rhiSpecularCubeMap;
            visCamera->envLightSpecularMipCount = rhiSpecularCubeMap->GetMipLevelCount();
        }
    }

    visCamera->outputRect = outputRect;
    visCamera->outputTexture = renderCamera->desc.outputTexture;
    visCamera->outputViewID = renderCamera->desc.outputViewID;
    visCamera->outputBeforeState = renderCamera->desc.outputBeforeState;
    visCamera->outputAfterState = renderCamera->desc.outputAfterState;
    visCamera->outputImagePSO = outputImagePSO;
    visCamera->renderWidth = renderWidth;
    visCamera->renderHeight = renderHeight;
    visCamera->clearMethod = renderCamera->desc.clearMethod;
    visCamera->clearColorMode = renderCamera->desc.clearColorMode;
    visCamera->clearColor = renderCamera->desc.clearColor;
    visCamera->projectionPolicy = renderCamera->desc.projectionPolicy;
    visCamera->projectionDepthA = renderCamera->projMatrix[2][2];
    visCamera->projectionDepthB = renderCamera->projMatrix[2][3];
    visCamera->projectionOrthogonal = renderCamera->desc.orthogonal;
    visCamera->projectionInfiniteZFar = renderCamera->desc.infiniteZFar;
    if (visCamera->projectionOrthogonal) {
        visCamera->viewBox = renderCamera->GetBox();
    } else {
        visCamera->viewFrustum = renderCamera->GetFrustum();
    }

    if (visCamera->projectionPolicy.IsZNegativeOneToOne()) {
        visCamera->ndcDepthScale = 2.0f;
        visCamera->ndcDepthBias = -1.0f;
    } else {
        visCamera->ndcDepthScale = 1.0f;
        visCamera->ndcDepthBias = 0.0f;
    }

    visCamera->flags = renderCamera->desc.flags;
    visCamera->needNormalOutput = needSSR;
    visCamera->needVelocityOutput = useTAA;
    visCamera->rtColorTexture = renderCamera->GetRTColorTexture();
    visCamera->rtColorMSAATexture = renderCamera->GetRTColorMSAATexture();
    visCamera->depthTexture = renderCamera->GetDepthTexture();
    visCamera->rtNormalTexture = renderCamera->GetRTNormalTexture();
    visCamera->rtNormalMSAATexture = renderCamera->GetRTNormalMSAATexture();
    visCamera->rtVelocityTexture = renderCamera->GetRTVelocityTexture();
    visCamera->rtVelocityMSAATexture = renderCamera->GetRTVelocityMSAATexture();
    visCamera->sceneSnapshotDownsamplePSO = useHalfResSnapshot ? sceneSnapshotDownsamplePSO : nullptr;
    visCamera->snapshotColorTexture = renderCamera->GetSnapshotColorTexture();
    visCamera->snapshotDepthTexture = renderCamera->GetSnapshotDepthTexture();

    visCamera->generateScreenSpaceDepthPyramid = requestScreenSpaceDepthPyramidGeneration;
    if (visCamera->generateScreenSpaceDepthPyramid) {
        visCamera->screenSpaceDepthPyramidTexture = renderCamera->GetScreenSpaceDepthPyramidTexture();
        visCamera->screenSpaceDepthPyramidMipCount = renderCamera->GetScreenSpaceDepthPyramidMipCount();
        visCamera->screenSpaceDepthPyramidValidWidth = renderCamera->GetScreenSpaceDepthPyramidValidWidth();
        visCamera->screenSpaceDepthPyramidValidHeight = renderCamera->GetScreenSpaceDepthPyramidValidHeight();

        for (uint32_t mipLevel = 0; mipLevel < RenderScreenSpaceDepthPyramidMaxLevelCount; ++mipLevel) {
            visCamera->screenSpaceDepthPyramidMipSRVs[mipLevel] = renderCamera->GetScreenSpaceDepthPyramidMipSRVs()[mipLevel];
            visCamera->screenSpaceDepthPyramidMipUAVs[mipLevel] = renderCamera->GetScreenSpaceDepthPyramidMipUAVs()[mipLevel];
            visCamera->screenSpaceDepthPyramidMipWidths[mipLevel] = renderCamera->GetScreenSpaceDepthPyramidMipWidths()[mipLevel];
            visCamera->screenSpaceDepthPyramidMipHeights[mipLevel] = renderCamera->GetScreenSpaceDepthPyramidMipHeights()[mipLevel];
        }
    }

    visCamera->traceSSR = requestSSRTrace;
    visCamera->ssrResolvedDepthTexture = renderCamera->GetSSRResolvedDepthTexture();
    visCamera->ssrTraceTexture = renderCamera->GetSSRTraceTexture();

    visCamera->generateHiZ = requestHiZGeneration;
    if (visCamera->generateHiZ) {
        visCamera->hiZTexture = renderCamera->GetHiZTexture();
        visCamera->hiZMipCount = renderCamera->GetHiZMipCount();

        for (uint32_t mipLevel = 0; mipLevel < RenderHiZMaxLevelCount; ++mipLevel) {
            visCamera->hiZMipSRVs[mipLevel] = renderCamera->GetHiZMipSRVs()[mipLevel];
            visCamera->hiZMipUAVs[mipLevel] = renderCamera->GetHiZMipUAVs()[mipLevel];
            visCamera->hiZMipWidths[mipLevel] = renderCamera->GetHiZMipWidths()[mipLevel];
            visCamera->hiZMipHeights[mipLevel] = renderCamera->GetHiZMipHeights()[mipLevel];
        }

        visCamera->hiZOcclusionCulling = renderCamera->IsPreviousHiZValid() && requestHiZOcclusionCulling;
        if (visCamera->hiZOcclusionCulling) {
            visCamera->hiZCullVisibleInstanceBuffer = renderCamera->GetHiZCullVisibleInstanceBuffer();
            visCamera->hiZCullIndirectArgsBuffer = renderCamera->GetHiZCullIndirectArgsBuffer();
        }

        visCamera->hiZDebugPreview = requestHiZDebugPreview;
        if (visCamera->hiZDebugPreview) {
            visCamera->hiZDebugPreviewTexture = renderCamera->GetHiZDebugPreviewTexture();
            visCamera->hiZDebugLinearDepthScale = renderCamera->desc.zFar > 0.0f ? 1.0f / renderCamera->desc.zFar : 1.0f;
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

    if (!skipPostProcess) {
        // 초기 LUT upload request 는 backend frame 실행 전에 처리되므로 Pending 상태의 RHI 포인터도 프레임에 전달할 수 있다.
        Texture *colorGradingLUTTexture = frameData->RetainTexture(renderSystem->GetColorGradingLUTTexture());
        visCamera->colorGradingLUTTexture = colorGradingLUTTexture ? colorGradingLUTTexture->GetRHITexture() : nullptr;

        Texture *bloomDirtMaskTexture = frameData->RetainTexture(renderCamera->GetBloomDirtMaskTexture());
        visCamera->bloomDirtMaskTexture = bloomDirtMaskTexture ? bloomDirtMaskTexture->GetRHITexture() : nullptr;

        if (bloomMode == RenderBloomMode::Pyramid) {
            SetupPyramidBloom(frameData, renderCamera, visCamera);
        } else if (bloomMode == RenderBloomMode::Convolution) {
            SetupConvolutionBloom(frameData, renderCamera, visCamera);
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
    if (!renderFrameSettings.hiZOcclusionCullingFreeze || !renderCamera->previousHiZValid) {
        renderCamera->hiZCullViewProjMatrix = visCamera->prevViewProjMatrix;
    }
    visCamera->hiZCullViewProjMatrix = renderCamera->hiZCullViewProjMatrix;

    DrawCamera(frameData, renderCamera, visCamera);

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

    if (!renderFrameSettings.hiZOcclusionCullingFreeze) {
        renderCamera->previousHiZValid = visCamera->generateHiZ;
    }
}

void RenderWorld::DrawCamera(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::DrawCamera");

    // RenderObject 중에서 컬링한 오브젝트를 프레임 데이터에 VisObject 로 등록한다.
    FindVisObjects(frameData, renderCamera, visCamera);

    // RenderLight 중에서 컬링한 라이트를 프레임 데이터에 VisLight 로 등록한다.
    FindVisLights(frameData, renderCamera, visCamera);

    // 단일 directional shadow 의 receiver 영역을 준비하고 caster 수집 후 투영 깊이를 확장한다.
    SetupDirectionalShadowView(frameData, renderCamera, visCamera);

    // visObjects 중에서 mesh surface 들을 visCamera->visSurfs 에 추가한다.
    AddMeshes(frameData, renderCamera, visCamera);

    // visibleLights 들로 GPU light culling 데이터를 구성한다.
    BuildLightCullingData(frameData, renderCamera, visCamera);

    // visObjects 중에서 text mesh 를 visCamera->visSurfs 에 추가한다.
    AddTextMeshes(frameData, renderCamera, visCamera);

    // Add drawing surface of skybox.
    AddSkyBoxMeshes(frameData, renderCamera, visCamera);

    // Ocean surface 와 query 정보를 프레임 데이터에 등록한다.
    if (hasOceanSurface) {
        frameData->RegisterVisOceanSurface(oceanSurface.desc.simulation, oceanSurface.desc.surface, oceanSurface.revision, oceanSurface.queries.Ptr(), static_cast<uint32_t>(oceanSurface.queries.Count()));
    }

    // 렌더 패스를 구성한다.
    renderPassBuilder.BuildMain(frameData, visCamera);

    // Post-Opaque 단계에서 그릴 ocean surface pipeline 을 준비한다.
    PrepareOceanSurfacePipeline(visCamera);

    // 카메라의 debug draw 데이터를 구성한다.
    BuildDebugDraw(frameData, visCamera);

    // 렌더링 커맨드에 visCamera 를 기록한다.
    frameData->CmdDrawCamera(visCamera);

    viewCount++;
}

void RenderWorld::SetupDirectionalShadowView(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    if (!visCamera->directionalLightVisible) {
        return;
    }

    const VisLight *directionalLight = frameData->GetPrimaryDirectionalLight();
    if (!directionalLight || !BE1::HasFlag(directionalLight->flags, RenderLight::Flag::CastShadows)) {
        return;
    }

    const RenderFrameSettings &settings = frameData->GetRenderFrameSettings();
    const RenderCamera::Desc &cameraDesc = renderCamera->GetDesc();
    const float receiverFar = BE1::Min(cameraDesc.zFar, settings.directionalShadowDistance);
    if (settings.directionalShadowDistance <= 0.0f || receiverFar <= cameraDesc.zNear) {
        return;
    }

    // view bounding volume 으로 directional shadow receiver 영역 (receiver sphere) 을 구한다.
    BE1::Sphere receiverSphere;
    if (cameraDesc.orthogonal) {
        BE1::OBB receiverBox = renderCamera->GetBox();
        const float halfDepth = (receiverFar - cameraDesc.zNear) * 0.5f;
        receiverBox.SetCenter(cameraDesc.origin + cameraDesc.axis[EngineAxis::Forward] * (cameraDesc.zNear + halfDepth));
        receiverBox.SetExtents(BE1::Vec3(halfDepth, cameraDesc.sizeX, cameraDesc.sizeY));
        receiverSphere = receiverBox.ToSphere();
    } else {
        BE1::Frustum receiverFrustum = renderCamera->GetFrustum();
        receiverFrustum.MoveFarDistance(receiverFar);
        receiverSphere = receiverFrustum.ToSphere();
    }

    // receiver sphere 를 포함하는 directional shadow world box 와 view-projection 행렬을 구한다.
    BE1::OBB shadowWorldBox;
    BE1::Mat4 shadowViewProjMatrix;
    if (!ComputeDirectionalShadowProjection(directionalLight->axis, receiverSphere, BE1::AABB::empty, settings.csmResolution, shadowWorldBox, shadowViewProjMatrix)) {
        return;
    }

    // directional shadow world box 를 이용해서 caster 를 컬링/수집하고 caster bounds 를 리턴한다.
    const BE1::AABB casterBounds = FindDirectionalShadowCasters(frameData, renderCamera, shadowWorldBox);

    if (!casterBounds.IsCleared()) {
        // 수집된 caster 의 광원 쪽 깊이만 확장하고 receiver 기준 XY 와 far 범위를 유지한다.
        if (!ComputeDirectionalShadowProjection(directionalLight->axis, receiverSphere, casterBounds, settings.csmResolution, shadowWorldBox, shadowViewProjMatrix)) {
            return;
        }
    }

    DirectionalShadowView *shadowView = new (frameData->MemAlloc(sizeof(DirectionalShadowView))) DirectionalShadowView();
    shadowView->receiverSphere = receiverSphere;
    shadowView->worldBox = shadowWorldBox;
    shadowView->viewProjMatrix = shadowViewProjMatrix;

    // worldToShadowCoordMatrix * worldPos = shadowCoord
    // shadowCoord.xy : shadow texture UV
    // shadowCoord.z  : shadow depth
    for (int column = 0; column < 4; column++) {
        // D3D texture UV: clip XY [-1, 1] -> [0, 1], Y 는 반전. Depth 는 이미 [0, 1] 이다.
        shadowView->shaderData.worldToShadowCoordMatrix.m[0][column] = 0.5f * (shadowViewProjMatrix[0][column] + shadowViewProjMatrix[3][column]);
        shadowView->shaderData.worldToShadowCoordMatrix.m[1][column] = 0.5f * (-shadowViewProjMatrix[1][column] + shadowViewProjMatrix[3][column]);
        shadowView->shaderData.worldToShadowCoordMatrix.m[2][column] = shadowViewProjMatrix[2][column];
    }

    const float texelSize = 1.0f / settings.csmResolution;
    shadowView->shaderData.texelSize = float2(texelSize, texelSize);

    visCamera->directionalShadowView = shadowView;
}

BE1::AABB RenderWorld::FindDirectionalShadowCasters(RenderFrameData *frameData, const RenderCamera *renderCamera, const BE1::OBB &shadowWorldBox) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::FindDirectionalShadowCasters");

    const RenderCamera::Desc &cameraDesc = renderCamera->GetDesc();
    const bool staticOnly = BE1::HasFlag(cameraDesc.flags, RenderCamera::Flag::StaticOnly);
    BE1::AABB casterBounds = BE1::AABB::empty;

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

        if (CullDirectionalShadowCaster(renderObject->worldAABB, shadowWorldBox)) {
            continue;
        }

        VisObject *visObject = RegisterVisObject(frameData, renderObject);
        if (!visObject) {
            continue;
        }

        // 카메라에 안 보이는 caster 는 shadow 에 현재 world matrix 만 필요하다.
        // 공용 ShaderSurface 가 prevWorldMatrix 도 업로드하므로 현재 행렬로 초기화한다.
        if (!BE1::HasFlag(visObject->flags, VisObject::Flag::CameraVisible)) {
            visObject->worldMatrix = renderObject->GetWorldMatrix();
            visObject->prevWorldMatrix = visObject->worldMatrix;
        }

        visObject->flags |= VisObject::Flag::ShadowVisible;

        casterBounds.AddAABB(renderObject->worldAABB);
    }

    return casterBounds;
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

VisLight *RenderWorld::RegisterVisLight(RenderFrameData *frameData, RenderLight *renderLight) {
    assert(frameActive);

    if (IsVisLightRegisteredInCurrentView(renderLight)) {
        return nullptr;
    }

    VisLight *visLight = frameData->AllocVisLight(renderLight->desc.type);
    if (!visLight) {
        return nullptr;
    }

    renderLight->viewCount = viewCount;

    if (renderLight->desc.type == RenderLight::Type::Point) {
        visLight->viewProjScaleBiasMatrix[0] = renderLight->viewProjScaleBiasMatrix[0];
        visLight->viewProjScaleBiasMatrix[1] = renderLight->viewProjScaleBiasMatrix[1];
    } else if (renderLight->desc.type == RenderLight::Type::Spot) {
        visLight->viewProjScaleBiasMatrix = renderLight->viewProjScaleBiasMatrix;

        VisSpotLightData *spotLightData = frameData->AllocVisSpotLightData();
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

VisObject *RenderWorld::RegisterVisObject(RenderFrameData *frameData, RenderObject *renderObject) {
    assert(frameActive);

    if (IsVisObjectRegisteredInCurrentView(renderObject)) {
        return renderObject->visObject;
    }

    VisObject *visObject = frameData->AllocVisObject();
    if (!visObject) {
        return nullptr;
    }
    renderObject->viewCount = viewCount;
    renderObject->visObject = visObject;
    visObject->meshLOD = renderObject->GetMeshLOD();

    if (renderObject->desc.meshInstance) {
        visObject->meshInstance = frameData->RetainMeshInstance(renderObject->desc.meshInstance);
    }

    if (visObject->meshInstance && visObject->meshInstance->IsSkinned() && renderObject->desc.skeleton) {
        VisSkinnedMesh *visSkinnedMesh = frameData->AllocVisSkinnedMesh();
        if (visSkinnedMesh) {
            visSkinnedMesh->space = visObject;
            visSkinnedMesh->meshInstance = visObject->meshInstance;
            visSkinnedMesh->skeleton = frameData->RetainSkeleton(renderObject->desc.skeleton);
            visObject->skinnedMesh = visSkinnedMesh;
        }
    }

    const int numMaterials = renderObject->desc.materials.Count();
    visObject->numMaterials = static_cast<uint16_t>(numMaterials);
    if (numMaterials > 0) {
        const Material **materials;
        if (numMaterials > COUNT_OF(visObject->baseMaterials)) {
            materials = (const Material **)frameData->MemAlloc(numMaterials * sizeof(materials[0]));
            visObject->materials = materials;
        } else {
            materials = visObject->baseMaterials;
        }
        for (int i = 0; i < numMaterials; ++i) {
            materials[i] = frameData->RetainMaterial(renderObject->desc.materials[i]);
        }
    }
    if (renderObject->desc.text && !renderObject->desc.text->text.IsEmpty()) {
        RenderObject::TextDesc &textDesc = *(renderObject->desc.text);
        VisText *visText = frameData->AllocVisText();
        if (visText) {
            visText->space = visObject;
            if (textDesc.font) {
                visText->font = frameData->RetainFont(textDesc.font);
            }

            int size = textDesc.text.Length() + 1;
            char *textMemPtr = (char *)frameData->MemAlloc(size);
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

void RenderWorld::FindVisObjects(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::FindVisObjects");

    const bool skipViewCulling = frameData->GetRenderFrameSettings().skipViewCulling;
    const float meshLODPixelError = frameData->GetRenderFrameSettings().meshLODPixelError;
    const RenderCamera::Desc &cameraDesc = renderCamera->GetDesc();
    const bool staticOnly = BE1::HasFlag(cameraDesc.flags, RenderCamera::Flag::StaticOnly);

    // 같은 프레임 데이터로 RenderScene 을 여러 번 호출하면 앞선 카메라의 VisObject, VisSkinnedMesh, VisText 가 누적되어 있다.
    // 현재 카메라에서 새로 등록할 각 레코드의 범위를 구분하기 위해 시작 인덱스를 기록한다.
    visCamera->firstVisObjectIndex = frameData->NumVisObjects();
    visCamera->firstVisSkinnedMeshIndex = frameData->NumVisSkinnedMeshes();
    visCamera->firstVisTextIndex = frameData->NumVisTexts();

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

        VisObject *visObject = RegisterVisObject(frameData, renderObject);
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
                visObject->squaredLODErrorLimit = renderCamera->CalcLocalSquaredLODErrorLimit(renderObject->GetWorldAABB(), visObject->worldMatrix, meshLODPixelError);
            }
        }

        visCamera->worldAABB.AddAABB(renderObject->worldAABB);
    }
}

void RenderWorld::UpdateSkinningJointCaches(RenderFrameData *frameData, const VisSkinnedMesh **objects, uint32_t numObjects, uint32_t numSkinningJoints) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::UpdateSkinningJointCaches");

    if (numObjects == 0 || numSkinningJoints == 0) {
        return;
    }

#ifdef USE_RENDER_TASK
    static constexpr uint32_t MaxSkinningJointsPerTask = 8192;

    uint32_t numTasks = (numSkinningJoints + MaxSkinningJointsPerTask - 1) / MaxSkinningJointsPerTask;
    numTasks = BE1::Min(frameData->NumRenderTaskThreads(), numTasks);

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

int RenderWorld::CountSkinningUploadJoints(const RenderFrameData::ChunkRange<VisSkinnedMesh> &visSkinnedMeshes, bool needVelocityOutput, uint64_t currentFrameSerial) const {
    uint32_t numTotalUploadJoints = 0;

    for (const VisSkinnedMesh &visSkinnedMesh : visSkinnedMeshes) {
        if ((visSkinnedMesh.space->flags & (VisObject::Flag::CameraVisible | VisObject::Flag::ShadowVisible)) == VisObject::Flag::None) {
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

void RenderWorld::BuildSkinningJointBuffer(RenderFrameData *frameData, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::BuildSkinningJointBuffer");

    if (frameData->NumVisSkinnedMeshes() == visCamera->firstVisSkinnedMeshIndex) {
        return;
    }

    skinningJointCacheUpdateObjects.SetCount(0, false);

    uint32_t numTotalJoints = 0;

    const auto visSkinnedMeshes = frameData->GetVisSkinnedMeshes(visCamera->firstVisSkinnedMeshIndex);
    const uint64_t currentFrameSerial = frameData->GetFrameSerial();

    for (const VisSkinnedMesh &visSkinnedMesh : visSkinnedMeshes) {
        if ((visSkinnedMesh.space->flags & (VisObject::Flag::CameraVisible | VisObject::Flag::ShadowVisible)) == VisObject::Flag::None) {
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
    UpdateSkinningJointCaches(frameData, skinningJointCacheUpdateObjects.Ptr(), skinningJointCacheUpdateObjects.Count(), numTotalJoints);

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
    RHI::Buffer *jointBuffer = frameData->GetThreadData(0)->AllocStructuredBuffer(sizeof(ShaderSkinningJoint), static_cast<uint32_t>(numTotalUploadJoints));
    if (!jointBuffer) {
        return;
    }

    // 업로드 버퍼에는 메인 스레드에서 연속으로 복사한다.
    BE1::Mat3x4 *dst = static_cast<BE1::Mat3x4 *>(jointBuffer->mappedPtr);
    uint32_t jointOffset = 0;
    for (VisSkinnedMesh &visSkinnedMesh : visSkinnedMeshes) {
        if ((visSkinnedMesh.space->flags & (VisObject::Flag::CameraVisible | VisObject::Flag::ShadowVisible)) == VisObject::Flag::None) {
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

void RenderWorld::AddMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::AddMeshes");

    // 현재 카메라에서 새로 등록된 VisObject 가 없으면 메시 처리를 생략한다.
    if (frameData->NumVisObjects() == visCamera->firstVisObjectIndex) {
        return;
    }

    BuildSkinningJointBuffer(frameData, visCamera);

    const bool skipViewCulling = frameData->GetRenderFrameSettings().skipViewCulling;

    for (VisObject &visObjectRef : frameData->GetVisObjects(visCamera->firstVisObjectIndex)) {
        VisObject *visObject = &visObjectRef;
        const bool cameraVisible = BE1::HasFlag(visObject->flags, VisObject::Flag::CameraVisible);
        const bool shadowVisible = visCamera->directionalShadowView && BE1::HasFlag(visObject->flags, VisObject::Flag::ShadowVisible);
        if ((!cameraVisible && !shadowVisible) || !visObject->meshInstance) {
            continue;
        }

        if (!visObject->meshInstance->EnsureCurrent()) {
            continue;
        }

        const bool skinned = visObject->meshInstance->IsSkinned();
        // Pose 준비에 실패한 skinned mesh 를 bind pose 상태로 등록하지 않는다.
        if (skinned && (!visObject->skinnedMesh || visObject->skinnedMesh->skinningJointOffset == ShaderSkinningJointOffsetInvalid)) {
            continue;
        }

        const int numSurfaces = visObject->meshInstance->NumSurfaces();
        // Skinned submesh 의 bounds 는 bind pose 기준이므로 오브젝트 수집 단계의 bounds 판정을 유지한다.
        const bool cullSurfaces = numSurfaces > 1 && !skinned;

        for (int meshSurfIndex = 0; meshSurfIndex < numSurfaces; ++meshSurfIndex) {
            const Mesh::Surface *meshSurf = visObject->meshInstance->GetSurface(meshSurfIndex);
            if (!meshSurf || !meshSurf->subMesh) {
                continue;
            }

            const Material *material = GetVisObjectMaterial(visObject, meshSurf->materialIndex);
            bool cameraSurfaceVisible = cameraVisible;
            bool shadowSurfaceVisible = shadowVisible && !BE1::HasFlag(material->GetFlags(), Material::Flag::NoShadow) && material->GetRenderingMode() != Material::RenderingMode::Blend;
            if (!cameraSurfaceVisible && !shadowSurfaceVisible) {
                continue;
            }

            // Static submesh 의 world bounds 는 카메라와 shadow 컬링에 한 번만 계산하여 공유한다.
            if (cullSurfaces && ((!skipViewCulling && cameraSurfaceVisible) || shadowSurfaceVisible)) {
                const BE1::AABB &localAABB = meshSurf->subMesh->GetBoundingBox();
                if (!localAABB.IsCleared()) {
                    BE1::AABB worldAABB;
                    worldAABB.SetFromTransformedAABB(localAABB, visObject->worldMatrix);

                    if (cameraSurfaceVisible && !skipViewCulling) {
                        cameraSurfaceVisible = !renderCamera->Cull(worldAABB);
                    }
                    if (shadowSurfaceVisible) {
                        shadowSurfaceVisible = !CullDirectionalShadowCaster(worldAABB, visCamera->directionalShadowView->worldBox);
                    }
                }
            }

            VisSurfFlag flags = VisSurfFlag::None;
            if (cameraSurfaceVisible) {
                flags |= VisSurfFlag::CameraVisible;
            }
            if (shadowSurfaceVisible) {
                flags |= VisSurfFlag::ShadowVisible;
            }
            if (flags == VisSurfFlag::None) {
                continue;
            }

            // 두 패스는 같은 surface 와 LOD 를 공유한다. Shadow 전용 오브젝트의 자동 LOD 는 기본값 0 을 사용한다.
            const int lodLevel = SelectMeshLOD(meshSurf->subMesh, visObject->meshLOD, visObject->squaredLODErrorLimit);
            AddVisSurf(frameData, visCamera, nullptr, visObject, material, meshSurf->subMesh, flags, lodLevel);
        }
    }
}

void RenderWorld::AddTextMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::AddTextMeshes");

    // 이전 카메라에서 등록한 VisText 는 제외하고 현재 카메라의 시작 위치부터 순회한다.
    for (VisText &visText : frameData->GetVisTexts(visCamera->firstVisTextIndex)) {
        VisObject *visObject = visText.space;
        if (!BE1::HasFlag(visObject->flags, VisObject::Flag::CameraVisible)) {
            continue;
        }

        textMesh.Clear();

        if (visText.textRect.IsEmpty()) {
            textMesh.DrawText3D(frameData->GetThreadData(0), (RenderObject::TextDrawMode)visText.textParams.bits.textDrawMode,
                (RenderObject::TextAnchor)visText.textParams.bits.textAnchor, (RenderObject::TextHorzAlignment)visText.textParams.bits.textHorzAlignment,
                visText.textLineSpacing, visText.textScale, BE1::Color4::white.ToUInt32(), visText.textShadowColor, visText.textShadowOffset, visText.font, visText.text);
        } else {
            textMesh.DrawText3D(frameData->GetThreadData(0), (RenderObject::TextDrawMode)visText.textParams.bits.textDrawMode, visText.textRect,
                (RenderObject::TextHorzAlignment)visText.textParams.bits.textHorzAlignment, (RenderObject::TextVertAlignment)visText.textParams.bits.textVertAlignment,
                (RenderObject::TextHorzOverflow)visText.textParams.bits.textHorzOverflow, (RenderObject::TextVertOverflow)visText.textParams.bits.textVertOverflow,
                visText.textLineSpacing, visText.textScale, BE1::Color4::white.ToUInt32(), visText.textShadowColor, visText.textShadowOffset, visText.font, visText.text);
        }

        textMesh.CacheIndexes(frameData->GetThreadData(0));

        const int numSurfaces = textMesh.NumSurfaces();
        for (int surfaceIndex = 0; surfaceIndex < numSurfaces; surfaceIndex++) {
            const GuiMesh::Surface *guiSurf = textMesh.GetSurface(surfaceIndex);
            if (!guiSurf->numIndexes) {
                break;
            }

            SubMesh *subMesh = (SubMesh *)frameData->MemAlloc(sizeof(SubMesh));
            new (subMesh) SubMesh();

            subMesh->numIndexes = guiSurf->numIndexes;
            subMesh->numVerts = guiSurf->numVerts;
            subMesh->vertexBuffer = guiSurf->vertexBuffer;
            subMesh->indexBuffer = guiSurf->indexBuffer;

            AddVisSurf(frameData, visCamera, nullptr, visObject, guiSurf->material, subMesh, VisSurfFlag::CameraVisible);
        }
    }
}

void RenderWorld::AddSkyBoxMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
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

    VisObject *visObject = frameData->AllocVisObject();
    if (!visObject) {
        return;
    }

    visObject->worldMatrix.SetTRS(renderCamera->GetDesc().origin, BE1::Mat3::identity, BE1::Vec3(10, 10, 10));
    visObject->prevWorldMatrix = visObject->worldMatrix;
    visObject->flags |= VisObject::Flag::CameraVisible;
    Material *material = frameData->RetainMaterial(skyboxMaterial);

    AddVisSurf(frameData, visCamera, nullptr, visObject, material, meshSurf->subMesh, VisSurfFlag::CameraVisible | VisSurfFlag::SkipSelection | VisSurfFlag::Skybox);
}

void RenderWorld::FindVisLights(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::FindVisLights");

    const bool skipViewCulling = frameData->GetRenderFrameSettings().skipViewCulling;
    const RenderCamera::Desc &cameraDesc = renderCamera->GetDesc();
    const bool staticOnly = BE1::HasFlag(cameraDesc.flags, RenderCamera::Flag::StaticOnly);
    const BE1::Vec3 &viewZAxis = cameraDesc.axis[EngineAxis::Forward];
    const float viewZOffset = -viewZAxis.Dot(cameraDesc.origin);

    // 현재 카메라의 local light 참조가 시작되는 프레임 공용 ShaderLightRef 테이블의 오프셋을 기록한다.
    visCamera->lightCulling.lightRefTableOffset = frameData->NumVisLocalLights();

    // RenderWorld 에 등록된 renderLights 개수만큼 미리 visCamera->visLocalLights 를 할당
    visCamera->maxVisLocalLights = static_cast<uint32_t>(renderLights.Count());
    if (visCamera->maxVisLocalLights > 0) {
        visCamera->visLocalLights = (const VisLight **)frameData->MemAlloc(visCamera->maxVisLocalLights * sizeof(visCamera->visLocalLights[0]));
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
            if (frameData->GetPrimaryDirectionalLight()) {
                continue;
            }

            VisLight *visLight = RegisterVisLight(frameData, renderLight);
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

            VisLight *visLight = RegisterVisLight(frameData, renderLight);
            if (!visLight) {
                continue;
            }

            visLight->viewZMin = viewZMin;
            visLight->viewZMax = viewZMax;

            ComputeLightTileRange(visCamera, screenClipRect, visLight->packedTileMin, visLight->packedTileMax);

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

void RenderWorld::ComputeLightTileRange(const VisCamera *visCamera, const BE1::Rect &screenClipRect, uint16_t &packedTileMin, uint16_t &packedTileMax) {
    if (visCamera->outputRect.w <= 0 || visCamera->outputRect.h <= 0 || visCamera->renderWidth == 0 || visCamera->renderHeight == 0) {
        assert(0);
        packedTileMin = 0;
        packedTileMax = 0;
        return;
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
        return;
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
}

void RenderWorld::BuildLightCullingData(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::BuildLightCullingData");

    // 현재 카메라의 visible lights 를 minmax z 에 따라 정렬하고, z-bins 를 작성한다.
    BuildLightZBins(frameData, renderCamera, visCamera);

    if (visCamera->lightCulling.zBins) {
        // 현재 카메라의 Z-bin 이 시작되는 프레임 공용 light Z-bin 테이블을 예약한다.
        visCamera->lightCulling.zBinTableOffset = frameData->ReserveLightCullingZBins();
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
            visCamera->lightCulling.tileWordTableOffset = frameData->ReserveLightCullingWords(static_cast<uint32_t>(numLightCullingWords));
        }
    }
}

static BE_INLINE uint32_t PackLightZBin(uint32_t minLightIndex, uint32_t maxLightIndex) {
    assert(minLightIndex <= ShaderLightZBinIndexMask);
    assert(maxLightIndex <= ShaderLightZBinIndexMask);
    return (maxLightIndex << 16) | minLightIndex;
}

void RenderWorld::BuildLightZBins(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
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
    visCamera->lightCulling.zBins = (uint32_t *)frameData->MemAlloc(ShaderLightZBinCount * sizeof(visCamera->lightCulling.zBins[0]));

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

void RenderWorld::BuildDebugDraw(RenderFrameData *frameData, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::BuildDebugDraw");

    if (BE1::HasFlag(visCamera->flags, RenderCamera::Flag::SkipDebugDraw)) {
        return;
    }

    visCamera->showDebugLights = frameData->GetRenderFrameSettings().showDebugLights && visCamera->numVisLocalLights > 0;

    // 현재 debug primitive 저장소의 프레임 snapshot 을 카메라에 연결한다.
    visCamera->debugDrawData = BuildDebugDrawData(frameData);
    PrepareDebugDrawPipelines(visCamera);
}

const DebugDrawData *RenderWorld::BuildDebugDrawData(RenderFrameData *frameData) {
    static_assert(static_cast<uint32_t>(DebugDrawPipelineId::Count) == DebugDrawData::MaxBatches);
    if (debugPrimitiveVertexCount == 0) {
        assert(debugPrimitiveCount == 0);
        return nullptr;
    }

    RHI::VertexBuffer *vertexBuffer = frameData->GetThreadData(0)->AllocVertex(sizeof(DebugDrawVertex), debugPrimitiveVertexCount);
    if (!vertexBuffer || !vertexBuffer->mappedPtr) {
        BE_WARNLOG("RenderWorld::BuildDebugDrawData: failed to allocate debug primitive vertex buffer\n");
        return nullptr;
    }

    DebugDrawData *newDebugDrawData = new (frameData->MemAlloc(sizeof(DebugDrawData))) DebugDrawData();
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

    RenderFrameData *frameData = RenderContext::activeContext->GetCurrentFrontendFrameData();

    uint32_t screenWidth = RenderContext::activeContext->GetWidth();
    uint32_t screenHeight = RenderContext::activeContext->GetHeight();

    // guiMesh 를 기반으로 GUI VisCamera 를 구성하여, 렌더링 커맨드에 DrawCamera 명령을 기록한다.
    DrawGUICamera(frameData, screenWidth, screenHeight, guiMesh);
}

void RenderWorld::DrawGUICamera(RenderFrameData *frameData, uint32_t screenWidth, uint32_t screenHeight, GuiMesh &guiMesh) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::DrawGUICamera");

    VisCamera *visCamera = frameData->AllocVisCamera();
    if (!visCamera) {
        return;
    }

    visCamera->maxVisSurfs = guiMesh.NumSurfaces();
    visCamera->visSurfs = (const VisSurf **)frameData->MemAlloc(visCamera->maxVisSurfs * sizeof(VisSurf *));
    visCamera->numRenderPasses = 0;
    visCamera->renderPasses = (RenderPass *)frameData->MemAlloc(VisCamera::MaxRenderPasses * sizeof(RenderPass));
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
    visCamera->projectionDepthA = visCamera->viewProjMatrix[2][2];
    visCamera->projectionDepthB = visCamera->viewProjMatrix[2][3];
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

    VisObject *visObject = frameData->AllocVisObject();
    if (!visObject) {
        return;
    }

    visObject->worldMatrix.SetIdentity();
    visObject->prevWorldMatrix.SetIdentity();

    guiMesh.CacheIndexes(frameData->GetThreadData(0));

    const int numSurfaces = guiMesh.NumSurfaces();
    for (int surfaceIndex = 0; surfaceIndex < numSurfaces; surfaceIndex++) {
        const GuiMesh::Surface *guiSurf = guiMesh.GetSurface(surfaceIndex);
        if (!guiSurf->numIndexes) {
            break;
        }

        SubMesh *subMesh = (SubMesh *)frameData->MemAlloc(sizeof(SubMesh));
        new (subMesh) SubMesh();

        subMesh->numIndexes = guiSurf->numIndexes;
        subMesh->numVerts = guiSurf->numVerts;
        subMesh->vertexBuffer = guiSurf->vertexBuffer;
        subMesh->indexBuffer = guiSurf->indexBuffer;

        AddVisSurf(frameData, visCamera, nullptr, visObject, guiSurf->material, subMesh, VisSurfFlag::None);
    }

    guiMesh.Clear();

    renderPassBuilder.BuildGUI(frameData, visCamera);

    frameData->CmdDrawCamera(visCamera);

    viewCount++;
}
