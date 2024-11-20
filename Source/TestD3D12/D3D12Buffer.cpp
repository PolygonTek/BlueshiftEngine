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
#include "D3D12Buffer.h"
#include "D3D12Renderer.h"

void D3D12Buffer::Release() {
#ifdef USE_D3D12_MEMALLOC
    SAFE_RELEASE(bufferAllocation);
#else
    SAFE_RELEASE(bufferResource);
#endif
}

ID3D12Resource *D3D12Buffer::GetResource() {
#ifdef USE_D3D12_MEMALLOC
    return bufferAllocation->GetResource();
#else
    return bufferResource;
#endif
}

UINT D3D12Buffer::GetSize() {
#ifdef USE_D3D12_MEMALLOC
    return bufferAllocation->GetSize();
#else
    D3D12_RESOURCE_DESC resourceDesc = bufferResource->GetDesc();
    return resourceDesc.Width;
#endif
}

D3D12Buffer* D3D12Buffer::CreateBuffer(D3D12Buffer::Usage::Enum usage, int size) {
    D3D12_HEAP_TYPE heapType;
    D3D12_RESOURCE_STATES initialState;

    switch (usage) {
    case D3D12Buffer::Usage::Default:
        heapType = D3D12_HEAP_TYPE_DEFAULT;
        initialState = D3D12_RESOURCE_STATE_COPY_DEST;
        break;
    case D3D12Buffer::Usage::Upload:
        heapType = D3D12_HEAP_TYPE_UPLOAD;
        initialState = D3D12_RESOURCE_STATE_COMMON;
        break;
    case D3D12Buffer::Usage::Readback:
        heapType = D3D12_HEAP_TYPE_READBACK;
        initialState = D3D12_RESOURCE_STATE_COPY_DEST;
        break;
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
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

#ifdef USE_D3D12_MEMALLOC
    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    //allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_CAN_ALIAS;
    allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_TIME;
    allocationDesc.HeapType = heapType;

    D3D12MA::Allocation *bufferAllocation;
    if (FAILED(renderer.allocator->CreateResource(
        &allocationDesc,
        &bufferDesc,
        initialState,
        nullptr,
        &bufferAllocation,
        IID_NULL, nullptr))) {
        return nullptr;
    }
#else
    ID3D12Resource *bufferResource = nullptr;
    if (FAILED(renderer.device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(heapType),
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
