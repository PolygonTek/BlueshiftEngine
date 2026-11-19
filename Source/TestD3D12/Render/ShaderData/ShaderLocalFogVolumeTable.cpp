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
#include "ShaderLocalFogVolumeTable.h"
#include "Render/RenderFrame.h"
#include "Render/RenderInternal.h"

void ShaderLocalFogVolumeTable::Shutdown() {
    DestroyBuffer();
}

bool ShaderLocalFogVolumeTable::EnsureBufferCapacity(uint32_t count) {
    if (count <= tableBuffer.capacity) {
        return true;
    }

    const uint32_t newCapacity = BE1::Max(count, BE1::Max(tableBuffer.capacity * 2u, 16u));
    const uint64_t bufferSize = static_cast<uint64_t>(newCapacity) * sizeof(ShaderLocalFogVolume);

    DestroyBuffer();

    tableBuffer.buffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::ShaderResource, bufferSize, nullptr);
    if (!tableBuffer.buffer) {
        return false;
    }
    RHI::renderer->SetName(tableBuffer.buffer, "shaderLocalFogVolumeTableBuffer");

    const RHI::ViewID view = RHI::renderer->CreateView(tableBuffer.buffer, RHI::ViewType::SRV, RHI::Format::Unknown, sizeof(ShaderLocalFogVolume), 0, bufferSize);
    if (!view.IsValid()) {
        DestroyBuffer();
        return false;
    }

    tableBuffer.capacity = newCapacity;
    tableBuffer.bindlessIndex = tableBuffer.buffer->GetBindlessIndex(RHI::ViewType::SRV, view);
    return true;
}

void ShaderLocalFogVolumeTable::DestroyBuffer() {
    if (tableBuffer.buffer) {
        RHI::renderer->DestroyBuffer(tableBuffer.buffer);
        tableBuffer.buffer = nullptr;
    }

    tableBuffer.state = RHI::GPUResourceState::Undefined;
    tableBuffer.capacity = 0;
    tableBuffer.bindlessIndex = 0;
}

void ShaderLocalFogVolumeTable::Build(const FrameLocalFogVolume &frameVolume, ShaderLocalFogVolume &shaderVolume) {
    const float fadeExtent = frameVolume.shape == LocalFogVolume::Shape::Box ? BE1::Min3(frameVolume.halfExtents.x, frameVolume.halfExtents.y, frameVolume.halfExtents.z) : frameVolume.radius;
    const float boundaryFadeDistance = frameVolume.boundaryFade * fadeExtent;

    shaderVolume.medium = frameVolume.medium;
    shaderVolume.origin = float3(frameVolume.origin);
    shaderVolume.shape = static_cast<uint32_t>(frameVolume.shape);
    shaderVolume.maskAddressMode = static_cast<uint32_t>(frameVolume.maskAddressMode);
    shaderVolume.boundaryFadeDistanceRcp = frameVolume.boundaryFade > 0.0f ? 1.0f / BE1::Max(boundaryFadeDistance, FLT_MIN) : 0.0f;

    shaderVolume.heightFogFalloff = frameVolume.heightFogFalloff;
    shaderVolume.heightFogOffset = frameVolume.heightFogOffset;

    shaderVolume.axisX = float3(frameVolume.axis[0]);
    shaderVolume.axisY = float3(frameVolume.axis[1]);
    shaderVolume.axisZ = float3(frameVolume.axis[2]);

    if (frameVolume.shape == LocalFogVolume::Shape::Box) {
        shaderVolume.halfExtents = float3(frameVolume.halfExtents);
    } else {
        shaderVolume.halfExtents = float3(frameVolume.radius, frameVolume.radius, frameVolume.radius);
    }

    const RHI::Texture *maskTexture = frameVolume.maskTexture;
    if (maskTexture) {
        const BE1::Vec3 &uvwScale = frameVolume.maskUVWScale;
        shaderVolume.maskUVWScale = float3(uvwScale);
        shaderVolume.maskTextureIndex = maskTexture->GetBindlessIndex(RHI::ViewType::SRV);
        shaderVolume.maskUVWBias = float3(frameVolume.maskUVWBias);
        shaderVolume.maskTexelsPerMeter = BE1::Max3(
            BE1::Math::Abs(uvwScale.x) * static_cast<float>(maskTexture->GetWidth()),
            BE1::Math::Abs(uvwScale.y) * static_cast<float>(maskTexture->GetHeight()),
            BE1::Math::Abs(uvwScale.z) * static_cast<float>(maskTexture->GetDepth()));
    } else {
        shaderVolume.maskUVWScale = float3(0, 0, 0);
        shaderVolume.maskTextureIndex = 0;
        shaderVolume.maskUVWBias = float3(0, 0, 0);
        shaderVolume.maskTexelsPerMeter = 0.0f;
    }
}

bool ShaderLocalFogVolumeTable::Upload(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const RenderFrame *frame) {
    PROFILER_CPU_SCOPED_EVENT("ShaderLocalFogVolumeTable::Upload");

    const uint32_t count = frame->NumFrameLocalFogVolumes();
    if (count == 0) {
        // 빈 프레임은 카메라별 numLocalFogVolumes 가 0 이므로 테이블을 참조하지 않는다.
        return true;
    }

    RHI::Buffer *uploadBuffer = frameThreadData->AllocStructuredBuffer(sizeof(ShaderLocalFogVolume), count, nullptr);
    if (!uploadBuffer || !uploadBuffer->mappedPtr || !EnsureBufferCapacity(count)) {
        return false;
    }

    ShaderLocalFogVolume *shaderVolumes = reinterpret_cast<ShaderLocalFogVolume *>(uploadBuffer->mappedPtr);
    uint32_t index = 0;
    for (const FrameLocalFogVolume &frameVolume : frame->GetFrameLocalFogVolumes()) {
        Build(frameVolume, shaderVolumes[index++]);
    }
    assert(index == count);

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(tableBuffer.buffer, tableBuffer.state, RHI::GPUResourceState::CopyDst));
    RHI::renderer->CopyBuffer(commandList, tableBuffer.buffer, 0, uploadBuffer, 0, count * static_cast<uint32_t>(sizeof(ShaderLocalFogVolume)));
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(tableBuffer.buffer, RHI::GPUResourceState::CopyDst, RHI::GPUResourceState::ShaderResource));
    tableBuffer.state = RHI::GPUResourceState::ShaderResource;

    return true;
}
