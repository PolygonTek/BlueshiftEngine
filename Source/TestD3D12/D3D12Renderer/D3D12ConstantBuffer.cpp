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
#include "D3D12ConstantBuffer.h"
#include "D3D12DescriptorPool.h"
#include "D3D12CommandList.h"
#include "D3D12FrameData.h"

void D3D12ConstantBuffer::Release() {
    if (!writePtr) {
        if (descriptorHandle.ptr != 0) {
            D3D12Renderer::GetRenderer()->resCpuDescriptorPool->Free(descriptorHandle);
            descriptorHandle.ptr = 0;
        }
    }
    if (buffer) {
        RHI::renderer->DestroyBuffer(buffer, true);
        buffer = nullptr;
    }
}

ID3D12Resource *D3D12ConstantBuffer::GetResource() const {
    return buffer->GetResource();
}

RHI::ConstantBuffer* D3D12Renderer::CreateConstantBuffer(RHI::BufferUsage usage, uint32_t size, const void *data) {
    D3D12Buffer *buffer = static_cast<D3D12Buffer *>(CreateBuffer(usage, RHI::ResourceFlag::ConstantBuffer, size, BE1::Image::Format::Unknown, 0, data));
    if (!buffer) {
        return nullptr;
    }

    // 상수 버퍼 뷰의 최대 크기는 64kb 이다.
    if (buffer->size > D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16) {
        BE_ERRLOG("D3D12Renderer::CreateConstantBuffer: The requested size of %i bytes exceeds the constant buffer maximum limit of 64Kb\n", buffer->size);
        DestroyBuffer(buffer);
        return nullptr;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = {};
    if (!resCpuDescriptorPool->Alloc(&descriptorHandle, nullptr)) {
        DestroyBuffer(buffer);
        return nullptr;
    }

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = buffer->GetResource()->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = size; // FIXME: alignedSize
    device->CreateConstantBufferView(&cbvDesc, descriptorHandle);

    D3D12ConstantBuffer* constantBuffer = new D3D12ConstantBuffer;
    constantBuffer->buffer = buffer;
    constantBuffer->descriptorHandle = descriptorHandle;

    return constantBuffer;
}

void D3D12Renderer::DestroyConstantBuffer(RHI::ConstantBuffer *constantBuffer, bool immediate) {
    if (immediate) {
        delete constantBuffer;
    } else {
        MarkForDelete(constantBuffer);
    }
}

void D3D12Renderer::SetConstantBuffer(RHI::CommandList *commandList, int slot, const RHI::ConstantBuffer *constantBuffer) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    D3D12FrameThreadData *threadData = static_cast<D3D12FrameThreadData *>(d3d12CommandList->GetFrameThreadData());

    // 슬롯 (레지스터) 에 대한 루트 파라미터 인덱스를 얻고, 디스크립터 테이블일 경우 테이블 인덱스도 얻어온다.
    const D3D12PipelineState::Binder &binder = d3d12CommandList->currentPSO->binder;
    uint8_t rootParameterIndex = binder.rootParameterBinder.cbv[slot];
    uint8_t descriptorIndex = binder.descriptorTableBinder.cbv[slot];

    const D3D12ConstantBuffer *d3d12ConstantBuffer = static_cast<const D3D12ConstantBuffer *>(constantBuffer);
    if (descriptorIndex != 0xFF) {
        threadData->tableCpuDescriptorHandles[rootParameterIndex][descriptorIndex] = d3d12ConstantBuffer->descriptorHandle;
    }
    threadData->cbvResources[slot] = d3d12ConstantBuffer;

    if (rootParameterIndex == 0xFF) {
        BE_ERRLOG("D3D12Renderer::SetConstantBuffer: Invalid root parameter index\n");
        return;
    }

    if (d3d12CommandList->currentPSO->graphics) {
        d3d12CommandList->graphicsRootParametersDirtyMask |= BIT64(rootParameterIndex);
    } else {
        d3d12CommandList->computeRootParametersDirtyMask |= BIT64(rootParameterIndex);
    }
}
