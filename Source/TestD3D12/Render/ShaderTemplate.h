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

#include "Containers/HashMap.h"
#include "Core/Property.h"
#include "ShaderPipelineState.h"

class ShaderTemplateManager;
class LoadingSystem;

struct ShaderPropertyInfo : public BE1::PropertyInfo {
    enum class ShaderFlag : uint8_t {
        None                        = 0,
        ShaderDefine                = BIT(0),
        Hidden                      = BIT(1),
        Normal                      = BIT(2),
        Linear                      = BIT(3)
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
    bool                            Parse(BE1::Lexer &lexer);

    ShaderFlag                      shaderFlags = ShaderFlag::None;
    BE1::Array<Condition>           conditions;
};

/*
-------------------------------------------------------------------------------

    ShaderTemplate

-------------------------------------------------------------------------------
*/

class ShaderTemplate : public std::enable_shared_from_this<ShaderTemplate> {
    friend class ShaderTemplateManager;
    friend class LoadingSystem;

public:
    enum class Flag : uint8_t {
        None                        = 0,
        LoadedFromFile              = BIT(0)
    };

    using Feature = ShaderFeature;
    using PropertyValue = BE1::Variant;

    struct StageSource {
        RHI::ShaderStage            shaderStage = RHI::ShaderStage::Count;
        RHI::ShaderModel            shaderModel = RHI::ShaderModel::SM_6_0;
        BE1::Str                    filename;
        BE1::Str                    entryPoint;
    };

    struct PipelineBuildData {
        void                        Clear();

        StageSource                 stageSources[to_int(RHI::ShaderStage::Count)] = {};
        BE1::StrArray               compileDefines;
    };

    ShaderTemplate() = default;
    ShaderTemplate(const ShaderTemplate &rhs) = delete;
    ShaderTemplate &operator=(const ShaderTemplate &rhs) = delete;
    ~ShaderTemplate() { Purge(); }

    const char *                    GetName() const { return name.c_str(); }
    const char *                    GetHashName() const { return hashName.c_str(); }
    const BE1::Array<ShaderPropertyInfo> &GetProperties() const { return internalData.properties; }

    const ShaderPropertyInfo *      FindProperty(const char *name) const;
    bool                            AddProperty(const ShaderPropertyInfo &propertyInfo);
    bool                            AddProperty(const char *name, const char *label, BE1::Variant::Type::Enum type, const BE1::Variant &defaultValue, const char *desc = "", int flags = BE1::PropertyInfo::Flag::Editor, ShaderPropertyInfo::ShaderFlag shaderFlags = ShaderPropertyInfo::ShaderFlag::None);
    bool                            CopyPropertiesFrom(const ShaderTemplate &shaderTemplate);

    void                            InitDefaultPropertyValues(BE1::Array<PropertyValue> &outPropertyValues) const;

    void                            AddStage(RHI::ShaderStage shaderStage, RHI::ShaderModel shaderModel, const char *filename, const char *entryPoint);

                                    // permutation 추가, permutation 은 추가만 가능하고 수정은 불가능하다.
    ShaderPermutationKey            AddPermutation(std::initializer_list<const char *> defines);
    ShaderPermutationKey            AddPermutation(const BE1::StrArray &defines);
    ShaderPermutationKey            AddPermutation(std::initializer_list<const char *> defines, const ShaderFeatureSet &featureSet);
    ShaderPermutationKey            AddPermutation(const BE1::StrArray &defines, const ShaderFeatureSet &featureSet);
    ShaderPermutationKey            AddPermutation(const BE1::Array<PropertyValue> &propertyValues);
    ShaderPermutationKey            AddPermutation(const BE1::Array<PropertyValue> &propertyValues, const BE1::StrArray &defines);
    ShaderPermutationKey            AddPermutation(const BE1::Array<PropertyValue> &propertyValues, const ShaderFeatureSet &featureSet);
    ShaderPermutationKey            AddPermutation(const BE1::Array<PropertyValue> &propertyValues, const BE1::StrArray &defines, const ShaderFeatureSet &featureSet);

    bool                            Create(const char *text, const char *baseDir);

    bool                            Load(const char *filename);
    bool                            Reload();

    bool                            PreparePipelineBuildData(ShaderPermutationKey permutationKey, PipelineBuildData &outBuildData) const;

    std::shared_ptr<ShaderPipelineState> GetGraphicsPipeline(ShaderPermutationKey permutationKey, const ShaderPipelineDesc &pipelineDesc);
    std::shared_ptr<ShaderPipelineState> GetComputePipeline(ShaderPermutationKey permutationKey);

private:
    struct InternalData {
        void                        Purge();
        void                        Swap(InternalData &rhs);

        const ShaderPropertyInfo *  FindProperty(const char *name) const;
        bool                        AddProperty(const ShaderPropertyInfo &propertyInfo);
        bool                        CopyPropertiesFrom(const InternalData &rhs, const char **duplicatedPropertyName = nullptr);
        void                        AddStage(RHI::ShaderStage shaderStage, RHI::ShaderModel shaderModel, const char *filename, const char *entryPoint);

        StageSource                 stageSources[to_int(RHI::ShaderStage::Count)] = {};
        BE1::StrArray               baseDefines;
        BE1::StrArray               featureDefines[to_int(Feature::Count)] = {};
        BE1::Array<ShaderPropertyInfo> properties;
        Flag                        flags = Flag::None;
    };

    struct ShaderPermutation {
        ShaderPermutationKey        key;
        ShaderFeatureSet            featureSet;
        BE1::StrArray               defines;
    };

    struct CachedGraphicsPipeline {
        uint64_t                    hash = 0;
        std::shared_ptr<ShaderPipelineState> pipelineState;
    };

    struct CachedComputePipeline {
        ShaderPermutationKey        permutationKey;
        std::shared_ptr<ShaderPipelineState> pipelineState;
    };

    void                            Purge();
    void                            ApplyPrepared(InternalData &newInternalData);
    bool                            Prepare(const char *text, const char *baseDir, InternalData &outInternalData);
    bool                            ParseInheritProperties(BE1::Lexer &lexer, const char *baseDir, InternalData &outInternalData);
    bool                            ParseBaseDefines(BE1::Lexer &lexer, InternalData &outInternalData);
    bool                            ParseFeatures(BE1::Lexer &lexer, InternalData &outInternalData);
    bool                            ParseStages(BE1::Lexer &lexer, const char *baseDir, InternalData &outInternalData);
    bool                            ParseProperties(BE1::Lexer &lexer, InternalData &outInternalData);

    void                            ClearCachedPipelines();
    void                            InvalidateCachedPipelines();
    std::shared_ptr<ShaderPipelineState> FindCachedGraphicsPipeline(ShaderPermutationKey permutationKey, const ShaderPipelineDesc &pipelineDesc) const;
    std::shared_ptr<ShaderPipelineState> FindCachedComputePipeline(ShaderPermutationKey permutationKey) const;

    void                            ClearPermutations();
    ShaderPermutation *             FindPermutation(ShaderPermutationKey permutationKey) const;
    bool                            HasTemplateDefine(const char *define, const ShaderFeatureSet &featureSet) const;
    bool                            AppendFeatureSetDefines(BE1::StrArray &defines, const ShaderFeatureSet &featureSet) const;

    static bool                     PreparePipelinePSO(const ShaderPipelineState &pipelineState, const PipelineBuildData &buildData, RHI::PipelineState *&preparedPSO);
    static bool                     PrepareStageShader(const PipelineBuildData &buildData, RHI::ShaderStage shaderStage, const BE1::StrArray &compileDefines, RHI::Shader *&outShader);
    static bool                     PreparePipelineGraphicsShaders(const PipelineBuildData &buildData, RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)]);
    static bool                     PreparePipelineComputeShader(const PipelineBuildData &buildData, RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)]);
    static RHI::PipelineState *     CreateGraphicsPSO(const PipelineBuildData &buildData, const ShaderPipelineDesc &pipelineDesc, RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)]);
    static RHI::PipelineState *     CreateComputePSO(const PipelineBuildData &buildData, RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)]);

    static uint64_t                 GenerateInputLayoutHash(const RHI::InputLayout *inputLayout);
    static uint64_t                 GenerateGraphicsPipelineHash(ShaderPermutationKey permutationKey, const ShaderPipelineDesc &pipelineDesc);

    BE1::Str                        hashName;
    BE1::Str                        name;
    InternalData                    internalData;

    BE1::HashMap<uint64_t, ShaderPermutation *> permutationMap;
    BE1::Array<CachedGraphicsPipeline> cachedGraphicsPipelines;
    BE1::Array<CachedComputePipeline> cachedComputePipelines;
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
    void                            Init();
    void                            Shutdown();

    void                            UpdateFrame();

    std::shared_ptr<ShaderTemplate> AllocShaderTemplate(const char *hashName);
    std::shared_ptr<ShaderTemplate> FindShaderTemplate(const char *hashName) const;
    std::shared_ptr<ShaderTemplate> GetShaderTemplate(const char *hashName);

    bool                            FindGlobalDefine(const char *define) const;
    void                            AddGlobalDefine(const char *define);
    void                            RemoveGlobalDefine(const char *define);
    uint32_t                        CopyGlobalDefines(BE1::StrArray &outDefines) const;

    std::shared_ptr<ShaderTemplate> fullscreenImageShaderTemplate;
    std::shared_ptr<ShaderTemplate> unlitShaderTemplate;
    std::shared_ptr<ShaderTemplate> litShaderTemplate;

private:
    void                            InitGlobalDefines();
    void                            InvalidateShaderTemplateCaches();
    void                            CleanExpiredShaderTemplates();
    void                            CreateEngineShaderTemplates();
    void                            DestroyEngineShaderTemplates();

    BE1::StrIHashMap<std::weak_ptr<ShaderTemplate>> shaderTemplateHashMap;
    int                             expiredShaderTemplateHashMapCursor = 0;
    BE1::StrArray                   globalDefines;
    uint32_t                        globalDefineModCount = 0;
    uint32_t                        lastInvalidatedGlobalDefineModCount = 0;
    BE1::PlatformMutex *            globalDefinesMutex = nullptr;
};

extern ShaderTemplateManager        shaderTemplateManager;
