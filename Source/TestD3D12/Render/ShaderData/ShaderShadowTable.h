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

class RenderFrame;

/*
---------------------------------------------------------------------------------------------------

    Shader Shadow Table

    이번 프레임의 카메라별 local shadow/view 데이터를 GPU default buffer 에 한 번에 업로드한다.
    버퍼는 첫 사용 시 할당하고 프레임 사이에 재사용한다. Shadow depth 캐싱과는 별개다.

---------------------------------------------------------------------------------------------------
*/

class ShaderShadowTable {
public:
    void                            Shutdown();

    bool                            Upload(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const RenderFrame *frame);

    uint32_t                        GetShadowBindlessIndex() const { return shadowBuffer.bindlessIndex; }
    uint32_t                        GetShadowViewBindlessIndex() const { return shadowViewBuffer.bindlessIndex; }

private:
    struct TableBuffer {
        RHI::Buffer *               buffer = nullptr;
        RHI::ViewID                 view = RHI::ViewID::InvalidID;
        RHI::GPUResourceState       state = RHI::GPUResourceState::Undefined;
        uint32_t                    capacity = 0;
        uint32_t                    bindlessIndex = 0;
    };

    bool                            EnsureBufferCapacity(TableBuffer &tableBuffer, uint32_t count, uint32_t elementSize, const char *name);
    void                            UploadBuffer(RHI::CommandList *commandList, TableBuffer &tableBuffer, RHI::Buffer *uploadBuffer, uint32_t uploadSize);
    void                            DestroyBuffer(TableBuffer &tableBuffer);

    TableBuffer                     shadowBuffer;
    TableBuffer                     shadowViewBuffer;
};
