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
#include "D3D12FrameData.h"
#include "D3D12CommandList.h"
#include "D3D12CommandListPool.h"
#include "D3D12DescriptorPool.h"
#include "D3D12RootDescriptorPool.h"

static constexpr uint32_t DynamicAllocationBlockSize = 65536 * 64;

D3D12DynamicAllocation::D3D12DynamicAllocation(uint64_t size) {
    // 업로드 버퍼 생성
    buffer = static_cast<D3D12Buffer *>(RHI::renderer->CreateBuffer(RHI::BufferUsage::Upload,
        RHI::ResourceFlag::ConstantBuffer | RHI::ResourceFlag::VertexBuffer | RHI::ResourceFlag::IndexBuffer | RHI::ResourceFlag::ShaderResource,
        size, BE1::Image::Format::R_32_TYPELESS, 0, nullptr));

    // 버퍼를 프로그램이 끝날 때 까지 Map 해놓고 쓴다. (Pinned) 
    buffer->GetResource()->Map(0, nullptr, reinterpret_cast<void **>(&mappedBase));
}

D3D12DynamicAllocation::~D3D12DynamicAllocation() {
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

    // 쉐이더에서 사용할 디스크립터 힙을 생성한다.
    rootDescriptorPool = new D3D12RootDescriptorPool(D3D12Renderer::GetRenderer()->device, 16384);

    // 미리 다이나믹 버퍼 블럭을 1개 생성한다.
    D3D12DynamicAllocation *dynamicAllocation = new D3D12DynamicAllocation(DynamicAllocationBlockSize);
    dynamicAllocations.SetGranularity(16);
    dynamicAllocations.Append(dynamicAllocation);

    // 다이나믹 버퍼에서 사용할 (CBV, SRV, UAV) 디스크립터 풀을 생성한다.
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
    dynamicAllocations.DeleteContents(false);

    dynamicDescriptorPool->Clear();
    dynamicDescriptorHandles.SetCount(0, false);

    SAFE_DELETE(dynamicDescriptorPool);
    SAFE_DELETE(rootDescriptorPool);
    SAFE_DELETE(graphicsCommandListPool);
    SAFE_DELETE(computeCommandListPool);
}

void D3D12FrameThreadData::BeginFrame() {
    // 다이나믹 버퍼를 리셋한다.
    for (D3D12DynamicAllocation *dynamicAllocation : dynamicAllocations) {
        dynamicAllocation->usedBytes = 0;
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
    rootDescriptorPool->Reset();

    // 커맨드 리스트 풀을 비운다.
    graphicsCommandListPool->Clear();
}

RHI::ConstantBuffer *D3D12FrameThreadData::AllocConstant(uint32_t size) {
    D3D12DynamicAllocation *currentDynamicAllocation = dynamicAllocations.Last();

    // 상수 버퍼의 오프셋 & 크기는 256 바이트 단위로 정렬
    uint32_t alignedOffset = BE1::AlignUp(currentDynamicAllocation->usedBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    uint32_t alignedSize = BE1::AlignUp(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    // 상수 버퍼의 크기는 64kb 를 넘길 수 없다
    if (alignedSize > D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16) {
        BE_WARNLOG("Constant buffer view size cannot exceeds 64KB limit\n");
        return nullptr;
    }

    // 필요한 데이터의 크기가 다이나믹 버퍼의 크기를 넘어간다면 추가로 다이나믹 버퍼를 생성한다.
    if (currentDynamicAllocation->usedBytes + alignedSize > currentDynamicAllocation->buffer->GetSize()) {
        currentDynamicAllocation = new D3D12DynamicAllocation(DynamicAllocationBlockSize);
        dynamicAllocations.Append(currentDynamicAllocation);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = {};
    if (!dynamicDescriptorPool->Alloc(&descriptorHandle, nullptr)) {
        return nullptr;
    }

    // 버퍼 리소스를 쪼개서 CBV 를 만들어 사용한다.
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = currentDynamicAllocation->buffer->GetResource()->GetGPUVirtualAddress() + alignedOffset;
    cbvDesc.SizeInBytes = alignedSize;

    D3D12Renderer::GetRenderer()->device->CreateConstantBufferView(&cbvDesc, descriptorHandle);
    dynamicDescriptorHandles.Append(descriptorHandle);

    D3D12ConstantBuffer dynamicConstantBuffer;
    dynamicConstantBuffer.writePtr = (byte *)currentDynamicAllocation->mappedBase + alignedOffset;
    dynamicConstantBuffer.descriptorHandle = descriptorHandle;
    dynamicConstantBuffers.Append(dynamicConstantBuffer);

    currentDynamicAllocation->usedBytes = alignedOffset + alignedSize;

    return &dynamicConstantBuffers.Last();
}

RHI::VertexBuffer *D3D12FrameThreadData::AllocVertex(uint32_t vertexSize, uint32_t count) {
    D3D12DynamicAllocation *currentDynamicAllocation = dynamicAllocations.Last();

    // 버텍스 버퍼의 오프셋은 4 바이트 단위로 정렬
    uint32_t alignedOffset = BE1::AlignUp(currentDynamicAllocation->usedBytes, 4);
    uint32_t size = vertexSize * count;

    // 필요한 데이터의 크기가 다이나믹 버퍼의 크기를 넘어간다면 추가로 다이나믹 버퍼를 생성한다.
    if (alignedOffset + size > currentDynamicAllocation->buffer->GetSize()) {
        currentDynamicAllocation = new D3D12DynamicAllocation(DynamicAllocationBlockSize);
        dynamicAllocations.Append(currentDynamicAllocation);
    }

    D3D12VertexBuffer dynamicVertexBuffer;
    dynamicVertexBuffer.writePtr = (byte *)currentDynamicAllocation->mappedBase + alignedOffset;
    dynamicVertexBuffer.vbv.BufferLocation = currentDynamicAllocation->buffer->GetResource()->GetGPUVirtualAddress() + alignedOffset;
    dynamicVertexBuffer.vbv.SizeInBytes = size;
    dynamicVertexBuffer.vbv.StrideInBytes = vertexSize;
    dynamicVertexBuffers.Append(dynamicVertexBuffer);

    currentDynamicAllocation->usedBytes = alignedOffset + size;

    return &dynamicVertexBuffers.Last();
}

RHI::IndexBuffer *D3D12FrameThreadData::AllocIndex(uint32_t indexSize, uint32_t count) {
    D3D12DynamicAllocation *currentDynamicAllocation = dynamicAllocations.Last();

    // 인덱스 버퍼의 오프셋은 4 바이트 단위로 정렬
    uint32_t alignedOffset = BE1::AlignUp(currentDynamicAllocation->usedBytes, 4);
    uint32_t size = indexSize * count;

    // 필요한 데이터의 크기가 다이나믹 버퍼의 크기를 넘어간다면 추가로 다이나믹 버퍼를 생성한다.
    if (alignedOffset + size > currentDynamicAllocation->buffer->GetSize()) {
        currentDynamicAllocation = new D3D12DynamicAllocation(DynamicAllocationBlockSize);
        dynamicAllocations.Append(currentDynamicAllocation);
    }

    D3D12IndexBuffer dynamicIndexBuffer;
    dynamicIndexBuffer.writePtr = (byte *)currentDynamicAllocation->mappedBase + alignedOffset;
    dynamicIndexBuffer.ibv.BufferLocation = currentDynamicAllocation->buffer->GetResource()->GetGPUVirtualAddress() + alignedOffset;
    dynamicIndexBuffer.ibv.SizeInBytes = size;
    dynamicIndexBuffer.ibv.Format = (indexSize == sizeof(uint16_t) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
    dynamicIndexBuffers.Append(dynamicIndexBuffer);

    currentDynamicAllocation->usedBytes = alignedOffset + size;

    return &dynamicIndexBuffers.Last();
}

RHI::Buffer *D3D12FrameThreadData::AllocBuffer(bool shaderWritable, BE1::Image::Format::Enum format, uint32_t structureByteStride, uint32_t count) {
    D3D12DynamicAllocation *currentDynamicAllocation = dynamicAllocations.Last();

    // 버퍼의 오프셋은 stride 단위로 정렬
    uint32_t stride = format == BE1::Image::Format::Unknown ? structureByteStride : BE1::Image::BytesPerPixel(format);
    uint32_t alignedOffset = BE1::AlignUp(currentDynamicAllocation->usedBytes, stride);
    uint32_t size = stride * count;

    // 필요한 데이터의 크기가 다이나믹 버퍼의 크기를 넘어간다면 추가로 다이나믹 버퍼를 생성한다.
    if (alignedOffset + size > currentDynamicAllocation->buffer->GetSize()) {
        currentDynamicAllocation = new D3D12DynamicAllocation(DynamicAllocationBlockSize);
        dynamicAllocations.Append(currentDynamicAllocation);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = {};
    if (!dynamicDescriptorPool->Alloc(&descriptorHandle, nullptr)) {
        return nullptr;
    }

    if (shaderWritable) {
        D3D12UAVDescriptor uavDescriptor;
        uavDescriptor.uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        D3D12Renderer::ImageFormatToDXGIFormat(format, false, &uavDescriptor.uavDesc.Format);
        uavDescriptor.uavDesc.Buffer.FirstElement = alignedOffset / stride;
        uavDescriptor.uavDesc.Buffer.NumElements = count;

        if (uavDescriptor.uavDesc.Format == DXGI_FORMAT_UNKNOWN) {
            uavDescriptor.uavDesc.Buffer.StructureByteStride = structureByteStride;
        } else if (uavDescriptor.uavDesc.Format == DXGI_FORMAT_R32_TYPELESS) {
            uavDescriptor.uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
        }

        D3D12Renderer::GetRenderer()->device->CreateUnorderedAccessView(currentDynamicAllocation->buffer->GetResource(), nullptr, &uavDescriptor.uavDesc, uavDescriptor.cpuDescriptorHandle);
        dynamicDescriptorHandles.Append(descriptorHandle);

        D3D12Buffer dynamicBuffer;
        dynamicBuffer.writePtr = (byte *)currentDynamicAllocation->mappedBase + alignedOffset;
        dynamicBuffer.uavDescriptors.Append(uavDescriptor);
        dynamicBuffers.Append(dynamicBuffer);
    } else {
        D3D12SRVDescriptor srvDescriptor;
        srvDescriptor.srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDescriptor.srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        D3D12Renderer::ImageFormatToDXGIFormat(format, false, &srvDescriptor.srvDesc.Format);
        srvDescriptor.srvDesc.Buffer.FirstElement = alignedOffset / stride;
        srvDescriptor.srvDesc.Buffer.NumElements = count;

        if (srvDescriptor.srvDesc.Format == DXGI_FORMAT_UNKNOWN) {
            srvDescriptor.srvDesc.Buffer.StructureByteStride = structureByteStride;
        } else if (srvDescriptor.srvDesc.Format == DXGI_FORMAT_R32_TYPELESS) {
            srvDescriptor.srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
        }

        D3D12Renderer::GetRenderer()->device->CreateShaderResourceView(currentDynamicAllocation->buffer->GetResource(), &srvDescriptor.srvDesc, srvDescriptor.cpuDescriptorHandle);
        dynamicDescriptorHandles.Append(descriptorHandle);

        D3D12Buffer dynamicBuffer;
        dynamicBuffer.writePtr = (byte *)currentDynamicAllocation->mappedBase + alignedOffset;
        dynamicBuffer.srvDescriptors.Append(srvDescriptor);
        dynamicBuffers.Append(dynamicBuffer);
    }

    currentDynamicAllocation->usedBytes = alignedOffset + size;

    return &dynamicBuffers.Last();
}

RHI::CommandList *D3D12FrameThreadData::BeginSecondaryCommandList(const RHI::CommandList *primaryCommandList) {
    // Secondary CommandList 를 얻어온다.
    D3D12CommandList *commandList = static_cast<D3D12CommandList *>(AllocGraphicsCommandList(RHI::CommandListType::Secondary));
    commandList->Reset(true, primaryCommandList);

    // Secondary CommandList 의 루트 디스크립터 힙을 지정한다.
    // 반드시 Primary CommandList 와 동일한 디스크립터 힙을 사용해야 한다.
    ID3D12DescriptorHeap *descriptorHeaps[] = { rootDescriptorPool->GetDescriptorHeap()};
    commandList->SetDescriptorHeaps(COUNT_OF(descriptorHeaps), descriptorHeaps);

    return commandList;
}
