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

#pragma once

#include "D3D12Common.h"

class D3D12CommandList;

class D3D12CommandListPool {
public:
    D3D12CommandListPool() = default;
    D3D12CommandListPool(ID3D12Device *device, int threadIndex, D3D12_COMMAND_LIST_TYPE commandListType, int maxCommandLists) { Init(device, threadIndex, commandListType, maxCommandLists); }
    ~D3D12CommandListPool() { Shutdown(); }

    void                            Init(ID3D12Device *device, int threadIndex, D3D12_COMMAND_LIST_TYPE commandListType, int maxCommandLists);
    void                            Shutdown();

    void                            Clear();
    D3D12CommandList *              Alloc();
    void                            Free(D3D12CommandList *commandList);

    int                             GetThreadIndex() const { return threadIndex; }

    int                             UsedCount() const { return usedCount; }

private:
    D3D12CommandList *              commandListPool = nullptr;
    int                             maxCommandLists = 0;
    LinkList<D3D12CommandList>      freeCommandLists;
    int                             threadIndex = -1;
    int                             usedCount = 0;
};
