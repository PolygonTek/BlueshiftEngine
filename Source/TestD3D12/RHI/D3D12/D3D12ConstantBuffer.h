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
#include "D3D12Buffer.h"
#include <atomic>

class D3D12Renderer;
class D3D12FrameThreadData;
class D3D12DescriptorPool;

class D3D12ConstantBuffer : public RHI::ConstantBuffer {
    friend class D3D12Renderer;
    friend class D3D12FrameThreadData;

public:
    virtual ~D3D12ConstantBuffer() { Release(); }

    void                            Release();

    virtual void *                  GetNativeResource() const override { return GetResource(); }
    virtual const void *            GetNativeBufferObject() const override { return buffer; }

    ID3D12Resource *                GetResource() const { return buffer->GetResource(); }

private:
    D3D12Buffer *                   buffer = nullptr; // static 상수 버퍼가 소유하는 버퍼. dynamic 상수 버퍼라면 nullptr.
    D3D12_GPU_VIRTUAL_ADDRESS       gpuAddress = 0;
    D3D12DescriptorPool *           descriptorPool = nullptr;
    mutable std::atomic<D3D12_CPU_DESCRIPTOR_HANDLE> descriptorHandle{};
    uint32_t                        size = 0;
    int32_t                         dynamicBlockIndex = -1;
};
