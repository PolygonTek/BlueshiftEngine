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

#include "Containers/HashIndex.h"
#include "RHI/RHI.h"
#include "Shaders/ShaderInterop.h"

class Material;

/*
---------------------------------------------------------------------------------------------------

    Shader Material Table

    등록된 Material 데이터를 ShaderMaterial 테이블로 구성해 GPU 에 업로드한다.
    테이블은 bindless SRV 로 셰이더에 제공한다.

---------------------------------------------------------------------------------------------------
*/

class ShaderMaterialTable {
public:
    bool                            Init(uint32_t initialCapacity = 256);
    void                            Shutdown();

    void                            Clear();

    uint32_t                        Register(const Material *material);

    bool                            EnsureBufferCapacity(uint32_t count);

    bool                            Upload(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, bool allowResize);

    uint32_t                        GetBindlessIndex() const { return tableBuffer.bindlessIndex; }

private:
    struct TableBuffer {
        RHI::Buffer *               buffer = nullptr;
        RHI::ViewID                 view = RHI::ViewID::InvalidID;
        RHI::GPUResourceState       state = RHI::GPUResourceState::Undefined;
        uint32_t                    capacity = 0;
        uint32_t                    bindlessIndex = 0;
    };

    void                            DestroyBuffer();
    void                            Build(const Material *material, ShaderMaterial &outShaderMaterial) const;

    BE1::Array<const Material *>    materials;
    BE1::HashIndex                  materialHash { 256, 256 };
    TableBuffer                     tableBuffer;
    bool                            dirty = false;
};
