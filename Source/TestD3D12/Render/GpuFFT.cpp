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
#include "Shaders/ShaderInterop.h"

void GpuFFT::Init() {
    realFFTTwiddle512Buffer = CreateRealFFTTwiddleBuffer(512);
    realFFTTwiddle1024Buffer = CreateRealFFTTwiddleBuffer(1024);
}

void GpuFFT::Shutdown() {
    if (realFFTTwiddle1024Buffer) {
        RHI::renderer->DestroyBuffer(realFFTTwiddle1024Buffer);
        realFFTTwiddle1024Buffer = nullptr;
    }

    if (realFFTTwiddle512Buffer) {
        RHI::renderer->DestroyBuffer(realFFTTwiddle512Buffer);
        realFFTTwiddle512Buffer = nullptr;
    }
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

bool GpuFFT::DispatchFFTComplexStage(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::PipelineState *pso, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t fftBlockCount, uint32_t stageIndex, float sign, float scale) {
    FFTStageParams fftParams = {};
    fftParams.stageIndex = stageIndex;
    fftParams.sign = sign;
    fftParams.scale = scale;

    RHI::ConstantBuffer *fftConstantBuffer = frameThreadData->AllocConstant(sizeof(FFTStageParams), &fftParams);
    if (!fftConstantBuffer) {
        return false;
    }

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetBuffer(commandList, 0, false, inputBuffer);
    RHI::renderer->SetBuffer(commandList, 0, true, outputBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 0, fftConstantBuffer);
    RHI::renderer->Dispatch(commandList, fftBlockCount, 1, 1);
    return true;
}

bool GpuFFT::DispatchFFTComplex256(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *pingBuffer0, RHI::Buffer *pingBuffer1, uint32_t fftBlockCount, float sign, float scale, RHI::PipelineState *regularStagePSO, RHI::Buffer **resultBuffer) {
    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto transition3 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1, const RHI::GPUBarrier &barrier2) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1, barrier2 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, regularStagePSO, inputBuffer, pingBuffer0, fftBlockCount, 0, sign, scale)) {
        return false;
    }

    transition3(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, regularStagePSO, pingBuffer0, pingBuffer1, fftBlockCount, 1, sign, scale)) {
        return false;
    }

    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, regularStagePSO, pingBuffer1, pingBuffer0, fftBlockCount, 2, sign, scale)) {
        return false;
    }

    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, regularStagePSO, pingBuffer0, pingBuffer1, fftBlockCount, 3, sign, scale)) {
        return false;
    }

    *resultBuffer = pingBuffer1;
    return true;
}

bool GpuFFT::DispatchFFTComplex512(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *pingBuffer0, RHI::Buffer *pingBuffer1, uint32_t fftBlockCount, float sign, float scale, RHI::PipelineState *regularStagePSO, RHI::Buffer **resultBuffer) {
    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto transition3 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1, const RHI::GPUBarrier &barrier2) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1, barrier2 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, regularStagePSO, inputBuffer, pingBuffer0, fftBlockCount, 0, sign, scale)) {
        return false;
    }

    transition3(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, regularStagePSO, pingBuffer0, pingBuffer1, fftBlockCount, 1, sign, scale)) {
        return false;
    }

    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, regularStagePSO, pingBuffer1, pingBuffer0, fftBlockCount, 2, sign, scale)) {
        return false;
    }

    *resultBuffer = pingBuffer0;
    return true;
}

bool GpuFFT::DispatchFFTComplex1024(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *pingBuffer0, RHI::Buffer *pingBuffer1, uint32_t fftBlockCount, float sign, float scale, RHI::PipelineState *regularStagePSO, RHI::PipelineState *lastStagePSO, RHI::Buffer **resultBuffer) {
    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto transition3 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1, const RHI::GPUBarrier &barrier2) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1, barrier2 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, regularStagePSO, inputBuffer, pingBuffer0, fftBlockCount, 0, sign, scale)) {
        return false;
    }

    transition3(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, regularStagePSO, pingBuffer0, pingBuffer1, fftBlockCount, 1, sign, scale)) {
        return false;
    }

    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, regularStagePSO, pingBuffer1, pingBuffer0, fftBlockCount, 2, sign, scale)) {
        return false;
    }

    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, lastStagePSO, pingBuffer0, pingBuffer1, fftBlockCount, 0, sign, scale)) {
        return false;
    }

    *resultBuffer = pingBuffer1;
    return true;
}

bool GpuFFT::DispatchFFTComplexToOutput(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *pingBuffer0, RHI::Buffer *pingBuffer1, uint32_t fftBlockCount, uint32_t regularStageCount, RHI::PipelineState *regularStagePSO, RHI::PipelineState *lastStagePSO, float sign, float scale) {
    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    const uint32_t totalStageCount = regularStageCount + (lastStagePSO ? 1 : 0);
    RHI::Buffer *stageInput = inputBuffer;

    for (uint32_t stageIndex = 0; stageIndex < totalStageCount; stageIndex++) {
        const bool isLastStage = stageIndex == totalStageCount - 1;
        const bool isLastStagePSO = stageIndex >= regularStageCount;
        RHI::Buffer *stageOutput = isLastStage ? outputBuffer : ((stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0);
        RHI::PipelineState *pso = isLastStagePSO ? lastStagePSO : regularStagePSO;

        transition2(
            RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
            RHI::Renderer::MakeBufferBarrier(stageOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
        if (!DispatchFFTComplexStage(commandList, frameThreadData, pso, stageInput, stageOutput, fftBlockCount, isLastStagePSO ? 0 : stageIndex, sign, scale)) {
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

bool GpuFFT::DispatchIFFTComplex256(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount, float sign, float scale, RHI::PipelineState *regularStagePSO, RHI::Buffer **resultBuffer) {
    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, regularStagePSO, inputBuffer, workBuffer, fftBlockCount, 0, sign, scale)) {
        return false;
    }

    transition2(
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, regularStagePSO, workBuffer, inputBuffer, fftBlockCount, 1, sign, scale)) {
        return false;
    }

    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, regularStagePSO, inputBuffer, workBuffer, fftBlockCount, 2, sign, scale)) {
        return false;
    }

    transition2(
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, regularStagePSO, workBuffer, inputBuffer, fftBlockCount, 3, sign, scale)) {
        return false;
    }

    *resultBuffer = inputBuffer;
    return true;
}

bool GpuFFT::DispatchIFFTComplex512(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount, float sign, float scale, RHI::PipelineState *regularStagePSO, RHI::Buffer **resultBuffer) {
    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, regularStagePSO, inputBuffer, workBuffer, fftBlockCount, 0, sign, scale)) {
        return false;
    }

    transition2(
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, regularStagePSO, workBuffer, inputBuffer, fftBlockCount, 1, sign, scale)) {
        return false;
    }

    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, regularStagePSO, inputBuffer, workBuffer, fftBlockCount, 2, sign, scale)) {
        return false;
    }

    *resultBuffer = workBuffer;
    return true;
}

bool GpuFFT::DispatchIFFTComplex1024(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount, float sign, float scale, RHI::PipelineState *regularStagePSO, RHI::PipelineState *lastStagePSO, RHI::Buffer **resultBuffer) {
    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, regularStagePSO, inputBuffer, workBuffer, fftBlockCount, 0, sign, scale)) {
        return false;
    }

    transition2(
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, regularStagePSO, workBuffer, inputBuffer, fftBlockCount, 1, sign, scale)) {
        return false;
    }

    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, regularStagePSO, inputBuffer, workBuffer, fftBlockCount, 2, sign, scale)) {
        return false;
    }

    transition2(
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, lastStagePSO, workBuffer, inputBuffer, fftBlockCount, 0, sign, scale)) {
        return false;
    }

    *resultBuffer = inputBuffer;
    return true;
}

bool GpuFFT::DispatchIFFTComplexToOutput(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *pingBuffer0, RHI::Buffer *pingBuffer1, uint32_t fftBlockCount, uint32_t regularStageCount, RHI::PipelineState *regularStagePSO, RHI::PipelineState *lastStagePSO, float sign, float scale) {
    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto transition3 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1, const RHI::GPUBarrier &barrier2) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1, barrier2 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    const uint32_t totalStageCount = regularStageCount + (lastStagePSO ? 1 : 0);
    RHI::Buffer *stageInput = inputBuffer;

    for (uint32_t stageIndex = 0; stageIndex < totalStageCount; stageIndex++) {
        const bool isLastStage = stageIndex == totalStageCount - 1;
        const bool isLastStagePSO = stageIndex >= regularStageCount;
        RHI::Buffer *stageOutput = isLastStage ? outputBuffer : ((stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0);
        RHI::PipelineState *pso = isLastStagePSO ? lastStagePSO : regularStagePSO;

        if (isLastStage) {
            RHI::Buffer *unusedBuffer = (stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0;
            transition3(
                RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
                RHI::Renderer::MakeBufferBarrier(stageOutput, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess),
                RHI::Renderer::MakeBufferBarrier(unusedBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined));
        } else {
            transition2(
                RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
                RHI::Renderer::MakeBufferBarrier(stageOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
        }

        if (!DispatchFFTComplexStage(commandList, frameThreadData, pso, stageInput, stageOutput, fftBlockCount, isLastStagePSO ? 0 : stageIndex, sign, scale)) {
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

bool GpuFFT::DispatchFFTComplex1D256(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTComplex1D256");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    RHI::Buffer *resultBuffer = nullptr;
    if (!DispatchFFTComplex256(commandList, frameThreadData, inputBuffer, workBuffer, outputBuffer, fftBlockCount, -1.0f, 1.0f, fftComplex256PSO, &resultBuffer)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchFFTComplex1D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTComplex1D512");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    RHI::Buffer *resultBuffer = nullptr;
    if (!DispatchFFTComplex512(commandList, frameThreadData, inputBuffer, outputBuffer, workBuffer, fftBlockCount, -1.0f, 1.0f, fftComplex512PSO, &resultBuffer)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchFFTComplex1D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTComplex1D1024");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    RHI::Buffer *resultBuffer = nullptr;
    if (!DispatchFFTComplex1024(commandList, frameThreadData, inputBuffer, workBuffer, outputBuffer, fftBlockCount, -1.0f, 1.0f, fftComplex1024PSO, fftComplex1024LastStagePSO, &resultBuffer)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchIFFTComplex1D256(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTComplex1D256");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    RHI::Buffer *resultBuffer = nullptr;
    if (!DispatchFFTComplex256(commandList, frameThreadData, inputBuffer, workBuffer, outputBuffer, fftBlockCount, 1.0f, 1.0f / 256.0f, fftComplex256PSO, &resultBuffer)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchIFFTComplex1D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTComplex1D512");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    RHI::Buffer *resultBuffer = nullptr;
    if (!DispatchFFTComplex512(commandList, frameThreadData, inputBuffer, outputBuffer, workBuffer, fftBlockCount, 1.0f, 1.0f / 512.0f, fftComplex512PSO, &resultBuffer)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchIFFTComplex1D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTComplex1D1024");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    RHI::Buffer *resultBuffer = nullptr;
    if (!DispatchFFTComplex1024(commandList, frameThreadData, inputBuffer, workBuffer, outputBuffer, fftBlockCount, 1.0f, 1.0f / 1024.0f, fftComplex1024PSO, fftComplex1024LastStagePSO, &resultBuffer)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchFFTReal1D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTReal1D512");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    RHI::Buffer *resultBuffer = nullptr;
    if (!DispatchFFTReal512(commandList, frameThreadData, inputBuffer, outputBuffer, workBuffer, fftBlockCount, &resultBuffer)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchFFTReal1D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTReal1D1024");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    RHI::Buffer *resultBuffer = nullptr;
    if (!DispatchFFTReal1024(commandList, frameThreadData, inputBuffer, workBuffer, outputBuffer, fftBlockCount, &resultBuffer)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchIFFTReal1D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTReal1D512");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    return DispatchIFFTReal512(commandList, frameThreadData, inputBuffer, outputBuffer, workBuffer, fftBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined);
}

bool GpuFFT::DispatchIFFTReal1D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTReal1D1024");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    return DispatchIFFTReal1024(commandList, frameThreadData, inputBuffer, outputBuffer, workBuffer, fftBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined);
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

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 가로 FFT 스테이지
    RHI::Buffer *stageInput = nullptr;
    if (!DispatchFFTComplex256(commandList, frameThreadData, inputBuffer, pingBuffer0, pingBuffer1, FFTBlockCount, sign, scale, fftComplex256PSO, &stageInput)) {
        return false;
    }

    // transpose
    RHI::Buffer *transposeOutput = (stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0;
    transition2(
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(transposeComplex256PSO, stageInput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 세로 FFT 스테이지
    stageInput = transposeOutput;
    return DispatchFFTComplexToOutput(commandList, frameThreadData, stageInput, outputBuffer, pingBuffer0, pingBuffer1, FFTBlockCount, 4, fftComplex256PSO, nullptr, sign, scale);
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

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 가로 FFT 스테이지
    RHI::Buffer *stageInput = nullptr;
    if (!DispatchFFTComplex512(commandList, frameThreadData, inputBuffer, pingBuffer0, pingBuffer1, FFTBlockCount, sign, scale, fftComplex512PSO, &stageInput)) {
        return false;
    }

    // transpose
    RHI::Buffer *transposeOutput = (stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0;
    transition2(
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(transposeComplex512PSO, stageInput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 세로 FFT 스테이지
    stageInput = transposeOutput;
    return DispatchFFTComplexToOutput(commandList, frameThreadData, stageInput, outputBuffer, pingBuffer0, pingBuffer1, FFTBlockCount, 3, fftComplex512PSO, nullptr, sign, scale);
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

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 가로 FFT 스테이지
    RHI::Buffer *stageInput = nullptr;
    if (!DispatchFFTComplex1024(commandList, frameThreadData, inputBuffer, pingBuffer0, pingBuffer1, FFTBlockCount, sign, scale, fftComplex1024PSO, fftComplex1024LastStagePSO, &stageInput)) {
        return false;
    }

    // transpose
    RHI::Buffer *transposeOutput = (stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0;
    transition2(
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(transposeComplex1024PSO, stageInput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 세로 FFT 스테이지
    stageInput = transposeOutput;
    return DispatchFFTComplexToOutput(commandList, frameThreadData, stageInput, outputBuffer, pingBuffer0, pingBuffer1, FFTBlockCount, 3, fftComplex1024PSO, fftComplex1024LastStagePSO, sign, scale);
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

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 가로 IFFT 스테이지
    RHI::Buffer *stageInput = nullptr;
    if (!DispatchIFFTComplex256(commandList, frameThreadData, inputBuffer, workBuffer, FFTBlockCount, sign, scale, fftComplex256PSO, &stageInput)) {
        return false;
    }

    // transpose
    RHI::Buffer *transposeOutput = (stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0;
    transition2(
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(transposeComplex256PSO, stageInput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 세로 IFFT 스테이지
    stageInput = transposeOutput;
    return DispatchIFFTComplexToOutput(commandList, frameThreadData, stageInput, outputBuffer, pingBuffer0, pingBuffer1, FFTBlockCount, 4, fftComplex256PSO, nullptr, sign, scale);
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

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 가로 IFFT 스테이지
    RHI::Buffer *stageInput = nullptr;
    if (!DispatchIFFTComplex512(commandList, frameThreadData, inputBuffer, workBuffer, FFTBlockCount, sign, scale, fftComplex512PSO, &stageInput)) {
        return false;
    }

    // transpose
    RHI::Buffer *transposeOutput = (stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0;
    transition2(
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(transposeComplex512PSO, stageInput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 세로 IFFT 스테이지
    stageInput = transposeOutput;
    return DispatchIFFTComplexToOutput(commandList, frameThreadData, stageInput, outputBuffer, pingBuffer0, pingBuffer1, FFTBlockCount, 3, fftComplex512PSO, nullptr, sign, scale);
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

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 가로 IFFT 스테이지
    RHI::Buffer *stageInput = nullptr;
    if (!DispatchIFFTComplex1024(commandList, frameThreadData, inputBuffer, workBuffer, FFTBlockCount, sign, scale, fftComplex1024PSO, fftComplex1024LastStagePSO, &stageInput)) {
        return false;
    }

    // transpose
    RHI::Buffer *transposeOutput = (stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0;
    transition2(
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(transposeComplex1024PSO, stageInput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 세로 IFFT 스테이지
    stageInput = transposeOutput;
    return DispatchIFFTComplexToOutput(commandList, frameThreadData, stageInput, outputBuffer, pingBuffer0, pingBuffer1, FFTBlockCount, 3, fftComplex1024PSO, fftComplex1024LastStagePSO, sign, scale);
}

bool GpuFFT::DispatchFFTReal2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTReal2D512");
    assert(realFFTTwiddle512Buffer);

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t RealFFTBlockCount = 512 * planeCount;
    const uint32_t VerticalComplexFFTBlockCount = 256 * planeCount;
    constexpr uint32_t TransposeTileCountX = 256 / TransposeComplexTileSize;
    constexpr uint32_t TransposeTileCountY = 512 / TransposeComplexTileSize;

    RHI::Buffer *pingBuffer0 = outputBuffer;
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    RHI::Buffer *stageInput = nullptr;
    if (!DispatchFFTReal512(commandList, frameThreadData, inputBuffer, pingBuffer0, pingBuffer1, RealFFTBlockCount, &stageInput)) {
        return false;
    }

    // 256 x 512 packed spectrum 을 512 x 256 으로 transpose 해서 세로 Complex FFT512 를 수행한다.
    RHI::Buffer *transposeOutput = (stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0;
    transition2(
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(transposeComplex256x512PSO, stageInput, transposeOutput, TransposeTileCountX, TransposeTileCountY, planeCount);

    // 세로 FFT512 스테이지
    return DispatchFFTComplexToOutput(commandList, frameThreadData, transposeOutput, outputBuffer, pingBuffer0, pingBuffer1, VerticalComplexFFTBlockCount, 3, fftComplex512PSO, nullptr, -1.0f, 1.0f);
}

bool GpuFFT::DispatchFFTReal2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTReal2D1024");
    assert(realFFTTwiddle1024Buffer);

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t RealFFTBlockCount = 1024 * planeCount;
    const uint32_t VerticalComplexFFTBlockCount = 512 * planeCount;
    constexpr uint32_t TransposeTileCountX = 512 / TransposeComplexTileSize;
    constexpr uint32_t TransposeTileCountY = 1024 / TransposeComplexTileSize;

    RHI::Buffer *pingBuffer0 = outputBuffer;
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    RHI::Buffer *stageInput = nullptr;
    if (!DispatchFFTReal1024(commandList, frameThreadData, inputBuffer, pingBuffer0, pingBuffer1, RealFFTBlockCount, &stageInput)) {
        return false;
    }

    // 512 x 1024 packed spectrum 을 1024 x 512 로 transpose 해서 세로 Complex FFT1024 를 수행한다.
    RHI::Buffer *transposeOutput = (stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0;
    transition2(
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(transposeComplex512x1024PSO, stageInput, transposeOutput, TransposeTileCountX, TransposeTileCountY, planeCount);

    // 세로 FFT1024 스테이지
    return DispatchFFTComplexToOutput(commandList, frameThreadData, transposeOutput, outputBuffer, pingBuffer0, pingBuffer1, VerticalComplexFFTBlockCount, 3, fftComplex1024PSO, fftComplex1024LastStagePSO, -1.0f, 1.0f);
}

bool GpuFFT::DispatchIFFTReal2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTReal2D512");
    assert(realFFTTwiddle512Buffer);

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t RealFFTBlockCount = 512 * planeCount;
    const uint32_t VerticalComplexFFTBlockCount = 256 * planeCount;
    constexpr uint32_t TransposeTileCountX = 512 / TransposeComplexTileSize;
    constexpr uint32_t TransposeTileCountY = 256 / TransposeComplexTileSize;

    RHI::Buffer *pingBuffer0 = inputBuffer; // Input is consumed as ping-pong storage.
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 세로 IFFT512 스테이지
    RHI::Buffer *stageInput = nullptr;
    if (!DispatchIFFTComplex512(commandList, frameThreadData, pingBuffer0, pingBuffer1, VerticalComplexFFTBlockCount, 1.0f, 1.0f / 512.0f, fftComplex512PSO, &stageInput)) {
        return false;
    }

    // 512 x 256 spectrum 을 256 x 512 로 transpose 해서 가로 Real IFFT512 를 수행한다.
    RHI::Buffer *transposeOutput = (stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0;
    transition2(
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(transposeComplex512x256PSO, stageInput, transposeOutput, TransposeTileCountX, TransposeTileCountY, planeCount);

    return DispatchIFFTReal512(commandList, frameThreadData, transposeOutput, outputBuffer, stageInput, RealFFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute);
}

bool GpuFFT::DispatchIFFTReal2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTReal2D1024");
    assert(realFFTTwiddle1024Buffer);

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t RealFFTBlockCount = 1024 * planeCount;
    const uint32_t VerticalComplexFFTBlockCount = 512 * planeCount;
    constexpr uint32_t TransposeTileCountX = 1024 / TransposeComplexTileSize;
    constexpr uint32_t TransposeTileCountY = 512 / TransposeComplexTileSize;

    RHI::Buffer *pingBuffer0 = inputBuffer; // Input is consumed as ping-pong storage.
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 세로 IFFT1024 스테이지
    RHI::Buffer *stageInput = nullptr;
    if (!DispatchIFFTComplex1024(commandList, frameThreadData, pingBuffer0, pingBuffer1, VerticalComplexFFTBlockCount, 1.0f, 1.0f / 1024.0f, fftComplex1024PSO, fftComplex1024LastStagePSO, &stageInput)) {
        return false;
    }

    // 1024 x 512 spectrum 을 512 x 1024 로 transpose 해서 가로 Real IFFT1024 를 수행한다.
    RHI::Buffer *transposeOutput = (stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0;
    transition2(
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(transposeComplex1024x512PSO, stageInput, transposeOutput, TransposeTileCountX, TransposeTileCountY, planeCount);

    return DispatchIFFTReal1024(commandList, frameThreadData, transposeOutput, outputBuffer, stageInput, RealFFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute);
}

bool GpuFFT::DispatchFFTDualReal1D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTDualReal1D512");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    RHI::Buffer *resultBuffer = nullptr;
    if (!DispatchFFTDualReal512(commandList, frameThreadData, inputBuffer, outputBuffer, workBuffer, fftBlockCount, &resultBuffer)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchFFTDualReal1D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTDualReal1D1024");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    RHI::Buffer *resultBuffer = nullptr;
    if (!DispatchFFTDualReal1024(commandList, frameThreadData, inputBuffer, workBuffer, outputBuffer, fftBlockCount, &resultBuffer)) {
        return false;
    }

    RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));

    return true;
}

bool GpuFFT::DispatchIFFTDualReal1D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTDualReal1D512");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    return DispatchIFFTDualReal512(commandList, frameThreadData, inputBuffer, outputBuffer, workBuffer, fftBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined);
}

bool GpuFFT::DispatchIFFTDualReal1D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTDualReal1D1024");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    return DispatchIFFTDualReal1024(commandList, frameThreadData, inputBuffer, outputBuffer, workBuffer, fftBlockCount, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::Undefined);
}

bool GpuFFT::DispatchFFTDualReal2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTDualReal2D512");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t FFTBlockCount = 512 * planeCount;
    constexpr uint32_t TransposeTileCount = 512 / TransposeComplexTileSize;

    RHI::Buffer *pingBuffer0 = outputBuffer; // Used as ping-pong storage before the final write.
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 가로 FFT 스테이지
    RHI::Buffer *stageInput = nullptr;
    if (!DispatchFFTDualReal512(commandList, frameThreadData, inputBuffer, pingBuffer0, pingBuffer1, FFTBlockCount, &stageInput)) {
        return false;
    }

    // 512 x 512 transpose 해서 다음 FFT 스테이지는 세로로 하게 된다.
    RHI::Buffer *transposeOutput = (stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0;
    transition2(
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(transposeComplex512PSO, stageInput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 세로 FFT 스테이지
    return DispatchFFTComplexToOutput(commandList, frameThreadData, transposeOutput, outputBuffer, pingBuffer0, pingBuffer1, FFTBlockCount, 3, fftComplex512PSO, nullptr, -1.0f, 1.0f);
}

bool GpuFFT::DispatchFFTDualReal2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchFFTDualReal2D1024");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t FFTBlockCount = 1024 * planeCount;
    constexpr uint32_t TransposeTileCount = 1024 / TransposeComplexTileSize;

    RHI::Buffer *pingBuffer0 = outputBuffer; // Used as ping-pong storage before the final write.
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 가로 FFT 스테이지
    RHI::Buffer *stageInput = nullptr;
    if (!DispatchFFTDualReal1024(commandList, frameThreadData, inputBuffer, pingBuffer0, pingBuffer1, FFTBlockCount, &stageInput)) {
        return false;
    }

    // 1024 x 1024 transpose 해서 다음 FFT 스테이지는 세로로 하게 된다.
    RHI::Buffer *transposeOutput = (stageInput == pingBuffer0) ? pingBuffer1 : pingBuffer0;
    transition2(
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(transposeOutput, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(transposeComplex1024PSO, stageInput, transposeOutput, TransposeTileCount, TransposeTileCount, planeCount);

    // 세로 FFT 스테이지
    return DispatchFFTComplexToOutput(commandList, frameThreadData, transposeOutput, outputBuffer, pingBuffer0, pingBuffer1, FFTBlockCount, 3, fftComplex1024PSO, fftComplex1024LastStagePSO, -1.0f, 1.0f);
}

bool GpuFFT::DispatchIFFTDualReal2D512(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTDualReal2D512");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t FFTBlockCount = 512 * planeCount;
    constexpr uint32_t TransposeTileCount = 512 / TransposeComplexTileSize;

    RHI::Buffer *pingBuffer0 = inputBuffer; // Input is consumed as ping-pong storage.
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 세로 IFFT 스테이지 (이미 DispatchFFTDualReal2D512 에서 마지막에 세로로 끝났으므로, 세로 데이터라고 가정하고 시작)
    RHI::Buffer *stageInput = nullptr;
    if (!DispatchIFFTComplex512(commandList, frameThreadData, pingBuffer0, pingBuffer1, FFTBlockCount, 1.0f, 1.0f / 512.0f, fftComplex512PSO, &stageInput)) {
        return false;
    }

    // 512 x 512 transpose 해서 다음 IFFT 스테이지는 가로로 하게 된다.
    transition2(
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(transposeComplex512PSO, stageInput, pingBuffer0, TransposeTileCount, TransposeTileCount, planeCount);

    return DispatchIFFTDualReal512(commandList, frameThreadData, pingBuffer0, outputBuffer, stageInput, FFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute);
}

bool GpuFFT::DispatchIFFTDualReal2D1024(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t planeCount) {
    PROFILER_CPU_SCOPED_EVENT("GpuFFT::DispatchIFFTDualReal2D1024");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    const uint32_t FFTBlockCount = 1024 * planeCount;
    constexpr uint32_t TransposeTileCount = 1024 / TransposeComplexTileSize;

    RHI::Buffer *pingBuffer0 = inputBuffer; // Input is consumed as ping-pong storage.
    RHI::Buffer *pingBuffer1 = workBuffer;

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pso, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) {
        RHI::renderer->SetPSO(commandList, pso);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, groupCountY, groupCountZ);
    };

    // 세로 IFFT 스테이지 (이미 DispatchFFTDualReal2D1024 에서 마지막에 세로로 끝났으므로, 세로 데이터라고 가정하고 시작)
    RHI::Buffer *stageInput = nullptr;
    if (!DispatchIFFTComplex1024(commandList, frameThreadData, pingBuffer0, pingBuffer1, FFTBlockCount, 1.0f, 1.0f / 1024.0f, fftComplex1024PSO, fftComplex1024LastStagePSO, &stageInput)) {
        return false;
    }

    // 1024 x 1024 transpose 해서 다음 IFFT 스테이지는 가로로 하게 된다.
    transition2(
        RHI::Renderer::MakeBufferBarrier(stageInput, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(transposeComplex1024PSO, stageInput, pingBuffer1, TransposeTileCount, TransposeTileCount, planeCount);

    return DispatchIFFTDualReal1024(commandList, frameThreadData, pingBuffer1, outputBuffer, pingBuffer0, FFTBlockCount, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute);
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

bool GpuFFT::DispatchFFTReal512(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *pingBuffer0, RHI::Buffer *pingBuffer1, uint32_t fftBlockCount, RHI::Buffer **resultBuffer) {
    assert(realFFTTwiddle512Buffer);

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto transition3 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1, const RHI::GPUBarrier &barrier2) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1, barrier2 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, 1, 1);
    };

    auto dispatchRealBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 1, false, realFFTTwiddle512Buffer);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, 1, 1);
    };

    // 가로 FFT 스테이지 0
    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(fftReal512FirstStagePSO, inputBuffer, pingBuffer1, fftBlockCount);

    // 가로 FFT256 스테이지 1
    transition3(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, fftComplex256PSO, pingBuffer1, pingBuffer0, fftBlockCount, 1, -1.0f, 1.0f)) {
        return false;
    }

    // 가로 FFT256 스테이지 2
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, fftComplex256PSO, pingBuffer0, pingBuffer1, fftBlockCount, 2, -1.0f, 1.0f)) {
        return false;
    }

    // 가로 FFT 스테이지 (마지막)
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchRealBufferPSO(fftReal512LastStagePSO, pingBuffer1, pingBuffer0, fftBlockCount);

    *resultBuffer = pingBuffer0;
    return true;
}

bool GpuFFT::DispatchFFTReal1024(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *pingBuffer0, RHI::Buffer *pingBuffer1, uint32_t fftBlockCount, RHI::Buffer **resultBuffer) {
    assert(realFFTTwiddle1024Buffer);

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto transition3 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1, const RHI::GPUBarrier &barrier2) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1, barrier2 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, 1, 1);
    };

    auto dispatchRealBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 1, false, realFFTTwiddle1024Buffer);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, 1, 1);
    };

    // 가로 FFT 스테이지 0
    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(fftReal1024FirstStagePSO, inputBuffer, pingBuffer1, fftBlockCount);

    // 가로 FFT512 스테이지 1
    transition3(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, fftComplex512PSO, pingBuffer1, pingBuffer0, fftBlockCount, 1, -1.0f, 1.0f)) {
        return false;
    }

    // 가로 FFT 스테이지 2 (마지막)
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchRealBufferPSO(fftReal1024LastStagePSO, pingBuffer0, pingBuffer1, fftBlockCount);

    *resultBuffer = pingBuffer1;
    return true;
}

bool GpuFFT::DispatchIFFTReal512(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount, RHI::GPUResourceState inputBufferState, RHI::GPUResourceState workBufferState) {
    assert(realFFTTwiddle512Buffer);

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto transition3 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1, const RHI::GPUBarrier &barrier2) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1, barrier2 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, 1, 1);
    };

    auto dispatchRealBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 1, false, realFFTTwiddle512Buffer);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, 1, 1);
    };

    // 가로 IFFT 스테이지 0
    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, inputBufferState, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(workBuffer, workBufferState, RHI::GPUResourceState::UnorderedAccess));
    dispatchRealBufferPSO(ifftReal512FirstStagePSO, inputBuffer, workBuffer, fftBlockCount);

    // 가로 IFFT256 스테이지 1
    transition2(
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, fftComplex256PSO, workBuffer, inputBuffer, fftBlockCount, 1, 1.0f, 1.0f / 256.0f)) {
        return false;
    }

    // 가로 IFFT256 스테이지 2
    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, fftComplex256PSO, inputBuffer, workBuffer, fftBlockCount, 2, 1.0f, 1.0f / 256.0f)) {
        return false;
    }

    // 가로 IFFT 스테이지 3 (마지막)
    transition3(
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined));
    dispatchBufferPSO(ifftReal512LastStagePSO, workBuffer, outputBuffer, fftBlockCount);

    transition2(
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined));

    return true;
}

bool GpuFFT::DispatchIFFTReal1024(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount, RHI::GPUResourceState inputBufferState, RHI::GPUResourceState workBufferState) {
    assert(realFFTTwiddle1024Buffer);

    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto transition3 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1, const RHI::GPUBarrier &barrier2) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1, barrier2 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, 1, 1);
    };

    auto dispatchRealBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 1, false, realFFTTwiddle1024Buffer);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, 1, 1);
    };

    // 가로 IFFT 스테이지 0
    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, inputBufferState, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(workBuffer, workBufferState, RHI::GPUResourceState::UnorderedAccess));
    dispatchRealBufferPSO(ifftReal1024FirstStagePSO, inputBuffer, workBuffer, fftBlockCount);

    // 가로 IFFT512 스테이지 1
    transition2(
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, fftComplex512PSO, workBuffer, inputBuffer, fftBlockCount, 1, 1.0f, 1.0f / 512.0f)) {
        return false;
    }

    // 가로 IFFT 스테이지 2 (마지막)
    transition3(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined));
    dispatchBufferPSO(ifftReal1024LastStagePSO, inputBuffer, outputBuffer, fftBlockCount);

    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined));

    return true;
}

bool GpuFFT::DispatchFFTDualReal512(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *pingBuffer0, RHI::Buffer *pingBuffer1, uint32_t fftBlockCount, RHI::Buffer **resultBuffer) {
    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto transition3 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1, const RHI::GPUBarrier &barrier2) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1, barrier2 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, 1, 1);
    };

    // 가로 FFT 스테이지 0
    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(fftDualReal512FirstStagePSO, inputBuffer, pingBuffer0, fftBlockCount);

    // 가로 FFT 스테이지 1
    transition3(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, fftComplex512PSO, pingBuffer0, pingBuffer1, fftBlockCount, 1, -1.0f, 1.0f)) {
        return false;
    }

    // 가로 FFT 스테이지 2 (마지막)
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(fftDualReal512LastStagePSO, pingBuffer1, pingBuffer0, fftBlockCount);

    *resultBuffer = pingBuffer0;
    return true;
}

bool GpuFFT::DispatchFFTDualReal1024(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *pingBuffer0, RHI::Buffer *pingBuffer1, uint32_t fftBlockCount, RHI::Buffer **resultBuffer) {
    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto transition3 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1, const RHI::GPUBarrier &barrier2) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1, barrier2 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, 1, 1);
    };

    // 가로 FFT 스테이지 0
    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(fftDualReal1024FirstStagePSO, inputBuffer, pingBuffer0, fftBlockCount);

    // 가로 FFT 스테이지 1
    transition3(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, fftComplex1024PSO, pingBuffer0, pingBuffer1, fftBlockCount, 1, -1.0f, 1.0f)) {
        return false;
    }

    // 가로 FFT 스테이지 2
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, fftComplex1024PSO, pingBuffer1, pingBuffer0, fftBlockCount, 2, -1.0f, 1.0f)) {
        return false;
    }

    // 가로 FFT 스테이지 3 (마지막)
    transition2(
        RHI::Renderer::MakeBufferBarrier(pingBuffer0, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(pingBuffer1, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(fftDualReal1024LastStagePSO, pingBuffer0, pingBuffer1, fftBlockCount);

    *resultBuffer = pingBuffer1;
    return true;
}

bool GpuFFT::DispatchIFFTDualReal512(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount, RHI::GPUResourceState inputBufferState, RHI::GPUResourceState workBufferState) {
    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto transition3 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1, const RHI::GPUBarrier &barrier2) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1, barrier2 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, 1, 1);
    };

    // 가로 IFFT 스테이지 0
    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, inputBufferState, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(workBuffer, workBufferState, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(ifftDualReal512FirstStagePSO, inputBuffer, workBuffer, fftBlockCount);

    // 가로 IFFT 스테이지 1
    transition2(
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, fftComplex512PSO, workBuffer, inputBuffer, fftBlockCount, 1, 1.0f, 1.0f / 512.0f)) {
        return false;
    }

    // 가로 IFFT 스테이지 2 (마지막)
    transition3(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined));
    dispatchBufferPSO(ifftDualReal512LastStagePSO, inputBuffer, outputBuffer, fftBlockCount);

    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined));

    return true;
}

bool GpuFFT::DispatchIFFTDualReal1024(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, RHI::Buffer *workBuffer, uint32_t fftBlockCount, RHI::GPUResourceState inputBufferState, RHI::GPUResourceState workBufferState) {
    auto transition2 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto transition3 = [&](const RHI::GPUBarrier &barrier0, const RHI::GPUBarrier &barrier1, const RHI::GPUBarrier &barrier2) {
        RHI::GPUBarrier barriers[] = { barrier0, barrier1, barrier2 };
        RHI::renderer->Barrier(commandList, barriers, COUNT_OF(barriers));
    };

    auto dispatchBufferPSO = [&](RHI::PipelineState *pipelineState, RHI::Buffer *input, RHI::Buffer *output, uint32_t groupCountX) {
        RHI::renderer->SetPSO(commandList, pipelineState);
        RHI::renderer->SetBuffer(commandList, 0, false, input);
        RHI::renderer->SetBuffer(commandList, 0, true, output);
        RHI::renderer->Dispatch(commandList, groupCountX, 1, 1);
    };

    // 가로 IFFT 스테이지 0
    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, inputBufferState, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(workBuffer, workBufferState, RHI::GPUResourceState::UnorderedAccess));
    dispatchBufferPSO(ifftDualReal1024FirstStagePSO, inputBuffer, workBuffer, fftBlockCount);

    // 가로 IFFT 스테이지 1
    transition2(
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, fftComplex1024PSO, workBuffer, inputBuffer, fftBlockCount, 1, 1.0f, 1.0f)) {
        return false;
    }

    // 가로 IFFT 스테이지 2
    transition2(
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::UnorderedAccess));
    if (!DispatchFFTComplexStage(commandList, frameThreadData, fftComplex1024PSO, inputBuffer, workBuffer, fftBlockCount, 2, 1.0f, 1.0f)) {
        return false;
    }

    // 가로 IFFT 스테이지 3 (마지막)
    transition3(
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess),
        RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined));
    dispatchBufferPSO(ifftDualReal1024LastStagePSO, workBuffer, outputBuffer, fftBlockCount);

    transition2(
        RHI::Renderer::MakeBufferBarrier(workBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined));

    return true;
}
