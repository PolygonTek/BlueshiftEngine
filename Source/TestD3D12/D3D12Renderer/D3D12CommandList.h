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

#include "../RHI.h"
#include "D3D12VertexBuffer.h"
#include "D3D12IndexBuffer.h"
#include "D3D12PipelineState.h"

class D3D12Renderer;
class D3D12CommandListPool;

class D3D12CommandList : public RHI::CommandList {
    friend class D3D12Renderer;
    friend class D3D12CommandListPool;

public:
    virtual void                    Reset(bool resetCacheStates = true, const RHI::CommandList *primaryCommandList = nullptr) override;

    virtual void                    Close() override;
    virtual void                    Execute() override;
    virtual void                    ExecuteSecondary(RHI::CommandList *primaryCommandList, const RHI::FrameThreadData *frameThreadData) override;
    virtual void                    CloseAndExecute() override;
    virtual void                    CloseAndExecuteSecondary(RHI::CommandList *primaryCommandList, const RHI::FrameThreadData *frameThreadData) override;

    virtual RHI::FrameThreadData *  GetFrameThreadData() const override;

    D3D12_COMMAND_LIST_TYPE         GetCommandListType() const;

    D3D12CommandListPool *          GetParentPool() const { return parentPool; }

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

BE_INLINE void D3D12CommandList::Close() {
    HRESULT hr = GetGraphicsCommandList()->Close();
    assert(SUCCEEDED(hr));
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
    graphicsRootParametersDirtyMask = 0;
}

BE_INLINE void D3D12CommandList::SetComputeRootSignature(ID3D12RootSignature *computeRootSignature) {
#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (computeRootSignature == cachedComputeRootSignature) {
        return;
    }
    cachedComputeRootSignature = computeRootSignature;
#endif
    GetGraphicsCommandList()->SetComputeRootSignature(computeRootSignature);
    computeRootParametersDirtyMask = 0;
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
        graphicsRootParametersDirtyMask = 0;
    } else {
        GetGraphicsCommandList()->SetComputeRootSignature(d3d12PipelineState->rootSignature);
        computeRootParametersDirtyMask = 0;
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
