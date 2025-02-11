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
#include "D3D12CommandList.h"

void D3D12IndexBuffer::Release() {
    if (dynamicBlockIndex == -1) {
        if (buffer) {
            RHI::renderer->DestroyBuffer(buffer, true);
            buffer = nullptr;
        }
    }
}

RHI::IndexBuffer *D3D12Renderer::CreateIndexBuffer(RHI::BufferUsage usage, uint32_t indexSize, uint32_t numIndexes, const void *data) {
    assert(indexSize == 2 || indexSize == 4);

    UINT size = indexSize * numIndexes;
    D3D12Buffer *buffer = static_cast<D3D12Buffer *>(CreateBuffer(usage, RHI::ResourceFlag::IndexBuffer, size, BE1::Image::Format::Unknown, 0, data));
    if (!buffer) {
        return nullptr;
    }

    D3D12IndexBuffer *indexBuffer = new D3D12IndexBuffer;
    indexBuffer->buffer = buffer;
    indexBuffer->ibv.BufferLocation = buffer->GetResource()->GetGPUVirtualAddress();
    indexBuffer->ibv.Format = (indexSize == sizeof(uint16_t) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
    indexBuffer->ibv.SizeInBytes = size;

    return indexBuffer;
}

void D3D12Renderer::DestroyIndexBuffer(RHI::IndexBuffer *indexBuffer, bool immediate) {
    if (immediate) {
        delete indexBuffer;
    } else {
        MarkForDelete(indexBuffer);
    }
}

void D3D12Renderer::SetIndexBuffer(RHI::CommandList *commandList, const RHI::IndexBuffer *indexBuffer) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    d3d12CommandList->SetIndexBuffer(indexBuffer);
}
