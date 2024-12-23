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

void D3D12Buffer::Release() {
#ifdef USE_D3D12_MEMALLOC
    SAFE_RELEASE(bufferAllocation);
#else
    SAFE_RELEASE(bufferResource);
#endif
}

ID3D12Resource *D3D12Buffer::GetResource() const {
#ifdef USE_D3D12_MEMALLOC
    return bufferAllocation->GetResource();
#else
    return bufferResource;
#endif
}

uint64_t D3D12Buffer::GetSize() {
#ifdef USE_D3D12_MEMALLOC
    return bufferAllocation->GetSize();
#else
    D3D12_RESOURCE_DESC resourceDesc = bufferResource->GetDesc();
    Image::Format::Enum imageFormat;
    if (D3D12Renderer::DXGIFormatToImageFormat(resourceDesc.Format, &imageFormat, nullptr)) {
        return Image::MemRequired(resourceDesc.Width, resourceDesc.Height, resourceDesc.DepthOrArraySize, resourceDesc.MipLevels, imageFormat);
    }
    return resourceDesc.Width;
#endif
}

RHIRenderer::Buffer *D3D12Renderer::CreateBuffer(BufferUsage usage, int flags, uint32_t size) {
    D3D12_HEAP_TYPE heapType;
    D3D12_RESOURCE_STATES initialState;
    D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;

    switch (usage) {
    case RHIRenderer::BufferUsage::Default:
        heapType = D3D12_HEAP_TYPE_DEFAULT;
        // NOTE: 텍스처와 달리 D3D12_RESOURCE_DIMENSION_BUFFER 리소스의 initialState 는 항상 D3D12_RESOURCE_STATE_COMMON 로 설정된다.
        initialState = D3D12_RESOURCE_STATE_COMMON;
        break;
    case RHIRenderer::BufferUsage::Upload:
        heapType = D3D12_HEAP_TYPE_UPLOAD;
        // NOTE: 리소스가 업로드 힙인 경우 initialState 는 다른 값으로 설정해도 무시되며, 항상 D3D12_RESOURCE_STATE_GENERIC_READ 로 설정된다.
        initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
        break;
    case RHIRenderer::BufferUsage::Readback:
        heapType = D3D12_HEAP_TYPE_READBACK;
        // NOTE: 리소스가 리드백 힙인 경우 initialState 는 다른 값으로 설정해도 무시되며, 항상 D3D12_RESOURCE_STATE_COPY_DEST 로 설정된다.
        initialState = D3D12_RESOURCE_STATE_COPY_DEST;
        resourceFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        break;
    default:
        return nullptr;
    }

    if (!(flags & BufferFlag::ShaderResource)) {
        resourceFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    }
    if (flags & BufferFlag::UnorderedAccess) {
        resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    if (flags & BufferFlag::ConstantBuffer) {
        // 상수 버퍼는 어차피 GPU 에 요청하면 256 바이트로 주소 & 사이즈가 정렬된다.
        size = AlignUp(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    }

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.MipLevels = 1;
    bufferDesc.Width = size;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
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

    D3D12Buffer* buffer = new D3D12Buffer;
#ifdef USE_D3D12_MEMALLOC
    buffer->bufferAllocation = bufferAllocation;
#else
    buffer->bufferResource = bufferResource;
#endif
    return buffer;
}

void D3D12Renderer::DestroyBuffer(Buffer *buffer, bool immediate) {
    if (immediate) {
        delete buffer;
    } else {
        MarkForDelete(buffer);
    }
}
