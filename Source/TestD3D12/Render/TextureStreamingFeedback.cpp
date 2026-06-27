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

bool TextureStreamingFeedback::Init(uint32_t initialCapacity) {
    return EnsureBufferCapacity(initialCapacity);
}

void TextureStreamingFeedback::Shutdown() {
    DestroyBuffers();
}

void TextureStreamingFeedback::DestroyBuffers() {
    for (int i = 0; i < COUNT_OF(readbackFrames); ++i) {
        ReadbackFrame &readbackFrame = readbackFrames[i];
        if (readbackFrame.buffer) {
            RHI::renderer->DestroyBuffer(readbackFrame.buffer);
            readbackFrame.buffer = nullptr;
        }
        readbackFrame.fenceValue.store(0);
        readbackFrame.state.store(ReadbackFrame::State::Idle);
    }

    if (buffer) {
        RHI::renderer->DestroyBuffer(buffer);
    }
    buffer = nullptr;
    bufferView = RHI::ViewID::InvalidID;
    bufferCapacity = 0;
    bufferBindlessIndex = 0;
    bufferCleared = false;
    readbackFrameIndex = 0;
}

bool TextureStreamingFeedback::EnsureBufferCapacity(uint32_t count) {
    assert(BE1::Engine::IsInMainThread());

    if (count <= bufferCapacity) {
        return true;
    }

    // 초기 개수는 4096 으로 시작해서, 필요한 용량이 될 때까지 2배씩 늘린다.
    uint32_t newCapacity = bufferCapacity > 0 ? bufferCapacity : 4096;
    while (newCapacity < count) {
        newCapacity <<= 1;
    }

    DestroyBuffers();

    // 디폴트 버퍼를 생성한다.
    const uint32_t bufferSize = newCapacity * sizeof(uint32_t);
    buffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::UnorderedAccess, bufferSize, nullptr);
    if (!buffer) {
        DestroyBuffers();
        return false;
    }

    RHI::renderer->SetName(buffer, "textureStreamingFeedbackBuffer");

    bufferView = RHI::renderer->CreateView(buffer, RHI::ViewType::UAV, RHI::Format::R32_UINT, 0, 0, bufferSize);
    if (!bufferView.IsValid()) {
        DestroyBuffers();
        return false;
    }

    bufferCapacity = newCapacity;
    bufferBindlessIndex = buffer->GetBindlessIndex(RHI::ViewType::UAV, bufferView);

    // 리드백 버퍼를 생성한다.
    for (int i = 0; i < COUNT_OF(readbackFrames); ++i) {
        ReadbackFrame &readbackFrame = readbackFrames[i];

        readbackFrame.buffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Readback, RHI::BufferFlag::None, bufferSize, nullptr);
        if (!readbackFrame.buffer) {
            DestroyBuffers();
            return false;
        }

        readbackFrame.fenceValue.store(0);
        readbackFrame.state.store(ReadbackFrame::State::Idle);

        RHI::renderer->SetName(readbackFrame.buffer, BE1::va("textureStreamingFeedbackReadbackFrames[%i].buffer", i));
    }

    bufferCleared = false;
    readbackFrameIndex = 0;
    return true;
}

void TextureStreamingFeedback::ClearBuffer(RHI::CommandList *commandList) {
    if (!buffer || !bufferView.IsValid()) {
        return;
    }

    // 텍스쳐 스트리밍 피드백 버퍼의 내용을 0 으로 초기화한다.
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(buffer, bufferCleared ? RHI::GPUResourceState::UnorderedAccess : RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess));
    RHI::renderer->ClearUAV(commandList, buffer, bufferView, 0);
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeMemoryBarrier(buffer));

    bufferCleared = true;
}

TextureStreamingFeedback::ReadbackFrame *TextureStreamingFeedback::Resolve(RHI::CommandList *commandList) {
    if (!buffer) {
        return nullptr;
    }

    ReadbackFrame &readbackFrame = readbackFrames[readbackFrameIndex];

    if (!readbackFrame.buffer || readbackFrame.state.load() != ReadbackFrame::State::Idle) {
        return nullptr;
    }

    readbackFrame.state.store(ReadbackFrame::State::Resolving);

    // 텍스쳐 스트리밍 피드백 버퍼의 내용을 Readback 버퍼로 복사한다.
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(buffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::CopySrc));
    RHI::renderer->CopyBuffer(commandList, readbackFrame.buffer, 0, buffer, 0, bufferCapacity * sizeof(uint32_t));
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(buffer, RHI::GPUResourceState::CopySrc, RHI::GPUResourceState::UnorderedAccess));

    readbackFrameIndex = (readbackFrameIndex + 1) % COUNT_OF(readbackFrames);

    return &readbackFrame;
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

        callback(static_cast<const uint32_t *>(readbackFrame.buffer->mappedPtr), bufferCapacity);

        readbackFrame.fenceValue.store(0);
        readbackFrame.state.store(ReadbackFrame::State::Idle);
    }
}

void TextureStreamingFeedback::ReadbackFrame::SetReadbackFenceValue(uint64_t fenceValue) {
    this->fenceValue.store(fenceValue);
    this->state.store(ReadbackFrame::State::Submitted);
}
