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

struct D3D12CommandList {
    void                            Reset();

    bool                            IsSameDescriptorHeaps(int numDescriptorHeaps, ID3D12DescriptorHeap *descriptorHeaps[]);
    void                            SetDescriptorHeaps(int numDescriptorHeaps, ID3D12DescriptorHeap* descriptorHeaps[]);
    void                            SetGraphicsRootSignature(ID3D12RootSignature* graphicsRootSignature);
    void                            SetPipelineState(ID3D12PipelineState* piplelineState);
    void                            SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology);

    ID3D12CommandAllocator *        commandAllocator = nullptr;
    ID3D12GraphicsCommandList *     commandList = nullptr;
    StaticArray<ID3D12DescriptorHeap *, 16> currentRootDescriptorHeaps;
    ID3D12RootSignature *           currentGraphicsRootSignature = nullptr;
    ID3D12PipelineState *           currentPipelineState = nullptr;
    D3D12_PRIMITIVE_TOPOLOGY        currentPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    LinkList<D3D12CommandList>      node;
};

BE_INLINE void D3D12CommandList::Reset() {
    // 커맨드 할당자를 재사용하도록 리셋
    commandAllocator->Reset();
    // 커맨드 리스트를 커맨드 할당자를 이용하여 초기 상태로 리셋
    commandList->Reset(commandAllocator, nullptr);

    // 각종 상태를 초기값으로 변경
    currentRootDescriptorHeaps.SetCount(0);
    currentGraphicsRootSignature = nullptr;
    currentPipelineState = nullptr;
    currentPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

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

BE_INLINE void D3D12CommandList::SetDescriptorHeaps(int numDescriptorHeaps, ID3D12DescriptorHeap *descriptorHeaps[]) {
    if (IsSameDescriptorHeaps(numDescriptorHeaps, descriptorHeaps)) {
        return;
    }
    currentRootDescriptorHeaps.SetCount(numDescriptorHeaps);
    for (int i = 0; i < numDescriptorHeaps; ++i) {
        currentRootDescriptorHeaps[i] = descriptorHeaps[i];
    }
    commandList->SetDescriptorHeaps(numDescriptorHeaps, descriptorHeaps);
}

BE_INLINE void D3D12CommandList::SetGraphicsRootSignature(ID3D12RootSignature *graphicsRootSignature) {
    if (graphicsRootSignature == currentGraphicsRootSignature) {
        return;
    }
    currentGraphicsRootSignature = graphicsRootSignature;
    commandList->SetGraphicsRootSignature(graphicsRootSignature);
}

BE_INLINE void D3D12CommandList::SetPipelineState(ID3D12PipelineState *piplelineState) {
    if (piplelineState == currentPipelineState) {
        return;
    }
    currentPipelineState = piplelineState;
    commandList->SetPipelineState(piplelineState);
}

BE_INLINE void D3D12CommandList::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology) {
    if (primitiveTopology == currentPrimitiveTopology) {
        return;
    }
    currentPrimitiveTopology = primitiveTopology;
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
