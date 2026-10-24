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

#include "Containers/HashIndex.h"
#include "Containers/HashMap.h"
#include "Core/Delegate.h"
#include "Core/Property.h"
#include "ShaderPipeline.h"

class ShaderTemplateManager;
class LoadingSystem;

/*
---------------------------------------------------------------------------------------------------

    Shader Property Descriptor

---------------------------------------------------------------------------------------------------
*/

struct ShaderPropertyDesc : public BE1::PropertyDesc {
    enum class ShaderFlag : uint8_t {
        None                        = 0,
        ShaderDefine                = BIT(0),
        Hidden                      = BIT(1),
        Normal                      = BIT(2),
        Linear                      = BIT(3),
        NoStreaming                 = BIT(4),
        NoMipmaps                   = BIT(5)
    };

    friend constexpr std::true_type EnableBitmaskEnum(ShaderFlag) noexcept { return {}; }

    enum class TextureType : uint8_t {
        None,
        Texture2D,
        TextureCube,
        Texture3D
    };

    struct Condition {
        Condition() = default;
        Condition(const char *name, const BE1::Array<int> &valueList) : name(name), valueList(valueList) {}

        BE1::Str                    name;
        BE1::Array<int>             valueList;
    };

    ShaderPropertyDesc() = default;
    ShaderPropertyDesc(const char *name, const char *label, BE1::Variant::Type::Enum type, const BE1::Variant &defaultValue, const char *description, int flags, ShaderFlag shaderFlags = ShaderFlag::None)
        : BE1::PropertyDesc(name, label, type, intptr_t(0), defaultValue, description, flags), shaderFlags(shaderFlags) {}

    bool                            IsShaderDefine() const { return BE1::HasFlag(shaderFlags, ShaderFlag::ShaderDefine); }
    bool                            IsHidden() const { return BE1::HasFlag(shaderFlags, ShaderFlag::Hidden); }
    bool                            IsNormal() const { return BE1::HasFlag(shaderFlags, ShaderFlag::Normal); }
    bool                            IsLinear() const { return BE1::HasFlag(shaderFlags, ShaderFlag::Linear); }
    bool                            IsTexture() const { return textureType != TextureType::None; }
    bool                            IsNoStreaming() const { return BE1::HasFlag(shaderFlags, ShaderFlag::NoStreaming); }
    bool                            IsNoMipmaps() const { return BE1::HasFlag(shaderFlags, ShaderFlag::NoMipmaps); }
    bool                            Parse(BE1::Lexer &lexer);

    ShaderFlag                      shaderFlags = ShaderFlag::None;
    TextureType                     textureType = TextureType::None;
    BE1::Array<Condition>           conditions;
};

/*
---------------------------------------------------------------------------------------------------

    Shader Template

    - stages, baseDefines, features, properties 정보를 보관하는 셰이더 템플릿이다.
    - shader define property 값들과 featureSet 조합으로 필요한 shader permutation 을 미리 등록해야 한다.
    - 셰이더 컴파일에 필요한 정보는 shaderTemplate 과 등록된 shaderPermutationKey 로 얻을 수 있다.

---------------------------------------------------------------------------------------------------
*/

class ShaderTemplate : public std::enable_shared_from_this<ShaderTemplate> {
    friend class ShaderTemplateManager;
    friend class LoadingSystem;

public:
    using ReloadedDelegate = BE1::MulticastDelegate<void(const ShaderTemplate *, const BE1::Array<ShaderPropertyDesc> &)>;

    enum class Flag : uint8_t {
        None                        = 0,
        LitSurface                  = BIT(1),
        SkySurface                  = BIT(2),
        LoadedFromFile              = BIT(3)
    };

    friend constexpr std::true_type EnableBitmaskEnum(Flag) noexcept { return {}; }

    enum class StandardProperty : uint8_t {
        AlphaCutoff,
        BaseColorMap,
        BaseColorFactor,
        NormalMap,
        EmissiveMap,
        EmissionColor,
        EmissionFactor,
        AoMap,
        AoStrength,
        MetallicMap,
        RoughnessMap,
        MetallicFactor,
        RoughnessFactor,
        ClearCoatMap,
        ClearCoatFactor,
        ClearCoatRoughnessMap,
        ClearCoatRoughnessFactor,
        ClearCoatNormalMap,
        HeightMap,
        HeightScale,
        ShadowTexture,
        SkyCubeMap,
        SkyBackMap,
        SkyFrontMap,
        SkyRightMap,
        SkyLeftMap,
        SkyTopMap,
        SkyDownMap,
        TcScale,
        TcTranslation,
        Count
    };

    using PropertyValue = BE1::Variant;

    ShaderTemplate() {}
    ShaderTemplate(const ShaderTemplate &rhs) = delete;
    ShaderTemplate &operator=(const ShaderTemplate &rhs) = delete;
    ~ShaderTemplate() { Purge(); }

    const char *                    GetName() const { return name.c_str(); }
    const char *                    GetHashName() const { return hashName.c_str(); }

    const BE1::Array<ShaderPropertyDesc> &GetProperties() const { return internalData.properties; }
    bool                            IsSkySurface() const { return BE1::HasFlag(internalData.flags, Flag::SkySurface); }

    const ShaderPropertyDesc *      FindProperty(const char *name) const { return internalData.FindProperty(name); }
    int                             FindPropertyIndex(const char *name) const { return internalData.FindPropertyIndex(name); }
    int                             GetStandardPropertyIndex(StandardProperty standardProperty) const { return internalData.GetStandardPropertyIndex(standardProperty); }
    bool                            AddProperty(const ShaderPropertyDesc &propertyDesc);
    bool                            AddProperty(const char *name, const char *label, BE1::Variant::Type::Enum type, const BE1::Variant &defaultValue, const char *description = "", int flags = BE1::PropertyDesc::Flag::Editor, ShaderPropertyDesc::ShaderFlag shaderFlags = ShaderPropertyDesc::ShaderFlag::None);
    bool                            CopyPropertiesFrom(const ShaderTemplate &shaderTemplate);

                                    // 프로퍼티 초기값 배열을 만들어 준다.
    void                            InitDefaultPropertyValues(BE1::Array<PropertyValue> &outPropertyValues) const;

                                    // 지원하는 feature 인지 검사
    bool                            SupportsFeature(ShaderFeature feature) const;

                                    // requestedFeatureSet 에서 이 템플릿이 지원하는 feature 만 포함한 featureSet 을 리턴한다.
    ShaderFeatureSet                MaskSupportedFeatures(const ShaderFeatureSet &requestedFeatureSet) const;

    bool                            Create(const char *text, const char *baseDir);

                                    // 리로드 완료 후 리소스를 사용할 수 있으면 true 를 반환한다. 새 데이터 로딩 성공 여부와는 무관하다.
    bool                            Reload();

                                    // defines (shader define properties) + featureSet flags 으로 permutation 을 등록하고 그 hash key 를 리턴한다.
    ShaderPermutationKey            RegisterPermutation();
    ShaderPermutationKey            RegisterPermutation(const BE1::Array<PropertyValue> &propertyValues);
    ShaderPermutationKey            RegisterPermutation(const BE1::Array<PropertyValue> &propertyValues, const ShaderFeatureSet &featureSet);

                                    // 셰이더 컴파일에 필요한 정보들을 준비한다.
    bool                            PrepareShaderCompileData(ShaderPermutationKey permutationKey, ShaderCompileData &outCompileData) const;

    static ReloadedDelegate         reloaded;

private:
    struct InternalData {
        static constexpr int        NumStandardProperties = static_cast<int>(StandardProperty::Count);

        InternalData();

        void                        Purge();
        void                        Swap(InternalData &rhs);

        const ShaderPropertyDesc *  FindProperty(const char *name) const;
        int                         FindPropertyIndex(const char *name) const;
        int                         GetStandardPropertyIndex(StandardProperty standardProperty) const;
        bool                        AddProperty(const ShaderPropertyDesc &propertyDesc);
        bool                        CopyPropertiesFrom(const InternalData &rhs, const char **duplicatedPropertyName = nullptr);
        void                        AddStage(RHI::ShaderStage shaderStage, RHI::ShaderModel shaderModel, const char *filename, const char *entryPoint);
        void                        ResetStandardPropertyIndices();

        ShaderStageSource           stageSources[to_int(RHI::ShaderStage::Count)] = {};
        BE1::Str                    inheritedShaderTemplateName;
        BE1::StrArray               baseDefines;
        BE1::StrArray               featureDefines[to_int(ShaderFeature::Count)] = {};
        uint32_t                    supportedFeatureMask = 0;
        BE1::Array<ShaderPropertyDesc> properties;
        BE1::HashIndex              propertyHash { 64, 64 };
        int                         standardPropertyIndices[NumStandardProperties];
        Flag                        flags = Flag::None;
    };

    struct ShaderPermutation {
        BE1::StrArray               propertyDefines;
        ShaderFeatureSet            featureSet;
    };

    void                            Purge();

    static bool                     Prepare(const char *filename, InternalData &outInternalData);
    void                            ApplyPrepared(InternalData &newInternalData);

    bool                            Load(const char *filename);

    static bool                     Parse(const char *text, const char *baseDir, const char *shaderTemplateName, InternalData &outInternalData);
    static bool                     ParseInheritProperties(BE1::Lexer &lexer, const char *baseDir, const char *shaderTemplateName, InternalData &outInternalData);
    static bool                     ParseBaseDefines(BE1::Lexer &lexer, const char *shaderTemplateName, InternalData &outInternalData);
    static bool                     ParseFeatures(BE1::Lexer &lexer, const char *shaderTemplateName, InternalData &outInternalData);
    static bool                     ParseStages(BE1::Lexer &lexer, const char *baseDir, const char *shaderTemplateName, InternalData &outInternalData);
    static bool                     ParseProperties(BE1::Lexer &lexer, const char *shaderTemplateName, InternalData &outInternalData);

    const ShaderPermutation *       FindPermutation(ShaderPermutationKey permutationKey) const;
    void                            ClearPermutations();

                                    // featureSetToCheck 에 포함된 feature 의 define 중 같은 이름이 있는지 확인한다. 값은 비교하지 않는다.
    bool                            HasFeatureDefine(const ShaderFeatureSet &featureSetToCheck, const char *defineName) const;

                                    // featureSet 에 포함된 feature 의 define 을 수집해 outDefines 의 기존 항목 뒤에 추가한다.
    bool                            CollectFeatureDefines(const ShaderFeatureSet &featureSet, BE1::StrArray &outDefines) const;

    BE1::Str                        hashName;
    BE1::Str                        name;
    InternalData                    internalData;

    BE1::HashMap<uint64_t, ShaderPermutation *> permutationMap;
};
