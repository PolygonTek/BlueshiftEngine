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
#include "D3D12Renderer.h"

#define ENABLE_STATE_CACHE_FOR_COMMAND_LIST

class D3D12CommandListPool;

class D3D12CommandList {
public:
    void                            Reset();

    void                            CloseAndExecute();

    void                            ResourceBarrier(ID3D12Resource *resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);

    void                            SetDescriptorHeaps(int numDescriptorHeaps, ID3D12DescriptorHeap* descriptorHeaps[]);
    void                            SetGraphicsRootSignature(ID3D12RootSignature* graphicsRootSignature);
    void                            SetPipelineState(ID3D12PipelineState* piplelineState);
    void                            SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology);
    void                            SetVertexBuffers(UINT startSlot, UINT numViews, const D3D12_VERTEX_BUFFER_VIEW* vertexBufferView);
    void                            SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* indexBufferView);

    ID3D12CommandAllocator *        commandAllocator = nullptr;
    ID3D12GraphicsCommandList *     graphicsCommandList = nullptr;
    D3D12CommandListPool *          parentPool = nullptr;
    LinkList<D3D12CommandList>      node;

#ifdef ENABLE_STATE_CACHE_FOR_COMMAND_LIST
    bool                            IsSameDescriptorHeaps(int numDescriptorHeaps, ID3D12DescriptorHeap *descriptorHeaps[]);

    StaticArray<ID3D12DescriptorHeap *, 16> cachedRootDescriptorHeaps;
    ID3D12RootSignature *           cachedGraphicsRootSignature = nullptr;
    ID3D12PipelineState *           cachedPipelineState = nullptr;
    D3D12_PRIMITIVE_TOPOLOGY        cachedPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    UINT                            cachedVertexBufferStartSlot = -1;
    UINT                            cachedVertexBufferNumViews = -1;
    D3D12_VERTEX_BUFFER_VIEW        cachedVertexBufferViews[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
    D3D12_INDEX_BUFFER_VIEW         cachedIndexBufferView = {};
#endif
};

BE_INLINE void D3D12CommandList::Reset() {
    // NOTE: CommandList 는 GPU 작업 완료 여부와 상관없이 Reset 하여 재사용할 수 있지만,
    // CommandAllocator 는 GPU 에서 해당 CommandList 를 사용하는 작업이 모두 완료되기 전에는 Reset 하면 안된다.
    // 
    // CommandAllocator 를 재사용하도록 리셋
    commandAllocator->Reset();

    // CommandList 를 CommandAllocator 를 이용하여 초기 상태로 리셋
    graphicsCommandList->Reset(commandAllocator, nullptr);

#ifdef ENABLE_STATE_CACHE_FOR_COMMAND_LIST
    // 각종 상태를 초기값으로 변경
    cachedRootDescriptorHeaps.SetCount(0);
    cachedGraphicsRootSignature = nullptr;
    cachedPipelineState = nullptr;
    cachedPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    for (int i = 0; i < D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++i) {
        cachedVertexBufferViews[i] = {};
    }
    cachedIndexBufferView = {};
#endif
}

BE_INLINE void D3D12CommandList::CloseAndExecute() {
    graphicsCommandList->Close();

    ID3D12CommandList *execCommandLists[] = { graphicsCommandList };
    renderer.commandQueue->ExecuteCommandLists(COUNT_OF(execCommandLists), execCommandLists);
}

BE_INLINE void D3D12CommandList::ResourceBarrier(ID3D12Resource *resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) {
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = stateBefore;
    barrier.Transition.StateAfter = stateAfter;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    graphicsCommandList->ResourceBarrier(1, &barrier);
}

#ifdef ENABLE_STATE_CACHE_FOR_COMMAND_LIST
BE_INLINE bool D3D12CommandList::IsSameDescriptorHeaps(int numDescriptorHeaps, ID3D12DescriptorHeap *descriptorHeaps[]) {
    if (numDescriptorHeaps != cachedRootDescriptorHeaps.Count()) {
        return false;
    }
    for (int i = 0; i < numDescriptorHeaps; ++i) {
        if (descriptorHeaps[i] != cachedRootDescriptorHeaps[i]) {
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
    cachedRootDescriptorHeaps.SetCount(numDescriptorHeaps);
    for (int i = 0; i < numDescriptorHeaps; ++i) {
        cachedRootDescriptorHeaps[i] = descriptorHeaps[i];
    }
#endif
    graphicsCommandList->SetDescriptorHeaps(numDescriptorHeaps, descriptorHeaps);
}

BE_INLINE void D3D12CommandList::SetGraphicsRootSignature(ID3D12RootSignature *graphicsRootSignature) {
#ifdef ENABLE_STATE_CACHE_FOR_COMMAND_LIST
    if (graphicsRootSignature == cachedGraphicsRootSignature) {
        return;
    }
    cachedGraphicsRootSignature = graphicsRootSignature;
#endif
    graphicsCommandList->SetGraphicsRootSignature(graphicsRootSignature);
}

BE_INLINE void D3D12CommandList::SetPipelineState(ID3D12PipelineState *piplelineState) {
#ifdef ENABLE_STATE_CACHE_FOR_COMMAND_LIST
    if (piplelineState == cachedPipelineState) {
        return;
    }
    cachedPipelineState = piplelineState;
#endif
    graphicsCommandList->SetPipelineState(piplelineState);
}

BE_INLINE void D3D12CommandList::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology) {
#ifdef ENABLE_STATE_CACHE_FOR_COMMAND_LIST
    if (primitiveTopology == cachedPrimitiveTopology) {
        return;
    }
    cachedPrimitiveTopology = primitiveTopology;
#endif
    graphicsCommandList->IASetPrimitiveTopology(primitiveTopology);
}

BE_INLINE void D3D12CommandList::SetVertexBuffers(UINT startSlot, UINT numViews, const D3D12_VERTEX_BUFFER_VIEW *vertexBufferViews) {
#ifdef ENABLE_STATE_CACHE_FOR_COMMAND_LIST
    bool needsUpdate = false;
    for (int i = 0; i < numViews; ++i) {
        int slot = startSlot + i;

        if (cachedVertexBufferViews[slot].BufferLocation != vertexBufferViews[i].BufferLocation ||
            cachedVertexBufferViews[slot].SizeInBytes != vertexBufferViews[i].SizeInBytes ||
            cachedVertexBufferViews[slot].StrideInBytes != vertexBufferViews[i].StrideInBytes) {
            cachedVertexBufferViews[slot] = vertexBufferViews[i];
            needsUpdate = true;
        }
    }
    if (!needsUpdate) {
        return;
    }
#endif
    graphicsCommandList->IASetVertexBuffers(startSlot, numViews, vertexBufferViews);
}

BE_INLINE void D3D12CommandList::SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW *indexBufferView) {
#ifdef ENABLE_STATE_CACHE_FOR_COMMAND_LIST
    if (!(cachedIndexBufferView.BufferLocation != indexBufferView->BufferLocation ||
        cachedIndexBufferView.SizeInBytes != indexBufferView->SizeInBytes ||
        cachedIndexBufferView.Format != indexBufferView->Format)) {
        return;
    }
    cachedIndexBufferView = *indexBufferView;
#endif
    graphicsCommandList->IASetIndexBuffer(indexBufferView);
}

class D3D12CommandListPool {
public:
    D3D12CommandListPool() = default;
    D3D12CommandListPool(D3D12_COMMAND_LIST_TYPE commandListType, int maxCommandLists) { Init(commandListType, maxCommandLists); }
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
