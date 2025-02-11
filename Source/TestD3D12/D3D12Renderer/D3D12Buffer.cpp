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
#include "D3D12Buffer.h"
#include "D3D12CommandList.h"
#include "D3D12DescriptorPool.h"
#include "D3D12FrameData.h"

void D3D12Buffer::Release() {
    if (srvDescriptor.cpuDescriptorHandle.ptr) {
        D3D12Renderer::GetRenderer()->resCpuDescriptorPool->Free(srvDescriptor.cpuDescriptorHandle);
        srvDescriptor.cpuDescriptorHandle = {};
    }

    if (uavDescriptor.cpuDescriptorHandle.ptr) {
        D3D12Renderer::GetRenderer()->uavCpuDescriptorPool->Free(uavDescriptor.cpuDescriptorHandle);
        uavDescriptor.cpuDescriptorHandle = {};
    }

    for (const D3D12SRVDescriptor &srvDescriptor : subresourceSrvDescriptors) {
        if (srvDescriptor.cpuDescriptorHandle.ptr != 0) {
            D3D12Renderer::GetRenderer()->resCpuDescriptorPool->Free(srvDescriptor.cpuDescriptorHandle);
        }
    }
    subresourceSrvDescriptors.Clear();

    for (const D3D12UAVDescriptor &uavDescriptor : subresourceUavDescriptors) {
        if (uavDescriptor.cpuDescriptorHandle.ptr != 0) {
            D3D12Renderer::GetRenderer()->uavCpuDescriptorPool->Free(uavDescriptor.cpuDescriptorHandle);
        }
    }
    subresourceUavDescriptors.Clear();

#ifdef USE_D3D12_MEMALLOC
    SAFE_RELEASE(bufferAllocation);
#else
    SAFE_RELEASE(bufferResource);
#endif
}

bool D3D12Buffer::IsValidSubresource(RHI::SubresourceType type, int subresourceIndex) const {
    if (type == RHI::SubresourceType::SRV) {
        if (subresourceIndex == -1) {
            return !!srvDescriptor.cpuDescriptorHandle.ptr;
        }
        return subresourceSrvDescriptors.IsValidIndex(subresourceIndex);
    }
    if (type == RHI::SubresourceType::UAV) {
        if (subresourceIndex == -1) {
            return !!uavDescriptor.cpuDescriptorHandle.ptr;
        }
        return subresourceUavDescriptors.IsValidIndex(subresourceIndex);
    }
    return false;
}

RHI::Buffer *D3D12Renderer::CreateBuffer(RHI::BufferUsage usage, RHI::ResourceFlag flags, uint64_t size, BE1::Image::Format format, uint32_t structureByteStride, const void *data) {
    // structureByteStride 는 4 의 배수 정렬 & 2048 보다 작아야 한다.
    assert(BE1::IsAligned(structureByteStride, 4) && structureByteStride < 2048);

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

    uint32_t alignedSize = size;

    if (BE1::HasFlag(flags, RHI::ResourceFlag::ConstantBuffer)) {
        // 상수 버퍼를 생성하면 내부적으로 GPU 에서 어차피 256 바이트로 사이즈가 정렬된다.
        alignedSize = BE1::AlignUp(alignedSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    }

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = alignedSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN; // 버퍼 리소스는 항상 DXGI_FORMAT_UNKNOWN 으로 생성한다.
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = resourceFlags;

#ifdef USE_D3D12_MEMALLOC
    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    //allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_CAN_ALIAS;
    allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_TIME;
    allocationDesc.HeapType = heapType;

    // NOTE: 내부적으로 Placed Resource 를 생성할 때, UAV/RTV/DSV 이거나 크기가 64kb 를 넘어간다면,
    // 오프셋을 64kb 에 정렬하고, 그게 아니면 4kb 에 정렬한다.
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
    buffer->flags = flags;

#ifdef USE_D3D12_MEMALLOC
    buffer->bufferAllocation = bufferAllocation;
#else
    buffer->bufferResource = bufferResource;
#endif

    buffer->size = alignedSize;

    // format == Image::Format::Unknown 이고, structureByteStride > 0 이면 structured buffer 다.
    // format == Image::Format::Unknown 이고, structureByteStride == 0 이면 raw buffer 다.
    buffer->format = format;

    buffer->structureByteStride = structureByteStride;

    ID3D12Resource *uploadBuffer = nullptr;

    if (data) {
        if (heapType == D3D12_HEAP_TYPE_DEFAULT) {
            D3D12_RESOURCE_DESC uploadBufferDesc;
            uploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            uploadBufferDesc.Alignment = 0;
            uploadBufferDesc.Width = alignedSize;
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
            D3D12CommandList *commandList = static_cast<D3D12CommandList *>(BeginCommandList(RHI::CommandQueueType::Graphics));
            commandList->ResourceBarrier(bufferResource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
            commandList->GetGraphicsCommandList()->CopyBufferRegion(bufferResource, 0, uploadBuffer, 0, size);
            commandList->ResourceBarrier(bufferResource, D3D12_RESOURCE_STATE_COPY_DEST, afterResourceState);
            commandList->CloseAndExecute();
            EndCommandList(commandList);
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

int D3D12Renderer::CreateSubresource(RHI::Buffer *buffer, RHI::SubresourceType type, uint64_t offset, uint64_t size, const BE1::Image::Format *newFormat) {
    D3D12Buffer *d3d12Buffer = static_cast<D3D12Buffer *>(buffer);

    if (type == RHI::SubresourceType::SRV) {
        return CreateSubresourceSRV(d3d12Buffer, offset, size, newFormat);
    }
    if (type == RHI::SubresourceType::UAV) {
        return CreateSubresourceUAV(d3d12Buffer, offset, size, newFormat);
    }
    return -1;
}

void D3D12Renderer::DestroySubresource(RHI::Buffer *buffer, RHI::SubresourceType type, int subresourceIndex) {
    D3D12Buffer *d3d12Buffer = static_cast<D3D12Buffer *>(buffer);

    if (type == RHI::SubresourceType::SRV) {
        if (subresourceIndex < 0) {
            if (d3d12Buffer->srvDescriptor.cpuDescriptorHandle.ptr) {
                resCpuDescriptorPool->Free(d3d12Buffer->srvDescriptor.cpuDescriptorHandle);
                d3d12Buffer->srvDescriptor = {};
            }
        } else {
            if (!d3d12Buffer->subresourceSrvDescriptors.IsValidIndex(subresourceIndex)) {
                BE_ERRLOG("D3D12Renderer::DestroySubresource: Invalid SRV subresource index (%i)\n", subresourceIndex);
                return;
            }
            resCpuDescriptorPool->Free(d3d12Buffer->subresourceSrvDescriptors[subresourceIndex].cpuDescriptorHandle);
            d3d12Buffer->subresourceSrvDescriptors.RemoveIndexFast(subresourceIndex);
        }
        return;
    }
    if (type == RHI::SubresourceType::UAV) {
        if (subresourceIndex < 0) {
            if (d3d12Buffer->uavDescriptor.cpuDescriptorHandle.ptr) {
                uavCpuDescriptorPool->Free(d3d12Buffer->uavDescriptor.cpuDescriptorHandle);
                d3d12Buffer->uavDescriptor = {};
            }
        } else {
            if (!d3d12Buffer->subresourceUavDescriptors.IsValidIndex(subresourceIndex)) {
                BE_ERRLOG("D3D12Renderer::DestroySubresource: Invalid UAV subresource index (%i)\n", subresourceIndex);
                return;
            }
            uavCpuDescriptorPool->Free(d3d12Buffer->subresourceUavDescriptors[subresourceIndex].cpuDescriptorHandle);
            d3d12Buffer->subresourceUavDescriptors.RemoveIndexFast(subresourceIndex);
        }
        return;
    }
}

int D3D12Renderer::CreateSubresourceSRV(D3D12Buffer *buffer, uint64_t offset, uint64_t size, const BE1::Image::Format *newFormat) {
    D3D12SRVDescriptor srvDescriptor;
    if (!resCpuDescriptorPool->Alloc(&srvDescriptor.cpuDescriptorHandle, nullptr)) {
        return -1;
    }

    srvDescriptor.srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDescriptor.srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    BE1::Image::Format subresourceFormat = buffer->format;
    if (newFormat) {
        // NOTE: 반드시 다른 서브 리소스의 포맷과 호환되어야 한다.
        subresourceFormat = *newFormat;
    }

    uint32_t byteStride = 0;

    if (subresourceFormat == BE1::Image::Format::Unknown) {
        if (buffer->structureByteStride == 0) {
            // Raw buffer (4 바이트 정렬된, 바이트 단위 접근이 가능한 버퍼)
            byteStride = 4;
            srvDescriptor.srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            srvDescriptor.srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
        } else {
            // Structured buffer
            byteStride = buffer->structureByteStride;
            srvDescriptor.srvDesc.Format = DXGI_FORMAT_UNKNOWN;
            srvDescriptor.srvDesc.Buffer.StructureByteStride = byteStride;
        }
    } else {
        // Typed buffer
        // 버퍼의 format 에 따라 swizzling 이 필요할 수도 있다.
        srvDescriptor.srvDesc.Shader4ComponentMapping = GetComponentSwizzling(subresourceFormat);

        byteStride = BE1::Image::BytesPerPixel(subresourceFormat);

        ImageFormatToDXGIFormat(subresourceFormat, false, &srvDescriptor.srvDesc.Format);
    }

    srvDescriptor.srvDesc.Buffer.FirstElement = offset / byteStride;
    srvDescriptor.srvDesc.Buffer.NumElements = BE1::Min(size, buffer->size - offset) / byteStride;

    device->CreateShaderResourceView(buffer->GetResource(), &srvDescriptor.srvDesc, srvDescriptor.cpuDescriptorHandle);

    // 전체 리소스에 대한 SRV
    if (!buffer->srvDescriptor.cpuDescriptorHandle.ptr) {
        buffer->srvDescriptor = srvDescriptor;
        return -1;
    }
    return buffer->subresourceSrvDescriptors.Append(srvDescriptor);
}

int D3D12Renderer::CreateSubresourceUAV(D3D12Buffer *buffer, uint64_t offset, uint64_t size, const BE1::Image::Format *newFormat) {
    D3D12UAVDescriptor uavDescriptor;
    if (!uavCpuDescriptorPool->Alloc(&uavDescriptor.cpuDescriptorHandle, nullptr)) {
        return -1;
    }

    // UAV 는 GPU 디스크립터도 같이 할당한다.
    D3D12_CPU_DESCRIPTOR_HANDLE destCpuDescriptorHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE destGpuDescriptorHandle;

    if (!uavGpuDescriptorPool->Alloc(&destCpuDescriptorHandle, &destGpuDescriptorHandle)) {
        return -1;
    }

    uavDescriptor.uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

    BE1::Image::Format subresourceFormat = buffer->format;
    if (newFormat) {
        // NOTE: 반드시 다른 서브 리소스의 포맷과 호환되어야 한다.
        subresourceFormat = *newFormat;
    }

    uint32_t byteStride = 0;

    if (subresourceFormat == BE1::Image::Format::Unknown) {
        if (buffer->structureByteStride == 0) {
            // Raw buffer (4 바이트 정렬된, 바이트 단위 접근이 가능한 버퍼)
            byteStride = 4;
            uavDescriptor.uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            uavDescriptor.uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
        } else {
            // Structured buffer
            byteStride = buffer->structureByteStride;
            uavDescriptor.uavDesc.Format = DXGI_FORMAT_UNKNOWN;
            uavDescriptor.uavDesc.Buffer.StructureByteStride = byteStride;
        }
    } else {
        // Typed buffer
        byteStride = BE1::Image::BytesPerPixel(subresourceFormat);

        ImageFormatToDXGIFormat(subresourceFormat, false, &uavDescriptor.uavDesc.Format);
    }

    uavDescriptor.uavDesc.Buffer.FirstElement = offset / byteStride;
    uavDescriptor.uavDesc.Buffer.NumElements = BE1::Min(size, buffer->size - offset) / byteStride;

    device->CreateUnorderedAccessView(buffer->GetResource(), nullptr, &uavDescriptor.uavDesc, uavDescriptor.cpuDescriptorHandle);

    // 만들어진 UAV 디스크립터를 shader visible 한 디스크립터에 복사 (CPU + GPU)
    device->CopyDescriptorsSimple(1, destCpuDescriptorHandle, uavDescriptor.cpuDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 전체 리소스에 대한 UAV
    if (!buffer->uavDescriptor.cpuDescriptorHandle.ptr) {
        buffer->uavDescriptor = uavDescriptor;
        return -1;
    }
    return buffer->subresourceUavDescriptors.Append(uavDescriptor);
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
    uint8_t descriptorIndex = 0xFF;

    if (shaderWritable) {
        // UAV
        rootParameterIndex = binder.rootParameterBinder.uav[slot];
        descriptorIndex = binder.descriptorTableBinder.uav[slot];
        if (descriptorIndex != 0xFF) {
            const D3D12UAVDescriptor &uavDescriptor = subresourceIndex < 0 ? d3d12Buffer->uavDescriptor : d3d12Buffer->subresourceUavDescriptors[subresourceIndex];
            threadData->tableCpuDescriptorHandles[rootParameterIndex][descriptorIndex] = uavDescriptor.cpuDescriptorHandle;
            if (threadData->tableCpuDescriptorHandles[rootParameterIndex][descriptorIndex].ptr == 0) {
                BE_ERRLOG("D3D12Renderer::SetBuffer: Buffer has no valid UAV descriptor handle\n");
                return;
            }
        }
        threadData->uavResources[slot] = d3d12Buffer;
    } else {
        // SRV
        rootParameterIndex = binder.rootParameterBinder.srv[slot];
        descriptorIndex = binder.descriptorTableBinder.srv[slot];
        if (descriptorIndex != 0xFF) {
            const D3D12SRVDescriptor &srvDescriptor = subresourceIndex < 0 ? d3d12Buffer->srvDescriptor : d3d12Buffer->subresourceSrvDescriptors[subresourceIndex];
            threadData->tableCpuDescriptorHandles[rootParameterIndex][descriptorIndex] = srvDescriptor.cpuDescriptorHandle;
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

    if (d3d12CommandList->currentPSO->graphics) {
        d3d12CommandList->graphicsRootParametersDirtyMask |= BIT64(rootParameterIndex);
    } else {
        d3d12CommandList->computeRootParametersDirtyMask |= BIT64(rootParameterIndex);
    }
}
