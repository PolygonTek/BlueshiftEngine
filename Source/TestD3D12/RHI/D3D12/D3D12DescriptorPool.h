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

class D3D12Renderer;

/*
---------------------------------------------------------------------------------------------------

    Descriptor Pool

    - Non-shader-visible 디스크립터 힙을 관리하는 디스크립터 풀
    - 디스크립터 단위 스테이징/보관 용도
    - IDAllocator 를 사용하여 디스크립터 힙 내에서 빈 슬롯을 thread-safe 하게 관리한다.

---------------------------------------------------------------------------------------------------
*/

class D3D12DescriptorPool {
    friend class D3D12Renderer;

public:
    enum class Type : byte {
        CBV_SRV_UAV,
        RTV,
        DSV,
        Sampler
    };

    D3D12DescriptorPool() = default;
    D3D12DescriptorPool(ID3D12Device *device, Type type, UINT maxCount, bool isGpuHeap) { Init(device, type, maxCount, isGpuHeap); }
    ~D3D12DescriptorPool() { Shutdown(); }

    void                            Init(ID3D12Device *device, Type type, UINT maxCount, bool isGpuHeap);
    void                            Shutdown();

                                    // 디스크립터 힙의 시작 CPU 핸들을 리턴 
    D3D12_CPU_DESCRIPTOR_HANDLE     GetCPUDescriptorHandleBase() const { return cpuDescriptorHandleBase; }
                                    // 디스크립터 힙의 시작 GPU 핸들을 리턴
    D3D12_GPU_DESCRIPTOR_HANDLE     GetGPUDescriptorHandleBase() const { return gpuDescriptorHandleBase; }

    void                            Clear();

    bool                            Alloc(D3D12_CPU_DESCRIPTOR_HANDLE *outCpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE *outGpuDescriptorHandle);
    void                            Free(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle);

    D3D12_CPU_DESCRIPTOR_HANDLE     AllocRange(int count);
    void                            FreeRange(const D3D12_CPU_DESCRIPTOR_HANDLE &descriptorHandle, int count);

                                    // 디스크립터 힙 내의 디스크립터들은 선형적으로 배치되므로 index 와 handle 을 서로 변환할 수 있다.
    uint32_t                        GetIndexFromCPUDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) const;
    uint32_t                        GetIndexFromGPUDescriptorHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle) const;
    D3D12_CPU_DESCRIPTOR_HANDLE     GetCPUDescriptorHandleFromIndex(uint32_t index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE     GetGPUDescriptorHandleFromIndex(uint32_t index) const;

private:
    uint32_t                        AllocIndex();
    void                            FreeIndex(uint32_t descriptorIndex);

    ID3D12DescriptorHeap *          descriptorHeap = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE     cpuDescriptorHandleBase = {};
    D3D12_GPU_DESCRIPTOR_HANDLE     gpuDescriptorHandleBase = {};
    UINT                            handleIncrement;
    UINT                            maxDescriptorCount;
    const char *                    heapTypeName = "Unknown";
    BE1::IDAllocator                idAllocator;
    BE1::Mutex                      mutex;
};
