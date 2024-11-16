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
    SAFE_RELEASE(indexBufferResource);
}

D3D12IndexBuffer *D3D12IndexBuffer::CreateIndexBuffer(int indexSize, int numIndexes, void *data) {
    assert(indexSize == 2 || indexSize == 4);

    ID3D12Resource* indexBufferResource = nullptr;
    UINT bufferSize = indexSize * numIndexes;
    D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT; // D3D12_HEAP_TYPE_UPLOAD

    // GPU 에 버텍스 버퍼 생성
    if (FAILED(renderer.device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(heapType),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr, IID_PPV_ARGS(&indexBufferResource)))) {
        return nullptr;
    }

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
            CD3DX12_RANGE readRange(0, 0);
            indexBufferResource->Map(0, &readRange, reinterpret_cast<void **>(&mappedPtr));
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
    indexBuffer->indexBufferResource = indexBufferResource;
    indexBuffer->ibv.BufferLocation = indexBufferResource->GetGPUVirtualAddress();
    indexBuffer->ibv.Format = (indexSize == sizeof(uint16_t) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
    indexBuffer->ibv.SizeInBytes = bufferSize;

    return indexBuffer;
}
