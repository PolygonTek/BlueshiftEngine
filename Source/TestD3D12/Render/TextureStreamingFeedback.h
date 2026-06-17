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

class TextureStreamingFeedback {
    friend class RenderBackend;

public:
    using ConsumeCallback       = void (*)(const uint32_t *feedbackData, uint32_t capacity);

    struct ReadbackFrame {
        enum class State : uint8_t {
            Idle,
            Resolving,
            Submitted
        };

        RHI::Buffer *           buffer = nullptr;
        std::atomic<uint64_t>   fenceValue = 0;
        std::atomic<State>      state = State::Idle;
    };

    void                        Init();
    void                        Shutdown();

    void                        Clear(RHI::CommandList *commandList);
    ReadbackFrame *             Resolve(RHI::CommandList *commandList);
    void                        SetReadbackFenceValue(ReadbackFrame *readbackFrame, uint64_t fenceValue);

    void                        ConsumeCompletedReadbacks(ConsumeCallback callback, void *userData = nullptr);

    bool                        NeedsGrowCapacity(uint32_t requiredCapacity) const { return requiredCapacity > capacity; }
    void                        EnsureCapacity(uint32_t requiredCapacity);

private:
    RHI::Buffer *               buffer = nullptr;
    bool                        bufferCleared = false;
    uint32_t                    capacity = 4096;
    int32_t                     readbackFrameIndex = 0;
    ReadbackFrame               readbackFrames[2] = {};
};
