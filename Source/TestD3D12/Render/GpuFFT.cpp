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
#include "RenderFrameData.h"
#include "ShaderTemplateManager.h"
#include "ShaderPipelineCache.h"
#include "Shaders/ShaderInterop.h"

struct FFTDispatchEntry {
    uint32_t                fftSize = 0;

    RHI::PipelineState *    fftComplexPSO = nullptr;
    RHI::PipelineState *    fftRealPSO = nullptr;
    RHI::PipelineState *    ifftRealPSO = nullptr;
    RHI::PipelineState *    fftDualRealPSO = nullptr;
    RHI::PipelineState *    ifftDualRealPSO = nullptr;

    RHI::PipelineState *    transposeComplexPSO = nullptr;
    RHI::PipelineState *    transposeComplexPackedToFullPSO = nullptr;
    RHI::PipelineState *    transposeComplexFullToPackedPSO = nullptr;

    RHI::Buffer *           fftRealTwiddleBuffer = nullptr;
};

static FFTDispatchEntry     fftDispatchTable[static_cast<uint32_t>(GpuFFT::FFTSize::Count)];

static SharedPtr<ShaderTemplate> CreateGpuFFTShaderTemplate(const char *hashName, RHI::ShaderModel shaderModel, const char *filename, const char *baseDir) {
    BE1::Str shaderTemplateText;
    shaderTemplateText.sPrintf(
        "{\n"
        "    stages {\n"
        "        compute %s \"%s\" \"%s\"\n"
        "    }\n"
        "}\n",
        RHI::Renderer::ShaderModelName(shaderModel), filename, "CSMain");

    SharedPtr<ShaderTemplate> shaderTemplate = shaderTemplateManager.AllocShaderTemplate(hashName);
    if (!shaderTemplate->Create(shaderTemplateText.c_str(), baseDir)) {
        BE_FATALERROR("GpuFFT::CreateShaderTemplate: failed to create shader template '%s'", hashName);
    }
    return shaderTemplate;
}

void GpuFFT::Init() {
    const char *baseDir = "Source/TestD3D12/Shaders";
    const RHI::ShaderModel computeShaderModel = RHI::ShaderModel::SM_6_6;

    fftComplex256ShaderTemplate = CreateGpuFFTShaderTemplate("_fftComplex256Shader", computeShaderModel, "FFT/FFTComplex256.hlsl", baseDir);
    fftComplex512ShaderTemplate = CreateGpuFFTShaderTemplate("_fftComplex512Shader", computeShaderModel, "FFT/FFTComplex512.hlsl", baseDir);
    fftComplex1024ShaderTemplate = CreateGpuFFTShaderTemplate("_fftComplex1024Shader", computeShaderModel, "FFT/FFTComplex1024.hlsl", baseDir);
    fftComplex2048ShaderTemplate = CreateGpuFFTShaderTemplate("_fftComplex2048Shader", computeShaderModel, "FFT/FFTComplex2048.hlsl", baseDir);

    fftReal512ShaderTemplate = CreateGpuFFTShaderTemplate("_fftReal512Shader", computeShaderModel, "FFT/FFTReal512.hlsl", baseDir);
    fftReal1024ShaderTemplate = CreateGpuFFTShaderTemplate("_fftReal1024Shader", computeShaderModel, "FFT/FFTReal1024.hlsl", baseDir);
    fftReal2048ShaderTemplate = CreateGpuFFTShaderTemplate("_fftReal2048Shader", computeShaderModel, "FFT/FFTReal2048.hlsl", baseDir);

    ifftReal512ShaderTemplate = CreateGpuFFTShaderTemplate("_ifftReal512Shader", computeShaderModel, "FFT/IFFTReal512.hlsl", baseDir);
    ifftReal1024ShaderTemplate = CreateGpuFFTShaderTemplate("_ifftReal1024Shader", computeShaderModel, "FFT/IFFTReal1024.hlsl", baseDir);
    ifftReal2048ShaderTemplate = CreateGpuFFTShaderTemplate("_ifftReal2048Shader", computeShaderModel, "FFT/IFFTReal2048.hlsl", baseDir);

    fftDualReal256ShaderTemplate = CreateGpuFFTShaderTemplate("_fftDualReal256Shader", computeShaderModel, "FFT/FFTDualReal256.hlsl", baseDir);
    fftDualReal512ShaderTemplate = CreateGpuFFTShaderTemplate("_fftDualReal512Shader", computeShaderModel, "FFT/FFTDualReal512.hlsl", baseDir);
    fftDualReal1024ShaderTemplate = CreateGpuFFTShaderTemplate("_fftDualReal1024Shader", computeShaderModel, "FFT/FFTDualReal1024.hlsl", baseDir);
    fftDualReal2048ShaderTemplate = CreateGpuFFTShaderTemplate("_fftDualReal2048Shader", computeShaderModel, "FFT/FFTDualReal2048.hlsl", baseDir);

    ifftDualReal256ShaderTemplate = CreateGpuFFTShaderTemplate("_ifftDualReal256Shader", computeShaderModel, "FFT/IFFTDualReal256.hlsl", baseDir);
    ifftDualReal512ShaderTemplate = CreateGpuFFTShaderTemplate("_ifftDualReal512Shader", computeShaderModel, "FFT/IFFTDualReal512.hlsl", baseDir);
    ifftDualReal1024ShaderTemplate = CreateGpuFFTShaderTemplate("_ifftDualReal1024Shader", computeShaderModel, "FFT/IFFTDualReal1024.hlsl", baseDir);
    ifftDualReal2048ShaderTemplate = CreateGpuFFTShaderTemplate("_ifftDualReal2048Shader", computeShaderModel, "FFT/IFFTDualReal2048.hlsl", baseDir);

    multiplyComplexSpectrum2DShaderTemplate = CreateGpuFFTShaderTemplate("_multiplyComplexSpectrum2DShader", computeShaderModel, "FFT/MultiplyComplexSpectrum2D.hlsl", baseDir);
    multiplyPackedRealSpectrum2DShaderTemplate = CreateGpuFFTShaderTemplate("_multiplyPackedRealSpectrum2DShader", computeShaderModel, "FFT/MultiplyPackedRealSpectrum2D.hlsl", baseDir);
    multiplyPackedDualRealSpectrum2DShaderTemplate = CreateGpuFFTShaderTemplate("_multiplyPackedDualRealSpectrum2DShader", computeShaderModel, "FFT/MultiplyPackedDualRealSpectrum2D.hlsl", baseDir);

    transposeComplex256ShaderTemplate = CreateGpuFFTShaderTemplate("_transposeComplex256Shader", computeShaderModel, "Transpose/TransposeComplex256.hlsl", baseDir);
    transposeComplex512ShaderTemplate = CreateGpuFFTShaderTemplate("_transposeComplex512Shader", computeShaderModel, "Transpose/TransposeComplex512.hlsl", baseDir);
    transposeComplex1024ShaderTemplate = CreateGpuFFTShaderTemplate("_transposeComplex1024Shader", computeShaderModel, "Transpose/TransposeComplex1024.hlsl", baseDir);
    transposeComplex2048ShaderTemplate = CreateGpuFFTShaderTemplate("_transposeComplex2048Shader", computeShaderModel, "Transpose/TransposeComplex2048.hlsl", baseDir);
    transposeComplex256x512ShaderTemplate = CreateGpuFFTShaderTemplate("_transposeComplex256x512Shader", computeShaderModel, "Transpose/TransposeComplex256x512.hlsl", baseDir);
    transposeComplex512x256ShaderTemplate = CreateGpuFFTShaderTemplate("_transposeComplex512x256Shader", computeShaderModel, "Transpose/TransposeComplex512x256.hlsl", baseDir);
    transposeComplex512x1024ShaderTemplate = CreateGpuFFTShaderTemplate("_transposeComplex512x1024Shader", computeShaderModel, "Transpose/TransposeComplex512x1024.hlsl", baseDir);
    transposeComplex1024x512ShaderTemplate = CreateGpuFFTShaderTemplate("_transposeComplex1024x512Shader", computeShaderModel, "Transpose/TransposeComplex1024x512.hlsl", baseDir);
    transposeComplex1024x2048ShaderTemplate = CreateGpuFFTShaderTemplate("_transposeComplex1024x2048Shader", computeShaderModel, "Transpose/TransposeComplex1024x2048.hlsl", baseDir);
    transposeComplex2048x1024ShaderTemplate = CreateGpuFFTShaderTemplate("_transposeComplex2048x1024Shader", computeShaderModel, "Transpose/TransposeComplex2048x1024.hlsl", baseDir);

    fftComplex256Pipeline = shaderPipelineCache.GetComputePipeline(fftComplex256ShaderTemplate, ShaderPermutationKey(), false);
    fftComplex512Pipeline = shaderPipelineCache.GetComputePipeline(fftComplex512ShaderTemplate, ShaderPermutationKey(), false);
    fftComplex1024Pipeline = shaderPipelineCache.GetComputePipeline(fftComplex1024ShaderTemplate, ShaderPermutationKey(), false);
    fftComplex2048Pipeline = shaderPipelineCache.GetComputePipeline(fftComplex2048ShaderTemplate, ShaderPermutationKey(), false);

    fftReal512Pipeline = shaderPipelineCache.GetComputePipeline(fftReal512ShaderTemplate, ShaderPermutationKey(), false);
    fftReal1024Pipeline = shaderPipelineCache.GetComputePipeline(fftReal1024ShaderTemplate, ShaderPermutationKey(), false);
    fftReal2048Pipeline = shaderPipelineCache.GetComputePipeline(fftReal2048ShaderTemplate, ShaderPermutationKey(), false);

    ifftReal512Pipeline = shaderPipelineCache.GetComputePipeline(ifftReal512ShaderTemplate, ShaderPermutationKey(), false);
    ifftReal1024Pipeline = shaderPipelineCache.GetComputePipeline(ifftReal1024ShaderTemplate, ShaderPermutationKey(), false);
    ifftReal2048Pipeline = shaderPipelineCache.GetComputePipeline(ifftReal2048ShaderTemplate, ShaderPermutationKey(), false);

    fftDualReal256Pipeline = shaderPipelineCache.GetComputePipeline(fftDualReal256ShaderTemplate, ShaderPermutationKey(), false);
    fftDualReal512Pipeline = shaderPipelineCache.GetComputePipeline(fftDualReal512ShaderTemplate, ShaderPermutationKey(), false);
    fftDualReal1024Pipeline = shaderPipelineCache.GetComputePipeline(fftDualReal1024ShaderTemplate, ShaderPermutationKey(), false);
    fftDualReal2048Pipeline = shaderPipelineCache.GetComputePipeline(fftDualReal2048ShaderTemplate, ShaderPermutationKey(), false);

    ifftDualReal256Pipeline = shaderPipelineCache.GetComputePipeline(ifftDualReal256ShaderTemplate, ShaderPermutationKey(), false);
    ifftDualReal512Pipeline = shaderPipelineCache.GetComputePipeline(ifftDualReal512ShaderTemplate, ShaderPermutationKey(), false);
    ifftDualReal1024Pipeline = shaderPipelineCache.GetComputePipeline(ifftDualReal1024ShaderTemplate, ShaderPermutationKey(), false);
    ifftDualReal2048Pipeline = shaderPipelineCache.GetComputePipeline(ifftDualReal2048ShaderTemplate, ShaderPermutationKey(), false);

    multiplyComplexSpectrum2DPipeline = shaderPipelineCache.GetComputePipeline(multiplyComplexSpectrum2DShaderTemplate, ShaderPermutationKey(), false);
    multiplyPackedRealSpectrum2DPipeline = shaderPipelineCache.GetComputePipeline(multiplyPackedRealSpectrum2DShaderTemplate, ShaderPermutationKey(), false);
    multiplyPackedDualRealSpectrum2DPipeline = shaderPipelineCache.GetComputePipeline(multiplyPackedDualRealSpectrum2DShaderTemplate, ShaderPermutationKey(), false);

    transposeComplex256Pipeline = shaderPipelineCache.GetComputePipeline(transposeComplex256ShaderTemplate, ShaderPermutationKey(), false);
    transposeComplex512Pipeline = shaderPipelineCache.GetComputePipeline(transposeComplex512ShaderTemplate, ShaderPermutationKey(), false);
    transposeComplex1024Pipeline = shaderPipelineCache.GetComputePipeline(transposeComplex1024ShaderTemplate, ShaderPermutationKey(), false);
    transposeComplex2048Pipeline = shaderPipelineCache.GetComputePipeline(transposeComplex2048ShaderTemplate, ShaderPermutationKey(), false);
    transposeComplex256x512Pipeline = shaderPipelineCache.GetComputePipeline(transposeComplex256x512ShaderTemplate, ShaderPermutationKey(), false);
    transposeComplex512x256Pipeline = shaderPipelineCache.GetComputePipeline(transposeComplex512x256ShaderTemplate, ShaderPermutationKey(), false);
    transposeComplex512x1024Pipeline = shaderPipelineCache.GetComputePipeline(transposeComplex512x1024ShaderTemplate, ShaderPermutationKey(), false);
    transposeComplex1024x512Pipeline = shaderPipelineCache.GetComputePipeline(transposeComplex1024x512ShaderTemplate, ShaderPermutationKey(), false);
    transposeComplex1024x2048Pipeline = shaderPipelineCache.GetComputePipeline(transposeComplex1024x2048ShaderTemplate, ShaderPermutationKey(), false);
    transposeComplex2048x1024Pipeline = shaderPipelineCache.GetComputePipeline(transposeComplex2048x1024ShaderTemplate, ShaderPermutationKey(), false);

    fftRealTwiddle512Buffer = CreateRealFFTTwiddleBuffer(512);
    fftRealTwiddle1024Buffer = CreateRealFFTTwiddleBuffer(1024);
    fftRealTwiddle2048Buffer = CreateRealFFTTwiddleBuffer(2048);

    fftDispatchTable[static_cast<uint32_t>(FFTSize::FFT256)] = FFTDispatchEntry{
        256,
        ShaderPipeline::GetReadyPSO(fftComplex256Pipeline),
        nullptr,
        nullptr,
        ShaderPipeline::GetReadyPSO(fftDualReal256Pipeline),
        ShaderPipeline::GetReadyPSO(ifftDualReal256Pipeline),
        ShaderPipeline::GetReadyPSO(transposeComplex256Pipeline),
        nullptr,
        nullptr,
        nullptr
    };

    fftDispatchTable[static_cast<uint32_t>(FFTSize::FFT512)] = FFTDispatchEntry{
        512,
        ShaderPipeline::GetReadyPSO(fftComplex512Pipeline),
        ShaderPipeline::GetReadyPSO(fftReal512Pipeline),
        ShaderPipeline::GetReadyPSO(ifftReal512Pipeline),
        ShaderPipeline::GetReadyPSO(fftDualReal512Pipeline),
        ShaderPipeline::GetReadyPSO(ifftDualReal512Pipeline),
        ShaderPipeline::GetReadyPSO(transposeComplex512Pipeline),
        ShaderPipeline::GetReadyPSO(transposeComplex256x512Pipeline),
        ShaderPipeline::GetReadyPSO(transposeComplex512x256Pipeline),
        fftRealTwiddle512Buffer
    };

    fftDispatchTable[static_cast<uint32_t>(FFTSize::FFT1024)] = FFTDispatchEntry{
        1024,
        ShaderPipeline::GetReadyPSO(fftComplex1024Pipeline),
        ShaderPipeline::GetReadyPSO(fftReal1024Pipeline),
        ShaderPipeline::GetReadyPSO(ifftReal1024Pipeline),
        ShaderPipeline::GetReadyPSO(fftDualReal1024Pipeline),
        ShaderPipeline::GetReadyPSO(ifftDualReal1024Pipeline),
        ShaderPipeline::GetReadyPSO(transposeComplex1024Pipeline),
        ShaderPipeline::GetReadyPSO(transposeComplex512x1024Pipeline),
        ShaderPipeline::GetReadyPSO(transposeComplex1024x512Pipeline),
        fftRealTwiddle1024Buffer
    };

    fftDispatchTable[static_cast<uint32_t>(FFTSize::FFT2048)] = FFTDispatchEntry{
        2048,
        ShaderPipeline::GetReadyPSO(fftComplex2048Pipeline),
        ShaderPipeline::GetReadyPSO(fftReal2048Pipeline),
        ShaderPipeline::GetReadyPSO(ifftReal2048Pipeline),
        ShaderPipeline::GetReadyPSO(fftDualReal2048Pipeline),
        ShaderPipeline::GetReadyPSO(ifftDualReal2048Pipeline),
        ShaderPipeline::GetReadyPSO(transposeComplex2048Pipeline),
        ShaderPipeline::GetReadyPSO(transposeComplex1024x2048Pipeline),
        ShaderPipeline::GetReadyPSO(transposeComplex2048x1024Pipeline),
        fftRealTwiddle2048Buffer
    };

    multiplyComplexSpectrum2DPSO = ShaderPipeline::GetReadyPSO(multiplyComplexSpectrum2DPipeline);
    multiplyPackedRealSpectrum2DPSO = ShaderPipeline::GetReadyPSO(multiplyPackedRealSpectrum2DPipeline);
    multiplyPackedDualRealSpectrum2DPSO = ShaderPipeline::GetReadyPSO(multiplyPackedDualRealSpectrum2DPipeline);
}

void GpuFFT::Shutdown() {
    fftComplex256Pipeline.reset();
    fftComplex512Pipeline.reset();
    fftComplex1024Pipeline.reset();
    fftComplex2048Pipeline.reset();

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

    for (uint32_t i = 0; i < static_cast<uint32_t>(FFTSize::Count); i++) {
        fftDispatchTable[i] = FFTDispatchEntry{};
    }

    multiplyComplexSpectrum2DPSO = nullptr;
    multiplyPackedRealSpectrum2DPSO = nullptr;
    multiplyPackedDualRealSpectrum2DPSO = nullptr;
}

bool GpuFFT::GetGpuFFTSize(uint32_t fftSizeValue, GpuFFT::FFTSize &fftSize) {
    switch (fftSizeValue) {
    case 256:
        fftSize = GpuFFT::FFTSize::FFT256;
        return true;
    case 512:
        fftSize = GpuFFT::FFTSize::FFT512;
        return true;
    case 1024:
        fftSize = GpuFFT::FFTSize::FFT1024;
        return true;
    case 2048:
        fftSize = GpuFFT::FFTSize::FFT2048;
        return true;
    default:
        assert(false);
        return false;
    }
    return false;
}

bool GpuFFT::DispatchFFTComplex1D(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, FFTSize fftSize, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTComplex1D");

    const uint32_t fftSizeIndex = static_cast<uint32_t>(fftSize);
    if (fftSizeIndex >= static_cast<uint32_t>(FFTSize::Count)) {
        assert(false);
        return false;
    }

    const FFTDispatchEntry &entry = fftDispatchTable[fftSizeIndex];
    RHI::PipelineState *pso = entry.fftComplexPSO;
    if (!pso) {
        return false;
    }

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    if (!DispatchFFTComplexFused(commandList, frameThreadData, pso, inputBuffer, outputBuffer, fftBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined, -1.0f, 1.0f)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchIFFTComplex1D(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, FFTSize fftSize, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTComplex1D");

    const uint32_t fftSizeIndex = static_cast<uint32_t>(fftSize);
    if (fftSizeIndex >= static_cast<uint32_t>(FFTSize::Count)) {
        assert(false);
        return false;
    }

    const FFTDispatchEntry &entry = fftDispatchTable[fftSizeIndex];
    RHI::PipelineState *pso = entry.fftComplexPSO;
    if (!pso) {
        return false;
    }

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const float scale = 1.0f / static_cast<float>(entry.fftSize);
    if (!DispatchFFTComplexFused(commandList, frameThreadData, pso, inputBuffer, outputBuffer, fftBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined, 1.0f, scale)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchFFTComplex2D(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, FFTSize fftSize, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTComplex2D");

    const uint32_t fftSizeIndex = static_cast<uint32_t>(fftSize);
    if (fftSizeIndex >= static_cast<uint32_t>(FFTSize::Count)) {
        assert(false);
        return false;
    }

    const FFTDispatchEntry &entry = fftDispatchTable[fftSizeIndex];
    RHI::PipelineState *fftPso = entry.fftComplexPSO;
    RHI::PipelineState *transposePso = entry.transposeComplexPSO;
    if (!fftPso || !transposePso) {
        return false;
    }

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t FFTBlockCount = entry.fftSize * planeCount;
    const uint32_t TransposeTileCount = entry.fftSize / TransposeComplexTileSize;
    constexpr float sign = -1.0f;
    constexpr float scale = 1.0f;

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 가로 FFT 스테이지
    RHI::Buffer *stageInput = outputBuffer;
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftPso, inputBuffer, stageInput, FFTBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined, sign, scale)) {
        return false;
    }

    // transpose
    RHI::Buffer *transposeOutput = workBuffer;
    RHI::GPUBarrier transposeBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
    dispatchBufferPSO(transposePso, stageInput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 세로 FFT 스테이지
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftPso, transposeOutput, outputBuffer, FFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute, sign, scale)) {
        return false;
    }

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool GpuFFT::DispatchIFFTComplex2D(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, FFTSize fftSize, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTComplex2D");

    const uint32_t fftSizeIndex = static_cast<uint32_t>(fftSize);
    if (fftSizeIndex >= static_cast<uint32_t>(FFTSize::Count)) {
        assert(false);
        return false;
    }

    const FFTDispatchEntry &entry = fftDispatchTable[fftSizeIndex];
    RHI::PipelineState *fftPso = entry.fftComplexPSO;
    RHI::PipelineState *transposePso = entry.transposeComplexPSO;
    if (!fftPso || !transposePso) {
        return false;
    }

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t FFTBlockCount = entry.fftSize * planeCount;
    const uint32_t TransposeTileCount = entry.fftSize / TransposeComplexTileSize;
    constexpr float sign = 1.0f;
    const float scale = 1.0f / static_cast<float>(entry.fftSize);

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 가로 IFFT 스테이지
    RHI::Buffer *stageInput = workBuffer;
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftPso, inputBuffer, stageInput, FFTBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined, sign, scale)) {
        return false;
    }

    // transpose
    RHI::Buffer *transposeOutput = inputBuffer;
    RHI::GPUBarrier transposeBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
    dispatchBufferPSO(transposePso, stageInput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 세로 IFFT 스테이지
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftPso, transposeOutput, outputBuffer, FFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined, sign, scale)) {
        return false;
    }

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool GpuFFT::DispatchFFTReal1D(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, FFTSize fftSize, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTReal1D");

    const uint32_t fftSizeIndex = static_cast<uint32_t>(fftSize);
    if (fftSizeIndex >= static_cast<uint32_t>(FFTSize::Count)) {
        assert(false);
        return false;
    }

    const FFTDispatchEntry &entry = fftDispatchTable[fftSizeIndex];
    RHI::PipelineState *pso = entry.fftRealPSO;
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
    RHI::PipelineState *pso = entry.ifftRealPSO;
    RHI::Buffer *twiddleBuffer = entry.fftRealTwiddleBuffer;
    if (!pso || !twiddleBuffer) {
        return false;
    }

    return DispatchIFFTRealFused(commandList, pso, twiddleBuffer, inputBuffer, outputBuffer, nullptr, fftBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined);
}

bool GpuFFT::DispatchFFTReal2D(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, FFTSize fftSize, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTReal2D");

    const uint32_t fftSizeIndex = static_cast<uint32_t>(fftSize);
    if (fftSizeIndex >= static_cast<uint32_t>(FFTSize::Count)) {
        assert(false);
        return false;
    }

    const FFTDispatchEntry &entry = fftDispatchTable[fftSizeIndex];
    RHI::PipelineState *realPso = entry.fftRealPSO;
    RHI::PipelineState *complexPso = entry.fftComplexPSO;
    RHI::PipelineState *transposePso = entry.transposeComplexPackedToFullPSO;
    RHI::Buffer *twiddleBuffer = entry.fftRealTwiddleBuffer;
    if (!realPso || !complexPso || !transposePso || !twiddleBuffer) {
        return false;
    }

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

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
    if (!DispatchFFTComplexFused(commandList, frameThreadData, complexPso, transposeOutput, outputBuffer, VerticalComplexFFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute, -1.0f, 1.0f)) {
        return false;
    }

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool GpuFFT::DispatchIFFTReal2D(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, FFTSize fftSize, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTReal2D");

    const uint32_t fftSizeIndex = static_cast<uint32_t>(fftSize);
    if (fftSizeIndex >= static_cast<uint32_t>(FFTSize::Count)) {
        assert(false);
        return false;
    }

    const FFTDispatchEntry &entry = fftDispatchTable[fftSizeIndex];
    RHI::PipelineState *realPso = entry.ifftRealPSO;
    RHI::PipelineState *complexPso = entry.fftComplexPSO;
    RHI::PipelineState *transposePso = entry.transposeComplexFullToPackedPSO;
    RHI::Buffer *twiddleBuffer = entry.fftRealTwiddleBuffer;
    if (!realPso || !complexPso || !transposePso || !twiddleBuffer) {
        return false;
    }

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

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
    if (!DispatchFFTComplexFused(commandList, frameThreadData, complexPso, inputBuffer, stageInput, VerticalComplexFFTBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined, 1.0f, 1.0f / static_cast<float>(entry.fftSize))) {
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
    RHI::PipelineState *pso = entry.fftDualRealPSO;
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
    RHI::PipelineState *pso = entry.ifftDualRealPSO;
    if (!pso) {
        return false;
    }

    return DispatchIFFTDualRealFused(commandList, pso, inputBuffer, outputBuffer, nullptr, fftBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined);
}

bool GpuFFT::DispatchFFTDualReal2D(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, FFTSize fftSize, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTDualReal2D");

    const uint32_t fftSizeIndex = static_cast<uint32_t>(fftSize);
    if (fftSizeIndex >= static_cast<uint32_t>(FFTSize::Count)) {
        assert(false);
        return false;
    }

    const FFTDispatchEntry &entry = fftDispatchTable[fftSizeIndex];
    RHI::PipelineState *dualRealPso = entry.fftDualRealPSO;
    RHI::PipelineState *complexPso = entry.fftComplexPSO;
    RHI::PipelineState *transposePso = entry.transposeComplexPSO;
    if (!dualRealPso || !complexPso || !transposePso) {
        return false;
    }

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

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
    if (!DispatchFFTComplexFused(commandList, frameThreadData, complexPso, transposeOutput, outputBuffer, FFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute, -1.0f, 1.0f)) {
        return false;
    }

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool GpuFFT::DispatchIFFTDualReal2D(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, FFTSize fftSize, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTDualReal2D");

    const uint32_t fftSizeIndex = static_cast<uint32_t>(fftSize);
    if (fftSizeIndex >= static_cast<uint32_t>(FFTSize::Count)) {
        assert(false);
        return false;
    }

    const FFTDispatchEntry &entry = fftDispatchTable[fftSizeIndex];
    RHI::PipelineState *complexPso = entry.fftComplexPSO;
    RHI::PipelineState *dualRealPso = entry.ifftDualRealPSO;
    RHI::PipelineState *transposePso = entry.transposeComplexPSO;
    if (!complexPso || !dualRealPso || !transposePso) {
        return false;
    }

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

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
    if (!DispatchFFTComplexFused(commandList, frameThreadData, complexPso, inputBuffer, verticalOutput, FFTBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined, 1.0f, 1.0f / static_cast<float>(entry.fftSize))) {
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

bool GpuFFT::DispatchMultiplyComplexSpectrum2D(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *lhsSpectrumBuffer, RHI::Buffer *rhsSpectrumBuffer, RHI::Buffer *outputSpectrumBuffer, uint32_t fftSize, uint32_t planeCount, float scale) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchMultiplyComplexSpectrum2D");

    assert(outputSpectrumBuffer != lhsSpectrumBuffer);
    assert(outputSpectrumBuffer != rhsSpectrumBuffer);

    if (fftSize == 0) {
        assert(false);
        return false;
    }

    if (!multiplyComplexSpectrum2DPSO) {
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

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);
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

    RHI::renderer->SetPSO(commandList, multiplyComplexSpectrum2DPSO);
    RHI::renderer->SetBuffer(commandList, 0, false, lhsSpectrumBuffer);
    RHI::renderer->SetBuffer(commandList, 1, false, rhsSpectrumBuffer);
    RHI::renderer->SetBuffer(commandList, 0, true, outputSpectrumBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 0, multiplyConstantBuffer);

    const uint32_t threadGroupCount = (elementCount + MultiplySpectrum2DBlockSize - 1) / MultiplySpectrum2DBlockSize;
    RHI::renderer->Dispatch(commandList, threadGroupCount, 1, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(lhsSpectrumBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(rhsSpectrumBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputSpectrumBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool GpuFFT::DispatchMultiplyPackedRealSpectrum2D(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *lhsSpectrumBuffer, RHI::Buffer *rhsSpectrumBuffer, RHI::Buffer *outputSpectrumBuffer, uint32_t fftSize, uint32_t planeCount, float scale) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchMultiplyPackedRealSpectrum2D");

    assert(outputSpectrumBuffer != lhsSpectrumBuffer);
    assert(outputSpectrumBuffer != rhsSpectrumBuffer);

    if (fftSize < 2 || (fftSize & 1) != 0) {
        assert(false);
        return false;
    }

    if (!multiplyPackedRealSpectrum2DPSO) {
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

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);
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

    RHI::renderer->SetPSO(commandList, multiplyPackedRealSpectrum2DPSO);
    RHI::renderer->SetBuffer(commandList, 0, false, lhsSpectrumBuffer);
    RHI::renderer->SetBuffer(commandList, 1, false, rhsSpectrumBuffer);
    RHI::renderer->SetBuffer(commandList, 0, true, outputSpectrumBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 0, multiplyConstantBuffer);

    const uint32_t threadGroupCount = (elementCount + MultiplySpectrum2DBlockSize - 1) / MultiplySpectrum2DBlockSize;
    RHI::renderer->Dispatch(commandList, threadGroupCount, 1, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(lhsSpectrumBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(rhsSpectrumBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputSpectrumBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool GpuFFT::DispatchMultiplyPackedDualRealSpectrum2D(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *lhsSpectrumBuffer, RHI::Buffer *rhsSpectrumBuffer, RHI::Buffer *outputSpectrumBuffer, uint32_t fftSize, uint32_t planeCount, float scale) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchMultiplyPackedDualRealSpectrum2D");

    assert(outputSpectrumBuffer != lhsSpectrumBuffer);
    assert(outputSpectrumBuffer != rhsSpectrumBuffer);

    if (fftSize < 2 || (fftSize & 1) != 0) {
        assert(false);
        return false;
    }

    if (!multiplyPackedDualRealSpectrum2DPSO) {
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

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);
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

    RHI::renderer->SetPSO(commandList, multiplyPackedDualRealSpectrum2DPSO);
    RHI::renderer->SetBuffer(commandList, 0, false, lhsSpectrumBuffer);
    RHI::renderer->SetBuffer(commandList, 1, false, rhsSpectrumBuffer);
    RHI::renderer->SetBuffer(commandList, 0, true, outputSpectrumBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 0, multiplyConstantBuffer);

    const uint32_t threadGroupCount = (elementCount + MultiplySpectrum2DBlockSize - 1) / MultiplySpectrum2DBlockSize;
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

static RHI::ConstantBuffer *AllocFFTStageConstantBuffer(RHI::FrameThreadData *frameThreadData, float sign, float scale) {
    FFTStageParams fftParams = {};
    fftParams.sign = sign;
    fftParams.scale = scale;

    return frameThreadData->AllocConstant(sizeof(FFTStageParams), &fftParams);
}


bool GpuFFT::DispatchFFTComplexFused(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::PipelineState *pso, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount, RHI::GPUResourceState inputBufferState, RHI::GPUResourceState outputBufferState, float sign, float scale) {
    RHI::ConstantBuffer *fftConstantBuffer = AllocFFTStageConstantBuffer(frameThreadData, sign, scale);
    if (!fftConstantBuffer) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, inputBufferState, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, outputBufferState, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetBuffer(commandList, 0, false, inputBuffer);
    RHI::renderer->SetBuffer(commandList, 0, true, outputBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 0, fftConstantBuffer);
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
