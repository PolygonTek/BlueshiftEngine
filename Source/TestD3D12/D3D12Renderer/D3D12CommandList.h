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

class D3D12CommandListPool;

class D3D12CommandList {
public:
    void                            Reset(bool resetCacheStates = true);

    void                            CloseAndExecute(D3D12CommandQueueType queueType);

    void                            ResourceBarrier(ID3D12Resource *resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);

    void                            SetDescriptorHeaps(int numDescriptorHeaps, ID3D12DescriptorHeap *descriptorHeaps[]);
    void                            SetGraphicsRootSignature(ID3D12RootSignature *graphicsRootSignature);
    void                            SetPipelineState(RHIRenderer::PipelineState *piplelineState);
    void                            SetPrimitiveTopology(RHIRenderer::PrimitiveTopology primitiveTopology);
    void                            SetVertexBuffers(int startSlot, int numViews, const RHIRenderer::VertexBuffer *vertexBuffers[]);
    void                            SetVertexBuffer(int slot, const RHIRenderer::VertexBuffer *vertexBuffer);
    void                            SetIndexBuffer(const RHIRenderer::IndexBuffer *indexBuffer);

    static D3D12_PRIMITIVE_TOPOLOGY ToD3D12PrimitiveTopology(RHIRenderer::PrimitiveTopology primitiveTopology);

    ID3D12CommandAllocator *        commandAllocator = nullptr;
    ID3D12GraphicsCommandList *     graphicsCommandList = nullptr;
    D3D12CommandListPool *          parentPool = nullptr;
    LinkList<D3D12CommandList>      node;

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    bool                            IsSameDescriptorHeaps(int numDescriptorHeaps, ID3D12DescriptorHeap *descriptorHeaps[]);

    StaticArray<ID3D12DescriptorHeap *, 16> cachedRootDescriptorHeaps;
    ID3D12RootSignature *           cachedGraphicsRootSignature = nullptr;
    ID3D12PipelineState *           cachedPipelineState = nullptr;
    D3D12_PRIMITIVE_TOPOLOGY        cachedPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    D3D12_VERTEX_BUFFER_VIEW        cachedVertexBufferViews[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
    D3D12_INDEX_BUFFER_VIEW         cachedIndexBufferView = {};
#endif
};

BE_INLINE void D3D12CommandList::Reset(bool resetCacheStates) {
    // NOTE: CommandList 는 GPU 작업 완료 여부와 상관없이 Reset 하여 재사용할 수 있지만,
    // CommandAllocator 는 GPU 에서 해당 CommandList 를 사용하는 작업이 모두 완료되기 전에는 Reset 하면 안된다.
    // 
    // CommandAllocator 를 재사용하도록 리셋
    commandAllocator->Reset();

    // CommandList 를 CommandAllocator 를 이용하여 초기 상태로 리셋
    graphicsCommandList->Reset(commandAllocator, nullptr);

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (resetCacheStates) {
        // 모든 캐시된 상태들을 초기값으로 변경
        cachedRootDescriptorHeaps.SetCount(0);
        cachedGraphicsRootSignature = nullptr;
        cachedPipelineState = nullptr;
        cachedPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        for (int i = 0; i < D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++i) {
            cachedVertexBufferViews[i] = {};
        }
        cachedIndexBufferView = {};
    }
#endif
}

BE_INLINE void D3D12CommandList::CloseAndExecute(D3D12CommandQueueType queueType) {
    assert(queueType < D3D12CommandQueueType::Count);

    graphicsCommandList->Close();

    ID3D12CommandList *execCommandLists[] = { graphicsCommandList };
    renderer.commandQueues[static_cast<int>(queueType)]->ExecuteCommandLists(COUNT_OF(execCommandLists), execCommandLists);
}

BE_INLINE void D3D12CommandList::ResourceBarrier(ID3D12Resource *resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) {
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = stateBefore;
    barrier.Transition.StateAfter = stateAfter;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    graphicsCommandList->ResourceBarrier(1, &barrier);
}

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
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
#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
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
#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (graphicsRootSignature == cachedGraphicsRootSignature) {
        return;
    }
    cachedGraphicsRootSignature = graphicsRootSignature;
#endif
    graphicsCommandList->SetGraphicsRootSignature(graphicsRootSignature);
}

BE_INLINE void D3D12CommandList::SetPipelineState(RHIRenderer::PipelineState *piplelineState) {
    D3D12PipelineState *d3d12PipelineState = static_cast<D3D12PipelineState *>(piplelineState);

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (d3d12PipelineState->pso == cachedPipelineState) {
        return;
    }
    cachedPipelineState = d3d12PipelineState->pso;
#endif
    graphicsCommandList->SetPipelineState(d3d12PipelineState->pso);
    graphicsCommandList->SetGraphicsRootSignature(d3d12PipelineState->rootSignature);
}

BE_INLINE void D3D12CommandList::SetPrimitiveTopology(RHIRenderer::PrimitiveTopology primitiveTopology) {
    D3D12_PRIMITIVE_TOPOLOGY d3d12PrimitiveTopology = D3D12CommandList::ToD3D12PrimitiveTopology(primitiveTopology);

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (d3d12PrimitiveTopology == cachedPrimitiveTopology) {
        return;
    }
    cachedPrimitiveTopology = d3d12PrimitiveTopology;
#endif
    graphicsCommandList->IASetPrimitiveTopology(d3d12PrimitiveTopology);
}

BE_INLINE void D3D12CommandList::SetVertexBuffers(int startSlot, int numViews, const RHIRenderer::VertexBuffer *vertexBuffers[]) {
    D3D12_VERTEX_BUFFER_VIEW vbv[8];
    for (int i = 0; i < numViews; ++i) {
        vbv[i] = static_cast<const D3D12VertexBuffer *>(vertexBuffers[i])->vbv;
    }

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    bool needsUpdate = false;
    for (int i = 0; i < numViews; ++i) {
        int slot = startSlot + i;

        if (cachedVertexBufferViews[slot].BufferLocation != vbv[i].BufferLocation ||
            cachedVertexBufferViews[slot].SizeInBytes != vbv[i].SizeInBytes ||
            cachedVertexBufferViews[slot].StrideInBytes != vbv[i].StrideInBytes) {
            cachedVertexBufferViews[slot] = vbv[i];
            needsUpdate = true;
            break;
        }
    }
    if (!needsUpdate) {
        return;
    }
#endif
    graphicsCommandList->IASetVertexBuffers(startSlot, numViews, vbv);
}

BE_INLINE void D3D12CommandList::SetVertexBuffer(int slot, const RHIRenderer::VertexBuffer *vertexBuffer) {
    const D3D12_VERTEX_BUFFER_VIEW &vbv = static_cast<const D3D12VertexBuffer *>(vertexBuffer)->vbv;

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (!(cachedVertexBufferViews[slot].BufferLocation != vbv.BufferLocation ||
        cachedVertexBufferViews[slot].SizeInBytes != vbv.SizeInBytes ||
        cachedVertexBufferViews[slot].StrideInBytes != vbv.StrideInBytes)) {
        cachedVertexBufferViews[slot] = vbv;
        return;
    }
#endif
    graphicsCommandList->IASetVertexBuffers(slot, 1, &vbv);
}

BE_INLINE void D3D12CommandList::SetIndexBuffer(const RHIRenderer::IndexBuffer *indexBuffer) {
    const D3D12IndexBuffer *d3d12IndexBuffer = static_cast<const D3D12IndexBuffer *>(indexBuffer);

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (!(cachedIndexBufferView.BufferLocation != d3d12IndexBuffer->ibv.BufferLocation ||
        cachedIndexBufferView.SizeInBytes != d3d12IndexBuffer->ibv.SizeInBytes ||
        cachedIndexBufferView.Format != d3d12IndexBuffer->ibv.Format)) {
        return;
    }
    cachedIndexBufferView = d3d12IndexBuffer->ibv;
#endif
    graphicsCommandList->IASetIndexBuffer(&d3d12IndexBuffer->ibv);
}
