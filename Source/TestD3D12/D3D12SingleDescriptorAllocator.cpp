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
#include "D3D12SingleDescriptorAllocator.h"

void D3D12SingleDescriptorAllocator::Init(UINT maxDescriptorCount) {
    this->maxDescriptorCount = maxDescriptorCount;

    D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    descriptorHandleSize = renderer.device->GetDescriptorHandleIncrementSize(descriptorHeapType);

    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    descriptorHeapDesc.NumDescriptors = maxDescriptorCount;
    descriptorHeapDesc.Type = descriptorHeapType;
    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    renderer.device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));

    // D3D12_DESCRIPTOR_HEAP_FLAG_NONE 타입은 CPU 쪽에만 힙을 만든다.
    // descriptorHeap->GetGPUDescriptorHandleForHeapStart() 를 호출하면 크래시 발생함
    baseDescriptorHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();

    descriptorAllocator.Init(maxDescriptorCount);
}

void D3D12SingleDescriptorAllocator::Shutdown() {
    SAFE_RELEASE(descriptorHeap);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12SingleDescriptorAllocator::Alloc() {
    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = {0};

    uint32_t newId;
    if (!descriptorAllocator.AllocateID(newId)) {
        BE_WARNLOG("D3D12SingleDescriptorAllocator::Alloc: no usable descriptor\n");
        return descriptorHandle;
    }

    descriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(baseDescriptorHandle, (INT)newId, descriptorHandleSize);
    return descriptorHandle;
}

void D3D12SingleDescriptorAllocator::Free(const D3D12_CPU_DESCRIPTOR_HANDLE &descriptorHandle) {
    uint32_t freeId = (uint32_t)(descriptorHandle.ptr - baseDescriptorHandle.ptr) / descriptorHandleSize;

    descriptorAllocator.FreeID(freeId);
}
