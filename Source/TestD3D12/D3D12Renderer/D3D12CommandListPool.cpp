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
#include "D3D12CommandListPool.h"
#include "D3D12CommandList.h"

void D3D12CommandListPool::Init(D3D12RootBindingData *rootBindingData, D3D12_COMMAND_LIST_TYPE commandListType, uint32_t maxPrimaryCommandLists, uint32_t maxSecondaryCommandLists) {
    HRESULT hr;

    this->rootBindingData = rootBindingData;
    this->commandListType = commandListType;
    this->maxCommandLists = maxPrimaryCommandLists + maxSecondaryCommandLists;
    this->commandListPool = new D3D12CommandList[maxCommandLists];

    ID3D12Device5 *device = D3D12Renderer::GetRenderer()->device;

    // primary command list 들을 생성
    for (int i = 0; i < maxPrimaryCommandLists; ++i) {
        D3D12CommandList *commandList = &commandListPool[i];
        commandList->parentPool = this;

        hr = device->CreateCommandAllocator(commandListType, IID_PPV_ARGS(&commandList->commandAllocator));
        if (FAILED(hr)) {
            BE_FATALERROR("CreateCommandAllocator: failed, ERROR: 0x%x", hr);
        }

        if (commandListType == D3D12_COMMAND_LIST_TYPE_DIRECT) {
            // Direct (Graphics) command list 생성
            ID3D12GraphicsCommandList6 *graphicsCommandList = nullptr;
            hr = device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&graphicsCommandList));
            commandList->commandList = graphicsCommandList;
        } else if (commandListType == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
            // Compute command list 생성
            ID3D12GraphicsCommandList6 *graphicsCommandList = nullptr;
            hr = device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&graphicsCommandList));
            commandList->commandList = graphicsCommandList;
        } else if (commandListType == D3D12_COMMAND_LIST_TYPE_COPY) {
            // Copy command list 생성
            ID3D12GraphicsCommandList1 *copyCommandList = nullptr;
            hr = device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_COPY, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&copyCommandList));
            commandList->commandList = copyCommandList;
        } else if (commandListType == D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE) {
            // Video decode command list 생성
            ID3D12VideoDecodeCommandList *videoDecodeCommandList = nullptr;
            hr = device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&videoDecodeCommandList));
            commandList->commandList = videoDecodeCommandList;
        } else if (commandListType == D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE) {
            // Video encode command list 생성
            ID3D12VideoEncodeCommandList *videoEncodeCommandList = nullptr;
            hr = device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&videoEncodeCommandList));
            commandList->commandList = videoEncodeCommandList;
        } else {
            BE_FATALERROR("Invalid D3D12_COMMAND_LIST_TYPE: %i", commandListType);
        }

        if (FAILED(hr)) {
            BE_FATALERROR("CreateCommandList: failed, ERROR: 0x%x", hr);
        }
    }

    // secondary command list 들을 생성
    for (int i = maxPrimaryCommandLists; i < maxCommandLists; ++i) {
        D3D12CommandList *commandList = &commandListPool[i];
        commandList->parentPool = this;
        commandList->isSecondary = true;

        hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&commandList->commandAllocator));
        if (FAILED(hr)) {
            BE_FATALERROR("CreateCommandAllocator: failed, ERROR: 0x%x", hr);
        }

        // Bundle command list 생성
        ID3D12GraphicsCommandList6 *bundleCommandList = nullptr;
        hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, commandList->commandAllocator, nullptr, IID_PPV_ARGS(&bundleCommandList));
        if (FAILED(hr)) {
            BE_FATALERROR("CreateCommandList: failed, ERROR: 0x%x", hr);
        }
        commandList->commandList = bundleCommandList;

        // CreateCommandList1 으로 command list 를 생성하면 closed 상태로 생성되지만,
        // CreateCommandList 로 command list 를 생성하면 recording 상태로 생성되므로 바로 close 해준다.
        bundleCommandList->Close();
    }

    // 모든 primary command list 들을 free 상태로 초기화
    for (int i = 0; i < maxPrimaryCommandLists; ++i) {
        D3D12CommandList *commandList = &commandListPool[i];

        commandList->node.SetOwner(commandList);
        commandList->node.AddToEnd(freeCommandLists[to_int(RHI::CommandListKind::Primary)]);
    }

    // 모든 secondary command list 들을 free 상태로 초기화
    for (int i = maxPrimaryCommandLists; i < maxCommandLists; ++i) {
        D3D12CommandList *commandList = &commandListPool[i];

        commandList->node.SetOwner(commandList);
        commandList->node.AddToEnd(freeCommandLists[to_int(RHI::CommandListKind::Secondary)]);
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

D3D12CommandList *D3D12CommandListPool::Alloc(RHI::CommandListKind kind) {
    BE1::LinkList<D3D12CommandList> &currentFreeCommandLists = freeCommandLists[to_int(kind)];

    if (currentFreeCommandLists.IsListEmpty()) {
        BE_ERRLOG("D3D12CommandListPool::Alloc: not enough free command list\n");
        return nullptr;
    }

    D3D12CommandList* newCommandList = currentFreeCommandLists.Next();
    newCommandList->node.Remove();

    usedCount++;

    return newCommandList;
}

void D3D12CommandListPool::Free(D3D12CommandList* commandList) {
    if (!commandList) {
        BE_ERRLOG("D3D12CommandListPool::Free: invalid command list\n");
        return;
    }

    if (commandList->node.InList()) {
        BE_ERRLOG("D3D12CommandListPool::Free: command list is already in the free list\n");
        return;
    }

    BE1::LinkList<D3D12CommandList> &currentFreeCommandLists = freeCommandLists[to_int(commandList->isSecondary ? RHI::CommandListKind::Secondary : RHI::CommandListKind::Primary)];
    commandList->node.AddToEnd(currentFreeCommandLists);

    usedCount--;
}
