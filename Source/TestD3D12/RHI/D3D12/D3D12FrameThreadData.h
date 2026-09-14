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

#include "../RHI.h"
#include "D3D12ConstantBuffer.h"
#include "D3D12VertexBuffer.h"
#include "D3D12IndexBuffer.h"
#include "D3D12CommandList.h"
#include "D3D12CommandListPool.h"
#include "D3D12ShaderVisibleDescriptorHeap.h"
#include "D3D12RootBindingData.h"

class D3D12DescriptorPool;
class D3D12Renderer;

/*
---------------------------------------------------------------------------------------------------

    Frame Thread Data

    한 프레임 안에서 필요한 렌더링 태스크 스레드들의 데이터를 관리한다.
    스레드 간 동기화를 피하기 위해 태스크 스레드 당 하나씩 생성된다.

    1. 다이나믹 버퍼 (프런트엔드/백엔드 모두 사용)
    2. 커맨드 리스트 풀 (백엔드에서만 사용)
        * 0번 스레드 데이터만 프라이머리 커맨드 리스트와 컴퓨트 커맨드 리스트 풀을 갖는다.
        * 나머지 스레드 데이터들은 세컨더리 커맨드 리스트만 갖는다.
    3. 루트 파라미터들의 바인딩 데이터

---------------------------------------------------------------------------------------------------
*/

class D3D12FrameThreadData : public RHI::FrameThreadData {
public:
    void                            Init(D3D12Renderer *renderer, int32_t frameIndex, int32_t numFrames, int32_t threadIndex, int32_t numRenderTaskThreads);
    void                            Shutdown();

                                    // 프레임이 초기화 될 때 호출되어야 한다.
    virtual void                    Reset() override;

                                    // 정렬 패딩을 포함한 다이나믹 버퍼 사용량을 반환한다.
    virtual uint64_t                GetUsedBytes() const override;

                                    // 다이나믹 버텍스 버퍼를 할당한다. Reset 이 호출되면 할당된 버퍼는 모두 해제된다.
    virtual RHI::VertexBuffer *     AllocVertex(uint32_t vertexSize, uint32_t count, const void *data = nullptr) override;

                                    // 다이나믹 인덱스 버퍼를 할당한다. Reset 이 호출되면 할당된 버퍼는 모두 해제된다.
    virtual RHI::IndexBuffer *      AllocIndex(uint32_t indexSize, uint32_t count, const void *data = nullptr) override;

                                    // 다이나믹 컨스턴트 버퍼를 할당한다. Reset 이 호출되면 할당된 버퍼는 모두 해제된다.
    virtual RHI::ConstantBuffer *   AllocConstant(uint32_t size, const void *data = nullptr) override;

                                    // 다이나믹 (raw) 버퍼를 할당한다. Reset 이 호출되면 할당된 버퍼는 모두 해제된다.
    virtual RHI::Buffer *           AllocRawBuffer(uint32_t size, uint32_t alignSize, const void *data = nullptr) override;

                                    // 다이나믹 (structured) 버퍼를 할당한다. Reset 이 호출되면 할당된 버퍼는 모두 해제된다.
    virtual RHI::Buffer *           AllocStructuredBuffer(uint32_t stride, uint32_t count, const void *data = nullptr) override;

                                    // 마지막에 할당했던 다이나믹 버텍스 버퍼에 데이터를 추가한다. 블럭의 크기가 부족하지 않은 이상 추가는 성공한다.
    virtual void *                  AppendVertex(RHI::VertexBuffer *vertexBuffer, uint32_t vertexSize, uint32_t count, const void *data = nullptr) override;

                                    // 마지막에 할당했던 다이나믹 인덱스 버퍼에 데이터를 추가한다. 블럭의 크기가 부족하지 않은 이상 추가는 성공한다.
    virtual void *                  AppendIndex(RHI::IndexBuffer *indexBuffer, uint32_t indexSize, uint32_t count, const void *data = nullptr) override;

    virtual RHI::CommandList *      AllocGraphicsCommandList(RHI::CommandListKind kind = RHI::CommandListKind::Primary) override;
    virtual RHI::CommandList *      AllocComputeCommandList() override;

    virtual RHI::CommandList *      BeginCommandList(RHI::CommandQueueType queueType) override;
    virtual RHI::CommandList *      BeginSecondaryCommandList() override;

private:
    // 다이나믹 버퍼를 블럭 단위로 관리하기 위한 블럭
    class DynamicBufferBlock {
    public:
        DynamicBufferBlock() = delete;
        DynamicBufferBlock(uint64_t size);
        ~DynamicBufferBlock();

        D3D12Buffer *               buffer = nullptr;
        UINT                        usedBytes = 0;
        int                         blockIndex = -1;
    };

    RHI::Buffer *                   AllocShaderResourceBuffer(uint32_t size, uint32_t alignSize, uint32_t structuredStride, const void *data = nullptr);
    DynamicBufferBlock *            FindFreeDynamicBufferBlock(uint32_t size, uint32_t alignSize, uint32_t *outAlignedOffset) const;

    BE1::Array<DynamicBufferBlock *> dynamicBufferBlocks;
    BE1::DynamicObjectAllocator<D3D12ConstantBuffer, 1024> dynamicConstantBuffers;
    BE1::DynamicObjectAllocator<D3D12VertexBuffer, 1024> dynamicVertexBuffers;
    BE1::DynamicObjectAllocator<D3D12IndexBuffer, 1024> dynamicIndexBuffers;
    BE1::DynamicObjectAllocator<D3D12Buffer, 1024> dynamicBuffers;
    D3D12DescriptorPool *           dynamicResDescriptorPool = nullptr;

    D3D12CommandListPool *          graphicsCommandListPool = nullptr;
    D3D12CommandListPool *          computeCommandListPool = nullptr;
    D3D12RootBindingData            rootBindingData = {}; // 루트 파라미터들에 대한 바인딩 리소스 정보
    D3D12DescriptorRangePool *      resourceDescriptorRangePool = nullptr; // Common resource (CBV/SRV/UAV) 디스크립터 테이블 풀
    D3D12DescriptorRangePool *      samplerDescriptorRangePool = nullptr; // Sampler 디스크립터 테이블 풀

};

BE_INLINE RHI::CommandList *D3D12FrameThreadData::AllocGraphicsCommandList(RHI::CommandListKind kind) {
    D3D12CommandList *commandList = graphicsCommandListPool->Alloc(kind);

    // 커맨드 리스트에서 사용할 디스크립터 레인지 풀을 지정한다.
    commandList->SetDescriptorTablePools(resourceDescriptorRangePool, samplerDescriptorRangePool);
    return commandList;
}

BE_INLINE RHI::CommandList *D3D12FrameThreadData::AllocComputeCommandList() {
    D3D12CommandList *commandList = computeCommandListPool->Alloc();

    // 커맨드 리스트에서 사용할 디스크립터 레인지 풀을 지정한다.
    commandList->SetDescriptorTablePools(resourceDescriptorRangePool, samplerDescriptorRangePool);
    return commandList;
}
