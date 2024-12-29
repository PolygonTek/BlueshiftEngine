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

#include "Precompiled.h"
#include "D3D12Renderer.h"
#include "D3D12DescriptorPool.h"

void D3D12DescriptorPool::Init(ID3D12Device *device, D3D12DescriptorPool::Type type, UINT maxDescriptorCount, bool isGpuHeap) {
    this->maxDescriptorCount = maxDescriptorCount;

    D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType;
    switch (type) {
    case Type::CBV_SRV_UAV:
        descriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        break;
    case Type::RTV:
        descriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        break;
    case Type::DSV:
        descriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        break;
    case Type::Sampler:
        descriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        break;
    default:
        assert(0);
        break;
    }

    descriptorHandleSize = device->GetDescriptorHandleIncrementSize(descriptorHeapType);

    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    descriptorHeapDesc.NumDescriptors = maxDescriptorCount;
    descriptorHeapDesc.Type = descriptorHeapType;
    descriptorHeapDesc.Flags = isGpuHeap ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));

    // 내부적으로 D3D12_DESCRIPTOR_HEAP_FLAG_NONE 타입은 CPU 쪽에만 힙을 만든다.
    // descriptorHeap->GetGPUDescriptorHandleForHeapStart() 를 호출하면 크래시 발생함
    baseCpuDescriptorHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();

    if (isGpuHeap) {
        baseGpuDescriptorHandle = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    }

    idAllocator.Init(maxDescriptorCount);
}

void D3D12DescriptorPool::Shutdown() {
    SAFE_RELEASE(descriptorHeap);
}

void D3D12DescriptorPool::Clear() {
    idAllocator.Clear();
}

uint32_t D3D12DescriptorPool::AllocIndex() {
    uint32_t newIndex = static_cast<uint32_t>(-1);
    if (!idAllocator.AllocateID(newIndex)) {
        BE_WARNLOG("D3D12DescriptorPool::Alloc: no usable descriptors\n");
    }
    return newIndex;
}

void D3D12DescriptorPool::FreeIndex(uint32_t descriptorIndex) {
    idAllocator.FreeID(descriptorIndex);
}

bool D3D12DescriptorPool::Alloc(D3D12_CPU_DESCRIPTOR_HANDLE *outCpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE *outGpuDescriptorHandle) {
    uint32_t newIndex = AllocIndex();
    if (newIndex == static_cast<uint32_t>(-1)) {
        return false;
    }

    if (outCpuDescriptorHandle) {
        assert(baseCpuDescriptorHandle.ptr);
        *outCpuDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(baseCpuDescriptorHandle, newIndex, descriptorHandleSize);
    }
    if (outGpuDescriptorHandle) {
        assert(baseGpuDescriptorHandle.ptr);
        *outGpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(baseGpuDescriptorHandle, newIndex, descriptorHandleSize);
    }
    return true;
}

uint32_t D3D12DescriptorPool::GetIndexFromCPUDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle) const {
    assert(baseCpuDescriptorHandle.ptr);
    assert(baseCpuDescriptorHandle.ptr <= descriptorHandle.ptr);
    uint32_t descriptorIndex = (uint32_t)(descriptorHandle.ptr - baseCpuDescriptorHandle.ptr) / descriptorHandleSize;
    return descriptorIndex;
}

uint32_t D3D12DescriptorPool::GetIndexFromGPUDescriptorHandle(D3D12_GPU_DESCRIPTOR_HANDLE descriptorHandle) const {
    assert(baseGpuDescriptorHandle.ptr);
    assert(baseGpuDescriptorHandle.ptr <= descriptorHandle.ptr);
    uint32_t descriptorIndex = (uint32_t)(descriptorHandle.ptr - baseGpuDescriptorHandle.ptr) / descriptorHandleSize;
    return descriptorIndex;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorPool::GetCPUDescriptorHandleFromIndex(uint32_t descriptorIndex) const {
    assert(baseCpuDescriptorHandle.ptr);
    assert(descriptorIndex < maxDescriptorCount);

    return CD3DX12_CPU_DESCRIPTOR_HANDLE(baseCpuDescriptorHandle, descriptorIndex, descriptorHandleSize);
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorPool::GetGPUDescriptorHandleFromIndex(uint32_t descriptorIndex) const {
    assert(baseGpuDescriptorHandle.ptr);
    assert(descriptorIndex < maxDescriptorCount);

    return CD3DX12_GPU_DESCRIPTOR_HANDLE(baseGpuDescriptorHandle, descriptorIndex, descriptorHandleSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorPool::AllocRange(int count) {
    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = {};

    uint32_t newId;
    if (!idAllocator.AllocateRange(newId, count)) {
        BE_WARNLOG("D3D12DescriptorPool::AllocRange: no usable consecutive descriptors\n");
        return descriptorHandle;
    }

    descriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(baseCpuDescriptorHandle, (INT)newId, descriptorHandleSize);
    return descriptorHandle;
}

void D3D12DescriptorPool::FreeRange(const D3D12_CPU_DESCRIPTOR_HANDLE &descriptorHandle, int count) {
    uint32_t freeId = (uint32_t)(descriptorHandle.ptr - baseCpuDescriptorHandle.ptr) / descriptorHandleSize;

    idAllocator.FreeRange(freeId, count);
}
