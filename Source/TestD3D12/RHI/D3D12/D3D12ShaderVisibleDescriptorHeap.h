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

#include "D3D12Common.h"
#include "Core/IDAllocator.h"

class D3D12Renderer;

class D3D12ShaderVisibleDescriptorHeap {
public:
    D3D12ShaderVisibleDescriptorHeap() = default;
    ~D3D12ShaderVisibleDescriptorHeap() { Shutdown(); }

    void                            Init(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT maxCount);
    void                            Shutdown();

    ID3D12DescriptorHeap *          GetDescriptorHeap() const { return descriptorHeap; }
    UINT                            GetHandleIncrement() const { return handleIncrement; }

    D3D12_CPU_DESCRIPTOR_HANDLE     GetCPUDescriptorHandleFromIndex(UINT index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE     GetGPUDescriptorHandleFromIndex(UINT index) const;

private:
    ID3D12DescriptorHeap *          descriptorHeap = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE     baseCpuDescriptorHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE     baseGpuDescriptorHandle = {};
    UINT                            handleIncrement = 0;
    UINT                            maxDescriptorCount = 0;
};

BE_INLINE D3D12_CPU_DESCRIPTOR_HANDLE D3D12ShaderVisibleDescriptorHeap::GetCPUDescriptorHandleFromIndex(UINT index) const {
    assert(descriptorHeap);
    assert(index < maxDescriptorCount);

    return CD3DX12_CPU_DESCRIPTOR_HANDLE(baseCpuDescriptorHandle, index, handleIncrement);
}

BE_INLINE D3D12_GPU_DESCRIPTOR_HANDLE D3D12ShaderVisibleDescriptorHeap::GetGPUDescriptorHandleFromIndex(UINT index) const {
    assert(descriptorHeap);
    assert(index < maxDescriptorCount);

    return CD3DX12_GPU_DESCRIPTOR_HANDLE(baseGpuDescriptorHandle, index, handleIncrement);
}

struct D3D12DescriptorHeapRegion {
    D3D12ShaderVisibleDescriptorHeap *descriptorHeap = nullptr;
    UINT                            baseDescriptorIndex = 0;
    UINT                            descriptorCount = 0;

    D3D12_CPU_DESCRIPTOR_HANDLE     GetCPUDescriptorHandleFromOffset(UINT offset) const;
    D3D12_GPU_DESCRIPTOR_HANDLE     GetGPUDescriptorHandleFromOffset(UINT offset) const;
};

BE_INLINE D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeapRegion::GetCPUDescriptorHandleFromOffset(UINT offset) const {
    assert(descriptorHeap);
    assert(offset < descriptorCount);

    return descriptorHeap->GetCPUDescriptorHandleFromIndex(baseDescriptorIndex + offset);
}

BE_INLINE D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeapRegion::GetGPUDescriptorHandleFromOffset(UINT offset) const {
    assert(descriptorHeap);
    assert(offset < descriptorCount);

    return descriptorHeap->GetGPUDescriptorHandleFromIndex(baseDescriptorIndex + offset);
}

class D3D12BindlessDescriptorAllocator {
public:
    D3D12BindlessDescriptorAllocator() = default;
    ~D3D12BindlessDescriptorAllocator() { Shutdown(); }

    void                            Init(const D3D12DescriptorHeapRegion &region);
    void                            Shutdown();

    uint32_t                        Alloc();
    void                            Free(uint32_t index);

    bool                            IsValidIndex(uint32_t index) const { return index != NullIndex && index < region.descriptorCount; }

    D3D12_CPU_DESCRIPTOR_HANDLE     GetCPUDescriptorHandleFromIndex(uint32_t index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE     GetGPUDescriptorHandleFromIndex(uint32_t index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE     GetBaseGPUDescriptorHandle() const { return region.GetGPUDescriptorHandleFromOffset(0); }

    static constexpr uint32_t       NullIndex = 0;

private:
    D3D12DescriptorHeapRegion       region = {};
    BE1::IDAllocator                idAllocator;
    BE1::Mutex                      mutex;
};

BE_INLINE D3D12_CPU_DESCRIPTOR_HANDLE D3D12BindlessDescriptorAllocator::GetCPUDescriptorHandleFromIndex(uint32_t index) const {
    assert(index != NullIndex);
    assert(index < region.descriptorCount);
    return region.GetCPUDescriptorHandleFromOffset(index);
}

BE_INLINE D3D12_GPU_DESCRIPTOR_HANDLE D3D12BindlessDescriptorAllocator::GetGPUDescriptorHandleFromIndex(uint32_t index) const {
    assert(index != NullIndex);
    assert(index < region.descriptorCount);
    return region.GetGPUDescriptorHandleFromOffset(index);
}

class D3D12DescriptorRangePool {
    friend class D3D12Renderer;

public:
    D3D12DescriptorRangePool() = default;
    D3D12DescriptorRangePool(const D3D12DescriptorHeapRegion &region) { Init(region); }
    ~D3D12DescriptorRangePool() { Shutdown(); }

    void                            Init(const D3D12DescriptorHeapRegion &region);
    void                            Shutdown();

    UINT                            IsEmpty() const { return usedCount == 0; }
    void                            Reset() { usedCount = 0; }

    bool                            AllocRange(UINT count, D3D12_CPU_DESCRIPTOR_HANDLE *outCpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE *outGpuDescriptorHandle);

    ID3D12DescriptorHeap *          GetDescriptorHeap() const { return region.descriptorHeap->GetDescriptorHeap(); }
    UINT                            GetHandleIncrement() const { return region.descriptorHeap->GetHandleIncrement(); }

private:
    D3D12DescriptorHeapRegion       region = {};
    UINT                            usedCount = 0;
};
