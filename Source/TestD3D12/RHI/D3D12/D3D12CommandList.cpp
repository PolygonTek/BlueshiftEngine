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

#include "Precompiled.h"
#include "D3D12Renderer.h"
#include "D3D12CommandList.h"
#include "D3D12CommandListPool.h"
#include "D3D12ShaderVisibleDescriptorHeap.h"
#include "D3D12FrameThreadData.h"

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
    if (isSecondary) {
        return D3D12_COMMAND_LIST_TYPE_BUNDLE;
    }
    return parentPool->GetCommandListType();
}

D3D12RootBindingData *D3D12CommandList::GetRootBindingData() const {
    return parentPool->GetRootBindingData();
}

void D3D12CommandList::SetBindlessRootDescriptorTables(D3D12_GPU_DESCRIPTOR_HANDLE resourceDescriptorGpuBaseHandle, D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptorGpuBaseHandle) {
    assert(currentPSO);

    const D3D12PipelineState::RegisterBinder &binder = currentPSO->binder;
    const uint64_t bindlessResRootParameterMask = binder.bindlessResourceRootParameterMask;
    const uint64_t bindlessSamRootParameterMask = binder.bindlessSamplerRootParameterMask;

    // 루트 시그니쳐에서 fallback bindless 디스크립터 테이블을 사용하지 않는다면 세팅할 필요가 없다.
    if (bindlessResRootParameterMask == 0 && bindlessSamRootParameterMask == 0) {
        return;
    }

    // 루트 시그니쳐가 동일하다면, fallback bindless 디스크립터 테이블을 중복 세팅하지 않도록 한다.
    if (bindlessRootSignature == currentPSO->rootSignature &&
        bindlessResourceDescriptorGpuBaseHandle.ptr == resourceDescriptorGpuBaseHandle.ptr &&
        bindlessSamplerDescriptorGpuBaseHandle.ptr == samplerDescriptorGpuBaseHandle.ptr) {
        return;
    }

    ID3D12GraphicsCommandList6 *cmd = GetGraphicsCommandList();

    // 루트 시그니쳐에 존재하는 모든 fallback bindless (리소스 or 샘플러) 디스크립터 테이블들을 세팅한다.
    if (currentPSO->isGraphics) {
        for (int rootParameterIndex = 0; rootParameterIndex < 64; ++rootParameterIndex) {
            if (bindlessResRootParameterMask & BIT64(rootParameterIndex)) {
                cmd->SetGraphicsRootDescriptorTable(rootParameterIndex, resourceDescriptorGpuBaseHandle);
            }
            if (bindlessSamRootParameterMask & BIT64(rootParameterIndex)) {
                cmd->SetGraphicsRootDescriptorTable(rootParameterIndex, samplerDescriptorGpuBaseHandle);
            }
        }
    } else {
        for (int rootParameterIndex = 0; rootParameterIndex < 64; ++rootParameterIndex) {
            if (bindlessResRootParameterMask & BIT64(rootParameterIndex)) {
                cmd->SetComputeRootDescriptorTable(rootParameterIndex, resourceDescriptorGpuBaseHandle);
            }
            if (bindlessSamRootParameterMask & BIT64(rootParameterIndex)) {
                cmd->SetComputeRootDescriptorTable(rootParameterIndex, samplerDescriptorGpuBaseHandle);
            }
        }
    }

    bindlessRootSignature = currentPSO->rootSignature;
    bindlessResourceDescriptorGpuBaseHandle = resourceDescriptorGpuBaseHandle;
    bindlessSamplerDescriptorGpuBaseHandle = samplerDescriptorGpuBaseHandle;
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
    assert(!!primaryCommandList ^ !isSecondary);
    // BeginRenderPass/EndRenderPass 중간에 Reset 하면 assert 가 발생한다.
    assert(endRenderPassBarriers.Count() == 0);
    assert(!shadingRateImage);

    // CommandAllocator 를 재사용하도록 리셋.
    // NOTE: command list 는 GPU 작업 완료 여부와 상관없이 Reset 하여 재사용할 수 있지만,
    // CommandAllocator 는 GPU 에서 해당 command list 를 사용하는 작업이 모두 완료되기 전까지는 Reset 하면 안된다.
    commandAllocator->Reset();

    // command list 를 CommandAllocator 를 이용하여 초기 상태로 리셋
    GetGraphicsCommandList()->Reset(commandAllocator, nullptr);

    currentPSO = nullptr;
    InvalidateRootSignatureDependentBindings();

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (resetCacheStates) {
        if (!primaryCommandList) {
            // primary command list 라면, 모든 캐시된 상태들을 초기값으로 변경한다.
            cachedGraphicsRootSignature = nullptr;
            cachedComputeRootSignature = nullptr;

            for (int i = 0; i < D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++i) {
                cachedVertexBufferViews[i] = {};
            }
            cachedIndexBufferView = {};
            cachedBlendFactor = BE1::Color4(1, 1, 1, 1);
            cachedStencilRef = 0;
            cachedShadingRate = D3D12_SHADING_RATE::D3D12_SHADING_RATE_1X1;
        } else {
            // secondary command list 는 primary command list 의 상태를 그대로 사용하므로,
            // 모든 캐시된 상태들을 primary command list 의 것과 같도록 초기화한다.
            const D3D12CommandList *_primaryCommandList = static_cast<const D3D12CommandList *>(primaryCommandList);
            cachedGraphicsRootSignature = _primaryCommandList->cachedGraphicsRootSignature;
            cachedComputeRootSignature = _primaryCommandList->cachedComputeRootSignature;

            for (int i = 0; i < D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++i) {
                cachedVertexBufferViews[i] = _primaryCommandList->cachedVertexBufferViews[i];
            }
            cachedIndexBufferView = _primaryCommandList->cachedIndexBufferView;
            cachedBlendFactor = _primaryCommandList->cachedBlendFactor;
            cachedStencilRef = _primaryCommandList->cachedStencilRef;
            cachedShadingRate = _primaryCommandList->cachedShadingRate;
        }

        // primary 든 secondary 는 상관없이 SetDescriptorHeaps() 와 IASetPrimitiveTopology() 는 따로 세팅해야 하므로,
        // 그와 관련된 캐시 상태는 항상 초기값으로 세팅한다.
        cachedRootDescriptorHeaps.SetCount(0);
        cachedPrimitiveTopology = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }
#endif
}

void D3D12CommandList::Execute() {
    int queueIndex = to_int(parentPool->commandListType == D3D12_COMMAND_LIST_TYPE_COMPUTE ? RHI::CommandQueueType::Compute : RHI::CommandQueueType::Graphics);

    // command queue 에 커맨드 리스트를 즉시 밀어넣고, GPU 에서 비동기로 실행하도록 한다.
    ID3D12CommandList *execCommandLists[] = { commandList };
    D3D12Renderer::GetRenderer()->commandQueues[queueIndex]->ExecuteCommandLists(COUNT_OF(execCommandLists), execCommandLists);
}

// 실제로 Execute 하는 건 아니고, primary command list 에 secondary command list 를 끼워 넣는다.
void D3D12CommandList::ExecuteSecondary(RHI::CommandList *secondaryCommandList) {
    D3D12CommandList *_secondaryCommandList = static_cast<D3D12CommandList *>(secondaryCommandList);
    assert(_secondaryCommandList->isSecondary);

    ID3D12DescriptorHeap *descriptorHeaps[] = {
        _secondaryCommandList->GetResDescriptorTablePool()->GetDescriptorHeap(),
        _secondaryCommandList->GetSamDescriptorTablePool()->GetDescriptorHeap()
    };

    // ExecuteBundle 을 호출하기 전에 primary 의 디스크립터 힙을 secondary 의 것과 동일하게 맞춘다.
    SetDescriptorHeaps(COUNT_OF(descriptorHeaps), descriptorHeaps);

    // primary command list 에 secondary command list 를 끼워 넣는다.
    GetGraphicsCommandList()->ExecuteBundle(_secondaryCommandList->GetGraphicsCommandList());
}
