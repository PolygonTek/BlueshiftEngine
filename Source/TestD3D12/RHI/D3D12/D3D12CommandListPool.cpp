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

void D3D12CommandListPool::Init(D3D12RootBindingData *rootBindingData, D3D12_COMMAND_LIST_TYPE commandListType, uint32_t initialPrimaryCommandLists, uint32_t initialSecondaryCommandLists) {
    assert(commandLists.Count() == 0);
    assert(initialPrimaryCommandLists > 0 || initialSecondaryCommandLists > 0);
    assert(initialSecondaryCommandLists == 0 || commandListType == D3D12_COMMAND_LIST_TYPE_DIRECT);

    this->rootBindingData = rootBindingData;
    this->commandListType = commandListType;
    supportedKinds[to_int(RHI::CommandListKind::Primary)] = initialPrimaryCommandLists > 0;
    supportedKinds[to_int(RHI::CommandListKind::Secondary)] = initialSecondaryCommandLists > 0;

    const uint32_t initialCommandListCount = initialPrimaryCommandLists + initialSecondaryCommandLists;
    commandListAllocator.ReserveBlocks((initialCommandListCount + CommandListBlockSize - 1) / CommandListBlockSize);
    commandLists.Reserve(initialCommandListCount);

    if (initialPrimaryCommandLists > 0) {
        AddCommandLists(RHI::CommandListKind::Primary, initialPrimaryCommandLists);
    }
    if (initialSecondaryCommandLists > 0) {
        AddCommandLists(RHI::CommandListKind::Secondary, initialSecondaryCommandLists);
    }

    usedCount = 0;
}

void D3D12CommandListPool::AddCommandLists(RHI::CommandListKind kind, uint32_t count) {
    const int kindIndex = to_int(kind);
    assert(kindIndex >= 0 && kindIndex < to_int(RHI::CommandListKind::Count));
    assert(supportedKinds[kindIndex]);
    assert(kind != RHI::CommandListKind::Secondary || commandListType == D3D12_COMMAND_LIST_TYPE_DIRECT);

    ID3D12Device5 *device = D3D12Renderer::GetRenderer()->GetDevice();

    for (uint32_t commandListIndex = 0; commandListIndex < count; ++commandListIndex) {
        D3D12CommandList *commandList = commandListAllocator.Alloc();
        commandList->parentPool = this;
        commandList->isSecondary = kind == RHI::CommandListKind::Secondary;

        HRESULT hr;
        if (kind == RHI::CommandListKind::Primary) {
            // Primary command allocator 를 생성한다.
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
        } else {
            // Secondary (Bundle) command allocator 를 생성한다.
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

        commandList->node.SetOwner(commandList);
        commandList->node.AddToEnd(freeCommandLists[kindIndex]);
        commandLists.Append(commandList);
        allocatedCounts[kindIndex]++;
    }
}

void D3D12CommandListPool::Shutdown() {
    Clear();

    for (BE1::LinkList<D3D12CommandList> &freeCommandList : freeCommandLists) {
        freeCommandList.Clear();
    }

    for (D3D12CommandList *commandList : commandLists) {
        SAFE_RELEASE(commandList->commandList);
        SAFE_RELEASE(commandList->commandAllocator);
        commandListAllocator.Free(commandList);
    }

    commandLists.Clear();
    commandListAllocator.FreeAllBlocks();

    rootBindingData = nullptr;
    commandListType = D3D12_COMMAND_LIST_TYPE_NONE;
    for (int kindIndex = 0; kindIndex < to_int(RHI::CommandListKind::Count); ++kindIndex) {
        supportedKinds[kindIndex] = false;
        allocatedCounts[kindIndex] = 0;
    }
    usedCount = 0;
}

void D3D12CommandListPool::Clear() {
    for (D3D12CommandList *commandList : commandLists) {
        if (!commandList->node.InList()) {
            Free(commandList);
        }
    }

    assert(usedCount == 0);
}

D3D12CommandList *D3D12CommandListPool::Alloc(RHI::CommandListKind kind) {
    const int kindIndex = to_int(kind);
    if (kindIndex < 0 || kindIndex >= to_int(RHI::CommandListKind::Count) || !supportedKinds[kindIndex]) {
        BE_FATALERROR("D3D12CommandListPool::Alloc: unsupported command list kind %i\n", kindIndex);
        return nullptr;
    }

    BE1::LinkList<D3D12CommandList> &currentFreeCommandLists = freeCommandLists[kindIndex];

    if (currentFreeCommandLists.IsListEmpty()) {
        const uint32_t previousCount = allocatedCounts[kindIndex];
        AddCommandLists(kind, CommandListBlockSize);
        BE_WARNLOG("D3D12CommandListPool::Alloc: grew %s command list pool from %u to %u\n", kind == RHI::CommandListKind::Primary ? "primary" : "secondary", previousCount, allocatedCounts[kindIndex]);
    }

    D3D12CommandList *newCommandList = currentFreeCommandLists.Next();
    newCommandList->node.Remove();

    usedCount++;

    return newCommandList;
}

void D3D12CommandListPool::Free(D3D12CommandList* commandList) {
    if (!commandList) {
        BE_ERRLOG("D3D12CommandListPool::Free: invalid command list\n");
        return;
    }

    if (commandList->parentPool != this) {
        BE_ERRLOG("D3D12CommandListPool::Free: command list belongs to another pool\n");
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
