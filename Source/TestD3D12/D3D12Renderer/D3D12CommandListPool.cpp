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
#include "D3D12CommandListPool.h"
#include "D3D12CommandList.h"

void D3D12CommandListPool::Init(ID3D12Device *device, D3D12FrameThreadData *frameThreadData, D3D12_COMMAND_LIST_TYPE commandListType, uint32_t maxPrimaryCommandLists, uint32_t maxSecondaryCommandLists) {
    HRESULT hr;

    this->frameThreadData = frameThreadData;
    this->maxCommandLists = maxPrimaryCommandLists + maxSecondaryCommandLists;
    this->commandListPool = new D3D12CommandList[maxCommandLists];
    this->commandListType = commandListType;

    // Primary 커맨드 리스트 생성
    for (int i = 0; i < maxPrimaryCommandLists; ++i) {
        D3D12CommandList *commandList = &commandListPool[i];
        commandList->parentPool = this;

        hr = device->CreateCommandAllocator(commandListType, IID_PPV_ARGS(&commandList->commandAllocator));
        if (FAILED(hr)) {
            BE_FATALERROR("CreateCommandAllocator : failed, ERROR: 0x%x", hr);
        }

        if (commandListType == D3D12_COMMAND_LIST_TYPE_DIRECT) {
            // 그래픽스 CommandList 생성
            ID3D12GraphicsCommandList6 *graphicsCommandList = nullptr;
            hr = device->CreateCommandList(0, commandListType, commandList->commandAllocator, nullptr, IID_PPV_ARGS(&graphicsCommandList));
            graphicsCommandList->Close();
            commandList->commandList = graphicsCommandList;
        } else if (commandListType == D3D12_COMMAND_LIST_TYPE_COPY) {
            // 카피 CommandList 생성
            ID3D12GraphicsCommandList1 *copyCommandList = nullptr;
            hr = device->CreateCommandList(0, commandListType, commandList->commandAllocator, nullptr, IID_PPV_ARGS(&copyCommandList));
            copyCommandList->Close();
            commandList->commandList = copyCommandList;
        } else if (commandListType == D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE) {
            // 비디오 디코딩 CommandList 생성
            ID3D12VideoDecodeCommandList *videoDecodeCommandList = nullptr;
            hr = device->CreateCommandList(0, commandListType, commandList->commandAllocator, nullptr, IID_PPV_ARGS(&videoDecodeCommandList));
            videoDecodeCommandList->Close();
            commandList->commandList = videoDecodeCommandList;
        } else if (commandListType == D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE) {
            // 비디오 인코딩 CommandList 생성
            ID3D12VideoEncodeCommandList *videoEncodeCommandList = nullptr;
            hr = device->CreateCommandList(0, commandListType, commandList->commandAllocator, nullptr, IID_PPV_ARGS(&videoEncodeCommandList));
            videoEncodeCommandList->Close();
            commandList->commandList = videoEncodeCommandList;
        } else if (commandListType == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
            // 컴퓨트 CommandList 생성
            ID3D12CommandList *computeCommandList = nullptr;
            hr = device->CreateCommandList(0, commandListType, commandList->commandAllocator, nullptr, IID_PPV_ARGS(&computeCommandList));
            commandList->commandList = computeCommandList;
        } else {
            BE_FATALERROR("Invalid D3D12_COMMAND_LIST_TYPE: 0x%x", commandListType);
        }

        if (FAILED(hr)) {
            BE_FATALERROR("CreateCommandList : failed, ERROR: 0x%x", hr);
        }
    }

    // Secondary 커맨드 리스트 생성
    for (int i = maxPrimaryCommandLists; i < maxCommandLists; ++i) {
        D3D12CommandList *commandList = &commandListPool[i];
        commandList->parentPool = this;
        commandList->secondary = true;

        hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&commandList->commandAllocator));
        if (FAILED(hr)) {
            BE_FATALERROR("CreateCommandAllocator : failed, ERROR: 0x%x", hr);
        }

        // 번들 CommandList 생성
        ID3D12GraphicsCommandList6 *bundleCommandList = nullptr;
        hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, commandList->commandAllocator, nullptr, IID_PPV_ARGS(&bundleCommandList));
        if (FAILED(hr)) {
            BE_FATALERROR("CreateCommandList : failed, ERROR: 0x%x", hr);
        }

        bundleCommandList->Close();
        commandList->commandList = bundleCommandList;
    }

    // 모든 Primary 커맨드 리스트들을 free 상태로 초기화
    for (int i = 0; i < maxPrimaryCommandLists; ++i) {
        D3D12CommandList *commandList = &commandListPool[i];

        commandList->node.SetOwner(commandList);
        commandList->node.AddToEnd(freeCommandLists[to_int(RHI::CommandListType::Primary)]);
    }

    // 모든 Secondary 커맨드 리스트들을 free 상태로 초기화
    for (int i = maxPrimaryCommandLists; i < maxCommandLists; ++i) {
        D3D12CommandList *commandList = &commandListPool[i];

        commandList->node.SetOwner(commandList);
        commandList->node.AddToEnd(freeCommandLists[to_int(RHI::CommandListType::Secondary)]);
    }

    usedCount = 0;
}

void D3D12CommandListPool::Shutdown() {
    Clear();

    for (int i = 0; i < maxCommandLists; ++i) {
        D3D12CommandList *currentCommandList = &commandListPool[i];

        SAFE_RELEASE(currentCommandList->commandList);
        SAFE_RELEASE(currentCommandList->commandAllocator);
    }
    SAFE_DELETE_ARRAY(commandListPool);
}

void D3D12CommandListPool::Clear() {
    for (int i = 0; i < maxCommandLists; ++i) {
        if (!commandListPool[i].node.InList()) {
            Free(&commandListPool[i]);
        }
    }

    assert(usedCount == 0);
}

D3D12CommandList *D3D12CommandListPool::Alloc(RHI::CommandListType type) {
    BE1::LinkList<D3D12CommandList> &currentFreeCommandLists = freeCommandLists[to_int(type)];

    if (currentFreeCommandLists.IsListEmpty()) {
        BE_ERRLOG("D3D12CommandListPool::Alooc: not enough free command list\n");
        return nullptr;
    }

    D3D12CommandList* newCommandList = currentFreeCommandLists.Next();
    newCommandList->node.Remove();

    usedCount++;

    return newCommandList;
}

void D3D12CommandListPool::Free(D3D12CommandList* commandList) {
    if (!commandList || commandList->node.InList()) {
        BE_ERRLOG("D3D12CommandListPool::Free: invalid command list\n");
        return;
    }

    BE1::LinkList<D3D12CommandList> &currentFreeCommandLists = freeCommandLists[to_int(commandList->secondary ? RHI::CommandListType::Secondary : RHI::CommandListType::Primary)];
    commandList->node.AddToEnd(currentFreeCommandLists);

    usedCount--;
}
