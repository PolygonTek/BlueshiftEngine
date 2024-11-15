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

void D3D12CommandListPool::Init(D3D12_COMMAND_LIST_TYPE commandListType, int maxCommandLists) {
    this->maxCommandLists = maxCommandLists;
    this->commandLists = new D3D12CommandList[maxCommandLists];

    for (int i = 0; i < maxCommandLists; ++i) {
        D3D12CommandList* currentCommandList = &commandLists[i];

        // 그래픽스 커맨드 리스트를 위한 커맨드 할당자 생성
        if (FAILED(renderer.device->CreateCommandAllocator(commandListType, IID_PPV_ARGS(&currentCommandList->commandAllocator)))) {
            BE_FATALERROR("CreateCommandAllocator : failed");
        }

        // 그래픽스 커맨드 리스트 생성
        if (FAILED(renderer.device->CreateCommandList(0, commandListType, currentCommandList->commandAllocator, nullptr, IID_PPV_ARGS(&currentCommandList->commandList)))) {
            BE_FATALERROR("CreateCommandList : failed");
        }

        // Command lists are created in the recording state, but there is nothing
        // to record yet. The main loop expects it to be closed, so close it now.
        currentCommandList->commandList->Close();
    }

    // 모든 commandLists 를 free 상태로 초기화
    for (int i = 0; i < maxCommandLists; ++i) {
        D3D12CommandList *currentCommandList = &commandLists[i];

        currentCommandList->node.SetOwner(currentCommandList);
        currentCommandList->node.AddToEnd(freeCommandLists);
    }

    usedCount = 0;
}

void D3D12CommandListPool::Shutdown() {
    Clear();

    for (int i = 0; i < maxCommandLists; ++i) {
        D3D12CommandList *currentCommandList = &commandLists[i];

        SAFE_RELEASE(currentCommandList->commandList);
        SAFE_RELEASE(currentCommandList->commandAllocator);
    }
    SAFE_DELETE_ARRAY(commandLists);
}

void D3D12CommandListPool::Clear() {
    for (int i = 0; i < maxCommandLists; ++i) {
        if (!commandLists[i].node.InList()) {
            Free(&commandLists[i]);
        }
    }

    assert(usedCount == 0);
}

D3D12CommandList *D3D12CommandListPool::Alloc() {
    if (freeCommandLists.IsListEmpty()) {
        BE_ERRLOG("D3D12CommandListPool::Alooc: not enough free command list\n");
        return nullptr;
    }

    D3D12CommandList* newCommandList = freeCommandLists.Next();
    newCommandList->node.Remove();

    usedCount++;

    return newCommandList;
}

void D3D12CommandListPool::Free(D3D12CommandList* commandList) {
    if (!commandList || commandList->node.InList()) {
        BE_ERRLOG("D3D12CommandListPool::Free: invalid command list\n");
        return;
    }

    commandList->node.AddToEnd(freeCommandLists);

    usedCount--;
}
