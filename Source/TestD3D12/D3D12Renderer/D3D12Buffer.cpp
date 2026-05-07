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
#include "D3D12FrameThreadData.h"

void D3D12Buffer::Release() {
    for (const D3D12SRVEntry &srvEntry : srvEntries) {
        if (srvEntry.cpuDescriptorHandle.ptr != 0) {
            D3D12Renderer::GetRenderer()->resCpuDescriptorPool->Free(srvEntry.cpuDescriptorHandle);
        }
    }
    srvEntries.Clear();

    for (const D3D12UAVEntry &uavEntry : uavEntries) {
        if (uavEntry.cpuDescriptorHandle.ptr != 0) {
            D3D12Renderer::GetRenderer()->resCpuDescriptorPool->Free(uavEntry.cpuDescriptorHandle);
        }
    }
    uavEntries.Clear();

#ifdef USE_D3D12_MEMALLOC
    SAFE_RELEASE(bufferAllocation);
#else
    SAFE_RELEASE(bufferResource);
#endif
}

bool D3D12Buffer::HasValidView(RHI::ViewType viewType, RHI::ViewID viewID) const {
    if (!viewID.IsValid()) {
        return false;
    }
    if (viewType == RHI::ViewType::SRV) {
        return srvEntries.IsValidIndex(viewID.value) && srvEntries[viewID.value].cpuDescriptorHandle.ptr != 0;
    }
    if (viewType == RHI::ViewType::UAV) {
        return uavEntries.IsValidIndex(viewID.value) && uavEntries[viewID.value].cpuDescriptorHandle.ptr != 0;
    }
    return false;
}

RHI::Buffer *D3D12Renderer::CreateBuffer(RHI::BufferUsage usage, RHI::BufferFlag flags, uint64_t size, const void *data) {
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

    if (BE1::HasFlag(flags, RHI::BufferFlag::UnorderedAccess)) {
        resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    uint32_t alignedSize = size;

    if (BE1::HasFlag(flags, RHI::BufferFlag::ConstantBuffer)) {
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

    // NOTE: 내부적으로 Placed Resource 를 생성할 때, UAV/RTV/DSV 이거나 크기가 64KB 를 넘어간다면,
    // 오프셋을 64KB 에 정렬하고, 그게 아니면 4KB 에 정렬한다.
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
    buffer->size = alignedSize;

#ifdef USE_D3D12_MEMALLOC
    buffer->bufferAllocation = bufferAllocation;
#else
    buffer->bufferResource = bufferResource;
#endif

    // 업로드 요청을 등록한다.
    if (data) {
        buffer->SetStatus(RHI::GPUResourceStatus::Pending);

        D3D12ResourceRequest request;
        request.type = D3D12ResourceRequest::Type::UploadBuffer;
        request.object = buffer;
        request.uploadBufferRequest.data = data;
        request.uploadBufferRequest.dataSize = size;
        request.uploadBufferRequest.heapType = heapType;
        request.uploadBufferRequest.resourceDesc = bufferDesc;
        request.uploadBufferRequest.initialState = initialState;
        EnqueueResourceRequest(std::move(request));
    }

    return buffer;
}

bool D3D12Renderer::ExecuteUploadBuffer(D3D12ResourceCommandContext &commandContext, D3D12Buffer *buffer, const D3D12UploadBufferRequest &request) {
    ID3D12Resource *uploadBuffer = nullptr;

    if (request.data) {
#ifdef USE_D3D12_MEMALLOC
        ID3D12Resource *bufferResource = buffer->bufferAllocation->GetResource();
#else
        ID3D12Resource *bufferResource = buffer->bufferResource;
#endif

        if (request.heapType == D3D12_HEAP_TYPE_DEFAULT) {
            D3D12_RESOURCE_DESC uploadBufferDesc;
            uploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            uploadBufferDesc.Alignment = 0;
            uploadBufferDesc.Width = request.resourceDesc.Width;
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
                return nullptr;
            }

            UINT8 *mappedPtr = nullptr;
            uploadBuffer->Map(0, nullptr, reinterpret_cast<void **>(&mappedPtr));

            BE1::simdProcessor->Memcpy(mappedPtr, request.data, request.dataSize);

            CD3DX12_RANGE writtenRange(0, request.dataSize);
            uploadBuffer->Unmap(0, &writtenRange);

            D3D12_RESOURCE_STATES afterResourceState = D3D12_RESOURCE_STATE_COMMON;
            if (BE1::HasAnyFlag(buffer->flags, RHI::BufferFlag::ConstantBuffer | RHI::BufferFlag::VertexBuffer)) {
                afterResourceState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            } else if (BE1::HasFlag(buffer->flags, RHI::BufferFlag::IndexBuffer)) {
                afterResourceState = D3D12_RESOURCE_STATE_INDEX_BUFFER;
            } else if (BE1::HasFlag(buffer->flags, RHI::BufferFlag::UnorderedAccess)) {
                afterResourceState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            }

            // 업로드 버퍼에서 디바이스 버퍼 (VRAM) 로 GPU 에 복사 요청
            D3D12CommandList *commandList = commandContext.GetGraphicsCommandList();
            commandList->ResourceBarrier(bufferResource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
            commandList->GetGraphicsCommandList()->CopyBufferRegion(bufferResource, 0, uploadBuffer, 0, request.dataSize);
            commandList->ResourceBarrier(bufferResource, D3D12_RESOURCE_STATE_COPY_DEST, afterResourceState);
        } else if (request.heapType == D3D12_HEAP_TYPE_UPLOAD) {
            UINT8 *mappedPtr = nullptr;
            bufferResource->Map(0, nullptr, reinterpret_cast<void **>(&mappedPtr));

            BE1::simdProcessor->Memcpy(mappedPtr, request.data, request.dataSize);

            CD3DX12_RANGE writtenRange(0, request.dataSize);
            bufferResource->Unmap(0, &writtenRange);
        } else {
            return nullptr;
        }
    }

    if (uploadBuffer) {
        RequestRelease(uploadBuffer);
    }

    return true;
}

RHI::ViewID D3D12Renderer::CreateView(RHI::Buffer *buffer, RHI::ViewType viewType, BE1::Image::Format format, uint32_t structuredStride, uint64_t offset, uint64_t size) {
    D3D12Buffer *_buffer = static_cast<D3D12Buffer *>(buffer);

    if (viewType == RHI::ViewType::SRV) {
        return CreateSRV(_buffer, format, structuredStride, offset, size);
    }
    if (viewType == RHI::ViewType::UAV) {
        return CreateUAV(_buffer, format, structuredStride, offset, size);
    }
    return RHI::ViewID::InvalidID;
}

void D3D12Renderer::DestroyView(RHI::Buffer *buffer, RHI::ViewType viewType, RHI::ViewID viewID) {
    D3D12Buffer *_buffer = static_cast<D3D12Buffer *>(buffer);

    if (viewType == RHI::ViewType::SRV) {
        if (!_buffer->srvEntries.IsValidIndex(viewID.value)) {
            BE_ERRLOG("D3D12Renderer::DestroyView: Invalid SRV view ID (%i)\n", viewID.value);
            return;
        }
        D3D12SRVEntry *srvEntryPtr = &_buffer->srvEntries[viewID.value];
        if (srvEntryPtr->cpuDescriptorHandle.ptr != 0) {
            resCpuDescriptorPool->Free(srvEntryPtr->cpuDescriptorHandle);
            *srvEntryPtr = {};
        }
        return;
    }
    if (viewType == RHI::ViewType::UAV) {
        if (!_buffer->uavEntries.IsValidIndex(viewID.value)) {
            BE_ERRLOG("D3D12Renderer::DestroyView: Invalid UAV view ID (%i)\n", viewID.value);
            return;
        }
        D3D12UAVEntry *uavEntryPtr = &_buffer->uavEntries[viewID.value];
        if (uavEntryPtr->cpuDescriptorHandle.ptr != 0) {
            resCpuDescriptorPool->Free(uavEntryPtr->cpuDescriptorHandle);
            *uavEntryPtr = {};
        }
        return;
    }
}

RHI::ViewID D3D12Renderer::CreateSRV(D3D12Buffer *buffer, BE1::Image::Format format, uint32_t structureByteStride, uint64_t offset, uint64_t size) {
    assert(offset < buffer->size);

    D3D12SRVEntry newSrvEntry;
    newSrvEntry.viewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

    uint32_t byteStride = 0;

    if (format != BE1::Image::Format::Unknown) {
        // Typed buffer
        // SRV 는 셰이더에서 .rgba 를 어떻게 보이게 할지 정할 수 있다.
        // 셰이더에서 .rgba 로 올바른 값으로 읽어오려면, 버퍼의 format 에 따라 swizzling 이 필요할 수 있다.
        newSrvEntry.viewDesc.Shader4ComponentMapping = GetComponentSwizzling(format);

        byteStride = BE1::Image::BytesPerPixel(format);

        ImageFormatToDXGIFormat(format, false, &newSrvEntry.viewDesc.Format);
    } else {
        newSrvEntry.viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        // format == Image::Format::Unknown 이고 structureByteStride == 0 이면, raw buffer 다.
        // format == Image::Format::Unknown 이고 structureByteStride > 0 이면, structured buffer 다.
        if (structureByteStride == 0) {
            // Raw buffer
            byteStride = 4;
            newSrvEntry.viewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            newSrvEntry.viewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
        } else {
            // structureByteStride 는 4의 배수이고, 2048 보다 작아야 한다.
            assert(BE1::IsAligned(structureByteStride, 4) && structureByteStride < 2048);

            // Structured buffer
            byteStride = structureByteStride;
            newSrvEntry.viewDesc.Format = DXGI_FORMAT_UNKNOWN;
            newSrvEntry.viewDesc.Buffer.StructureByteStride = structureByteStride;
        }
    }

    if (!BE1::IsAligned(offset, byteStride)) {
        BE_ERRLOG("D3D12Renderer::CreateSRV: offset must be aligned to element stride\n");
        return RHI::ViewID::InvalidID;
    }

    uint64_t viewSize = BE1::Min(size, buffer->size - offset);
    assert(viewSize > 0);
    assert(BE1::IsAligned(viewSize, byteStride));

    newSrvEntry.viewDesc.Buffer.FirstElement = offset / byteStride;
    newSrvEntry.viewDesc.Buffer.NumElements = viewSize / byteStride;

    // 디스크립터 슬롯 할당
    if (!resCpuDescriptorPool->Alloc(&newSrvEntry.cpuDescriptorHandle, nullptr)) {
        return RHI::ViewID::InvalidID;
    }

    // 디스크립터 슬롯에 SRV 를 기록
    device->CreateShaderResourceView(buffer->GetResource(), &newSrvEntry.viewDesc, newSrvEntry.cpuDescriptorHandle);

    // SRV 추가 후 viewID 를 리턴
    RHI::ViewID viewID;

    for (int i = 0; i < buffer->srvEntries.Count(); ++i) {
        if (!buffer->srvEntries[i].cpuDescriptorHandle.ptr) {
            buffer->srvEntries[i] = newSrvEntry;
            viewID.value = i;
            return viewID;
        }
    }
    viewID.value = buffer->srvEntries.Append(newSrvEntry);
    return viewID;
}

RHI::ViewID D3D12Renderer::CreateUAV(D3D12Buffer *buffer, BE1::Image::Format format, uint32_t structureByteStride, uint64_t offset, uint64_t size) {
    assert(offset < buffer->size);

    D3D12UAVEntry newUavEntry;
    newUavEntry.viewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

    uint32_t byteStride = 0;

    if (format != BE1::Image::Format::Unknown) {
        // Typed buffer
        byteStride = BE1::Image::BytesPerPixel(format);

        ImageFormatToDXGIFormat(format, false, &newUavEntry.viewDesc.Format);
    } else {
        // format == Image::Format::Unknown 이고 structureByteStride == 0 이면, raw buffer 다.
        // format == Image::Format::Unknown 이고 structureByteStride > 0 이면, structured buffer 다.
        if (structureByteStride == 0) {
            // Raw buffer
            byteStride = 4;
            newUavEntry.viewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            newUavEntry.viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
        } else {
            // structureByteStride 는 4의 배수이고, 2048 보다 작아야 한다.
            assert(BE1::IsAligned(structureByteStride, 4) && structureByteStride < 2048);

            // Structured buffer
            byteStride = structureByteStride;
            newUavEntry.viewDesc.Format = DXGI_FORMAT_UNKNOWN;
            newUavEntry.viewDesc.Buffer.StructureByteStride = structureByteStride;
        }
    }

    if (!BE1::IsAligned(offset, byteStride)) {
        BE_ERRLOG("D3D12Renderer::CreateUAV: offset must be aligned to element stride\n");
        return RHI::ViewID::InvalidID;
    }

    uint64_t viewSize = BE1::Min(size, buffer->size - offset);
    assert(viewSize > 0);
    assert(BE1::IsAligned(viewSize, byteStride));

    newUavEntry.viewDesc.Buffer.FirstElement = offset / byteStride;
    newUavEntry.viewDesc.Buffer.NumElements = viewSize / byteStride;

    // 디스크립터 슬롯 할당
    if (!resCpuDescriptorPool->Alloc(&newUavEntry.cpuDescriptorHandle, nullptr)) {
        return RHI::ViewID::InvalidID;
    }

    // 디스크립터 슬롯에 UAV 를 기록
    device->CreateUnorderedAccessView(buffer->GetResource(), nullptr, &newUavEntry.viewDesc, newUavEntry.cpuDescriptorHandle);

    // UAV 추가 후 viewID 를 리턴
    RHI::ViewID viewID;

    for (int i = 0; i < buffer->uavEntries.Count(); ++i) {
        if (!buffer->uavEntries[i].cpuDescriptorHandle.ptr) {
            buffer->uavEntries[i] = newUavEntry;
            viewID.value = i;
            return viewID;
        }
    }
    viewID.value = buffer->uavEntries.Append(newUavEntry);
    return viewID;
}

void D3D12Renderer::DestroyBuffer(RHI::Buffer *buffer, bool immediate) {
    if (immediate) {
        delete buffer;
    } else {
        DeleteGPUObject(buffer);
    }
}

void D3D12Renderer::SetBuffer(RHI::CommandList *commandList, int shaderRegister, bool shaderWritable, const RHI::Buffer *buffer, RHI::ViewID viewID) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    D3D12RootBindingData *rootBindingData = _commandList->GetRootBindingData();

    const D3D12Buffer *_buffer = static_cast<const D3D12Buffer *>(buffer);

    // 셰이더 레지스터에 대한 루트 파라미터 인덱스를 얻는다.
    const D3D12PipelineState::RegisterBinder &binder = _commandList->currentPSO->binder;
    uint8_t rootParameterIndex = 0xFF;
    uint8_t indexInTable = 0xFF;

    if (shaderWritable) {
        // UAV
        assert(shaderRegister < COUNT_OF(binder.rootParameterIndexLUT.uav));
        rootParameterIndex = binder.rootParameterIndexLUT.uav[shaderRegister];
        if (rootParameterIndex >= 64) {
            BE_ERRLOG("D3D12Renderer::SetBuffer: Invalid root parameter index\n");
            return;
        }

        // 루트 파라미터가 디스크립터 테이블일 경우 테이블 내의 인덱스 LUT 에는 유효한 값이 들어있다.
        indexInTable = binder.descriptorTableIndexLUT.uav[shaderRegister];
        if (indexInTable != 0xFF) {
            if (!_buffer->HasValidView(RHI::ViewType::UAV, viewID)) {
                BE_ERRLOG("D3D12Renderer::SetBuffer: Invalid UAV view ID (%i)\n", viewID.value);
                return;
            }

            // 사용할 디스크립터 테이블 변수에 디스크립터 핸들을 채워 넣는다.
            const D3D12UAVEntry &uavEntry = _buffer->uavEntries[viewID.value];
            rootBindingData->descriptorTableHandles[rootParameterIndex][indexInTable] = uavEntry.cpuDescriptorHandle;
            if (rootBindingData->descriptorTableHandles[rootParameterIndex][indexInTable].ptr == 0) {
                BE_ERRLOG("D3D12Renderer::SetBuffer: Buffer has no valid UAV descriptor handle\n");
                return;
            }
        }
        rootBindingData->uavResources[shaderRegister] = _buffer;
    } else {
        // SRV
        assert(shaderRegister < COUNT_OF(binder.rootParameterIndexLUT.srv));
        rootParameterIndex = binder.rootParameterIndexLUT.srv[shaderRegister];
        if (rootParameterIndex >= 64) {
            BE_ERRLOG("D3D12Renderer::SetBuffer: Invalid root parameter index\n");
            return;
        }

        // 루트 파라미터가 디스크립터 테이블일 경우 테이블 내의 인덱스 LUT 에는 유효한 값이 들어있다.
        indexInTable = binder.descriptorTableIndexLUT.srv[shaderRegister];
        if (indexInTable != 0xFF) {
            if (!_buffer->HasValidView(RHI::ViewType::SRV, viewID)) {
                BE_ERRLOG("D3D12Renderer::SetBuffer: Invalid SRV view ID (%i)\n", viewID.value);
                return;
            }

            // 사용할 디스크립터 테이블 변수에 디스크립터 핸들을 채워 넣는다.
            const D3D12SRVEntry &srvEntry = _buffer->srvEntries[viewID.value];
            rootBindingData->descriptorTableHandles[rootParameterIndex][indexInTable] = srvEntry.cpuDescriptorHandle;
            if (rootBindingData->descriptorTableHandles[rootParameterIndex][indexInTable].ptr == 0) {
                BE_ERRLOG("D3D12Renderer::SetBuffer: Buffer has no valid SRV descriptor handle\n");
                return;
            }
        }
        rootBindingData->srvResources[shaderRegister] = _buffer;
    }

    // 변경할 루트 파라미터 인덱스에 대한 dirty mask 를 설정한다.
    if (_commandList->currentPSO->isGraphics) {
        _commandList->graphicsRootParametersDirtyMask |= BIT64(rootParameterIndex);
    } else {
        _commandList->computeRootParametersDirtyMask |= BIT64(rootParameterIndex);
    }
}
