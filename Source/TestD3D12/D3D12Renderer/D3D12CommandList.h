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

#include "D3D12Renderer.h"
#include "D3D12VertexBuffer.h"
#include "D3D12IndexBuffer.h"
#include "D3D12PipelineState.h"

class D3D12CommandListPool;

class D3D12CommandList : public RHI::CommandList {
    friend class D3D12Renderer;
    friend class D3D12CommandListPool;

public:
    virtual void                    Reset(bool resetCacheStates = true, const RHI::CommandList *primaryCommandList = nullptr) override;

    virtual void                    CloseAndExecute(RHI::CommandQueueType queueType) override;
    virtual void                    CloseAndExecuteSecondary(RHI::CommandList *primaryCommandList) override;

    virtual int                     GetThreadIndex() const override;

    D3D12_COMMAND_LIST_TYPE         GetCommandListType() const;

                                    // NOTE: PIX_SCOPED_EVENT 매크로에서 사용하기 위해, 포인터가 아닌 포인터 참조를 리턴하도록 한다.
    ID3D12CommandList *&            GetCommandList() { return commandList; }
    ID3D12GraphicsCommandList6 *&   GetGraphicsCommandList() { return reinterpret_cast<ID3D12GraphicsCommandList6 *&>(commandList); }

    void                            ResourceBarrier(ID3D12Resource *resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);

    void                            SetDescriptorHeaps(int numDescriptorHeaps, ID3D12DescriptorHeap *descriptorHeaps[]);
    void                            SetGraphicsRootSignature(ID3D12RootSignature *graphicsRootSignature);
    void                            SetComputeRootSignature(ID3D12RootSignature *computeRootSignature);

    void                            SetPipelineState(const RHI::PipelineState *pipelineState);
    void                            SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology);
    void                            SetVertexBuffers(int startSlot, int numViews, const RHI::VertexBuffer *vertexBuffers[]);
    void                            SetVertexBuffer(int slot, const RHI::VertexBuffer *vertexBuffer);
    void                            SetIndexBuffer(const RHI::IndexBuffer *indexBuffer);
    void                            SetBlendFactor(const BE1::Color4 &rgba);
    void                            SetStencilRef(uint32_t value);
    void                            SetShadingRate(RHI::ShadingRate shadingRate);

private:
    ID3D12CommandAllocator *        commandAllocator = nullptr;
    ID3D12CommandList *             commandList = nullptr;
    D3D12CommandListPool *          parentPool = nullptr;
    BE1::LinkList<D3D12CommandList> node;
    const D3D12PipelineState *      currentPSO = nullptr;
    uint64_t                        graphicsRootParametersDirtyMask = 0;
    uint64_t                        computeRootParametersDirtyMask = 0;
    BE1::Array<D3D12_RESOURCE_BARRIER> endRenderPassBarriers;
    ID3D12Resource *                shadingRateImage = nullptr;

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    bool                            IsSameDescriptorHeaps(int numDescriptorHeaps, ID3D12DescriptorHeap *descriptorHeaps[]);

    BE1::StaticArray<ID3D12DescriptorHeap*, 16> cachedRootDescriptorHeaps;
    ID3D12RootSignature *           cachedGraphicsRootSignature = nullptr;
    ID3D12RootSignature *           cachedComputeRootSignature = nullptr;
    D3D12_PRIMITIVE_TOPOLOGY        cachedPrimitiveTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    D3D12_VERTEX_BUFFER_VIEW        cachedVertexBufferViews[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
    D3D12_INDEX_BUFFER_VIEW         cachedIndexBufferView = {};
    BE1::Color4                     cachedBlendFactor = BE1::Color4(1, 1, 1, 1);
    uint32_t                        cachedStencilRef = 0;
    D3D12_SHADING_RATE              cachedShadingRate = D3D12_SHADING_RATE::D3D12_SHADING_RATE_1X1;
#endif

    bool                            secondary = false;
};

BE_INLINE void D3D12CommandList::Reset(bool resetCacheStates, const RHI::CommandList *primaryCommandList) {
    // NOTE: CommandList 는 GPU 작업 완료 여부와 상관없이 Reset 하여 재사용할 수 있지만,
    // CommandAllocator 는 GPU 에서 해당 CommandList 를 사용하는 작업이 모두 완료되기 전에는 Reset 하면 안된다.
    // 
    // CommandAllocator 를 재사용하도록 리셋
    commandAllocator->Reset();

    // CommandList 를 CommandAllocator 를 이용하여 초기 상태로 리셋
    GetGraphicsCommandList()->Reset(commandAllocator, nullptr);

    currentPSO = nullptr;
    graphicsRootParametersDirtyMask = 0;
    computeRootParametersDirtyMask = 0;

    // BeginRenderPass/EndRenderPass 중간에 Reset 하면 assert 가 발생한다
    assert(endRenderPassBarriers.Count() == 0);
    assert(!shadingRateImage);

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (resetCacheStates) {
        cachedRootDescriptorHeaps.SetCount(0);
        cachedPrimitiveTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

        if (primaryCommandList) {
            // 모든 캐시된 상태들을 primaryCommandList 의 상태와 같도록 초기화
            // SetDescriptorHeaps() 와 IASetPrimitiveTopology() 는 상속되지 않는다.
            const D3D12CommandList *d3d12PrimaryCommandList = static_cast<const D3D12CommandList *>(primaryCommandList);
            cachedGraphicsRootSignature = d3d12PrimaryCommandList->cachedGraphicsRootSignature;
            cachedComputeRootSignature = d3d12PrimaryCommandList->cachedComputeRootSignature;
            for (int i = 0; i < D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++i) {
                cachedVertexBufferViews[i] = d3d12PrimaryCommandList->cachedVertexBufferViews[i];
            }
            cachedIndexBufferView = d3d12PrimaryCommandList->cachedIndexBufferView;
            cachedBlendFactor = d3d12PrimaryCommandList->cachedBlendFactor;
            cachedStencilRef = d3d12PrimaryCommandList->cachedStencilRef;
            cachedShadingRate = d3d12PrimaryCommandList->cachedShadingRate;
        } else {
            // 모든 캐시된 상태들을 초기값으로 변경
            cachedGraphicsRootSignature = nullptr;
            cachedComputeRootSignature = nullptr;
            for (int i = 0; i < D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++i) {
                cachedVertexBufferViews[i] = {};
            }
            cachedIndexBufferView = {};
            cachedBlendFactor = BE1::Color4(1, 1, 1, 1);
            cachedStencilRef = 0;
            cachedShadingRate = D3D12_SHADING_RATE::D3D12_SHADING_RATE_1X1;
        }
    }
#endif
}

BE_INLINE void D3D12CommandList::CloseAndExecute(RHI::CommandQueueType queueType) {
    assert(queueType < RHI::CommandQueueType::Count);

    HRESULT hr = GetGraphicsCommandList()->Close();
    assert(SUCCEEDED(hr));

    ID3D12CommandList *execCommandLists[] = { commandList };
    renderer->commandQueues[to_int(queueType)]->ExecuteCommandLists(COUNT_OF(execCommandLists), execCommandLists);
}

BE_INLINE void D3D12CommandList::CloseAndExecuteSecondary(RHI::CommandList *primaryCommandList) {
    HRESULT hr = GetGraphicsCommandList()->Close();
    assert(SUCCEEDED(hr));

    D3D12CommandList *d3d12PrimaryCommandList = static_cast<D3D12CommandList *>(primaryCommandList);
    d3d12PrimaryCommandList->GetGraphicsCommandList()->ExecuteBundle(GetGraphicsCommandList());
}

BE_INLINE void D3D12CommandList::ResourceBarrier(ID3D12Resource *resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) {
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = stateBefore;
    barrier.Transition.StateAfter = stateAfter;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    GetGraphicsCommandList()->ResourceBarrier(1, &barrier);
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
    GetGraphicsCommandList()->SetDescriptorHeaps(numDescriptorHeaps, descriptorHeaps);
}

BE_INLINE void D3D12CommandList::SetGraphicsRootSignature(ID3D12RootSignature *graphicsRootSignature) {
#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (graphicsRootSignature == cachedGraphicsRootSignature) {
        return;
    }
    cachedGraphicsRootSignature = graphicsRootSignature;
#endif
    GetGraphicsCommandList()->SetGraphicsRootSignature(graphicsRootSignature);
}

BE_INLINE void D3D12CommandList::SetComputeRootSignature(ID3D12RootSignature *computeRootSignature) {
#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (computeRootSignature == cachedComputeRootSignature) {
        return;
    }
    cachedComputeRootSignature = computeRootSignature;
#endif
    GetGraphicsCommandList()->SetComputeRootSignature(computeRootSignature);
}

BE_INLINE void D3D12CommandList::SetPipelineState(const RHI::PipelineState *pipelineState) {
    const D3D12PipelineState *d3d12PipelineState = static_cast<const D3D12PipelineState *>(pipelineState);

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (d3d12PipelineState != currentPSO) {
        GetGraphicsCommandList()->SetPipelineState(d3d12PipelineState->pso);

        if (pipelineState->graphics) {
            SetGraphicsRootSignature(d3d12PipelineState->rootSignature);
            SetPrimitiveTopology(d3d12PipelineState->primitiveTopology);
        } else {
            SetComputeRootSignature(d3d12PipelineState->rootSignature);
        }
        currentPSO = d3d12PipelineState;
    }
#else
    GetGraphicsCommandList()->SetPipelineState(d3d12PipelineState->pso);

    if (pipelineState->graphics) {
        GetGraphicsCommandList()->SetGraphicsRootSignature(d3d12PipelineState->rootSignature);
        GetGraphicsCommandList()->IASetPrimitiveTopology(d3d12PipelineState->primitiveTopology);
    } else {
        GetGraphicsCommandList()->SetComputeRootSignature(d3d12PipelineState->rootSignature);
    }
    currentPSO = d3d12PipelineState;
#endif
}

BE_INLINE void D3D12CommandList::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology) {
#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (cachedPrimitiveTopology == primitiveTopology) {
        return;
    }
    cachedPrimitiveTopology = primitiveTopology;
#endif
    GetGraphicsCommandList()->IASetPrimitiveTopology(primitiveTopology);
}

BE_INLINE void D3D12CommandList::SetVertexBuffers(int startSlot, int numViews, const RHI::VertexBuffer *vertexBuffers[]) {
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
    GetGraphicsCommandList()->IASetVertexBuffers(startSlot, numViews, vbv);
}

BE_INLINE void D3D12CommandList::SetVertexBuffer(int slot, const RHI::VertexBuffer *vertexBuffer) {
    const D3D12_VERTEX_BUFFER_VIEW &vbv = static_cast<const D3D12VertexBuffer *>(vertexBuffer)->vbv;

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (!(cachedVertexBufferViews[slot].BufferLocation != vbv.BufferLocation ||
        cachedVertexBufferViews[slot].SizeInBytes != vbv.SizeInBytes ||
        cachedVertexBufferViews[slot].StrideInBytes != vbv.StrideInBytes)) {
        cachedVertexBufferViews[slot] = vbv;
        return;
    }
#endif
    GetGraphicsCommandList()->IASetVertexBuffers(slot, 1, &vbv);
}

BE_INLINE void D3D12CommandList::SetIndexBuffer(const RHI::IndexBuffer *indexBuffer) {
    const D3D12IndexBuffer *d3d12IndexBuffer = static_cast<const D3D12IndexBuffer *>(indexBuffer);

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (!(cachedIndexBufferView.BufferLocation != d3d12IndexBuffer->ibv.BufferLocation ||
        cachedIndexBufferView.SizeInBytes != d3d12IndexBuffer->ibv.SizeInBytes ||
        cachedIndexBufferView.Format != d3d12IndexBuffer->ibv.Format)) {
        return;
    }
    cachedIndexBufferView = d3d12IndexBuffer->ibv;
#endif
    GetGraphicsCommandList()->IASetIndexBuffer(&d3d12IndexBuffer->ibv);
}

BE_INLINE void D3D12CommandList::SetBlendFactor(const BE1::Color4 &rgba) {
#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (cachedBlendFactor == rgba) {
        return;
    }
    cachedBlendFactor = rgba;
#endif
    GetGraphicsCommandList()->OMSetBlendFactor(rgba.Ptr());
}

BE_INLINE void D3D12CommandList::SetStencilRef(uint32_t value) {
#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (cachedStencilRef == value) {
        return;
    }
    cachedStencilRef = value;
#endif
    GetGraphicsCommandList()->OMSetStencilRef(value);
}
