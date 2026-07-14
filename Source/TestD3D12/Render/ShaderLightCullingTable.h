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
#include "Shaders/ShaderInterop.h"

class RenderFrameData;
struct VisCamera;
struct VisLight;

/*
---------------------------------------------------------------------------------------------------

    Shader Light Culling Table

    이번 프레임의 카메라별 ShaderLightRef 테이블과 light Z-bin 테이블을 GPU 에 업로드한다.
    ShaderLightRef 의 화면 타일 범위로 flat tile-light mask 를 생성한다.
    세 테이블은 bindless SRV 로 셰이더에 제공한다.

---------------------------------------------------------------------------------------------------
*/

class ShaderLightCullingTable {
public:
    bool                            Init(uint32_t initialLightRefCapacity = 1024, uint32_t initialZBinCapacity = ShaderLightZBinCount, uint32_t initialLightTileMaskCapacity = 16384);
    void                            Shutdown();

    bool                            EnsureBufferCapacity(uint32_t lightRefCount, uint32_t lightZBinCount, uint32_t lightTileMaskCount);

    bool                            Upload(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const RenderFrameData *frameData, bool allowResize);
    bool                            DispatchLightCulling(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, RHI::PipelineState *pipelineState, const RenderFrameData *frameData);

    uint32_t                        GetLightRefBindlessIndex() const { return lightRefBuffer.bindlessIndex; }
    uint32_t                        GetLightZBinBindlessIndex() const { return lightZBinBuffer.bindlessIndex; }
    uint32_t                        GetLightTileMaskBindlessIndex() const { return lightTileMaskBuffer.bindlessIndex; }

private:
    struct TableBuffer {
        RHI::Buffer *               buffer = nullptr;
        RHI::ViewID                 view = RHI::ViewID::InvalidID;
        RHI::ViewID                 uavView = RHI::ViewID::InvalidID;
        RHI::GPUResourceState       state = RHI::GPUResourceState::Undefined;
        uint32_t                    capacity = 0;
        uint32_t                    bindlessIndex = 0;
    };

    bool                            EnsureBufferCapacity(TableBuffer &tableBuffer, uint32_t count, uint32_t elementSize, RHI::Format format, uint32_t stride, bool unorderedAccess, const char *name);
    void                            UploadBuffer(RHI::CommandList *commandList, TableBuffer &tableBuffer, RHI::Buffer *uploadBuffer, uint32_t uploadSize);
    void                            DestroyBuffer(TableBuffer &tableBuffer);

    void                            Build(const VisCamera &visCamera, const VisLight &visLight, ShaderLightRef &shaderLightRef) const;

    TableBuffer                     lightRefBuffer;
    TableBuffer                     lightZBinBuffer;
    TableBuffer                     lightTileMaskBuffer;
};
