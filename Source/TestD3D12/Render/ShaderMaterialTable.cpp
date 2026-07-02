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
    DestroyBuffers();
}

void ShaderMaterialTable::Clear() {
    materials.SetCount(0, false);
    materialHash.Clear();

    dirty = false;
}

void ShaderMaterialTable::DestroyBuffers() {
    if (buffer) {
        RHI::renderer->DestroyBuffer(buffer);
        buffer = nullptr;
    }

    bufferView = RHI::ViewID::InvalidID;
    bufferState = RHI::GPUResourceState::Undefined;
    bufferCapacity = 0;
    bufferBindlessIndex = 0;
}

bool ShaderMaterialTable::EnsureBufferCapacity(uint32_t count) {
    if (count <= bufferCapacity) {
        return true;
    }

    // 초기 개수는 256 으로 시작해서, 필요한 용량이 될 때까지 2배씩 늘린다.
    uint32_t newCapacity = bufferCapacity > 0 ? bufferCapacity : 256;
    while (newCapacity < count) {
        newCapacity <<= 1;
    }

    DestroyBuffers();

    // 디폴트 버퍼를 생성한다.
    uint32_t bufferSize = newCapacity * sizeof(ShaderMaterial);
    buffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::ShaderResource, bufferSize, nullptr);
    if (!buffer) {
        DestroyBuffers();
        return false;
    }

    RHI::renderer->SetName(buffer, "shaderMaterialTableBuffer");

    bufferView = RHI::renderer->CreateView(buffer, RHI::ViewType::SRV, RHI::Format::Unknown, sizeof(ShaderMaterial), 0, bufferSize);
    if (!bufferView.IsValid()) {
        DestroyBuffers();
        return false;
    }

    bufferCapacity = newCapacity;
    bufferState = RHI::GPUResourceState::Undefined;
    bufferBindlessIndex = buffer->GetBindlessIndex(RHI::ViewType::SRV, bufferView);
    return true;
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
        const SharedPtr<Texture> *textureProperty = material->GetPropertyTexture(ShaderTemplate::BuiltInProperty::BaseColorTexture);
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

bool ShaderMaterialTable::Upload(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, bool allowResize) {
    PROFILER_CPU_SCOPED_EVENT("ShaderMaterialTable::Upload");

    if (!dirty || materials.Count() == 0) {
        return true;
    }
    if (!frameThreadData || !commandList) {
        return false;
    }

    const int numMaterials = materials.Count();
    if (numMaterials > bufferCapacity) {
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

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(buffer, bufferState, RHI::GPUResourceState::CopyDst));
    RHI::renderer->CopyBuffer(commandList, buffer, 0, uploadBuffer, 0, uploadSize);
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(buffer, RHI::GPUResourceState::CopyDst, RHI::GPUResourceState::ShaderResource));

    bufferState = RHI::GPUResourceState::ShaderResource;
    dirty = false;
    return true;
}
