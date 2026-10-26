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
#include "D3D12RootBindingData.h"
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
    ExecuteResourceRequests(localRequests, frameThreadData ? &commandContext : nullptr);

    if (commandContext.graphicsCommandList) {
        commandContext.graphicsCommandList->CloseAndExecute();
    }
    if (commandContext.computeCommandList) {
        commandContext.computeCommandList->CloseAndExecute();
    }
}

void D3D12Renderer::FlushResourceRequests() {
    BE1::Array<D3D12ResourceRequest> localRequests;
    {
        BE1::ScopedLock lock(resourceRequestMutex);
        localRequests.Swap(resourceRequests);
    }

    if (localRequests.IsEmpty()) {
        return;
    }

    // 초기화 중에는 프레임이 사용하지 않는 기존 shader-visible 힙의 transient 영역을 빌린다.
    D3D12RootBindingData rootBindingData = {};
    D3D12DescriptorRangePool resourceDescriptorRangePool({ &resourceShaderVisibleDescriptorHeap, MaxPersistentResourceDescriptors, MaxTransientResourceDescriptors });
    D3D12DescriptorRangePool samplerDescriptorRangePool({ &samplerShaderVisibleDescriptorHeap, MaxPersistentSamplerDescriptors, MaxTransientSamplerDescriptors });
    D3D12CommandList *commandList = static_cast<D3D12CommandList *>(BeginUtilityCommandList(RHI::CommandQueueType::Graphics, &rootBindingData));

    commandList->SetDescriptorTablePools(&resourceDescriptorRangePool, &samplerDescriptorRangePool);

    ID3D12DescriptorHeap *descriptorHeaps[] = {
        resourceShaderVisibleDescriptorHeap.GetDescriptorHeap(),
        samplerShaderVisibleDescriptorHeap.GetDescriptorHeap()
    };
    commandList->SetDescriptorHeaps(COUNT_OF(descriptorHeaps), descriptorHeaps);

    // 복사와 GPU 밉맵 생성을 하나의 Direct command list 에 순서대로 기록한다.
    D3D12ResourceCommandContext commandContext;
    commandContext.graphicsCommandList = commandList;
    commandContext.computeCommandList = commandList;

    do {
        ExecuteResourceRequests(localRequests, &commandContext);
        localRequests.Clear();

        // 밉맵 생성 중 등록된 임시 텍스처 해제 요청도 같은 flush 에서 처리한다.
        BE1::ScopedLock lock(resourceRequestMutex);
        localRequests.Swap(resourceRequests);
    } while (!localRequests.IsEmpty());

    commandList->CloseAndExecute();
    DrainReleaseRequestsToRetireList();

    // transient 디스크립터와 command allocator 를 GPU 완료 전에 재사용하지 않도록 기다린다.
    Finish(RHI::CommandQueueType::Graphics);

    EndUtilityCommandList(commandList);
    FreeRetiredResources(true);
}

void D3D12Renderer::ExecuteResourceRequests(BE1::Array<D3D12ResourceRequest> &requests, D3D12ResourceCommandContext *commandContext) {
    // 종료 경로의 nullptr context 는 업로드를 생략하고 해제 요청만 처리한다.
    for (D3D12ResourceRequest &request : requests) {
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
