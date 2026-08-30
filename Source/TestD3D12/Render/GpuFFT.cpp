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
#include "GpuFFT.h"
#include "ShaderPipelineBootstrap.h"
#include "Render/ShaderPipeline/ShaderPipeline.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"
#include "Render/ShaderPipeline/ShaderPipelineCache.h"
#include "Shaders/ShaderInterop.h"

struct FFTDispatchEntry {
    uint32_t                fftSize = 0;

    ShaderPipeline *        fftComplexPipeline = nullptr;
    ShaderPipeline *        ifftComplexPipeline = nullptr;
    ShaderPipeline *        fftRealPipeline = nullptr;
    ShaderPipeline *        ifftRealPipeline = nullptr;
    ShaderPipeline *        fftDualRealPipeline = nullptr;
    ShaderPipeline *        ifftDualRealPipeline = nullptr;

    ShaderPipeline *        transposeComplexPipeline = nullptr;
    ShaderPipeline *        transposeComplexPackedToFullPipeline = nullptr;
    ShaderPipeline *        transposeComplexFullToPackedPipeline = nullptr;

    RHI::Buffer *           fftRealTwiddleBuffer = nullptr;
};

static FFTDispatchEntry     fftDispatchTable[static_cast<uint32_t>(GpuFFT::FFTSize::Count)];

void GpuFFT::Init() {
    const char *baseDir = "Source/TestD3D12/Shaders";
    const RHI::ShaderModel shaderModel = RHI::Renderer::MinimumShaderModel();
    constexpr bool asyncPipelineBuild = true;

    fftComplex256ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_fftComplex256Shader", shaderModel, "FFT/FFTComplex256.hlsl", baseDir, "CSMain", "FFT_INVERSE=0");
    fftComplex512ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_fftComplex512Shader", shaderModel, "FFT/FFTComplex512.hlsl", baseDir, "CSMain", "FFT_INVERSE=0");
    fftComplex1024ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_fftComplex1024Shader", shaderModel, "FFT/FFTComplex1024.hlsl", baseDir, "CSMain", "FFT_INVERSE=0");
    fftComplex2048ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_fftComplex2048Shader", shaderModel, "FFT/FFTComplex2048.hlsl", baseDir, "CSMain", "FFT_INVERSE=0");

    ifftComplex256ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_ifftComplex256Shader", shaderModel, "FFT/FFTComplex256.hlsl", baseDir, "CSMain", "FFT_INVERSE=1");
    ifftComplex512ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_ifftComplex512Shader", shaderModel, "FFT/FFTComplex512.hlsl", baseDir, "CSMain", "FFT_INVERSE=1");
    ifftComplex1024ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_ifftComplex1024Shader", shaderModel, "FFT/FFTComplex1024.hlsl", baseDir, "CSMain", "FFT_INVERSE=1");
    ifftComplex2048ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_ifftComplex2048Shader", shaderModel, "FFT/FFTComplex2048.hlsl", baseDir, "CSMain", "FFT_INVERSE=1");

    fftReal512ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_fftReal512Shader", shaderModel, "FFT/FFTReal512.hlsl", baseDir);
    fftReal1024ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_fftReal1024Shader", shaderModel, "FFT/FFTReal1024.hlsl", baseDir);
    fftReal2048ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_fftReal2048Shader", shaderModel, "FFT/FFTReal2048.hlsl", baseDir);

    ifftReal512ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_ifftReal512Shader", shaderModel, "FFT/IFFTReal512.hlsl", baseDir);
    ifftReal1024ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_ifftReal1024Shader", shaderModel, "FFT/IFFTReal1024.hlsl", baseDir);
    ifftReal2048ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_ifftReal2048Shader", shaderModel, "FFT/IFFTReal2048.hlsl", baseDir);

    fftDualReal256ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_fftDualReal256Shader", shaderModel, "FFT/FFTDualReal256.hlsl", baseDir);
    fftDualReal512ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_fftDualReal512Shader", shaderModel, "FFT/FFTDualReal512.hlsl", baseDir);
    fftDualReal1024ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_fftDualReal1024Shader", shaderModel, "FFT/FFTDualReal1024.hlsl", baseDir);
    fftDualReal2048ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_fftDualReal2048Shader", shaderModel, "FFT/FFTDualReal2048.hlsl", baseDir);

    ifftDualReal256ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_ifftDualReal256Shader", shaderModel, "FFT/IFFTDualReal256.hlsl", baseDir);
    ifftDualReal512ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_ifftDualReal512Shader", shaderModel, "FFT/IFFTDualReal512.hlsl", baseDir);
    ifftDualReal1024ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_ifftDualReal1024Shader", shaderModel, "FFT/IFFTDualReal1024.hlsl", baseDir);
    ifftDualReal2048ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_ifftDualReal2048Shader", shaderModel, "FFT/IFFTDualReal2048.hlsl", baseDir);

    multiplyComplexSpectrum2DShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_multiplyComplexSpectrum2DShader", shaderModel, "FFT/MultiplyComplexSpectrum2D.hlsl", baseDir);
    multiplyPackedRealSpectrum2DShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_multiplyPackedRealSpectrum2DShader", shaderModel, "FFT/MultiplyPackedRealSpectrum2D.hlsl", baseDir);
    multiplyPackedDualRealSpectrum2DShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_multiplyPackedDualRealSpectrum2DShader", shaderModel, "FFT/MultiplyPackedDualRealSpectrum2D.hlsl", baseDir);

    transposeComplex256ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_transposeComplex256Shader", shaderModel, "Transpose/TransposeComplex256.hlsl", baseDir);
    transposeComplex512ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_transposeComplex512Shader", shaderModel, "Transpose/TransposeComplex512.hlsl", baseDir);
    transposeComplex1024ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_transposeComplex1024Shader", shaderModel, "Transpose/TransposeComplex1024.hlsl", baseDir);
    transposeComplex2048ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_transposeComplex2048Shader", shaderModel, "Transpose/TransposeComplex2048.hlsl", baseDir);
    transposeComplex256x512ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_transposeComplex256x512Shader", shaderModel, "Transpose/TransposeComplex256x512.hlsl", baseDir);
    transposeComplex512x256ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_transposeComplex512x256Shader", shaderModel, "Transpose/TransposeComplex512x256.hlsl", baseDir);
    transposeComplex512x1024ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_transposeComplex512x1024Shader", shaderModel, "Transpose/TransposeComplex512x1024.hlsl", baseDir);
    transposeComplex1024x512ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_transposeComplex1024x512Shader", shaderModel, "Transpose/TransposeComplex1024x512.hlsl", baseDir);
    transposeComplex1024x2048ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_transposeComplex1024x2048Shader", shaderModel, "Transpose/TransposeComplex1024x2048.hlsl", baseDir);
    transposeComplex2048x1024ShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_transposeComplex2048x1024Shader", shaderModel, "Transpose/TransposeComplex2048x1024.hlsl", baseDir);

    fftComplex256Pipeline = shaderPipelineCache.GetComputePipeline(fftComplex256ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    fftComplex512Pipeline = shaderPipelineCache.GetComputePipeline(fftComplex512ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    fftComplex1024Pipeline = shaderPipelineCache.GetComputePipeline(fftComplex1024ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    fftComplex2048Pipeline = shaderPipelineCache.GetComputePipeline(fftComplex2048ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);

    ifftComplex256Pipeline = shaderPipelineCache.GetComputePipeline(ifftComplex256ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    ifftComplex512Pipeline = shaderPipelineCache.GetComputePipeline(ifftComplex512ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    ifftComplex1024Pipeline = shaderPipelineCache.GetComputePipeline(ifftComplex1024ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    ifftComplex2048Pipeline = shaderPipelineCache.GetComputePipeline(ifftComplex2048ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);

    fftReal512Pipeline = shaderPipelineCache.GetComputePipeline(fftReal512ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    fftReal1024Pipeline = shaderPipelineCache.GetComputePipeline(fftReal1024ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    fftReal2048Pipeline = shaderPipelineCache.GetComputePipeline(fftReal2048ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);

    ifftReal512Pipeline = shaderPipelineCache.GetComputePipeline(ifftReal512ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    ifftReal1024Pipeline = shaderPipelineCache.GetComputePipeline(ifftReal1024ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    ifftReal2048Pipeline = shaderPipelineCache.GetComputePipeline(ifftReal2048ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);

    fftDualReal256Pipeline = shaderPipelineCache.GetComputePipeline(fftDualReal256ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    fftDualReal512Pipeline = shaderPipelineCache.GetComputePipeline(fftDualReal512ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    fftDualReal1024Pipeline = shaderPipelineCache.GetComputePipeline(fftDualReal1024ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    fftDualReal2048Pipeline = shaderPipelineCache.GetComputePipeline(fftDualReal2048ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);

    ifftDualReal256Pipeline = shaderPipelineCache.GetComputePipeline(ifftDualReal256ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    ifftDualReal512Pipeline = shaderPipelineCache.GetComputePipeline(ifftDualReal512ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    ifftDualReal1024Pipeline = shaderPipelineCache.GetComputePipeline(ifftDualReal1024ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    ifftDualReal2048Pipeline = shaderPipelineCache.GetComputePipeline(ifftDualReal2048ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);

    multiplyComplexSpectrum2DPipeline = shaderPipelineCache.GetComputePipeline(multiplyComplexSpectrum2DShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    multiplyPackedRealSpectrum2DPipeline = shaderPipelineCache.GetComputePipeline(multiplyPackedRealSpectrum2DShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    multiplyPackedDualRealSpectrum2DPipeline = shaderPipelineCache.GetComputePipeline(multiplyPackedDualRealSpectrum2DShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);

    transposeComplex256Pipeline = shaderPipelineCache.GetComputePipeline(transposeComplex256ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    transposeComplex512Pipeline = shaderPipelineCache.GetComputePipeline(transposeComplex512ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    transposeComplex1024Pipeline = shaderPipelineCache.GetComputePipeline(transposeComplex1024ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    transposeComplex2048Pipeline = shaderPipelineCache.GetComputePipeline(transposeComplex2048ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    transposeComplex256x512Pipeline = shaderPipelineCache.GetComputePipeline(transposeComplex256x512ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    transposeComplex512x256Pipeline = shaderPipelineCache.GetComputePipeline(transposeComplex512x256ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    transposeComplex512x1024Pipeline = shaderPipelineCache.GetComputePipeline(transposeComplex512x1024ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    transposeComplex1024x512Pipeline = shaderPipelineCache.GetComputePipeline(transposeComplex1024x512ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    transposeComplex1024x2048Pipeline = shaderPipelineCache.GetComputePipeline(transposeComplex1024x2048ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    transposeComplex2048x1024Pipeline = shaderPipelineCache.GetComputePipeline(transposeComplex2048x1024ShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);

    fftRealTwiddle512Buffer = CreateRealFFTTwiddleBuffer(512);
    fftRealTwiddle1024Buffer = CreateRealFFTTwiddleBuffer(1024);
    fftRealTwiddle2048Buffer = CreateRealFFTTwiddleBuffer(2048);

    fftDispatchTable[static_cast<uint32_t>(FFTSize::FFT256)] = FFTDispatchEntry{
        256,
        fftComplex256Pipeline.get(),
        ifftComplex256Pipeline.get(),
        nullptr,
        nullptr,
        fftDualReal256Pipeline.get(),
        ifftDualReal256Pipeline.get(),
        transposeComplex256Pipeline.get(),
        nullptr,
        nullptr,
        nullptr
    };

    fftDispatchTable[static_cast<uint32_t>(FFTSize::FFT512)] = FFTDispatchEntry{
        512,
        fftComplex512Pipeline.get(),
        ifftComplex512Pipeline.get(),
        fftReal512Pipeline.get(),
        ifftReal512Pipeline.get(),
        fftDualReal512Pipeline.get(),
        ifftDualReal512Pipeline.get(),
        transposeComplex512Pipeline.get(),
        transposeComplex256x512Pipeline.get(),
        transposeComplex512x256Pipeline.get(),
        fftRealTwiddle512Buffer
    };

    fftDispatchTable[static_cast<uint32_t>(FFTSize::FFT1024)] = FFTDispatchEntry{
        1024,
        fftComplex1024Pipeline.get(),
        ifftComplex1024Pipeline.get(),
        fftReal1024Pipeline.get(),
        ifftReal1024Pipeline.get(),
        fftDualReal1024Pipeline.get(),
        ifftDualReal1024Pipeline.get(),
        transposeComplex1024Pipeline.get(),
        transposeComplex512x1024Pipeline.get(),
        transposeComplex1024x512Pipeline.get(),
        fftRealTwiddle1024Buffer
    };

    fftDispatchTable[static_cast<uint32_t>(FFTSize::FFT2048)] = FFTDispatchEntry{
        2048,
        fftComplex2048Pipeline.get(),
        ifftComplex2048Pipeline.get(),
        fftReal2048Pipeline.get(),
        ifftReal2048Pipeline.get(),
        fftDualReal2048Pipeline.get(),
        ifftDualReal2048Pipeline.get(),
        transposeComplex2048Pipeline.get(),
        transposeComplex1024x2048Pipeline.get(),
        transposeComplex2048x1024Pipeline.get(),
        fftRealTwiddle2048Buffer
    };
}

void GpuFFT::Shutdown() {
    for (uint32_t i = 0; i < static_cast<uint32_t>(FFTSize::Count); i++) {
        fftDispatchTable[i] = FFTDispatchEntry{};
    }

    fftComplex256Pipeline.reset();
    fftComplex512Pipeline.reset();
    fftComplex1024Pipeline.reset();
    fftComplex2048Pipeline.reset();

    ifftComplex256Pipeline.reset();
    ifftComplex512Pipeline.reset();
    ifftComplex1024Pipeline.reset();
    ifftComplex2048Pipeline.reset();

    fftReal512Pipeline.reset();
    fftReal1024Pipeline.reset();
    fftReal2048Pipeline.reset();

    ifftReal512Pipeline.reset();
    ifftReal1024Pipeline.reset();
    ifftReal2048Pipeline.reset();

    fftDualReal256Pipeline.reset();
    fftDualReal512Pipeline.reset();
    fftDualReal1024Pipeline.reset();
    fftDualReal2048Pipeline.reset();

    ifftDualReal256Pipeline.reset();
    ifftDualReal512Pipeline.reset();
    ifftDualReal1024Pipeline.reset();
    ifftDualReal2048Pipeline.reset();

    multiplyComplexSpectrum2DPipeline.reset();
    multiplyPackedRealSpectrum2DPipeline.reset();
    multiplyPackedDualRealSpectrum2DPipeline.reset();

    transposeComplex256Pipeline.reset();
    transposeComplex512Pipeline.reset();
    transposeComplex1024Pipeline.reset();
    transposeComplex2048Pipeline.reset();
    transposeComplex256x512Pipeline.reset();
    transposeComplex512x256Pipeline.reset();
    transposeComplex512x1024Pipeline.reset();
    transposeComplex1024x512Pipeline.reset();
    transposeComplex1024x2048Pipeline.reset();
    transposeComplex2048x1024Pipeline.reset();

    fftComplex256ShaderTemplate.reset();
    fftComplex512ShaderTemplate.reset();
    fftComplex1024ShaderTemplate.reset();
    fftComplex2048ShaderTemplate.reset();

    ifftComplex256ShaderTemplate.reset();
    ifftComplex512ShaderTemplate.reset();
    ifftComplex1024ShaderTemplate.reset();
    ifftComplex2048ShaderTemplate.reset();

    fftReal512ShaderTemplate.reset();
    fftReal1024ShaderTemplate.reset();
    fftReal2048ShaderTemplate.reset();

    ifftReal512ShaderTemplate.reset();
    ifftReal1024ShaderTemplate.reset();
    ifftReal2048ShaderTemplate.reset();

    fftDualReal256ShaderTemplate.reset();
    fftDualReal512ShaderTemplate.reset();
    fftDualReal1024ShaderTemplate.reset();
    fftDualReal2048ShaderTemplate.reset();

    ifftDualReal256ShaderTemplate.reset();
    ifftDualReal512ShaderTemplate.reset();
    ifftDualReal1024ShaderTemplate.reset();
    ifftDualReal2048ShaderTemplate.reset();

    multiplyComplexSpectrum2DShaderTemplate.reset();
    multiplyPackedRealSpectrum2DShaderTemplate.reset();
    multiplyPackedDualRealSpectrum2DShaderTemplate.reset();

    transposeComplex256ShaderTemplate.reset();
    transposeComplex512ShaderTemplate.reset();
    transposeComplex1024ShaderTemplate.reset();
    transposeComplex2048ShaderTemplate.reset();
    transposeComplex256x512ShaderTemplate.reset();
    transposeComplex512x256ShaderTemplate.reset();
    transposeComplex512x1024ShaderTemplate.reset();
    transposeComplex1024x512ShaderTemplate.reset();
    transposeComplex1024x2048ShaderTemplate.reset();
    transposeComplex2048x1024ShaderTemplate.reset();

    if (fftRealTwiddle512Buffer) {
        RHI::renderer->DestroyBuffer(fftRealTwiddle512Buffer);
        fftRealTwiddle512Buffer = nullptr;
    }

    if (fftRealTwiddle1024Buffer) {
        RHI::renderer->DestroyBuffer(fftRealTwiddle1024Buffer);
        fftRealTwiddle1024Buffer = nullptr;
    }

    if (fftRealTwiddle2048Buffer) {
        RHI::renderer->DestroyBuffer(fftRealTwiddle2048Buffer);
        fftRealTwiddle2048Buffer = nullptr;
    }
}

void GpuFFT::TrackBootstrapPipelines(ShaderPipelineBootstrap &bootstrap) const {
    static constexpr char groupName[] = "GpuFFT";

    bootstrap.TrackPipeline(groupName, fftComplex256Pipeline);
    bootstrap.TrackPipeline(groupName, fftComplex512Pipeline);
    bootstrap.TrackPipeline(groupName, fftComplex1024Pipeline);
    bootstrap.TrackPipeline(groupName, fftComplex2048Pipeline);

    bootstrap.TrackPipeline(groupName, ifftComplex256Pipeline);
    bootstrap.TrackPipeline(groupName, ifftComplex512Pipeline);
    bootstrap.TrackPipeline(groupName, ifftComplex1024Pipeline);
    bootstrap.TrackPipeline(groupName, ifftComplex2048Pipeline);

    bootstrap.TrackPipeline(groupName, fftReal512Pipeline);
    bootstrap.TrackPipeline(groupName, fftReal1024Pipeline);
    bootstrap.TrackPipeline(groupName, fftReal2048Pipeline);

    bootstrap.TrackPipeline(groupName, ifftReal512Pipeline);
    bootstrap.TrackPipeline(groupName, ifftReal1024Pipeline);
    bootstrap.TrackPipeline(groupName, ifftReal2048Pipeline);

    bootstrap.TrackPipeline(groupName, fftDualReal256Pipeline);
    bootstrap.TrackPipeline(groupName, fftDualReal512Pipeline);
    bootstrap.TrackPipeline(groupName, fftDualReal1024Pipeline);
    bootstrap.TrackPipeline(groupName, fftDualReal2048Pipeline);

    bootstrap.TrackPipeline(groupName, ifftDualReal256Pipeline);
    bootstrap.TrackPipeline(groupName, ifftDualReal512Pipeline);
    bootstrap.TrackPipeline(groupName, ifftDualReal1024Pipeline);
    bootstrap.TrackPipeline(groupName, ifftDualReal2048Pipeline);

    bootstrap.TrackPipeline(groupName, multiplyComplexSpectrum2DPipeline);
    bootstrap.TrackPipeline(groupName, multiplyPackedRealSpectrum2DPipeline);
    bootstrap.TrackPipeline(groupName, multiplyPackedDualRealSpectrum2DPipeline);

    bootstrap.TrackPipeline(groupName, transposeComplex256Pipeline);
    bootstrap.TrackPipeline(groupName, transposeComplex512Pipeline);
    bootstrap.TrackPipeline(groupName, transposeComplex1024Pipeline);
    bootstrap.TrackPipeline(groupName, transposeComplex2048Pipeline);
    bootstrap.TrackPipeline(groupName, transposeComplex256x512Pipeline);
    bootstrap.TrackPipeline(groupName, transposeComplex512x256Pipeline);
    bootstrap.TrackPipeline(groupName, transposeComplex512x1024Pipeline);
    bootstrap.TrackPipeline(groupName, transposeComplex1024x512Pipeline);
    bootstrap.TrackPipeline(groupName, transposeComplex1024x2048Pipeline);
    bootstrap.TrackPipeline(groupName, transposeComplex2048x1024Pipeline);
}

bool GpuFFT::GetGpuFFTSize(uint32_t fftSizeValue, GpuFFT::FFTSize &fftSizeEnum) {
    switch (fftSizeValue) {
    case 256:
        fftSizeEnum = GpuFFT::FFTSize::FFT256;
        return true;
    case 512:
        fftSizeEnum = GpuFFT::FFTSize::FFT512;
        return true;
    case 1024:
        fftSizeEnum = GpuFFT::FFTSize::FFT1024;
        return true;
    case 2048:
        fftSizeEnum = GpuFFT::FFTSize::FFT2048;
        return true;
    default:
        assert(false);
        return false;
    }
    return false;
}

uint32_t GpuFFT::GetGpuFFTSizeValue(GpuFFT::FFTSize fftSizeEnum) {
    switch (fftSizeEnum) {
    case GpuFFT::FFTSize::FFT256:
        return 256u;
    case GpuFFT::FFTSize::FFT512:
        return 512u;
    case GpuFFT::FFTSize::FFT1024:
        return 1024u;
    case GpuFFT::FFTSize::FFT2048:
        return 2048u;
    default:
        assert(false);
        return 0u;
    }
    return 0u;
}

bool GpuFFT::DispatchFFTComplex1D(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, FFTSize fftSize, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTComplex1D");

    const uint32_t fftSizeIndex = static_cast<uint32_t>(fftSize);
    if (fftSizeIndex >= static_cast<uint32_t>(FFTSize::Count)) {
        assert(false);
        return false;
    }

    const FFTDispatchEntry &entry = fftDispatchTable[fftSizeIndex];
    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(entry.fftComplexPipeline);
    if (!pso) {
        return false;
    }

    if (!DispatchFFTComplexFused(commandList, pso, inputBuffer, outputBuffer, fftBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchIFFTComplex1D(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, FFTSize fftSize, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTComplex1D");

    const uint32_t fftSizeIndex = static_cast<uint32_t>(fftSize);
    if (fftSizeIndex >= static_cast<uint32_t>(FFTSize::Count)) {
        assert(false);
        return false;
    }

    const FFTDispatchEntry &entry = fftDispatchTable[fftSizeIndex];
    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(entry.ifftComplexPipeline);
    if (!pso) {
        return false;
    }

    if (!DispatchFFTComplexFused(commandList, pso, inputBuffer, outputBuffer, fftBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchComplexFFT2D(RHI::CommandList *commandList, RHI::PipelineState *fftPso, RHI::PipelineState *transposePso, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftSize, uint32_t planeCount, Complex2DDataLayout inputLayout, Complex2DDataLayout outputLayout) {
    const uint32_t FFTBlockCount = fftSize * planeCount;
    const uint32_t TransposeTileCount = fftSize / TransposeComplexTileSize;
    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // Normal -> Normal or Transposed -> Transposed
    // 입출력 레이아웃이 같으면 transpose 를 두번 수행하여 원래 레이아웃으로 되돌린다.
    if (inputLayout == outputLayout) {
        RHI::Buffer *firstStageOutput = workBuffer;
        if (!DispatchFFTComplexFused(commandList, fftPso, inputBuffer, firstStageOutput, FFTBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined)) {
            return false;
        }

        RHI::Buffer *firstTransposeOutput = outputBuffer;
        RHI::GPUBarrier firstTransposeBarriers[] = {
            RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
            RHI::Renderer::MakeBufferBarrier(firstStageOutput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
            RHI::Renderer::MakeBufferBarrier(firstTransposeOutput, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
        };
        RHI::renderer->Barrier(commandList, firstTransposeBarriers, COUNT_OF(firstTransposeBarriers));
        dispatchBufferPSO(transposePso, firstStageOutput, firstTransposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

        RHI::Buffer *secondStageOutput = workBuffer;
        if (!DispatchFFTComplexFused(commandList, fftPso, firstTransposeOutput, secondStageOutput, FFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute)) {
            return false;
        }

        RHI::Buffer *secondTransposeOutput = outputBuffer;
        RHI::GPUBarrier secondTransposeBarriers[] = {
            RHI::Renderer::MakeBufferBarrier(secondStageOutput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
            RHI::Renderer::MakeBufferBarrier(secondTransposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess)
        };
        RHI::renderer->Barrier(commandList, secondTransposeBarriers, COUNT_OF(secondTransposeBarriers));
        dispatchBufferPSO(transposePso, secondStageOutput, secondTransposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

        RHI::GPUBarrier endBarriers[] = {
            RHI::Renderer::MakeBufferBarrier(secondStageOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
            RHI::Renderer::MakeBufferBarrier(secondTransposeOutput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
        };
        RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));
        return true;
    }

    // Normal -> Transposed
    if (inputLayout == Complex2DDataLayout::Normal) {
        // 가로 FFT/IFFT 스테이지
        RHI::Buffer *firstStageOutput = outputBuffer;
        if (!DispatchFFTComplexFused(commandList, fftPso, inputBuffer, firstStageOutput, FFTBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined)) {
            return false;
        }

        // transpose
        RHI::Buffer *transposeOutput = workBuffer;
        RHI::GPUBarrier transposeBarriers[] = {
            RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
            RHI::Renderer::MakeBufferBarrier(firstStageOutput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
            RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
        };
        RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
        dispatchBufferPSO(transposePso, firstStageOutput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

        // 세로 FFT/IFFT 스테이지. 결과는 transposed 레이아웃이다.
        if (!DispatchFFTComplexFused(commandList, fftPso, transposeOutput, outputBuffer, FFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute)) {
            return false;
        }

        RHI::GPUBarrier endBarriers[] = {
            RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
            RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
        };
        RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));
        return true;
    }

    // Transposed -> Normal
    // 세로 FFT/IFFT 스테이지
    RHI::Buffer *firstStageOutput = workBuffer;
    if (!DispatchFFTComplexFused(commandList, fftPso, inputBuffer, firstStageOutput, FFTBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined)) {
        return false;
    }

    RHI::Buffer *transposeOutput = inputBuffer;
    RHI::GPUBarrier transposeBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(firstStageOutput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
    dispatchBufferPSO(transposePso, firstStageOutput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 가로 FFT/IFFT 스테이지. 결과는 일반 레이아웃이다.
    if (!DispatchFFTComplexFused(commandList, fftPso, transposeOutput, outputBuffer, FFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)) {
        return false;
    }

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(firstStageOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));
    return true;
}

bool GpuFFT::DispatchFFTComplex2D(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, FFTSize fftSize, uint32_t planeCount, Complex2DDataLayout inputLayout, Complex2DDataLayout outputLayout) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTComplex2D");

    const uint32_t fftSizeIndex = static_cast<uint32_t>(fftSize);
    if (fftSizeIndex >= static_cast<uint32_t>(FFTSize::Count)) {
        assert(false);
        return false;
    }

    const FFTDispatchEntry &entry = fftDispatchTable[fftSizeIndex];
    RHI::PipelineState *fftPso = ShaderPipeline::GetReadyPSO(entry.fftComplexPipeline);
    RHI::PipelineState *transposePso = ShaderPipeline::GetReadyPSO(entry.transposeComplexPipeline);
    if (!fftPso || !transposePso) {
        return false;
    }

    return DispatchComplexFFT2D(commandList, fftPso, transposePso, inputBuffer, outputBuffer, workBuffer, entry.fftSize, planeCount, inputLayout, outputLayout);
}

bool GpuFFT::DispatchIFFTComplex2D(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, FFTSize fftSize, uint32_t planeCount, Complex2DDataLayout inputLayout, Complex2DDataLayout outputLayout) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTComplex2D");

    const uint32_t fftSizeIndex = static_cast<uint32_t>(fftSize);
    if (fftSizeIndex >= static_cast<uint32_t>(FFTSize::Count)) {
        assert(false);
        return false;
    }

    const FFTDispatchEntry &entry = fftDispatchTable[fftSizeIndex];
    RHI::PipelineState *ifftPso = ShaderPipeline::GetReadyPSO(entry.ifftComplexPipeline);
    RHI::PipelineState *transposePso = ShaderPipeline::GetReadyPSO(entry.transposeComplexPipeline);
    if (!ifftPso || !transposePso) {
        return false;
    }

    return DispatchComplexFFT2D(commandList, ifftPso, transposePso, inputBuffer, outputBuffer, workBuffer, entry.fftSize, planeCount, inputLayout, outputLayout);
}

bool GpuFFT::DispatchFFTReal1D(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, FFTSize fftSize, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTReal1D");

    const uint32_t fftSizeIndex = static_cast<uint32_t>(fftSize);
    if (fftSizeIndex >= static_cast<uint32_t>(FFTSize::Count)) {
        assert(false);
        return false;
    }

    const FFTDispatchEntry &entry = fftDispatchTable[fftSizeIndex];
    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(entry.fftRealPipeline);
    RHI::Buffer *twiddleBuffer = entry.fftRealTwiddleBuffer;
    if (!pso || !twiddleBuffer) {
        return false;
    }

    RHI::Buffer *resultBuffer = nullptr;
    if (!DispatchFFTRealFused(commandList, pso, twiddleBuffer, inputBuffer, outputBuffer, fftBlockCount, &resultBuffer)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchIFFTReal1D(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, FFTSize fftSize, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTReal1D");

    const uint32_t fftSizeIndex = static_cast<uint32_t>(fftSize);
    if (fftSizeIndex >= static_cast<uint32_t>(FFTSize::Count)) {
        assert(false);
        return false;
    }

    const FFTDispatchEntry &entry = fftDispatchTable[fftSizeIndex];
    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(entry.ifftRealPipeline);
    RHI::Buffer *twiddleBuffer = entry.fftRealTwiddleBuffer;
    if (!pso || !twiddleBuffer) {
        return false;
    }

    return DispatchIFFTRealFused(commandList, pso, twiddleBuffer, inputBuffer, outputBuffer, nullptr, fftBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined);
}

bool GpuFFT::DispatchFFTReal2D(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, FFTSize fftSize, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTReal2D");

    const uint32_t fftSizeIndex = static_cast<uint32_t>(fftSize);
    if (fftSizeIndex >= static_cast<uint32_t>(FFTSize::Count)) {
        assert(false);
        return false;
    }

    const FFTDispatchEntry &entry = fftDispatchTable[fftSizeIndex];
    RHI::PipelineState *realPso = ShaderPipeline::GetReadyPSO(entry.fftRealPipeline);
    RHI::PipelineState *complexPso = ShaderPipeline::GetReadyPSO(entry.fftComplexPipeline);
    RHI::PipelineState *transposePso = ShaderPipeline::GetReadyPSO(entry.transposeComplexPackedToFullPipeline);
    RHI::Buffer *twiddleBuffer = entry.fftRealTwiddleBuffer;
    if (!realPso || !complexPso || !transposePso || !twiddleBuffer) {
        return false;
    }

    const uint32_t packedSize = entry.fftSize / 2;
    const uint32_t RealFFTBlockCount = entry.fftSize * planeCount;
    const uint32_t VerticalComplexFFTBlockCount = packedSize * planeCount;
    const uint32_t TransposeTileCountX = packedSize / TransposeComplexTileSize;
    const uint32_t TransposeTileCountY = entry.fftSize / TransposeComplexTileSize;

    auto dispatchBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 가로 FFT 스테이지
    RHI::Buffer *stageInput = nullptr;
    if (!DispatchFFTRealFused(commandList, realPso, twiddleBuffer, inputBuffer, outputBuffer, RealFFTBlockCount, &stageInput)) {
        return false;
    }

    // packedSize x fftSize packed spectrum 을 fftSize x packedSize 으로 transpose 해서 세로 Complex FFT 를 수행한다.
    RHI::Buffer *transposeOutput = workBuffer;
    RHI::GPUBarrier transposeBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
    dispatchBufferPSO(transposePso, stageInput, transposeOutput, TransposeTileCountX, TransposeTileCountY, planeCount);

    // 세로 FFT 스테이지
    if (!DispatchFFTComplexFused(commandList, complexPso, transposeOutput, outputBuffer, VerticalComplexFFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute)) {
        return false;
    }

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool GpuFFT::DispatchIFFTReal2D(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, FFTSize fftSize, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTReal2D");

    const uint32_t fftSizeIndex = static_cast<uint32_t>(fftSize);
    if (fftSizeIndex >= static_cast<uint32_t>(FFTSize::Count)) {
        assert(false);
        return false;
    }

    const FFTDispatchEntry &entry = fftDispatchTable[fftSizeIndex];
    RHI::PipelineState *realPso = ShaderPipeline::GetReadyPSO(entry.ifftRealPipeline);
    RHI::PipelineState *complexPso = ShaderPipeline::GetReadyPSO(entry.ifftComplexPipeline);
    RHI::PipelineState *transposePso = ShaderPipeline::GetReadyPSO(entry.transposeComplexFullToPackedPipeline);
    RHI::Buffer *twiddleBuffer = entry.fftRealTwiddleBuffer;
    if (!realPso || !complexPso || !transposePso || !twiddleBuffer) {
        return false;
    }

    const uint32_t packedSize = entry.fftSize / 2;
    const uint32_t RealFFTBlockCount = entry.fftSize * planeCount;
    const uint32_t VerticalComplexFFTBlockCount = packedSize * planeCount;
    const uint32_t TransposeTileCountX = entry.fftSize / TransposeComplexTileSize;
    const uint32_t TransposeTileCountY = packedSize / TransposeComplexTileSize;

    auto dispatchBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 세로 IFFT 스테이지
    RHI::Buffer *stageInput = workBuffer;
    if (!DispatchFFTComplexFused(commandList, complexPso, inputBuffer, stageInput, VerticalComplexFFTBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined)) {
        return false;
    }

    // fftSize x packedSize spectrum 을 packedSize x fftSize 로 transpose 해서 가로 Real IFFT 를 수행한다.
    RHI::Buffer *transposeOutput = inputBuffer;
    RHI::GPUBarrier transposeBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
    dispatchBufferPSO(transposePso, stageInput, transposeOutput, TransposeTileCountX, TransposeTileCountY, planeCount);

    // 가로 IFFT 스테이지
    return DispatchIFFTRealFused(commandList, realPso, twiddleBuffer, transposeOutput, outputBuffer, stageInput, RealFFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute);
}

bool GpuFFT::DispatchFFTDualReal1D(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, FFTSize fftSize, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTDualReal1D");

    const uint32_t fftSizeIndex = static_cast<uint32_t>(fftSize);
    if (fftSizeIndex >= static_cast<uint32_t>(FFTSize::Count)) {
        assert(false);
        return false;
    }

    const FFTDispatchEntry &entry = fftDispatchTable[fftSizeIndex];
    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(entry.fftDualRealPipeline);
    if (!pso) {
        return false;
    }

    RHI::Buffer *resultBuffer = nullptr;
    if (!DispatchFFTDualRealFused(commandList, pso, inputBuffer, outputBuffer, fftBlockCount, &resultBuffer)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchIFFTDualReal1D(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, FFTSize fftSize, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTDualReal1D");

    const uint32_t fftSizeIndex = static_cast<uint32_t>(fftSize);
    if (fftSizeIndex >= static_cast<uint32_t>(FFTSize::Count)) {
        assert(false);
        return false;
    }

    const FFTDispatchEntry &entry = fftDispatchTable[fftSizeIndex];
    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(entry.ifftDualRealPipeline);
    if (!pso) {
        return false;
    }

    return DispatchIFFTDualRealFused(commandList, pso, inputBuffer, outputBuffer, nullptr, fftBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined);
}

bool GpuFFT::DispatchFFTDualReal2D(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, FFTSize fftSize, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTDualReal2D");

    const uint32_t fftSizeIndex = static_cast<uint32_t>(fftSize);
    if (fftSizeIndex >= static_cast<uint32_t>(FFTSize::Count)) {
        assert(false);
        return false;
    }

    const FFTDispatchEntry &entry = fftDispatchTable[fftSizeIndex];
    RHI::PipelineState *dualRealPso = ShaderPipeline::GetReadyPSO(entry.fftDualRealPipeline);
    RHI::PipelineState *complexPso = ShaderPipeline::GetReadyPSO(entry.fftComplexPipeline);
    RHI::PipelineState *transposePso = ShaderPipeline::GetReadyPSO(entry.transposeComplexPipeline);
    if (!dualRealPso || !complexPso || !transposePso) {
        return false;
    }

    const uint32_t FFTBlockCount = entry.fftSize * planeCount;
    const uint32_t TransposeTileCount = entry.fftSize / TransposeComplexTileSize;

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 가로 FFT 스테이지
    RHI::Buffer *horizontalOutput = outputBuffer;
    RHI::Buffer *stageInput = nullptr;
    if (!DispatchFFTDualRealFused(commandList, dualRealPso, inputBuffer, horizontalOutput, FFTBlockCount, &stageInput)) {
        return false;
    }

    // fftSize x fftSize transpose 해서 다음 FFT 스테이지는 세로로 하게 된다.
    RHI::Buffer *transposeOutput = workBuffer;
    RHI::GPUBarrier transposeBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
    dispatchBufferPSO(transposePso, stageInput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 세로 FFT 스테이지
    if (!DispatchFFTComplexFused(commandList, complexPso, transposeOutput, outputBuffer, FFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute)) {
        return false;
    }

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool GpuFFT::DispatchIFFTDualReal2D(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, FFTSize fftSize, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTDualReal2D");

    const uint32_t fftSizeIndex = static_cast<uint32_t>(fftSize);
    if (fftSizeIndex >= static_cast<uint32_t>(FFTSize::Count)) {
        assert(false);
        return false;
    }

    const FFTDispatchEntry &entry = fftDispatchTable[fftSizeIndex];
    RHI::PipelineState *complexPso = ShaderPipeline::GetReadyPSO(entry.ifftComplexPipeline);
    RHI::PipelineState *dualRealPso = ShaderPipeline::GetReadyPSO(entry.ifftDualRealPipeline);
    RHI::PipelineState *transposePso = ShaderPipeline::GetReadyPSO(entry.transposeComplexPipeline);
    if (!complexPso || !dualRealPso || !transposePso) {
        return false;
    }

    const uint32_t FFTBlockCount = entry.fftSize * planeCount;
    const uint32_t TransposeTileCount = entry.fftSize / TransposeComplexTileSize;

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 세로 IFFT 스테이지 (이미 DispatchFFTDualReal2D 에서 마지막에 세로로 끝났으므로, 세로 데이터라고 가정하고 시작)
    RHI::Buffer *verticalOutput = workBuffer;
    if (!DispatchFFTComplexFused(commandList, complexPso, inputBuffer, verticalOutput, FFTBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined)) {
        return false;
    }

    // fftSize x fftSize transpose 해서 다음 IFFT 스테이지는 가로로 하게 된다.
    RHI::Buffer *transposeOutput = inputBuffer;
    RHI::GPUBarrier transposeBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(verticalOutput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
    dispatchBufferPSO(transposePso, verticalOutput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 가로 IFFT 스테이지
    return DispatchIFFTDualRealFused(commandList, dualRealPso, transposeOutput, outputBuffer, verticalOutput, FFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute);
}

bool GpuFFT::DispatchMultiplyComplexSpectrum2D(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *lhsSpectrumBuffer, RHI::Buffer *rhsSpectrumBuffer, RHI::Buffer *outputSpectrumBuffer, uint32_t fftSize, uint32_t planeCount, float scale) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchMultiplyComplexSpectrum2D");

    assert(outputSpectrumBuffer != lhsSpectrumBuffer);
    assert(outputSpectrumBuffer != rhsSpectrumBuffer);

    if (fftSize == 0) {
        assert(false);
        return false;
    }

    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(multiplyComplexSpectrum2DPipeline);
    if (!pso) {
        return false;
    }

    const uint64_t elementCount64 = static_cast<uint64_t>(fftSize) * static_cast<uint64_t>(fftSize) * static_cast<uint64_t>(planeCount);
    if (elementCount64 > 0xFFFFFFFFull) {
        return false;
    }

    const uint32_t elementCount = static_cast<uint32_t>(elementCount64);

    MultiplySpectrum2DParams multiplyParams = {};
    multiplyParams.fftSize = fftSize;
    multiplyParams.planeCount = planeCount;
    multiplyParams.scale = scale;

    RHI::ConstantBuffer *multiplyConstantBuffer = frameThreadData->AllocConstant(sizeof(MultiplySpectrum2DParams), &multiplyParams);
    if (!multiplyConstantBuffer) {
        return false;
    }

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(lhsSpectrumBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(rhsSpectrumBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputSpectrumBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetBuffer(commandList, 0, false, lhsSpectrumBuffer);
    RHI::renderer->SetBuffer(commandList, 1, false, rhsSpectrumBuffer);
    RHI::renderer->SetBuffer(commandList, 0, true, outputSpectrumBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 0, multiplyConstantBuffer);

    const uint32_t threadGroupCount = BE1::DivCeil(elementCount, MultiplySpectrum2DBlockSize);
    RHI::renderer->Dispatch(commandList, threadGroupCount, 1, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(lhsSpectrumBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(rhsSpectrumBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputSpectrumBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool GpuFFT::DispatchMultiplyPackedRealSpectrum2D(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *lhsSpectrumBuffer, RHI::Buffer *rhsSpectrumBuffer, RHI::Buffer *outputSpectrumBuffer, uint32_t fftSize, uint32_t planeCount, float scale) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchMultiplyPackedRealSpectrum2D");

    assert(outputSpectrumBuffer != lhsSpectrumBuffer);
    assert(outputSpectrumBuffer != rhsSpectrumBuffer);

    if (fftSize < 2 || (fftSize & 1) != 0) {
        assert(false);
        return false;
    }

    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(multiplyPackedRealSpectrum2DPipeline);
    if (!pso) {
        return false;
    }

    const uint32_t halfSize = fftSize >> 1;
    const uint64_t elementCount64 = static_cast<uint64_t>(fftSize) * static_cast<uint64_t>(halfSize) * static_cast<uint64_t>(planeCount);
    if (elementCount64 > 0xFFFFFFFFull) {
        return false;
    }

    const uint32_t elementCount = static_cast<uint32_t>(elementCount64);

    MultiplySpectrum2DParams multiplyParams = {};
    multiplyParams.fftSize = fftSize;
    multiplyParams.planeCount = planeCount;
    multiplyParams.scale = scale;

    RHI::ConstantBuffer *multiplyConstantBuffer = frameThreadData->AllocConstant(sizeof(MultiplySpectrum2DParams), &multiplyParams);
    if (!multiplyConstantBuffer) {
        return false;
    }

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(lhsSpectrumBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(rhsSpectrumBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputSpectrumBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetBuffer(commandList, 0, false, lhsSpectrumBuffer);
    RHI::renderer->SetBuffer(commandList, 1, false, rhsSpectrumBuffer);
    RHI::renderer->SetBuffer(commandList, 0, true, outputSpectrumBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 0, multiplyConstantBuffer);

    const uint32_t threadGroupCount = BE1::DivCeil(elementCount, MultiplySpectrum2DBlockSize);
    RHI::renderer->Dispatch(commandList, threadGroupCount, 1, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(lhsSpectrumBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(rhsSpectrumBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputSpectrumBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool GpuFFT::DispatchMultiplyPackedDualRealSpectrum2D(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *lhsSpectrumBuffer, RHI::Buffer *rhsSpectrumBuffer, RHI::Buffer *outputSpectrumBuffer, uint32_t fftSize, uint32_t planeCount, float scale) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchMultiplyPackedDualRealSpectrum2D");

    assert(outputSpectrumBuffer != lhsSpectrumBuffer);
    assert(outputSpectrumBuffer != rhsSpectrumBuffer);

    if (fftSize < 2 || (fftSize & 1) != 0) {
        assert(false);
        return false;
    }

    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(multiplyPackedDualRealSpectrum2DPipeline);
    if (!pso) {
        return false;
    }

    const uint64_t elementCount64 = static_cast<uint64_t>(fftSize) * static_cast<uint64_t>(fftSize) * static_cast<uint64_t>(planeCount);
    if (elementCount64 > 0xFFFFFFFFull) {
        return false;
    }

    const uint32_t elementCount = static_cast<uint32_t>(elementCount64);

    MultiplySpectrum2DParams multiplyParams = {};
    multiplyParams.fftSize = fftSize;
    multiplyParams.planeCount = planeCount;
    multiplyParams.scale = scale;

    RHI::ConstantBuffer *multiplyConstantBuffer = frameThreadData->AllocConstant(sizeof(MultiplySpectrum2DParams), &multiplyParams);
    if (!multiplyConstantBuffer) {
        return false;
    }

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(lhsSpectrumBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(rhsSpectrumBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputSpectrumBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetBuffer(commandList, 0, false, lhsSpectrumBuffer);
    RHI::renderer->SetBuffer(commandList, 1, false, rhsSpectrumBuffer);
    RHI::renderer->SetBuffer(commandList, 0, true, outputSpectrumBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 0, multiplyConstantBuffer);

    const uint32_t threadGroupCount = BE1::DivCeil(elementCount, MultiplySpectrum2DBlockSize);
    RHI::renderer->Dispatch(commandList, threadGroupCount, 1, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(lhsSpectrumBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(rhsSpectrumBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputSpectrumBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

RHI::Buffer *GpuFFT::CreateRealFFTTwiddleBuffer(uint32_t fftSize) {
    const uint32_t halfSize = fftSize / 2;

    BE1::Array<float2> twiddles;
    twiddles.SetCount(halfSize);

    for (uint32_t i = 0; i < halfSize; i++) {
        const float angle = BE1::Math::TwoPi * static_cast<float>(i) / static_cast<float>(fftSize);
        float s, c;
        BE1::Math::SinCos(angle, s, c);
        twiddles[i] = float2(c, s);
    }

    const uint64_t bufferSize = uint64_t(halfSize) * sizeof(float2);
    RHI::Buffer *buffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::ShaderResource, bufferSize, twiddles.Ptr());
    if (!buffer) {
        BE_WARNLOG("GpuFFT::CreateRealFFTTwiddleBuffer: failed to create twiddle buffer for FFT%u\n", fftSize);
        return nullptr;
    }

    RHI::renderer->SetName(buffer, BE1::va("GpuFFT::realFFTTwiddle%iBuffer", fftSize));

    RHI::ViewID srvViewID = RHI::renderer->CreateView(buffer, RHI::ViewType::SRV, RHI::Format::Unknown, sizeof(float2), 0, bufferSize);
    if (!srvViewID.IsValid()) {
        BE_WARNLOG("GpuFFT::CreateRealFFTTwiddleBuffer: failed to create twiddle SRV for FFT%u\n", fftSize);
        RHI::renderer->DestroyBuffer(buffer);
        return nullptr;
    }

    return buffer;
}

bool GpuFFT::DispatchFFTComplexFused(RHI::CommandList *commandList, RHI::PipelineState *pso, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount, RHI::GPUResourceState inputBufferState, RHI::GPUResourceState outputBufferState) {
    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, inputBufferState, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, outputBufferState, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetBuffer(commandList, 0, false, inputBuffer);
    RHI::renderer->SetBuffer(commandList, 0, true, outputBuffer);
    RHI::renderer->Dispatch(commandList, fftBlockCount, 1, 1);
    return true;
}

bool GpuFFT::DispatchFFTRealFused(RHI::CommandList *commandList, RHI::PipelineState *pso, RHI::Buffer *twiddleBuffer, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount, RHI::Buffer **resultBuffer) {
    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetBuffer(commandList, 0, false, inputBuffer);
    RHI::renderer->SetBuffer(commandList, 1, false, twiddleBuffer);
    RHI::renderer->SetBuffer(commandList, 0, true, outputBuffer);
    RHI::renderer->Dispatch(commandList, fftBlockCount, 1, 1);

    *resultBuffer = outputBuffer;
    return true;
}

bool GpuFFT::DispatchIFFTRealFused(RHI::CommandList *commandList, RHI::PipelineState *pso, RHI::Buffer *twiddleBuffer, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount, RHI::GPUResourceState inputBufferState, RHI::GPUResourceState workBufferState) {
    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, inputBufferState, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetBuffer(commandList, 0, false, inputBuffer);
    RHI::renderer->SetBuffer(commandList, 1, false, twiddleBuffer);
    RHI::renderer->SetBuffer(commandList, 0, true, outputBuffer);
    RHI::renderer->Dispatch(commandList, fftBlockCount, 1, 1);

    if (workBufferState != RHI::GPUResourceState::Undefined) {
        RHI::GPUBarrier endBarriers[] = {
            RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
            RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined),
            RHI::Renderer::MakeBufferBarrier(workBuffer, workBufferState, RHI::GPUResourceState::Undefined)
        };
        RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));
    } else {
        RHI::GPUBarrier endBarriers[] = {
            RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
            RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
        };
        RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));
    }

    return true;
}

bool GpuFFT::DispatchFFTDualRealFused(RHI::CommandList *commandList, RHI::PipelineState *pso, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount, RHI::Buffer **resultBuffer) {
    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetBuffer(commandList, 0, false, inputBuffer);
    RHI::renderer->SetBuffer(commandList, 0, true, outputBuffer);
    RHI::renderer->Dispatch(commandList, fftBlockCount, 1, 1);

    *resultBuffer = outputBuffer;
    return true;
}

bool GpuFFT::DispatchIFFTDualRealFused(RHI::CommandList *commandList, RHI::PipelineState *pso, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount, RHI::GPUResourceState inputBufferState, RHI::GPUResourceState workBufferState) {
    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, inputBufferState, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetBuffer(commandList, 0, false, inputBuffer);
    RHI::renderer->SetBuffer(commandList, 0, true, outputBuffer);
    RHI::renderer->Dispatch(commandList, fftBlockCount, 1, 1);

    if (workBufferState != RHI::GPUResourceState::Undefined) {
        RHI::GPUBarrier endBarriers[] = {
            RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
            RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined),
            RHI::Renderer::MakeBufferBarrier(workBuffer, workBufferState, RHI::GPUResourceState::Undefined)
        };
        RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));
    } else {
        RHI::GPUBarrier endBarriers[] = {
            RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
            RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
        };
        RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));
    }

    return true;
}
