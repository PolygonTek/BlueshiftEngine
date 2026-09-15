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

#pragma once

#include "RHI/RHI.h"

class RenderContext;
class RenderFrameData;

/*
---------------------------------------------------------------------------------------------------

    GPU Timing

---------------------------------------------------------------------------------------------------
*/

class GpuTiming {
public:
    enum class Tag : uint8_t {
        GpuFrame,
        ShadowPass,
        DepthPrepass,
        OpaquePass,
        SkyPass,
        SceneSnapshot,
        OceanPass,
        TransparentPass,
        GuiPass,
        LightCulling,
        HiZCulling,
        GenHiZ,
        HiZDebugPreview,
        GenScreenSpaceDepthPyramid,
        SSRTrace,
        SSAO,
        SurfaceOutputMSAAResolve,
        DepthMSAAResolve,
        LuminancePass1,
        LuminancePass2,
        BloomPrefilter,
        BloomDownsample,
        BloomUpsample,
        ConvolutionBloomKernelPrepare,
        ConvolutionBloomScreenPrefilter,
        ConvolutionBloomScreenPrepare,
        ConvolutionBloomFFT,
        ConvolutionBloomMultiplySpectrum,
        ConvolutionBloomIFFT,
        ConvolutionBloomResolve,
        TAAResolve,
        FinalPostProcess,
        BlitToOutput,
        ImGuiOverlay,
        Count
    };

    class ScopedTimer {
    public:
        ScopedTimer(GpuTiming &owner, RHI::CommandList *commandList, Tag tag);
        ScopedTimer(const ScopedTimer &) = delete;
        ScopedTimer &operator=(const ScopedTimer &) = delete;
        ScopedTimer(ScopedTimer &&rhs) noexcept;
        ScopedTimer &operator=(ScopedTimer &&rhs) = delete;
        ~ScopedTimer();

    private:
        GpuTiming *                 owner = nullptr;
        RHI::CommandList *          commandList = nullptr;
        uint32_t                    tagSampleIndex = 0;
        bool                        active = false;
    };

    void                            Init();
    void                            Shutdown();

    void                            BeginFrame(RHI::CommandList *commandList, const RenderFrameData *frameData, RenderContext *renderContext);
    void                            EndFrame(RHI::CommandList *commandList);

                                    // 현재 프레임의 GPU 타이밍 측정이 활성화되어 있는지 리턴한다.
    bool                            IsFrameActive() const { return currentFrameSlotIndex >= 0; }

    void                            SetFrameFence(uint64_t fenceValue);

                                    // renderContext 에 연결된 모든 프레임 슬롯들을 초기화한다.
    void                            Discard(const RenderContext *renderContext);

    static const char *             TagName(Tag tag);

private:
    static constexpr int            FrameSlotCount = 8;
    static constexpr int            SampleInterval = 4; // 몇 프레임 마다 측정할 것인지 (must be a power of two)
    static constexpr uint32_t       MaxTagSamplesPerFrame = 65; // 프레임당 최대 태그 샘플 수
    static constexpr uint32_t       SlotTimestampCount = MaxTagSamplesPerFrame * 2; // 전체 프레임 슬롯이 사용하는 타임스탬프 수 (Begin/End 쌍이므로 x 2)

    struct FrameSlot {
        RenderContext *             renderContext = nullptr;
        uint64_t                    fenceValue = 0;
        Tag                         samples[MaxTagSamplesPerFrame] = {};
        uint32_t                    sampleCount = 0;
        bool                        pending = false;
    };

                                    // 프레임 슬롯의 시작 타임스탬프 인덱스를 계산한다.
    uint32_t                        FrameSlotBaseTimestampIndex(int frameSlotIndex) const { return static_cast<uint32_t>(frameSlotIndex) * SlotTimestampCount; }
    float                           TimestampToMsec(uint64_t beginTimestamp, uint64_t endTimestamp) const { return 1000.0f * (float)(endTimestamp - beginTimestamp) / timestampFrequency; }

    void                            ReadCompleted();
    bool                            BeginScope(RHI::CommandList *commandList, Tag tag, uint32_t &tagSampleIndex);
    void                            EndScope(RHI::CommandList *commandList, uint32_t tagSampleIndex);
    static void                     ResetSlot(FrameSlot &slot);

    uint64_t                        timestampFrequency = 0;
    RHI::QueryHeap *                timestampQueryHeap = nullptr;
    RHI::Buffer *                   timestampReadbackBuffer = nullptr;
    FrameSlot                       frameSlots[FrameSlotCount];
    int                             currentFrameSlotIndex = -1;
    int                             nextFrameSlotIndex = 0;
};

BE_INLINE GpuTiming::ScopedTimer::ScopedTimer(GpuTiming &owner, RHI::CommandList *commandList, Tag tag)
    : owner(&owner), commandList(commandList) {
    active = owner.BeginScope(commandList, tag, tagSampleIndex);
}

BE_INLINE GpuTiming::ScopedTimer::ScopedTimer(ScopedTimer &&rhs) noexcept
    : owner(rhs.owner), commandList(rhs.commandList), tagSampleIndex(rhs.tagSampleIndex), active(rhs.active) {
    rhs.owner = nullptr;
    rhs.commandList = nullptr;
    rhs.active = false;
}

BE_INLINE GpuTiming::ScopedTimer::~ScopedTimer() {
    if (active) {
        owner->EndScope(commandList, tagSampleIndex);
    }
}
