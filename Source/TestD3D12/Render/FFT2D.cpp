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
#include "FFT2D.h"
#include "RenderFrameData.h"
#include "Shaders/ShaderInterop.h"

void FFT2D::Init() {
    realFFTTwiddle512Buffer = CreateRealFFTTwiddleBuffer(512);
    realFFTTwiddle1024Buffer = CreateRealFFTTwiddleBuffer(1024);
}

void FFT2D::Shutdown() {
    if (realFFTTwiddle1024Buffer) {
        RHI::renderer->DestroyBuffer(realFFTTwiddle1024Buffer);
        realFFTTwiddle1024Buffer = nullptr;
    }

    if (realFFTTwiddle512Buffer) {
        RHI::renderer->DestroyBuffer(realFFTTwiddle512Buffer);
        realFFTTwiddle512Buffer = nullptr;
    }
}

RHI::Buffer *FFT2D::CreateRealFFTTwiddleBuffer(uint32_t fftSize) {
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
        BE_WARNLOG("FFT2D::CreateRealFFTTwiddleBuffer: failed to create twiddle buffer for FFT%u\n", fftSize);
        return nullptr;
    }

    RHI::renderer->SetName(buffer, BE1::va("FFT2D::realFFTTwiddle%iBuffer", fftSize));

    RHI::ViewID srvViewID = RHI::renderer->CreateView(buffer, RHI::ViewType::SRV, RHI::Format::Unknown, sizeof(float2), 0, bufferSize);
    if (!srvViewID.IsValid()) {
        BE_WARNLOG("FFT2D::CreateRealFFTTwiddleBuffer: failed to create twiddle SRV for FFT%u\n", fftSize);
        RHI::renderer->DestroyBuffer(buffer);
        return nullptr;
    }

    return buffer;
}

bool FFT2D::DispatchFFTComplex2D(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount, const FFTComplex2DDispatchDesc &desc) {
    if (!commandList || !frameData || !inputBuffer || !outputBuffer || !workBuffer || planeCount == 0) {
        return false;
    }
    if (desc.fftSize == 0 || desc.regularStageCount == 0 || !desc.regularStagePSO || !desc.transposePSO) {
        return false;
    }

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t totalStageCount = desc.regularStageCount + (desc.lastStagePSO ? 1 : 0);
    const uint32_t FFTBlockCount = desc.fftSize * planeCount;
    const uint32_t TransposeTileCount = desc.fftSize / TransposeComplexTileSize;
    const float sign = -1.0f;
    const float scale = 1.0f;

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto transition3 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1, const RHI::GPUBarrier &barrier2) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1, barrier2 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    auto dispatchFFTStage = [&](uint32_t stageIndex, RHI::Buffer *input, RHI::Buffer *output) -> bool {
        const bool isLastStagePSO = stageIndex >= desc.regularStageCount;
        RHI::PipelineState *pso = isLastStagePSO ? desc.lastStagePSO : desc.regularStagePSO;
        if (!pso) {
            return false;
        }

        FFTStageParams fftParams = {};
        fftParams.stageIndex = isLastStagePSO ? 0 : stageIndex;
        fftParams.sign = sign;
        fftParams.scale = scale;

        RHI::ConstantBuffer *fftConstantBuffer = frameThreadData->AllocConstant(sizeof(FFTStageParams), &fftParams);
        if (!fftConstantBuffer) {
            return false;
        }

        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->SetConstantBuffer(commandList, 0, fftConstantBuffer);
        RHI::renderer->Dispatch(commandList, FFTBlockCount, 1, 1);
        return true;
    };

    RHI::Buffer *pingBuffer0 = outputBuffer;
    RHI::Buffer *pingBuffer1 = workBuffer;

    RHI::Buffer *stageInput = inputBuffer;
    RHI::Buffer *stageOutput = pingBuffer1;

    // 가로 FFT 스테이지
    for (uint32_t stageIndex = 0; stageIndex < totalStageCount; stageIndex++) {
        stageOutput = (stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0;

        if (stageIndex == 0) {
            transition2(
                RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
                RHI::Renderer::MakeBufferBarrier(stageOutput, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));
        } else if (stageIndex == 1) {
            transition3(
                RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
                RHI::Renderer::MakeBufferBarrier(stageOutput, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess),
                RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined));
        } else {
            transition2(
                RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
                RHI::Renderer::MakeBufferBarrier(stageOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
        }

        if (!dispatchFFTStage(stageIndex, stageInput, stageOutput)) {
            return false;
        }

        stageInput = stageOutput;
    }

    // transpose
    RHI::Buffer *transposeOutput = (stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0;

    transition2(
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(desc.transposePSO, stageInput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    stageInput = transposeOutput;
    stageOutput = nullptr;

    // 세로 FFT 스테이지
    for (uint32_t stageIndex = 0; stageIndex < totalStageCount; stageIndex++) {
        const bool isLastStage = stageIndex == totalStageCount - 1;
        if (isLastStage) {
            stageOutput = outputBuffer;
            transition2(
                RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
                RHI::Renderer::MakeBufferBarrier(stageOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
        } else {
            stageOutput = (stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0;
            transition2(
                RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
                RHI::Renderer::MakeBufferBarrier(stageOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
        }

        if (!dispatchFFTStage(stageIndex, stageInput, stageOutput)) {
            return false;
        }

        if (!isLastStage) {
            stageInput = stageOutput;
        }
    }

    transition2(
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined));

    return true;
}

bool FFT2D::DispatchIFFTComplex2D(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount, const FFTComplex2DDispatchDesc &desc) {
    if (!commandList || !frameData || !inputBuffer || !outputBuffer || !workBuffer || planeCount == 0) {
        return false;
    }
    if (desc.fftSize == 0 || desc.regularStageCount == 0 || !desc.regularStagePSO || !desc.transposePSO) {
        return false;
    }

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t totalStageCount = desc.regularStageCount + (desc.lastStagePSO ? 1 : 0);
    const uint32_t FFTBlockCount = desc.fftSize * planeCount;
    const uint32_t TransposeTileCount = desc.fftSize / TransposeComplexTileSize;
    const float sign = 1.0f;
    const float scale = 1.0f / static_cast<float>(desc.fftSize);

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto transition3 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1, const RHI::GPUBarrier &barrier2) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1, barrier2 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    auto dispatchFFTStage = [&](uint32_t stageIndex, RHI::Buffer *input, RHI::Buffer *output) -> bool {
        const bool isLastStagePSO = stageIndex >= desc.regularStageCount;
        RHI::PipelineState *pso = isLastStagePSO ? desc.lastStagePSO : desc.regularStagePSO;
        if (!pso) {
            return false;
        }

        FFTStageParams fftParams = {};
        fftParams.stageIndex = isLastStagePSO ? 0 : stageIndex;
        fftParams.sign = sign;
        fftParams.scale = scale;

        RHI::ConstantBuffer *fftConstantBuffer = frameThreadData->AllocConstant(sizeof(FFTStageParams), &fftParams);
        if (!fftConstantBuffer) {
            return false;
        }

        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->SetConstantBuffer(commandList, 0, fftConstantBuffer);
        RHI::renderer->Dispatch(commandList, FFTBlockCount, 1, 1);
        return true;
    };

    RHI::Buffer *pingBuffer0 = inputBuffer;
    RHI::Buffer *pingBuffer1 = workBuffer;

    RHI::Buffer *stageInput = inputBuffer;
    RHI::Buffer *stageOutput = pingBuffer1;

    // 가로 IFFT 스테이지
    for (uint32_t stageIndex = 0; stageIndex < totalStageCount; stageIndex++) {
        stageOutput = (stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0;

        if (stageIndex == 0) {
            transition2(
                RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
                RHI::Renderer::MakeBufferBarrier(stageOutput, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));
        } else {
            transition2(
                RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
                RHI::Renderer::MakeBufferBarrier(stageOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
        }

        if (!dispatchFFTStage(stageIndex, stageInput, stageOutput)) {
            return false;
        }

        stageInput = stageOutput;
    }

    // transpose
    RHI::Buffer *transposeOutput = (stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0;

    transition2(
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(desc.transposePSO, stageInput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    stageInput = transposeOutput;
    stageOutput = nullptr;

    // 세로 IFFT 스테이지
    for (uint32_t stageIndex = 0; stageIndex < totalStageCount; stageIndex++) {
        const bool isLastStage = stageIndex == totalStageCount - 1;
        if (isLastStage) {
            stageOutput = outputBuffer;
            RHI::Buffer *unusedBuffer = (stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0;
            transition3(
                RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
                RHI::Renderer::MakeBufferBarrier(stageOutput, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess),
                RHI::Renderer::MakeBufferBarrier(unusedBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined));
        } else {
            stageOutput = (stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0;
            transition2(
                RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
                RHI::Renderer::MakeBufferBarrier(stageOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
        }

        if (!dispatchFFTStage(stageIndex, stageInput, stageOutput)) {
            return false;
        }

        if (!isLastStage) {
            stageInput = stageOutput;
        }
    }

    transition2(
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined));

    return true;
}

bool FFT2D::DispatchFFTComplex2D256(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("FFT2D::DispatchFFTComplex2D256");

    const FFTComplex2DDispatchDesc desc = { 256, 4, fftComplex256PSO, nullptr, transposeComplex256PSO };
    return DispatchFFTComplex2D(commandList, frameData, inputBuffer, outputBuffer, workBuffer, planeCount, desc);
}

bool FFT2D::DispatchIFFTComplex2D256(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("FFT2D::DispatchIFFTComplex2D256");

    const FFTComplex2DDispatchDesc desc = { 256, 4, fftComplex256PSO, nullptr, transposeComplex256PSO };
    return DispatchIFFTComplex2D(commandList, frameData, inputBuffer, outputBuffer, workBuffer, planeCount, desc);
}

bool FFT2D::DispatchFFTComplex2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("FFT2D::DispatchFFTComplex2D512");

    const FFTComplex2DDispatchDesc desc = { 512, 3, fftComplex512PSO, nullptr, transposeComplex512PSO };
    return DispatchFFTComplex2D(commandList, frameData, inputBuffer, outputBuffer, workBuffer, planeCount, desc);
}

bool FFT2D::DispatchIFFTComplex2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("FFT2D::DispatchIFFTComplex2D512");

    const FFTComplex2DDispatchDesc desc = { 512, 3, fftComplex512PSO, nullptr, transposeComplex512PSO };
    return DispatchIFFTComplex2D(commandList, frameData, inputBuffer, outputBuffer, workBuffer, planeCount, desc);
}

bool FFT2D::DispatchFFTComplex2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("FFT2D::DispatchFFTComplex2D1024");

    const FFTComplex2DDispatchDesc desc = { 1024, 3, fftComplex1024PSO, fftComplex1024LastStagePSO, transposeComplex1024PSO };
    return DispatchFFTComplex2D(commandList, frameData, inputBuffer, outputBuffer, workBuffer, planeCount, desc);
}

bool FFT2D::DispatchIFFTComplex2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("FFT2D::DispatchIFFTComplex2D1024");

    const FFTComplex2DDispatchDesc desc = { 1024, 3, fftComplex1024PSO, fftComplex1024LastStagePSO, transposeComplex1024PSO };
    return DispatchIFFTComplex2D(commandList, frameData, inputBuffer, outputBuffer, workBuffer, planeCount, desc);
}

bool FFT2D::DispatchFFTReal2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("FFT2D::DispatchFFTReal2D512");

    if (!commandList || !frameData || !inputBuffer || !outputBuffer || !workBuffer || planeCount == 0) {
        return false;
    }

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t RealFFTBlockCount = 512 * planeCount;
    const uint32_t HorizontalComplexFFTBlockCount = 512 * planeCount;
    const uint32_t VerticalComplexFFTBlockCount = 256 * planeCount;
    constexpr uint32_t TransposeTileCountX = 256 / TransposeComplexTileSize;
    constexpr uint32_t TransposeTileCountY = 512 / TransposeComplexTileSize;

    RHI::Buffer *pingBuffer0 = outputBuffer;
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto transition3 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1, const RHI::GPUBarrier &barrier2) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1, barrier2 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    auto dispatchRealBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 1, false, realFFTTwiddle512Buffer);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, 1, 1);
    };

    auto dispatchFFT256Stage = [&](uint32_t stageIndex, RHI::Buffer *input, RHI::Buffer *output, uint32_t fftBlockCount) -> bool {
        FFTStageParams fftParams = {};
        fftParams.stageIndex = stageIndex;
        fftParams.sign = -1.0f;
        fftParams.scale = 1.0f;

        RHI::ConstantBuffer *fftConstantBuffer = frameThreadData->AllocConstant(sizeof(FFTStageParams), &fftParams);
        if (!fftConstantBuffer) {
            return false;
        }

        RHI::renderer->SetPSO(commandList, fftComplex256PSO);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->SetConstantBuffer(commandList, 0, fftConstantBuffer);
        RHI::renderer->Dispatch(commandList, fftBlockCount, 1, 1);
        return true;
    };

    auto dispatchFFT512Stage = [&](uint32_t stageIndex, RHI::Buffer *input, RHI::Buffer *output, uint32_t fftBlockCount) -> bool {
        FFTStageParams fftParams = {};
        fftParams.stageIndex = stageIndex;
        fftParams.sign = -1.0f;
        fftParams.scale = 1.0f;

        RHI::ConstantBuffer *fftConstantBuffer = frameThreadData->AllocConstant(sizeof(FFTStageParams), &fftParams);
        if (!fftConstantBuffer) {
            return false;
        }

        RHI::renderer->SetPSO(commandList, fftComplex512PSO);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->SetConstantBuffer(commandList, 0, fftConstantBuffer);
        RHI::renderer->Dispatch(commandList, fftBlockCount, 1, 1);
        return true;
    };

    // 가로 FFT 스테이지 0
    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(fftReal512FirstStagePSO, inputBuffer, pingBuffer1, RealFFTBlockCount);

    // 가로 FFT256 스테이지 1
    transition3(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined));
    if (!dispatchFFT256Stage(1, pingBuffer1, pingBuffer0, HorizontalComplexFFTBlockCount)) {
        return false;
    }

    // 가로 FFT256 스테이지 2
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT256Stage(2, pingBuffer0, pingBuffer1, HorizontalComplexFFTBlockCount)) {
        return false;
    }

    // 가로 FFT 스테이지 (마지막)
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchRealBufferPSO(fftReal512LastStagePSO, pingBuffer1, pingBuffer0, RealFFTBlockCount);

    // 256 x 512 packed spectrum 을 512 x 256 으로 transpose 해서 세로 Complex FFT512 를 수행한다.
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(transposeComplex256x512PSO, pingBuffer0, pingBuffer1, TransposeTileCountX, TransposeTileCountY, planeCount);

    // 세로 FFT512 스테이지 0
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT512Stage(0, pingBuffer1, pingBuffer0, VerticalComplexFFTBlockCount)) {
        return false;
    }

    // 세로 FFT512 스테이지 1
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT512Stage(1, pingBuffer0, pingBuffer1, VerticalComplexFFTBlockCount)) {
        return false;
    }

    // 세로 FFT512 스테이지 2 (마지막)
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT512Stage(2, pingBuffer1, outputBuffer, VerticalComplexFFTBlockCount)) {
        return false;
    }

    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined));

    return true;
}

bool FFT2D::DispatchFFTReal2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("FFT2D::DispatchFFTReal2D1024");

    if (!commandList || !frameData || !inputBuffer || !outputBuffer || !workBuffer || planeCount == 0) {
        return false;
    }

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t RealFFTBlockCount = 1024 * planeCount;
    const uint32_t HorizontalComplexFFTBlockCount = 1024 * planeCount;
    const uint32_t VerticalComplexFFTBlockCount = 512 * planeCount;
    constexpr uint32_t TransposeTileCountX = 512 / TransposeComplexTileSize;
    constexpr uint32_t TransposeTileCountY = 1024 / TransposeComplexTileSize;

    RHI::Buffer *pingBuffer0 = outputBuffer;
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto transition3 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1, const RHI::GPUBarrier &barrier2) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1, barrier2 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    auto dispatchRealBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 1, false, realFFTTwiddle1024Buffer);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, 1, 1);
    };

    auto dispatchFFT512Stage = [&](uint32_t stageIndex, RHI::Buffer *input, RHI::Buffer *output, uint32_t fftBlockCount) -> bool {
        FFTStageParams fftParams = {};
        fftParams.stageIndex = stageIndex;
        fftParams.sign = -1.0f;
        fftParams.scale = 1.0f;

        RHI::ConstantBuffer *fftConstantBuffer = frameThreadData->AllocConstant(sizeof(FFTStageParams), &fftParams);
        if (!fftConstantBuffer) {
            return false;
        }

        RHI::renderer->SetPSO(commandList, fftComplex512PSO);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->SetConstantBuffer(commandList, 0, fftConstantBuffer);
        RHI::renderer->Dispatch(commandList, fftBlockCount, 1, 1);
        return true;
    };

    auto dispatchFFT1024Stage = [&](uint32_t stageIndex, RHI::Buffer *input, RHI::Buffer *output, uint32_t fftBlockCount) -> bool {
        FFTStageParams fftParams = {};
        fftParams.stageIndex = stageIndex;
        fftParams.sign = -1.0f;
        fftParams.scale = 1.0f;

        RHI::ConstantBuffer *fftConstantBuffer = frameThreadData->AllocConstant(sizeof(FFTStageParams), &fftParams);
        if (!fftConstantBuffer) {
            return false;
        }

        RHI::renderer->SetPSO(commandList, fftComplex1024PSO);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->SetConstantBuffer(commandList, 0, fftConstantBuffer);
        RHI::renderer->Dispatch(commandList, fftBlockCount, 1, 1);
        return true;
    };

    auto dispatchFFT1024LastStage = [&](RHI::Buffer *input, RHI::Buffer *output, uint32_t fftBlockCount) -> bool {
        FFTStageParams fftParams = {};
        fftParams.sign = -1.0f;
        fftParams.scale = 1.0f;

        RHI::ConstantBuffer *fftConstantBuffer = frameThreadData->AllocConstant(sizeof(FFTStageParams), &fftParams);
        if (!fftConstantBuffer) {
            return false;
        }

        RHI::renderer->SetPSO(commandList, fftComplex1024LastStagePSO);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->SetConstantBuffer(commandList, 0, fftConstantBuffer);
        RHI::renderer->Dispatch(commandList, fftBlockCount, 1, 1);
        return true;
    };

    // 가로 FFT 스테이지 0
    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(fftReal1024FirstStagePSO, inputBuffer, pingBuffer1, RealFFTBlockCount);

    // 가로 FFT512 스테이지 1
    transition3(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined));
    if (!dispatchFFT512Stage(1, pingBuffer1, pingBuffer0, HorizontalComplexFFTBlockCount)) {
        return false;
    }

    // 가로 FFT 스테이지 2 (마지막)
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchRealBufferPSO(fftReal1024LastStagePSO, pingBuffer0, pingBuffer1, RealFFTBlockCount);

    // 512 x 1024 packed spectrum 을 1024 x 512 로 transpose 해서 세로 Complex FFT1024 를 수행한다.
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(transposeComplex512x1024PSO, pingBuffer1, pingBuffer0, TransposeTileCountX, TransposeTileCountY, planeCount);

    // 세로 FFT1024 스테이지 0
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT1024Stage(0, pingBuffer0, pingBuffer1, VerticalComplexFFTBlockCount)) {
        return false;
    }

    // 세로 FFT1024 스테이지 1
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT1024Stage(1, pingBuffer1, pingBuffer0, VerticalComplexFFTBlockCount)) {
        return false;
    }

    // 세로 FFT1024 스테이지 2
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT1024Stage(2, pingBuffer0, pingBuffer1, VerticalComplexFFTBlockCount)) {
        return false;
    }

    // 세로 FFT1024 스테이지 3 (마지막)
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT1024LastStage(pingBuffer1, outputBuffer, VerticalComplexFFTBlockCount)) {
        return false;
    }

    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined));

    return true;
}

bool FFT2D::DispatchIFFTReal2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("FFT2D::DispatchIFFTReal2D512");

    if (!commandList || !frameData || !inputBuffer || !outputBuffer || !workBuffer || planeCount == 0) {
        return false;
    }

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t RealFFTBlockCount = 512 * planeCount;
    const uint32_t HorizontalComplexFFTBlockCount = 512 * planeCount;
    const uint32_t VerticalComplexFFTBlockCount = 256 * planeCount;
    constexpr uint32_t TransposeTileCountX = 512 / TransposeComplexTileSize;
    constexpr uint32_t TransposeTileCountY = 256 / TransposeComplexTileSize;

    RHI::Buffer *pingBuffer0 = inputBuffer; // Input is consumed as ping-pong storage.
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto transition3 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1, const RHI::GPUBarrier &barrier2) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1, barrier2 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    auto dispatchRealBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 1, false, realFFTTwiddle512Buffer);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, 1, 1);
    };

    auto dispatchFFT256Stage = [&](uint32_t stageIndex, RHI::Buffer *input, RHI::Buffer *output, uint32_t fftBlockCount) -> bool {
        FFTStageParams fftParams = {};
        fftParams.stageIndex = stageIndex;
        fftParams.sign = 1.0f;
        fftParams.scale = 1.0f / 256.0f;

        RHI::ConstantBuffer *fftConstantBuffer = frameThreadData->AllocConstant(sizeof(FFTStageParams), &fftParams);
        if (!fftConstantBuffer) {
            return false;
        }

        RHI::renderer->SetPSO(commandList, fftComplex256PSO);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->SetConstantBuffer(commandList, 0, fftConstantBuffer);
        RHI::renderer->Dispatch(commandList, fftBlockCount, 1, 1);
        return true;
    };

    auto dispatchFFT512Stage = [&](uint32_t stageIndex, RHI::Buffer *input, RHI::Buffer *output, uint32_t fftBlockCount) -> bool {
        FFTStageParams fftParams = {};
        fftParams.stageIndex = stageIndex;
        fftParams.sign = 1.0f;
        fftParams.scale = 1.0f / 512.0f;

        RHI::ConstantBuffer *fftConstantBuffer = frameThreadData->AllocConstant(sizeof(FFTStageParams), &fftParams);
        if (!fftConstantBuffer) {
            return false;
        }

        RHI::renderer->SetPSO(commandList, fftComplex512PSO);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->SetConstantBuffer(commandList, 0, fftConstantBuffer);
        RHI::renderer->Dispatch(commandList, fftBlockCount, 1, 1);
        return true;
    };

    // 세로 IFFT512 스테이지 0
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT512Stage(0, pingBuffer0, pingBuffer1, VerticalComplexFFTBlockCount)) {
        return false;
    }

    // 세로 IFFT512 스테이지 1
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT512Stage(1, pingBuffer1, pingBuffer0, VerticalComplexFFTBlockCount)) {
        return false;
    }

    // 세로 IFFT512 스테이지 2 (마지막)
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT512Stage(2, pingBuffer0, pingBuffer1, VerticalComplexFFTBlockCount)) {
        return false;
    }

    // 512 x 256 spectrum 을 256 x 512 로 transpose 해서 가로 Real IFFT512 를 수행한다.
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(transposeComplex512x256PSO, pingBuffer1, pingBuffer0, TransposeTileCountX, TransposeTileCountY, planeCount);

    // 가로 IFFT 스테이지 0
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchRealBufferPSO(ifftReal512FirstStagePSO, pingBuffer0, pingBuffer1, RealFFTBlockCount);

    // 가로 IFFT256 스테이지 1
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT256Stage(1, pingBuffer1, pingBuffer0, HorizontalComplexFFTBlockCount)) {
        return false;
    }

    // 가로 IFFT256 스테이지 2
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT256Stage(2, pingBuffer0, pingBuffer1, HorizontalComplexFFTBlockCount)) {
        return false;
    }

    // 가로 IFFT 스테이지 3 (마지막)
    transition3(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined));
    dispatchBufferPSO(ifftReal512LastStagePSO, pingBuffer1, outputBuffer, RealFFTBlockCount);

    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined));

    return true;
}

bool FFT2D::DispatchIFFTReal2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("FFT2D::DispatchIFFTReal2D1024");

    if (!commandList || !frameData || !inputBuffer || !outputBuffer || !workBuffer || planeCount == 0) {
        return false;
    }

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t RealFFTBlockCount = 1024 * planeCount;
    const uint32_t HorizontalComplexFFTBlockCount = 1024 * planeCount;
    const uint32_t VerticalComplexFFTBlockCount = 512 * planeCount;
    constexpr uint32_t TransposeTileCountX = 1024 / TransposeComplexTileSize;
    constexpr uint32_t TransposeTileCountY = 512 / TransposeComplexTileSize;

    RHI::Buffer *pingBuffer0 = inputBuffer; // Input is consumed as ping-pong storage.
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto transition3 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1, const RHI::GPUBarrier &barrier2) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1, barrier2 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    auto dispatchRealBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 1, false, realFFTTwiddle1024Buffer);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, 1, 1);
    };

    auto dispatchFFT512Stage = [&](uint32_t stageIndex, RHI::Buffer *input, RHI::Buffer *output, uint32_t fftBlockCount) -> bool {
        FFTStageParams fftParams = {};
        fftParams.stageIndex = stageIndex;
        fftParams.sign = 1.0f;
        fftParams.scale = 1.0f / 512.0f;

        RHI::ConstantBuffer *fftConstantBuffer = frameThreadData->AllocConstant(sizeof(FFTStageParams), &fftParams);
        if (!fftConstantBuffer) {
            return false;
        }

        RHI::renderer->SetPSO(commandList, fftComplex512PSO);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->SetConstantBuffer(commandList, 0, fftConstantBuffer);
        RHI::renderer->Dispatch(commandList, fftBlockCount, 1, 1);
        return true;
    };

    auto dispatchFFT1024Stage = [&](uint32_t stageIndex, RHI::Buffer *input, RHI::Buffer *output, uint32_t fftBlockCount) -> bool {
        FFTStageParams fftParams = {};
        fftParams.stageIndex = stageIndex;
        fftParams.sign = 1.0f;
        fftParams.scale = 1.0f / 1024.0f;

        RHI::ConstantBuffer *fftConstantBuffer = frameThreadData->AllocConstant(sizeof(FFTStageParams), &fftParams);
        if (!fftConstantBuffer) {
            return false;
        }

        RHI::renderer->SetPSO(commandList, fftComplex1024PSO);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->SetConstantBuffer(commandList, 0, fftConstantBuffer);
        RHI::renderer->Dispatch(commandList, fftBlockCount, 1, 1);
        return true;
    };

    auto dispatchFFT1024LastStage = [&](RHI::Buffer *input, RHI::Buffer *output, uint32_t fftBlockCount) -> bool {
        FFTStageParams fftParams = {};
        fftParams.sign = 1.0f;
        fftParams.scale = 1.0f / 1024.0f;

        RHI::ConstantBuffer *fftConstantBuffer = frameThreadData->AllocConstant(sizeof(FFTStageParams), &fftParams);
        if (!fftConstantBuffer) {
            return false;
        }

        RHI::renderer->SetPSO(commandList, fftComplex1024LastStagePSO);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->SetConstantBuffer(commandList, 0, fftConstantBuffer);
        RHI::renderer->Dispatch(commandList, fftBlockCount, 1, 1);
        return true;
    };

    // 세로 IFFT1024 스테이지 0
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT1024Stage(0, pingBuffer0, pingBuffer1, VerticalComplexFFTBlockCount)) {
        return false;
    }

    // 세로 IFFT1024 스테이지 1
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT1024Stage(1, pingBuffer1, pingBuffer0, VerticalComplexFFTBlockCount)) {
        return false;
    }

    // 세로 IFFT1024 스테이지 2
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT1024Stage(2, pingBuffer0, pingBuffer1, VerticalComplexFFTBlockCount)) {
        return false;
    }

    // 세로 IFFT1024 스테이지 3 (마지막)
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT1024LastStage(pingBuffer1, pingBuffer0, VerticalComplexFFTBlockCount)) {
        return false;
    }

    // 1024 x 512 spectrum 을 512 x 1024 로 transpose 해서 가로 Real IFFT1024 를 수행한다.
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(transposeComplex1024x512PSO, pingBuffer0, pingBuffer1, TransposeTileCountX, TransposeTileCountY, planeCount);

    // 가로 IFFT 스테이지 0
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchRealBufferPSO(ifftReal1024FirstStagePSO, pingBuffer1, pingBuffer0, RealFFTBlockCount);

    // 가로 IFFT512 스테이지 1
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT512Stage(1, pingBuffer0, pingBuffer1, HorizontalComplexFFTBlockCount)) {
        return false;
    }

    // 가로 IFFT 스테이지 2 (마지막)
    transition3(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined));
    dispatchBufferPSO(ifftReal1024LastStagePSO, pingBuffer1, outputBuffer, RealFFTBlockCount);

    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined));

    return true;
}

bool FFT2D::DispatchFFTDualReal2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("FFT2D::DispatchFFTDualReal2D512");

    if (!commandList || !frameData || !inputBuffer || !outputBuffer || !workBuffer || planeCount == 0) {
        return false;
    }

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t FFTBlockCount = 512 * planeCount;
    constexpr uint32_t TransposeTileCount = 512 / TransposeComplexTileSize;

    RHI::Buffer *pingBuffer0 = outputBuffer; // Used as ping-pong storage before the final write.
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto transition3 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1, const RHI::GPUBarrier &barrier2) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1, barrier2 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    auto dispatchFFT512Stage = [&](uint32_t stageIndex, RHI::Buffer *input, RHI::Buffer *output) -> bool {
        FFTStageParams fftParams = {};
        fftParams.stageIndex = stageIndex;
        fftParams.sign = -1.0f;
        fftParams.scale = 1.0f;

        RHI::ConstantBuffer *fftConstantBuffer = frameThreadData->AllocConstant(sizeof(FFTStageParams), &fftParams);
        if (!fftConstantBuffer) {
            return false;
        }

        RHI::renderer->SetPSO(commandList, fftComplex512PSO);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->SetConstantBuffer(commandList, 0, fftConstantBuffer);
        RHI::renderer->Dispatch(commandList, FFTBlockCount, 1, 1);
        return true;
    };

    // 가로 FFT 스테이지 0
    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(fftDualReal512FirstStagePSO, inputBuffer, pingBuffer0, FFTBlockCount);

    // 가로 FFT 스테이지 1
    transition3(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined));
    if (!dispatchFFT512Stage(1, pingBuffer0, pingBuffer1)) {
        return false;
    }

    // 가로 FFT 스테이지 2 (마지막)
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(fftDualReal512LastStagePSO, pingBuffer1, pingBuffer0, FFTBlockCount);

    // 512 x 512 transpose 해서 다음 FFT 스테이지는 세로로 하게 된다.
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(transposeComplex512PSO, pingBuffer0, pingBuffer1, TransposeTileCount, TransposeTileCount, planeCount);

    // 세로 FFT 스테이지 0
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT512Stage(0, pingBuffer1, pingBuffer0)) {
        return false;
    }

    // 세로 FFT 스테이지 1
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT512Stage(1, pingBuffer0, pingBuffer1)) {
        return false;
    }

    // 세로 FFT 스테이지 2 (마지막)
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT512Stage(2, pingBuffer1, outputBuffer)) {
        return false;
    }

    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined));

    return true;
}

bool FFT2D::DispatchFFTDualReal2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("FFT2D::DispatchFFTDualReal2D1024");

    if (!commandList || !frameData || !inputBuffer || !outputBuffer || !workBuffer || planeCount == 0) {
        return false;
    }

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t FFTBlockCount = 1024 * planeCount;
    constexpr uint32_t TransposeTileCount = 1024 / TransposeComplexTileSize;

    RHI::Buffer *pingBuffer0 = outputBuffer; // Used as ping-pong storage before the final write.
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto transition3 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1, const RHI::GPUBarrier &barrier2) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1, barrier2 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    auto dispatchFFT1024Stage = [&](uint32_t stageIndex, RHI::Buffer *input, RHI::Buffer *output) -> bool {
        FFTStageParams fftParams = {};
        fftParams.stageIndex = stageIndex;
        fftParams.sign = -1.0f;
        fftParams.scale = 1.0f;

        RHI::ConstantBuffer *fftConstantBuffer = frameThreadData->AllocConstant(sizeof(FFTStageParams), &fftParams);
        if (!fftConstantBuffer) {
            return false;
        }

        RHI::renderer->SetPSO(commandList, fftComplex1024PSO);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->SetConstantBuffer(commandList, 0, fftConstantBuffer);
        RHI::renderer->Dispatch(commandList, FFTBlockCount, 1, 1);
        return true;
    };

    auto dispatchFFT1024LastStage = [&](RHI::Buffer *input, RHI::Buffer *output) -> bool {
        FFTStageParams fftParams = {};
        fftParams.sign = -1.0f;
        fftParams.scale = 1.0f;

        RHI::ConstantBuffer *fftConstantBuffer = frameThreadData->AllocConstant(sizeof(FFTStageParams), &fftParams);
        if (!fftConstantBuffer) {
            return false;
        }

        RHI::renderer->SetPSO(commandList, fftComplex1024LastStagePSO);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->SetConstantBuffer(commandList, 0, fftConstantBuffer);
        RHI::renderer->Dispatch(commandList, FFTBlockCount, 1, 1);
        return true;
    };

    // 가로 FFT 스테이지 0
    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(fftDualReal1024FirstStagePSO, inputBuffer, pingBuffer0, FFTBlockCount);

    // 가로 FFT 스테이지 1
    transition3(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined));
    if (!dispatchFFT1024Stage(1, pingBuffer0, pingBuffer1)) {
        return false;
    }

    // 가로 FFT 스테이지 2
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT1024Stage(2, pingBuffer1, pingBuffer0)) {
        return false;
    }

    // 가로 FFT 스테이지 3 (마지막)
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(fftDualReal1024LastStagePSO, pingBuffer0, pingBuffer1, FFTBlockCount);

    // 1024 x 1024 transpose 해서 다음 FFT 스테이지는 세로로 하게 된다.
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(transposeComplex1024PSO, pingBuffer1, pingBuffer0, TransposeTileCount, TransposeTileCount, planeCount);

    // 세로 FFT 스테이지 0
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT1024Stage(0, pingBuffer0, pingBuffer1)) {
        return false;
    }

    // 세로 FFT 스테이지 1
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT1024Stage(1, pingBuffer1, pingBuffer0)) {
        return false;
    }

    // 세로 FFT 스테이지 2
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT1024Stage(2, pingBuffer0, pingBuffer1)) {
        return false;
    }

    // 세로 FFT 스테이지 3 (마지막)
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT1024LastStage(pingBuffer1, outputBuffer)) {
        return false;
    }

    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined));

    return true;
}

bool FFT2D::DispatchIFFTDualReal2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("FFT2D::DispatchIFFTDualReal2D512");

    if (!commandList || !frameData || !inputBuffer || !outputBuffer || !workBuffer || planeCount == 0) {
        return false;
    }

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t FFTBlockCount = 512 * planeCount;
    constexpr uint32_t TransposeTileCount = 512 / TransposeComplexTileSize;

    RHI::Buffer *pingBuffer0 = inputBuffer; // Input is consumed as ping-pong storage.
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto transition3 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1, const RHI::GPUBarrier &barrier2) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1, barrier2 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    auto dispatchFFT512Stage = [&](uint32_t stageIndex, RHI::Buffer *input, RHI::Buffer *output) -> bool {
        FFTStageParams fftParams = {};
        fftParams.stageIndex = stageIndex;
        fftParams.sign = 1.0f;
        fftParams.scale = 1.0f / 512.0f;

        RHI::ConstantBuffer *fftConstantBuffer = frameThreadData->AllocConstant(sizeof(FFTStageParams), &fftParams);
        if (!fftConstantBuffer) {
            return false;
        }

        RHI::renderer->SetPSO(commandList, fftComplex512PSO);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->SetConstantBuffer(commandList, 0, fftConstantBuffer);
        RHI::renderer->Dispatch(commandList, FFTBlockCount, 1, 1);
        return true;
    };

    // 세로 IFFT 스테이지 0 (이미 DispatchFFTDualReal2D512 에서 마지막에 세로로 끝났으므로, 세로 데이터라고 가정하고 시작)
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT512Stage(0, pingBuffer0, pingBuffer1)) {
        return false;
    }

    // 세로 IFFT 스테이지 1
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT512Stage(1, pingBuffer1, pingBuffer0)) {
        return false;
    }

    // 세로 IFFT 스테이지 2
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT512Stage(2, pingBuffer0, pingBuffer1)) {
        return false;
    }

    // 512 x 512 transpose 해서 다음 IFFT 스테이지는 가로로 하게 된다.
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(transposeComplex512PSO, pingBuffer1, pingBuffer0, TransposeTileCount, TransposeTileCount, planeCount);

    // 가로 IFFT 스테이지 0
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(ifftDualReal512FirstStagePSO, pingBuffer0, pingBuffer1, FFTBlockCount);

    // 가로 IFFT 스테이지 1
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT512Stage(1, pingBuffer1, pingBuffer0)) {
        return false;
    }

    // 가로 IFFT 스테이지 2 (마지막)
    transition3(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined));
    dispatchBufferPSO(ifftDualReal512LastStagePSO, pingBuffer0, outputBuffer, FFTBlockCount);

    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined));

    return true;
}

bool FFT2D::DispatchIFFTDualReal2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("FFT2D::DispatchIFFTDualReal2D1024");

    if (!commandList || !frameData || !inputBuffer || !outputBuffer || !workBuffer || planeCount == 0) {
        return false;
    }

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t FFTBlockCount = 1024 * planeCount;
    constexpr uint32_t TransposeTileCount = 1024 / TransposeComplexTileSize;

    RHI::Buffer *pingBuffer0 = inputBuffer; // Input is consumed as ping-pong storage.
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto transition3 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1, const RHI::GPUBarrier &barrier2) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1, barrier2 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    auto dispatchFFT1024Stage = [&](uint32_t stageIndex, RHI::Buffer *input, RHI::Buffer *output) -> bool {
        FFTStageParams fftParams = {};
        fftParams.stageIndex = stageIndex;
        fftParams.sign = 1.0f;
        fftParams.scale = 1.0f;

        RHI::ConstantBuffer *fftConstantBuffer = frameThreadData->AllocConstant(sizeof(FFTStageParams), &fftParams);
        if (!fftConstantBuffer) {
            return false;
        }

        RHI::renderer->SetPSO(commandList, fftComplex1024PSO);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->SetConstantBuffer(commandList, 0, fftConstantBuffer);
        RHI::renderer->Dispatch(commandList, FFTBlockCount, 1, 1);
        return true;
    };

    auto dispatchFFT1024LastStage = [&](RHI::Buffer *input, RHI::Buffer *output) -> bool {
        FFTStageParams fftParams = {};
        fftParams.sign = 1.0f;
        fftParams.scale = 1.0f / 1024.0f;

        RHI::ConstantBuffer *fftConstantBuffer = frameThreadData->AllocConstant(sizeof(FFTStageParams), &fftParams);
        if (!fftConstantBuffer) {
            return false;
        }

        RHI::renderer->SetPSO(commandList, fftComplex1024LastStagePSO);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->SetConstantBuffer(commandList, 0, fftConstantBuffer);
        RHI::renderer->Dispatch(commandList, FFTBlockCount, 1, 1);
        return true;
    };

    // 세로 IFFT 스테이지 0 (이미 DispatchFFTDualReal2D1024 에서 마지막에 세로로 끝났으므로, 세로 데이터라고 가정하고 시작)
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT1024Stage(0, pingBuffer0, pingBuffer1)) {
        return false;
    }

    // 세로 IFFT 스테이지 1
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT1024Stage(1, pingBuffer1, pingBuffer0)) {
        return false;
    }

    // 세로 IFFT 스테이지 2
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT1024Stage(2, pingBuffer0, pingBuffer1)) {
        return false;
    }

    // 세로 IFFT 스테이지 3 (마지막)
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT1024LastStage(pingBuffer1, pingBuffer0)) {
        return false;
    }

    // 1024 x 1024 transpose 해서 다음 IFFT 스테이지는 가로로 하게 된다.
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(transposeComplex1024PSO, pingBuffer0, pingBuffer1, TransposeTileCount, TransposeTileCount, planeCount);

    // 가로 IFFT 스테이지 0
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(ifftDualReal1024FirstStagePSO, pingBuffer1, pingBuffer0, FFTBlockCount);

    // 가로 IFFT 스테이지 1
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT1024Stage(1, pingBuffer0, pingBuffer1)) {
        return false;
    }

    // 가로 IFFT 스테이지 2
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!dispatchFFT1024Stage(2, pingBuffer1, pingBuffer0)) {
        return false;
    }

    // 가로 IFFT 스테이지 3 (마지막)
    transition3(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined));
    dispatchBufferPSO(ifftDualReal1024LastStagePSO, pingBuffer0, outputBuffer, FFTBlockCount);

    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined));

    return true;
}

bool FFT2D::DispatchMultiplyComplexSpectrum2D(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *lhsSpectrumBuffer, RHI::Buffer *rhsSpectrumBuffer, RHI::Buffer *outputSpectrumBuffer, uint32_t fftSize, uint32_t planeCount, float scale) {
    PROFILER_CPU_SCOPED_EVENT("FFT2D::DispatchMultiplyComplexSpectrum2D");

    if (!commandList ||
        !frameData ||
        !multiplyComplexSpectrum2DPSO ||
        !lhsSpectrumBuffer ||
        !rhsSpectrumBuffer ||
        !outputSpectrumBuffer ||
        fftSize == 0 ||
        planeCount == 0) {
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

bool FFT2D::DispatchMultiplyPackedRealSpectrum2D(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *lhsSpectrumBuffer, RHI::Buffer *rhsSpectrumBuffer, RHI::Buffer *outputSpectrumBuffer, uint32_t fftSize, uint32_t planeCount, float scale) {
    PROFILER_CPU_SCOPED_EVENT("FFT2D::DispatchMultiplyPackedRealSpectrum2D");

    if (!commandList ||
        !frameData ||
        !multiplyPackedRealSpectrum2DPSO ||
        !lhsSpectrumBuffer ||
        !rhsSpectrumBuffer ||
        !outputSpectrumBuffer ||
        fftSize < 2 ||
        planeCount == 0) {
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

bool FFT2D::DispatchMultiplyPackedDualRealSpectrum2D(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *lhsSpectrumBuffer, RHI::Buffer *rhsSpectrumBuffer, RHI::Buffer *outputSpectrumBuffer, uint32_t fftSize, uint32_t planeCount, float scale) {
    PROFILER_CPU_SCOPED_EVENT("FFT2D::DispatchMultiplyPackedDualRealSpectrum2D");

    if (!commandList ||
        !frameData ||
        !multiplyPackedDualRealSpectrum2DPSO ||
        !lhsSpectrumBuffer ||
        !rhsSpectrumBuffer ||
        !outputSpectrumBuffer ||
        fftSize == 0 ||
        planeCount == 0) {
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
