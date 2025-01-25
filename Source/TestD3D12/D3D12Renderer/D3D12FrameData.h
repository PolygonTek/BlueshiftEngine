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

#include "../RHI.h"
#include "D3D12ConstantBuffer.h"
#include "D3D12VertexBuffer.h"
#include "D3D12IndexBuffer.h"
#include "D3D12CommandList.h"
#include "D3D12CommandListPool.h"

class D3D12DescriptorPool;
class D3D12RootDescriptorPool;

class D3D12DynamicAllocation {
public:
    D3D12DynamicAllocation(uint64_t size);
    ~D3D12DynamicAllocation();

    D3D12Buffer *                       buffer = nullptr;
    void *                              mappedBase = nullptr;
    UINT                                usedBytes = 0;
};

class D3D12FrameThreadData : public RHI::FrameThreadData {
public:
    void                                Init();
    void                                Shutdown();

    virtual void                        Reset() override;

                                        // 프레임 별로 임시로 할당하는 다이나믹 버퍼
                                        // RTV/DSV 는 지원하지 않음
    virtual RHI::ConstantBuffer *       AllocConstant(uint32_t size) override;
    virtual RHI::VertexBuffer *         AllocVertex(uint32_t vertexSize, uint32_t count) override;
    virtual RHI::IndexBuffer *          AllocIndex(uint32_t indexSize, uint32_t count) override;
    virtual RHI::Buffer *               AllocBuffer(bool shaderStorage, BE1::Image::Format format, uint32_t structureByteStride, uint32_t count) override;

    virtual RHI::CommandList *          AllocGraphicsCommandList(RHI::CommandListType type = RHI::CommandListType::Primary) override;

    virtual RHI::CommandList *          BeginSecondaryCommandList(const RHI::CommandList *primaryCommandList) override;

    static constexpr uint32_t           MaxRootParameters = 64;

    D3D12CommandListPool *              graphicsCommandListPool = nullptr;
    D3D12CommandListPool *              computeCommandListPool = nullptr;

    BE1::Array<D3D12DynamicAllocation *> dynamicAllocations;
    BE1::Array<D3D12ConstantBuffer>     dynamicConstantBuffers;
    BE1::Array<D3D12VertexBuffer>       dynamicVertexBuffers;
    BE1::Array<D3D12IndexBuffer>        dynamicIndexBuffers;
    BE1::Array<D3D12Buffer>             dynamicBuffers;
    D3D12DescriptorPool *               dynamicDescriptorPool = nullptr;
    BE1::Array<D3D12_CPU_DESCRIPTOR_HANDLE> dynamicDescriptorHandles;

    D3D12RootDescriptorPool *           rootDescriptorPool = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE         tableCpuDescriptorHandles[MaxRootParameters][64] = { CD3DX12_CPU_DESCRIPTOR_HANDLE() };
    D3D12_GPU_DESCRIPTOR_HANDLE         tableGpuDescriptorStarts[MaxRootParameters] = { CD3DX12_GPU_DESCRIPTOR_HANDLE() };
    const RHI::GPUResource *            cbvResources[16] = {};
    const RHI::GPUResource *            srvResources[128] = {};
    const RHI::GPUResource *            uavResources[8] = {};
    uint32_t                            rootConstants[64] = {};
};

BE_INLINE RHI::CommandList *D3D12FrameThreadData::AllocGraphicsCommandList(RHI::CommandListType type) {
    return graphicsCommandListPool->Alloc(type);
}
