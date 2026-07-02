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
#include "Core/Property.h"
#include "ShaderPipeline.h"

class ShaderTemplateManager;

struct ShaderPropertyInfo : public BE1::PropertyInfo {
    enum class ShaderFlag : uint8_t {
        None                        = 0,
        ShaderDefine                = BIT(0),
        Hidden                      = BIT(1),
        Normal                      = BIT(2),
        Linear                      = BIT(3),
        NoStreaming                 = BIT(4),
        NoMipmaps                   = BIT(5)
    };

    enum class TextureType : uint8_t {
        None,
        Texture2D,
        TextureCube
    };

    struct Condition {
        Condition() = default;
        Condition(const char *name, const BE1::Array<int> &valueList) : name(name), valueList(valueList) {}

        BE1::Str                    name;
        BE1::Array<int>             valueList;
    };

    ShaderPropertyInfo() = default;
    ShaderPropertyInfo(const char *name, const char *label, BE1::Variant::Type::Enum type, const BE1::Variant &defaultValue, const char *desc, int flags, ShaderFlag shaderFlags = ShaderFlag::None)
        : BE1::PropertyInfo(name, label, type, intptr_t(0), defaultValue, desc, flags), shaderFlags(shaderFlags) {}

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

    .shader 파일에서 읽은 stage source, base define, feature define, property 정보를 보관하는 템플릿이다.
    shader define property 값과 featureSet 조합은 RegisterPermutation 함수로 등록되어 ShaderPermutationKey 로 관리된다.
    ShaderPipeline 빌드 시 PrepareShaderCompileData 는 key 로 등록된 permutation 을 찾아 compile define 과 stage source 를 ShaderCompileData 로 만든다.

---------------------------------------------------------------------------------------------------
*/

class ShaderTemplate : public std::enable_shared_from_this<ShaderTemplate> {
    friend class ShaderTemplateManager;

public:
    enum class Flag : uint8_t {
        None                        = 0,
        LitSurface                  = BIT(1),
        SkySurface                  = BIT(2),
        LoadedFromFile              = BIT(3)
    };

    enum class BuiltInProperty : uint8_t {
        BaseColorTexture,
        SkyCubeMap,
        SkyBackMap,
        SkyFrontMap,
        SkyRightMap,
        SkyLeftMap,
        SkyTopMap,
        SkyDownMap,
        Count
    };

    using Feature = ShaderFeature;
    using PropertyValue = BE1::Variant;

    ShaderTemplate() = default;
    ShaderTemplate(const ShaderTemplate &rhs) = delete;
    ShaderTemplate &operator=(const ShaderTemplate &rhs) = delete;
    ~ShaderTemplate() { Purge(); }

    const char *                    GetName() const { return name.c_str(); }
    const char *                    GetHashName() const { return hashName.c_str(); }

    const BE1::Array<ShaderPropertyInfo> &GetProperties() const { return internalData.properties; }
    bool                            IsSkySurface() const { return BE1::HasFlag(internalData.flags, Flag::SkySurface); }

    const ShaderPropertyInfo *      FindProperty(const char *name) const { return internalData.FindProperty(name); }
    int                             FindPropertyIndex(const char *name) const { return internalData.FindPropertyIndex(name); }
    int                             GetBuiltInPropertyIndex(BuiltInProperty builtInProperty) const { return internalData.GetBuiltInPropertyIndex(builtInProperty); }
    bool                            AddProperty(const ShaderPropertyInfo &propertyInfo);
    bool                            AddProperty(const char *name, const char *label, BE1::Variant::Type::Enum type, const BE1::Variant &defaultValue, const char *desc = "", int flags = BE1::PropertyInfo::Flag::Editor, ShaderPropertyInfo::ShaderFlag shaderFlags = ShaderPropertyInfo::ShaderFlag::None);
    bool                            CopyPropertiesFrom(const ShaderTemplate &shaderTemplate);

                                    // 초기값 배열을 만들어 준다.
    void                            InitDefaultPropertyValues(BE1::Array<PropertyValue> &outPropertyValues) const;

                                    // 지원하는 feature 인지 검사
    bool                            SupportsFeature(Feature feature) const;

                                    // 지원하지 않는 features 는 거른다.
    ShaderFeatureSet                MaskSupportedFeatures(const ShaderFeatureSet &featureSet) const;

    bool                            Create(const char *text, const char *baseDir);

    bool                            Load(const char *filename);
    bool                            Reload();

                                    // shader define property 값과 featureSet 조합을 permutation 으로 등록하고 key 를 리턴한다.
                                    // 반환된 key 는 ShaderPipelineCache 의 캐시 키이자, PrepareShaderCompileData 에서 등록된 compile define/feature define 조합을 찾기 위한 핸들이다.
    ShaderPermutationKey            RegisterPermutation();
    ShaderPermutationKey            RegisterPermutation(const BE1::Array<PropertyValue> &propertyValues);
    ShaderPermutationKey            RegisterPermutation(const BE1::Array<PropertyValue> &propertyValues, const ShaderFeatureSet &featureSet);

                                    // permutation key 로 ShaderPipeline 빌드에 필요한 shader compile data 를 준비한다.
    bool                            PrepareShaderCompileData(ShaderPermutationKey permutationKey, ShaderCompileData &outCompileData) const;

private:
    struct InternalData {
        static constexpr int        NumBuiltInProperties = static_cast<int>(BuiltInProperty::Count);

        InternalData();

        void                        Purge();
        void                        Swap(InternalData &rhs);

        const ShaderPropertyInfo *  FindProperty(const char *name) const;
        int                         FindPropertyIndex(const char *name) const;
        int                         GetBuiltInPropertyIndex(BuiltInProperty builtInProperty) const;
        bool                        AddProperty(const ShaderPropertyInfo &propertyInfo);
        bool                        CopyPropertiesFrom(const InternalData &rhs, const char **duplicatedPropertyName = nullptr);
        void                        AddStage(RHI::ShaderStage shaderStage, RHI::ShaderModel shaderModel, const char *filename, const char *entryPoint);
        void                        ResetBuiltInPropertyIndices();
        void                        RefreshBuiltInPropertyIndices();

        ShaderStageSource           stageSources[to_int(RHI::ShaderStage::Count)] = {};
        BE1::StrArray               baseDefines;
        BE1::StrArray               featureDefines[to_int(Feature::Count)] = {};
        BE1::Array<ShaderPropertyInfo> properties;
        BE1::HashIndex              propertyHash { 64, 64 };
        int                         builtInPropertyIndices[NumBuiltInProperties];
        Flag                        flags = Flag::None;
    };

    struct ShaderPermutation {
        ShaderPermutationKey        key;
        BE1::StrArray               defines;        // permutation defines (shader define properties)
        ShaderFeatureSet            featureSet;
    };

    void                            Purge();

    bool                            Parse(const char *text, const char *baseDir, InternalData &outInternalData);
    bool                            ParseInheritProperties(BE1::Lexer &lexer, const char *baseDir, InternalData &outInternalData);
    bool                            ParseBaseDefines(BE1::Lexer &lexer, InternalData &outInternalData);
    bool                            ParseFeatures(BE1::Lexer &lexer, InternalData &outInternalData);
    bool                            ParseStages(BE1::Lexer &lexer, const char *baseDir, InternalData &outInternalData);
    bool                            ParseProperties(BE1::Lexer &lexer, InternalData &outInternalData);

    ShaderPermutation *             FindPermutation(ShaderPermutationKey permutationKey) const;
    void                            ClearPermutations();

    bool                            HasTemplateDefine(const char *define, const ShaderFeatureSet &featureSet) const;
    bool                            AppendFeatureSetDefines(BE1::StrArray &defines, const ShaderFeatureSet &featureSet) const;

    BE1::Str                        hashName;
    BE1::Str                        name;
    InternalData                    internalData;

    mutable BE1::HashMap<uint64_t, ShaderPermutation *> permutationMap;
};

template<>
struct enable_bitmask_operators<ShaderTemplate::Flag> {
    static constexpr bool enable = true;
};

template<>
struct enable_bitmask_operators<ShaderPropertyInfo::ShaderFlag> {
    static constexpr bool enable = true;
};
