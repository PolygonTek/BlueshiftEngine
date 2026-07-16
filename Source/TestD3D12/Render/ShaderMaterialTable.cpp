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
#include "ShaderMaterialTable.h"
#include "RenderInternal.h"
#include "Render/Material/Material.h"
#include "Render/Texture/TextureManager.h"

bool ShaderMaterialTable::Init(uint32_t initialCapacity) {
    return EnsureBufferCapacity(initialCapacity);
}

void ShaderMaterialTable::Shutdown() {
    DestroyBuffer();
}

void ShaderMaterialTable::Clear() {
    materials.SetCount(0, false);
    materialHash.Clear();

    dirty = false;
}

uint32_t ShaderMaterialTable::Register(const Material *material) {
    assert(material);

    int hash = materialHash.GenerateHash(material);
    for (int index = materialHash.First(hash); index != -1; index = materialHash.Next(index)) {
        if (materials[index] == material) {
            return index;
        }
    }

    uint32_t newShaderMaterialIndex = materials.Count();
    materials.Append(material);
    materialHash.Add(hash, newShaderMaterialIndex);
    dirty = true;

    return newShaderMaterialIndex;
}

bool ShaderMaterialTable::EnsureBufferCapacity(uint32_t count) {
    if (count <= tableBuffer.capacity) {
        return true;
    }

    // 초기 개수는 256 으로 시작해서, 필요한 용량이 될 때까지 2배씩 늘린다.
    uint32_t newCapacity = tableBuffer.capacity > 0 ? tableBuffer.capacity : 256;
    while (newCapacity < count) {
        newCapacity <<= 1;
    }

    DestroyBuffer();

    // GPU 디폴트 버퍼를 생성한다.
    uint32_t bufferSize = newCapacity * sizeof(ShaderMaterial);
    tableBuffer.buffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::ShaderResource, bufferSize, nullptr);
    if (!tableBuffer.buffer) {
        DestroyBuffer();
        return false;
    }

    RHI::renderer->SetName(tableBuffer.buffer, "shaderMaterialTableBuffer");

    tableBuffer.view = RHI::renderer->CreateView(tableBuffer.buffer, RHI::ViewType::SRV, RHI::Format::Unknown, sizeof(ShaderMaterial), 0, bufferSize);
    if (!tableBuffer.view.IsValid()) {
        DestroyBuffer();
        return false;
    }

    tableBuffer.capacity = newCapacity;
    tableBuffer.state = RHI::GPUResourceState::Undefined;
    tableBuffer.bindlessIndex = tableBuffer.buffer->GetBindlessIndex(RHI::ViewType::SRV, tableBuffer.view);
    return true;
}

bool ShaderMaterialTable::Upload(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, bool allowResize) {
    PROFILER_CPU_SCOPED_EVENT("ShaderMaterialTable::Upload");

    if (!dirty || materials.Count() == 0) {
        return true;
    }

    const int numMaterials = materials.Count();
    if (numMaterials > tableBuffer.capacity) {
        assert(allowResize);
        if (!allowResize) {
            return false;
        }
        if (!EnsureBufferCapacity(numMaterials)) {
            return false;
        }
    }

    RHI::Buffer *uploadBuffer = frameThreadData->AllocStructuredBuffer(sizeof(ShaderMaterial), numMaterials, nullptr);
    if (!uploadBuffer) {
        return false;
    }

    ShaderMaterial *shaderMaterials = reinterpret_cast<ShaderMaterial *>(uploadBuffer->mappedPtr);
    for (int materialIndex = 0; materialIndex < numMaterials; materialIndex++) {
        Build(materials[materialIndex], shaderMaterials[materialIndex]);
    }

    const uint32_t uploadSize = numMaterials * sizeof(ShaderMaterial);

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(tableBuffer.buffer, tableBuffer.state, RHI::GPUResourceState::CopyDst));
    RHI::renderer->CopyBuffer(commandList, tableBuffer.buffer, 0, uploadBuffer, 0, uploadSize);
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(tableBuffer.buffer, RHI::GPUResourceState::CopyDst, RHI::GPUResourceState::ShaderResource));

    tableBuffer.state = RHI::GPUResourceState::ShaderResource;
    dirty = false;
    return true;
}

void ShaderMaterialTable::DestroyBuffer() {
    if (tableBuffer.buffer) {
        RHI::renderer->DestroyBuffer(tableBuffer.buffer);
        tableBuffer.buffer = nullptr;
    }

    tableBuffer.view = RHI::ViewID::InvalidID;
    tableBuffer.state = RHI::GPUResourceState::Undefined;
    tableBuffer.capacity = 0;
    tableBuffer.bindlessIndex = 0;
}

static const Texture *GetDefaultMaterialTexture() {
    if (textureManager.whiteTexture) {
        return textureManager.whiteTexture.get();
    }
    if (textureManager.defaultTexture) {
        return textureManager.defaultTexture.get();
    }
    return nullptr;
}

static const Texture *ResolveMaterialBaseColorTexture(const Material *material) {
    if (material) {
        const SharedPtr<Texture> *textureProperty = material->GetPropertyTexture(ShaderTemplate::StandardProperty::BaseColorTexture);
        if (textureProperty && *textureProperty) {
            return textureProperty->get();
        }
    }

    return textureManager.defaultTexture.get();
}

static const RHI::Texture *GetReadyRHITexture(const Texture *texture) {
    if (!texture || !texture->IsReadyForRendering()) {
        return nullptr;
    }

    return texture->GetRHITexture();
}

static float GetMaterialAlphaCutoff(const Material *material) {
    static constexpr float DefaultAlphaCutoff = 0.5f;

    if (material) {
        const ShaderTemplate::PropertyValue *propertyValue = material->GetPropertyValue(ShaderTemplate::StandardProperty::AlphaCutoff);
        if (propertyValue && propertyValue->GetType() == BE1::Variant::Type::Float) {
            return propertyValue->As<float>();
        }
    }

    return DefaultAlphaCutoff;
}

void ShaderMaterialTable::Build(const Material *material, ShaderMaterial &outShaderMaterial) const {
    const Texture *texture = ResolveMaterialBaseColorTexture(material);
    const RHI::Texture *rhiTexture = GetReadyRHITexture(texture);
    if (!rhiTexture) {
        texture = GetDefaultMaterialTexture();
        rhiTexture = GetReadyRHITexture(texture);
    }

    outShaderMaterial.baseColorTextureIndex = rhiTexture ? rhiTexture->GetBindlessIndex(RHI::ViewType::SRV) : 0;
    outShaderMaterial.normalTextureIndex = 0;
    outShaderMaterial.emissiveTextureIndex = 0;
    outShaderMaterial.streamingMaterialIndex = material ? material->GetStreamingListIndex() : TextureStreamingInvalidIndex;
    outShaderMaterial.baseColorFactor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    outShaderMaterial.alphaCutoff = GetMaterialAlphaCutoff(material);
    outShaderMaterial.padding0 = float3(0.0f, 0.0f, 0.0f);
}
