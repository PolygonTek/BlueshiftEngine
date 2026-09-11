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
#include "RenderInternal.h"

bool ShaderSurfaceTable::Init(uint32_t initialCapacity) {
    return EnsureBufferCapacity(initialCapacity);
}

void ShaderSurfaceTable::Shutdown() {
    DestroyBuffer();
}

void ShaderSurfaceTable::Clear() {
}

bool ShaderSurfaceTable::EnsureBufferCapacity(uint32_t count) {
    if (count <= tableBuffer.capacity) {
        return true;
    }

    uint32_t newCapacity = tableBuffer.capacity > 0 ? tableBuffer.capacity : 1024;
    while (newCapacity < count) {
        newCapacity <<= 1;
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

bool ShaderSurfaceTable::Upload(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const VisCamera *visCamera, bool allowResize) {
    PROFILER_CPU_SCOPED_EVENT("ShaderSurfaceTable::Upload");

    if (visCamera->numVisSurfs == 0) {
        Clear();
        return true;
    }

    const uint32_t surfaceCount = visCamera->numVisSurfs;
    if (surfaceCount > tableBuffer.capacity) {
        assert(allowResize);
        if (!allowResize) {
            return false;
        }
        if (!EnsureBufferCapacity(surfaceCount)) {
            return false;
        }
    }

    RHI::Buffer *uploadBuffer = frameThreadData->AllocStructuredBuffer(sizeof(ShaderSurface), surfaceCount, nullptr);
    if (!uploadBuffer) {
        return false;
    }

    ShaderSurface *shaderSurfaces = reinterpret_cast<ShaderSurface *>(uploadBuffer->mappedPtr);
    for (uint32_t surfaceIndex = 0; surfaceIndex < surfaceCount; ++surfaceIndex) {
        const VisSurf *visSurf = visCamera->visSurfs[surfaceIndex];
        assert(visSurf);
        assert(visSurf->index == surfaceIndex);
        Build(visSurf, shaderSurfaces[surfaceIndex]);
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

void ShaderSurfaceTable::Build(const VisSurf *visSurf, ShaderSurface &outShaderSurface) const {
    assert(visSurf);
    assert(visSurf->space);

    const BE1::Mat3x4 &worldMatrix = visSurf->space->worldMatrix;
    BE1::simdProcessor->Memcpy(&outShaderSurface.worldMatrix, &worldMatrix, sizeof(outShaderSurface.worldMatrix));
    BE1::simdProcessor->Memcpy(&outShaderSurface.prevWorldMatrix, &visSurf->space->prevWorldMatrix, sizeof(outShaderSurface.prevWorldMatrix));

    if (visSurf->subMesh) {
        const BE1::Sphere &sphere = visSurf->subMesh->GetBoundingSphere();
        const BE1::Vec3 &sphereCenter = sphere.Center();
        outShaderSurface.localBoundingSphere = float4(sphereCenter.x, sphereCenter.y, sphereCenter.z, sphere.Radius());
    } else {
        outShaderSurface.localBoundingSphere = float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    // TODO: 외부에서 non-uniform scale 여부를 직접 전달할 수 있게 되면 ShaderSurface flags 에 반영한다.
    // 현재는 모든 surface 를 uniform scale 로 가정하여 매 프레임 행렬을 검사하는 비용을 피한다.
    /*
    const float scaleXSqr = worldMatrix[0][0] * worldMatrix[0][0] + worldMatrix[1][0] * worldMatrix[1][0] + worldMatrix[2][0] * worldMatrix[2][0];
    const float scaleYSqr = worldMatrix[0][1] * worldMatrix[0][1] + worldMatrix[1][1] * worldMatrix[1][1] + worldMatrix[2][1] * worldMatrix[2][1];
    const float scaleZSqr = worldMatrix[0][2] * worldMatrix[0][2] + worldMatrix[1][2] * worldMatrix[1][2] + worldMatrix[2][2] * worldMatrix[2][2];
    const float minScaleSqr = BE1::Min3(scaleXSqr, scaleYSqr, scaleZSqr);
    const float maxScaleSqr = BE1::Max3(scaleXSqr, scaleYSqr, scaleZSqr);
    const bool nonUniformScale = maxScaleSqr - minScaleSqr > maxScaleSqr * MATRIX_EPSILON;
    */

    outShaderSurface.shaderMaterialIndex = visSurf->shaderMaterialIndex;
    outShaderSurface.flags = 0;

    const VisSkinnedMesh *visSkinnedMesh = visSurf->space->skinnedMesh;
    if (visSkinnedMesh) {
        outShaderSurface.skinningJointOffset = visSkinnedMesh->skinningJointOffset;
        outShaderSurface.prevSkinningJointOffset = visSkinnedMesh->prevSkinningJointOffset;
    } else {
        outShaderSurface.skinningJointOffset = ShaderSkinningJointOffsetInvalid;
        outShaderSurface.prevSkinningJointOffset = ShaderSkinningJointOffsetInvalid;
    }
}
