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

#include "Precompiled.h"
#include "D3D12ShaderVisibleDescriptorHeap.h"

void D3D12ShaderVisibleDescriptorHeap::Init(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT maxCount) {
    assert(device);
    assert(maxCount > 0);
    assert(!descriptorHeap);

    maxDescriptorCount = maxCount;
    descriptorHandleSize = device->GetDescriptorHandleIncrementSize(type);

    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    descriptorHeapDesc.NumDescriptors = maxCount;
    descriptorHeapDesc.Type = type;
    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
    assert(SUCCEEDED(hr));

    baseCpuDescriptorHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    baseGpuDescriptorHandle = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
}

void D3D12ShaderVisibleDescriptorHeap::Shutdown() {
    SAFE_RELEASE(descriptorHeap);
}

void D3D12BindlessDescriptorAllocator::Init(const D3D12DescriptorHeapRegion &region) {
    assert(region.descriptorHeap);
    assert(region.descriptorCount > 1);

    this->region = region;

    idAllocator.Init(region.descriptorCount - 1);
}

void D3D12BindlessDescriptorAllocator::Shutdown() {
    idAllocator.Clear();
}

uint32_t D3D12BindlessDescriptorAllocator::Alloc() {
    uint32_t id = static_cast<uint32_t>(-1);
    {
        BE1::ScopedLock lock(mutex);
        if (!idAllocator.AllocateID(id)) {
            BE_WARNLOG("D3D12BindlessDescriptorAllocator::Alloc: out of descriptors\n");
            return NullIndex;
        }
    }

    const uint32_t index = id + 1;

    assert(index != NullIndex);
    assert(index < region.descriptorCount);
    return index;
}

void D3D12BindlessDescriptorAllocator::Free(uint32_t index) {
    assert(index != NullIndex);
    assert(index < region.descriptorCount);
    if (index == NullIndex) {
        return;
    }

    BE1::ScopedLock lock(mutex);
    idAllocator.FreeID(index - 1);
}

void D3D12DescriptorTablePool::Init(const D3D12DescriptorHeapRegion &region) {
    assert(region.descriptorHeap);
    assert(region.descriptorCount > 0);

    this->region = region;
    usedCount = 0;
}

void D3D12DescriptorTablePool::Shutdown() {
    region = {};
    usedCount = 0;
}

bool D3D12DescriptorTablePool::AllocRange(UINT count, D3D12_CPU_DESCRIPTOR_HANDLE *outCpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE *outGpuDescriptorHandle) {
    if (usedCount + count > region.descriptorCount) {
        BE_WARNLOG("D3D12DescriptorTablePool::AllocRange: exceeds the descriptor region count\n");
        return false;
    }

    if (outCpuDescriptorHandle) {
        *outCpuDescriptorHandle = region.GetCPUDescriptorHandleFromOffset(usedCount);
    }
    if (outGpuDescriptorHandle) {
        *outGpuDescriptorHandle = region.GetGPUDescriptorHandleFromOffset(usedCount);
    }

    usedCount += count;
    return true;
}
