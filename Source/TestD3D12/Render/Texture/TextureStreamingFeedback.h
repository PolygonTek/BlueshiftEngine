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
    using ConsumeCallback           = void (*)(const uint32_t *feedbackData, uint32_t capacity);

    struct ReadbackFrame {
        enum class State : uint8_t {
            Idle,
            Resolving,
            Submitted
        };

        RHI::Buffer *               buffer = nullptr;
        std::atomic<uint64_t>       fenceValue = 0;
        std::atomic<State>          state = State::Idle;

        void                        SetReadbackFenceValue(uint64_t fenceValue);
    };

    bool                            Init(uint32_t initialCapacity = 4096);
    void                            Shutdown();

    void                            ClearBuffer(RHI::CommandList *commandList);
    [[nodiscard]] ReadbackFrame *   Resolve(RHI::CommandList *commandList);

    void                            ConsumeCompletedReadbacks(ConsumeCallback callback, void *userData = nullptr);

    bool                            NeedsGrowCapacity(uint32_t count) const { return count > bufferCapacity; }
    bool                            EnsureBufferCapacity(uint32_t count);

    uint32_t                        GetBindlessIndex() const { return bufferBindlessIndex; }

private:
    void                            DestroyBuffers();

    RHI::Buffer *                   buffer = nullptr;
    RHI::ViewID                     bufferView = RHI::ViewID::InvalidID;
    uint32_t                        bufferCapacity = 0;
    uint32_t                        bufferBindlessIndex = 0;
    bool                            bufferCleared = false;

    ReadbackFrame                   readbackFrames[2] = {};
    int32_t                         readbackFrameIndex = 0;
};
