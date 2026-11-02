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
#include <atomic>

class D3D12Renderer;

// shader-visible 디스크립터 힙을 관리하고 인덱스에 대응하는 CPU/GPU 핸들을 제공한다.
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
    D3D12_CPU_DESCRIPTOR_HANDLE     cpuDescriptorHandleBase = {};
    D3D12_GPU_DESCRIPTOR_HANDLE     gpuDescriptorHandleBase = {};
    UINT                            handleIncrement = 0;
    UINT                            maxDescriptorCount = 0;
};

BE_INLINE D3D12_CPU_DESCRIPTOR_HANDLE D3D12ShaderVisibleDescriptorHeap::GetCPUDescriptorHandleFromIndex(UINT index) const {
    assert(descriptorHeap);
    assert(index < maxDescriptorCount);

    return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuDescriptorHandleBase, index, handleIncrement);
}

BE_INLINE D3D12_GPU_DESCRIPTOR_HANDLE D3D12ShaderVisibleDescriptorHeap::GetGPUDescriptorHandleFromIndex(UINT index) const {
    assert(descriptorHeap);
    assert(index < maxDescriptorCount);

    return CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuDescriptorHandleBase, index, handleIncrement);
}

// shader-visible 디스크립터 힙의 일정 범위를 나타내는 구조체.
struct D3D12DescriptorHeapRegion {
    D3D12ShaderVisibleDescriptorHeap *descriptorHeap = nullptr;
    UINT                            baseDescriptorIndex = 0;    // heap 내에서의 시작 인덱스
    UINT                            descriptorCount = 0;        // 이 영역에 포함된 디스크립터 슬롯 수

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

// 지정된 영역에서 bindless 디스크립터 인덱스를 개별 할당/해제한다.
// Alloc/Free 는 스레드 세이프하게 인덱스를 할당/해제한다. (인덱스 0 은 NullIndex 로 예약되어 있다)
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

// 지정된 영역에서 연속된 디스크립터 구간을 중복 없이 할당한다.
// AllocRange 는 스레드 세이프하게 블럭을 할당한다.
class D3D12DescriptorRangeAllocator {
public:
    void                            Init(const D3D12DescriptorHeapRegion &region);

                                    // Reset 은 GPU 사용 완료 후, 로컬 풀들을 리셋하기 전에 한 번만 호출한다.
    void                            Reset() { allocatedCount.store(0, std::memory_order_relaxed); }

                                    // count 이상을 확보하되, 남은 용량 내에서 preferredCount 만큼 블럭을 확보한다.
    bool                            AllocRange(UINT count, UINT preferredCount, D3D12DescriptorHeapRegion *outSubregion);

    const D3D12DescriptorHeapRegion &GetRegion() const { return region; }

private:
    D3D12DescriptorHeapRegion       region = {};
    std::atomic<UINT>               allocatedCount{0}; // 전체 영역에서 블럭으로 배정한 디스크립터 수
};

// 디스크립터 힙의 일정 영역을 사용하는 풀.
// 스레드마다 별도 풀을 사용하고, 공유 할당기는 여러 스레드에서 공유되는 할당기이다.
// 현재 블럭의 잔여 공간이 부족할 때 공유 할당기에서 새 블럭을 확보한다.
class D3D12DescriptorRangePool {
    friend class D3D12Renderer;

public:
    D3D12DescriptorRangePool() = default;
    D3D12DescriptorRangePool(const D3D12DescriptorHeapRegion &region) { Init(region); }
    ~D3D12DescriptorRangePool() { Shutdown(); }

                                    // 고정 영역 모드로 초기화 (blockAllocator 가 필요없음)
    void                            Init(const D3D12DescriptorHeapRegion &region);

                                    // 공유 할당기 모드로 초기화 (blockAllocator 는 여러 스레드에서 공유되는 할당기)
    void                            Init(D3D12DescriptorRangeAllocator *blockAllocator, UINT blockSize);

    void                            Shutdown();

                                    // 현재 블럭에 남은 슬롯 수. 고정 영역 모드에서는 지정된 영역 전체가 한 블럭이다.
    UINT                            GetRemainingInBlock() const { return currentBlock.descriptorCount - currentBlockOffset; }
    UINT                            IsEmpty() const { return currentBlockOffset == 0; }

                                    // NOTE: 공유 할당기 모드에서는 blockAllocator 를 먼저 리셋한 뒤 호출해야 한다.
    void                            Reset();

                                    // 현재 블럭에서 count 만큼 연속된 디스크립터 슬롯을 할당한다. 부족하면 공유 할당기에서 새 블럭을 확보한다.
    bool                            AllocRange(UINT count, D3D12_CPU_DESCRIPTOR_HANDLE *outCpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE *outGpuDescriptorHandle);

    ID3D12DescriptorHeap *          GetDescriptorHeap() const { return currentBlock.descriptorHeap->GetDescriptorHeap(); }
    UINT                            GetHandleIncrement() const { return currentBlock.descriptorHeap->GetHandleIncrement(); }

private:
    D3D12DescriptorHeapRegion       currentBlock = {};
    UINT                            currentBlockOffset = 0; // 현재 블럭 내 다음 할당 위치
    UINT                            blockSize = 0;
    D3D12DescriptorRangeAllocator * blockAllocator = nullptr; // 여러 스레드에서 공유되는 할당기
};
