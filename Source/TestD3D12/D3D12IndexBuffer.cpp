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
#include "D3D12CommandListPool.h"
#include "D3D12IndexBuffer.h"
#include "D3D12Buffer.h"
#include "D3D12Renderer.h"

void D3D12IndexBuffer::Release() {
    SAFE_DELETE(buffer);
}

D3D12IndexBuffer *D3D12IndexBuffer::CreateIndexBuffer(D3D12IndexBuffer::Type::Enum type, int indexSize, int numIndexes, void *data) {
    assert(indexSize == 2 || indexSize == 4);

    UINT bufferSize = indexSize * numIndexes;
    D3D12Buffer *buffer = nullptr;

    if (type == D3D12IndexBuffer::Type::Static) {
        buffer = D3D12Buffer::CreateBuffer(D3D12Buffer::Usage::Default, bufferSize);
    } else {
        buffer = D3D12Buffer::CreateBuffer(D3D12Buffer::Usage::Upload, bufferSize);
    }

    if (!buffer) {
        return nullptr;
    }

    ID3D12Resource* bufferResource = buffer->GetResource();
    ID3D12Resource* uploadBuffer = nullptr;

    if (data) {
        if (type == D3D12IndexBuffer::Type::Static) {
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
            if (FAILED(renderer.device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &uploadBufferDesc,
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
            renderer.resourceCommandList->Reset();
            renderer.resourceCommandList->commandList->CopyBufferRegion(bufferResource, 0, uploadBuffer, 0, bufferSize);
            renderer.resourceCommandList->ResourceBarrier(bufferResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
            renderer.resourceCommandList->CloseAndExecute();
        } else if (type == D3D12IndexBuffer::Type::Dynamic) {
            UINT8* mappedPtr = nullptr;
            bufferResource->Map(0, nullptr, reinterpret_cast<void **>(&mappedPtr));

            simdProcessor->MemcpyStream(mappedPtr, data, bufferSize);

            CD3DX12_RANGE writtenRange(0, bufferSize);
            bufferResource->Unmap(0, &writtenRange);
        } else {
            SAFE_DELETE(buffer);
            return nullptr;
        }
    }

    if (uploadBuffer) {
        renderer.MarkForRelease(uploadBuffer);
    }

    D3D12IndexBuffer *indexBuffer = new D3D12IndexBuffer;
    indexBuffer->buffer = buffer;

    indexBuffer->ibv.BufferLocation = bufferResource->GetGPUVirtualAddress();
    indexBuffer->ibv.Format = (indexSize == sizeof(uint16_t) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
    indexBuffer->ibv.SizeInBytes = bufferSize;

    return indexBuffer;
}
