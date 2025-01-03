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

#include "D3D12Common.h"

class D3D12Renderer;

class D3D12DescriptorPool {
    friend class D3D12Renderer;

public:
    enum class Type : byte {
        CBV_SRV_UAV,
        RTV,
        DSV,
        Sampler
    };

    D3D12DescriptorPool() = default;
    D3D12DescriptorPool(ID3D12Device *device, Type type, UINT maxCount, bool isGpuHeap) { Init(device, type, maxCount, isGpuHeap); }
    ~D3D12DescriptorPool() { Shutdown(); }

    void                            Init(ID3D12Device *device, Type type, UINT maxCount, bool isGpuHeap);
    void                            Shutdown();

    void                            Clear();

    uint32_t                        AllocIndex();
    void                            FreeIndex(uint32_t descriptorIndex);
    bool                            Alloc(D3D12_CPU_DESCRIPTOR_HANDLE *outCpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE *outGpuDescriptorHandle);
    void                            Free(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle);

    D3D12_CPU_DESCRIPTOR_HANDLE     GetBaseCPUDescriptorHandle() const { return baseCpuDescriptorHandle; }
    D3D12_GPU_DESCRIPTOR_HANDLE     GetBaseGPUDescriptorHandle() const { return baseGpuDescriptorHandle; }

    uint32_t                        GetIndexFromCPUDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle) const;
    uint32_t                        GetIndexFromGPUDescriptorHandle(D3D12_GPU_DESCRIPTOR_HANDLE descriptorHandle) const;
    D3D12_CPU_DESCRIPTOR_HANDLE     GetCPUDescriptorHandleFromIndex(uint32_t descriptorIndex) const;
    D3D12_GPU_DESCRIPTOR_HANDLE     GetGPUDescriptorHandleFromIndex(uint32_t descriptorIndex) const;

    D3D12_CPU_DESCRIPTOR_HANDLE     AllocRange(int count);
    void                            FreeRange(const D3D12_CPU_DESCRIPTOR_HANDLE &descriptorHandle, int count);

private:
    ID3D12DescriptorHeap *          descriptorHeap = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE     baseCpuDescriptorHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE     baseGpuDescriptorHandle = {};
    UINT                            descriptorHandleSize;
    UINT                            maxDescriptorCount;
    BE1::IDAllocator                idAllocator;
};
