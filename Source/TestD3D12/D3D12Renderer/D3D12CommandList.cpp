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
#include "D3D12CommandList.h"
#include "D3D12CommandListPool.h"
#include "D3D12RootDescriptorPool.h"
#include "D3D12FrameData.h"

static constexpr D3D12_SHADING_RATE ToD3D12ShadingRate(RHI::ShadingRate shadingRate) {
    switch (shadingRate) {
    case RHI::ShadingRate::Rate1X1:
        return D3D12_SHADING_RATE_1X1;
    case RHI::ShadingRate::Rate1X2:
        return D3D12_SHADING_RATE_1X2;
    case RHI::ShadingRate::Rate2X1:
        return D3D12_SHADING_RATE_2X1;
    case RHI::ShadingRate::Rate2X2:
        return D3D12_SHADING_RATE_2X2;
    case RHI::ShadingRate::Rate2X4:
        return D3D12_SHADING_RATE_2X4;
    case RHI::ShadingRate::Rate4X2:
        return D3D12_SHADING_RATE_4X2;
    case RHI::ShadingRate::Rate4X4:
        return D3D12_SHADING_RATE_4X4;
    }
    return D3D12_SHADING_RATE_1X1;
}

D3D12_COMMAND_LIST_TYPE D3D12CommandList::GetCommandListType() const {
    if (secondary) {
        return D3D12_COMMAND_LIST_TYPE_BUNDLE;
    }
    return parentPool->GetCommandListType();
}

RHI::FrameThreadData *D3D12CommandList::GetFrameThreadData() const {
    return parentPool->GetFrameThreadData();
}

void D3D12CommandList::SetShadingRate(RHI::ShadingRate shadingRate) {
    D3D12_SHADING_RATE rate = ToD3D12ShadingRate(shadingRate);
#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (cachedShadingRate == rate) {
        return;
    }
    cachedShadingRate = rate;
#endif
    D3D12_SHADING_RATE_COMBINER combiners[] = {
        D3D12_SHADING_RATE_COMBINER_MAX,
        D3D12_SHADING_RATE_COMBINER_MAX
    };
    GetGraphicsCommandList()->RSSetShadingRate(rate, combiners);
}

void D3D12CommandList::Reset(bool resetCacheStates, const RHI::CommandList *primaryCommandList) {
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

void D3D12CommandList::Execute(RHI::CommandQueueType queueType) {
    assert(queueType < RHI::CommandQueueType::Count);
    ID3D12CommandList *execCommandLists[] = { commandList };

    D3D12Renderer::GetRenderer()->commandQueues[to_int(queueType)]->ExecuteCommandLists(COUNT_OF(execCommandLists), execCommandLists);
}

void D3D12CommandList::ExecuteSecondary(RHI::CommandList *primaryCommandList, const RHI::FrameThreadData *frameThreadData) {
    // ExecuteBundle 을 실행하기 전에 Primary CommandList 의 루트 디스크립터 힙을 지정한다.
    ID3D12DescriptorHeap *descriptorHeaps[] = { static_cast<const D3D12FrameThreadData *>(frameThreadData)->rootDescriptorPool->GetDescriptorHeap() };
    static_cast<D3D12CommandList *>(primaryCommandList)->SetDescriptorHeaps(COUNT_OF(descriptorHeaps), descriptorHeaps);

    D3D12CommandList *d3d12PrimaryCommandList = static_cast<D3D12CommandList *>(primaryCommandList);
    d3d12PrimaryCommandList->GetGraphicsCommandList()->ExecuteBundle(GetGraphicsCommandList());
}

void D3D12CommandList::CloseAndExecute(RHI::CommandQueueType queueType) {
    HRESULT hr = GetGraphicsCommandList()->Close();
    assert(SUCCEEDED(hr));

    assert(queueType < RHI::CommandQueueType::Count);
    ID3D12CommandList *execCommandLists[] = { commandList };
    D3D12Renderer::GetRenderer()->commandQueues[to_int(queueType)]->ExecuteCommandLists(COUNT_OF(execCommandLists), execCommandLists);
}

void D3D12CommandList::CloseAndExecuteSecondary(RHI::CommandList *primaryCommandList, const RHI::FrameThreadData *frameThreadData) {
    HRESULT hr = GetGraphicsCommandList()->Close();
    assert(SUCCEEDED(hr));

    // ExecuteBundle 을 실행하기 전에 Primary CommandList 의 루트 디스크립터 힙을 지정한다.
    ID3D12DescriptorHeap *descriptorHeaps[] = { static_cast<const D3D12FrameThreadData *>(frameThreadData)->rootDescriptorPool->GetDescriptorHeap() };
    static_cast<D3D12CommandList *>(primaryCommandList)->SetDescriptorHeaps(COUNT_OF(descriptorHeaps), descriptorHeaps);

    D3D12CommandList *d3d12PrimaryCommandList = static_cast<D3D12CommandList *>(primaryCommandList);
    d3d12PrimaryCommandList->GetGraphicsCommandList()->ExecuteBundle(GetGraphicsCommandList());
}

