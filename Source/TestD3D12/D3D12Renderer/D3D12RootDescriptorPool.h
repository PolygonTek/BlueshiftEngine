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

class D3D12RootDescriptorPool {
    friend class D3D12Renderer;

public:
    D3D12RootDescriptorPool() = default;
    D3D12RootDescriptorPool(ID3D12Device *device, UINT maxCount) { Init(device, maxCount); }
    ~D3D12RootDescriptorPool() { Shutdown(); }

    void                            Init(ID3D12Device *device, UINT maxCount);
    void                            Shutdown();

    void                            Reset();
    bool                            AllocRange(UINT count, D3D12_CPU_DESCRIPTOR_HANDLE *outCpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE *outGpuDescriptorHandle);

private:
    ID3D12DescriptorHeap *          descriptorHeap = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE     baseCpuDescriptorHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE     baseGpuDescriptorHandle;
    UINT                            descriptorHandleSize;
    UINT                            maxDescriptorCount;
    UINT                            usedCount = 0;
};
