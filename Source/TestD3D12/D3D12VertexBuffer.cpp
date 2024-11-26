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
        buffer = D3D12Buffer::CreateBuffer(D3D12Buffer::Usage::Default, bufferSize);
    } else {
        buffer = D3D12Buffer::CreateBuffer(D3D12Buffer::Usage::Upload, bufferSize);
    }

    if (!buffer) {
        return nullptr;
    }

    ID3D12Resource *bufferResource = buffer->GetResource();
    ID3D12Resource* uploadBuffer = nullptr;

    if (data) {
        if (type == D3D12VertexBuffer::Type::Static) {
            D3D12_HEAP_PROPERTIES heapProperties;
            heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
            heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heapProperties.CreationNodeMask = 1;
            heapProperties.VisibleNodeMask = 1;

            // CPU 에서 GPU 로 전송할 업로드 버퍼 생성
            if (FAILED(renderer.device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr, IID_PPV_ARGS(&uploadBuffer)))) {
                SAFE_DELETE(buffer);
                return nullptr;
            }

            UINT8* mappedPtr = nullptr;
            uploadBuffer->Map(0, nullptr, reinterpret_cast<void **>(&mappedPtr));

            simdProcessor->MemcpyStream(mappedPtr, data, bufferSize);

            CD3DX12_RANGE writtenRange(0, bufferSize);
            uploadBuffer->Unmap(0, &writtenRange);

            // 업로드 버퍼에서 GPU 버퍼로 데이터 카피
            renderer.commandAllocator->Reset();
            renderer.commandList->Reset(renderer.commandAllocator, nullptr);
            renderer.commandList->CopyBufferRegion(bufferResource, 0, uploadBuffer, 0, bufferSize);
            renderer.commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(bufferResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
            renderer.commandList->Close();

            // CommandQueue 실행
            ID3D12CommandList *execCommandLists[] = { renderer.commandList };
            renderer.commandQueue->ExecuteCommandLists(COUNT_OF(execCommandLists), execCommandLists);
        } else if (type == D3D12VertexBuffer::Type::Dynamic) {
            UINT8 *mappedPtr = nullptr;
            bufferResource->Map(0, nullptr, reinterpret_cast<void **>(&mappedPtr));

            simdProcessor->MemcpyStream(mappedPtr, data, bufferSize);

            CD3DX12_RANGE writtenRange(0, 0);
            bufferResource->Unmap(0, &writtenRange);
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
