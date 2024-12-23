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
#include "D3D12StorageBuffer.h"
#include "D3D12DescriptorPool.h"
#include "D3D12CommandList.h"

void D3D12StorageBuffer::Release() {
    if (descriptorHandle.ptr != 0) {
        renderer->uavDescriptorPool->Free(descriptorHandle);
        descriptorHandle.ptr = 0;
    }
    if (buffer) {
        renderer->DestroyBuffer(buffer, true);
        buffer = nullptr;
    }
}

ID3D12Resource *D3D12StorageBuffer::GetResource() const {
    return buffer->GetResource();
}

// Image::Format 을 쓰지 말고, 전용 Format enum 을 사용하자!!
RHIRenderer::StorageBuffer* D3D12Renderer::CreateStorageBuffer(BufferType type, Image::Format::Enum format, uint32_t structuredByteStride, uint32_t count, void *data) {
    D3D12Buffer *buffer = nullptr;

    uint32_t bytesPerPixel = Image::BytesPerPixel(format);
    uint32_t size = bytesPerPixel * count;

    if (type == RHIRenderer::BufferType::Static) {
        buffer = static_cast<D3D12Buffer *>(CreateBuffer(RHIRenderer::BufferUsage::Default, BufferFlag::ShaderResource | BufferFlag::UnorderedAccess, size));
    } else {
        buffer = static_cast<D3D12Buffer *>(CreateBuffer(RHIRenderer::BufferUsage::Upload, BufferFlag::ShaderResource | BufferFlag::UnorderedAccess, size));
    }

    if (!buffer) {
        return nullptr;
    }

    UINT bufferSize = buffer->GetSize();
    ID3D12Resource *bufferResource = buffer->GetResource();
    ID3D12Resource *uploadBuffer = nullptr;

    if (data) {
        if (type == RHIRenderer::BufferType::Static) {
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
            if (FAILED(device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &uploadBufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr, IID_PPV_ARGS(&uploadBuffer)))) {
                SAFE_DELETE(buffer);
                return nullptr;
            }

            UINT8 *mappedPtr = nullptr;
            uploadBuffer->Map(0, nullptr, reinterpret_cast<void **>(&mappedPtr));

            simdProcessor->MemcpyStream(mappedPtr, data, size);

            CD3DX12_RANGE writtenRange(0, size);
            uploadBuffer->Unmap(0, &writtenRange);

            // 업로드 버퍼에서 GPU 버퍼로 데이터 카피
            resourceCommandList->Reset();
            resourceCommandList->ResourceBarrier(bufferResource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
            resourceCommandList->GetGraphicsCommandList()->CopyBufferRegion(bufferResource, 0, uploadBuffer, 0, size);
            resourceCommandList->ResourceBarrier(bufferResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            resourceCommandList->CloseAndExecute(CommandQueueType::Graphics);
        } else if (type == RHIRenderer::BufferType::Dynamic) {
            UINT8 *mappedPtr = nullptr;
            bufferResource->Map(0, nullptr, reinterpret_cast<void **>(&mappedPtr));

            simdProcessor->MemcpyStream(mappedPtr, data, size);

            CD3DX12_RANGE writtenRange(0, size);
            bufferResource->Unmap(0, &writtenRange);
        } else {
            SAFE_DELETE(buffer);
            return nullptr;
        }
    }

    if (uploadBuffer) {
        MarkForRelease(uploadBuffer);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = { 0 };

    if (type == RHIRenderer::BufferType::Static) {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = count;
        uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

        if (format != Image::Format::Unknown) {
            DXGI_FORMAT dxgiFormat;
            ImageFormatToDXGIFormat(format, false, &dxgiFormat);
            uavDesc.Format = dxgiFormat;
            uavDesc.Buffer.StructureByteStride = 0;
        } else {
            uavDesc.Format = DXGI_FORMAT_UNKNOWN;
            uavDesc.Buffer.StructureByteStride = structuredByteStride;
        }

        descriptorHandle = uavDescriptorPool->Alloc();
        device->CreateUnorderedAccessView(bufferResource, nullptr, &uavDesc, descriptorHandle);
    }

    D3D12StorageBuffer* storageBuffer = new D3D12StorageBuffer;
    storageBuffer->bufferType = type;
    storageBuffer->buffer = buffer;
    storageBuffer->descriptorHandle = descriptorHandle;

    return storageBuffer;
}

void D3D12Renderer::DestroyStorageBuffer(StorageBuffer *storageBuffer, bool immediate) {
    if (immediate) {
        delete storageBuffer;
    } else {
        MarkForDelete(storageBuffer);
    }
}

void D3D12Renderer::SetStorageBuffer(CommandList *commandList, int slot, const StorageBuffer *storageBuffer) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    int threadIndex = d3d12CommandList->GetThreadIndex();

    int rootParameterIndex = d3d12CommandList->currentPSO->binder.rootParameterBinder.uav[slot];
    int descriptorIndex = d3d12CommandList->currentPSO->binder.descriptorTableBinder.uav[slot];

    const D3D12StorageBuffer *d3d12StorageBuffer = static_cast<const D3D12StorageBuffer *>(storageBuffer);
    D3D12FrameData::DataPerThread &threadData = currentFrameData->threadData[threadIndex];
    threadData.psoDescriptorHandles[rootParameterIndex][descriptorIndex] = d3d12StorageBuffer->descriptorHandle;
    threadData.cbvResources[rootParameterIndex] = d3d12StorageBuffer;

    if (d3d12CommandList->GetCommandListType() == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
        d3d12CommandList->computeRootParametersDirtyMask |= BIT64(rootParameterIndex);
    } else {
        d3d12CommandList->graphicsRootParametersDirtyMask |= BIT64(rootParameterIndex);
    }
}
