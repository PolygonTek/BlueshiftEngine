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
#include "D3D12Buffer.h"
#include "D3D12CommandList.h"
#include "D3D12DescriptorPool.h"
#include "D3D12FrameData.h"

void D3D12Buffer::Release() {
    for (const D3D12SRVDescriptor &srvDescriptor : srvDescriptors) {
        if (srvDescriptor.cpuDescriptorHandle.ptr != 0) {
            D3D12Renderer::GetRenderer()->resCpuDescriptorPool->Free(srvDescriptor.cpuDescriptorHandle);
        }
    }
    srvDescriptors.Clear();

    for (const D3D12UAVDescriptor &uavDescriptor : uavDescriptors) {
        if (uavDescriptor.cpuDescriptorHandle.ptr != 0) {
            D3D12Renderer::GetRenderer()->uavCpuDescriptorPool->Free(uavDescriptor.cpuDescriptorHandle);
        }
    }
    uavDescriptors.Clear();

#ifdef USE_D3D12_MEMALLOC
    SAFE_RELEASE(bufferAllocation);
#else
    SAFE_RELEASE(bufferResource);
#endif
}

uint64_t D3D12Buffer::GetSize() {
#ifdef USE_D3D12_MEMALLOC
    return bufferAllocation->GetSize();
#else
    D3D12_RESOURCE_DESC resourceDesc = bufferResource->GetDesc();
    BE1::Image::Format imageFormat;
    if (D3D12Renderer::DXGIFormatToImageFormat(resourceDesc.Format, &imageFormat, nullptr)) {
        return BE1::Image::MemRequired(resourceDesc.Width, resourceDesc.Height, resourceDesc.DepthOrArraySize, resourceDesc.MipLevels, imageFormat);
    }
    return resourceDesc.Width;
#endif
}

RHI::Buffer *D3D12Renderer::CreateBuffer(RHI::BufferUsage usage, RHI::ResourceFlag flags, uint64_t size, BE1::Image::Format format, uint32_t structuredStride, const void *data) {
    D3D12_HEAP_TYPE heapType;
    D3D12_RESOURCE_STATES initialState;
    D3D12_RESOURCE_FLAGS resourceFlags;

    switch (usage) {
    case RHI::BufferUsage::Default:
        heapType = D3D12_HEAP_TYPE_DEFAULT;
        // NOTE: 텍스처와 달리 D3D12_RESOURCE_DIMENSION_BUFFER 리소스의 initialState 는 항상 D3D12_RESOURCE_STATE_COMMON 로 설정된다.
        initialState = D3D12_RESOURCE_STATE_COMMON;
        resourceFlags = D3D12_RESOURCE_FLAG_NONE;
        break;
    case RHI::BufferUsage::Upload:
        heapType = D3D12_HEAP_TYPE_UPLOAD;
        // NOTE: 리소스가 업로드 힙인 경우 initialState 는 다른 값으로 설정해도 무시되며, 항상 D3D12_RESOURCE_STATE_GENERIC_READ 로 설정된다.
        initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
        resourceFlags = D3D12_RESOURCE_FLAG_NONE;
        break;
    case RHI::BufferUsage::Readback:
        heapType = D3D12_HEAP_TYPE_READBACK;
        // NOTE: 리소스가 리드백 힙인 경우 initialState 는 다른 값으로 설정해도 무시되며, 항상 D3D12_RESOURCE_STATE_COPY_DEST 로 설정된다.
        initialState = D3D12_RESOURCE_STATE_COPY_DEST;
        resourceFlags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        break;
    default:
        BE_ERRLOG("D3D12Renderer::CreateBuffer: Invalid buffer usage (%i)\n", static_cast<int>(usage));
        return nullptr;
    }

    if (BE1::HasFlag(flags, RHI::ResourceFlag::UnorderedAccess)) {
        resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    } else if (!BE1::HasFlag(flags, RHI::ResourceFlag::ShaderResource)) {
        resourceFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    }

    uint32_t bufferSize = size;

    if (BE1::HasFlag(flags, RHI::ResourceFlag::ConstantBuffer)) {
        // 상수 버퍼를 생성하면 내부적으로 GPU 에서 어차피 256 바이트로 사이즈가 정렬된다.
        bufferSize = BE1::AlignUp(bufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    }

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = bufferSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = resourceFlags;

#ifdef USE_D3D12_MEMALLOC
    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    //allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_CAN_ALIAS;
    allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_TIME;
    allocationDesc.HeapType = heapType;

    D3D12MA::Allocation *bufferAllocation;
    if (FAILED(allocator->CreateResource(
        &allocationDesc,
        &bufferDesc,
        initialState,
        nullptr,
        &bufferAllocation,
        IID_NULL, nullptr))) {
        return nullptr;
    }
    ID3D12Resource *bufferResource = bufferAllocation->GetResource();
#else
    D3D12_HEAP_PROPERTIES heapProperties;
    heapProperties.Type = heapType;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    ID3D12Resource *bufferResource = nullptr;
    if (FAILED(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        initialState,
        nullptr, IID_PPV_ARGS(&bufferResource)))) {
        return nullptr;
    }
#endif

    D3D12Buffer *buffer = new D3D12Buffer;
    buffer->bufferUsage = usage;
    buffer->flags = flags;

#ifdef USE_D3D12_MEMALLOC
    buffer->bufferAllocation = bufferAllocation;
#else
    buffer->bufferResource = bufferResource;
#endif

    buffer->size = size;
    // buffer view 가 SRV 이거나 UAV 일 경우..
    // 1) format == Image::Format::Unknown 라면, structured buffer 다.
    // 2) format == Image::Format::R_32_TYPELESS 라면, raw buffer 다.
    buffer->format = format;
    // structuredStride 는 structured buffer 에서만 사용된다.
    buffer->structuredStride = structuredStride;

    ID3D12Resource *uploadBuffer = nullptr;

    if (data) {
        if (heapType == D3D12_HEAP_TYPE_DEFAULT) {
            D3D12_RESOURCE_DESC uploadBufferDesc;
            uploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            uploadBufferDesc.Alignment = 0;
            uploadBufferDesc.Width = bufferSize;
            uploadBufferDesc.Height = 1;
            uploadBufferDesc.DepthOrArraySize = 1;
            uploadBufferDesc.MipLevels = 1;
            uploadBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
            uploadBufferDesc.SampleDesc.Count = 1;
            uploadBufferDesc.SampleDesc.Quality = 0;
            uploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            uploadBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            D3D12_HEAP_PROPERTIES heapProperties;
            heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
            heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heapProperties.CreationNodeMask = 1;
            heapProperties.VisibleNodeMask = 1;

            // CPU 에서 GPU 로 전송할 업로드 버퍼 생성
            if (FAILED(device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &uploadBufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr, IID_PPV_ARGS(&uploadBuffer)))) {
                SAFE_DELETE(buffer);
                return nullptr;
            }

            UINT8 *mappedPtr = nullptr;
            uploadBuffer->Map(0, nullptr, reinterpret_cast<void **>(&mappedPtr));

            BE1::simdProcessor->MemcpyStream(mappedPtr, data, size);

            CD3DX12_RANGE writtenRange(0, size);
            uploadBuffer->Unmap(0, &writtenRange);

            D3D12_RESOURCE_STATES afterResourceState = D3D12_RESOURCE_STATE_COMMON;
            if (BE1::HasFlag(flags, RHI::ResourceFlag::ConstantBuffer | RHI::ResourceFlag::VertexBuffer)) {
                afterResourceState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            } else if (BE1::HasFlag(flags, RHI::ResourceFlag::IndexBuffer)) {
                afterResourceState = D3D12_RESOURCE_STATE_INDEX_BUFFER;
            } else if (BE1::HasFlag(flags, RHI::ResourceFlag::UnorderedAccess)) {
                afterResourceState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            }

            // 업로드 버퍼에서 GPU 버퍼로 데이터 카피
            resourceCommandList->Reset();
            resourceCommandList->ResourceBarrier(bufferResource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
            resourceCommandList->GetGraphicsCommandList()->CopyBufferRegion(bufferResource, 0, uploadBuffer, 0, size);
            resourceCommandList->ResourceBarrier(bufferResource, D3D12_RESOURCE_STATE_COPY_DEST, afterResourceState);
            resourceCommandList->CloseAndExecute(RHI::CommandQueueType::Graphics);
        } else if (heapType == D3D12_HEAP_TYPE_UPLOAD) {
            UINT8 *mappedPtr = nullptr;
            bufferResource->Map(0, nullptr, reinterpret_cast<void **>(&mappedPtr));

            BE1::simdProcessor->MemcpyStream(mappedPtr, data, size);

            CD3DX12_RANGE writtenRange(0, size);
            bufferResource->Unmap(0, &writtenRange);
        } else {
            SAFE_DELETE(buffer);
            return nullptr;
        }
    }

    if (uploadBuffer) {
        MarkForRelease(uploadBuffer);
    }

    if (!BE1::HasFlag(flags, RHI::ResourceFlag::SkipDefaultViews)) {
        if (BE1::HasFlag(flags, RHI::ResourceFlag::ShaderResource)) {
            CreateSubresource(buffer, RHI::SubresourceType::SRV);
        }
        if (BE1::HasFlag(flags, RHI::ResourceFlag::UnorderedAccess)) {
            CreateSubresource(buffer, RHI::SubresourceType::UAV);
        }
    }

    return buffer;
}

int D3D12Renderer::CreateSubresource(RHI::Buffer *buffer, RHI::SubresourceType type, uint64_t offset, uint64_t size) {
    D3D12Buffer *d3d12Buffer = static_cast<D3D12Buffer *>(buffer);

    if (type == RHI::SubresourceType::SRV) {
        return CreateSubresourceSRV(d3d12Buffer, offset, size);
    }
    if (type == RHI::SubresourceType::UAV) {
        return CreateSubresourceUAV(d3d12Buffer, offset, size);
    }
    return -1;
}

int D3D12Renderer::CreateSubresourceSRV(D3D12Buffer *buffer, uint64_t offset, uint64_t size) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    ImageFormatToDXGIFormat(buffer->format, false, &srvDesc.Format);

    uint32_t byteStride = 0;

    if (buffer->format == BE1::Image::Format::Unknown) {
        // Structured buffer
        byteStride = buffer->structuredStride;
        srvDesc.Buffer.StructureByteStride = byteStride;
    } else {
        byteStride = BE1::Image::BytesPerPixel(buffer->format);

        if (buffer->format == BE1::Image::Format::R_32_TYPELESS) {
            // Raw buffer (4 바이트 정렬된, 바이트 단위 접근이 가능한 버퍼)
            srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
        }
    }

    srvDesc.Buffer.FirstElement = offset / byteStride;
    srvDesc.Buffer.NumElements = BE1::Min(size, buffer->size - offset) / byteStride;

    D3D12_CPU_DESCRIPTOR_HANDLE srvCpuDescriptorHandle;
    if (!resCpuDescriptorPool->Alloc(&srvCpuDescriptorHandle, nullptr)) {
        return -1;
    }
    device->CreateShaderResourceView(buffer->GetResource(), &srvDesc, srvCpuDescriptorHandle);

    D3D12SRVDescriptor srvDescriptor;
    srvDescriptor.srvDesc = srvDesc;
    srvDescriptor.cpuDescriptorHandle = srvCpuDescriptorHandle;
    return buffer->srvDescriptors.Append(srvDescriptor);
}

int D3D12Renderer::CreateSubresourceUAV(D3D12Buffer *buffer, uint64_t offset, uint64_t size) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    ImageFormatToDXGIFormat(buffer->format, false, &uavDesc.Format);

    uint32_t byteStride = 0;

    if (buffer->format == BE1::Image::Format::Unknown) {
        // Structured buffer
        byteStride = buffer->structuredStride;
        uavDesc.Buffer.StructureByteStride = byteStride;
    } else {
        byteStride = BE1::Image::BytesPerPixel(buffer->format);

        if (buffer->format == BE1::Image::Format::R_32_TYPELESS) {
            // Raw buffer (4 바이트 정렬된, 바이트 단위 접근이 가능한 버퍼)
            uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
        }
    }

    uavDesc.Buffer.FirstElement = offset / byteStride;
    uavDesc.Buffer.NumElements = BE1::Min(size, buffer->size - offset) / byteStride;

    // UAV 는 GPU 디스크립터도 같이 할당한다.
    D3D12_CPU_DESCRIPTOR_HANDLE uavCpuDescriptorHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE destCpuDescriptorHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE destGpuDescriptorHandle;

    if (!uavCpuDescriptorPool->Alloc(&uavCpuDescriptorHandle, nullptr)) {
        return -1;
    }
    if (!uavGpuDescriptorPool->Alloc(&destCpuDescriptorHandle, &destGpuDescriptorHandle)) {
        return -1;
    }
    device->CreateUnorderedAccessView(buffer->GetResource(), nullptr, &uavDesc, uavCpuDescriptorHandle);

    // 만들어진 UAV 디스크립터를 shader visible 한 디스크립터에 복사 (CPU + GPU)
    device->CopyDescriptorsSimple(1, destCpuDescriptorHandle, uavCpuDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12UAVDescriptor uavDescriptor;
    uavDescriptor.uavDesc = uavDesc;
    uavDescriptor.cpuDescriptorHandle = uavCpuDescriptorHandle;
    return buffer->uavDescriptors.Append(uavDescriptor);
}

void D3D12Renderer::DestroyBuffer(RHI::Buffer *buffer, bool immediate) {
    if (immediate) {
        delete buffer;
    } else {
        MarkForDelete(buffer);
    }
}

void D3D12Renderer::SetBuffer(RHI::CommandList *commandList, int slot, bool shaderWritable, const RHI::Buffer *buffer, int subresourceIndex) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    D3D12FrameThreadData *threadData = static_cast<D3D12FrameThreadData *>(d3d12CommandList->GetFrameThreadData());

    const D3D12Buffer *d3d12Buffer = static_cast<const D3D12Buffer *>(buffer);

    // 슬롯 (레지스터) 에 대한 루트 파라미터 인덱스를 얻고, 디스크립터 테이블일 경우 테이블 인덱스도 얻어온다.
    const D3D12PipelineState::Binder &binder = d3d12CommandList->currentPSO->binder;
    uint8_t rootParameterIndex = 0xFF;

    if (shaderWritable) {
        // UAV
        rootParameterIndex = binder.rootParameterBinder.uav[slot];
        uint8_t descriptorIndex = binder.descriptorTableBinder.uav[slot];
        if (descriptorIndex != 0xFF) {
            threadData->tableCpuDescriptorHandles[rootParameterIndex][descriptorIndex] = d3d12Buffer->uavDescriptors[subresourceIndex].cpuDescriptorHandle;
            if (threadData->tableCpuDescriptorHandles[rootParameterIndex][descriptorIndex].ptr == 0) {
                BE_ERRLOG("D3D12Renderer::SetBuffer: Buffer has no valid UAV descriptor handle\n");
                return;
            }
        }
        threadData->uavResources[slot] = d3d12Buffer;
    } else {
        // SRV
        rootParameterIndex = binder.rootParameterBinder.srv[slot];
        uint8_t descriptorIndex = binder.descriptorTableBinder.srv[slot];
        if (descriptorIndex != 0xFF) {
            threadData->tableCpuDescriptorHandles[rootParameterIndex][descriptorIndex] = d3d12Buffer->srvDescriptors[subresourceIndex].cpuDescriptorHandle;
            if (threadData->tableCpuDescriptorHandles[rootParameterIndex][descriptorIndex].ptr == 0) {
                BE_ERRLOG("D3D12Renderer::SetBuffer: Buffer has no valid SRV descriptor handle\n");
                return;
            }
        }
        threadData->srvResources[slot] = d3d12Buffer;
    }

    if (rootParameterIndex == 0xFF) {
        BE_ERRLOG("D3D12Renderer::SetBuffer: Invalid root parameter index\n");
        return;
    }

    if (d3d12CommandList->GetCommandListType() == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
        d3d12CommandList->computeRootParametersDirtyMask |= BIT64(rootParameterIndex);
    } else {
        d3d12CommandList->graphicsRootParametersDirtyMask |= BIT64(rootParameterIndex);
    }
}
