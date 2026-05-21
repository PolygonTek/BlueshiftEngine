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
#include "TextureStreamingFeedback.h"

void TextureStreamingFeedback::Init() {
    bufferCleared = false;

    buffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::UnorderedAccess, capacity * sizeof(uint32_t), nullptr);
    RHI::renderer->SetName(buffer, "buffer");
    RHI::renderer->CreateView(buffer, RHI::ViewType::UAV, RHI::Format::R32_UINT, 0, 0, capacity * sizeof(uint32_t));

    for (int i = 0; i < COUNT_OF(readbackFrames); ++i) {
        ReadbackFrame &readbackFrame = readbackFrames[i];

        readbackFrame.buffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Readback, RHI::BufferFlag::None, capacity * sizeof(uint32_t), nullptr);
        readbackFrame.mappedData = readbackFrame.buffer->GetMappedPtr();
        readbackFrame.fenceValue.store(0);
        readbackFrame.state.store(ReadbackFrame::State::Idle);

        RHI::renderer->SetName(readbackFrames[i].buffer, BE1::va("readbackFrames[%i].buffer", i));
    }

    readbackFrameIndex = 0;
}

void TextureStreamingFeedback::Shutdown() {
    for (int i = 0; i < COUNT_OF(readbackFrames); ++i) {
        RHI::renderer->DestroyBuffer(readbackFrames[i].buffer);
        readbackFrames[i].buffer = nullptr;
    }

    RHI::renderer->DestroyBuffer(buffer);
    buffer = nullptr;
}

void TextureStreamingFeedback::Clear(RHI::CommandList *commandList) {
    // 텍스쳐 스트리밍 피드백 버퍼의 내용을 0 으로 초기화한다.
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(buffer, bufferCleared ? RHI::GPUResourceState::UnorderedAccess : RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));
    RHI::renderer->ClearUAV(commandList, buffer, RHI::ViewID::PrimaryID, 0);
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeMemoryBarrier(buffer));

    bufferCleared = true;
}

TextureStreamingFeedback::ReadbackFrame *TextureStreamingFeedback::Resolve(RHI::CommandList *commandList) {
    ReadbackFrame &readbackFrame = readbackFrames[readbackFrameIndex];

    if (readbackFrame.state.load() != ReadbackFrame::State::Idle) {
        return nullptr;
    }

    readbackFrame.state.store(ReadbackFrame::State::Resolving);

    // 텍스쳐 스트리밍 피드백 버퍼의 내용을 Readback 버퍼로 복사한다.
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(buffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::CopySrc));
    RHI::renderer->CopyBuffer(commandList, readbackFrame.buffer, 0, buffer, 0, capacity * sizeof(uint32_t));
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(buffer, RHI::GPUResourceState::CopySrc, RHI::GPUResourceState::UnorderedAccess));

    readbackFrameIndex = (readbackFrameIndex + 1) % COUNT_OF(readbackFrames);

    return &readbackFrame;
}

void TextureStreamingFeedback::SetReadbackFenceValue(TextureStreamingFeedback::ReadbackFrame *readbackFrame, uint64_t fenceValue) {
    assert(readbackFrame);
    readbackFrame->fenceValue.store(fenceValue);
    readbackFrame->state.store(ReadbackFrame::State::Submitted);
}

void TextureStreamingFeedback::ConsumeCompletedReadbacks(ConsumeCallback callback, void *userData) {
    assert(BE1::Engine::IsInMainThread());

    if (!callback) {
        return;
    }

    for (ReadbackFrame &readbackFrame : readbackFrames) {
        if (readbackFrame.state.load() != ReadbackFrame::State::Submitted) {
            continue;
        }

        uint64_t fenceValue = readbackFrame.fenceValue.load();
        if (!RHI::renderer->IsFenceComplete(RHI::CommandQueueType::Graphics, fenceValue)) {
            continue;
        }

        callback(static_cast<const uint32_t *>(readbackFrame.mappedData), capacity);

        readbackFrame.fenceValue.store(0);
        readbackFrame.state.store(ReadbackFrame::State::Idle);
    }
}

void TextureStreamingFeedback::EnsureCapacity(uint32_t requiredCapacity) {
    assert(BE1::Engine::IsInMainThread());

    if (requiredCapacity <= capacity) {
        return;
    }

    uint32_t newCapacity = capacity;
    while (newCapacity < requiredCapacity) {
        newCapacity = BE1::Min(newCapacity << 1, (uint32_t)(1 << 24));
        if (newCapacity < requiredCapacity && newCapacity == (1u << 24)) {
            BE_FATALERROR("TextureStreamingFeedback::EnsureCapacity: exceeded max capacity");
        }
    }

    Shutdown();

    capacity = newCapacity;

    Init();
}
