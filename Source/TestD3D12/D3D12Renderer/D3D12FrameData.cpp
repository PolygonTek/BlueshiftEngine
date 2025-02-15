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
#include "D3D12FrameData.h"
#include "D3D12DescriptorPool.h"
#include "D3D12RootDescriptorPool.h"

static constexpr uint32_t DynamicBufferBlockSize = 1 << 22; // 4MB

D3D12FrameThreadData::DynamicBlock::DynamicBlock(uint64_t size) {
    // 다이나믹 업로드 버퍼 생성
    // CBV/VBV/IBV/SRV 용으로 사용할 수 있다.
    buffer = static_cast<D3D12Buffer *>(RHI::renderer->CreateBuffer(RHI::BufferUsage::Upload,
        RHI::ResourceFlag::ConstantBuffer |
        RHI::ResourceFlag::VertexBuffer |
        RHI::ResourceFlag::IndexBuffer |
        RHI::ResourceFlag::ShaderResource |
        RHI::ResourceFlag::Typeless,
        size, BE1::Image::Format::Unknown, 0, nullptr));

    // 버퍼를 프로그램이 끝날 때 까지 Map 해놓고 쓴다. (Pinned) 
    buffer->GetResource()->Map(0, nullptr, reinterpret_cast<void **>(&mappedBase));
}

D3D12FrameThreadData::DynamicBlock::~DynamicBlock() {
    RHI::renderer->DestroyBuffer(buffer);
}

RHI::FrameThreadData *D3D12Renderer::CreateFrameThreadData() {
    D3D12FrameThreadData *d3d12FrameThreadData = new D3D12FrameThreadData;
    d3d12FrameThreadData->Init();
    return d3d12FrameThreadData;
}

void D3D12Renderer::DestroyFrameThreadData(RHI::FrameThreadData *frameThreadData) {
    D3D12FrameThreadData *d3d12FrameThreadData = static_cast<D3D12FrameThreadData *>(frameThreadData);
    d3d12FrameThreadData->Shutdown();
    delete d3d12FrameThreadData;
}

void D3D12FrameThreadData::Init() {
    uint32_t maxSecondaryCommandLists = 8;

    // 그래픽스 커맨드 리스트 풀을 생성한다.
    graphicsCommandListPool = new D3D12CommandListPool(this, D3D12_COMMAND_LIST_TYPE_DIRECT, 8, maxSecondaryCommandLists);

    // 컴퓨트 커맨드 리스트 풀을 생성한다.
    computeCommandListPool = new D3D12CommandListPool(this, D3D12_COMMAND_LIST_TYPE_COMPUTE, 8);

    // Shader-visible CBV/SRV/UAV 디스크립터 힙을 생성한다.
    // 공식 문서에 따르면, CBV/SRV/UAV 의 디스크립터 개수는 HW Tier 2+ 에서 100 만개로 되어있다. 32 스레드 기준 스레드 당 약 31250 개가 된다.
    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/hardware-support
    resRootDescriptorPool = new D3D12RootDescriptorPool(D3D12Renderer::GetRenderer()->device, D3D12RootDescriptorPool::Type::CBV_SRV_UAV, 30000);
    // Shader-visible sampler 디스크립터 힙을 생성한다.
    // 공식 문서에 따르면, Sampler 의 디스크립터 개수는 HW Tier 2+ 에서 2048 개로 되어있다. 32 스레드 기중 스레드 당 약 64 개가 된다.
    samRootDescriptorPool = new D3D12RootDescriptorPool(D3D12Renderer::GetRenderer()->device, D3D12RootDescriptorPool::Type::Sampler, 64);

    dynamicBlocks.SetGranularity(16);
    // 미리 다이나믹 버퍼 블럭을 미리 1개 생성한다.
    DynamicBlock *dynamicBlock = new DynamicBlock(DynamicBufferBlockSize);
    dynamicBlock->blockIndex = dynamicBlocks.Append(dynamicBlock);

    // 다이나믹 버퍼에서 사용할 (CBV, SRV) 디스크립터 풀을 생성한다.
    dynamicDescriptorPool = new D3D12DescriptorPool(D3D12Renderer::GetRenderer()->device, D3D12DescriptorPool::Type::CBV_SRV_UAV, 8192, false);

    dynamicDescriptorHandles.SetGranularity(2048);
    dynamicDescriptorHandles.Reserve(4096);

    dynamicConstantBuffers.SetGranularity(256);
    dynamicConstantBuffers.Reserve(256);

    dynamicVertexBuffers.SetGranularity(256);
    dynamicVertexBuffers.Reserve(256);

    dynamicIndexBuffers.SetGranularity(256);
    dynamicIndexBuffers.Reserve(256);

    dynamicBuffers.SetGranularity(256);
    dynamicBuffers.Reserve(256);
}

void D3D12FrameThreadData::Shutdown() {
    dynamicBlocks.DeleteContents(false);

    dynamicDescriptorPool->Clear();
    dynamicDescriptorHandles.SetCount(0, false);

    SAFE_DELETE(dynamicDescriptorPool);
    SAFE_DELETE(resRootDescriptorPool);
    SAFE_DELETE(samRootDescriptorPool);
    SAFE_DELETE(graphicsCommandListPool);
    SAFE_DELETE(computeCommandListPool);
}

void D3D12FrameThreadData::Reset() {
    // 다이나믹 버퍼를 리셋한다.
    // TODO: 블럭의 개수가 너무 많아지면 실시간으로 블록 크기 키워서 새로 만들어야 할 수도 있음
    for (DynamicBlock *dynamicBlock : dynamicBlocks) {
        dynamicBlock->usedBytes = 0;
    }

    // 이번에 프레임에 사용할 다이나믹 버퍼의 디스크립터들을 초기화
    dynamicDescriptorPool->Clear();
    dynamicDescriptorHandles.SetCount(0, false);

    // 다이나믹 버퍼들도 비운다.
    dynamicConstantBuffers.SetCount(0, false);
    dynamicVertexBuffers.SetCount(0, false);
    dynamicIndexBuffers.SetCount(0, false);
    dynamicBuffers.SetCount(0, false);

    // 루트 디스크립터 풀을 비운다.
    resRootDescriptorPool->Reset();
    samRootDescriptorPool->Reset();

    // 커맨드 리스트 풀을 비운다.
    graphicsCommandListPool->Clear();
}

D3D12FrameThreadData::DynamicBlock *D3D12FrameThreadData::FindFreeDynamicBlock(uint32_t size, uint32_t alignSize, uint32_t *outOffset) const {
    DynamicBlock *currentBlock = nullptr;

    for (int blockIndex = 0; blockIndex < dynamicBlocks.Count(); ++blockIndex) {
        currentBlock = dynamicBlocks[blockIndex];

        uint32_t alignedOffset = BE1::AlignUp(currentBlock->usedBytes, alignSize);

        // 필요한 데이터의 크기가 현재 다이나믹 버퍼 블럭의 크기를 넘어가지 않는다면, 이 블럭을 사용한다.
        if (alignedOffset + size <= currentBlock->buffer->GetSize()) {
            *outOffset = alignedOffset;

            return currentBlock;
        }
    }
    return nullptr;
}

RHI::ConstantBuffer *D3D12FrameThreadData::AllocConstant(uint32_t size, const void *data) {
    // 상수 버퍼 뷰의 사이즈는 D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT (256) 로 정렬되어야 한다.
    uint32_t alignedSize = BE1::AlignUp(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    // 상수 버퍼 뷰의 최대 크기는 64kb 이다.
    if (alignedSize > D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16) {
        BE_WARNLOG("Constant buffer view size cannot exceeds 64KB limit\n");
        return nullptr;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = {};
    if (!dynamicDescriptorPool->Alloc(&descriptorHandle, nullptr)) {
        return nullptr;
    }

    uint32_t offset;
    // 상수 버퍼 뷰의 오프셋은 D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT (256) 로 정렬되어야 한다.
    DynamicBlock *currentBlock = FindFreeDynamicBlock(alignedSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, &offset);
    if (!currentBlock) {
        currentBlock = new DynamicBlock(BE1::Max(alignedSize, DynamicBufferBlockSize));
        currentBlock->blockIndex = dynamicBlocks.Append(currentBlock);

        offset = 0;
    }

    // 버퍼 리소스를 쪼개서 CBV 를 만들어 사용한다.
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = currentBlock->buffer->GetResource()->GetGPUVirtualAddress() + offset;
    cbvDesc.SizeInBytes = alignedSize;

    D3D12Renderer::GetRenderer()->device->CreateConstantBufferView(&cbvDesc, descriptorHandle);
    dynamicDescriptorHandles.Append(descriptorHandle);

    D3D12ConstantBuffer dynamicConstantBuffer;
    dynamicConstantBuffer.writePtr = reinterpret_cast<byte *>(currentBlock->mappedBase) + offset;
    dynamicConstantBuffer.descriptorHandle = descriptorHandle;
    dynamicConstantBuffers.Append(dynamicConstantBuffer);

    currentBlock->usedBytes = offset + alignedSize;

    if (data) {
        BE1::simdProcessor->MemcpyStream(dynamicConstantBuffer.writePtr, data, size);
    }

    return &dynamicConstantBuffers.Last();
}

RHI::VertexBuffer *D3D12FrameThreadData::AllocVertex(uint32_t vertexSize, uint32_t count, const void *data) {
    uint32_t size = vertexSize * count;

    uint32_t alignedOffset;
    DynamicBlock *currentBlock = FindFreeDynamicBlock(size, vertexSize, &alignedOffset);
    if (!currentBlock) {
        currentBlock = new DynamicBlock(BE1::Max(size, DynamicBufferBlockSize));
        currentBlock->blockIndex = dynamicBlocks.Append(currentBlock);

        alignedOffset = 0;
    }

    // 버퍼 리소스를 쪼개서 VBV 를 만들어 사용한다.
    D3D12VertexBuffer dynamicVertexBuffer;
    dynamicVertexBuffer.writePtr = reinterpret_cast<byte *>(currentBlock->mappedBase) + alignedOffset;
    dynamicVertexBuffer.buffer = currentBlock->buffer;
    dynamicVertexBuffer.vbv.BufferLocation = currentBlock->buffer->GetResource()->GetGPUVirtualAddress() + alignedOffset;
    dynamicVertexBuffer.vbv.SizeInBytes = size;
    dynamicVertexBuffer.vbv.StrideInBytes = vertexSize;
    dynamicVertexBuffer.dynamicBlockIndex = currentBlock->blockIndex;
    dynamicVertexBuffers.Append(dynamicVertexBuffer);

    currentBlock->usedBytes = alignedOffset + size;

    if (data) {
        BE1::simdProcessor->MemcpyStream(dynamicVertexBuffer.writePtr, data, size);
    }

    return &dynamicVertexBuffers.Last();
}

RHI::IndexBuffer *D3D12FrameThreadData::AllocIndex(uint32_t indexSize, uint32_t count, const void *data) {
    uint32_t size = indexSize * count;

    uint32_t offset;
    DynamicBlock *currentBlock = FindFreeDynamicBlock(size, indexSize, &offset);
    if (!currentBlock) {
        currentBlock = new DynamicBlock(BE1::Max(size, DynamicBufferBlockSize));
        currentBlock->blockIndex = dynamicBlocks.Append(currentBlock);

        offset = 0;
    }

    // 버퍼 리소스를 쪼개서 IBV 를 만들어 사용한다.
    D3D12IndexBuffer dynamicIndexBuffer;
    dynamicIndexBuffer.writePtr = reinterpret_cast<byte *>(currentBlock->mappedBase) + offset;
    dynamicIndexBuffer.buffer = currentBlock->buffer;
    dynamicIndexBuffer.ibv.BufferLocation = currentBlock->buffer->GetResource()->GetGPUVirtualAddress() + offset;
    dynamicIndexBuffer.ibv.SizeInBytes = size;
    dynamicIndexBuffer.ibv.Format = (indexSize == sizeof(uint16_t) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
    dynamicIndexBuffer.dynamicBlockIndex = currentBlock->blockIndex;
    dynamicIndexBuffers.Append(dynamicIndexBuffer);

    currentBlock->usedBytes = offset + size;

    if (data) {
        BE1::simdProcessor->MemcpyStream(dynamicIndexBuffer.writePtr, data, size);
    }

    return &dynamicIndexBuffers.Last();
}

RHI::Buffer *D3D12FrameThreadData::AllocBuffer(BE1::Image::Format format, uint32_t structureByteStride, uint32_t count, const void *data) {
    // structureByteStride 는 4 의 배수 정렬 & 2048 보다 작아야 한다.
    assert(BE1::IsAligned(structureByteStride, 4) && structureByteStride < 2048);

    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = {};
    if (!dynamicDescriptorPool->Alloc(&descriptorHandle, nullptr)) {
        return nullptr;
    }

    uint32_t stride = format == BE1::Image::Format::Unknown ? (structureByteStride == 0 ? 4 : structureByteStride) : BE1::Image::BytesPerPixel(format);
    uint32_t size = stride * count;

    uint32_t offset;
    DynamicBlock *currentBlock = FindFreeDynamicBlock(size, stride, &offset);
    if (!currentBlock) {
        currentBlock = new DynamicBlock(BE1::Max(size, DynamicBufferBlockSize));
        currentBlock->blockIndex = dynamicBlocks.Append(currentBlock);

        offset = 0;
    }

    // 버퍼 리소스를 쪼개서 SRV 를 만들어 사용한다.
    D3D12SRVDescriptor srvDescriptor;
    srvDescriptor.srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDescriptor.srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptor.srvDesc.Buffer.FirstElement = offset / stride;
    srvDescriptor.srvDesc.Buffer.NumElements = count;

    if (format == BE1::Image::Format::Unknown) {
        srvDescriptor.srvDesc.Buffer.StructureByteStride = structureByteStride;

        if (structureByteStride == 0) {
            // Raw buffer (4 바이트 정렬된, 바이트 단위 접근이 가능한 버퍼)
            srvDescriptor.srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            srvDescriptor.srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
        } else {
            // Structured buffer
            srvDescriptor.srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        }
    } else {
        // Typed buffer
        // 버퍼의 format 에 따라 swizzling 이 필요할 수도 있다.
        srvDescriptor.srvDesc.Shader4ComponentMapping = D3D12Renderer::GetComponentSwizzling(format);

        D3D12Renderer::ImageFormatToDXGIFormat(format, false, &srvDescriptor.srvDesc.Format);
    }

    D3D12Renderer::GetRenderer()->device->CreateShaderResourceView(currentBlock->buffer->GetResource(), &srvDescriptor.srvDesc, descriptorHandle);
    dynamicDescriptorHandles.Append(descriptorHandle);

    D3D12Buffer dynamicBuffer;
    dynamicBuffer.writePtr = reinterpret_cast<byte *>(currentBlock->mappedBase) + offset;
    dynamicBuffer.srvDescriptor = srvDescriptor;
    dynamicBuffer.size = size;
    dynamicBuffer.structureByteStride = structureByteStride;
    dynamicBuffer.format = format;
    dynamicBuffer.dynamicBlockIndex = currentBlock->blockIndex;
    dynamicBuffers.Append(dynamicBuffer);

    currentBlock->usedBytes = offset + size;

    if (data) {
        BE1::simdProcessor->MemcpyStream(dynamicBuffer.writePtr, data, size);
    }

    return &dynamicBuffers.Last();
}

void *D3D12FrameThreadData::AppendVertex(RHI::VertexBuffer *vertexBuffer, uint32_t vertexSize, uint32_t count, const void *data) {
    D3D12VertexBuffer *d3d12VertexBuffer = static_cast<D3D12VertexBuffer *>(vertexBuffer);
    if (d3d12VertexBuffer->dynamicBlockIndex == -1) {
        return nullptr;
    }

    DynamicBlock *block = dynamicBlocks[d3d12VertexBuffer->dynamicBlockIndex];
    if (!block) {
        return nullptr;
    }

    uint32_t alignedOffset = BE1::AlignUp(block->usedBytes, vertexSize);
    uint32_t size = vertexSize * count;

    // 버퍼의 전체 크기와 비교하여, 추가 데이터를 수용할 수 있는지 검사
    if (alignedOffset + size > block->buffer->GetSize()) {
        return nullptr;
    }

    d3d12VertexBuffer->vbv.SizeInBytes += size;

    block->usedBytes = alignedOffset + size;

    byte *destPtr = reinterpret_cast<byte *>(block->mappedBase) + alignedOffset;
    if (destPtr) {
        BE1::simdProcessor->MemcpyStream(destPtr, data, size);
    }
    return destPtr;
}

void *D3D12FrameThreadData::AppendIndex(RHI::IndexBuffer *indexBuffer, uint32_t indexSize, uint32_t count, const void *data) {
    D3D12IndexBuffer *d3d12IndexBuffer = static_cast<D3D12IndexBuffer *>(indexBuffer);
    if (d3d12IndexBuffer->dynamicBlockIndex == -1) {
        return nullptr;
    }

    DynamicBlock *block = dynamicBlocks[d3d12IndexBuffer->dynamicBlockIndex];
    if (!block) {
        return nullptr;
    }

    uint32_t alignedOffset = BE1::AlignUp(block->usedBytes, indexSize);
    uint32_t size = indexSize * count;

    // 버퍼의 전체 크기와 비교하여, 추가 데이터를 수용할 수 있는지 검사
    if (alignedOffset + size > block->buffer->GetSize()) {
        return nullptr;
    }

    d3d12IndexBuffer->ibv.SizeInBytes += size;

    block->usedBytes = alignedOffset + size;

    byte *destPtr = reinterpret_cast<byte *>(block->mappedBase) + alignedOffset;
    if (destPtr) {
        BE1::simdProcessor->MemcpyStream(destPtr, data, size);
    }
    return destPtr;
}

RHI::CommandList *D3D12FrameThreadData::BeginCommandList(RHI::CommandQueueType queueType) {
    D3D12CommandList *commandList = nullptr;
    if (queueType == RHI::CommandQueueType::Graphics) {
        commandList = static_cast<D3D12CommandList *>(AllocGraphicsCommandList(RHI::CommandListType::Primary));
    } else {
        commandList = static_cast<D3D12CommandList *>(AllocComputeCommandList());
    }

    // CommandAllocator 를 재사용하도록 리셋하고, CommandList 를 CommandAllocator 를 이용하여 초기 상태로 리셋
    commandList->Reset();

    ID3D12DescriptorHeap *descriptorHeaps[] = {
        resRootDescriptorPool->GetDescriptorHeap(),
        samRootDescriptorPool->GetDescriptorHeap()
    };
    commandList->SetDescriptorHeaps(COUNT_OF(descriptorHeaps), descriptorHeaps);

    return commandList;
}

RHI::CommandList *D3D12FrameThreadData::BeginSecondaryCommandList(const RHI::CommandList *primaryCommandList) {
    // Secondary CommandList 를 얻어온다.
    D3D12CommandList *commandList = static_cast<D3D12CommandList *>(AllocGraphicsCommandList(RHI::CommandListType::Secondary));

    // CommandAllocator 를 재사용하도록 리셋하고, CommandList 를 CommandAllocator 를 이용하여 초기 상태로 리셋
    commandList->Reset(true, primaryCommandList);

    // Secondary CommandList 의 루트 디스크립터 힙을 지정한다.
    // 반드시 Primary CommandList 와 동일한 디스크립터 힙을 사용해야 한다.
    ID3D12DescriptorHeap *descriptorHeaps[] = {
        resRootDescriptorPool->GetDescriptorHeap(),
        samRootDescriptorPool->GetDescriptorHeap()
    };
    commandList->SetDescriptorHeaps(COUNT_OF(descriptorHeaps), descriptorHeaps);

    return commandList;
}
