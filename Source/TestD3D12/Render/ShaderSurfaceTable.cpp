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
    DestroyBuffers();
}

void ShaderSurfaceTable::Clear() {
}

bool ShaderSurfaceTable::EnsureBufferCapacity(uint32_t count) {
    if (count <= bufferCapacity) {
        return true;
    }

    uint32_t newCapacity = bufferCapacity > 0 ? bufferCapacity : 1024;
    while (newCapacity < count) {
        newCapacity <<= 1;
    }

    DestroyBuffers();

    const uint32_t bufferSize = newCapacity * sizeof(ShaderSurface);

    // GPU 디폴트 버퍼를 생성한다.
    buffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::ShaderResource, bufferSize, nullptr);
    if (!buffer) {
        DestroyBuffers();
        return false;
    }

    RHI::renderer->SetName(buffer, "shaderSurfaceTableBuffer");

    bufferView = RHI::renderer->CreateView(buffer, RHI::ViewType::SRV, RHI::Format::Unknown, sizeof(ShaderSurface), 0, bufferSize);
    if (!bufferView.IsValid()) {
        DestroyBuffers();
        return false;
    }

    bufferCapacity = newCapacity;
    bufferState = RHI::GPUResourceState::Undefined;
    bufferBindlessIndex = buffer->GetBindlessIndex(RHI::ViewType::SRV, bufferView);
    return true;
}

bool ShaderSurfaceTable::Upload(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const VisCamera *visCamera, bool allowResize) {
    PROFILER_CPU_SCOPED_EVENT("ShaderSurfaceTable::Upload");

    if (visCamera->numVisSurfs == 0) {
        Clear();
        return true;
    }

    const uint32_t surfaceCount = visCamera->numVisSurfs;
    if (surfaceCount > bufferCapacity) {
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

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(buffer, bufferState, RHI::GPUResourceState::CopyDst));
    RHI::renderer->CopyBuffer(commandList, buffer, 0, uploadBuffer, 0, uploadSize);
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(buffer, RHI::GPUResourceState::CopyDst, RHI::GPUResourceState::ShaderResource));

    bufferState = RHI::GPUResourceState::ShaderResource;
    return true;
}

void ShaderSurfaceTable::DestroyBuffers() {
    if (buffer) {
        RHI::renderer->DestroyBuffer(buffer);
        buffer = nullptr;
    }

    bufferView = RHI::ViewID::InvalidID;
    bufferState = RHI::GPUResourceState::Undefined;
    bufferCapacity = 0;
    bufferBindlessIndex = 0;
}

void ShaderSurfaceTable::Build(const VisSurf *visSurf, ShaderSurface &outShaderSurface) const {
    assert(visSurf);
    assert(visSurf->space);

    BE1::simdProcessor->Memcpy(&outShaderSurface.worldMatrix, &visSurf->space->worldMatrix, sizeof(outShaderSurface.worldMatrix));
    BE1::simdProcessor->Memcpy(&outShaderSurface.prevWorldMatrix, &visSurf->space->prevWorldMatrix, sizeof(outShaderSurface.prevWorldMatrix));

    if (visSurf->subMesh) {
        const BE1::Sphere &sphere = visSurf->subMesh->GetBoundingSphere();
        const BE1::Vec3 &sphereCenter = sphere.Center();
        outShaderSurface.localBoundingSphere = float4(sphereCenter.x, sphereCenter.y, sphereCenter.z, sphere.Radius());
    } else {
        outShaderSurface.localBoundingSphere = float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    outShaderSurface.shaderMaterialIndex = visSurf->shaderMaterialIndex;
    outShaderSurface.padding0 = 0;
    outShaderSurface.padding1 = 0;
    outShaderSurface.padding2 = 0;
}
