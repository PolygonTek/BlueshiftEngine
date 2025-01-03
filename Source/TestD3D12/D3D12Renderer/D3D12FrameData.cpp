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

#include "Precompiled.h"
#include "D3D12Renderer.h"
#include "D3D12ConstantBuffer.h"
#include "D3D12FrameData.h"
#include "D3D12CommandListPool.h"
#include "D3D12RootDescriptorPool.h"
#include "D3D12DescriptorPool.h"
#include "../D3D12VisObject.h"

static constexpr uint32_t DynamicAllocationBlockSize = 65536 * 64;
static constexpr uint32_t MaxMemSizePerBlock = 0x1000000;
static constexpr uint32_t MemAlignSize = 32;

void D3D12FrameData::Init() {
    InitMemBlocks();

#ifdef USE_RENDER_TASK
    numThreads = renderer->renderTaskManager.NumThreads();
#else
    numThreads = 1;
#endif

    for (int threadIndex = 0; threadIndex < numThreads; ++threadIndex) {
        DataPerThread *data = &threadData[threadIndex];

#ifdef USE_SECONDARY_COMMAND_LISTS
        uint32_t maxSecondaryCommandLists = 8;
#else
        uint32_t maxSecondaryCommandLists = 0;
#endif
        // 그래픽스 커맨드 리스트 풀을 생성한다.
        data->graphicsCommandListPool = new D3D12CommandListPool(renderer->device, threadIndex, D3D12_COMMAND_LIST_TYPE_DIRECT, 8, maxSecondaryCommandLists);

        // 컴퓨트 커맨드 리스트 풀을 생성한다.
        data->computeCommandListPool = new D3D12CommandListPool(renderer->device, threadIndex, D3D12_COMMAND_LIST_TYPE_COMPUTE, 8);

        // 쉐이더에서 사용할 디스크립터 힙을 생성한다.
        data->rootDescriptorPool = new D3D12RootDescriptorPool(renderer->device, 16384);

        // 미리 다이나믹 버퍼 블럭을 1개 생성한다.
        D3D12DynamicAllocation *dynamicAllocation = new D3D12DynamicAllocation(DynamicAllocationBlockSize);
        data->dynamicAllocations.SetGranularity(16);
        data->dynamicAllocations.Append(dynamicAllocation);

        // 다이나믹 버퍼에서 사용할 (CBV, SRV, UAV) 디스크립터 풀을 생성한다.
        data->dynamicDescriptorPool = new D3D12DescriptorPool(renderer->device, D3D12DescriptorPool::Type::CBV_SRV_UAV, 8192, false);

        data->dynamicDescriptorHandles.SetGranularity(2048);
        data->dynamicDescriptorHandles.Reserve(4096);

        data->dynamicConstantBuffers.SetGranularity(256);
        data->dynamicConstantBuffers.Reserve(256);

        data->dynamicVertexBuffers.SetGranularity(256);
        data->dynamicVertexBuffers.Reserve(256);

        data->dynamicIndexBuffers.SetGranularity(256);
        data->dynamicIndexBuffers.Reserve(256);

        data->dynamicBuffers.SetGranularity(256);
        data->dynamicBuffers.Reserve(256);
    }
}

void D3D12FrameData::Shutdown() {
    for (int threadIndex = 0; threadIndex < numThreads; ++threadIndex) {
        DataPerThread *data = &threadData[threadIndex];

        data->dynamicAllocations.DeleteContents(false);

        data->dynamicDescriptorPool->Clear();
        data->dynamicDescriptorHandles.SetCount(0, false);

        SAFE_DELETE(data->dynamicDescriptorPool);
        SAFE_DELETE(data->rootDescriptorPool);
        SAFE_DELETE(data->graphicsCommandListPool);
        SAFE_DELETE(data->computeCommandListPool);
    }

    FreeVisObjects();

    ClearMemBlocks();
}

void D3D12FrameData::BeginFrame() {
    // 쓰레드 별로 사용할 자원을 Reset 한다.
    for (int threadIndex = 0; threadIndex < numThreads; ++threadIndex) {
        DataPerThread *data = &threadData[threadIndex];

        // 다이나믹 버퍼를 리셋한다.
        for (int i = 0; i < data->dynamicAllocations.Count(); ++i) {
            data->dynamicAllocations[i]->usedBytes = 0;
        }

        // 이번에 프레임에 사용할 다이나믹 버퍼의 디스크립터들을 초기화
        data->dynamicDescriptorPool->Clear();
        data->dynamicDescriptorHandles.SetCount(0, false);

        // 다이나믹 버퍼들도 비운다.
        data->dynamicConstantBuffers.SetCount(0, false);
        data->dynamicVertexBuffers.SetCount(0, false);
        data->dynamicIndexBuffers.SetCount(0, false);
        data->dynamicBuffers.SetCount(0, false);

        // 루트 디스크립터 풀을 비운다.
        data->rootDescriptorPool->Reset();

        // 커맨드 리스트 풀을 비운다.
        data->graphicsCommandListPool->Clear();
    }

    // 이번 프레임에 사용할 프레임 데이터를 사용하기 위해서는, GPU 에서 이전 프레임에 대한 렌더링이 완료되야 한다.
    renderer->WaitFence(fenceValue);
}

void D3D12FrameData::EndFrame() {
    fenceValue = renderer->SignalFence();
}

void D3D12FrameData::InitMemBlocks() {
    headBlock = AllocMemBlock();
    currentBlock = headBlock;
}

void D3D12FrameData::ClearMemBlocks() {
    MemBlock *nextBlock;

    for (MemBlock *block = headBlock; block; block = nextBlock) {
        nextBlock = block->next;
        Mem_Free(block);
    }

    headBlock = nullptr;
    currentBlock = nullptr;
}

D3D12FrameData::MemBlock *D3D12FrameData::AllocMemBlock() {
    MemBlock *block = (MemBlock *)Mem_Alloc(sizeof(*block) + MemAlignSize - 1 + MaxMemSizePerBlock);
    if (!block) {
        BE_FATALERROR("D3D12FrameData::AllocMemBlock: failed to allocate memory");
    }

    block->base = (byte *)AlignUp((intptr_t)block + sizeof(*block), MemAlignSize);
    block->size = MaxMemSizePerBlock;
    block->used = 0;
    block->next = nullptr;
    return block;
}

void *D3D12FrameData::MemAlloc(int size) {
    size = AlignUp(size, MemAlignSize);
    if (size > MaxMemSizePerBlock) {
        BE_FATALERROR("D3D12FrameData::MemAlloc: %i exceeded MaxMemSizePerBlock", size);
    }

    for (MemBlock *block = currentBlock; block; block = block->next) {
        if (block->size - block->used >= size) {
            void *alloc = block->base + block->used;
            block->used += size;
            currentBlock = block;
            return alloc;
        }
    }

    MemBlock *newBlock = AllocMemBlock();
    currentBlock->next = newBlock;
    currentBlock = newBlock;
    currentBlock->used = size;

    return currentBlock->base;
}

void *D3D12FrameData::ClearedMemAlloc(int size) {
    void *mem = MemAlloc(size);
    simdProcessor->Memset(mem, 0, size);
    return mem;
}

void D3D12FrameData::ClearMemAllocs() {
    FreeVisObjects();

    // Reset the mem allocation to the first block.
    currentBlock = headBlock;

    // Clear all the blocks.
    for (MemBlock *block = headBlock; block; block = block->next) {
        block->used = 0;
    }
}

D3D12VisObject* D3D12FrameData::AllocVisObjects(int numVisObjects) {
    assert(!visObjects);

    this->numVisObjects = numVisObjects;
    this->visObjects = (D3D12VisObject *)MemAlloc(sizeof(D3D12VisObject) * numVisObjects);

    // placement new 로 생성자 호출
    for (int visObjectIndex = 0; visObjectIndex < numVisObjects; ++visObjectIndex) {
        new (visObjects + visObjectIndex) D3D12VisObject();
    }
    return visObjects;
}

void D3D12FrameData::FreeVisObjects() {
    if (!visObjects) {
        return;
    }

    // 소멸자 호출
    for (int visObjectIndex = 0; visObjectIndex < numVisObjects; ++visObjectIndex) {
        (visObjects + visObjectIndex)->~D3D12VisObject();
    }

    visObjects = nullptr;
    numVisObjects = 0;
}

RHIRenderer::ConstantBuffer *D3D12FrameData::AllocConstant(int threadIndex, uint32_t size) {
    DataPerThread *data = &threadData[threadIndex];
    D3D12DynamicAllocation *currentDynamicAllocation = data->dynamicAllocations.Last();

    // 상수 버퍼의 오프셋 & 크기는 256 바이트 단위로 정렬
    uint32_t alignedOffset = AlignUp(currentDynamicAllocation->usedBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    uint32_t alignedSize = AlignUp(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    // 상수 버퍼의 크기는 64kb 를 넘길 수 없다
    if (alignedSize > D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16) {
        BE_WARNLOG("Constant buffer view size cannot exceeds 64KB limit\n");
        return nullptr;
    }

    // 필요한 데이터의 크기가 다이나믹 버퍼의 크기를 넘어간다면 추가로 다이나믹 버퍼를 생성한다.
    if (currentDynamicAllocation->usedBytes + alignedSize > currentDynamicAllocation->buffer->GetSize()) {
        currentDynamicAllocation = new D3D12DynamicAllocation(DynamicAllocationBlockSize);
        data->dynamicAllocations.Append(currentDynamicAllocation);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = {};
    if (!data->dynamicDescriptorPool->Alloc(&descriptorHandle, nullptr)) {
        return nullptr;
    }

    // 버퍼 리소스를 쪼개서 CBV 를 만들어 사용한다.
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = currentDynamicAllocation->buffer->GetResource()->GetGPUVirtualAddress() + alignedOffset;
    cbvDesc.SizeInBytes = alignedSize;

    renderer->device->CreateConstantBufferView(&cbvDesc, descriptorHandle);
    data->dynamicDescriptorHandles.Append(descriptorHandle);

    D3D12ConstantBuffer dynamicConstantBuffer;
    dynamicConstantBuffer.writePtr = (byte *)currentDynamicAllocation->mappedBase + alignedOffset;
    dynamicConstantBuffer.descriptorHandle = descriptorHandle;
    data->dynamicConstantBuffers.Append(dynamicConstantBuffer);

    currentDynamicAllocation->usedBytes = alignedOffset + alignedSize;

    return &data->dynamicConstantBuffers.Last();
}

RHIRenderer::VertexBuffer *D3D12FrameData::AllocVertex(int threadIndex, uint32_t vertexSize, uint32_t count) {
    DataPerThread *data = &threadData[threadIndex];
    D3D12DynamicAllocation *currentDynamicAllocation = data->dynamicAllocations.Last();

    // 버텍스 버퍼의 오프셋은 4 바이트 단위로 정렬
    uint32_t alignedOffset = AlignUp(currentDynamicAllocation->usedBytes, 4);
    uint32_t size = vertexSize * count;

    // 필요한 데이터의 크기가 다이나믹 버퍼의 크기를 넘어간다면 추가로 다이나믹 버퍼를 생성한다.
    if (alignedOffset + size > currentDynamicAllocation->buffer->GetSize()) {
        currentDynamicAllocation = new D3D12DynamicAllocation(DynamicAllocationBlockSize);
        data->dynamicAllocations.Append(currentDynamicAllocation);
    }

    D3D12VertexBuffer dynamicVertexBuffer;
    dynamicVertexBuffer.writePtr = (byte *)currentDynamicAllocation->mappedBase + alignedOffset;
    dynamicVertexBuffer.vbv.BufferLocation = currentDynamicAllocation->buffer->GetResource()->GetGPUVirtualAddress() + alignedOffset;
    dynamicVertexBuffer.vbv.SizeInBytes = size;
    dynamicVertexBuffer.vbv.StrideInBytes = vertexSize;
    data->dynamicVertexBuffers.Append(dynamicVertexBuffer);

    currentDynamicAllocation->usedBytes = alignedOffset + size;

    return &data->dynamicVertexBuffers.Last();
}

RHIRenderer::IndexBuffer *D3D12FrameData::AllocIndex(int threadIndex, uint32_t indexSize, uint32_t count) {
    DataPerThread *data = &threadData[threadIndex];
    D3D12DynamicAllocation *currentDynamicAllocation = data->dynamicAllocations.Last();

    // 인덱스 버퍼의 오프셋은 4 바이트 단위로 정렬
    uint32_t alignedOffset = AlignUp(currentDynamicAllocation->usedBytes, 4);
    uint32_t size = indexSize * count;

    // 필요한 데이터의 크기가 다이나믹 버퍼의 크기를 넘어간다면 추가로 다이나믹 버퍼를 생성한다.
    if (alignedOffset + size > currentDynamicAllocation->buffer->GetSize()) {
        currentDynamicAllocation = new D3D12DynamicAllocation(DynamicAllocationBlockSize);
        data->dynamicAllocations.Append(currentDynamicAllocation);
    }

    D3D12IndexBuffer dynamicIndexBuffer;
    dynamicIndexBuffer.writePtr = (byte *)currentDynamicAllocation->mappedBase + alignedOffset;
    dynamicIndexBuffer.ibv.BufferLocation = currentDynamicAllocation->buffer->GetResource()->GetGPUVirtualAddress() + alignedOffset;
    dynamicIndexBuffer.ibv.SizeInBytes = size;
    dynamicIndexBuffer.ibv.Format = (indexSize == sizeof(uint16_t) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
    data->dynamicIndexBuffers.Append(dynamicIndexBuffer);

    currentDynamicAllocation->usedBytes = alignedOffset + size;

    return &data->dynamicIndexBuffers.Last();
}

RHIRenderer::Buffer *D3D12FrameData::AllocBuffer(int threadIndex, bool shaderWritable, Image::Format::Enum format, uint32_t structureByteStride, uint32_t count) {
    DataPerThread *data = &threadData[threadIndex];
    D3D12DynamicAllocation *currentDynamicAllocation = data->dynamicAllocations.Last();

    // 버퍼의 오프셋은 stride 단위로 정렬
    uint32_t stride = format == Image::Format::Unknown ? structureByteStride : Image::BytesPerPixel(format);
    uint32_t alignedOffset = AlignUp(currentDynamicAllocation->usedBytes, stride);
    uint32_t size = stride * count;

    // 필요한 데이터의 크기가 다이나믹 버퍼의 크기를 넘어간다면 추가로 다이나믹 버퍼를 생성한다.
    if (alignedOffset + size > currentDynamicAllocation->buffer->GetSize()) {
        currentDynamicAllocation = new D3D12DynamicAllocation(DynamicAllocationBlockSize);
        data->dynamicAllocations.Append(currentDynamicAllocation);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = {};
    if (!data->dynamicDescriptorPool->Alloc(&descriptorHandle, nullptr)) {
        return nullptr;
    }

    if (shaderWritable) {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        D3D12Renderer::ImageFormatToDXGIFormat(format, false, &uavDesc.Format);
        uavDesc.Buffer.FirstElement = alignedOffset / stride;
        uavDesc.Buffer.NumElements = count;

        if (uavDesc.Format == DXGI_FORMAT_UNKNOWN) {
            uavDesc.Buffer.StructureByteStride = structureByteStride;
        } else if (uavDesc.Format == DXGI_FORMAT_R32_TYPELESS) {
            uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
        }

        renderer->device->CreateUnorderedAccessView(currentDynamicAllocation->buffer->GetResource(), nullptr, &uavDesc, descriptorHandle);
        data->dynamicDescriptorHandles.Append(descriptorHandle);
    } else {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        D3D12Renderer::ImageFormatToDXGIFormat(format, false, &srvDesc.Format);
        srvDesc.Buffer.FirstElement = alignedOffset / stride;
        srvDesc.Buffer.NumElements = count;

        if (srvDesc.Format == DXGI_FORMAT_UNKNOWN) {
            srvDesc.Buffer.StructureByteStride = structureByteStride;
        } else if (srvDesc.Format == DXGI_FORMAT_R32_TYPELESS) {
            srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
        }

        renderer->device->CreateShaderResourceView(currentDynamicAllocation->buffer->GetResource(), &srvDesc, descriptorHandle);
        data->dynamicDescriptorHandles.Append(descriptorHandle);
    }

    D3D12Buffer dynamicBuffer;
    dynamicBuffer.writePtr = (byte *)currentDynamicAllocation->mappedBase + alignedOffset;
    if (shaderWritable) {
        dynamicBuffer.uavCpuDescriptorHandles.Append(descriptorHandle);
    } else {
        dynamicBuffer.srvCpuDescriptorHandles.Append(descriptorHandle);
    }
    data->dynamicBuffers.Append(dynamicBuffer);

    currentDynamicAllocation->usedBytes = alignedOffset + size;

    return &data->dynamicBuffers.Last();
}

D3D12DynamicAllocation::D3D12DynamicAllocation(uint64_t size) {
    // 업로드 버퍼 생성
    buffer = static_cast<D3D12Buffer *>(renderer->CreateBuffer(RHIRenderer::BufferUsage::Upload,
        RHIRenderer::ResourceFlag::ConstantBuffer | RHIRenderer::ResourceFlag::VertexBuffer | RHIRenderer::ResourceFlag::IndexBuffer | RHIRenderer::ResourceFlag::ShaderResource,
        size, Image::Format::R_32_TYPELESS, 0, nullptr));

    // 버퍼를 프로그램이 끝날 때 까지 Map 해놓고 쓴다. (Pinned) 
    buffer->GetResource()->Map(0, nullptr, reinterpret_cast<void **>(&mappedBase));
}

D3D12DynamicAllocation::~D3D12DynamicAllocation() {
    renderer->DestroyBuffer(buffer);
}
