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
    maxDescriptorCount = maxCount;
    handleIncrement = device->GetDescriptorHandleIncrementSize(type);

    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    descriptorHeapDesc.NumDescriptors = maxCount;
    descriptorHeapDesc.Type = type;
    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
    assert(SUCCEEDED(hr));

    cpuDescriptorHandleBase = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    gpuDescriptorHandleBase = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
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

void D3D12DescriptorRangeAllocator::Init(const D3D12DescriptorHeapRegion &region) {
    assert(region.descriptorHeap);
    assert(region.descriptorCount > 0);

    this->region = region;
    Reset();
}

bool D3D12DescriptorRangeAllocator::AllocRange(UINT count, UINT preferredCount, D3D12DescriptorHeapRegion *outSubregion) {
    assert(outSubregion);

    // 스레드 세이프하게 블럭으로 배정한 디스크립터 수를 증가시킨다.
    UINT offset = allocatedCount.load(std::memory_order_relaxed);
    for (;;) {
        const UINT availableCount = region.descriptorCount - offset;
        if (count == 0 || count > availableCount) {
            return false;
        }

        const UINT newAllocCount = BE1::Min(availableCount, BE1::Max(count, preferredCount));
        if (allocatedCount.compare_exchange_weak(offset, offset + newAllocCount, std::memory_order_relaxed)) {
            *outSubregion = { region.descriptorHeap, region.baseDescriptorIndex + offset, newAllocCount };
            return true;
        }
    }
}

void D3D12DescriptorRangePool::Init(const D3D12DescriptorHeapRegion &region) {
    assert(region.descriptorHeap);
    assert(region.descriptorCount > 0);

    this->currentBlock = region;
    this->currentBlockOffset = 0;
    this->blockSize = 0;
    this->blockAllocator = nullptr;
}

void D3D12DescriptorRangePool::Init(D3D12DescriptorRangeAllocator *blockAllocator, UINT blockSize) {
    assert(blockAllocator && blockAllocator->GetRegion().descriptorHeap);
    assert(blockSize > 0);

    this->currentBlock = { blockAllocator->GetRegion().descriptorHeap, 0, 0 };
    this->currentBlockOffset = 0;
    this->blockSize = blockSize;
    this->blockAllocator = blockAllocator;
}

void D3D12DescriptorRangePool::Shutdown() {
    currentBlock = {};
    currentBlockOffset = 0;
    blockSize = 0;
    blockAllocator = nullptr;
}

void D3D12DescriptorRangePool::Reset() {
    currentBlockOffset = 0;

    if (blockAllocator) {
        // 이전 블럭 참조를 초기화한다.
        currentBlock.baseDescriptorIndex = 0;
        currentBlock.descriptorCount = 0;
    }
}

bool D3D12DescriptorRangePool::AllocRange(UINT count, D3D12_CPU_DESCRIPTOR_HANDLE *outCpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE *outGpuDescriptorHandle) {
    if (count == 0) {
        return false;
    }

    if (count > GetRemainingInBlock()) {
        // 현재 블럭에 남은 슬롯이 부족하면 새 블럭을 확보한다.
        if (!blockAllocator || !blockAllocator->AllocRange(count, blockSize, &currentBlock)) {
            BE_WARNLOG("D3D12DescriptorRangePool::AllocRange: out of transient descriptors\n");
            return false;
        }
        currentBlockOffset = 0;
    }

    if (outCpuDescriptorHandle) {
        *outCpuDescriptorHandle = currentBlock.GetCPUDescriptorHandleFromOffset(currentBlockOffset);
    }
    if (outGpuDescriptorHandle) {
        *outGpuDescriptorHandle = currentBlock.GetGPUDescriptorHandleFromOffset(currentBlockOffset);
    }

    currentBlockOffset += count;
    return true;
}
