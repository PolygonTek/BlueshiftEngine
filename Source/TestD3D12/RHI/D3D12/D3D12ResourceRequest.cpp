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
#include "D3D12CommandList.h"
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

void D3D12ResourceCommandContext::CloseAndExecute() {
    if (graphicsCommandList) {
        graphicsCommandList->CloseAndExecute();
    }
    if (computeCommandList) {
        computeCommandList->CloseAndExecute();
    }
}

void D3D12Renderer::EnqueueResourceRequest(D3D12ResourceRequest &&request) {
    BE1::ScopedLock lock(resourceRequestMutex);
    resourceRequests.Append(std::move(request));
}

bool D3D12Renderer::DequeueResourceRequests(BE1::Array<D3D12ResourceRequest> &outRequests) {
    outRequests.Clear();
    {
        BE1::ScopedLock lock(resourceRequestMutex);
        outRequests.Swap(resourceRequests);
    }
    return !outRequests.IsEmpty();
}

void D3D12Renderer::DeleteGPUObject(RHI::GPUObject *object) {
    D3D12ResourceRequest request;
    request.type = D3D12ResourceRequest::Type::DeleteGPUObject;
    request.object = object;

    EnqueueResourceRequest(std::move(request));
}

void D3D12Renderer::FlushResourceRequests(RHI::FrameThreadData *frameThreadData) {
    BE1::Array<D3D12ResourceRequest> localRequests;
    if (!DequeueResourceRequests(localRequests)) {
        return;
    }

    D3D12ResourceCommandContext commandContext;
    commandContext.frameThreadData = (D3D12FrameThreadData *)frameThreadData;
    ExecuteResourceRequests(localRequests.Ptr(), localRequests.Count(), frameThreadData ? &commandContext : nullptr);

    commandContext.CloseAndExecute();
}

void D3D12Renderer::FinishResourceRequests(RHI::FrameThreadData *frameThreadData) {
    assert(BE1::Engine::IsInMainThread());
    assert(frameThreadData);

    D3D12ResourceCommandContext commandContext;
    commandContext.frameThreadData = (D3D12FrameThreadData *)frameThreadData;

    // 같은 command context 로 밉맵 생성 중 등록된 임시 텍스처 삭제 요청 등 파생 요청까지 처리한다.
    BE1::Array<D3D12ResourceRequest> localRequests;
    while (DequeueResourceRequests(localRequests)) {
        ExecuteResourceRequests(localRequests.Ptr(), localRequests.Count(), &commandContext);
    }

    commandContext.CloseAndExecute();

    // 요청 큐가 비어 있어도 이전 Flush 가 제출한 GPU 작업은 완료까지 기다린다.
    Finish(RHI::CommandQueueType::Graphics);
    Finish(RHI::CommandQueueType::Compute);
}

// commandContext 이 nullptr 이면 해제 요청만 처리하고, 업로드는 생략한다.
void D3D12Renderer::ExecuteResourceRequests(D3D12ResourceRequest *requests, int numRequests, D3D12ResourceCommandContext *commandContext) {
    for (int requestIndex = 0; requestIndex < numRequests; ++requestIndex) {
        D3D12ResourceRequest &request = requests[requestIndex];
        switch (request.type) {
        case D3D12ResourceRequest::Type::DeleteGPUObject:
        {
            BE1::ScopedLock lock(releaseRequestMutex);
            releaseRequests.Append(D3D12ReleaseRequest{ nullptr, request.object });
        }
        break;
        case D3D12ResourceRequest::Type::UploadBuffer:
            if (commandContext) {
                if (ExecuteUploadBuffer(*commandContext, static_cast<D3D12Buffer *>(request.object), request.uploadBufferRequest)) {
                    static_cast<RHI::GPUResource *>(request.object)->SetStatus(RHI::GPUResourceStatus::Ready);
                } else {
                    static_cast<RHI::GPUResource *>(request.object)->SetStatus(RHI::GPUResourceStatus::Failed);
                }
            } else if (request.uploadBufferRequest.uploadBuffer) {
                RequestRelease(request.uploadBufferRequest.uploadBuffer);
            }
            break;
        case D3D12ResourceRequest::Type::UploadTexture:
            if (commandContext) {
                if (ExecuteUploadTexture(*commandContext, static_cast<D3D12Texture *>(request.object), request.uploadTextureRequest)) {
                    static_cast<RHI::GPUResource *>(request.object)->SetStatus(RHI::GPUResourceStatus::Ready);
                } else {
                    static_cast<RHI::GPUResource *>(request.object)->SetStatus(RHI::GPUResourceStatus::Failed);
                }
            }
            break;
        case D3D12ResourceRequest::Type::UploadBufferSubData:
            if (commandContext) {
                ExecuteUploadBufferSubData(*commandContext, static_cast<D3D12Buffer *>(request.object), request.uploadBufferSubDataRequest);
            } else {
                RequestRelease(request.uploadBufferSubDataRequest.uploadBuffer);
            }
            break;
        case D3D12ResourceRequest::Type::UploadTextureSubImage:
            if (commandContext) {
                ExecuteUploadTextureSubImage(*commandContext, static_cast<D3D12Texture *>(request.object), request.uploadTextureSubImageRequest);
            } else {
                RequestRelease(request.uploadTextureSubImageRequest.uploadBuffer);
            }
            break;
        case D3D12ResourceRequest::Type::BuildMipChain:
            if (commandContext) {
                if (ExecuteBuildMipChain(*commandContext, static_cast<D3D12Texture *>(request.object), std::move(request.buildMipChainRequest))) {
                    static_cast<RHI::GPUResource *>(request.object)->SetStatus(RHI::GPUResourceStatus::Ready);
                } else {
                    static_cast<RHI::GPUResource *>(request.object)->SetStatus(RHI::GPUResourceStatus::Failed);
                }
            }
            break;
        }
    }
}
