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
#include "ScreenSpaceAO.h"
#include "Render/Backend/RenderBackend.h"
#include "Render/RenderInternal.h"
#include "Render/ShaderPipeline/ShaderPipeline.h"
#include "Render/ShaderPipeline/ShaderPipelineCache.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"
#include "Render/ShaderPipelineBootstrap.h"
#include "Shaders/ShaderInterop.h"

static_assert(RenderGTAODepthMipMaxLevelCount == GTAODepthMipLevelCount, "GTAO depth mip counts must match");

void ScreenSpaceAO::Init() {
    const char *baseDir = "Source/TestD3D12/Shaders";
    const RHI::ShaderModel computeShaderModel = RHI::Renderer::MinimumShaderModel();

    aoBilateralHorizontalShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_aoBilateralHorizontalShader", computeShaderModel, "AO/AOBilateral.hlsl", baseDir, "CSMainHorizontal");
    aoBilateralVerticalShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_aoBilateralVerticalShader", computeShaderModel, "AO/AOBilateral.hlsl", baseDir, "CSMainVertical");
    aoUpsampleShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_aoUpsampleShader", computeShaderModel, "AO/AOUpsample.hlsl", baseDir);

    ssao16FullResShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_ssao16FullResShader", computeShaderModel, "AO/SSAO.hlsl", baseDir);
    ssao16HalfResShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_ssao16HalfResShader", computeShaderModel, "AO/SSAO.hlsl", baseDir, "CSMainHalfResolution");
    ssao8FullResShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_ssao8FullResShader", computeShaderModel, "AO/SSAO.hlsl", baseDir, "CSMain", "SSAO_SAMPLE_COUNT=8");
    ssao8HalfResShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_ssao8HalfResShader", computeShaderModel, "AO/SSAO.hlsl", baseDir, "CSMainHalfResolution", "SSAO_SAMPLE_COUNT=8");

    gtaoPrefilterShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_gtaoPrefilterShader", computeShaderModel, "AO/GTAOPrefilter.hlsl", baseDir);
    gtaoMainLowShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_gtaoMainLowShader", computeShaderModel, "AO/GTAOMain.hlsl", baseDir, "CSMain", "GTAO_SLICE_COUNT=1, GTAO_STEPS_PER_SLICE=2");
    gtaoMainMediumShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_gtaoMainMediumShader", computeShaderModel, "AO/GTAOMain.hlsl", baseDir, "CSMain", "GTAO_SLICE_COUNT=2, GTAO_STEPS_PER_SLICE=2");
    gtaoMainHighShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_gtaoMainHighShader", computeShaderModel, "AO/GTAOMain.hlsl", baseDir, "CSMain", "GTAO_SLICE_COUNT=3, GTAO_STEPS_PER_SLICE=3");
    gtaoMainUltraShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_gtaoMainUltraShader", computeShaderModel, "AO/GTAOMain.hlsl", baseDir, "CSMain", "GTAO_SLICE_COUNT=9, GTAO_STEPS_PER_SLICE=3");
    gtaoDenoiseShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_gtaoDenoiseShader", computeShaderModel, "AO/GTAODenoise.hlsl", baseDir);
    gtaoDebugShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_gtaoDebugShader", computeShaderModel, "AO/GTAODebug.hlsl", baseDir);

    aoBilateralHorizontalPipeline = shaderPipelineCache.GetComputePipeline(aoBilateralHorizontalShaderTemplate, ShaderPermutationKey(), true);
    aoBilateralVerticalPipeline = shaderPipelineCache.GetComputePipeline(aoBilateralVerticalShaderTemplate, ShaderPermutationKey(), true);
    aoUpsamplePipeline = shaderPipelineCache.GetComputePipeline(aoUpsampleShaderTemplate, ShaderPermutationKey(), true);

    ssao16FullResPipeline = shaderPipelineCache.GetComputePipeline(ssao16FullResShaderTemplate, ShaderPermutationKey(), true);
    ssao16HalfResPipeline = shaderPipelineCache.GetComputePipeline(ssao16HalfResShaderTemplate, ShaderPermutationKey(), true);
    ssao8FullResPipeline = shaderPipelineCache.GetComputePipeline(ssao8FullResShaderTemplate, ShaderPermutationKey(), true);
    ssao8HalfResPipeline = shaderPipelineCache.GetComputePipeline(ssao8HalfResShaderTemplate, ShaderPermutationKey(), true);

    gtaoPrefilterPipeline = shaderPipelineCache.GetComputePipeline(gtaoPrefilterShaderTemplate, ShaderPermutationKey(), true);
    gtaoMainLowPipeline = shaderPipelineCache.GetComputePipeline(gtaoMainLowShaderTemplate, ShaderPermutationKey(), true);
    gtaoMainMediumPipeline = shaderPipelineCache.GetComputePipeline(gtaoMainMediumShaderTemplate, ShaderPermutationKey(), true);
    gtaoMainHighPipeline = shaderPipelineCache.GetComputePipeline(gtaoMainHighShaderTemplate, ShaderPermutationKey(), true);
    gtaoMainUltraPipeline = shaderPipelineCache.GetComputePipeline(gtaoMainUltraShaderTemplate, ShaderPermutationKey(), true);
    gtaoDenoisePipeline = shaderPipelineCache.GetComputePipeline(gtaoDenoiseShaderTemplate, ShaderPermutationKey(), true);
    gtaoDebugPipeline = shaderPipelineCache.GetComputePipeline(gtaoDebugShaderTemplate, ShaderPermutationKey(), true);
}

void ScreenSpaceAO::TrackBootstrapPipelines(ShaderPipelineBootstrap &bootstrap) const {
    static constexpr char groupName[] = "ScreenSpaceAO";

    bootstrap.TrackPipeline(groupName, aoBilateralHorizontalPipeline);
    bootstrap.TrackPipeline(groupName, aoBilateralVerticalPipeline);
    bootstrap.TrackPipeline(groupName, aoUpsamplePipeline);
    bootstrap.TrackPipeline(groupName, ssao16FullResPipeline);
    bootstrap.TrackPipeline(groupName, ssao16HalfResPipeline);
    bootstrap.TrackPipeline(groupName, ssao8FullResPipeline);
    bootstrap.TrackPipeline(groupName, ssao8HalfResPipeline);
    bootstrap.TrackPipeline(groupName, gtaoPrefilterPipeline);
    bootstrap.TrackPipeline(groupName, gtaoMainLowPipeline);
    bootstrap.TrackPipeline(groupName, gtaoMainMediumPipeline);
    bootstrap.TrackPipeline(groupName, gtaoMainHighPipeline);
    bootstrap.TrackPipeline(groupName, gtaoMainUltraPipeline);
    bootstrap.TrackPipeline(groupName, gtaoDenoisePipeline);
    bootstrap.TrackPipeline(groupName, gtaoDebugPipeline);
}

void ScreenSpaceAO::Shutdown() {
    aoBilateralHorizontalShaderTemplate.reset();
    aoBilateralVerticalShaderTemplate.reset();
    aoUpsampleShaderTemplate.reset();
    ssao16FullResShaderTemplate.reset();
    ssao16HalfResShaderTemplate.reset();
    ssao8FullResShaderTemplate.reset();
    ssao8HalfResShaderTemplate.reset();
    gtaoPrefilterShaderTemplate.reset();
    gtaoMainLowShaderTemplate.reset();
    gtaoMainMediumShaderTemplate.reset();
    gtaoMainHighShaderTemplate.reset();
    gtaoMainUltraShaderTemplate.reset();
    gtaoDenoiseShaderTemplate.reset();
    gtaoDebugShaderTemplate.reset();

    aoBilateralHorizontalPipeline.reset();
    aoBilateralVerticalPipeline.reset();
    aoUpsamplePipeline.reset();
    ssao16FullResPipeline.reset();
    ssao16HalfResPipeline.reset();
    ssao8FullResPipeline.reset();
    ssao8HalfResPipeline.reset();
    gtaoPrefilterPipeline.reset();
    gtaoMainLowPipeline.reset();
    gtaoMainMediumPipeline.reset();
    gtaoMainHighPipeline.reset();
    gtaoMainUltraPipeline.reset();
    gtaoDenoisePipeline.reset();
    gtaoDebugPipeline.reset();

    frameAOBilateralHorizontalPSO = nullptr;
    frameAOBilateralVerticalPSO = nullptr;
    frameAOUpsamplePSO = nullptr;
    frameSSAOPSO = nullptr;
    frameSSAOHalfResolutionPSO = nullptr;
    frameGTAOPrefilterPSO = nullptr;
    frameGTAOMainPSO = nullptr;
    frameGTAODenoisePSO = nullptr;
    frameGTAODebugPSO = nullptr;
}

void ScreenSpaceAO::UpdateFramePSOs(const RenderFrameSettings &settings) {
    const bool useLowQualitySSAO = settings.ssaoQuality == 0;

    frameSSAOPSO = ShaderPipeline::GetReadyPSO(useLowQualitySSAO ? ssao8FullResPipeline : ssao16FullResPipeline);
    frameSSAOHalfResolutionPSO = ShaderPipeline::GetReadyPSO(useLowQualitySSAO ? ssao8HalfResPipeline : ssao16HalfResPipeline);
    frameAOBilateralHorizontalPSO = ShaderPipeline::GetReadyPSO(aoBilateralHorizontalPipeline);
    frameAOBilateralVerticalPSO = ShaderPipeline::GetReadyPSO(aoBilateralVerticalPipeline);
    frameAOUpsamplePSO = ShaderPipeline::GetReadyPSO(aoUpsamplePipeline);
    frameGTAOPrefilterPSO = ShaderPipeline::GetReadyPSO(gtaoPrefilterPipeline);

    const uint32_t gtaoQuality = BE1::Min(settings.gtaoQuality, GTAOQualityLevelCount - 1u);
    switch (gtaoQuality) {
    case 0:
        frameGTAOMainPSO = ShaderPipeline::GetReadyPSO(gtaoMainLowPipeline);
        break;
    case 1:
        frameGTAOMainPSO = ShaderPipeline::GetReadyPSO(gtaoMainMediumPipeline);
        break;
    case 2:
        frameGTAOMainPSO = ShaderPipeline::GetReadyPSO(gtaoMainHighPipeline);
        break;
    case 3:
        frameGTAOMainPSO = ShaderPipeline::GetReadyPSO(gtaoMainUltraPipeline);
        break;
    }
    frameGTAODenoisePSO = ShaderPipeline::GetReadyPSO(gtaoDenoisePipeline);
    frameGTAODebugPSO = ShaderPipeline::GetReadyPSO(gtaoDebugPipeline);
}

bool ScreenSpaceAO::HasRequiredResources(const ScreenSpaceAOFrameData &data) {
    const ScreenSpaceAOTextures &textures = data.textures;
    if (!data.depthTexture || !data.normalTexture || !textures.outputTexture) {
        return false;
    }

    switch (data.mode) {
    case RenderAOMode::SSAO:
        return true;
    case RenderAOMode::GTAO:
        assert(!textures.gtaoPrefilterDepthTexture || textures.gtaoDepthMipCount > 0);
        return textures.rawTexture && textures.gtaoPrefilterDepthTexture && textures.gtaoPackedEdgesTexture;
    default:
        return false;
    }
}

bool ScreenSpaceAO::PrepareDispatch(const ScreenSpaceAOFrameData &data, const RenderFrameSettings &settings, DispatchParams &params) const {
    assert(HasRequiredResources(data));
    params = {};

    const ScreenSpaceAOTextures &textures = data.textures;

    if (data.mode == RenderAOMode::SSAO) {
        // Keep full-resolution AO available while half-resolution resources are not ready.
        params.useHalfResolution = settings.aoHalfResolution && frameSSAOHalfResolutionPSO && frameAOUpsamplePSO && textures.rawTexture && textures.halfDepthTexture;

        // Full-resolution fallback 에서는 half-resolution 중간 텍스처를 필터에 사용하지 않는다.
        const bool fullResolutionFallback = settings.aoHalfResolution && !params.useHalfResolution;
        params.useBlur = settings.ssaoBilateral && !fullResolutionFallback && frameAOBilateralHorizontalPSO && frameAOBilateralVerticalPSO && textures.rawTexture && textures.tempTexture;
        return (params.useHalfResolution ? frameSSAOHalfResolutionPSO : frameSSAOPSO) != nullptr;
    }

    if (!frameGTAOPrefilterPSO || !frameGTAOMainPSO || !frameGTAODenoisePSO) {
        return false;
    }

    // GTAO resources use the requested resolution; there is no full-resolution fallback.
    params.useHalfResolution = settings.aoHalfResolution;
    if (params.useHalfResolution && (!frameAOUpsamplePSO || !textures.halfDepthTexture)) {
        return false;
    }

    // Preserve raw visibility for the debug preview; ping-pong denoise can otherwise overwrite it.
    params.denoisePassCount = settings.gtaoDebug == 2 ? 1u : BE1::Max(settings.gtaoDenoisePasses, 1u);

    const bool needTempTexture = params.useHalfResolution || params.denoisePassCount > 1;
    return !needTempTexture || textures.tempTexture;
}

void ScreenSpaceAO::Dispatch(RenderBackend &backend, const DispatchParams &params) const {
    const ScreenSpaceAOFrameData &data = backend.GetVisCamera()->ao;

    switch (data.mode) {
    case RenderAOMode::SSAO:
        DispatchSSAO(backend, params);
        break;
    case RenderAOMode::GTAO:
        DispatchGTAO(backend, params);
        break;
    default:
        assert(false);
        break;
    }
}
