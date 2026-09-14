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
    const uint64_t bindlessResourceRootParameterMask = binder.bindlessResourceRootParameterMask;
    const uint64_t bindlessSamplerRootParameterMask = binder.bindlessSamplerRootParameterMask;

    // 루트 시그니처에서 fallback bindless 디스크립터 테이블을 사용하지 않는다면 세팅할 필요가 없다.
    if (bindlessResourceRootParameterMask == 0 && bindlessSamplerRootParameterMask == 0) {
        return;
    }

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    // 루트 시그니처와 GPU 베이스 핸들이 동일하다면, fallback bindless 디스크립터 테이블을 중복 세팅하지 않도록 한다.
    if (cache.bindlessRootSignature == currentPSO->rootSignature &&
        cache.bindlessResourceDescriptorGpuBaseHandle.ptr == resourceDescriptorGpuBaseHandle.ptr &&
        cache.bindlessSamplerDescriptorGpuBaseHandle.ptr == samplerDescriptorGpuBaseHandle.ptr) {
        return;
    }
#endif

    ID3D12GraphicsCommandList6 *cmd = GetGraphicsCommandList();

    // 루트 시그니처에 존재하는 모든 fallback bindless 디스크립터 테이블들을 세팅한다.
    if (currentPSO->isGraphics) {
        for (int rootParameterIndex = 0; rootParameterIndex < 64; ++rootParameterIndex) {
            if (bindlessResourceRootParameterMask & BIT64(rootParameterIndex)) {
                cmd->SetGraphicsRootDescriptorTable(rootParameterIndex, resourceDescriptorGpuBaseHandle);
            }
            if (bindlessSamplerRootParameterMask & BIT64(rootParameterIndex)) {
                cmd->SetGraphicsRootDescriptorTable(rootParameterIndex, samplerDescriptorGpuBaseHandle);
            }
        }
    } else {
        for (int rootParameterIndex = 0; rootParameterIndex < 64; ++rootParameterIndex) {
            if (bindlessResourceRootParameterMask & BIT64(rootParameterIndex)) {
                cmd->SetComputeRootDescriptorTable(rootParameterIndex, resourceDescriptorGpuBaseHandle);
            }
            if (bindlessSamplerRootParameterMask & BIT64(rootParameterIndex)) {
                cmd->SetComputeRootDescriptorTable(rootParameterIndex, samplerDescriptorGpuBaseHandle);
            }
        }
    }

#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    cache.bindlessRootSignature = currentPSO->rootSignature;
    cache.bindlessResourceDescriptorGpuBaseHandle = resourceDescriptorGpuBaseHandle;
    cache.bindlessSamplerDescriptorGpuBaseHandle = samplerDescriptorGpuBaseHandle;
#endif
}

void D3D12CommandList::SetShadingRate(RHI::ShadingRate shadingRate) {
    D3D12_SHADING_RATE rate = ToD3D12ShadingRate(shadingRate);
#ifdef USE_STATE_CACHE_FOR_COMMAND_LIST
    if (cache.shadingRateValid && cache.shadingRate == rate) {
        return;
    }
    cache.shadingRate = rate;
    cache.shadingRateValid = true;
#endif
    D3D12_SHADING_RATE_COMBINER combiners[] = {
        D3D12_SHADING_RATE_COMBINER_MAX,
        D3D12_SHADING_RATE_COMBINER_MAX
    };
    GetGraphicsCommandList()->RSSetShadingRate(rate, combiners);
}

void D3D12CommandList::Reset() {
    // BeginRenderPass/EndRenderPass 중간에 Reset 하면 assert 가 발생한다.
    assert(endRenderPassBarriers.Count() == 0);
    assert(!shadingRateImage);

    // CommandAllocator 를 재사용하도록 리셋.
    // NOTE: command list 는 GPU 작업 완료 여부와 상관없이 Reset 하여 재사용할 수 있지만,
    // CommandAllocator 는 GPU 에서 해당 command list 를 사용하는 작업이 모두 완료되기 전까지는 Reset 하면 안된다.
    commandAllocator->Reset();

    // command list 를 CommandAllocator 를 이용하여 초기 상태로 리셋
    GetGraphicsCommandList()->Reset(commandAllocator, nullptr);

    InvalidateStateCache();
}

void D3D12CommandList::Execute() {
    int queueIndex = to_int(parentPool->commandListType == D3D12_COMMAND_LIST_TYPE_COMPUTE ? RHI::CommandQueueType::Compute : RHI::CommandQueueType::Graphics);

    // command queue 에 커맨드 리스트를 즉시 밀어넣고, GPU 에서 비동기로 실행하도록 한다.
    ID3D12CommandList *execCommandLists[] = { commandList };
    D3D12Renderer::GetRenderer()->commandQueues[queueIndex]->ExecuteCommandLists(COUNT_OF(execCommandLists), execCommandLists);
}

// 부모 커맨드 리스트에 번들의 실행을 기록한다.
void D3D12CommandList::ExecuteSecondary(RHI::CommandList *secondaryCommandList) {
    D3D12CommandList *_secondaryCommandList = static_cast<D3D12CommandList *>(secondaryCommandList);
    assert(!isSecondary && _secondaryCommandList->isSecondary);

    ID3D12DescriptorHeap *secondaryDescriptorHeaps[] = {
        _secondaryCommandList->GetResourceDescriptorRangePool()->GetDescriptorHeap(),
        _secondaryCommandList->GetSamplerDescriptorRangePool()->GetDescriptorHeap()
    };

    // 번들이 사용할 디스크립터 힙을 부모에 바인딩하여, ExecuteBundle 실행 시 해당 힙을 상속받도록 한다.
    SetDescriptorHeaps(COUNT_OF(secondaryDescriptorHeaps), secondaryDescriptorHeaps);

    // 부모 커맨드 리스트에 번들을 기록한다.
    // ExecuteBundle 시점에서 번들의 시작 상태는 PSO 와 PrimitiveTopology 를 제외하고 부모로부터 상속 받는다.
    GetGraphicsCommandList()->ExecuteBundle(_secondaryCommandList->GetGraphicsCommandList());

    // ExecuteBundle 이 실행되면 부모 커맨드 리스트의 상태가 번들로 인해 변경될 수 있으므로, 번들 실행 후 상태 캐시를 무효화한다.
    // 디스크립터 힙은 실행 직전에 맞췄으므로 캐시를 유지한다.
    InvalidateStateCache(false);
}
