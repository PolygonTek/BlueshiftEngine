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
    DestroyBuffer(volumeBuffer);
    DestroyBuffer(volumeBoundsBuffer);
    boundsUploaded = false;
}

bool ShaderLocalFogVolumeTable::EnsureBufferCapacity(TableBuffer &buffer, uint32_t count, uint32_t elementSize, RHI::Format format, uint32_t stride, const char *name) {
    if (count <= buffer.capacity) {
        return true;
    }

    const uint32_t maxCapacity = UINT32_MAX / elementSize;
    if (count > maxCapacity) {
        return false;
    }
    const uint32_t newCapacity = BE1::Max(count, BE1::Min(BE1::Max(buffer.capacity * 2u, 16u), maxCapacity));
    const uint32_t bufferSize = newCapacity * elementSize;

    DestroyBuffer(buffer);

    buffer.buffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::ShaderResource, bufferSize, nullptr);
    if (!buffer.buffer) {
        return false;
    }
    RHI::renderer->SetName(buffer.buffer, name);

    const RHI::ViewID view = RHI::renderer->CreateView(buffer.buffer, RHI::ViewType::SRV, format, stride, 0, bufferSize);
    if (!view.IsValid()) {
        DestroyBuffer(buffer);
        return false;
    }

    buffer.capacity = newCapacity;
    buffer.bindlessIndex = buffer.buffer->GetBindlessIndex(RHI::ViewType::SRV, view);
    return true;
}

void ShaderLocalFogVolumeTable::UploadBuffer(RHI::CommandList *commandList, TableBuffer &buffer, RHI::Buffer *uploadBuffer, uint32_t uploadSize) {
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(buffer.buffer, buffer.state, RHI::GPUResourceState::CopyDst));
    RHI::renderer->CopyBuffer(commandList, buffer.buffer, 0, uploadBuffer, 0, uploadSize);
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(buffer.buffer, RHI::GPUResourceState::CopyDst, RHI::GPUResourceState::ShaderResource));
    buffer.state = RHI::GPUResourceState::ShaderResource;
}

void ShaderLocalFogVolumeTable::DestroyBuffer(TableBuffer &buffer) {
    if (buffer.buffer) {
        RHI::renderer->DestroyBuffer(buffer.buffer);
        buffer.buffer = nullptr;
    }

    buffer.state = RHI::GPUResourceState::Undefined;
    buffer.capacity = 0;
    buffer.bindlessIndex = 0;
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

    // 비활성/빈 프레임이나 업로드 실패 시 이전 프레임의 범위를 참조하지 않는다.
    boundsUploaded = false;

    const uint32_t count = frame->NumFrameLocalFogVolumes();
    if (count == 0) {
        // 빈 프레임은 카메라별 numLocalFogVolumes 가 0 이므로 테이블을 참조하지 않는다.
        return true;
    }

    RHI::Buffer *volumeUploadBuffer = frameThreadData->AllocStructuredBuffer(sizeof(ShaderLocalFogVolume), count, nullptr);
    if (!volumeUploadBuffer || !volumeUploadBuffer->mappedPtr ||
        !EnsureBufferCapacity(volumeBuffer, count, sizeof(ShaderLocalFogVolume), RHI::Format::Unknown, sizeof(ShaderLocalFogVolume), "shaderLocalFogVolumeTableBuffer")) {
        return false;
    }

    bool needsBounds = false;
    if (frame->GetRenderFrameSettings().vfogLocalVolumeFroxelCulling) {
        const FrameCamera *cameras = frame->GetFrameCameras();
        for (uint32_t i = 0; i < frame->NumFrameCameras(); ++i) {
            if (cameras[i].numFrameLocalFogVolumes > 0 && cameras[i].fog.volumetric.enabled && (cameras[i].fog.volumetric.vfogLightingParams.flags & VFogFlagLocalVolumeFroxelCulling) != 0) {
                needsBounds = true;
                break;
            }
        }
    }

    RHI::Buffer *volumeBoundsUploadBuffer = nullptr;
    ShaderLocalFogVolumeBounds *shaderBounds = nullptr;
    if (needsBounds) {
        volumeBoundsUploadBuffer = frameThreadData->AllocStructuredBuffer(sizeof(ShaderLocalFogVolumeBounds), count, nullptr);
        if (volumeBoundsUploadBuffer && volumeBoundsUploadBuffer->mappedPtr && EnsureBufferCapacity(volumeBoundsBuffer, count, sizeof(ShaderLocalFogVolumeBounds), RHI::Format::R32_UINT, 0, "shaderLocalFogVolumeBoundsBuffer")) {
            shaderBounds = reinterpret_cast<ShaderLocalFogVolumeBounds *>(volumeBoundsUploadBuffer->mappedPtr);
        } else {
            // 컬링 데이터만 실패하면 매질 업로드는 계속하고 기존 전체 순회로 폴백한다.
            BE_WARNLOG("ShaderLocalFogVolumeTable::Upload: failed to upload local fog bounds; culling disabled\n");
        }
    }

    ShaderLocalFogVolume *shaderVolumes = reinterpret_cast<ShaderLocalFogVolume *>(volumeUploadBuffer->mappedPtr);
    uint32_t index = 0;
    for (const FrameLocalFogVolume &frameVolume : frame->GetFrameLocalFogVolumes()) {
        Build(frameVolume, shaderVolumes[index]);

        if (shaderBounds) {
            shaderBounds[index] = frameVolume.froxelBounds;
        }
        ++index;
    }
    assert(index == count);

    UploadBuffer(commandList, volumeBuffer, volumeUploadBuffer, count * static_cast<uint32_t>(sizeof(ShaderLocalFogVolume)));

    if (shaderBounds) {
        UploadBuffer(commandList, volumeBoundsBuffer, volumeBoundsUploadBuffer, count * static_cast<uint32_t>(sizeof(ShaderLocalFogVolumeBounds)));
        boundsUploaded = true;
    }

    return true;
}
