// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "../RHIRenderer.h"
#include "D3D12Common.h"

#ifdef USE_D3D12_MEMALLOC
#include "D3D12MemoryAllocator/D3D12MemAlloc.h"
#endif

class D3D12Renderer;
class D3D12FrameData;

class D3D12Buffer : public RHIRenderer::Buffer {
    friend class D3D12Renderer;
    friend class D3D12FrameData;

public:
    D3D12Buffer() = default;
    virtual ~D3D12Buffer() { Release(); }

    void                            Release();

    virtual void *                  GetNativeResource() const override { return GetResource(); }
    virtual const void *            GetNativeBufferObject() const override { return this; }

    ID3D12Resource *                GetResource() const;
    uint64_t                        GetSize();

private:
#ifdef USE_D3D12_MEMALLOC
    D3D12MA::Allocation *           bufferAllocation = nullptr;
#else
    ID3D12Resource *                bufferResource = nullptr;
#endif
    Image::Format::Enum             format = Image::Format::Unknown;
    uint32_t                        stride = 0;
    uint64_t                        size = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE     srvCpuDescriptorHandle = {};
    D3D12_CPU_DESCRIPTOR_HANDLE     uavCpuDescriptorHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE     uavGpuDescriptorHandle = {};
};
