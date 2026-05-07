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
#include "D3D12ResourceRequest.h"
#include "D3D12FrameThreadData.h"
#include "D3D12Texture.h"

D3D12CommandList *D3D12ResourceCommandContext::GetGraphicsCommandList() {
    if (!graphicsCommandList) {
        graphicsCommandList = static_cast<D3D12CommandList *>(frameThreadData->BeginCommandList(RHI::CommandQueueType::Graphics));
    }
    return graphicsCommandList;
}

D3D12CommandList *D3D12ResourceCommandContext::GetComputeCommandList() {
    if (!computeCommandList) {
        computeCommandList = static_cast<D3D12CommandList *>(frameThreadData->BeginCommandList(RHI::CommandQueueType::Compute));
    }
    return computeCommandList;
}

void D3D12Renderer::EnqueueResourceRequest(D3D12ResourceRequest &&request) {
    BE1::ScopedLock lock(resourceRequestMutex);
    resourceRequests.Append(std::move(request));
}

void D3D12Renderer::DeleteGPUObject(RHI::GPUObject *object) {
    D3D12ResourceRequest request;
    request.type = D3D12ResourceRequest::Type::DeleteGPUObject;
    request.object = object;

    EnqueueResourceRequest(std::move(request));
}

void D3D12Renderer::ProcessResourceRequests(RHI::FrameThreadData *frameThreadData) {
    BE1::Array<D3D12ResourceRequest> localRequests;
    {
        BE1::ScopedLock lock(resourceRequestMutex);
        localRequests.Swap(resourceRequests);
    }

    if (localRequests.IsEmpty()) {
        return;
    }

    D3D12ResourceCommandContext commandContext;
    commandContext.frameThreadData = (D3D12FrameThreadData *)frameThreadData;

    for (D3D12ResourceRequest &request : localRequests) {
        switch (request.type) {
        case D3D12ResourceRequest::Type::DeleteGPUObject:
        {
            BE1::ScopedLock lock(releaseRequestMutex);
            releaseRequests.Append(D3D12ReleaseRequest{ nullptr, request.object });
        }
        break;
        case D3D12ResourceRequest::Type::UploadBuffer:
            if (frameThreadData) {
                if (ExecuteUploadBuffer(commandContext, static_cast<D3D12Buffer *>(request.object), request.uploadBufferRequest)) {
                    static_cast<RHI::GPUResource *>(request.object)->SetStatus(RHI::GPUResourceStatus::Ready);
                } else {
                    static_cast<RHI::GPUResource *>(request.object)->SetStatus(RHI::GPUResourceStatus::Failed);
                }
            }
            break;
        case D3D12ResourceRequest::Type::UploadTexture:
            if (frameThreadData) {
                if (ExecuteUploadTexture(commandContext, static_cast<D3D12Texture *>(request.object), request.uploadTextureRequest)) {
                    static_cast<RHI::GPUResource *>(request.object)->SetStatus(RHI::GPUResourceStatus::Ready);
                } else {
                    static_cast<RHI::GPUResource *>(request.object)->SetStatus(RHI::GPUResourceStatus::Failed);
                }
            }
            break;
        case D3D12ResourceRequest::Type::UploadTextureSubImage:
            if (frameThreadData) {
                ExecuteUploadTextureSubImage(commandContext, static_cast<D3D12Texture *>(request.object), request.uploadTextureSubImageRequest);
            } else {
                RequestRelease(request.uploadTextureSubImageRequest.uploadBuffer);
            }
            break;
        case D3D12ResourceRequest::Type::GenerateMipmaps:
            if (frameThreadData) {
                ExecuteGenerateMipmaps(commandContext, static_cast<D3D12Texture *>(request.object));
            }
            break;
        }
    }

    if (commandContext.graphicsCommandList) {
        commandContext.graphicsCommandList->CloseAndExecute();
    }
    if (commandContext.computeCommandList) {
        commandContext.computeCommandList->CloseAndExecute();
    }
}
