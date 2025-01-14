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
#include "D3D12RootDescriptorPool.h"

void D3D12RootDescriptorPool::Init(ID3D12Device *device, UINT maxDescriptorCount) {
    this->maxDescriptorCount = maxDescriptorCount;
    this->usedCount = 0;

    D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    descriptorHandleSize = device->GetDescriptorHandleIncrementSize(descriptorHeapType);

    // 미리 크게 할당해 놓고, 순차적으로 사용할 예정
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    descriptorHeapDesc.NumDescriptors = maxDescriptorCount;
    descriptorHeapDesc.Type = descriptorHeapType;
    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));

    baseCpuDescriptorHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    baseGpuDescriptorHandle = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
}

void D3D12RootDescriptorPool::Shutdown() {
    SAFE_RELEASE(descriptorHeap);
}

// 중간에 디스크립터를 반납할 일이 없으므로 IDAllocator 를 안쓰고, 통으로 사용한다.
// 한 프레임이 끝나면 풀에 있는 전체 디스크립터 핸들을 지운다.
bool D3D12RootDescriptorPool::AllocRange(UINT count, D3D12_CPU_DESCRIPTOR_HANDLE *outCpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE *outGpuDescriptorHandle) {
    if (usedCount + count > maxDescriptorCount) {
        BE_WARNLOG("D3D12DescriptorPool::AllocDescriptors: exceeds max descriptor count\n");
        return false;
    }

    if (outCpuDescriptorHandle) {
        *outCpuDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(baseCpuDescriptorHandle, usedCount, descriptorHandleSize);
    }
    if (outGpuDescriptorHandle) {
        *outGpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(baseGpuDescriptorHandle, usedCount, descriptorHandleSize);
    }

    usedCount += count;
    return true;
}
