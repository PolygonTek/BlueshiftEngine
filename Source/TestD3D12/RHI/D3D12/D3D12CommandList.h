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

#pragma once

#include "../RHI.h"
#include "D3D12VertexBuffer.h"
#include "D3D12IndexBuffer.h"
#include "D3D12PipelineState.h"

class D3D12Renderer;
class D3D12CommandListPool;
class D3D12DescriptorRangePool;
struct D3D12RootBindingData;

/*
---------------------------------------------------------------------------------------------------

    Command List

    - D3D12CommandList 객체는 D3D12CommandListPool 객체에서 할당되고 관리된다.
    - 커맨드 리스트는 Reset() 함수로 초기화한 후, 커맨드를 기록하고 Close() 함수로 닫는다.
    - 닫힌 커맨드 리스트는 Execute() 함수를 호출하여 실행할 수 있다.
    - 각종 상태 변수들은 커맨드 리스트 단위로 캐싱한다. (같은 상태로 중복 세팅 코드가 호출되는 것을 방지를 위함)
    - 커맨드 리스트는 자신을 관리하는 커맨드 리스트 풀의 D3D12RootBindingData 객체를 참조하여, 같은 풀에서 할당한 커맨드 리스트들이 동일한 바인딩 데이터를 공유하도록 한다.
    - dirtyMask 의 각 비트는 루트 파라미터 하나에 대응하며, 그래픽스/컴퓨트 루트 시그니처 설정 이후 해당 루트 파라미터의 바인딩 상태가 변경되어 현재 command list 에 commit 되어야 함을 나타낸다.

---------------------------------------------------------------------------------------------------
*/

class D3D12CommandList : public RHI::CommandList {
    friend class D3D12Renderer;
    friend class D3D12CommandListPool;

public:

                                    // command list 와 상태 캐시를 초기화하여 재사용을 준비한다.
    virtual void                    Reset() override;

    virtual void                    Close() override;
    virtual void                    Execute() override;
    virtual void                    CloseAndExecute() override;

                                    // secondary command list 의 실행을 현재 primary command list 에 기록한다.
    virtual void                    ExecuteSecondary(RHI::CommandList *secondaryCommandList) override;

    D3D12_COMMAND_LIST_TYPE         GetCommandListType() const;

                                    // 이 command list 를 관리하는 command list pool 을 리턴한다.
    D3D12CommandListPool *          GetParentPool() const { return parentPool; }

    D3D12RootBindingData *          GetRootBindingData() const;

    void                            SetDescriptorTablePools(D3D12DescriptorRangePool *resourceDescriptorRangePool, D3D12DescriptorRangePool *samplerDescriptorRangePool);
    D3D12DescriptorRangePool *      GetResourceDescriptorRangePool() const { return resourceDescriptorRangePool; }
    D3D12DescriptorRangePool *      GetSamplerDescriptorRangePool() const { return samplerDescriptorRangePool; }

                                    // NOTE: PIX_SCOPED_EVENT 매크로에서 사용하기 위해, 포인터가 아닌 포인터 참조를 리턴하도록 한다.
    ID3D12CommandList *&            GetCommandList() { return commandList; }
    ID3D12GraphicsCommandList6 *&   GetGraphicsCommandList() { return reinterpret_cast<ID3D12GraphicsCommandList6 *&>(commandList); }

    void                            ResourceBarrier(ID3D12Resource *resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);
                                    // 다른 descriptor heap 을 설정한 뒤에는 PSO 와 필요한 리소스 바인딩을 다시 설정해야 한다.
    void                            SetDescriptorHeaps(int numDescriptorHeaps, ID3D12DescriptorHeap *descriptorHeaps[]);
    void                            SetGraphicsRootSignature(ID3D12RootSignature *graphicsRootSignature);
    void                            SetComputeRootSignature(ID3D12RootSignature *computeRootSignature);
    void                            SetPipelineState(const RHI::PipelineState *pipelineState);
    void                            SetBindlessRootDescriptorTables(D3D12_GPU_DESCRIPTOR_HANDLE resourceDescriptorGpuBaseHandle, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptorGpuBaseHandle);
    void                            SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology);
    void                            SetVertexBuffers(int startSlot, int count, const RHI::VertexBuffer *vertexBuffers[]);
    void                            SetVertexBuffer(int slot, const RHI::VertexBuffer *vertexBuffer);
    void                            SetVertexBufferView(int slot, const D3D12_VERTEX_BUFFER_VIEW &vbv);
    void                            SetIndexBuffer(const RHI::IndexBuffer *indexBuffer);
    void                            SetIndexBufferView(const D3D12_INDEX_BUFFER_VIEW &ibv);
    void                            SetBlendFactor(const BE1::Color4 &rgba);
    void                            SetStencilRef(uint32_t value);
    void                            SetShadingRate(RHI::ShadingRate shadingRate);

                                    // 다음 상태 설정을 생략하지 않도록 캐시를 무효화한다.
                                    // 실제 디스크립터 힙을 알고 있다면 invalidateDescriptorHeaps 를 false 로 지정하여 힙 캐시를 유지한다.
    void                            InvalidateStateCache(bool invalidateDescriptorHeaps = true);

private:
    void                            InvalidateRootSignatureDependentBindings();

    BE1::LinkList<D3D12CommandList> node;
    D3D12CommandListPool *          parentPool = nullptr;
    ID3D12CommandAllocator *        commandAllocator = nullptr;
    ID3D12CommandList *             commandList = nullptr;

    const D3D12PipelineState *      currentPSO = nullptr;
    uint64_t                        graphicsRootParametersDirtyMask = 0;
    uint64_t                        computeRootParametersDirtyMask = 0;
    D3D12DescriptorRangePool *      resourceDescriptorRangePool = nullptr;
    D3D12DescriptorRangePool *      samplerDescriptorRangePool = nullptr;
    ID3D12Resource *                shadingRateImage = nullptr;
    BE1::Array<D3D12_RESOURCE_BARRIER> endRenderPassBarriers;

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    bool                            IsSameDescriptorHeaps(int numDescriptorHeaps, ID3D12DescriptorHeap *descriptorHeaps[]);

    struct Cache {
        BE1::StaticArray<ID3D12DescriptorHeap *, 2> rootDescriptorHeaps;
        ID3D12RootSignature *       graphicsRootSignature = nullptr;
        ID3D12RootSignature *       computeRootSignature = nullptr;
        ID3D12RootSignature *       bindlessRootSignature = nullptr;
        D3D12_GPU_DESCRIPTOR_HANDLE bindlessResourceDescriptorGpuBaseHandle = {};
        D3D12_GPU_DESCRIPTOR_HANDLE bindlessSamplerDescriptorGpuBaseHandle = {};
        D3D12_PRIMITIVE_TOPOLOGY    primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        D3D12_VERTEX_BUFFER_VIEW    vertexBufferViews[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
        D3D12_INDEX_BUFFER_VIEW     indexBufferView = {};
        uint32_t                    vertexBufferValidMask = 0;
        bool                        indexBufferValid = false;
        bool                        blendFactorValid = false;
        bool                        stencilRefValid = false;
        bool                        shadingRateValid = false;
        BE1::Color4                 blendFactor = BE1::Color4(1, 1, 1, 1);
        uint32_t                    stencilRef = 0;
        D3D12_SHADING_RATE          shadingRate = D3D12_SHADING_RATE::D3D12_SHADING_RATE_1X1;
    };
    Cache                           cache;
#endif

    bool                            isSecondary = false;
};

BE_INLINE void D3D12CommandList::Close() {
    HRESULT hr = GetGraphicsCommandList()->Close();
    assert(SUCCEEDED(hr));
}

BE_INLINE void D3D12CommandList::SetDescriptorTablePools(D3D12DescriptorRangePool *resourceDescriptorRangePool, D3D12DescriptorRangePool *samplerDescriptorRangePool) {
    this->resourceDescriptorRangePool = resourceDescriptorRangePool;
    this->samplerDescriptorRangePool = samplerDescriptorRangePool;
}

BE_INLINE void D3D12CommandList::InvalidateRootSignatureDependentBindings() {
#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    cache.bindlessRootSignature = nullptr;
    cache.bindlessResourceDescriptorGpuBaseHandle = {};
    cache.bindlessSamplerDescriptorGpuBaseHandle = {};
#endif
}

BE_INLINE void D3D12CommandList::InvalidateStateCache(bool invalidateDescriptorHeaps) {
    currentPSO = nullptr;
    InvalidateRootSignatureDependentBindings();
    graphicsRootParametersDirtyMask = 0;
    computeRootParametersDirtyMask = 0;

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (invalidateDescriptorHeaps) {
        cache.rootDescriptorHeaps.SetCount(0);
    }
    cache.graphicsRootSignature = nullptr;
    cache.computeRootSignature = nullptr;
    cache.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    cache.vertexBufferValidMask = 0;
    cache.indexBufferValid = false;
    cache.blendFactorValid = false;
    cache.stencilRefValid = false;
    cache.shadingRateValid = false;
#endif
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
    if (numDescriptorHeaps != cache.rootDescriptorHeaps.Count()) {
        return false;
    }
    for (int i = 0; i < numDescriptorHeaps; ++i) {
        if (descriptorHeaps[i] != cache.rootDescriptorHeaps[i]) {
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

    cache.rootDescriptorHeaps.SetCount(numDescriptorHeaps);
    for (int i = 0; i < numDescriptorHeaps; ++i) {
        cache.rootDescriptorHeaps[i] = descriptorHeaps[i];
    }
#endif
    GetGraphicsCommandList()->SetDescriptorHeaps(numDescriptorHeaps, descriptorHeaps);

    // 디스크립터 힙 변경은 descriptor table 바인딩을 무효화한다.
    // 다음 SetPSO 에서 root signature 와 fallback bindless table 을 다시 설정하고, 일반 리소스는 호출자가 다시 바인딩해야 한다.
    // 다른 command list / 패스와 공유하는 임시 바인딩 데이터를 자동으로 재사용하지 않는다.
    InvalidateRootSignatureDependentBindings();
    graphicsRootParametersDirtyMask = 0;
    computeRootParametersDirtyMask = 0;

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    cache.graphicsRootSignature = nullptr;
    cache.computeRootSignature = nullptr;
#endif
}

BE_INLINE void D3D12CommandList::SetGraphicsRootSignature(ID3D12RootSignature *graphicsRootSignature) {
#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (graphicsRootSignature == cache.graphicsRootSignature) {
        return;
    }
    cache.graphicsRootSignature = graphicsRootSignature;
#endif
    GetGraphicsCommandList()->SetGraphicsRootSignature(graphicsRootSignature);
    InvalidateRootSignatureDependentBindings();
    graphicsRootParametersDirtyMask = 0; // 루트 시그니처를 바꿀 때 마다 리셋
}

BE_INLINE void D3D12CommandList::SetComputeRootSignature(ID3D12RootSignature *computeRootSignature) {
#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (computeRootSignature == cache.computeRootSignature) {
        return;
    }
    cache.computeRootSignature = computeRootSignature;
#endif
    GetGraphicsCommandList()->SetComputeRootSignature(computeRootSignature);
    InvalidateRootSignatureDependentBindings();
    computeRootParametersDirtyMask = 0; // 루트 시그니처를 바꿀 때 마다 리셋
}

BE_INLINE void D3D12CommandList::SetPipelineState(const RHI::PipelineState *pipelineState) {
    const D3D12PipelineState *_pipelineState = static_cast<const D3D12PipelineState *>(pipelineState);

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (_pipelineState != currentPSO) {
        GetGraphicsCommandList()->SetPipelineState(_pipelineState->pso);
    }
#else
    GetGraphicsCommandList()->SetPipelineState(_pipelineState->pso);
#endif

    // Root signature 와 topology 는 PSO 외부에서도 바뀔 수 있으므로 각각의 캐시로 확인한다.
    // 같은 PSO 여도 직접 변경되거나 heap 변경으로 무효화된 바인딩 상태를 복구해야 한다.
    if (pipelineState->isGraphics) {
        SetGraphicsRootSignature(_pipelineState->rootSignature);
        SetPrimitiveTopology(_pipelineState->primitiveTopology);
    } else {
        SetComputeRootSignature(_pipelineState->rootSignature);
    }
    currentPSO = _pipelineState;
}

BE_INLINE void D3D12CommandList::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology) {
#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (cache.primitiveTopology == primitiveTopology) {
        return;
    }
    cache.primitiveTopology = primitiveTopology;
#endif
    GetGraphicsCommandList()->IASetPrimitiveTopology(primitiveTopology);
}

BE_INLINE void D3D12CommandList::SetVertexBuffers(int startSlot, int count, const RHI::VertexBuffer *vertexBuffers[]) {
    D3D12_VERTEX_BUFFER_VIEW vbv[8];
    for (int i = 0; i < count; ++i) {
        vbv[i] = static_cast<const D3D12VertexBuffer *>(vertexBuffers[i])->vbv;
    }

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    bool needsUpdate = false;
    for (int i = 0; i < count; ++i) {
        int slot = startSlot + i;
        const uint32_t slotMask = uint32_t(1) << slot;

        if (!(cache.vertexBufferValidMask & slotMask) ||
            cache.vertexBufferViews[slot].BufferLocation != vbv[i].BufferLocation ||
            cache.vertexBufferViews[slot].SizeInBytes != vbv[i].SizeInBytes ||
            cache.vertexBufferViews[slot].StrideInBytes != vbv[i].StrideInBytes) {
            cache.vertexBufferViews[slot] = vbv[i];
            cache.vertexBufferValidMask |= slotMask;
            needsUpdate = true;
        }
    }
    if (needsUpdate) {
        GetGraphicsCommandList()->IASetVertexBuffers(startSlot, count, vbv);
    }
#else
    GetGraphicsCommandList()->IASetVertexBuffers(startSlot, count, vbv);
#endif
}

BE_INLINE void D3D12CommandList::SetVertexBuffer(int slot, const RHI::VertexBuffer *vertexBuffer) {
    SetVertexBufferView(slot, static_cast<const D3D12VertexBuffer *>(vertexBuffer)->vbv);
}

BE_INLINE void D3D12CommandList::SetVertexBufferView(int slot, const D3D12_VERTEX_BUFFER_VIEW &vbv) {
    assert(slot >= 0 && slot < D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    const uint32_t slotMask = uint32_t(1) << slot;
    if (!(cache.vertexBufferValidMask & slotMask) ||
        cache.vertexBufferViews[slot].BufferLocation != vbv.BufferLocation ||
        cache.vertexBufferViews[slot].SizeInBytes != vbv.SizeInBytes ||
        cache.vertexBufferViews[slot].StrideInBytes != vbv.StrideInBytes) {
        cache.vertexBufferViews[slot] = vbv;
        cache.vertexBufferValidMask |= slotMask;
        GetGraphicsCommandList()->IASetVertexBuffers(slot, 1, &vbv);
    }
#else
    GetGraphicsCommandList()->IASetVertexBuffers(slot, 1, &vbv);
#endif
}

BE_INLINE void D3D12CommandList::SetIndexBuffer(const RHI::IndexBuffer *indexBuffer) {
    SetIndexBufferView(static_cast<const D3D12IndexBuffer *>(indexBuffer)->ibv);
}

BE_INLINE void D3D12CommandList::SetIndexBufferView(const D3D12_INDEX_BUFFER_VIEW &ibv) {
#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (!cache.indexBufferValid ||
        cache.indexBufferView.BufferLocation != ibv.BufferLocation ||
        cache.indexBufferView.SizeInBytes != ibv.SizeInBytes ||
        cache.indexBufferView.Format != ibv.Format) {
        cache.indexBufferView = ibv;
        cache.indexBufferValid = true;
        GetGraphicsCommandList()->IASetIndexBuffer(&ibv);
    }
#else
    GetGraphicsCommandList()->IASetIndexBuffer(&ibv);
#endif
}

BE_INLINE void D3D12CommandList::SetBlendFactor(const BE1::Color4 &rgba) {
#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (cache.blendFactorValid && cache.blendFactor == rgba) {
        return;
    }
    cache.blendFactor = rgba;
    cache.blendFactorValid = true;
#endif
    GetGraphicsCommandList()->OMSetBlendFactor(rgba.Ptr());
}

BE_INLINE void D3D12CommandList::SetStencilRef(uint32_t value) {
#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (cache.stencilRefValid && cache.stencilRef == value) {
        return;
    }
    cache.stencilRef = value;
    cache.stencilRefValid = true;
#endif
    GetGraphicsCommandList()->OMSetStencilRef(value);
}

BE_INLINE void D3D12CommandList::CloseAndExecute() {
    HRESULT hr = GetGraphicsCommandList()->Close();
    assert(SUCCEEDED(hr));

    Execute();
}
