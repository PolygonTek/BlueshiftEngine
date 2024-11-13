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

class D3D12SingleDescriptorAllocator {
public:
    ~D3D12SingleDescriptorAllocator() { Shutdown(); }

    void                            Init(UINT maxCount);
    void                            Shutdown();

    D3D12_CPU_DESCRIPTOR_HANDLE*    Alloc();
    void                            Free(const D3D12_CPU_DESCRIPTOR_HANDLE *cpuDescriptorHandle);

    ID3D12DescriptorHeap*           descriptorHeap = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE     baseDescriptorHandle;
    UINT                            descriptorHandleSize;
    UINT                            maxDescriptorCount;
    BE1::Array<CD3DX12_CPU_DESCRIPTOR_HANDLE*> descriptorHandleList;
};
