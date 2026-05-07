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

    Descriptor Table Pool

    - Shader-visible 디스크립터 힙을 관리하는 선형 할당 풀.
    - 바인딩 용 디스크립터 테이블 (연속된 핸들) 을 할당한다.
    - 개별 해제는 제공하지 않으며, Reset() 을 통해 전체 디스크립터 풀을 초기화하여 재사용한다.

---------------------------------------------------------------------------------------------------
*/

class D3D12DescriptorTablePool {
    friend class D3D12Renderer;

public:
    enum class Type : byte {
        CBV_SRV_UAV, // Common Resource
        Sampler
    };

    D3D12DescriptorTablePool() = default;
    D3D12DescriptorTablePool(ID3D12Device *device, Type type, UINT maxCount) { Init(device, type, maxCount); }
    ~D3D12DescriptorTablePool() { Shutdown(); }

    void                            Init(ID3D12Device *device, Type type, UINT maxCount);
    void                            Shutdown();

    UINT                            IsEmpty() const { return usedCount == 0; }
    void                            Reset() { usedCount = 0; }

    bool                            IsUsed() const { return usedCount > lastUsedCount; }
    void                            SaveUsedCount() { lastUsedCount = usedCount; }

                                    // 디스크립터 테이블 (연속된 디스크립터 핸들) 을 할당한다.
    bool                            AllocRange(UINT count, D3D12_CPU_DESCRIPTOR_HANDLE *outCpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE *outGpuDescriptorHandle);

    ID3D12DescriptorHeap *          GetDescriptorHeap() const { return descriptorHeap; }

private:
    ID3D12DescriptorHeap *          descriptorHeap = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE     baseCpuDescriptorHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE     baseGpuDescriptorHandle = {};
    UINT                            descriptorHandleSize = 0;
    UINT                            maxDescriptorCount = 0;
    UINT                            usedCount = 0;
    UINT                            lastUsedCount = 0;
};
