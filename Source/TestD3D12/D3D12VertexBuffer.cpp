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
#include "D3D12VertexBuffer.h"
#include "D3D12Buffer.h"
#include "D3D12Renderer.h"

void D3D12VertexBuffer::Release() {
    SAFE_DELETE(buffer);
}

D3D12VertexBuffer* D3D12VertexBuffer::CreateVertexBuffer(D3D12VertexBuffer::Type::Enum type, int vertexSize, int numVerts, void *data) {
    UINT bufferSize = vertexSize * numVerts;
    D3D12Buffer *buffer = nullptr;

    if (type == D3D12VertexBuffer::Type::Static) {
        buffer = D3D12Buffer::CreateGPUBuffer(bufferSize);
    } else {
        buffer = D3D12Buffer::CreateCPUBuffer(bufferSize);
    }

    if (!buffer) {
        return nullptr;
    }

    ID3D12Resource *bufferResource = buffer->GetResource();
    ID3D12Resource* uploadBuffer = nullptr;

    if (data) {
        if (type == D3D12VertexBuffer::Type::Static) {
            // CPU 에서 GPU 로 전송할 업로드 버퍼 생성
            if (FAILED(renderer.device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr, IID_PPV_ARGS(&uploadBuffer)))) {
                SAFE_DELETE(buffer);
                return nullptr;
            }

            UINT8* mappedPtr = nullptr;
            CD3DX12_RANGE writeRange(0, 0);
            uploadBuffer->Map(0, &writeRange, reinterpret_cast<void **>(&mappedPtr));
            memcpy(mappedPtr, data, bufferSize);
            uploadBuffer->Unmap(0, nullptr);

            // 업로드 버퍼에서 GPU 버퍼로 데이터 카피
            renderer.commandAllocator->Reset();
            renderer.commandList->Reset(renderer.commandAllocator, nullptr);
            renderer.commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(bufferResource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
            renderer.commandList->CopyBufferRegion(bufferResource, 0, uploadBuffer, 0, bufferSize);
            renderer.commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(bufferResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
            renderer.commandList->Close();

            // 커맨드 큐 실행
            ID3D12CommandList *ppCommandLists[] = { renderer.commandList };
            renderer.commandQueue->ExecuteCommandLists(COUNT_OF(ppCommandLists), ppCommandLists);
        } else if (type == D3D12VertexBuffer::Type::Dynamic) {
            UINT8 *mappedPtr = nullptr;
            CD3DX12_RANGE range(0, 0);
            bufferResource->Map(0, &range, reinterpret_cast<void **>(&mappedPtr));
            memcpy(mappedPtr, data, bufferSize);
            bufferResource->Unmap(0, nullptr);
        } else {
            SAFE_DELETE(buffer);
            return nullptr;
        }
    }

    if (uploadBuffer) {
        renderer.MarkForRelease(uploadBuffer);
    }

    D3D12VertexBuffer* vertexBuffer = new D3D12VertexBuffer;
    vertexBuffer->buffer = buffer;

    vertexBuffer->vbv.BufferLocation = bufferResource->GetGPUVirtualAddress();
    vertexBuffer->vbv.StrideInBytes = vertexSize;
    vertexBuffer->vbv.SizeInBytes = bufferSize;

    return vertexBuffer;
}
