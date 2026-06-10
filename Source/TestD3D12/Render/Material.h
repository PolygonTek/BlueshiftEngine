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

#pragma once

#include "ShaderTemplate.h"
#include "Shaders/ShaderInterop.h"

class MaterialManager;
class Texture;

/*
---------------------------------------------------------------------------------------------------

    Material

---------------------------------------------------------------------------------------------------
*/

class Material : public std::enable_shared_from_this<Material> {
    friend class MaterialManager;

public:
    enum class Flag : uint16_t {
        None                        = 0,
        NoShadow                    = BIT(0),
        LoadedFromFile              = BIT(3)
    };

    enum class RenderingMode : uint8_t {
        Opaque,
        AlphaCutoff,
        Blend
    };

    enum class TextureHint : uint8_t {
        None,
        Sprite,
        Overlay,
        EnvCubeMap
    };

    const char *                    GetName() const { return name.c_str(); }
    const char *                    GetHashName() const { return hashName.c_str(); }

    uint64_t                        GetRetainedFrameSerial() const { return retainedFrameSerial; }
    void                            SetRetainedFrameSerial(uint64_t frameSerial) { retainedFrameSerial = frameSerial; }

    uint32_t                        GetIndex() const { return index; }
    const SharedPtr<ShaderTemplate> &GetShaderTemplate() const { return internalData.shaderTemplate; }
    RHI::CullMode                   GetCullMode() const { return internalData.cullMode; }
    RenderingMode                   GetRenderingMode() const { return internalData.renderingMode; }
    RHI::BlendStateType             GetBlendType() const { return internalData.blendType; }
    Flag                            GetFlags() const { return internalData.flags; }

    int                             FindPropertyIndex(const char *propertyName) const;
    const ShaderTemplate::PropertyValue *GetPropertyValue(int propertyIndex) const;
    const ShaderTemplate::PropertyValue *GetPropertyValue(const char *propertyName) const;
    const SharedPtr<Texture> *      GetTextureProperty(int propertyIndex) const;
    const SharedPtr<Texture> *      GetTextureProperty(const char *propertyName) const;
    bool                            SetPropertyValue(int propertyIndex, const ShaderTemplate::PropertyValue &value);
    bool                            SetPropertyValue(const char *propertyName, const ShaderTemplate::PropertyValue &value);

    uint32_t                        GetStreamingListIndex() const { return streamingListIndex; }

    void                            InvalidatePermutationKeys() const;
    ShaderPermutationKey            EnsurePermutation(const ShaderFeatureSet &featureSet = {}) const;
    void                            EnsurePermutations(const BE1::Array<ShaderFeatureSet> &featureSets) const;

    SharedPtr<ShaderPipeline>       GetGraphicsPipeline(const ShaderFeatureSet &featureSet, const ShaderPipelineDesc &baseDesc) const;

    bool                            Create(const char *text, const char *baseDir);

    bool                            Load(const char *filename);
    bool                            Reload();

private:
    struct TextureProperty {
        int                         propertyIndex = -1; // shader property index
        SharedPtr<Texture>          texture;
    };

    struct InternalData {
        void                        Purge();
        void                        Swap(InternalData &rhs);

        SharedPtr<ShaderTemplate>   shaderTemplate;
        BE1::Array<ShaderTemplate::PropertyValue> propertyValues;
        BE1::Array<TextureProperty> textureProperties;
        RHI::CullMode               cullMode = RHI::CullMode::Back;
        RenderingMode               renderingMode = RenderingMode::Opaque;
        RHI::BlendStateType         blendType = RHI::BlendStateType::Disabled;
        Flag                        flags = Flag::None;
    };

    struct CachedPermutationKey {
        ShaderFeatureSet            featureSet;
        ShaderPermutationKey        key;
    };

    void                            Purge();

    bool                            Parse(const char *text, const char *baseDir, InternalData &outInternalData);
    bool                            ParseCull(BE1::Lexer &lexer, InternalData &outInternalData);
    bool                            ParseRenderingMode(BE1::Lexer &lexer, InternalData &outInternalData);
    bool                            ParseBlendType(BE1::Lexer &lexer, InternalData &outInternalData);
    bool                            ParseShaderTemplate(BE1::Lexer &lexer, const char *baseDir, InternalData &outInternalData);
    bool                            ParseProperties(BE1::Lexer &lexer, InternalData &outInternalData);

    void                            UpdateTextureProperties();
    void                            RemapShaderTemplateProperties(const BE1::Array<ShaderPropertyInfo> &oldProperties);
    bool                            HasStreamingTextureProperties() const;
    void                            RequestStreamingMaxDimension(uint32_t requestedMaxDimension) const;

    BE1::Str                        hashName;
    BE1::Str                        name;
    uint32_t                        index = 0;
    uint64_t                        retainedFrameSerial = 0;
    InternalData                    internalData;
    uint32_t                        streamingListIndex = TextureStreamingInvalidIndex;

    mutable BE1::Array<CachedPermutationKey> cachedPermutationKeys;
};

template<>
struct enable_bitmask_operators<Material::Flag> {
    static const bool enable = true;
};

class MaterialManager {
    friend class Material;

public:
    void                            Init();
    void                            Shutdown();

    void                            UpdateFrame();

    SharedPtr<Material>             AllocMaterial(const char *hashName);
    SharedPtr<Material>             FindMaterial(const char *hashName) const;
    SharedPtr<Material>             GetMaterial(const char *hashName);
    SharedPtr<Material>             GetSingleTextureMaterial(const char *textureFilename, Material::TextureHint hint = Material::TextureHint::None);

    uint32_t                        GetStreamingMaterialCount() const { return streamingMaterials.Count(); }
    void                            ApplyStreamingFeedback(const uint32_t *feedbackData, uint32_t feedbackCapacity);

    SharedPtr<Material>             defaultUnlitMaterial;
    SharedPtr<Material>             defaultLitMaterial;
    SharedPtr<Material>             whiteUnlitMaterial;
    SharedPtr<Material>             whiteLitMaterial;
    SharedPtr<Material>             blendUnlitMaterial;
    SharedPtr<Material>             blendLitMaterial;

private:
    void                            OnShaderTemplateReloaded(const ShaderTemplate *shaderTemplate, const BE1::Array<ShaderPropertyInfo> &oldProperties);
    void                            CleanExpiredMaterials();
    void                            CreateEngineMaterials();
    void                            DestroyEngineMaterials();
    void                            RefreshStreamingMaterial(const SharedPtr<Material> &material);
    void                            RegisterStreamingMaterial(const SharedPtr<Material> &material);
    void                            UnregisterStreamingMaterial(Material *material);

    BE1::StrIHashMap<WeakPtr<Material>> materialHashMap;
    int                             expiredMaterialHashMapCursor = 0;
    uint32_t                        nextMaterialIndex = 0;
    BE1::Array<WeakPtr<Material>>   streamingMaterials;
    BE1::DelegateHandle             shaderTemplateReloadedHandle;
};

extern MaterialManager              materialManager;
