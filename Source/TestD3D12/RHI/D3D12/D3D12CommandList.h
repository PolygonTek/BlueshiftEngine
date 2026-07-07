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
class D3D12RootBindingData;
class D3D12DescriptorTablePool;

/*
---------------------------------------------------------------------------------------------------

    Command List

    - D3D12CommandList 객체는 D3D12CommandListPool 객체에서 할당되고 관리된다.
    - 커맨드 리스트는 Reset() 함수로 초기화한 후, 커맨드를 기록하고 Close() 함수로 닫는다.
    - 닫힌 커맨드 리스트는 Execute() 함수를 호출하여 실행할 수 있다.
    - 각종 상태 변수들은 커맨드 리스트 단위로 캐싱한다. (같은 상태로 중복 세팅 코드가 호출되는 것을 방지를 위함)
    - 커맨드 리스트는 부모 (커맨드 리스트 풀) 의 D3D12BindingData 객체를 참조하여, 같은 풀에서 할당한 커맨드 리스트들이 동일한 바인딩 데이터를 공유하도록 한다.
    - dirtyMask 의 각 비트는 루트 파라미터 하나에 대응하며, 그래픽스/컴퓨트 루트 시그니쳐 설정 이후 해당 루트 파라미터의 바인딩 상태가 변경되어 현재 command list 에 commit 되어야 함을 나타낸다.

---------------------------------------------------------------------------------------------------
*/

class D3D12CommandList : public RHI::CommandList {
    friend class D3D12Renderer;
    friend class D3D12CommandListPool;

public:

                                    // command list 를 재사용하기 위해 초기화한다.
                                    // secondary (bundle) command list 의 경우 primaryCommandList 를 전달받아 primary 와 동일한 캐시 상태를 기준으로 초기화한다.
    virtual void                    Reset(bool resetCacheStates = true, const RHI::CommandList *primaryCommandList = nullptr) override;

    virtual void                    Close() override;
    virtual void                    Execute() override;
    virtual void                    CloseAndExecute() override;

                                    // secondary (bundle) command list 를 현재 primary command list 에 기록한다.
    virtual void                    ExecuteSecondary(RHI::CommandList *secondaryCommandList) override;

    D3D12_COMMAND_LIST_TYPE         GetCommandListType() const;

                                    // 부모 (커맨드 리스트 풀) 객체를 리턴한다.
    D3D12CommandListPool *          GetParentPool() const { return parentPool; }

    D3D12RootBindingData *          GetRootBindingData() const;

    void                            SetDescriptorTablePools(D3D12DescriptorTablePool *resDescriptorTablePool, D3D12DescriptorTablePool *samDescriptorTablePool);
    D3D12DescriptorTablePool *      GetResDescriptorTablePool() const { return resDescriptorTablePool; }
    D3D12DescriptorTablePool *      GetSamDescriptorTablePool() const { return samDescriptorTablePool; }

                                    // NOTE: PIX_SCOPED_EVENT 매크로에서 사용하기 위해, 포인터가 아닌 포인터 참조를 리턴하도록 한다.
    ID3D12CommandList *&            GetCommandList() { return commandList; }
    ID3D12GraphicsCommandList6 *&   GetGraphicsCommandList() { return reinterpret_cast<ID3D12GraphicsCommandList6 *&>(commandList); }

    void                            ResourceBarrier(ID3D12Resource *resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);
    void                            SetDescriptorHeaps(int numDescriptorHeaps, ID3D12DescriptorHeap *descriptorHeaps[]);
    void                            SetGraphicsRootSignature(ID3D12RootSignature *graphicsRootSignature);
    void                            SetComputeRootSignature(ID3D12RootSignature *computeRootSignature);
    void                            SetPipelineState(const RHI::PipelineState *pipelineState);
    void                            SetBindlessRootDescriptorTables(D3D12_GPU_DESCRIPTOR_HANDLE resourceDescriptorGpuBaseHandle, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptorGpuBaseHandle);
    void                            SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology);
    void                            SetVertexBuffers(int startSlot, int count, const RHI::VertexBuffer *vertexBuffers[]);
    void                            SetVertexBuffer(int slot, const RHI::VertexBuffer *vertexBuffer);
    void                            SetIndexBuffer(const RHI::IndexBuffer *indexBuffer);
    void                            SetBlendFactor(const BE1::Color4 &rgba);
    void                            SetStencilRef(uint32_t value);
    void                            SetShadingRate(RHI::ShadingRate shadingRate);

    void                            InvalidateStateCache();

private:
    void                            InvalidateRootSignatureDependentBindings();

    BE1::LinkList<D3D12CommandList> node;
    D3D12CommandListPool *          parentPool = nullptr;
    ID3D12CommandAllocator *        commandAllocator = nullptr;
    ID3D12CommandList *             commandList = nullptr;

    const D3D12PipelineState *      currentPSO = nullptr;
    uint64_t                        graphicsRootParametersDirtyMask = 0;
    uint64_t                        computeRootParametersDirtyMask = 0;
    D3D12DescriptorTablePool *      resDescriptorTablePool = nullptr;
    D3D12DescriptorTablePool *      samDescriptorTablePool = nullptr;
    ID3D12Resource *                shadingRateImage = nullptr;
    BE1::Array<D3D12_RESOURCE_BARRIER> endRenderPassBarriers;
    ID3D12RootSignature *           bindlessRootSignature = nullptr;
    D3D12_GPU_DESCRIPTOR_HANDLE     bindlessResourceDescriptorGpuBaseHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE     bindlessSamplerDescriptorGpuBaseHandle = {};

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

    bool                            isSecondary = false;
};

BE_INLINE void D3D12CommandList::Close() {
    HRESULT hr = GetGraphicsCommandList()->Close();
    assert(SUCCEEDED(hr));
}

BE_INLINE void D3D12CommandList::SetDescriptorTablePools(D3D12DescriptorTablePool *resDescriptorTablePool, D3D12DescriptorTablePool *samDescriptorTablePool) {
    this->resDescriptorTablePool = resDescriptorTablePool;
    this->samDescriptorTablePool = samDescriptorTablePool;
}

BE_INLINE void D3D12CommandList::InvalidateRootSignatureDependentBindings() {
    bindlessRootSignature = nullptr;
    bindlessResourceDescriptorGpuBaseHandle = {};
    bindlessSamplerDescriptorGpuBaseHandle = {};
}

BE_INLINE void D3D12CommandList::InvalidateStateCache() {
    currentPSO = nullptr;
    InvalidateRootSignatureDependentBindings();

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    cachedRootDescriptorHeaps.SetCount(0);
    cachedGraphicsRootSignature = nullptr;
    cachedComputeRootSignature = nullptr;
    cachedPrimitiveTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    for (int i = 0; i < D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++i) {
        cachedVertexBufferViews[i] = {};
    }
    cachedIndexBufferView = {};
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
    InvalidateRootSignatureDependentBindings();
    graphicsRootParametersDirtyMask = 0; // 루트 시그니쳐를 바꿀 때 마다 리셋
}

BE_INLINE void D3D12CommandList::SetComputeRootSignature(ID3D12RootSignature *computeRootSignature) {
#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (computeRootSignature == cachedComputeRootSignature) {
        return;
    }
    cachedComputeRootSignature = computeRootSignature;
#endif
    GetGraphicsCommandList()->SetComputeRootSignature(computeRootSignature);
    InvalidateRootSignatureDependentBindings();
    computeRootParametersDirtyMask = 0; // 루트 시그니쳐를 바꿀 때 마다 리셋
}

BE_INLINE void D3D12CommandList::SetPipelineState(const RHI::PipelineState *pipelineState) {
    const D3D12PipelineState *_pipelineState = static_cast<const D3D12PipelineState *>(pipelineState);

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (_pipelineState != currentPSO) {
        GetGraphicsCommandList()->SetPipelineState(_pipelineState->pso);

        if (pipelineState->isGraphics) {
            SetGraphicsRootSignature(_pipelineState->rootSignature);
            SetPrimitiveTopology(_pipelineState->primitiveTopology);
        } else {
            SetComputeRootSignature(_pipelineState->rootSignature);
        }
        currentPSO = _pipelineState;
    }
#else
    GetGraphicsCommandList()->SetPipelineState(_pipelineState->pso);

    if (pipelineState->isGraphics) {
        GetGraphicsCommandList()->SetGraphicsRootSignature(_pipelineState->rootSignature);
        InvalidateRootSignatureDependentBindings();
        GetGraphicsCommandList()->IASetPrimitiveTopology(_pipelineState->primitiveTopology);
        graphicsRootParametersDirtyMask = 0; // 루트 시그니쳐를 바꿀 때 마다 리셋
    } else {
        GetGraphicsCommandList()->SetComputeRootSignature(_pipelineState->rootSignature);
        InvalidateRootSignatureDependentBindings();
        computeRootParametersDirtyMask = 0; // 루트 시그니쳐를 바꿀 때 마다 리셋
    }
    currentPSO = _pipelineState;
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

BE_INLINE void D3D12CommandList::SetVertexBuffers(int startSlot, int count, const RHI::VertexBuffer *vertexBuffers[]) {
    D3D12_VERTEX_BUFFER_VIEW vbv[8];
    for (int i = 0; i < count; ++i) {
        vbv[i] = static_cast<const D3D12VertexBuffer *>(vertexBuffers[i])->vbv;
    }

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    bool needsUpdate = false;
    for (int i = 0; i < count; ++i) {
        int slot = startSlot + i;

        if (cachedVertexBufferViews[slot].BufferLocation != vbv[i].BufferLocation ||
            cachedVertexBufferViews[slot].SizeInBytes != vbv[i].SizeInBytes ||
            cachedVertexBufferViews[slot].StrideInBytes != vbv[i].StrideInBytes) {
            cachedVertexBufferViews[slot] = vbv[i];
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
    const D3D12_VERTEX_BUFFER_VIEW &vbv = static_cast<const D3D12VertexBuffer *>(vertexBuffer)->vbv;

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (cachedVertexBufferViews[slot].BufferLocation != vbv.BufferLocation ||
        cachedVertexBufferViews[slot].SizeInBytes != vbv.SizeInBytes ||
        cachedVertexBufferViews[slot].StrideInBytes != vbv.StrideInBytes) {
        cachedVertexBufferViews[slot] = vbv;
        GetGraphicsCommandList()->IASetVertexBuffers(slot, 1, &vbv);
    }
#else
    GetGraphicsCommandList()->IASetVertexBuffers(slot, 1, &vbv);
#endif
}

BE_INLINE void D3D12CommandList::SetIndexBuffer(const RHI::IndexBuffer *indexBuffer) {
    const D3D12_INDEX_BUFFER_VIEW &ibv = static_cast<const D3D12IndexBuffer *>(indexBuffer)->ibv;

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (cachedIndexBufferView.BufferLocation != ibv.BufferLocation ||
        cachedIndexBufferView.SizeInBytes != ibv.SizeInBytes ||
        cachedIndexBufferView.Format != ibv.Format) {
        cachedIndexBufferView = ibv;
        GetGraphicsCommandList()->IASetIndexBuffer(&ibv);
    }
#else
    GetGraphicsCommandList()->IASetIndexBuffer(&ibv);
#endif
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

BE_INLINE void D3D12CommandList::CloseAndExecute() {
    HRESULT hr = GetGraphicsCommandList()->Close();
    assert(SUCCEEDED(hr));

    Execute();
}
