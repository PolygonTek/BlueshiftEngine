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
#include "D3D12Renderer.h"
#include "D3D12DescriptorPool.h"

void D3D12DescriptorPool::Init(ID3D12Device *device, D3D12DescriptorPool::Type type, UINT maxDescriptorCount, bool isGpuHeap) {
    this->maxDescriptorCount = maxDescriptorCount;

    D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType = {};
    switch (type) {
    case Type::CBV_SRV_UAV:
        descriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapTypeName = "CBV_SRV_UAV";
        break;
    case Type::RTV:
        descriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        heapTypeName = "RTV";
        break;
    case Type::DSV:
        descriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        heapTypeName = "DSV";
        break;
    case Type::Sampler:
        descriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        heapTypeName = "Sampler";
        break;
    default:
        assert(0);
        break;
    }

    // 디스크립터 힙 타입에 대한 디스크립터 엔트리의 간격 (stride) 얻기 (보통은 32 바이트)
    // 디스크립터 핸들은 디스크립터 엔트리의 주소처럼 동작하는 값 (상대 주소 같은 개념, 역참조는 불가능) 을 가지고 있다.
    descriptorHandleSize = device->GetDescriptorHandleIncrementSize(descriptorHeapType);

    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    descriptorHeapDesc.NumDescriptors = maxDescriptorCount;
    descriptorHeapDesc.Type = descriptorHeapType;
    descriptorHeapDesc.Flags = isGpuHeap ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));

    // 디스크립터 힙을 만들 때 D3D12_DESCRIPTOR_HEAP_FLAG_NONE 이라면, 디스크립터 핸들은 CPU 에서만 접근 가능하게 된다.
    baseCpuDescriptorHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();

    if (isGpuHeap) {
        // GPU 에서 접근 가능한 힙일 때는 디스크립터의 GPU 핸들도 얻을 수 있다.
        // 힙은 하나지만, 접근할 때 CPU 에서 사용하는 핸들과 GPU 에서 사용하는 핸들로 나뉜다.
        // NOTE: CPU 접근 전용 힙일 때 GPU 핸들을 얻으려고 하면 즉, descriptorHeap->GetGPUDescriptorHandleForHeapStart() 를 호출하면 크래시 발생함.
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
    {
        BE1::ScopedLock lock(mutex);

        if (!idAllocator.AllocateID(newIndex)) {
            // TODO: 확장 가능하도록 수정할 것
            BE_WARNLOG("D3D12DescriptorPool::AllocIndex: no usable descriptors (type=%s, capacity=%u, heap=%p)\n",
                heapTypeName, maxDescriptorCount, static_cast<void *>(descriptorHeap));
        }
    }
    return newIndex;
}

void D3D12DescriptorPool::FreeIndex(uint32_t descriptorIndex) {
    BE1::ScopedLock lock(mutex);

    idAllocator.FreeID(descriptorIndex);
}

bool D3D12DescriptorPool::Alloc(D3D12_CPU_DESCRIPTOR_HANDLE *outCpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE *outGpuDescriptorHandle) {
    uint32_t newIndex = AllocIndex();
    if (newIndex == static_cast<uint32_t>(-1)) {
        if (outCpuDescriptorHandle) {
            *outCpuDescriptorHandle = {};
        }
        if (outGpuDescriptorHandle) {
            *outGpuDescriptorHandle = {};
        }
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

void D3D12DescriptorPool::Free(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle) {
    uint32_t descriptorIndex = GetIndexFromCPUDescriptorHandle(cpuDescriptorHandle);

    FreeIndex(descriptorIndex);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorPool::AllocRange(int count) {
    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = {};
    uint32_t newId = static_cast<uint32_t>(-1);
    {
        BE1::ScopedLock lock(mutex);

        if (!idAllocator.AllocateRange(newId, count)) {
            BE_WARNLOG("D3D12DescriptorPool::AllocRange: no usable consecutive descriptors (type=%s, capacity=%u, requested=%i, available=%u, heap=%p)\n",
                heapTypeName, maxDescriptorCount, count, static_cast<uint32_t>(idAllocator.GetAvailableCount()), static_cast<void *>(descriptorHeap));
            return descriptorHandle;
        }
    }

    descriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(baseCpuDescriptorHandle, (INT)newId, descriptorHandleSize);
    return descriptorHandle;
}

void D3D12DescriptorPool::FreeRange(const D3D12_CPU_DESCRIPTOR_HANDLE &descriptorHandle, int count) {
    uint32_t freeId = (uint32_t)(descriptorHandle.ptr - baseCpuDescriptorHandle.ptr) / descriptorHandleSize;
    {
        BE1::ScopedLock lock(mutex);

        idAllocator.FreeRange(freeId, count);
    }
}

uint32_t D3D12DescriptorPool::GetIndexFromCPUDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) const {
    assert(baseCpuDescriptorHandle.ptr);
    assert(baseCpuDescriptorHandle.ptr <= handle.ptr);
    uint32_t descriptorIndex = (uint32_t)(handle.ptr - baseCpuDescriptorHandle.ptr) / descriptorHandleSize;
    return descriptorIndex;
}

uint32_t D3D12DescriptorPool::GetIndexFromGPUDescriptorHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle) const {
    assert(baseGpuDescriptorHandle.ptr);
    assert(baseGpuDescriptorHandle.ptr <= handle.ptr);
    uint32_t descriptorIndex = (uint32_t)(handle.ptr - baseGpuDescriptorHandle.ptr) / descriptorHandleSize;
    return descriptorIndex;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorPool::GetCPUDescriptorHandleFromIndex(uint32_t index) const {
    assert(baseCpuDescriptorHandle.ptr);
    assert(index < maxDescriptorCount);

    return CD3DX12_CPU_DESCRIPTOR_HANDLE(baseCpuDescriptorHandle, index, descriptorHandleSize);
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorPool::GetGPUDescriptorHandleFromIndex(uint32_t index) const {
    assert(baseGpuDescriptorHandle.ptr);
    assert(index < maxDescriptorCount);

    return CD3DX12_GPU_DESCRIPTOR_HANDLE(baseGpuDescriptorHandle, index, descriptorHandleSize);
}
