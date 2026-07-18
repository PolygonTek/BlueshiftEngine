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
#include "ShaderLightTable.h"
#include "RenderFrameData.h"
#include "RenderInternal.h"

bool ShaderLightTable::Init(uint32_t initialPointLightCapacity, uint32_t initialSpotLightCapacity, uint32_t initialBoxDirLightCapacity) {
    if (!EnsureBufferCapacity(initialPointLightCapacity, initialSpotLightCapacity, initialBoxDirLightCapacity)) {
        Shutdown();
        return false;
    }

    return true;
}

void ShaderLightTable::Shutdown() {
    DestroyBuffer(pointLightBuffer);
    DestroyBuffer(spotLightBuffer);
    DestroyBuffer(boxDirLightBuffer);
}

void ShaderLightTable::Clear() {
}

bool ShaderLightTable::EnsureBufferCapacity(uint32_t pointLightCount, uint32_t spotLightCount, uint32_t boxDirLightCount) {
    if (!EnsureBufferCapacity(pointLightBuffer, pointLightCount, sizeof(ShaderPointLight), "shaderPointLightTableBuffer")) {
        return false;
    }
    if (!EnsureBufferCapacity(spotLightBuffer, spotLightCount, sizeof(ShaderSpotLight), "shaderSpotLightTableBuffer")) {
        return false;
    }
    if (!EnsureBufferCapacity(boxDirLightBuffer, boxDirLightCount, sizeof(ShaderBoxDirLight), "shaderBoxDirLightTableBuffer")) {
        return false;
    }
    return true;
}

template<typename T>
static bool AllocShaderLightUploadBuffer(RHI::FrameThreadData *frameThreadData, uint32_t count, RHI::Buffer *&uploadBuffer, T *&shaderLights) {
    uploadBuffer = nullptr;
    shaderLights = nullptr;

    if (count == 0) {
        return true;
    }

    uploadBuffer = frameThreadData->AllocStructuredBuffer(sizeof(T), count, nullptr);
    if (!uploadBuffer) {
        return false;
    }

    shaderLights = reinterpret_cast<T *>(uploadBuffer->mappedPtr);
    return shaderLights != nullptr;
}

bool ShaderLightTable::Upload(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const RenderFrameData *frameData, bool allowResize) {
    PROFILER_CPU_SCOPED_EVENT("ShaderLightTable::Upload");

    const uint32_t pointLightCount = frameData->NumVisPointLights();
    const uint32_t spotLightCount = frameData->NumVisSpotLights();
    const uint32_t boxDirLightCount = frameData->NumVisBoxDirLights();

    if (pointLightCount == 0 && spotLightCount == 0 && boxDirLightCount == 0) {
        Clear();
        return true;
    }

    if (pointLightCount > pointLightBuffer.capacity || spotLightCount > spotLightBuffer.capacity || boxDirLightCount > boxDirLightBuffer.capacity) {
        assert(allowResize);
        if (!allowResize || !EnsureBufferCapacity(pointLightCount, spotLightCount, boxDirLightCount)) {
            return false;
        }
    }

    RHI::Buffer *pointLightUploadBuffer;
    RHI::Buffer *spotLightUploadBuffer;
    RHI::Buffer *boxDirLightUploadBuffer;

    ShaderPointLight *shaderPointLights;
    ShaderSpotLight *shaderSpotLights;
    ShaderBoxDirLight *shaderBoxDirLights;

    if (!AllocShaderLightUploadBuffer(frameThreadData, pointLightCount, pointLightUploadBuffer, shaderPointLights) ||
        !AllocShaderLightUploadBuffer(frameThreadData, spotLightCount, spotLightUploadBuffer, shaderSpotLights) ||
        !AllocShaderLightUploadBuffer(frameThreadData, boxDirLightCount, boxDirLightUploadBuffer, shaderBoxDirLights)) {
        return false;
    }

    for (const VisLight &visLight : frameData->GetVisLights()) {
        switch (visLight.type) {
        case RenderLight::Type::Point:
            assert(visLight.typeIndex < pointLightCount);
            Build(visLight, shaderPointLights[visLight.typeIndex]);
            break;
        case RenderLight::Type::Spot:
            assert(visLight.typeIndex < spotLightCount);
            Build(visLight, shaderSpotLights[visLight.typeIndex]);
            break;
        case RenderLight::Type::BoxDir:
            assert(visLight.typeIndex < boxDirLightCount);
            Build(visLight, shaderBoxDirLights[visLight.typeIndex]);
            break;
        case RenderLight::Type::Directional:
            break;
        default:
            assert(0);
            break;
        }
    }

    UploadBuffer(commandList, pointLightBuffer, pointLightUploadBuffer, pointLightCount * sizeof(ShaderPointLight));
    UploadBuffer(commandList, spotLightBuffer, spotLightUploadBuffer, spotLightCount * sizeof(ShaderSpotLight));
    UploadBuffer(commandList, boxDirLightBuffer, boxDirLightUploadBuffer, boxDirLightCount * sizeof(ShaderBoxDirLight));
    return true;
}

bool ShaderLightTable::EnsureBufferCapacity(TableBuffer &tableBuffer, uint32_t count, uint32_t elementSize, const char *name) {
    if (count <= tableBuffer.capacity) {
        return true;
    }

    uint32_t newCapacity = tableBuffer.capacity > 0 ? tableBuffer.capacity : 1;
    while (newCapacity < count) {
        newCapacity <<= 1;
    }

    DestroyBuffer(tableBuffer);

    const uint32_t bufferSize = newCapacity * elementSize;

    // GPU 디폴트 버퍼를 생성한다.
    tableBuffer.buffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::ShaderResource, bufferSize, nullptr);
    if (!tableBuffer.buffer) {
        DestroyBuffer(tableBuffer);
        return false;
    }

    RHI::renderer->SetName(tableBuffer.buffer, name);

    tableBuffer.view = RHI::renderer->CreateView(tableBuffer.buffer, RHI::ViewType::SRV, RHI::Format::Unknown, elementSize, 0, bufferSize);
    if (!tableBuffer.view.IsValid()) {
        DestroyBuffer(tableBuffer);
        return false;
    }

    tableBuffer.capacity = newCapacity;
    tableBuffer.state = RHI::GPUResourceState::Undefined;
    tableBuffer.bindlessIndex = tableBuffer.buffer->GetBindlessIndex(RHI::ViewType::SRV, tableBuffer.view);
    return true;
}

void ShaderLightTable::UploadBuffer(RHI::CommandList *commandList, TableBuffer &tableBuffer, RHI::Buffer *uploadBuffer, uint32_t uploadSize) {
    if (uploadSize == 0) {
        return;
    }

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(tableBuffer.buffer, tableBuffer.state, RHI::GPUResourceState::CopyDst));
    RHI::renderer->CopyBuffer(commandList, tableBuffer.buffer, 0, uploadBuffer, 0, uploadSize);
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(tableBuffer.buffer, RHI::GPUResourceState::CopyDst, RHI::GPUResourceState::ShaderResource));

    tableBuffer.state = RHI::GPUResourceState::ShaderResource;
}

void ShaderLightTable::DestroyBuffer(TableBuffer &tableBuffer) {
    if (tableBuffer.buffer) {
        RHI::renderer->DestroyBuffer(tableBuffer.buffer);
        tableBuffer.buffer = nullptr;
    }

    tableBuffer.view = RHI::ViewID::InvalidID;
    tableBuffer.state = RHI::GPUResourceState::Undefined;
    tableBuffer.capacity = 0;
    tableBuffer.bindlessIndex = 0;
}

void ShaderLightTable::Build(const VisLight &visLight, ShaderPointLight &shaderLight) const {
    BE1::simdProcessor->Memcpy(&shaderLight.worldToProjectionU, &visLight.viewProjScaleBiasMatrix[0], sizeof(shaderLight.worldToProjectionU) * 2);

    const float invRadius = 1.0f / visLight.size[0];
    shaderLight.origin = float3(visLight.origin.x, visLight.origin.y, visLight.origin.z);
    shaderLight.invRadiusSquared = invRadius * invRadius;
    shaderLight.lightColor = float3(visLight.lightColor.r, visLight.lightColor.g, visLight.lightColor.b);
    shaderLight.fallOffExponent = visLight.fallOffExponent;
    shaderLight.flags = static_cast<uint32_t>(visLight.flags);
    shaderLight.padding0 = float3(0.0f, 0.0f, 0.0f);
}

void ShaderLightTable::Build(const VisLight &visLight, ShaderSpotLight &shaderLight) const {
    BE1::simdProcessor->Memcpy(&shaderLight.viewProjScaleBiasMatrix, &visLight.viewProjScaleBiasMatrix, sizeof(shaderLight.viewProjScaleBiasMatrix));
    BE1::simdProcessor->Memcpy(&shaderLight.fallOffMatrix, &visLight.fallOffMatrix, sizeof(shaderLight.fallOffMatrix));

    const BE1::Vec3 &direction = visLight.axis[0];
    shaderLight.origin = float3(visLight.origin.x, visLight.origin.y, visLight.origin.z);
    shaderLight.fallOffExponent = visLight.fallOffExponent;
    shaderLight.direction = float3(direction.x, direction.y, direction.z);
    shaderLight.zNear = visLight.zNear;
    shaderLight.size = float3(visLight.size.x, visLight.size.y, visLight.size.z);
    shaderLight.flags = static_cast<uint32_t>(visLight.flags);
    shaderLight.lightColor = float3(visLight.lightColor.r, visLight.lightColor.g, visLight.lightColor.b);
    shaderLight.padding0 = 0.0f;
}

void ShaderLightTable::Build(const VisLight &visLight, ShaderBoxDirLight &shaderLight) const {
    BE1::simdProcessor->Memcpy(&shaderLight.worldToLightVolumeMatrix, &visLight.viewProjScaleBiasMatrix, sizeof(shaderLight.worldToLightVolumeMatrix));

    const BE1::Vec3 &direction = visLight.axis[0];
    shaderLight.origin = float3(visLight.origin.x, visLight.origin.y, visLight.origin.z);
    shaderLight.flags = static_cast<uint32_t>(visLight.flags);
    shaderLight.direction = float3(direction.x, direction.y, direction.z);
    shaderLight.fallOffExponent = visLight.fallOffExponent;
    shaderLight.size = float3(visLight.size.x, visLight.size.y, visLight.size.z);
    shaderLight.padding1 = 0.0f;
    shaderLight.lightColor = float3(visLight.lightColor.r, visLight.lightColor.g, visLight.lightColor.b);
    shaderLight.padding2 = 0.0f;
}
