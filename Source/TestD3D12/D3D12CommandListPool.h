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

#define ENABLE_STATE_CACHE_FOR_COMMAND_LIST

class D3D12CommandListPool;

class D3D12CommandList {
public:
    void                            Reset();

    void                            SetDescriptorHeaps(int numDescriptorHeaps, ID3D12DescriptorHeap* descriptorHeaps[]);
    void                            SetGraphicsRootSignature(ID3D12RootSignature* graphicsRootSignature);
    void                            SetPipelineState(ID3D12PipelineState* piplelineState);
    void                            SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology);

    ID3D12CommandAllocator *        commandAllocator = nullptr;
    ID3D12GraphicsCommandList *     commandList = nullptr;
    D3D12CommandListPool *          parentPool = nullptr;
    LinkList<D3D12CommandList>      node;

#ifdef ENABLE_STATE_CACHE_FOR_COMMAND_LIST
    bool                            IsSameDescriptorHeaps(int numDescriptorHeaps, ID3D12DescriptorHeap *descriptorHeaps[]);

    StaticArray<ID3D12DescriptorHeap *, 16> currentRootDescriptorHeaps;
    ID3D12RootSignature *           currentGraphicsRootSignature = nullptr;
    ID3D12PipelineState *           currentPipelineState = nullptr;
    D3D12_PRIMITIVE_TOPOLOGY        currentPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
#endif
};

BE_INLINE void D3D12CommandList::Reset() {
    // NOTE: CommandList 는 GPU 작업 완료 여부와 상관없이 Reset 하여 재사용할 수 있지만,
    // CommandAllocator 는 GPU 에서 해당 CommandList 를 사용하는 작업이 모두 완료되기 전에는 Reset 하면 안된다.
    // 
    // CommandAllocator 를 재사용하도록 리셋
    commandAllocator->Reset();

    // CommandList 를 CommandAllocator 를 이용하여 초기 상태로 리셋
    commandList->Reset(commandAllocator, nullptr);

#ifdef ENABLE_STATE_CACHE_FOR_COMMAND_LIST
    // 각종 상태를 초기값으로 변경
    currentRootDescriptorHeaps.SetCount(0);
    currentGraphicsRootSignature = nullptr;
    currentPipelineState = nullptr;
    currentPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
#endif
}

#ifdef ENABLE_STATE_CACHE_FOR_COMMAND_LIST
BE_INLINE bool D3D12CommandList::IsSameDescriptorHeaps(int numDescriptorHeaps, ID3D12DescriptorHeap *descriptorHeaps[]) {
    if (numDescriptorHeaps != currentRootDescriptorHeaps.Count()) {
        return false;
    }
    for (int i = 0; i < numDescriptorHeaps; ++i) {
        if (descriptorHeaps[i] != currentRootDescriptorHeaps[i]) {
            return false;
        }
    }
    return true;
}
#endif

BE_INLINE void D3D12CommandList::SetDescriptorHeaps(int numDescriptorHeaps, ID3D12DescriptorHeap *descriptorHeaps[]) {
#ifdef ENABLE_STATE_CACHE_FOR_COMMAND_LIST
    if (IsSameDescriptorHeaps(numDescriptorHeaps, descriptorHeaps)) {
        return;
    }
    currentRootDescriptorHeaps.SetCount(numDescriptorHeaps);
    for (int i = 0; i < numDescriptorHeaps; ++i) {
        currentRootDescriptorHeaps[i] = descriptorHeaps[i];
    }
#endif
    commandList->SetDescriptorHeaps(numDescriptorHeaps, descriptorHeaps);
}

BE_INLINE void D3D12CommandList::SetGraphicsRootSignature(ID3D12RootSignature *graphicsRootSignature) {
#ifdef ENABLE_STATE_CACHE_FOR_COMMAND_LIST
    if (graphicsRootSignature == currentGraphicsRootSignature) {
        return;
    }
    currentGraphicsRootSignature = graphicsRootSignature;
#endif
    commandList->SetGraphicsRootSignature(graphicsRootSignature);
}

BE_INLINE void D3D12CommandList::SetPipelineState(ID3D12PipelineState *piplelineState) {
#ifdef ENABLE_STATE_CACHE_FOR_COMMAND_LIST
    if (piplelineState == currentPipelineState) {
        return;
    }
    currentPipelineState = piplelineState;
#endif
    commandList->SetPipelineState(piplelineState);
}

BE_INLINE void D3D12CommandList::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology) {
#ifdef ENABLE_STATE_CACHE_FOR_COMMAND_LIST
    if (primitiveTopology == currentPrimitiveTopology) {
        return;
    }
    currentPrimitiveTopology = primitiveTopology;
#endif
    commandList->IASetPrimitiveTopology(primitiveTopology);
}

class D3D12CommandListPool {
public:
    ~D3D12CommandListPool() { Shutdown(); }

    void                            Init(D3D12_COMMAND_LIST_TYPE commandListType, int maxCommandLists);
    void                            Shutdown();

    void                            Clear();
    D3D12CommandList *              Alloc();
    void                            Free(D3D12CommandList *commandList);

    int                             UsedCount() const { return usedCount; }

private:
    D3D12CommandList *              commandListPool = nullptr;
    int                             maxCommandLists = 0;
    LinkList<D3D12CommandList>      freeCommandLists;
    int                             usedCount = 0;
};
