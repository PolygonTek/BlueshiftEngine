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
#include "Bloom.h"
#include "Render/Backend/RenderBackend.h"
#include "Render/RenderInternal.h"
#include "Render/ShaderPipeline/ShaderPipeline.h"
#include "Render/ShaderPipeline/ShaderPipelineCache.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"
#include "Render/ShaderPipelineBootstrap.h"

void Bloom::Init() {
    const char *baseDir = "Source/TestD3D12/Shaders";
    const RHI::ShaderModel computeShaderModel = RHI::Renderer::MinimumShaderModel();

    bloomPrefilterShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_bloomPrefilterShader", computeShaderModel, "Bloom/BloomPrefilter.hlsl", baseDir);
    bloomDownsampleShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_bloomDownsampleShader", computeShaderModel, "Bloom/BloomDownsample.hlsl", baseDir);
    bloomUpsampleShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_bloomUpsampleShader", computeShaderModel, "Bloom/BloomUpsample.hlsl", baseDir);
    convolutionBloomScreenPrepareShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_convolutionBloomScreenPrepareShader", computeShaderModel, "Bloom/ConvolutionBloomScreenPrepare.hlsl", baseDir);
    convolutionBloomKernelPrepareShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_convolutionBloomKernelPrepareShader", computeShaderModel, "Bloom/ConvolutionBloomKernelPrepare.hlsl", baseDir);
    convolutionBloomResolveShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_convolutionBloomResolveShader", computeShaderModel, "Bloom/ConvolutionBloomResolve.hlsl", baseDir);

    bloomPrefilterPipeline = shaderPipelineCache.GetComputePipeline(bloomPrefilterShaderTemplate, ShaderPermutationKey(), true);
    bloomDownsamplePipeline = shaderPipelineCache.GetComputePipeline(bloomDownsampleShaderTemplate, ShaderPermutationKey(), true);
    bloomUpsamplePipeline = shaderPipelineCache.GetComputePipeline(bloomUpsampleShaderTemplate, ShaderPermutationKey(), true);
    convolutionBloomScreenPreparePipeline = shaderPipelineCache.GetComputePipeline(convolutionBloomScreenPrepareShaderTemplate, ShaderPermutationKey(), true);
    convolutionBloomKernelPreparePipeline = shaderPipelineCache.GetComputePipeline(convolutionBloomKernelPrepareShaderTemplate, ShaderPermutationKey(), true);
    convolutionBloomResolvePipeline = shaderPipelineCache.GetComputePipeline(convolutionBloomResolveShaderTemplate, ShaderPermutationKey(), true);
}

void Bloom::Shutdown() {
    bloomPrefilterPipeline.reset();
    bloomPrefilterShaderTemplate.reset();
    frameBloomPrefilterPSO = nullptr;
    bloomDownsamplePipeline.reset();
    bloomDownsampleShaderTemplate.reset();
    frameBloomDownsamplePSO = nullptr;
    bloomUpsamplePipeline.reset();
    bloomUpsampleShaderTemplate.reset();
    frameBloomUpsamplePSO = nullptr;
    convolutionBloomScreenPreparePipeline.reset();
    convolutionBloomScreenPrepareShaderTemplate.reset();
    frameConvolutionBloomScreenPreparePSO = nullptr;
    convolutionBloomKernelPreparePipeline.reset();
    convolutionBloomKernelPrepareShaderTemplate.reset();
    frameConvolutionBloomKernelPreparePSO = nullptr;
    convolutionBloomResolvePipeline.reset();
    convolutionBloomResolveShaderTemplate.reset();
    frameConvolutionBloomResolvePSO = nullptr;
}

void Bloom::TrackBootstrapPipelines(ShaderPipelineBootstrap &bootstrap) const {
    static constexpr char groupName[] = "Bloom";

    bootstrap.TrackPipeline(groupName, bloomPrefilterPipeline);
    bootstrap.TrackPipeline(groupName, bloomDownsamplePipeline);
    bootstrap.TrackPipeline(groupName, bloomUpsamplePipeline);
    bootstrap.TrackPipeline(groupName, convolutionBloomScreenPreparePipeline);
    bootstrap.TrackPipeline(groupName, convolutionBloomKernelPreparePipeline);
    bootstrap.TrackPipeline(groupName, convolutionBloomResolvePipeline);
}

void Bloom::UpdateFramePSOs() {
    frameBloomPrefilterPSO = ShaderPipeline::GetReadyPSO(bloomPrefilterPipeline);
    frameBloomDownsamplePSO = ShaderPipeline::GetReadyPSO(bloomDownsamplePipeline);
    frameBloomUpsamplePSO = ShaderPipeline::GetReadyPSO(bloomUpsamplePipeline);
    frameConvolutionBloomScreenPreparePSO = ShaderPipeline::GetReadyPSO(convolutionBloomScreenPreparePipeline);
    frameConvolutionBloomKernelPreparePSO = ShaderPipeline::GetReadyPSO(convolutionBloomKernelPreparePipeline);
    frameConvolutionBloomResolvePSO = ShaderPipeline::GetReadyPSO(convolutionBloomResolvePipeline);
}

bool Bloom::Dispatch(RenderBackend &backend) const {
    PROFILER_CPU_SCOPED_EVENT("Bloom::Dispatch");

    const RenderFrameSettings &renderFrameSettings = backend.GetFrameSettings();

    const RenderBloomMode bloomMode = renderFrameSettings.bloomMode;
    switch (bloomMode) {
    case RenderBloomMode::Pyramid:
        if (!frameBloomPrefilterPSO || !frameBloomDownsamplePSO || !frameBloomUpsamplePSO) {
            return false;
        }
        return DispatchPyramidBloom(backend);
    case RenderBloomMode::Convolution:
        if (!frameBloomPrefilterPSO || !frameConvolutionBloomScreenPreparePSO || !frameConvolutionBloomKernelPreparePSO || !frameConvolutionBloomResolvePSO) {
            return false;
        }
        return DispatchConvolutionBloom(backend);
    default:
        return false;
    }
}
