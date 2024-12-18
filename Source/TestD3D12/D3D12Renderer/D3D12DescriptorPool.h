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

class D3D12DescriptorPool {
    friend class D3D12Renderer;

public:
    enum class Type : byte {
        SRV,
        RTV,
        DSV,
        Sampler
    };

    D3D12DescriptorPool() = default;
    D3D12DescriptorPool(ID3D12Device *device, Type type, UINT maxCount, bool isShaderVisible) { Init(device, type, maxCount, isShaderVisible); }
    ~D3D12DescriptorPool() { Shutdown(); }

    void                            Init(ID3D12Device *device, Type type, UINT maxCount, bool isShaderVisible);
    void                            Shutdown();

    void                            Clear();

    D3D12_CPU_DESCRIPTOR_HANDLE     Alloc();
    void                            Free(const D3D12_CPU_DESCRIPTOR_HANDLE& descriptorHandle);

    D3D12_CPU_DESCRIPTOR_HANDLE     AllocRange(int count);
    void                            FreeRange(const D3D12_CPU_DESCRIPTOR_HANDLE &descriptorHandle, int count);

private:
    ID3D12DescriptorHeap *          descriptorHeap = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE     baseDescriptorHandle;
    UINT                            descriptorHandleSize;
    UINT                            maxDescriptorCount;
    IDAllocator                     idAllocator;
};
