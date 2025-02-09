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
#include "D3D12VertexBuffer.h"
#include "D3D12CommandList.h"

void D3D12VertexBuffer::Release() {
    if (buffer) {
        RHI::renderer->DestroyBuffer(buffer, true);
        buffer = nullptr;
    }
}

RHI::VertexBuffer* D3D12Renderer::CreateVertexBuffer(RHI::BufferUsage usage, uint32_t vertexSize, uint32_t numVerts, const void *data) {
    UINT size = vertexSize * numVerts;
    D3D12Buffer *buffer = static_cast<D3D12Buffer *>(CreateBuffer(usage, RHI::ResourceFlag::VertexBuffer, size, BE1::Image::Format::Unknown, 0, data));
    if (!buffer) {
        return nullptr;
    }

    D3D12VertexBuffer* vertexBuffer = new D3D12VertexBuffer;
    vertexBuffer->buffer = buffer;
    vertexBuffer->vbv.BufferLocation = buffer->GetResource()->GetGPUVirtualAddress();
    vertexBuffer->vbv.StrideInBytes = vertexSize;
    vertexBuffer->vbv.SizeInBytes = size;

    return vertexBuffer;
}

void D3D12Renderer::DestroyVertexBuffer(RHI::VertexBuffer *vertexBuffer, bool immediate) {
    if (immediate) {
        delete vertexBuffer;
    } else {
        MarkForDelete(vertexBuffer);
    }
}

void D3D12Renderer::SetVertexBuffer(RHI::CommandList *commandList, int slot, const RHI::VertexBuffer *vertexBuffer) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    d3d12CommandList->SetVertexBuffer(slot, vertexBuffer);
}
