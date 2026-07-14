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
struct VisLight;

/*
---------------------------------------------------------------------------------------------------

    Shader Light Table

    이번 프레임의 visible light 데이터를 타입별 테이블로 구성해 GPU 에 업로드한다.
    각 타입별 테이블은 bindless SRV 로 셰이더에 제공한다.

---------------------------------------------------------------------------------------------------
*/

class ShaderLightTable {
public:
    bool                            Init(uint32_t initialPointLightCapacity = 1024, uint32_t initialSpotLightCapacity = 256, uint32_t initialBoxDirLightCapacity = 64);
    void                            Shutdown();

    void                            Clear();

    bool                            EnsureBufferCapacity(uint32_t pointLightCount, uint32_t spotLightCount, uint32_t boxDirLightCount);

    bool                            Upload(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const RenderFrameData *frameData, bool allowResize);

    uint32_t                        GetPointLightBindlessIndex() const { return pointLightBuffer.bindlessIndex; }
    uint32_t                        GetSpotLightBindlessIndex() const { return spotLightBuffer.bindlessIndex; }
    uint32_t                        GetBoxDirLightBindlessIndex() const { return boxDirLightBuffer.bindlessIndex; }

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

    void                            Build(const VisLight &visLight, ShaderPointLight &shaderLight) const;
    void                            Build(const VisLight &visLight, ShaderSpotLight &shaderLight) const;
    void                            Build(const VisLight &visLight, ShaderBoxDirLight &shaderLight) const;

    TableBuffer                     pointLightBuffer;
    TableBuffer                     spotLightBuffer;
    TableBuffer                     boxDirLightBuffer;
};
