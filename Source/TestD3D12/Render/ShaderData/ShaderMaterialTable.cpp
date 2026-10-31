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
#include "Render/RenderInternal.h"
#include "ShaderPacking.h"
#include "Render/Material/Material.h"
#include "Render/Texture/Sampler.h"
#include "Render/Texture/SamplerManager.h"
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

static uint32_t GetMaterialTextureIndex(const Material *material, ShaderTemplate::StandardProperty standardProperty, const Texture *fallbackTexture) {
    if (material) {
        const SharedPtr<Texture> *textureProperty = material->GetPropertyTexture(standardProperty);
        if (textureProperty && *textureProperty) {
            const RHI::Texture *rhiTexture = (*textureProperty)->GetReadyRHITexture();
            if (!rhiTexture) {
                // 준비되지 않은 재질 텍스처는 white 로 대체한다.
                rhiTexture = textureManager.whiteTexture->GetRHITexture();
            }
            return rhiTexture->GetBindlessIndex(RHI::ViewType::SRV);
        }
    }
    return fallbackTexture->GetRHITexture()->GetBindlessIndex(RHI::ViewType::SRV);
}

static float GetMaterialFactor(const Material *material, ShaderTemplate::StandardProperty standardProperty) {
    static constexpr float DefaultFactor = 1.0f;

    if (material) {
        const ShaderTemplate::PropertyValue *propertyValue = material->GetPropertyValue(standardProperty);
        if (propertyValue && propertyValue->GetType() == BE1::Variant::Type::Float) {
            return BE1::Clamp(propertyValue->As<float>(), 0.0f, 1.0f);
        }
    }
    return DefaultFactor;
}

static float4 GetMaterialBaseColorFactor(const Material *material) {
    if (material) {
        const ShaderTemplate::PropertyValue *propertyValue = material->GetPropertyValue(ShaderTemplate::StandardProperty::BaseColorFactor);
        if (propertyValue && propertyValue->GetType() == BE1::Variant::Type::Color4) {
            const BE1::Color4 color = propertyValue->As<BE1::Color4>();
            return float4(color);
        }
    }
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}

static float3 GetMaterialEmissionColor(const Material *material) {
    if (material) {
        const ShaderTemplate::PropertyValue *propertyValue = material->GetPropertyValue(ShaderTemplate::StandardProperty::EmissionColor);
        if (propertyValue && propertyValue->GetType() == BE1::Variant::Type::Color3) {
            const BE1::Color3 color = propertyValue->As<BE1::Color3>();
            return float3(color);
        }
    }
    return float3(1.0f, 1.0f, 1.0f);
}

static float GetMaterialEmissionFactor(const Material *material) {
    static constexpr float DefaultEmissionFactor = 1.0f;

    if (material) {
        const ShaderTemplate::PropertyValue *propertyValue = material->GetPropertyValue(ShaderTemplate::StandardProperty::EmissionFactor);
        if (propertyValue && propertyValue->GetType() == BE1::Variant::Type::Float) {
            return BE1::Clamp(propertyValue->As<float>(), 0.0f, 16.0f);
        }
    }
    return DefaultEmissionFactor;
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

static float GetMaterialHeightScale(const Material *material) {
    static constexpr float DefaultHeightScale = 0.0f;

    if (material) {
        const ShaderTemplate::PropertyValue *propertyValue = material->GetPropertyValue(ShaderTemplate::StandardProperty::HeightScale);
        if (propertyValue && propertyValue->GetType() == BE1::Variant::Type::Float) {
            return BE1::Clamp(propertyValue->As<float>(), 0.0f, 1.0f);
        }
    }
    return DefaultHeightScale;
}

static BE1::Vec2 GetMaterialTexCoordProperty(const Material *material, ShaderTemplate::StandardProperty standardProperty, const BE1::Vec2 &defaultValue) {
    if (material) {
        const ShaderTemplate::PropertyValue *propertyValue = material->GetPropertyValue(standardProperty);
        if (propertyValue && propertyValue->GetType() == BE1::Variant::Type::Vec2) {
            return propertyValue->As<BE1::Vec2>();
        }
    }
    return defaultValue;
}

void ShaderMaterialTable::Build(const Material *material, ShaderMaterial &outShaderMaterial) const {
    outShaderMaterial.baseColorTextureIndex = GetMaterialTextureIndex(material, ShaderTemplate::StandardProperty::BaseColorMap, textureManager.missingTexture.get());
    outShaderMaterial.normalTextureIndex = GetMaterialTextureIndex(material, ShaderTemplate::StandardProperty::NormalMap, textureManager.flatNormalTexture.get());
    outShaderMaterial.emissiveTextureIndex = GetMaterialTextureIndex(material, ShaderTemplate::StandardProperty::EmissiveMap, textureManager.blackTexture.get());
    outShaderMaterial.metallicTextureIndex = GetMaterialTextureIndex(material, ShaderTemplate::StandardProperty::MetallicMap, textureManager.whiteTexture.get());
    outShaderMaterial.roughnessTextureIndex = GetMaterialTextureIndex(material, ShaderTemplate::StandardProperty::RoughnessMap, textureManager.whiteTexture.get());
    outShaderMaterial.aoTextureIndex = GetMaterialTextureIndex(material, ShaderTemplate::StandardProperty::AoMap, textureManager.whiteTexture.get());
    outShaderMaterial.clearCoatTextureIndex = GetMaterialTextureIndex(material, ShaderTemplate::StandardProperty::ClearCoatMap, textureManager.whiteTexture.get());
    outShaderMaterial.clearCoatRoughnessTextureIndex = GetMaterialTextureIndex(material, ShaderTemplate::StandardProperty::ClearCoatRoughnessMap, textureManager.whiteTexture.get());
    outShaderMaterial.clearCoatNormalTextureIndex = GetMaterialTextureIndex(material, ShaderTemplate::StandardProperty::ClearCoatNormalMap, textureManager.flatNormalTexture.get());
    outShaderMaterial.heightTextureIndex = GetMaterialTextureIndex(material, ShaderTemplate::StandardProperty::HeightMap, textureManager.whiteTexture.get());

    const Sampler *sampler = material ? material->GetSampler() : nullptr;
    if (!sampler) {
        sampler = samplerManager.defaultSampler;
    }
    outShaderMaterial.samplerIndex = sampler->GetBindlessIndex();
    outShaderMaterial.streamingMaterialIndex = material ? material->GetStreamingListIndex() : TextureStreamingInvalidIndex;

    outShaderMaterial.packedBaseColorFactor = ShaderPacking::PackFloat4ToRGBA8UNorm(GetMaterialBaseColorFactor(material));
    outShaderMaterial.alphaCutoff = GetMaterialAlphaCutoff(material);

    const float3 emissionColor = GetMaterialEmissionColor(material);
    outShaderMaterial.packedEmissionColorFactor = ShaderPacking::PackFloat4ToRGBA16F(float4(emissionColor.x, emissionColor.y, emissionColor.z, GetMaterialEmissionFactor(material)));

    outShaderMaterial.packedMetallicRoughnessFactors = ShaderPacking::PackFloat2ToRG16UNorm(float2(
        GetMaterialFactor(material, ShaderTemplate::StandardProperty::MetallicFactor),
        GetMaterialFactor(material, ShaderTemplate::StandardProperty::RoughnessFactor)));

    outShaderMaterial.aoStrength = GetMaterialFactor(material, ShaderTemplate::StandardProperty::AoStrength);

    outShaderMaterial.packedClearCoatFactors = ShaderPacking::PackFloat2ToRG16UNorm(float2(
        GetMaterialFactor(material, ShaderTemplate::StandardProperty::ClearCoatFactor),
        GetMaterialFactor(material, ShaderTemplate::StandardProperty::ClearCoatRoughnessFactor)));
    outShaderMaterial.heightScale = GetMaterialHeightScale(material);

    const BE1::Vec2 tcScale = GetMaterialTexCoordProperty(material, ShaderTemplate::StandardProperty::TcScale, BE1::Vec2(1.0f, 1.0f));
    const BE1::Vec2 tcTranslation = GetMaterialTexCoordProperty(material, ShaderTemplate::StandardProperty::TcTranslation, BE1::Vec2::zero);
    outShaderMaterial.packedTexCoordTransform = ShaderPacking::PackFloat4ToRGBA16F(float4(tcScale.x, tcScale.y, tcTranslation.x, tcTranslation.y));

    uint32_t flags = 0;
    if (material && material->GetRenderingMode() == Material::RenderingMode::Dithered) {
        // GPU table indices are assigned per frame; use the persistent material ID for a fixed pattern.
        flags |= (material->GetIndex() & 63u) << ShaderMaterialDitherPatternShift;
    }
    if (material && material->GetRenderingMode() == Material::RenderingMode::Blend) {
        flags |= ShaderMaterialFlagTransparent;
    }
    if (tcScale.x != 1.0f || tcScale.y != 1.0f || tcTranslation.x != 0.0f || tcTranslation.y != 0.0f) {
        flags |= ShaderMaterialFlagTexCoordTransform;
    }
    outShaderMaterial.flags = flags;
}
