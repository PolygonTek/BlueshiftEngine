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

class D3D12Texture : public RHIRenderer::Texture {
    friend class D3D12Renderer;

public:
    virtual ~D3D12Texture() { Release(); }

    void                            Release();

    virtual void *                  GetNativeResource() const override { return GetResource(); }
    virtual const void *            GetNativeTextureObject() const override { return this; }

    ID3D12Resource *                GetResource() const;

    static void                     AdjustTextureFormat(bool useCompression, bool useNormalMap, Image::Format::Enum inFormat, Image::Format::Enum *outFormat);

private:
#ifdef USE_D3D12_MEMALLOC
    D3D12MA::Allocation *           textureAllocation = nullptr;
#else
    ID3D12Resource *                textureResource = nullptr;
#endif
    D3D12_RESOURCE_DESC             textureDesc;
    D3D12_CPU_DESCRIPTOR_HANDLE     srvCpuDescriptorHandle = {};
    D3D12_CPU_DESCRIPTOR_HANDLE     rtvCpuDescriptorHandle = {};
    D3D12_CPU_DESCRIPTOR_HANDLE     dsvCpuDescriptorHandle = {};
    D3D12_CPU_DESCRIPTOR_HANDLE     uavCpuDescriptorHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE     uavGpuDescriptorHandle = {};
};
