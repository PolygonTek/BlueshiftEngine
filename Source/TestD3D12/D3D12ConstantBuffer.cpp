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
#include "D3D12ConstantBuffer.h"
#include "D3D12Renderer.h"

void D3D12ConstantBuffer::Release() {
#ifdef USE_D3D12_MEMALLOC
    SAFE_RELEASE(constantBufferAllocation);
#else
    SAFE_RELEASE(constantBufferResource);
#endif
}

void *D3D12ConstantBuffer::Map(SIZE_T begin, SIZE_T end) {
    void* mappedBase = nullptr;
    CD3DX12_RANGE range(0, 0); // We do not intend to read from this resource on the CPU.
#ifdef USE_D3D12_MEMALLOC
    ID3D12Resource* resource = constantBufferAllocation->GetResource();
#else
    ID3D12Resource *resource = constantBufferResource;
#endif
    resource->Map(0, &range, reinterpret_cast<void **>(&mappedBase));
    return mappedBase;
}

void D3D12ConstantBuffer::Unmap() {
#ifdef USE_D3D12_MEMALLOC
    ID3D12Resource *resource = constantBufferAllocation->GetResource();
#else
    ID3D12Resource *resource = constantBufferResource;
#endif
    resource->Unmap(0, nullptr);
}

D3D12ConstantBuffer* D3D12ConstantBuffer::CreateConstantBuffer(int size) {
    // 상수 버퍼는 어차피 GPU 에 요청하면 256 바이트로 주소 & 사이즈가 정렬된다.
    UINT alignedSize = (UINT)AlignUp(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    // CPU 에 버텍스 버퍼 생성
    D3D12_RESOURCE_DESC constantBufferDesc = {};
    constantBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    constantBufferDesc.Alignment = 0;
    constantBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    constantBufferDesc.MipLevels = 1;
    constantBufferDesc.Width = alignedSize;
    constantBufferDesc.Height = 1;
    constantBufferDesc.DepthOrArraySize = 1;
    constantBufferDesc.SampleDesc.Count = 1;
    constantBufferDesc.SampleDesc.Quality = 0;
    constantBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    constantBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

#ifdef USE_D3D12_MEMALLOC
    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    //allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_CAN_ALIAS;
    //allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_TIME;
    allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

    D3D12MA::Allocation *allocation;
    renderer.allocator->CreateResource(
        &allocationDesc,
        &constantBufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        &allocation,
        IID_NULL, nullptr);
    ID3D12Resource *constantBufferResource = allocation->GetResource();
#else
    ID3D12Resource *constantBufferResource;
    renderer.device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &constantBufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr, IID_PPV_ARGS(&constantBufferResource));
#endif

    D3D12ConstantBuffer* constantBuffer = new D3D12ConstantBuffer;
#ifdef USE_D3D12_MEMALLOC
    constantBuffer->constantBufferAllocation = allocation;
#else
    constantBuffer->constantBufferResource = constantBufferResource;
#endif

    return constantBuffer;
}
