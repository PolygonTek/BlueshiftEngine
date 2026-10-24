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

#include "Render/ShaderPipeline/ShaderTemplate.h"
#include "Shaders/ShaderInterop.h"

class MaterialManager;
class LoadingSystem;
class Sampler;
class Texture;

/*
---------------------------------------------------------------------------------------------------

    Material

---------------------------------------------------------------------------------------------------
*/

class Material : public std::enable_shared_from_this<Material> {
    friend class MaterialManager;
    friend class LoadingSystem;

public:
    ~Material() { Purge(); }

    enum class Flag : uint16_t {
        None                        = 0,
        NoShadow                    = BIT(0),
        LoadedFromFile              = BIT(3)
    };

    friend constexpr std::true_type EnableBitmaskEnum(Flag) noexcept { return {}; }

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
                                    // shader template 을 교체하고 이름과 타입이 같은 기존 프로퍼티 값을 유지한다.
    bool                            SetShaderTemplate(const SharedPtr<ShaderTemplate> &shaderTemplate);
    bool                            IsSkySurface() const { return internalData.shaderTemplate && internalData.shaderTemplate->IsSkySurface(); }
    RHI::CullMode                   GetCullMode() const { return internalData.cullMode; }
    RHI::TextureAddressMode         GetTextureAddressMode() const { return internalData.textureAddressMode; }
    const Sampler *                 GetSampler() const { return sampler; }
    RenderingMode                   GetRenderingMode() const { return internalData.renderingMode; }
    RHI::BlendStateType             GetBlendType() const { return internalData.blendType; }
    Flag                            GetFlags() const { return internalData.flags; }

    int                             FindPropertyIndex(const char *propertyName) const;
    int                             FindPropertyIndex(ShaderTemplate::StandardProperty standardProperty) const;
    const ShaderTemplate::PropertyValue *GetPropertyValue(int propertyIndex) const;
    const ShaderTemplate::PropertyValue *GetPropertyValue(const char *propertyName) const;
    const ShaderTemplate::PropertyValue *GetPropertyValue(ShaderTemplate::StandardProperty standardProperty) const;
    const SharedPtr<Texture> *      GetPropertyTexture(int propertyIndex) const;
    const SharedPtr<Texture> *      GetPropertyTexture(const char *propertyName) const;
    const SharedPtr<Texture> *      GetPropertyTexture(ShaderTemplate::StandardProperty standardProperty) const;

                                    // 메인 스레드에서 property 값을 안전하게 변경하기 위해 이전 프레임 렌더링 완료 후 적용된다.
    bool                            SetPropertyValue(int propertyIndex, const ShaderTemplate::PropertyValue &value);
    bool                            SetPropertyValue(const char *propertyName, const ShaderTemplate::PropertyValue &value);

    uint32_t                        GetStreamingListIndex() const { return streamingListIndex; }

                                    // material 에 캐시된 permutation key 를 무효화한다.
    void                            InvalidatePermutationKeys() const;

                                    // (지원하지 않는 shader features 를 제외한) material permutation key 를 등록한다.
    ShaderPermutationKey            EnsurePermutation(const ShaderFeatureSet &featureSet = {}) const;
    void                            EnsurePermutations(const BE1::Array<ShaderFeatureSet> &featureSets) const;

                                    // featureSet/baseGraphicsPipelineDesc 기반에서 material 의 렌더링 상태를 반영하여 graphics pipeline 을 가져온다.
    ShaderPipeline *                GetGraphicsPipeline(const ShaderFeatureSet &featureSet, const GraphicsPipelineDesc &baseGraphicsPipelineDesc, bool async = true) const;

    bool                            Create(const char *text, const char *baseDir);

                                    // 리로드 완료 후 리소스를 사용할 수 있으면 true 를 반환한다. 새 데이터 로딩 성공 여부와는 무관하다.
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
        RHI::TextureAddressMode     textureAddressMode = RHI::TextureAddressMode::Repeat;
        RenderingMode               renderingMode = RenderingMode::Opaque;
        RHI::BlendStateType         blendType = RHI::BlendStateType::Disabled;
        Flag                        flags = Flag::None;
    };

    struct CachedPermutationKey {
        ShaderPermutationKey        key;
        ShaderFeatureSet            featureSet;
    };

    void                            Purge();

    static bool                     Prepare(const char *filename, InternalData &outInternalData);
    void                            ApplyPrepared(InternalData &newInternalData);

    bool                            Load(const char *filename);

    static bool                     Parse(const char *text, const char *baseDir, const char *materialName, InternalData &outInternalData);
    static bool                     ParseCull(BE1::Lexer &lexer, const char *materialName, InternalData &outInternalData);
    static bool                     ParseTextureAddressMode(BE1::Lexer &lexer, const char *materialName, InternalData &outInternalData);
    static bool                     ParseRenderingMode(BE1::Lexer &lexer, const char *materialName, InternalData &outInternalData);
    static bool                     ParseBlendType(BE1::Lexer &lexer, const char *materialName, InternalData &outInternalData);
    static bool                     ParseShaderTemplate(BE1::Lexer &lexer, const char *baseDir, const char *materialName, InternalData &outInternalData);
    static bool                     ParseProperties(BE1::Lexer &lexer, const char *materialName, InternalData &outInternalData);

    void                            UpdateSampler();
    void                            UpdateTextureProperties();
    void                            RemapShaderTemplateProperties(const BE1::Array<ShaderPropertyDesc> &oldProperties);
    bool                            HasStreamingTextureProperties() const;
    void                            RequestStreamingMaxDimension(uint32_t requestedMaxDimension) const;

    BE1::Str                        hashName;
    BE1::Str                        name;
    uint32_t                        index = 0;
    uint64_t                        retainedFrameSerial = 0;
    InternalData                    internalData;
    Sampler *                       sampler = nullptr;
    uint32_t                        streamingListIndex = TextureStreamingInvalidIndex;

    mutable BE1::Array<CachedPermutationKey> cachedPermutationKeys;
};
