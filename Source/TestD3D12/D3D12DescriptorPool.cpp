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
#include "D3D12App.h"
#include "D3D12DescriptorPool.h"

void D3D12DescriptorPool::Init(ID3D12Device5* device, UINT maxCount) {
    maxDescriptorCount = maxCount;
    usedCount = 0;

    srvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    descriptorHeapDesc.NumDescriptors = maxCount;
    descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));

    cpuDescriptorHandleForHeapStart = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    gpuDescriptorHandleForHeapStart = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
}

void D3D12DescriptorPool::Shutdown() {
    SAFE_RELEASE(descriptorHeap);
}

void D3D12DescriptorPool::Reset() {
    usedCount = 0;
}

bool D3D12DescriptorPool::AllocDescriptors(UINT descriptorCount, D3D12_CPU_DESCRIPTOR_HANDLE *outCpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE *outGpuDescriptorHandle) {
    if (usedCount + descriptorCount > maxDescriptorCount) {
        return false;
    }

    if (outCpuDescriptorHandle) {
        *outCpuDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuDescriptorHandleForHeapStart, usedCount, srvDescriptorSize);
    }
    if (outGpuDescriptorHandle) {
        *outGpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuDescriptorHandleForHeapStart, usedCount, srvDescriptorSize);
    }

    usedCount += descriptorCount;
    return true;
}

