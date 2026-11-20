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
#include "Shaders/Fog/FogInterop.h"

class RenderFrame;
struct FrameLocalFogVolume;

class ShaderLocalFogVolumeTable {
public:
    void                            Shutdown();

    bool                            Upload(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const RenderFrame *frame);

    uint32_t                        GetVolumeBindlessIndex() const { return volumeBuffer.bindlessIndex; }
    uint32_t                        GetVolumeBoundsBindlessIndex() const { return boundsUploaded ? volumeBoundsBuffer.bindlessIndex : 0; }

private:
    struct TableBuffer {
        RHI::Buffer *               buffer = nullptr;
        RHI::GPUResourceState       state = RHI::GPUResourceState::Undefined;
        uint32_t                    capacity = 0;
        uint32_t                    bindlessIndex = 0;
    };

    bool                            EnsureBufferCapacity(TableBuffer &buffer, uint32_t count, uint32_t elementSize, RHI::Format format, uint32_t stride, const char *name);
    void                            UploadBuffer(RHI::CommandList *commandList, TableBuffer &buffer, RHI::Buffer *uploadBuffer, uint32_t uploadSize);
    void                            DestroyBuffer(TableBuffer &buffer);

    static void                     Build(const FrameLocalFogVolume &frameVolume, ShaderLocalFogVolume &shaderVolume);

    TableBuffer                     volumeBuffer;
    TableBuffer                     volumeBoundsBuffer;
    bool                            boundsUploaded = false;
};
