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
#include "ShaderSurfaceTable.h"
#include "Render/RenderFrame.h"
#include "Render/RenderInternal.h"

bool ShaderSurfaceTable::Init(uint32_t initialCapacity) {
    return EnsureBufferCapacity(initialCapacity);
}

void ShaderSurfaceTable::Shutdown() {
    DestroyBuffer();
}

void ShaderSurfaceTable::Clear() {
}

bool ShaderSurfaceTable::EnsureBufferCapacity(uint32_t count) {
    constexpr uint32_t maxCapacity = UINT32_MAX / sizeof(ShaderSurface);
    if (count > maxCapacity) {
        return false;
    }

    if (count <= tableBuffer.capacity) {
        return true;
    }

    uint32_t newCapacity = tableBuffer.capacity > 0 ? tableBuffer.capacity : 1024;
    while (newCapacity < count) {
        newCapacity = BE1::Min(newCapacity << 1, maxCapacity);
    }

    DestroyBuffer();

    const uint32_t bufferSize = newCapacity * sizeof(ShaderSurface);

    // GPU 디폴트 버퍼를 생성한다.
    tableBuffer.buffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::ShaderResource, bufferSize, nullptr);
    if (!tableBuffer.buffer) {
        DestroyBuffer();
        return false;
    }

    RHI::renderer->SetName(tableBuffer.buffer, "shaderSurfaceTableBuffer");

    tableBuffer.view = RHI::renderer->CreateView(tableBuffer.buffer, RHI::ViewType::SRV, RHI::Format::Unknown, sizeof(ShaderSurface), 0, bufferSize);
    if (!tableBuffer.view.IsValid()) {
        DestroyBuffer();
        return false;
    }

    tableBuffer.capacity = newCapacity;
    tableBuffer.state = RHI::GPUResourceState::Undefined;
    tableBuffer.bindlessIndex = tableBuffer.buffer->GetBindlessIndex(RHI::ViewType::SRV, tableBuffer.view);
    return true;
}

bool ShaderSurfaceTable::Upload(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const RenderFrame *frame) {
    PROFILER_CPU_SCOPED_EVENT("ShaderSurfaceTable::Upload");

    const uint32_t surfaceCount = frame->NumFrameSurfaces();
    if (surfaceCount == 0) {
        Clear();
        return true;
    }

    if (!EnsureBufferCapacity(surfaceCount)) {
        return false;
    }

    RHI::Buffer *uploadBuffer = frameThreadData->AllocStructuredBuffer(sizeof(ShaderSurface), surfaceCount, nullptr);
    if (!uploadBuffer) {
        return false;
    }

    ShaderSurface *shaderSurfaces = reinterpret_cast<ShaderSurface *>(uploadBuffer->mappedPtr);
    const FrameCamera *frameCameras = frame->GetFrameCameras();
    for (uint32_t cameraIndex = 0; cameraIndex < frame->NumFrameCameras(); ++cameraIndex) {
        const FrameCamera &frameCamera = frameCameras[cameraIndex];
        if (frameCamera.firstFrameSurface == UINT32_MAX) {
            continue;
        }

        if (frameCamera.firstFrameSurface > surfaceCount || frameCamera.numFrameSurfs > surfaceCount - frameCamera.firstFrameSurface) {
            assert(0);
            return false;
        }

        // 프런트엔드에서 확정한 연속 구간에 기록한다. Instance 매핑도 같은 시작 인덱스를 사용한다.
        ShaderSurface *cameraSurfaces = shaderSurfaces + frameCamera.firstFrameSurface;

        for (uint32_t surfaceIndex = 0; surfaceIndex < frameCamera.numFrameSurfs; ++surfaceIndex) {
            const FrameSurf *frameSurf = frameCamera.frameSurfs[surfaceIndex];
            assert(frameSurf);
            assert(frameSurf->index == surfaceIndex);
            Build(frameSurf, cameraSurfaces[surfaceIndex]);
        }
    }

    const uint32_t uploadSize = surfaceCount * sizeof(ShaderSurface);

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(tableBuffer.buffer, tableBuffer.state, RHI::GPUResourceState::CopyDst));
    RHI::renderer->CopyBuffer(commandList, tableBuffer.buffer, 0, uploadBuffer, 0, uploadSize);
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(tableBuffer.buffer, RHI::GPUResourceState::CopyDst, RHI::GPUResourceState::ShaderResource));

    tableBuffer.state = RHI::GPUResourceState::ShaderResource;
    return true;
}

void ShaderSurfaceTable::DestroyBuffer() {
    if (tableBuffer.buffer) {
        RHI::renderer->DestroyBuffer(tableBuffer.buffer);
        tableBuffer.buffer = nullptr;
    }

    tableBuffer.view = RHI::ViewID::InvalidID;
    tableBuffer.state = RHI::GPUResourceState::Undefined;
    tableBuffer.capacity = 0;
    tableBuffer.bindlessIndex = 0;
}

void ShaderSurfaceTable::Build(const FrameSurf *frameSurf, ShaderSurface &outShaderSurface) const {
    assert(frameSurf);
    assert(frameSurf->space);

    const BE1::Mat3x4 &worldMatrix = frameSurf->space->worldMatrix;
    BE1::simdProcessor->Memcpy(&outShaderSurface.worldMatrix, &worldMatrix, sizeof(outShaderSurface.worldMatrix));
    BE1::simdProcessor->Memcpy(&outShaderSurface.prevWorldMatrix, &frameSurf->space->prevWorldMatrix, sizeof(outShaderSurface.prevWorldMatrix));

    if (frameSurf->subMesh) {
        const BE1::Sphere &sphere = frameSurf->subMesh->GetBoundingSphere();
        const BE1::Vec3 &sphereCenter = sphere.Center();
        outShaderSurface.localBoundingSphere = float4(sphereCenter.x, sphereCenter.y, sphereCenter.z, sphere.Radius());
    } else {
        outShaderSurface.localBoundingSphere = float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    outShaderSurface.shaderMaterialIndex = frameSurf->shaderMaterialIndex;
    outShaderSurface.flags = BE1::HasFlag(frameSurf->space->flags, FrameObject::Flag::NonUniformScale) ? ShaderSurfaceFlagNonUniformScale : 0;
    outShaderSurface.flags |= (static_cast<uint32_t>(frameSurf->space->ditherPatternSeed) & ShaderSurfaceDitherPatternMask) << ShaderSurfaceDitherPatternShift;
    outShaderSurface.flags |= static_cast<uint32_t>(frameSurf->space->fadeOpacity * 65535.0f + 0.5f) << ShaderSurfaceFadeOpacityShift;

    const FrameSkinnedMesh *frameSkinnedMesh = frameSurf->space->skinnedMesh;
    if (frameSkinnedMesh) {
        outShaderSurface.skinningJointOffset = frameSkinnedMesh->skinningJointOffset;
        outShaderSurface.prevSkinningJointOffset = frameSkinnedMesh->prevSkinningJointOffset;
    } else {
        outShaderSurface.skinningJointOffset = ShaderSkinningJointOffsetInvalid;
        outShaderSurface.prevSkinningJointOffset = ShaderSkinningJointOffsetInvalid;
    }
}
