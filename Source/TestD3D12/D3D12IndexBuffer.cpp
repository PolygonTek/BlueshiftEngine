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
#include "D3D12IndexBuffer.h"
#include "D3D12Renderer.h"

void D3D12IndexBuffer::Release() {
#ifdef USE_D3D12_MEMALLOC
    SAFE_RELEASE(indexBufferAllocation);
#else
    SAFE_RELEASE(indexBufferResource);
#endif
}

D3D12IndexBuffer *D3D12IndexBuffer::CreateIndexBuffer(int indexSize, int numIndexes, void *data) {
    assert(indexSize == 2 || indexSize == 4);

    UINT bufferSize = indexSize * numIndexes;
    D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT; // D3D12_HEAP_TYPE_UPLOAD

    // GPU 에 버텍스 버퍼 생성
    D3D12_RESOURCE_DESC indexBufferDesc = {};
    indexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    indexBufferDesc.Alignment = 0;
    indexBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    indexBufferDesc.MipLevels = 1;
    indexBufferDesc.Width = bufferSize;
    indexBufferDesc.Height = 1;
    indexBufferDesc.DepthOrArraySize = 1;
    indexBufferDesc.SampleDesc.Count = 1;
    indexBufferDesc.SampleDesc.Quality = 0;
    indexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    indexBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

#ifdef USE_D3D12_MEMALLOC
    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    //allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_CAN_ALIAS;
    allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_TIME;
    allocationDesc.HeapType = heapType;

    D3D12MA::Allocation *allocation;
    if (FAILED(renderer.allocator->CreateResource(
        &allocationDesc,
        &indexBufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        &allocation,
        IID_NULL, nullptr))) {
        return nullptr;
    }
    ID3D12Resource *indexBufferResource = allocation->GetResource();
#else
    ID3D12Resource *indexBufferResource = nullptr;
    if (FAILED(renderer.device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(heapType),
        D3D12_HEAP_FLAG_NONE,
        &indexBufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr, IID_PPV_ARGS(&indexBufferResource)))) {
        return nullptr;
    }
#endif

    ID3D12Resource* uploadBuffer = nullptr;

    if (data) {
        if (heapType == D3D12_HEAP_TYPE_DEFAULT) {
            // CPU 에서 GPU 로 업로드할 버텍스 버퍼 생성
            if (FAILED(renderer.device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr, IID_PPV_ARGS(&uploadBuffer)))) {
                indexBufferResource->Release();
                return nullptr;
            }

            UINT8* mappedPtr = nullptr;
            CD3DX12_RANGE writeRange(0, 0);
            uploadBuffer->Map(0, &writeRange, reinterpret_cast<void **>(&mappedPtr));
            memcpy(mappedPtr, data, bufferSize);
            uploadBuffer->Unmap(0, nullptr);

            // 업로드 버퍼에서 인덱스 버퍼로 데이터 카피
            renderer.commandAllocator->Reset();
            renderer.commandList->Reset(renderer.commandAllocator, nullptr);
            renderer.commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(indexBufferResource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
            renderer.commandList->CopyBufferRegion(indexBufferResource, 0, uploadBuffer, 0, bufferSize);
            renderer.commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(indexBufferResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));
            renderer.commandList->Close();

            // 커맨드 큐 실행
            ID3D12CommandList *ppCommandLists[] = { renderer.commandList };
            renderer.commandQueue->ExecuteCommandLists(COUNT_OF(ppCommandLists), ppCommandLists);
        } else if (heapType == D3D12_HEAP_TYPE_UPLOAD) {
            UINT8* mappedPtr = nullptr;
            CD3DX12_RANGE range(0, 0);
            indexBufferResource->Map(0, &range, reinterpret_cast<void **>(&mappedPtr));
            memcpy(mappedPtr, data, bufferSize);
            indexBufferResource->Unmap(0, nullptr);
        } else {
            indexBufferResource->Release();
            return nullptr;
        }
    }

    if (uploadBuffer) {
        renderer.MarkForRelease(uploadBuffer);
    }

    D3D12IndexBuffer *indexBuffer = new D3D12IndexBuffer;
#ifdef USE_D3D12_MEMALLOC
    indexBuffer->indexBufferAllocation = allocation;
#else
    indexBuffer->indexBufferResource = indexBufferResource;
#endif
    indexBuffer->ibv.BufferLocation = indexBufferResource->GetGPUVirtualAddress();
    indexBuffer->ibv.Format = (indexSize == sizeof(uint16_t) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
    indexBuffer->ibv.SizeInBytes = bufferSize;

    return indexBuffer;
}
