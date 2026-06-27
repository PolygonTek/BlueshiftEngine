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

struct VisCamera;
struct VisSurf;

class ShaderSurfaceTable {
public:
    bool                            Init(uint32_t initialCapacity = 1024);
    void                            Shutdown();

    void                            Clear();

    bool                            EnsureBufferCapacity(uint32_t count);

    bool                            Upload(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const VisCamera *visCamera, bool allowResize);

    uint32_t                        GetBindlessIndex() const { return bufferBindlessIndex; }

private:
    void                            DestroyBuffers();
    void                            Build(const VisSurf *visSurf, ShaderSurface &outShaderSurface) const;

    RHI::Buffer *                   buffer = nullptr;
    RHI::ViewID                     bufferView = RHI::ViewID::InvalidID;
    RHI::GPUResourceState           bufferState = RHI::GPUResourceState::Undefined;
    uint32_t                        bufferCapacity = 0;
    uint32_t                        bufferBindlessIndex = 0;
};
