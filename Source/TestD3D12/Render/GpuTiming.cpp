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
#include "GpuTiming.h"
#include "RenderContext.h"
#include "RenderFrameData.h"

const char *GpuTiming::TagName(Tag tag) {
    static const char *tagNames[static_cast<int>(GpuTiming::Tag::Count)] = {
        "GPU Frame",
        "Shadow Pass",
        "Depth Prepass",
        "Opaque Pass",
        "Sky Pass",
        "Scene Snapshot",
        "Ocean Pass",
        "Transparent Pass",
        "GUI Pass",
        "Light Culling",
        "Hi-Z Culling",
        "Gen Hi-Z",
        "Hi-Z Debug Preview",
        "Gen Screen-Space Depth Pyramid",
        "SSR Trace",
        "SSAO",
        "GTAO Depth Prefilter",
        "GTAO Main",
        "GTAO Denoise",
        "AO Bilateral Filter",
        "AO Upsample",
        "Surface Output MSAA Resolve",
        "Depth MSAA Resolve",
        "Luminance Pass 1",
        "Luminance Pass 2",
        "Bloom Prefilter",
        "Bloom Downsample",
        "Bloom Upsample",
        "Conv Bloom Kernel Prepare",
        "Conv Bloom Screen Prefilter",
        "Conv Bloom Screen Prepare",
        "Conv Bloom FFT",
        "Conv Bloom Multiply Spectrum",
        "Conv Bloom IFFT",
        "Conv Bloom Resolve",
        "TAA Resolve",
        "Final Post Process",
        "Blit To Output",
        "ImGui Overlay"
    };
    static_assert(COUNT_OF(tagNames) == static_cast<size_t>(GpuTiming::Tag::Count), "GpuTiming tag name table must match Tag::Count");

    const int tagIndex = static_cast<int>(tag);
    if (tagIndex < 0 || tagIndex >= static_cast<int>(Tag::Count)) {
        return "Unknown";
    }
    return tagNames[tagIndex];
}

void GpuTiming::Init() {
    // GPU 타임스탬프가 초당 몇번 증가하는지를 얻어온다.
    timestampFrequency = RHI::renderer->GetTimestampFrequency(RHI::CommandQueueType::Graphics);

    // GPU 타임스탬프를 위한 쿼리 힙과 리드백 버퍼를 생성한다.
    // [Tag1Begin, Tag1End, Tag2Begin, Tag2End, ...]
    RHI::QueryHeapDesc queryHeapDesc;
    queryHeapDesc.type = RHI::QueryType::Timestamp;
    queryHeapDesc.queryCount = FrameSlotCount * SlotTimestampCount;
    timestampQueryHeap = RHI::renderer->CreateQueryHeap(&queryHeapDesc);

    timestampReadbackBuffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Readback, RHI::BufferFlag::None, sizeof(uint64_t) * queryHeapDesc.queryCount, nullptr);
}

void GpuTiming::Shutdown() {
    if (timestampReadbackBuffer) {
        RHI::renderer->DestroyBuffer(timestampReadbackBuffer);
        timestampReadbackBuffer = nullptr;
    }

    if (timestampQueryHeap) {
        RHI::renderer->DestroyQueryHeap(timestampQueryHeap);
        timestampQueryHeap = nullptr;
    }

    timestampFrequency = 0;
    nextFrameSlotIndex = 0;
    currentFrameSlotIndex = -1;

    for (int frameSlotIndex = 0; frameSlotIndex < FrameSlotCount; ++frameSlotIndex) {
        ResetSlot(frameSlots[frameSlotIndex]);
    }
}

bool GpuTiming::BeginScope(RHI::CommandList *commandList, Tag tag, uint32_t &tagSampleIndex) {
    if (currentFrameSlotIndex < 0) {
        return false;
    }

    FrameSlot &slot = frameSlots[currentFrameSlotIndex];
    if (slot.sampleCount >= MaxTagSamplesPerFrame) {
        return false;
    }

    tagSampleIndex = slot.sampleCount++;
    slot.samples[tagSampleIndex] = tag;

    const uint32_t timestampIndex = FrameSlotBaseTimestampIndex(currentFrameSlotIndex) + tagSampleIndex * 2;
    RHI::renderer->EndQuery(commandList, timestampQueryHeap, timestampIndex);
    return true;
}

void GpuTiming::EndScope(RHI::CommandList *commandList, uint32_t tagSampleIndex) {
    if (currentFrameSlotIndex < 0) {
        return;
    }

    FrameSlot &slot = frameSlots[currentFrameSlotIndex];
    if (tagSampleIndex >= slot.sampleCount) {
        return;
    }

    const uint32_t timestampIndex = FrameSlotBaseTimestampIndex(currentFrameSlotIndex) + tagSampleIndex * 2;
    RHI::renderer->EndQuery(commandList, timestampQueryHeap, timestampIndex + 1);
}

void GpuTiming::BeginFrame(RHI::CommandList *commandList, const RenderFrameData *frameData, RenderContext *renderContext) {
    if (!frameData->IsStatEnabled()) {
        return;
    }

    // 이미 BeginFrame 이 호출되었다면, SetFrameFence 가 호출될 때까지 무시한다.
    if (currentFrameSlotIndex >= 0) {
        return;
    }

    // RenderContext 별로 SampleInterval 에 따라 한번씩만 처리한다.
    if ((renderContext->gpuTimingSampleFrameCount++ & (SampleInterval - 1)) != 0) {
        return;
    }

    // 완료된 슬롯들의 타임스탬프를 읽어서 처리한다.
    ReadCompleted();

    // 비어있는 다음 프레임 슬롯을 찾아서 현재 프레임 슬롯으로 설정한다.
    for (int i = 0; i < FrameSlotCount; ++i) {
        int frameSlotIndex = (nextFrameSlotIndex + i) % FrameSlotCount;
        FrameSlot &slot = frameSlots[frameSlotIndex];

        if (!slot.pending) {
            currentFrameSlotIndex = frameSlotIndex;
            nextFrameSlotIndex = (frameSlotIndex + 1) % FrameSlotCount; // 다음 슬롯부터 순회하도록 설정

            slot.renderContext = renderContext;
            slot.fenceValue = 0;
            slot.samples[0] = Tag::GpuFrame;
            slot.sampleCount = 1;

            // Frame Begin Timestamp 찍기
            uint32_t timestampIndex = FrameSlotBaseTimestampIndex(frameSlotIndex);
            RHI::renderer->EndQuery(commandList, timestampQueryHeap, timestampIndex);
            break;
        }
    }
}

void GpuTiming::EndFrame(RHI::CommandList *commandList) {
    if (currentFrameSlotIndex < 0) {
        return;
    }

    FrameSlot &slot = frameSlots[currentFrameSlotIndex];
    const uint32_t timestampIndex = FrameSlotBaseTimestampIndex(currentFrameSlotIndex);

    // Frame End Timestamp 찍기
    RHI::renderer->EndQuery(commandList, timestampQueryHeap, timestampIndex + 1);

    // 현재 슬롯의 태그 샘플 Timestamp 들을 Readback 버퍼로 읽어오기
    const uint32_t timestampCount = slot.sampleCount * 2;
    RHI::renderer->ResolveQuery(commandList, timestampQueryHeap, timestampIndex, timestampCount, timestampReadbackBuffer, sizeof(uint64_t) * timestampIndex);
}

void GpuTiming::SetFrameFence(uint64_t fenceValue) {
    if (currentFrameSlotIndex < 0) {
        return;
    }

    FrameSlot &slot = frameSlots[currentFrameSlotIndex];
    slot.fenceValue = fenceValue;
    slot.pending = true;
    currentFrameSlotIndex = -1;
}

void GpuTiming::ReadCompleted() {
    static_assert(static_cast<int>(Tag::Count) <= 64, "GPU timing tags must fit in measuredTagMask");
    const uint64_t *timestamps = static_cast<const uint64_t *>(timestampReadbackBuffer->mappedPtr);

    // 모든 frame slot 들을 순회하면서, 완료된 슬롯들의 타임스탬프를 읽어서 renderContext 에 전달한다.
    for (int frameSlotIndex = 0; frameSlotIndex < FrameSlotCount; ++frameSlotIndex) {
        FrameSlot &slot = frameSlots[frameSlotIndex];
        if (!slot.pending) {
            continue;
        }

        if (!RHI::renderer->IsFenceComplete(RHI::CommandQueueType::Graphics, slot.fenceValue)) {
            continue;
        }

        const uint32_t baseTimestampIndex = FrameSlotBaseTimestampIndex(frameSlotIndex);
        if (slot.renderContext) {
            float tagFrameMsec[static_cast<int>(Tag::Count)] = {};
            uint64_t measuredTagMask = 0;
            for (uint32_t tagSampleIndex = 0; tagSampleIndex < slot.sampleCount; ++tagSampleIndex) {
                const int tagIndex = static_cast<int>(slot.samples[tagSampleIndex]);
                if (tagIndex < 0 || tagIndex >= static_cast<int>(Tag::Count)) {
                    continue;
                }

                const uint32_t tagTimestampIndex = baseTimestampIndex + tagSampleIndex * 2;
                const uint64_t tagBeginTimestamp = timestamps[tagTimestampIndex + 0];
                const uint64_t tagEndTimestamp = timestamps[tagTimestampIndex + 1];

                // 같은 tag 가 여러번 샘플되었을 수 있으므로, 같은 tag frame msec 을 누적한다.
                if (tagEndTimestamp >= tagBeginTimestamp) {
                    measuredTagMask |= uint64_t(1) << tagIndex;
                    tagFrameMsec[tagIndex] += TimestampToMsec(tagBeginTimestamp, tagEndTimestamp);
                }
            }
            slot.renderContext->UpdateGpuTagFrameTimes(tagFrameMsec, measuredTagMask);
        }

        ResetSlot(slot);
    }
}

void GpuTiming::Discard(const RenderContext *renderContext) {
    for (int frameSlotIndex = 0; frameSlotIndex < FrameSlotCount; ++frameSlotIndex) {
        FrameSlot &slot = frameSlots[frameSlotIndex];

        if (slot.renderContext != renderContext) {
            continue;
        }

        if (frameSlotIndex == currentFrameSlotIndex) {
            currentFrameSlotIndex = -1;
        }

        ResetSlot(slot);
    }
}

void GpuTiming::ResetSlot(FrameSlot &slot) {
    slot.renderContext = nullptr;
    slot.fenceValue = 0;
    slot.sampleCount = 0;
    slot.pending = false;
}
