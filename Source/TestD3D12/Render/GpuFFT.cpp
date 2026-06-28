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
#include "ShaderTemplate.h"
#include "ShaderPipelineCache.h"
#include "Shaders/ShaderInterop.h"

void GpuFFT::Init() {
    fftComplex256Pipeline = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftComplex256ShaderTemplate, ShaderPermutationKey(), false);
    fftComplex512Pipeline = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftComplex512ShaderTemplate, ShaderPermutationKey(), false);
    fftComplex1024Pipeline = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftComplex1024ShaderTemplate, ShaderPermutationKey(), false);

    fftReal512Pipeline = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftReal512ShaderTemplate, ShaderPermutationKey(), false);
    fftReal1024Pipeline = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftReal1024ShaderTemplate, ShaderPermutationKey(), false);

    ifftReal512Pipeline = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.ifftReal512ShaderTemplate, ShaderPermutationKey(), false);
    ifftReal1024Pipeline = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.ifftReal1024ShaderTemplate, ShaderPermutationKey(), false);

    fftDualReal256Pipeline = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftDualReal256ShaderTemplate, ShaderPermutationKey(), false);
    fftDualReal512Pipeline = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftDualReal512ShaderTemplate, ShaderPermutationKey(), false);
    fftDualReal1024Pipeline = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.fftDualReal1024ShaderTemplate, ShaderPermutationKey(), false);

    ifftDualReal256Pipeline = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.ifftDualReal256ShaderTemplate, ShaderPermutationKey(), false);
    ifftDualReal512Pipeline = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.ifftDualReal512ShaderTemplate, ShaderPermutationKey(), false);
    ifftDualReal1024Pipeline = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.ifftDualReal1024ShaderTemplate, ShaderPermutationKey(), false);

    multiplyComplexSpectrum2DPipeline = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.multiplyComplexSpectrum2DShaderTemplate, ShaderPermutationKey(), false);
    multiplyPackedRealSpectrum2DPipeline = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.multiplyPackedRealSpectrum2DShaderTemplate, ShaderPermutationKey(), false);
    multiplyPackedDualRealSpectrum2DPipeline = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.multiplyPackedDualRealSpectrum2DShaderTemplate, ShaderPermutationKey(), false);

    transposeComplex256Pipeline = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.transposeComplex256ShaderTemplate, ShaderPermutationKey(), false);
    transposeComplex256x512Pipeline = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.transposeComplex256x512ShaderTemplate, ShaderPermutationKey(), false);
    transposeComplex512x256Pipeline = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.transposeComplex512x256ShaderTemplate, ShaderPermutationKey(), false);
    transposeComplex512x1024Pipeline = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.transposeComplex512x1024ShaderTemplate, ShaderPermutationKey(), false);
    transposeComplex1024x512Pipeline = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.transposeComplex1024x512ShaderTemplate, ShaderPermutationKey(), false);
    transposeComplex512Pipeline = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.transposeComplex512ShaderTemplate, ShaderPermutationKey(), false);
    transposeComplex1024Pipeline = shaderPipelineCache.GetComputePipeline(shaderTemplateManager.transposeComplex1024ShaderTemplate, ShaderPermutationKey(), false);

    UpdateReadyPSOs();

    fftRealTwiddle512Buffer = CreateRealFFTTwiddleBuffer(512);
    fftRealTwiddle1024Buffer = CreateRealFFTTwiddleBuffer(1024);
}

void GpuFFT::Shutdown() {
    fftComplex256Pipeline.reset();
    fftComplex512Pipeline.reset();
    fftComplex1024Pipeline.reset();

    fftReal512Pipeline.reset();
    fftReal1024Pipeline.reset();

    ifftReal512Pipeline.reset();
    ifftReal1024Pipeline.reset();

    fftDualReal256Pipeline.reset();
    fftDualReal512Pipeline.reset();
    fftDualReal1024Pipeline.reset();

    ifftDualReal256Pipeline.reset();
    ifftDualReal512Pipeline.reset();
    ifftDualReal1024Pipeline.reset();

    multiplyComplexSpectrum2DPipeline.reset();
    multiplyPackedRealSpectrum2DPipeline.reset();
    multiplyPackedDualRealSpectrum2DPipeline.reset();

    transposeComplex256Pipeline.reset();
    transposeComplex256x512Pipeline.reset();
    transposeComplex512x256Pipeline.reset();
    transposeComplex512x1024Pipeline.reset();
    transposeComplex1024x512Pipeline.reset();
    transposeComplex512Pipeline.reset();
    transposeComplex1024Pipeline.reset();

    UpdateReadyPSOs();

    if (fftRealTwiddle1024Buffer) {
        RHI::renderer->DestroyBuffer(fftRealTwiddle1024Buffer);
        fftRealTwiddle1024Buffer = nullptr;
    }

    if (fftRealTwiddle512Buffer) {
        RHI::renderer->DestroyBuffer(fftRealTwiddle512Buffer);
        fftRealTwiddle512Buffer = nullptr;
    }
}

void GpuFFT::UpdateReadyPSOs() {
    fftComplex256PSO = ShaderPipeline::GetReadyPSO(fftComplex256Pipeline);
    fftComplex512PSO = ShaderPipeline::GetReadyPSO(fftComplex512Pipeline);
    fftComplex1024PSO = ShaderPipeline::GetReadyPSO(fftComplex1024Pipeline);

    fftReal512PSO = ShaderPipeline::GetReadyPSO(fftReal512Pipeline);
    fftReal1024PSO = ShaderPipeline::GetReadyPSO(fftReal1024Pipeline);

    ifftReal512PSO = ShaderPipeline::GetReadyPSO(ifftReal512Pipeline);
    ifftReal1024PSO = ShaderPipeline::GetReadyPSO(ifftReal1024Pipeline);

    fftDualReal256PSO = ShaderPipeline::GetReadyPSO(fftDualReal256Pipeline);
    fftDualReal512PSO = ShaderPipeline::GetReadyPSO(fftDualReal512Pipeline);
    fftDualReal1024PSO = ShaderPipeline::GetReadyPSO(fftDualReal1024Pipeline);

    ifftDualReal256PSO = ShaderPipeline::GetReadyPSO(ifftDualReal256Pipeline);
    ifftDualReal512PSO = ShaderPipeline::GetReadyPSO(ifftDualReal512Pipeline);
    ifftDualReal1024PSO = ShaderPipeline::GetReadyPSO(ifftDualReal1024Pipeline);

    multiplyComplexSpectrum2DPSO = ShaderPipeline::GetReadyPSO(multiplyComplexSpectrum2DPipeline);
    multiplyPackedRealSpectrum2DPSO = ShaderPipeline::GetReadyPSO(multiplyPackedRealSpectrum2DPipeline);
    multiplyPackedDualRealSpectrum2DPSO = ShaderPipeline::GetReadyPSO(multiplyPackedDualRealSpectrum2DPipeline);

    transposeComplex256PSO = ShaderPipeline::GetReadyPSO(transposeComplex256Pipeline);
    transposeComplex256x512PSO = ShaderPipeline::GetReadyPSO(transposeComplex256x512Pipeline);
    transposeComplex512x256PSO = ShaderPipeline::GetReadyPSO(transposeComplex512x256Pipeline);
    transposeComplex512x1024PSO = ShaderPipeline::GetReadyPSO(transposeComplex512x1024Pipeline);
    transposeComplex1024x512PSO = ShaderPipeline::GetReadyPSO(transposeComplex1024x512Pipeline);
    transposeComplex512PSO = ShaderPipeline::GetReadyPSO(transposeComplex512Pipeline);
    transposeComplex1024PSO = ShaderPipeline::GetReadyPSO(transposeComplex1024Pipeline);
}

bool GpuFFT::DispatchFFTComplex1D256(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTComplex1D256");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex256PSO, inputBuffer, outputBuffer, fftBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined, -1.0f, 1.0f)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchFFTComplex1D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTComplex1D512");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex512PSO, inputBuffer, outputBuffer, fftBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined, -1.0f, 1.0f)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchFFTComplex1D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTComplex1D1024");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex1024PSO, inputBuffer, outputBuffer, fftBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined, -1.0f, 1.0f)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchIFFTComplex1D256(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTComplex1D256");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex256PSO, inputBuffer, outputBuffer, fftBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined, 1.0f, 1.0f / 256.0f)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchIFFTComplex1D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTComplex1D512");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex512PSO, inputBuffer, outputBuffer, fftBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined, 1.0f, 1.0f / 512.0f)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchIFFTComplex1D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTComplex1D1024");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex1024PSO, inputBuffer, outputBuffer, fftBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined, 1.0f, 1.0f / 1024.0f)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchFFTComplex2D256(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTComplex2D256");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t FFTBlockCount = 256 * planeCount;
    constexpr uint32_t TransposeTileCount = 256 / TransposeComplexTileSize;
    constexpr float sign = -1.0f;
    constexpr float scale = 1.0f;

    RHI::Buffer *pingBuffer0 = outputBuffer;
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 가로 FFT 스테이지
    RHI::Buffer *stageInput = pingBuffer0;
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex256PSO, inputBuffer, stageInput, FFTBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined, sign, scale)) {
        return false;
    }

    // transpose
    RHI::Buffer *transposeOutput = pingBuffer1;
    RHI::GPUBarrier transposeBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
    dispatchBufferPSO(transposeComplex256PSO, stageInput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 세로 FFT 스테이지
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex256PSO, transposeOutput, outputBuffer, FFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute, sign, scale)) {
        return false;
    }

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool GpuFFT::DispatchFFTComplex2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTComplex2D512");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t FFTBlockCount = 512 * planeCount;
    constexpr uint32_t TransposeTileCount = 512 / TransposeComplexTileSize;
    constexpr float sign = -1.0f;
    constexpr float scale = 1.0f;

    RHI::Buffer *pingBuffer0 = outputBuffer;
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 가로 FFT 스테이지
    RHI::Buffer *stageInput = pingBuffer0;
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex512PSO, inputBuffer, stageInput, FFTBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined, sign, scale)) {
        return false;
    }

    // transpose
    RHI::Buffer *transposeOutput = pingBuffer1;
    RHI::GPUBarrier transposeBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
    dispatchBufferPSO(transposeComplex512PSO, stageInput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 세로 FFT 스테이지
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex512PSO, transposeOutput, outputBuffer, FFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute, sign, scale)) {
        return false;
    }

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool GpuFFT::DispatchFFTComplex2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTComplex2D1024");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t FFTBlockCount = 1024 * planeCount;
    constexpr uint32_t TransposeTileCount = 1024 / TransposeComplexTileSize;
    constexpr float sign = -1.0f;
    constexpr float scale = 1.0f;

    RHI::Buffer *pingBuffer0 = outputBuffer;
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 가로 FFT 스테이지
    RHI::Buffer *stageInput = pingBuffer0;
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex1024PSO, inputBuffer, stageInput, FFTBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined, sign, scale)) {
        return false;
    }

    // transpose
    RHI::Buffer *transposeOutput = pingBuffer1;
    RHI::GPUBarrier transposeBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
    dispatchBufferPSO(transposeComplex1024PSO, stageInput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 세로 FFT 스테이지
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex1024PSO, transposeOutput, outputBuffer, FFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute, sign, scale)) {
        return false;
    }

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool GpuFFT::DispatchIFFTComplex2D256(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTComplex2D256");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t FFTBlockCount = 256 * planeCount;
    constexpr uint32_t TransposeTileCount = 256 / TransposeComplexTileSize;
    constexpr float sign = 1.0f;
    constexpr float scale = 1.0f / 256.0f;

    RHI::Buffer *pingBuffer0 = inputBuffer;
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 가로 IFFT 스테이지
    RHI::Buffer *stageInput = pingBuffer1;
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex256PSO, inputBuffer, stageInput, FFTBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined, sign, scale)) {
        return false;
    }

    // transpose
    RHI::Buffer *transposeOutput = pingBuffer0;
    RHI::GPUBarrier transposeBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
    dispatchBufferPSO(transposeComplex256PSO, stageInput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 세로 IFFT 스테이지
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex256PSO, transposeOutput, outputBuffer, FFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined, sign, scale)) {
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

bool GpuFFT::DispatchIFFTComplex2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTComplex2D512");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t FFTBlockCount = 512 * planeCount;
    constexpr uint32_t TransposeTileCount = 512 / TransposeComplexTileSize;
    constexpr float sign = 1.0f;
    constexpr float scale = 1.0f / 512.0f;

    RHI::Buffer *pingBuffer0 = inputBuffer;
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 가로 IFFT 스테이지
    RHI::Buffer *stageInput = pingBuffer1;
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex512PSO, inputBuffer, stageInput, FFTBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined, sign, scale)) {
        return false;
    }

    // transpose
    RHI::Buffer *transposeOutput = pingBuffer0;
    RHI::GPUBarrier transposeBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
    dispatchBufferPSO(transposeComplex512PSO, stageInput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 세로 IFFT 스테이지
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex512PSO, transposeOutput, outputBuffer, FFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined, sign, scale)) {
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

bool GpuFFT::DispatchIFFTComplex2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTComplex2D1024");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t FFTBlockCount = 1024 * planeCount;
    constexpr uint32_t TransposeTileCount = 1024 / TransposeComplexTileSize;
    constexpr float sign = 1.0f;
    constexpr float scale = 1.0f / 1024.0f;

    RHI::Buffer *pingBuffer0 = inputBuffer;
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 가로 IFFT 스테이지
    RHI::Buffer *stageInput = pingBuffer1;
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex1024PSO, inputBuffer, stageInput, FFTBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined, sign, scale)) {
        return false;
    }

    // transpose
    RHI::Buffer *transposeOutput = pingBuffer0;
    RHI::GPUBarrier transposeBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
    dispatchBufferPSO(transposeComplex1024PSO, stageInput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 세로 IFFT 스테이지
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex1024PSO, transposeOutput, outputBuffer, FFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined, sign, scale)) {
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

bool GpuFFT::DispatchFFTReal1D512(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTReal1D512");

    RHI::Buffer *resultBuffer = nullptr;
    if (!DispatchFFTReal512Fused(commandList, inputBuffer, outputBuffer, fftBlockCount, &resultBuffer)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchFFTReal1D1024(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTReal1D1024");

    RHI::Buffer *resultBuffer = nullptr;
    if (!DispatchFFTReal1024Fused(commandList, inputBuffer, outputBuffer, fftBlockCount, &resultBuffer)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchIFFTReal1D512(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTReal1D512");

    return DispatchIFFTReal512Fused(commandList, inputBuffer, outputBuffer, nullptr, fftBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined);
}

bool GpuFFT::DispatchIFFTReal1D1024(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTReal1D1024");

    return DispatchIFFTReal1024Fused(commandList, inputBuffer, outputBuffer, nullptr, fftBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined);
}

bool GpuFFT::DispatchFFTReal2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTReal2D512");

    assert(fftRealTwiddle512Buffer);

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t RealFFTBlockCount = 512 * planeCount;
    const uint32_t VerticalComplexFFTBlockCount = 256 * planeCount;
    constexpr uint32_t TransposeTileCountX = 256 / TransposeComplexTileSize;
    constexpr uint32_t TransposeTileCountY = 512 / TransposeComplexTileSize;

    RHI::Buffer *pingBuffer0 = outputBuffer;
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto dispatchBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 가로 FFT 스테이지
    RHI::Buffer *stageInput = nullptr;
    if (!DispatchFFTReal512Fused(commandList, inputBuffer, pingBuffer0, RealFFTBlockCount, &stageInput)) {
        return false;
    }

    // 256 x 512 packed spectrum 을 512 x 256 으로 transpose 해서 세로 Complex FFT512 를 수행한다.
    RHI::Buffer *transposeOutput = pingBuffer1;
    RHI::GPUBarrier transposeBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
    dispatchBufferPSO(transposeComplex256x512PSO, stageInput, transposeOutput, TransposeTileCountX, TransposeTileCountY, planeCount);

    // 세로 FFT 스테이지
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex512PSO, transposeOutput, outputBuffer, VerticalComplexFFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute, -1.0f, 1.0f)) {
        return false;
    }

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool GpuFFT::DispatchFFTReal2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTReal2D1024");

    assert(fftRealTwiddle1024Buffer);

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t RealFFTBlockCount = 1024 * planeCount;
    const uint32_t VerticalComplexFFTBlockCount = 512 * planeCount;
    constexpr uint32_t TransposeTileCountX = 512 / TransposeComplexTileSize;
    constexpr uint32_t TransposeTileCountY = 1024 / TransposeComplexTileSize;

    RHI::Buffer *pingBuffer0 = outputBuffer;
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto dispatchBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 가로 FFT 스테이지
    RHI::Buffer *stageInput = nullptr;
    if (!DispatchFFTReal1024Fused(commandList, inputBuffer, pingBuffer0, RealFFTBlockCount, &stageInput)) {
        return false;
    }

    // 512 x 1024 packed spectrum 을 1024 x 512 로 transpose 해서 세로 Complex FFT1024 를 수행한다.
    RHI::Buffer *transposeOutput = pingBuffer1;
    RHI::GPUBarrier transposeBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
    dispatchBufferPSO(transposeComplex512x1024PSO, stageInput, transposeOutput, TransposeTileCountX, TransposeTileCountY, planeCount);

    // 세로 FFT 스테이지
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex1024PSO, transposeOutput, outputBuffer, VerticalComplexFFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute, -1.0f, 1.0f)) {
        return false;
    }

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool GpuFFT::DispatchIFFTReal2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTReal2D512");

    assert(fftRealTwiddle512Buffer);

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t RealFFTBlockCount = 512 * planeCount;
    const uint32_t VerticalComplexFFTBlockCount = 256 * planeCount;
    constexpr uint32_t TransposeTileCountX = 512 / TransposeComplexTileSize;
    constexpr uint32_t TransposeTileCountY = 256 / TransposeComplexTileSize;

    RHI::Buffer *pingBuffer0 = inputBuffer; // Input is consumed as ping-pong storage.
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto dispatchBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 세로 IFFT 스테이지
    RHI::Buffer *stageInput = pingBuffer1;
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex512PSO, pingBuffer0, stageInput, VerticalComplexFFTBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined, 1.0f, 1.0f / 512.0f)) {
        return false;
    }

    // 512 x 256 spectrum 을 256 x 512 로 transpose 해서 가로 Real IFFT512 를 수행한다.
    RHI::Buffer *transposeOutput = (stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0;
    RHI::GPUBarrier transposeBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
    dispatchBufferPSO(transposeComplex512x256PSO, stageInput, transposeOutput, TransposeTileCountX, TransposeTileCountY, planeCount);

    // 가로 IFFT 스테이지
    return DispatchIFFTReal512Fused(commandList, transposeOutput, outputBuffer, stageInput, RealFFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute);
}

bool GpuFFT::DispatchIFFTReal2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTReal2D1024");

    assert(fftRealTwiddle1024Buffer);

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t RealFFTBlockCount = 1024 * planeCount;
    const uint32_t VerticalComplexFFTBlockCount = 512 * planeCount;
    constexpr uint32_t TransposeTileCountX = 1024 / TransposeComplexTileSize;
    constexpr uint32_t TransposeTileCountY = 512 / TransposeComplexTileSize;

    RHI::Buffer *pingBuffer0 = inputBuffer; // Input is consumed as ping-pong storage.
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto dispatchBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 세로 IFFT 스테이지
    RHI::Buffer *stageInput = pingBuffer1;
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex1024PSO, pingBuffer0, stageInput, VerticalComplexFFTBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined, 1.0f, 1.0f / 1024.0f)) {
        return false;
    }

    // 1024 x 512 spectrum 을 512 x 1024 로 transpose 해서 가로 Real IFFT1024 를 수행한다.
    RHI::Buffer *transposeOutput = (stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0;
    RHI::GPUBarrier transposeBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
    dispatchBufferPSO(transposeComplex1024x512PSO, stageInput, transposeOutput, TransposeTileCountX, TransposeTileCountY, planeCount);

    // 가로 IFFT 스테이지
    return DispatchIFFTReal1024Fused(commandList, transposeOutput, outputBuffer, stageInput, RealFFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute);
}

bool GpuFFT::DispatchFFTDualReal1D256(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTDualReal1D256");

    RHI::Buffer *resultBuffer = nullptr;
    if (!DispatchFFTDualReal256Fused(commandList, inputBuffer, outputBuffer, fftBlockCount, &resultBuffer)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchFFTDualReal1D512(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTDualReal1D512");

    RHI::Buffer *resultBuffer = nullptr;
    if (!DispatchFFTDualReal512Fused(commandList, inputBuffer, outputBuffer, fftBlockCount, &resultBuffer)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchFFTDualReal1D1024(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTDualReal1D1024");

    RHI::Buffer *resultBuffer = nullptr;
    if (!DispatchFFTDualReal1024Fused(commandList, inputBuffer, outputBuffer, fftBlockCount, &resultBuffer)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchIFFTDualReal1D256(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTDualReal1D256");

    return DispatchIFFTDualReal256Fused(commandList, inputBuffer, outputBuffer, nullptr, fftBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined);
}

bool GpuFFT::DispatchIFFTDualReal1D512(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTDualReal1D512");

    return DispatchIFFTDualReal512Fused(commandList, inputBuffer, outputBuffer, nullptr, fftBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined);
}

bool GpuFFT::DispatchIFFTDualReal1D1024(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTDualReal1D1024");

    return DispatchIFFTDualReal1024Fused(commandList, inputBuffer, outputBuffer, nullptr, fftBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined);
}

bool GpuFFT::DispatchFFTDualReal2D256(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTDualReal2D256");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t FFTBlockCount = 256 * planeCount;
    constexpr uint32_t TransposeTileCount = 256 / TransposeComplexTileSize;

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 가로 FFT 스테이지
    RHI::Buffer *horizontalOutput = outputBuffer;
    RHI::Buffer *stageInput = nullptr;
    if (!DispatchFFTDualReal256Fused(commandList, inputBuffer, horizontalOutput, FFTBlockCount, &stageInput)) {
        return false;
    }

    // 256 x 256 transpose 해서 다음 FFT 스테이지는 세로로 하게 된다.
    RHI::Buffer *transposeOutput = workBuffer;
    RHI::GPUBarrier transposeBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
    dispatchBufferPSO(transposeComplex256PSO, stageInput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 세로 FFT 스테이지
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex256PSO, transposeOutput, outputBuffer, FFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute, -1.0f, 1.0f)) {
        return false;
    }

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool GpuFFT::DispatchFFTDualReal2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTDualReal2D512");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t FFTBlockCount = 512 * planeCount;
    constexpr uint32_t TransposeTileCount = 512 / TransposeComplexTileSize;

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 가로 FFT 스테이지
    RHI::Buffer *horizontalOutput = outputBuffer;
    RHI::Buffer *stageInput = nullptr;
    if (!DispatchFFTDualReal512Fused(commandList, inputBuffer, horizontalOutput, FFTBlockCount, &stageInput)) {
        return false;
    }

    // 512 x 512 transpose 해서 다음 FFT 스테이지는 세로로 하게 된다.
    RHI::Buffer *transposeOutput = workBuffer;
    RHI::GPUBarrier transposeBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
    dispatchBufferPSO(transposeComplex512PSO, stageInput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 세로 FFT 스테이지
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex512PSO, transposeOutput, outputBuffer, FFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute, -1.0f, 1.0f)) {
        return false;
    }

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool GpuFFT::DispatchFFTDualReal2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTDualReal2D1024");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t FFTBlockCount = 1024 * planeCount;
    constexpr uint32_t TransposeTileCount = 1024 / TransposeComplexTileSize;

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 가로 FFT 스테이지
    RHI::Buffer *horizontalOutput = outputBuffer;
    RHI::Buffer *stageInput = nullptr;
    if (!DispatchFFTDualReal1024Fused(commandList, inputBuffer, horizontalOutput, FFTBlockCount, &stageInput)) {
        return false;
    }

    // 1024 x 1024 transpose 해서 다음 FFT 스테이지는 세로로 하게 된다.
    RHI::Buffer *transposeOutput = workBuffer;
    RHI::GPUBarrier transposeBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
    dispatchBufferPSO(transposeComplex1024PSO, stageInput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 세로 FFT 스테이지
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex1024PSO, transposeOutput, outputBuffer, FFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute, -1.0f, 1.0f)) {
        return false;
    }

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool GpuFFT::DispatchIFFTDualReal2D256(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTDualReal2D256");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t FFTBlockCount = 256 * planeCount;
    constexpr uint32_t TransposeTileCount = 256 / TransposeComplexTileSize;

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 세로 IFFT 스테이지 (이미 DispatchFFTDualReal2D256 에서 마지막에 세로로 끝났으므로, 세로 데이터라고 가정하고 시작)
    RHI::Buffer *verticalOutput = workBuffer;
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex256PSO, inputBuffer, verticalOutput, FFTBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined, 1.0f, 1.0f / 256.0f)) {
        return false;
    }

    // 256 x 256 transpose 해서 다음 IFFT 스테이지는 가로로 하게 된다.
    RHI::Buffer *transposeOutput = inputBuffer;
    RHI::GPUBarrier transposeBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(verticalOutput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
    dispatchBufferPSO(transposeComplex256PSO, verticalOutput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 가로 IFFT 스테이지
    return DispatchIFFTDualReal256Fused(commandList, transposeOutput, outputBuffer, verticalOutput, FFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute);
}

bool GpuFFT::DispatchIFFTDualReal2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTDualReal2D512");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t FFTBlockCount = 512 * planeCount;
    constexpr uint32_t TransposeTileCount = 512 / TransposeComplexTileSize;

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 세로 IFFT 스테이지 (이미 DispatchFFTDualReal2D512 에서 마지막에 세로로 끝났으므로, 세로 데이터라고 가정하고 시작)
    RHI::Buffer *verticalOutput = workBuffer;
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex512PSO, inputBuffer, verticalOutput, FFTBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined, 1.0f, 1.0f / 512.0f)) {
        return false;
    }

    // 512 x 512 transpose 해서 다음 IFFT 스테이지는 가로로 하게 된다.
    RHI::Buffer *transposeOutput = inputBuffer;
    RHI::GPUBarrier transposeBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(verticalOutput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
    dispatchBufferPSO(transposeComplex512PSO, verticalOutput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 가로 IFFT 스테이지
    return DispatchIFFTDualReal512Fused(commandList, transposeOutput, outputBuffer, verticalOutput, FFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute);
}

bool GpuFFT::DispatchIFFTDualReal2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTDualReal2D1024");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t FFTBlockCount = 1024 * planeCount;
    constexpr uint32_t TransposeTileCount = 1024 / TransposeComplexTileSize;

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 세로 IFFT 스테이지 (이미 DispatchFFTDualReal2D1024 에서 마지막에 세로로 끝났으므로, 세로 데이터라고 가정하고 시작)
    RHI::Buffer *verticalOutput = workBuffer;
    if (!DispatchFFTComplexFused(commandList, frameThreadData, fftComplex1024PSO, inputBuffer, verticalOutput, FFTBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined, 1.0f, 1.0f / 1024.0f)) {
        return false;
    }

    // 1024 x 1024 transpose 해서 다음 IFFT 스테이지는 가로로 하게 된다.
    RHI::Buffer *transposeOutput = inputBuffer;
    RHI::GPUBarrier transposeBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(verticalOutput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, transposeBarriers, COUNT_OF(transposeBarriers));
    dispatchBufferPSO(transposeComplex1024PSO, verticalOutput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 가로 IFFT 스테이지
    return DispatchIFFTDualReal1024Fused(commandList, transposeOutput, outputBuffer, verticalOutput, FFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute);
}

bool GpuFFT::DispatchMultiplyComplexSpectrum2D(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *lhsSpectrumBuffer, RHI::Buffer *rhsSpectrumBuffer, RHI::Buffer *outputSpectrumBuffer, uint32_t fftSize, uint32_t planeCount, float scale) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchMultiplyComplexSpectrum2D");

    assert(outputSpectrumBuffer != lhsSpectrumBuffer);
    assert(outputSpectrumBuffer != rhsSpectrumBuffer);

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
    assert(fftSize >= 2 && (fftSize & 1) == 0);

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

bool GpuFFT::DispatchFFTReal512Fused(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount, RHI::Buffer **resultBuffer) {
    assert(fftRealTwiddle512Buffer);

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, fftReal512PSO);
    RHI::renderer->SetBuffer(commandList, 0, false, inputBuffer);
    RHI::renderer->SetBuffer(commandList, 1, false, fftRealTwiddle512Buffer);
    RHI::renderer->SetBuffer(commandList, 0, true, outputBuffer);
    RHI::renderer->Dispatch(commandList, fftBlockCount, 1, 1);

    *resultBuffer = outputBuffer;
    return true;
}

bool GpuFFT::DispatchFFTReal1024Fused(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount, RHI::Buffer **resultBuffer) {
    assert(fftRealTwiddle1024Buffer);

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, fftReal1024PSO);
    RHI::renderer->SetBuffer(commandList, 0, false, inputBuffer);
    RHI::renderer->SetBuffer(commandList, 1, false, fftRealTwiddle1024Buffer);
    RHI::renderer->SetBuffer(commandList, 0, true, outputBuffer);
    RHI::renderer->Dispatch(commandList, fftBlockCount, 1, 1);

    *resultBuffer = outputBuffer;
    return true;
}

bool GpuFFT::DispatchIFFTReal512Fused(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount, RHI::GPUResourceState inputBufferState, RHI::GPUResourceState workBufferState) {
    assert(fftRealTwiddle512Buffer);

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, inputBufferState, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, ifftReal512PSO);
    RHI::renderer->SetBuffer(commandList, 0, false, inputBuffer);
    RHI::renderer->SetBuffer(commandList, 1, false, fftRealTwiddle512Buffer);
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

bool GpuFFT::DispatchIFFTReal1024Fused(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount, RHI::GPUResourceState inputBufferState, RHI::GPUResourceState workBufferState) {
    assert(fftRealTwiddle1024Buffer);

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, inputBufferState, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, ifftReal1024PSO);
    RHI::renderer->SetBuffer(commandList, 0, false, inputBuffer);
    RHI::renderer->SetBuffer(commandList, 1, false, fftRealTwiddle1024Buffer);
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

bool GpuFFT::DispatchFFTDualReal256Fused(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount, RHI::Buffer **resultBuffer) {
    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, fftDualReal256PSO);
    RHI::renderer->SetBuffer(commandList, 0, false, inputBuffer);
    RHI::renderer->SetBuffer(commandList, 0, true, outputBuffer);
    RHI::renderer->Dispatch(commandList, fftBlockCount, 1, 1);

    *resultBuffer = outputBuffer;
    return true;
}

bool GpuFFT::DispatchFFTDualReal512Fused(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount, RHI::Buffer **resultBuffer) {
    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, fftDualReal512PSO);
    RHI::renderer->SetBuffer(commandList, 0, false, inputBuffer);
    RHI::renderer->SetBuffer(commandList, 0, true, outputBuffer);
    RHI::renderer->Dispatch(commandList, fftBlockCount, 1, 1);

    *resultBuffer = outputBuffer;
    return true;
}

bool GpuFFT::DispatchFFTDualReal1024Fused(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount, RHI::Buffer **resultBuffer) {
    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, fftDualReal1024PSO);
    RHI::renderer->SetBuffer(commandList, 0, false, inputBuffer);
    RHI::renderer->SetBuffer(commandList, 0, true, outputBuffer);
    RHI::renderer->Dispatch(commandList, fftBlockCount, 1, 1);

    *resultBuffer = outputBuffer;
    return true;
}

bool GpuFFT::DispatchIFFTDualReal256Fused(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount, RHI::GPUResourceState inputBufferState, RHI::GPUResourceState workBufferState) {
    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, inputBufferState, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, ifftDualReal256PSO);
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

bool GpuFFT::DispatchIFFTDualReal512Fused(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount, RHI::GPUResourceState inputBufferState, RHI::GPUResourceState workBufferState) {
    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, inputBufferState, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, ifftDualReal512PSO);
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

bool GpuFFT::DispatchIFFTDualReal1024Fused(RHI::CommandList *commandList, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount, RHI::GPUResourceState inputBufferState, RHI::GPUResourceState workBufferState) {
    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, inputBufferState, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, ifftDualReal1024PSO);
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
