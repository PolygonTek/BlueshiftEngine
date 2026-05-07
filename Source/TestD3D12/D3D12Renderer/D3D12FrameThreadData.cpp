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

#include "Precompiled.h"
#include "D3D12Renderer.h"
#include "D3D12FrameThreadData.h"
#include "D3D12DescriptorPool.h"
#include "D3D12DescriptorTablePool.h"

static constexpr uint32_t DynamicBufferBlockSize = 1 << 22; // 4MB

D3D12FrameThreadData::DynamicBufferBlock::DynamicBufferBlock(uint64_t size) {
    // 블럭 단위로 관리할 다이나믹 버퍼 (업로드 버퍼) 를 생성한다.
    // CB/VB/IB 용으로 사용할 수 있다.
    buffer = static_cast<D3D12Buffer *>(RHI::renderer->CreateBuffer(RHI::BufferUsage::Upload,
        RHI::BufferFlag::ConstantBuffer |
        RHI::BufferFlag::VertexBuffer |
        RHI::BufferFlag::IndexBuffer,
        size, nullptr));

    // 다이나믹 버퍼는 항상 Map 해놓고 쓴다. (Pinned Memory)
    buffer->GetResource()->Map(0, nullptr, reinterpret_cast<void **>(&mappedBase));
}

D3D12FrameThreadData::DynamicBufferBlock::~DynamicBufferBlock() {
    RHI::renderer->DestroyBuffer(buffer);
}

RHI::FrameThreadData *D3D12Renderer::CreateFrameThreadData(int32_t threadIndex) {
    D3D12FrameThreadData *_frameThreadData = new D3D12FrameThreadData;
    _frameThreadData->Init(threadIndex);
    return _frameThreadData;
}

void D3D12Renderer::DestroyFrameThreadData(RHI::FrameThreadData *frameThreadData) {
    D3D12FrameThreadData *_frameThreadData = static_cast<D3D12FrameThreadData *>(frameThreadData);
    _frameThreadData->Shutdown();
    delete _frameThreadData;
}

void D3D12FrameThreadData::Init(int32_t threadIndex) {
    constexpr uint32_t maxPrimaryCommandLists = 8;
    constexpr uint32_t maxSecondaryCommandLists = 8;

    // Shader-visible CBV/SRV/UAV 디스크립터 테이블 풀을 생성한다.
    resDescriptorTablePool = new D3D12DescriptorTablePool(D3D12Renderer::GetRenderer()->device, D3D12DescriptorTablePool::Type::CBV_SRV_UAV, MaxResBindingDescriptors);

    // Shader-visible sampler 디스크립터 테이블 풀을 생성한다.
    samDescriptorTablePool = new D3D12DescriptorTablePool(D3D12Renderer::GetRenderer()->device, D3D12DescriptorTablePool::Type::Sampler, MaxSamBindingDescriptors);

    if (threadIndex == 0) {
        // 그래픽스 (Direct) 커맨드 리스트 풀을 생성한다.
        graphicsCommandListPool = new D3D12CommandListPool(&rootBindingData, D3D12_COMMAND_LIST_TYPE_DIRECT, maxPrimaryCommandLists, maxSecondaryCommandLists);

        // 컴퓨트 커맨드 리스트 풀을 생성한다.
        computeCommandListPool = new D3D12CommandListPool(&rootBindingData, D3D12_COMMAND_LIST_TYPE_COMPUTE, 8);
    } else {
        // 번들만 있는 그래픽스 (Direct) 커맨드 리스트 풀을 생성한다.
        graphicsCommandListPool = new D3D12CommandListPool(&rootBindingData, D3D12_COMMAND_LIST_TYPE_DIRECT, 0, maxSecondaryCommandLists);
    }

    dynamicBufferBlocks.SetGranularity(16);
    // 미리 다이나믹 버퍼 블럭을 1개만 생성한다.
    DynamicBufferBlock *dynamicBlock = new DynamicBufferBlock(DynamicBufferBlockSize);
    dynamicBlock->blockIndex = dynamicBufferBlocks.Append(dynamicBlock);

    // 다이나믹 버퍼에서 사용할 (CBV) 디스크립터 풀을 생성한다.
    dynamicResDescriptorPool = new D3D12DescriptorPool(D3D12Renderer::GetRenderer()->device, D3D12DescriptorPool::Type::CBV_SRV_UAV, 8192, false);

    dynamicConstantBuffers.SetGranularity(256);
    dynamicConstantBuffers.Reserve(256);

    dynamicVertexBuffers.SetGranularity(256);
    dynamicVertexBuffers.Reserve(256);

    dynamicIndexBuffers.SetGranularity(256);
    dynamicIndexBuffers.Reserve(256);
}

void D3D12FrameThreadData::Shutdown() {
    dynamicBufferBlocks.DeleteContents(false);

    dynamicResDescriptorPool->Clear();

    SAFE_DELETE(dynamicResDescriptorPool);
    SAFE_DELETE(graphicsCommandListPool);
    SAFE_DELETE(computeCommandListPool);

    SAFE_DELETE(resDescriptorTablePool);
    SAFE_DELETE(samDescriptorTablePool);
}

void D3D12FrameThreadData::Reset() {
    // 다이나믹 버퍼를 리셋한다.
    // TODO: 블럭의 개수가 너무 많아지면 실시간으로 블록 크기를 키워서 새로 만들어야 할 수도 있음
    for (DynamicBufferBlock *dynamicBlock : dynamicBufferBlocks) {
        dynamicBlock->usedBytes = 0;
    }

    // 이번에 프레임에 사용할 다이나믹 버퍼의 디스크립터들을 초기화
    dynamicResDescriptorPool->Clear();

    // 다이나믹 버퍼들도 비운다.
    dynamicConstantBuffers.SetCount(0, false);
    dynamicVertexBuffers.SetCount(0, false);
    dynamicIndexBuffers.SetCount(0, false);

    // 디스크립터 테이블 풀을 비운다.
    resDescriptorTablePool->Reset();
    samDescriptorTablePool->Reset();

    // 커맨드 리스트 풀을 비운다.
    graphicsCommandListPool->Clear();
    if (computeCommandListPool) {
        computeCommandListPool->Clear();
    }
}

D3D12FrameThreadData::DynamicBufferBlock *D3D12FrameThreadData::FindFreeDynamicBufferBlock(uint32_t size, uint32_t alignSize, uint32_t *outOffset) const {
    DynamicBufferBlock *currentBlock = nullptr;

    for (int blockIndex = 0; blockIndex < dynamicBufferBlocks.Count(); ++blockIndex) {
        currentBlock = dynamicBufferBlocks[blockIndex];

        uint32_t alignedOffset = BE1::AlignUp(currentBlock->usedBytes, alignSize);

        // 필요한 데이터의 크기가 현재 블럭의 크기를 넘어가지 않는다면, 이 블럭을 사용한다.
        if (alignedOffset + size <= currentBlock->buffer->GetSize()) {
            *outOffset = alignedOffset;

            return currentBlock;
        }
    }
    return nullptr;
}

RHI::VertexBuffer *D3D12FrameThreadData::AllocVertex(uint32_t vertexSize, uint32_t count, const void *data) {
    uint32_t size = vertexSize * count;

    uint32_t offset;
    DynamicBufferBlock *currentBlock = FindFreeDynamicBufferBlock(size, vertexSize, &offset);
    if (!currentBlock) {
        currentBlock = new DynamicBufferBlock(BE1::Max(size, DynamicBufferBlockSize));
        currentBlock->blockIndex = dynamicBufferBlocks.Append(currentBlock);

        offset = 0;
    }

    currentBlock->usedBytes = offset + size;

    // 현재 블럭의 버퍼 리소스를 쪼개서 VBV 를 만든다.
    D3D12VertexBuffer &dynamicVertexBuffer = dynamicVertexBuffers.Alloc();
    dynamicVertexBuffer.writePtr = reinterpret_cast<byte *>(currentBlock->mappedBase) + offset;
    dynamicVertexBuffer.buffer = currentBlock->buffer;
    dynamicVertexBuffer.vbv.BufferLocation = currentBlock->buffer->GetResource()->GetGPUVirtualAddress() + offset;
    dynamicVertexBuffer.vbv.SizeInBytes = size;
    dynamicVertexBuffer.vbv.StrideInBytes = vertexSize;
    dynamicVertexBuffer.dynamicBlockIndex = currentBlock->blockIndex;

    if (data) {
        BE1::simdProcessor->Memcpy(dynamicVertexBuffer.writePtr, data, size);
    }

    return &dynamicVertexBuffer;
}

RHI::IndexBuffer *D3D12FrameThreadData::AllocIndex(uint32_t indexSize, uint32_t count, const void *data) {
    uint32_t size = indexSize * count;

    uint32_t offset;
    DynamicBufferBlock *currentBlock = FindFreeDynamicBufferBlock(size, indexSize, &offset);
    if (!currentBlock) {
        currentBlock = new DynamicBufferBlock(BE1::Max(size, DynamicBufferBlockSize));
        currentBlock->blockIndex = dynamicBufferBlocks.Append(currentBlock);

        offset = 0;
    }

    currentBlock->usedBytes = offset + size;

    // 현재 블럭의 버퍼 리소스를 쪼개서 IBV 를 만든다.
    D3D12IndexBuffer &dynamicIndexBuffer = dynamicIndexBuffers.Alloc();
    dynamicIndexBuffer.writePtr = reinterpret_cast<byte *>(currentBlock->mappedBase) + offset;
    dynamicIndexBuffer.buffer = currentBlock->buffer;
    dynamicIndexBuffer.ibv.BufferLocation = currentBlock->buffer->GetResource()->GetGPUVirtualAddress() + offset;
    dynamicIndexBuffer.ibv.SizeInBytes = size;
    dynamicIndexBuffer.ibv.Format = (indexSize == sizeof(uint16_t) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
    dynamicIndexBuffer.dynamicBlockIndex = currentBlock->blockIndex;

    if (data) {
        BE1::simdProcessor->Memcpy(dynamicIndexBuffer.writePtr, data, size);
    }

    return &dynamicIndexBuffer;
}

RHI::ConstantBuffer *D3D12FrameThreadData::AllocConstant(uint32_t size, const void *data) {
    // 상수 버퍼의 사이즈는 D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT (256) 로 정렬되어야 한다.
    uint32_t alignedSize = BE1::AlignUp(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    // 상수 버퍼의 최대 크기는 64KB 이다.
    if (alignedSize > D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16) {
        BE_WARNLOG("Constant buffer size exceeds 64KB limit\n");
        return nullptr;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle = {};
    if (!dynamicResDescriptorPool->Alloc(&cpuDescriptorHandle, nullptr)) {
        return nullptr;
    }

    uint32_t offset;
    // 상수 버퍼의 오프셋은 D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT (256) 로 정렬되어야 한다.
    DynamicBufferBlock *currentBlock = FindFreeDynamicBufferBlock(alignedSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, &offset);
    if (!currentBlock) {
        currentBlock = new DynamicBufferBlock(BE1::Max(alignedSize, DynamicBufferBlockSize));
        currentBlock->blockIndex = dynamicBufferBlocks.Append(currentBlock);

        offset = 0;
    }

    currentBlock->usedBytes = offset + alignedSize;

    // 현재 블럭의 버퍼 리소스를 쪼개서 CBV 를 만든다.
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = currentBlock->buffer->GetResource()->GetGPUVirtualAddress() + offset;
    cbvDesc.SizeInBytes = alignedSize;

    D3D12Renderer::GetRenderer()->device->CreateConstantBufferView(&cbvDesc, cpuDescriptorHandle);

    D3D12ConstantBuffer &dynamicConstantBuffer = dynamicConstantBuffers.Alloc();
    dynamicConstantBuffer.writePtr = reinterpret_cast<byte *>(currentBlock->mappedBase) + offset;
    dynamicConstantBuffer.descriptorHandle = cpuDescriptorHandle;
    
    if (data) {
        BE1::simdProcessor->Memcpy(dynamicConstantBuffer.writePtr, data, size);
    }

    return &dynamicConstantBuffer;
}

void *D3D12FrameThreadData::AppendVertex(RHI::VertexBuffer *vertexBuffer, uint32_t vertexSize, uint32_t count, const void *data) {
    D3D12VertexBuffer *_vertexBuffer = static_cast<D3D12VertexBuffer *>(vertexBuffer);
    if (_vertexBuffer->dynamicBlockIndex == -1) {
        return nullptr;
    }

    DynamicBufferBlock *block = dynamicBufferBlocks[_vertexBuffer->dynamicBlockIndex];
    if (!block) {
        return nullptr;
    }

    uint32_t alignedOffset = BE1::AlignUp(block->usedBytes, vertexSize);
    uint32_t size = vertexSize * count;

    // 버퍼의 전체 크기와 비교하여, 추가 데이터를 수용할 수 있는지 검사
    if (alignedOffset + size > block->buffer->GetSize()) {
        return nullptr;
    }

    _vertexBuffer->vbv.SizeInBytes += size;

    block->usedBytes = alignedOffset + size;

    byte *destPtr = reinterpret_cast<byte *>(block->mappedBase) + alignedOffset;
    if (destPtr) {
        BE1::simdProcessor->Memcpy(destPtr, data, size);
    }
    return destPtr;
}

void *D3D12FrameThreadData::AppendIndex(RHI::IndexBuffer *indexBuffer, uint32_t indexSize, uint32_t count, const void *data) {
    D3D12IndexBuffer *_indexBuffer = static_cast<D3D12IndexBuffer *>(indexBuffer);
    if (_indexBuffer->dynamicBlockIndex == -1) {
        return nullptr;
    }

    DynamicBufferBlock *block = dynamicBufferBlocks[_indexBuffer->dynamicBlockIndex];
    if (!block) {
        return nullptr;
    }

    uint32_t alignedOffset = BE1::AlignUp(block->usedBytes, indexSize);
    uint32_t size = indexSize * count;

    // 버퍼의 전체 크기와 비교하여, 추가 데이터를 수용할 수 있는지 검사
    if (alignedOffset + size > block->buffer->GetSize()) {
        return nullptr;
    }

    _indexBuffer->ibv.SizeInBytes += size;

    block->usedBytes = alignedOffset + size;

    byte *destPtr = reinterpret_cast<byte *>(block->mappedBase) + alignedOffset;
    if (destPtr) {
        BE1::simdProcessor->Memcpy(destPtr, data, size);
    }
    return destPtr;
}

RHI::CommandList *D3D12FrameThreadData::BeginCommandList(RHI::CommandQueueType queueType) {
    D3D12CommandList *commandList = nullptr;
    if (queueType == RHI::CommandQueueType::Graphics) {
        commandList = static_cast<D3D12CommandList *>(AllocGraphicsCommandList(RHI::CommandListKind::Primary));
    } else {
        commandList = static_cast<D3D12CommandList *>(AllocComputeCommandList());
    }

    // command allocator 를 재사용하도록 리셋하고, command list 를 command allocator 를 이용하여 초기 상태로 리셋
    commandList->Reset();

    ID3D12DescriptorHeap *descriptorHeaps[] = {
        commandList->GetResDescriptorTablePool()->GetDescriptorHeap(),
        commandList->GetSamDescriptorTablePool()->GetDescriptorHeap()
    };
    commandList->SetDescriptorHeaps(COUNT_OF(descriptorHeaps), descriptorHeaps);

    return commandList;
}

RHI::CommandList *D3D12FrameThreadData::BeginSecondaryCommandList(const RHI::CommandList *primaryCommandList) {
    // secondary command list 를 얻어온다.
    D3D12CommandList *commandList = static_cast<D3D12CommandList *>(AllocGraphicsCommandList(RHI::CommandListKind::Secondary));

    // command allocator 를 재사용하도록 리셋하고, command list 를 command allocator 를 이용하여 초기 상태로 리셋
    commandList->Reset(true, primaryCommandList);

    // secondary command list 의 디스크립터 힙을 지정한다. (디스크립터 힙의 지정 상태는 primary command list 에서 자동으로 이어지지 않음)
    // 반드시 primary command list 와 동일한 디스크립터 힙을 사용해야 한다.
    ID3D12DescriptorHeap *descriptorHeaps[] = {
        commandList->GetResDescriptorTablePool()->GetDescriptorHeap(),
        commandList->GetSamDescriptorTablePool()->GetDescriptorHeap()
    };
    commandList->SetDescriptorHeaps(COUNT_OF(descriptorHeaps), descriptorHeaps);

    return commandList;
}
