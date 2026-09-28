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
#include "PostLuminance.h"
#include "Render/Backend/RenderBackend.h"
#include "Render/GpuTiming.h"
#include "Render/RenderInternal.h"
#include "Render/ShaderPipeline/ShaderPipeline.h"
#include "Render/ShaderPipeline/ShaderPipelineCache.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"
#include "Render/ShaderPipelineBootstrap.h"
#include "Shaders/ShaderInterop.h"

void PostLuminance::Init() {
    const char *baseDir = "Source/TestD3D12/Shaders";
    const RHI::ShaderModel computeShaderModel = RHI::Renderer::MinimumShaderModel();

    luminancePass1ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_luminancePass1Shader", computeShaderModel, "ToneMapping/LuminancePass1.hlsl", baseDir);
    luminancePass2ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_luminancePass2Shader", computeShaderModel, "ToneMapping/LuminancePass2.hlsl", baseDir);

    luminancePass1Pipeline = shaderPipelineCache.GetComputePipeline(luminancePass1ShaderTemplate, ShaderPermutationKey(), true);
    luminancePass2Pipeline = shaderPipelineCache.GetComputePipeline(luminancePass2ShaderTemplate, ShaderPermutationKey(), true);
}

void PostLuminance::Shutdown() {
    luminancePass1ShaderTemplate.reset();
    luminancePass2ShaderTemplate.reset();

    luminancePass1Pipeline.reset();
    luminancePass2Pipeline.reset();

    frameLuminancePass1PSO = nullptr;
    frameLuminancePass2PSO = nullptr;
}

void PostLuminance::TrackBootstrapPipelines(ShaderPipelineBootstrap &bootstrap) const {
    static constexpr char groupName[] = "PostLuminance";

    bootstrap.TrackPipeline(groupName, luminancePass1Pipeline);
    bootstrap.TrackPipeline(groupName, luminancePass2Pipeline);
}

void PostLuminance::UpdateFramePSOs() {
    frameLuminancePass1PSO = ShaderPipeline::GetReadyPSO(luminancePass1Pipeline);
    frameLuminancePass2PSO = ShaderPipeline::GetReadyPSO(luminancePass2Pipeline);
}

// 히스토그램 기반 luminance 계산: https://www.alextardif.com/HistogramLuminance.html
bool PostLuminance::Dispatch(RenderBackend &backend) const {
    PROFILER_CPU_SCOPED_EVENT("PostLuminance::Dispatch");

    RHI::CommandList *commandList = backend.GetCommandList();
    const RHI::ConstantBuffer *frameConstantBuffer = backend.GetFrameConstantBuffer();
    GpuTiming &gpuTiming = backend.GetGpuTiming();
    RHI::Texture *inputTexture = backend.GetPostProcessInputTexture();
    RHI::Buffer *luminanceBuffer = backend.GetFrameCamera()->luminanceBuffer;
    const RenderFrameSettings &settings = backend.GetFrameSettings();

    if (!settings.toneMapping || !frameLuminancePass1PSO || !frameLuminancePass2PSO) {
        return false;
    }

    if (!inputTexture || !luminanceBuffer) {
        return false;
    }

    const uint32_t inputW = inputTexture->GetWidth();
    const uint32_t inputH = inputTexture->GetHeight();
    if (inputW == 0 || inputH == 0) {
        return false;
    }

    const uint32_t sampleW = BE1::Max(inputW / 2, 1u);
    const uint32_t sampleH = BE1::Max(inputH / 2, 1u);
    const float lumLogMin = settings.toneMapLuminanceLogMin;
    const float lumLogRange = settings.toneMapLuminanceLogMax - lumLogMin;

    LuminancePass1Params pass1Params = {};
    pass1Params.sampleSize = uint2(sampleW, sampleH);
    pass1Params.sampleSizeRcp = float2(1.0f / sampleW, 1.0f / sampleH);
    pass1Params.luminanceLogMin = lumLogMin;
    pass1Params.luminanceLogRangeRcp = 1.0f / lumLogRange;

    LuminancePass2Params pass2Params = {};
    pass2Params.luminanceLogMin = lumLogMin;
    pass2Params.luminanceLogRange = lumLogRange;
    pass2Params.luminancePixelCount = static_cast<float>(sampleW * sampleH);
    pass2Params.luminanceAdaptationRate = settings.toneMapLuminanceAdaptationRate;
    pass2Params.middleGray = settings.toneMapMiddleGray;
    pass2Params.exposureCompensation = settings.toneMapExposureCompensation;
    pass2Params.minExposure = BE1::Math::Pow(2.0f, settings.toneMapMinExposureEV);
    pass2Params.maxExposure = BE1::Math::Pow(2.0f, settings.toneMapMaxExposureEV);

    // Pass 1 : Log Luminance 히스토그램 계산
    {
        GpuTiming::ScopedTimer gpuScope(gpuTiming, commandList, GpuTiming::Tag::LuminancePass1);

        RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute));

        RHI::renderer->SetPSO(commandList, frameLuminancePass1PSO);
        RHI::renderer->SetTexture(commandList, 0, false, inputTexture);
        RHI::renderer->SetBuffer(commandList, 0, true, luminanceBuffer);
        RHI::renderer->SetConstants(commandList, 1, &pass1Params, sizeof(pass1Params), 0);

        const uint32_t threadGroupCountX = BE1::DivCeil(pass1Params.sampleSize.x, LuminanceBlockSize);
        const uint32_t threadGroupCountY = BE1::DivCeil(pass1Params.sampleSize.y, LuminanceBlockSize);
        RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);
    }

    // Pass 2 : 평균 Luminance 를 계산하고, 보간된 exposure 도 계산한다.
    {
        GpuTiming::ScopedTimer gpuScope(gpuTiming, commandList, GpuTiming::Tag::LuminancePass2);

        RHI::renderer->Barrier(commandList, RHI::Renderer::MakeMemoryBarrier(luminanceBuffer));

        RHI::renderer->SetPSO(commandList, frameLuminancePass2PSO);
        RHI::renderer->SetBuffer(commandList, 0, true, luminanceBuffer);
        RHI::renderer->SetConstantBuffer(commandList, 0, frameConstantBuffer);
        RHI::renderer->SetConstants(commandList, 1, &pass2Params, sizeof(pass2Params), 0);

        RHI::renderer->Dispatch(commandList, 1, 1, 1);

        RHI::GPUBarrier endBarriers[] = {
            RHI::Renderer::MakeMemoryBarrier(luminanceBuffer),
            RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource)
        };
        RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));
    }

    return true;
}
