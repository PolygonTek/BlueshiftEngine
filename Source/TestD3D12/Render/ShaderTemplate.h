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

    ShaderTemplate

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

                                    // ShaderPipeline 을 얻기 위해서는 필요한 permutation key 를 미리 등록해야 한다.
                                    // 등록된 permutation 은 PSO 빌드 시 compile define 목록을 복원하는 데 사용된다.
    ShaderPermutationKey            AddPermutation();
    ShaderPermutationKey            AddPermutation(const BE1::Array<PropertyValue> &propertyValues);
    ShaderPermutationKey            AddPermutation(const BE1::Array<PropertyValue> &propertyValues, const ShaderFeatureSet &featureSet);

    bool                            Create(const char *text, const char *baseDir);

    bool                            Load(const char *filename);
    bool                            Reload();

                                    // permutation key 로 ShaderPipeline 을 빌드 (PSO 생성) 할 수 있는 데이터를 준비한다.
    bool                            PreparePipelineBuildData(ShaderPermutationKey permutationKey, ShaderPipelineBuildData &outBuildData) const;

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
    static const bool enable = true;
};

template<>
struct enable_bitmask_operators<ShaderPropertyInfo::ShaderFlag> {
    static const bool enable = true;
};

class ShaderTemplateManager {
    friend class ShaderTemplate;

public:
    using ReloadedDelegate = BE1::MulticastDelegate<void(const ShaderTemplate *, const BE1::Array<ShaderPropertyInfo> &)>;

    void                            Init();
    void                            Shutdown();

    void                            UpdateFrame();

    SharedPtr<ShaderTemplate>       AllocShaderTemplate(const char *hashName);
    SharedPtr<ShaderTemplate>       FindShaderTemplate(const char *hashName) const;
    SharedPtr<ShaderTemplate>       GetShaderTemplate(const char *hashName);

    bool                            FindGlobalDefine(const char *define) const;
    void                            AddGlobalDefine(const char *define);
    void                            RemoveGlobalDefine(const char *define);
    uint32_t                        CopyGlobalDefines(BE1::StrArray &outDefines) const;

    ReloadedDelegate                shaderTemplateReloaded;

    SharedPtr<ShaderTemplate>       fullscreenImageShaderTemplate;

    SharedPtr<ShaderTemplate>       fftComplex256ShaderTemplate;
    SharedPtr<ShaderTemplate>       fftComplex512ShaderTemplate;
    SharedPtr<ShaderTemplate>       fftComplex1024ShaderTemplate;

    SharedPtr<ShaderTemplate>       fftReal512ShaderTemplate;
    SharedPtr<ShaderTemplate>       fftReal1024ShaderTemplate;

    SharedPtr<ShaderTemplate>       ifftReal512ShaderTemplate;
    SharedPtr<ShaderTemplate>       ifftReal1024ShaderTemplate;

    SharedPtr<ShaderTemplate>       fftDualReal256ShaderTemplate;
    SharedPtr<ShaderTemplate>       fftDualReal512ShaderTemplate;
    SharedPtr<ShaderTemplate>       fftDualReal1024ShaderTemplate;

    SharedPtr<ShaderTemplate>       ifftDualReal256ShaderTemplate;
    SharedPtr<ShaderTemplate>       ifftDualReal512ShaderTemplate;
    SharedPtr<ShaderTemplate>       ifftDualReal1024ShaderTemplate;

    SharedPtr<ShaderTemplate>       multiplyComplexSpectrum2DShaderTemplate;
    SharedPtr<ShaderTemplate>       multiplyPackedRealSpectrum2DShaderTemplate;
    SharedPtr<ShaderTemplate>       multiplyPackedDualRealSpectrum2DShaderTemplate;

    SharedPtr<ShaderTemplate>       transposeComplex256ShaderTemplate;
    SharedPtr<ShaderTemplate>       transposeComplex256x512ShaderTemplate;
    SharedPtr<ShaderTemplate>       transposeComplex512x256ShaderTemplate;
    SharedPtr<ShaderTemplate>       transposeComplex512x1024ShaderTemplate;
    SharedPtr<ShaderTemplate>       transposeComplex1024x512ShaderTemplate;
    SharedPtr<ShaderTemplate>       transposeComplex512ShaderTemplate;
    SharedPtr<ShaderTemplate>       transposeComplex1024ShaderTemplate;

    SharedPtr<ShaderTemplate>       bloomPrefilterShaderTemplate;
    SharedPtr<ShaderTemplate>       bloomDownsampleShaderTemplate;
    SharedPtr<ShaderTemplate>       bloomUpsampleShaderTemplate;
    SharedPtr<ShaderTemplate>       convolutionBloomPrefilterShaderTemplate;
    SharedPtr<ShaderTemplate>       convolutionBloomKernelPrepareShaderTemplate;
    SharedPtr<ShaderTemplate>       convolutionBloomResolveShaderTemplate;

    SharedPtr<ShaderTemplate>       luminancePass1ShaderTemplate;
    SharedPtr<ShaderTemplate>       luminancePass2ShaderTemplate;
    SharedPtr<ShaderTemplate>       toneMappingShaderTemplate;

    SharedPtr<ShaderTemplate>       unlitShaderTemplate;
    SharedPtr<ShaderTemplate>       litShaderTemplate;

private:
    void                            InitGlobalDefines();
    void                            CleanExpiredShaderTemplates();
    void                            CreateEngineShaderTemplates();
    void                            CreateFFTShaderTemplates(RHI::ShaderModel shaderModel, const char *baseDir);
    void                            CreateTransposeShaderTemplates(RHI::ShaderModel shaderModel, const char *baseDir);
    void                            CreateBloomShaderTemplates(RHI::ShaderModel shaderModel, const char *baseDir);
    void                            CreateToneMappingShaderTemplates(RHI::ShaderModel shaderModel, const char *baseDir);
    void                            DestroyEngineShaderTemplates();

    BE1::StrIHashMap<WeakPtr<ShaderTemplate>> shaderTemplateHashMap;
    int                             expiredShaderTemplateHashMapCursor = 0;
    BE1::StrArray                   globalDefines;
    uint32_t                        globalDefineModCount = 0;
    uint32_t                        lastInvalidatedGlobalDefineModCount = 0;
    BE1::PlatformMutex *            globalDefinesMutex = nullptr;
};

extern ShaderTemplateManager        shaderTemplateManager;
