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
#include "D3D12FrameData.h"

RHI::FrameData *D3D12Renderer::CreateFrameData(int32_t partitionIndex, int32_t numPartitions, int32_t frameIndex, int32_t numFrames) {
    assert(BE1::Engine::IsInMainThread());

    D3D12FrameData *frameData = new D3D12FrameData;
    frameData->Init(partitionIndex, numPartitions, frameIndex, numFrames);
    return frameData;
}

void D3D12Renderer::DestroyFrameData(RHI::FrameData *frameData) {
    assert(BE1::Engine::IsInMainThread());

    D3D12FrameData *_frameData = static_cast<D3D12FrameData *>(frameData);
    delete _frameData;
}

//---------------------------------------------------------------------------------------------------------------------------------

void D3D12FrameData::Init(int32_t partitionIndex, int32_t numPartitions, int32_t frameIndex, int32_t numFrames) {
    assert(numPartitions > 0 && partitionIndex >= 0 && partitionIndex < numPartitions);
    assert(numFrames > 0 && frameIndex >= 0 && frameIndex < numFrames);

    D3D12Renderer *renderer = D3D12Renderer::GetRenderer();

    // 파티션 별 transient 리소스/샘플러 디스크립터 개수.
    const uint32_t numResourceDescriptorsPerPartition = D3D12Renderer::MaxTransientResourceDescriptors / numPartitions;
    const uint32_t numSamplerDescriptorsPerPartition = D3D12Renderer::MaxTransientSamplerDescriptors / numPartitions;

    // 파티션 당 프레임 별 transient 리소스/샘플러 디스크립터 개수.
    const uint32_t numResourceDescriptorsPerFrame = numResourceDescriptorsPerPartition / numFrames;
    const uint32_t numSamplerDescriptorsPerFrame = numSamplerDescriptorsPerPartition / numFrames;

    // shader-visible 디스크립터 힙에서 persistent 영역 뒤의 공간이 transient 영역이다.
    const UINT baseResourceDescriptorIndex = D3D12Renderer::MaxPersistentResourceDescriptors + partitionIndex * numResourceDescriptorsPerPartition + frameIndex * numResourceDescriptorsPerFrame;
    const UINT baseSamplerDescriptorIndex = D3D12Renderer::MaxPersistentSamplerDescriptors + partitionIndex * numSamplerDescriptorsPerPartition + frameIndex * numSamplerDescriptorsPerFrame;

    descriptorAllocators[Resource].Init({ &renderer->resourceShaderVisibleDescriptorHeap, baseResourceDescriptorIndex, numResourceDescriptorsPerFrame });
    descriptorAllocators[Sampler].Init({ &renderer->samplerShaderVisibleDescriptorHeap, baseSamplerDescriptorIndex, numSamplerDescriptorsPerFrame });
}

void D3D12FrameData::Reset() {
    for (auto &allocator : descriptorAllocators) {
        allocator.Reset();
    }
}
