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
#include "ShaderTemplate.h"
#include "IO/FileSystem.h"

const ShaderPropertyInfo *ShaderTemplate::FindProperty(const char *name) const {
    return internalData.FindProperty(name);
}

void ShaderTemplate::PipelineBuildData::Clear() {
    for (StageSource &stageSource : stageSources) {
        stageSource = StageSource();
    }

    compileDefines.Clear();
}

void ShaderTemplate::InternalData::Purge() {
    for (StageSource &stageSource : stageSources) {
        stageSource = StageSource();
    }

    baseDefines.Clear();
    for (BE1::StrArray &defines : featureDefines) {
        defines.Clear();
    }
    properties.Clear();
}

void ShaderTemplate::InternalData::Swap(InternalData &rhs) {
    for (int i = 0; i < to_int(RHI::ShaderStage::Count); i++) {
        BE1::Swap(stageSources[i], rhs.stageSources[i]);
    }

    baseDefines.Swap(rhs.baseDefines);
    for (int i = 0; i < to_int(Feature::Count); i++) {
        featureDefines[i].Swap(rhs.featureDefines[i]);
    }
    properties.Swap(rhs.properties);
    BE1::Swap(flags, rhs.flags);
}

const ShaderPropertyInfo *ShaderTemplate::InternalData::FindProperty(const char *name) const {
    for (const ShaderPropertyInfo &propertyInfo : properties) {
        if (!BE1::Str::Cmp(propertyInfo.GetName(), name)) {
            return &propertyInfo;
        }
    }
    return nullptr;
}

bool ShaderTemplate::AddProperty(const char *name, const char *label, BE1::Variant::Type::Enum type, const BE1::Variant &defaultValue, const char *desc, int flags, ShaderPropertyInfo::ShaderFlag shaderFlags) {
    ShaderPropertyInfo propertyInfo(name, label, type, defaultValue, desc, flags, shaderFlags);
    if (!internalData.AddProperty(propertyInfo)) {
        BE_WARNLOG("ShaderTemplate::AddProperty: duplicated property name '%s' in shader template '%s'\n", propertyInfo.GetName(), hashName.c_str());
        return false;
    }

    return true;
}

bool ShaderTemplate::AddProperty(const ShaderPropertyInfo &propertyInfo) {
    if (!internalData.AddProperty(propertyInfo)) {
        BE_WARNLOG("ShaderTemplate::AddProperty: duplicated property name '%s' in shader template '%s'\n", propertyInfo.GetName(), hashName.c_str());
        return false;
    }

    return true;
}

bool ShaderTemplate::InternalData::AddProperty(const ShaderPropertyInfo &propertyInfo) {
    if (FindProperty(propertyInfo.GetName())) {
        assert(0);
        return false;
    }

    properties.Append(propertyInfo);
    return true;
}

bool ShaderTemplate::CopyPropertiesFrom(const ShaderTemplate &shaderTemplate) {
    const char *duplicatedPropertyName = nullptr;
    if (!internalData.CopyPropertiesFrom(shaderTemplate.internalData, &duplicatedPropertyName)) {
        BE_WARNLOG("ShaderTemplate::CopyPropertiesFrom: duplicated property name '%s' in shader template '%s'\n", duplicatedPropertyName ? duplicatedPropertyName : "", hashName.c_str());
        return false;
    }

    return true;
}

bool ShaderTemplate::InternalData::CopyPropertiesFrom(const InternalData &rhs, const char **duplicatedPropertyName) {
    for (const ShaderPropertyInfo &propertyInfo : rhs.properties) {
        if (FindProperty(propertyInfo.GetName())) {
            assert(0);
            if (duplicatedPropertyName) {
                *duplicatedPropertyName = propertyInfo.GetName();
            }
            return false;
        }
    }

    for (const ShaderPropertyInfo &propertyInfo : rhs.properties) {
        properties.Append(propertyInfo);
    }

    return true;
}

void ShaderTemplate::InitDefaultPropertyValues(BE1::Array<PropertyValue> &outPropertyValues) const {
    outPropertyValues.Clear();

    for (const ShaderPropertyInfo &propertyInfo : internalData.properties) {
        outPropertyValues.Append(propertyInfo.GetDefaultValue());
    }
}

void ShaderTemplate::AddStage(RHI::ShaderStage shaderStage, RHI::ShaderModel shaderModel, const char *filename, const char *entryPoint) {
    assert(shaderStage >= RHI::ShaderStage::Vertex && shaderStage < RHI::ShaderStage::Count);

    StageSource &stageSource = internalData.stageSources[to_int(shaderStage)];
    assert(stageSource.shaderStage == RHI::ShaderStage::Count);

    internalData.AddStage(shaderStage, shaderModel, filename, entryPoint);
}

void ShaderTemplate::InternalData::AddStage(RHI::ShaderStage shaderStage, RHI::ShaderModel shaderModel, const char *filename, const char *entryPoint) {
    assert(shaderStage >= RHI::ShaderStage::Vertex && shaderStage < RHI::ShaderStage::Count);

    StageSource &stageSource = stageSources[to_int(shaderStage)];

    stageSource.shaderStage = shaderStage;
    stageSource.shaderModel = shaderModel;
    stageSource.filename = filename;
    stageSource.entryPoint = entryPoint;
}

// 같은 이름의 define 이 있는지 검사한다. define name 만 비교하며, value 는 비교하지 않는다.
static bool HasDefine(const BE1::StrArray &defines, const char *keyDefine) {
    if (!keyDefine || !keyDefine[0] || defines.Count() == 0) {
        return false;
    }

    int keyDefineNameLength = 0;
    while (keyDefine[keyDefineNameLength] && keyDefine[keyDefineNameLength] != '=') {
        keyDefineNameLength++;
    }

    for (const BE1::Str &define : defines) {
        const int equalIndex = define.Find('=');
        const int defineNameLength = equalIndex >= 0 ? equalIndex : define.Length();

        if (defineNameLength == keyDefineNameLength && !BE1::Str::Cmpn(define.c_str(), keyDefine, keyDefineNameLength)) {
            return true;
        }
    }

    return false;
}

static void AddDefine(BE1::StrArray &defines, const char *newDefineName, int value) {
    if (!newDefineName || !newDefineName[0]) {
        return;
    }

    BE1::Str newDefine = newDefineName;
    newDefine.Append("=");
    newDefine.Append(BE1::Str(value));
    defines.Append(newDefine);
}

static bool AddDefine(BE1::StrArray &defines, const char *newDefine) {
    if (!newDefine || !newDefine[0]) {
        return false;
    }

    if (HasDefine(defines, newDefine)) {
        return false;
    }

    defines.Append(newDefine);
    return true;
}

static bool AppendDefines(BE1::StrArray &defines, const BE1::StrArray &appendDefines) {
    bool addedAllDefines = true;

    for (const BE1::Str &define : appendDefines) {
        if (!AddDefine(defines, define.c_str())) {
            addedAllDefines = false;
            continue;
        }
    }

    return addedAllDefines;
}

ShaderPermutationKey ShaderTemplate::AddPermutation(std::initializer_list<const char *> defines) {
    return AddPermutation(defines, ShaderFeatureSet());
}

ShaderPermutationKey ShaderTemplate::AddPermutation(const BE1::StrArray &defines) {
    return AddPermutation(defines, ShaderFeatureSet());
}

ShaderPermutationKey ShaderTemplate::AddPermutation(std::initializer_list<const char *> defines, const ShaderFeatureSet &featureSet) {
    BE1::StrArray permutationDefines;

    for (const char *define : defines) {
        if (define && define[0]) {
            permutationDefines.Append(define);
        }
    }

    return AddPermutation(permutationDefines, featureSet);
}

ShaderPermutationKey ShaderTemplate::AddPermutation(const BE1::StrArray &defines, const ShaderFeatureSet &featureSet) {
    BE1::StrArray permutationDefines;

    for (const BE1::Str &define : defines) {
        if (HasTemplateDefine(define.c_str(), featureSet)) {
            BE_ERRLOG("ShaderTemplate::AddPermutation: permutation define name '%s' conflicts with a template define in shader template '%s'\n", define.c_str(), hashName.c_str());
            continue;
        }

        if (!AddDefine(permutationDefines, define.c_str())) {
            BE_ERRLOG("ShaderTemplate::AddPermutation: duplicated permutation define name '%s' in shader template '%s'\n", define.c_str(), hashName.c_str());
            continue;
        }
    }
    ShaderPermutationKey::NormalizePermutationDefines(permutationDefines);

    ShaderPermutationKey permutationKey = ShaderPermutationKey::GeneratePermutationKey(permutationDefines, featureSet);
    ShaderPermutation *permutation = FindPermutation(permutationKey);
    if (permutation) {
        return permutationKey;
    }

    permutation = new ShaderPermutation;
    permutation->key = permutationKey;
    permutation->featureSet = featureSet;
    permutation->defines = permutationDefines;
    permutationMap.Set(permutationKey.hash, permutation);
    return permutationKey;
}

ShaderPermutationKey ShaderTemplate::AddPermutation(const BE1::Array<PropertyValue> &propertyValues) {
    return AddPermutation(propertyValues, BE1::StrArray(), ShaderFeatureSet());
}

ShaderPermutationKey ShaderTemplate::AddPermutation(const BE1::Array<PropertyValue> &propertyValues, const BE1::StrArray &defines) {
    return AddPermutation(propertyValues, defines, ShaderFeatureSet());
}

ShaderPermutationKey ShaderTemplate::AddPermutation(const BE1::Array<PropertyValue> &propertyValues, const ShaderFeatureSet &featureSet) {
    return AddPermutation(propertyValues, BE1::StrArray(), featureSet);
}

ShaderPermutationKey ShaderTemplate::AddPermutation(const BE1::Array<PropertyValue> &propertyValues, const BE1::StrArray &defines, const ShaderFeatureSet &featureSet) {
    BE1::StrArray permutationDefines;

    for (const BE1::Str &define : defines) {
        if (HasTemplateDefine(define.c_str(), featureSet)) {
            BE_ERRLOG("ShaderTemplate::AddPermutation: permutation define name '%s' conflicts with a template define in shader template '%s'\n", define.c_str(), hashName.c_str());
            continue;
        }

        if (!AddDefine(permutationDefines, define.c_str())) {
            BE_ERRLOG("ShaderTemplate::AddPermutation: duplicated permutation define name '%s' in shader template '%s'\n", define.c_str(), hashName.c_str());
            continue;
        }
    }

    for (int i = 0; i < internalData.properties.Count(); i++) {
        const ShaderPropertyInfo &propertyInfo = internalData.properties[i];
        if (!propertyInfo.IsShaderDefine()) {
            continue;
        }

        const char *propertyDefineName = propertyInfo.GetName();
        if (HasTemplateDefine(propertyDefineName, featureSet) || HasDefine(permutationDefines, propertyDefineName)) {
            // permutation 의 define name 과 프로퍼티의 define name 은 중복되면 안된다.
            BE_ERRLOG("ShaderTemplate::AddPermutation: permutation define name '%s' conflicts with an existing shader property in shader template '%s'\n", propertyDefineName, hashName.c_str());
            continue;
        }

        const PropertyValue *propertyValue = i < propertyValues.Count() ? &propertyValues[i] : nullptr;
        const BE1::Variant &value = propertyValue ? *propertyValue : propertyInfo.GetDefaultValue();

        switch (propertyInfo.GetType()) {
        case BE1::Variant::Type::Bool:
            AddDefine(permutationDefines, propertyDefineName, value.As<bool>() ? 1 : 0);
            break;
        case BE1::Variant::Type::Int:
            AddDefine(permutationDefines, propertyDefineName, value.As<int>());
            break;
        default:
            BE_WARNLOG("ShaderTemplate::AddPermutation: shader define property '%s' has unsupported type\n", propertyDefineName);
            break;
        }
    }

    ShaderPermutationKey::NormalizePermutationDefines(permutationDefines);

    ShaderPermutationKey permutationKey = ShaderPermutationKey::GeneratePermutationKey(permutationDefines, featureSet);
    ShaderPermutation *permutation = FindPermutation(permutationKey);
    if (permutation) {
        return permutationKey;
    }

    permutation = new ShaderPermutation;
    permutation->key = permutationKey;
    permutation->featureSet = featureSet;
    permutation->defines = permutationDefines;
    permutationMap.Set(permutationKey.hash, permutation);
    return permutationKey;
}

bool ShaderTemplate::Create(const char *text, const char *baseDir) {
    InternalData newInternalData;

    if (!Prepare(text, baseDir, newInternalData)) {
        return false;
    }

    ApplyPrepared(newInternalData);
    return true;
}

bool ShaderTemplate::Prepare(const char *text, const char *baseDir, InternalData &outInternalData) {
    outInternalData.Purge();

    BE1::Lexer lexer;
    lexer.Init(BE1::Lexer::Flag::NoErrors);
    lexer.Load(text, BE1::Str::Length(text), hashName);

    // '{'
    if (!lexer.ExpectPunctuation(BE1::Lexer::PuncType::BraceOpen)) {
        return false;
    }

    BE1::Str token;
    bool closed = false;
    while (lexer.ReadToken(&token)) {
        if (token.IsEmpty()) {
            break;
        } else if (token[0] == '}') {
            closed = true;
            break;
        } else if (!token.Icmp("inheritProperties")) {
            if (!ParseInheritProperties(lexer, baseDir, outInternalData)) {
                return false;
            }
        } else if (!token.Icmp("baseDefines")) {
            if (!ParseBaseDefines(lexer, outInternalData)) {
                return false;
            }
        } else if (!token.Icmp("features")) {
            if (!ParseFeatures(lexer, outInternalData)) {
                return false;
            }
        } else if (!token.Icmp("stages")) {
            if (!ParseStages(lexer, baseDir, outInternalData)) {
                return false;
            }
        } else if (!token.Icmp("properties")) {
            if (!ParseProperties(lexer, outInternalData)) {
                return false;
            }
        } else {
            BE_WARNLOG("unknown token '%s' in shader template '%s'\n", token.c_str(), hashName.c_str());
            return false;
        }
    }

    if (!closed) {
        BE_WARNLOG("ShaderTemplate::Create: missing closing brace in shader template '%s'\n", hashName.c_str());
        return false;
    }

    bool hasShaderStage = false;
    for (const StageSource &stageSource : outInternalData.stageSources) {
        if (stageSource.shaderStage != RHI::ShaderStage::Count) {
            hasShaderStage = true;
            break;
        }
    }

    if (!hasShaderStage) {
        BE_WARNLOG("ShaderTemplate::Create: shader template '%s' has no shader stages\n", hashName.c_str());
        return false;
    }

    return true;
}

static bool ParseShaderStage(const BE1::Str &token, RHI::ShaderStage &outShaderStage) {
    struct StageName {
        const char *stageName;
        RHI::ShaderStage shaderStage;
    };

    static constexpr StageName StageNames[] = {
        { "vertex",     RHI::ShaderStage::Vertex },
        { "vs",         RHI::ShaderStage::Vertex },
        { "hull",       RHI::ShaderStage::Hull },
        { "hs",         RHI::ShaderStage::Hull },
        { "domain",     RHI::ShaderStage::Domain },
        { "ds",         RHI::ShaderStage::Domain },
        { "geometry",   RHI::ShaderStage::Geometry },
        { "gs",         RHI::ShaderStage::Geometry },
        { "fragment",   RHI::ShaderStage::Fragment },
        { "pixel",      RHI::ShaderStage::Fragment },
        { "ps",         RHI::ShaderStage::Fragment },
        { "compute",    RHI::ShaderStage::Compute },
        { "cs",         RHI::ShaderStage::Compute }
    };

    for (const StageName &stageName : StageNames) {
        if (!token.Icmp(stageName.stageName)) {
            outShaderStage = stageName.shaderStage;
            return true;
        }
    }

    return false;
}

static bool ParseShaderModel(const BE1::Str &token, RHI::ShaderModel &outShaderModel) {
    struct ModelName {
        const char *modelName;
        RHI::ShaderModel shaderModel;
    };

    static constexpr ModelName ModelNames[] = {
        { "SM_5_0", RHI::ShaderModel::SM_5_0 },
        { "SM_6_0", RHI::ShaderModel::SM_6_0 },
        { "SM_6_1", RHI::ShaderModel::SM_6_1 },
        { "SM_6_2", RHI::ShaderModel::SM_6_2 },
        { "SM_6_3", RHI::ShaderModel::SM_6_3 },
        { "SM_6_4", RHI::ShaderModel::SM_6_4 },
        { "SM_6_5", RHI::ShaderModel::SM_6_5 },
        { "SM_6_6", RHI::ShaderModel::SM_6_6 },
        { "SM_6_7", RHI::ShaderModel::SM_6_7 }
    };

    for (const ModelName &modelName : ModelNames) {
        if (!token.Icmp(modelName.modelName)) {
            outShaderModel = modelName.shaderModel;
            return true;
        }
    }

    return false;
}

static BE1::Str ResolveShaderStageFilename(const char *baseDir, const char *filename) {
    if (!baseDir || !baseDir[0] || !BE1::FileSystem::IsRelativePath(filename)) {
        return filename;
    }

    BE1::Str resolvedFilename = baseDir;
    resolvedFilename.AppendPath(filename, '/');

    return resolvedFilename;
}

bool ShaderTemplate::ParseInheritProperties(BE1::Lexer &lexer, const char *baseDir, InternalData &outInternalData) {
    BE1::Str inheritedName;
    if (!lexer.ExpectTokenType(BE1::Lexer::TokenType::String, &inheritedName)) {
        return false;
    }

    std::shared_ptr<ShaderTemplate> inheritedShaderTemplate = shaderTemplateManager.GetShaderTemplate(inheritedName.c_str());
    if (!inheritedShaderTemplate && baseDir && baseDir[0] && BE1::FileSystem::IsRelativePath(inheritedName.c_str())) {
        BE1::Str resolvedName = baseDir;
        resolvedName.AppendPath(inheritedName, '/');
        inheritedShaderTemplate = shaderTemplateManager.GetShaderTemplate(resolvedName.c_str());
    }

    if (!inheritedShaderTemplate) {
        BE_WARNLOG("ShaderTemplate::ParseInheritProperties: shader template '%s' was not found while parsing shader template '%s'\n", inheritedName.c_str(), hashName.c_str());
        return false;
    }

    const char *duplicatedPropertyName = nullptr;
    if (!outInternalData.CopyPropertiesFrom(inheritedShaderTemplate->internalData, &duplicatedPropertyName)) {
        BE_WARNLOG("ShaderTemplate::ParseInheritProperties: duplicated property name '%s' in shader template '%s'\n", duplicatedPropertyName ? duplicatedPropertyName : "", hashName.c_str());
        return false;
    }

    return true;
}

bool ShaderTemplate::ParseBaseDefines(BE1::Lexer &lexer, InternalData &outInternalData) {
    BE1::Str token;
    bool closed = false;

    if (!lexer.ExpectPunctuation(BE1::Lexer::PuncType::BraceOpen)) {
        return false;
    }

    while (lexer.ReadToken(&token)) {
        if (token[0] == '}') {
            closed = true;
            break;
        }

        if (token == "," || token == ";") {
            continue;
        }

        BE1::Str define = token;
        BE1::Str nextToken;
        if (lexer.ReadToken(&nextToken, false)) {
            if (nextToken == "=") {
                BE1::Str valueToken;
                if (!lexer.ReadToken(&valueToken, false)) {
                    BE_WARNLOG("ShaderTemplate::ParseBaseDefines: missing value for base define '%s' in shader template '%s'\n", define.c_str(), hashName.c_str());
                    return false;
                }
                define.Append("=");
                define.Append(valueToken);
            } else {
                lexer.UnreadToken(&nextToken);
            }
        }

        if (HasDefine(outInternalData.baseDefines, define.c_str())) {
            BE_WARNLOG("ShaderTemplate::ParseBaseDefines: duplicated base define name '%s' in shader template '%s'\n", define.c_str(), hashName.c_str());
            return false;
        }

        outInternalData.baseDefines.Append(define);
    }

    if (!closed) {
        BE_WARNLOG("ShaderTemplate::ParseBaseDefines: missing closing brace in shader template '%s'\n", hashName.c_str());
        return false;
    }

    return true;
}

static bool ParseShaderFeature(const BE1::Str &token, ShaderTemplate::Feature &outFeature) {
    struct FeatureName {
        const char *featureName;
        ShaderTemplate::Feature feature;
    };

    static constexpr FeatureName FeatureNames[] = {
        { "perforated",         ShaderTemplate::Feature::Perforated },
        { "gpuSkinningWeight1", ShaderTemplate::Feature::GpuSkinningWeight1 },
        { "gpuSkinningWeight4", ShaderTemplate::Feature::GpuSkinningWeight4 },
        { "gpuSkinningWeight8", ShaderTemplate::Feature::GpuSkinningWeight8 },
        { "gpuInstancing",      ShaderTemplate::Feature::GpuInstancing }
    };

    for (const FeatureName &featureName : FeatureNames) {
        if (!token.Icmp(featureName.featureName)) {
            outFeature = featureName.feature;
            return true;
        }
    }

    return false;
}

bool ShaderTemplate::ParseFeatures(BE1::Lexer &lexer, InternalData &outInternalData) {
    BE1::Str token;
    bool closed = false;
    bool declaredFeatures[to_int(Feature::Count)] = {};

    if (!lexer.ExpectPunctuation(BE1::Lexer::PuncType::BraceOpen)) {
        return false;
    }

    while (lexer.ReadToken(&token)) {
        if (token[0] == '}') {
            closed = true;
            break;
        }

        Feature feature = Feature::Count;
        if (!ParseShaderFeature(token, feature)) {
            BE_WARNLOG("ShaderTemplate::ParseFeatures: unknown feature '%s' in shader template '%s'\n", token.c_str(), hashName.c_str());
            return false;
        }

        int featureIndex = to_int(feature);
        if (declaredFeatures[featureIndex]) {
            BE_WARNLOG("ShaderTemplate::ParseFeatures: duplicated feature '%s' in shader template '%s'\n", token.c_str(), hashName.c_str());
            return false;
        }
        declaredFeatures[featureIndex] = true;

        BE1::StrArray &featureDefines = outInternalData.featureDefines[featureIndex];

        bool featureClosed = false;
        if (!lexer.ExpectPunctuation(BE1::Lexer::PuncType::BraceOpen)) {
            return false;
        }

        BE1::Str defineToken;
        while (lexer.ReadToken(&defineToken)) {
            if (defineToken[0] == '}') {
                featureClosed = true;
                break;
            }

            if (defineToken == "," || defineToken == ";") {
                continue;
            }

            BE1::Str define = defineToken;
            BE1::Str nextToken;
            if (lexer.ReadToken(&nextToken, false)) {
                if (nextToken == "=") {
                    BE1::Str valueToken;
                    if (!lexer.ReadToken(&valueToken, false)) {
                        BE_WARNLOG("ShaderTemplate::ParseFeatures: missing value for feature define '%s' in shader template '%s'\n", define.c_str(), hashName.c_str());
                        return false;
                    }
                    define.Append("=");
                    define.Append(valueToken);
                } else {
                    lexer.UnreadToken(&nextToken);
                }
            }

            if (HasDefine(featureDefines, define.c_str())) {
                BE_WARNLOG("ShaderTemplate::ParseFeatures: duplicated feature define name '%s' in shader template '%s'\n", define.c_str(), hashName.c_str());
                return false;
            }

            featureDefines.Append(define);
        }

        if (!featureClosed) {
            BE_WARNLOG("ShaderTemplate::ParseFeatures: missing closing brace in feature '%s' in shader template '%s'\n", token.c_str(), hashName.c_str());
            return false;
        }
    }

    if (!closed) {
        BE_WARNLOG("ShaderTemplate::ParseFeatures: missing closing brace in shader template '%s'\n", hashName.c_str());
        return false;
    }

    return true;
}

bool ShaderTemplate::ParseStages(BE1::Lexer &lexer, const char *baseDir, InternalData &outInternalData) {
    BE1::Str token;
    bool closed = false;

    if (!lexer.ExpectPunctuation(BE1::Lexer::PuncType::BraceOpen)) {
        return false;
    }

    while (lexer.ReadToken(&token)) {
        if (token[0] == '}') {
            closed = true;
            break;
        }

        if (!token.Icmp("stage")) {
            if (!lexer.ReadToken(&token, false)) {
                return false;
            }
        }

        RHI::ShaderStage shaderStage = RHI::ShaderStage::Count;
        if (!ParseShaderStage(token, shaderStage)) {
            BE_WARNLOG("unknown shader stage '%s' in shader template '%s'\n", token.c_str(), hashName.c_str());
            return false;
        }

        BE1::Str shaderModelName;
        if (!lexer.ReadToken(&shaderModelName, false)) {
            return false;
        }

        RHI::ShaderModel shaderModel = RHI::ShaderModel::SM_6_0;
        if (!ParseShaderModel(shaderModelName, shaderModel)) {
            BE_WARNLOG("unknown shader model '%s' in shader template '%s'\n", shaderModelName.c_str(), hashName.c_str());
            return false;
        }

        BE1::Str filename;
        if (!lexer.ExpectTokenType(BE1::Lexer::TokenType::String, &filename)) {
            return false;
        }

        BE1::Str entryPoint;
        if (!lexer.ReadToken(&entryPoint, false)) {
            return false;
        }

        BE1::Str resolvedFilename = ResolveShaderStageFilename(baseDir, filename.c_str());
        outInternalData.AddStage(shaderStage, shaderModel, resolvedFilename.c_str(), entryPoint.c_str());
    }

    if (!closed) {
        BE_WARNLOG("ShaderTemplate::ParseStages: missing closing brace in shader template '%s'\n", hashName.c_str());
        return false;
    }

    return true;
}

bool ShaderTemplate::ParseProperties(BE1::Lexer &lexer, InternalData &outInternalData) {
    BE1::Str token;
    bool closed = false;

    if (!lexer.ExpectPunctuation(BE1::Lexer::PuncType::BraceOpen)) {
        return false;
    }

    while (lexer.ReadToken(&token)) {
        if (token[0] == '}') {
            closed = true;
            break;
        } else {
            if (outInternalData.FindProperty(token)) {
                assert(0);
                BE_WARNLOG("duplicated property name '%s' in shader template '%s'\n", token.c_str(), hashName.c_str());
                return false;
            }

            lexer.UnreadToken(&token);

            ShaderPropertyInfo shaderPropInfo;
            if (!shaderPropInfo.Parse(lexer)) {
                BE_WARNLOG("error occurred in parsing property propInfo in shader '%s'\n", hashName.c_str());
                lexer.SkipRestOfLine();
                continue;
            }

            if (!outInternalData.AddProperty(shaderPropInfo)) {
                BE_WARNLOG("duplicated property name '%s' in shader template '%s'\n", shaderPropInfo.GetName(), hashName.c_str());
                return false;
            }
        }
    }

    if (!closed) {
        BE_WARNLOG("ShaderTemplate::ParseProperties: missing closing brace in shader template '%s'\n", hashName.c_str());
        return false;
    }

    return true;
}

bool ShaderPropertyInfo::Parse(BE1::Lexer &lexer) {
    type = BE1::Variant::Type::None;
    flags = BE1::PropertyInfo::Flag::Editor;
    shaderFlags = ShaderFlag::None;
    range = BE1::Rangef(0, 0, 1);
    metaObject = nullptr;

    if (!lexer.ReadToken(&name, false)) {
        return false;
    }

    if (!lexer.ExpectPunctuation(BE1::Lexer::PuncType::ParenthesesOpen)) {
        return false;
    }

    if (!lexer.ExpectTokenType(BE1::Lexer::TokenType::String, &label)) {
        return false;
    }

    if (!lexer.ExpectPunctuation(BE1::Lexer::PuncType::ParenthesesClose)) {
        return false;
    }

    if (!lexer.ExpectPunctuation(BE1::Lexer::PuncType::Colon)) {
        return false;
    }

    BE1::Str typeStr;
    if (!lexer.ReadToken(&typeStr, false)) {
        return false;
    }

    if (!BE1::Str::Cmp(typeStr, "bool")) {
        type = BE1::Variant::Type::Bool;
    } else if (!BE1::Str::Cmp(typeStr, "int")) {
        type = BE1::Variant::Type::Int;
    } else if (!BE1::Str::Cmp(typeStr, "point")) {
        type = BE1::Variant::Type::Point;
    } else if (!BE1::Str::Cmp(typeStr, "size")) {
        type = BE1::Variant::Type::Size;
    } else if (!BE1::Str::Cmp(typeStr, "rect")) {
        type = BE1::Variant::Type::Rect;
    } else if (!BE1::Str::Cmp(typeStr, "float")) {
        type = BE1::Variant::Type::Float;
    } else if (!BE1::Str::Cmp(typeStr, "vec2")) {
        type = BE1::Variant::Type::Vec2;
    } else if (!BE1::Str::Cmp(typeStr, "vec3")) {
        type = BE1::Variant::Type::Vec3;
    } else if (!BE1::Str::Cmp(typeStr, "vec4")) {
        type = BE1::Variant::Type::Vec4;
    } else if (!BE1::Str::Cmp(typeStr, "color3")) {
        type = BE1::Variant::Type::Color3;
    } else if (!BE1::Str::Cmp(typeStr, "color4")) {
        type = BE1::Variant::Type::Color4;
    } else if (!BE1::Str::Cmp(typeStr, "enum")) {
        BE1::Str enumSequence;
        if (!lexer.ExpectTokenType(BE1::Lexer::TokenType::String, &enumSequence)) {
            return false;
        }
        type = BE1::Variant::Type::Int;
        enumeration.Clear();
        SplitStringIntoList(enumeration, enumSequence, ";");
    } else if (!BE1::Str::Cmp(typeStr, "texture")) {
        type = BE1::Variant::Type::Guid;

        BE1::Str textureTypeStr;
        if (!lexer.ReadToken(&textureTypeStr, false)) {
            return false;
        }

        /*if (!BE1::Str::Icmp(textureTypeStr, "2D")) {
            metaObject = &Texture2DResource::metaObject;
        } else if (!BE1::Str::Icmp(textureTypeStr, "CUBE")) {
            metaObject = &TextureCubeMapResource::metaObject;
        } else if (!BE1::Str::Icmp(textureTypeStr, "SPRITE")) {
            metaObject = &TextureSpriteResource::metaObject;
        } else {
            BE_WARNLOG("unknown texture type %s in shader '%s'\n", textureTypeStr.c_str(), lexer.GetFilename());
            metaObject = &Texture2DResource::metaObject;
        }*/
    } else {
        BE_WARNLOG("unknown property type '%s' in shader template '%s'\n", typeStr.c_str(), lexer.GetFilename());
        return false;
    }

    if (type == BE1::Variant::Type::Int ||
        type == BE1::Variant::Type::Float ||
        type == BE1::Variant::Type::Vec2 ||
        type == BE1::Variant::Type::Vec3 ||
        type == BE1::Variant::Type::Vec4) {
        BE1::Str token;
        lexer.ReadToken(&token, false);

        if (token == "range") {
            range.minValue = lexer.ParseNumber();
            range.maxValue = lexer.ParseNumber();
            range.step = lexer.ParseNumber();
        } else {
            if (!token.IsEmpty()) {
                lexer.UnreadToken(&token);
            }
        }
    }

    if (!lexer.ExpectPunctuation(BE1::Lexer::PuncType::Assign)) {
        return false;
    }

    BE1::Str defaultValueString;
    if (!lexer.ExpectTokenType(BE1::Lexer::TokenType::String, &defaultValueString)) {
        return false;
    }

    if (!BE1::Str::Cmp(typeStr, "texture")) {
        defaultValue = BE1::resourceGuidMapper.Get(defaultValueString);
    } else {
        defaultValue = BE1::Variant::FromString(type, defaultValueString);
    }

    BE1::Str token;
    lexer.ReadToken(&token, false);
    if (token == "(") {
        bool closed = false;
        while (lexer.ReadToken(&token, false)) {
            if (token == ")") {
                closed = true;
                break;
            } else if (token == "condition") {
                BE1::Str conditionName;
                BE1::Array<int> valueList;
                bool conditionClosed = false;

                while (lexer.ReadToken(&token, false)) {
                    if (token == ")") {
                        conditions.Append(ShaderPropertyInfo::Condition(conditionName, valueList));

                        lexer.UnreadToken(&token);
                        conditionClosed = true;
                        break;
                    } else if (lexer.GetTokenType() & BE1::Lexer::TokenType::Identifier) {
                        conditionName = token;
                    } else if (lexer.GetTokenType() & BE1::Lexer::TokenType::Integer) {
                        valueList.Append((int)BE1::Str::ToI32(token));
                    } else {
                        BE_WARNLOG("unknown condition token '%s' in shader property '%s'\n", token.c_str(), name.c_str());
                        return false;
                    }
                }

                if (!conditionClosed) {
                    BE_WARNLOG("missing closing parenthesis in shader property '%s'\n", this->name.c_str());
                    return false;
                }
            } else {
                BE_WARNLOG("unknown property option '%s' in shader property '%s'\n", token.c_str(), name.c_str());
                return false;
            }
        }

        if (!closed) {
            BE_WARNLOG("missing closing parenthesis in shader property '%s'\n", name.c_str());
            return false;
        }
    } else {
        if (!token.IsEmpty()) {
            lexer.UnreadToken(&token);
        }
    }

    lexer.ReadToken(&token, false);
    if (token == "[") {
        bool closed = false;
        while (lexer.ReadToken(&token, false)) {
            if (token == "]") {
                closed = true;
                break;
            } else if (token == "shaderDefine") {
                shaderFlags |= ShaderPropertyInfo::ShaderFlag::ShaderDefine;
            } else if (token == "hidden") {
                shaderFlags |= ShaderPropertyInfo::ShaderFlag::Hidden;
            } else if (token == "normal") {
                shaderFlags |= ShaderPropertyInfo::ShaderFlag::Normal;
            } else if (token == "linear") {
                shaderFlags |= ShaderPropertyInfo::ShaderFlag::Linear;
            } else {
                BE_WARNLOG("unknown shader property flag '%s' in shader property '%s'\n", token.c_str(), name.c_str());
                return false;
            }
        }

        if (!closed) {
            BE_WARNLOG("missing closing bracket in shader property '%s'\n", name.c_str());
            return false;
        }
    } else {
        if (!token.IsEmpty()) {
            lexer.UnreadToken(&token);
        }
    }

    return true;
}

bool ShaderTemplate::Load(const char *filename) {
    BE1::Str exactFilename = filename;
    exactFilename.DefaultFileExtension(".shader");

    char *data;
    size_t size = BE1::fileSystem.LoadFile(exactFilename, true, (void **)&data);
    if (!data) {
        return false;
    }

    BE1::Lexer lexer;
    lexer.Init(BE1::Lexer::Flag::NoErrors);
    lexer.Load(data, (int)size, exactFilename);

    if (!lexer.ExpectTokenString("shader")) {
        BE1::fileSystem.FreeFile(data);
        return false;
    }

    BE1::Str name;
    if (!lexer.ReadToken(&name)) {
        BE1::fileSystem.FreeFile(data);
        return false;
    }

    BE1::Str baseDir = exactFilename;
    baseDir.StripFileName();
    if (!Create(data + lexer.GetCurrentOffset(), baseDir)) {
        BE1::fileSystem.FreeFile(data);
        return false;
    }

    BE1::fileSystem.FreeFile(data);

    internalData.flags |= ShaderTemplate::Flag::LoadedFromFile;

    return true;
}

bool ShaderTemplate::Reload() {
    if (!BE1::HasFlag(internalData.flags, Flag::LoadedFromFile)) {
        BE_WARNLOG("ShaderTemplate::Reload: cannot reload shader template '%s' because it was not loaded from a file\n", GetHashName());
        return false;
    }

    return Load(hashName);
}

std::shared_ptr<ShaderPipelineState> ShaderTemplate::FindCachedGraphicsPipeline(ShaderPermutationKey permutationKey, const ShaderPipelineDesc &pipelineDesc) const {
    uint64_t hash = GenerateGraphicsPipelineHash(permutationKey, pipelineDesc);

    for (const CachedGraphicsPipeline &cachedPipeline : cachedGraphicsPipelines) {
        if (cachedPipeline.hash == hash) {
            return cachedPipeline.pipelineState;
        }
    }

    return nullptr;
}

std::shared_ptr<ShaderPipelineState> ShaderTemplate::FindCachedComputePipeline(ShaderPermutationKey permutationKey) const {
    for (const CachedComputePipeline &cachedPipeline : cachedComputePipelines) {
        if (cachedPipeline.permutationKey == permutationKey) {
            return cachedPipeline.pipelineState;
        }
    }

    return nullptr;
}

std::shared_ptr<ShaderPipelineState> ShaderTemplate::GetGraphicsPipeline(ShaderPermutationKey permutationKey, const ShaderPipelineDesc &pipelineDesc) {
    if (!FindPermutation(permutationKey)) {
        BE_WARNLOG("ShaderTemplate::GetGraphicsPipeline: shader template '%s' has no permutation 0x%llx\n", GetHashName(), permutationKey.hash);
        return nullptr;
    }

    std::shared_ptr<ShaderPipelineState> pipelineState = FindCachedGraphicsPipeline(permutationKey, pipelineDesc);
    if (!pipelineState) {
        uint64_t hash = GenerateGraphicsPipelineHash(permutationKey, pipelineDesc);

        CachedGraphicsPipeline &cachedPipeline = cachedGraphicsPipelines.Alloc();
        cachedPipeline.hash = hash;
        cachedPipeline.pipelineState = std::shared_ptr<ShaderPipelineState>(new ShaderPipelineState(ShaderPipelineState::Type::Graphics, permutationKey, pipelineDesc));

        pipelineState = cachedPipeline.pipelineState;
    }

    if (pipelineState->GetState() == ShaderPipelineState::State::Empty) {
        pipelineState->EnsureBuildAsync(shared_from_this());
    }

    return pipelineState;
}

std::shared_ptr<ShaderPipelineState> ShaderTemplate::GetComputePipeline(ShaderPermutationKey permutationKey) {
    std::shared_ptr<ShaderPipelineState> pipelineState = FindCachedComputePipeline(permutationKey);
    if (!pipelineState) {
        CachedComputePipeline &cachedPipeline = cachedComputePipelines.Alloc();
        cachedPipeline.permutationKey = permutationKey;
        cachedPipeline.pipelineState = std::shared_ptr<ShaderPipelineState>(new ShaderPipelineState(ShaderPipelineState::Type::Compute, permutationKey));

        pipelineState = cachedPipeline.pipelineState;
    }

    if (!pipelineState->IsReady()) {
        pipelineState->EnsureBuildAsync(shared_from_this());
    }

    return pipelineState;
}

RHI::PipelineState *ShaderTemplate::CreateGraphicsPSO(const PipelineBuildData &buildData, const ShaderPipelineDesc &pipelineDesc, RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)]) {
    if (!ShaderTemplate::PreparePipelineGraphicsShaders(buildData, stageShaders)) {
        return nullptr;
    }

    RHI::PipelineStateDesc psoDesc;
    psoDesc.vs = stageShaders[to_int(RHI::ShaderStage::Vertex)];
    psoDesc.ps = stageShaders[to_int(RHI::ShaderStage::Fragment)];
    psoDesc.ds = stageShaders[to_int(RHI::ShaderStage::Domain)];
    psoDesc.hs = stageShaders[to_int(RHI::ShaderStage::Hull)];
    psoDesc.gs = stageShaders[to_int(RHI::ShaderStage::Geometry)];
    psoDesc.inputLayout = pipelineDesc.inputLayout;
    psoDesc.rasterizerState = RHI::renderer->GetRasterizerState(pipelineDesc.rasterizerStateType);
    psoDesc.depthStencilState = RHI::renderer->GetDepthStencilState(pipelineDesc.depthStencilStateType);
    psoDesc.blendState = RHI::renderer->GetBlendState(pipelineDesc.blendStateType);
    psoDesc.renderDest = &pipelineDesc.renderDest;
    psoDesc.primitiveTopology = pipelineDesc.primitiveTopology;
    psoDesc.sampleMask = pipelineDesc.sampleMask;

    return RHI::renderer->CreateGraphicsPSO(&psoDesc);
}

RHI::PipelineState *ShaderTemplate::CreateComputePSO(const PipelineBuildData &buildData, RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)]) {
    if (!ShaderTemplate::PreparePipelineComputeShader(buildData, stageShaders)) {
        return nullptr;
    }

    RHI::Shader *computeShader = stageShaders[to_int(RHI::ShaderStage::Compute)];
    return RHI::renderer->CreateComputePSO(computeShader);
}

void ShaderTemplate::Purge() {
    ClearCachedPipelines();
    ClearPermutations();

    internalData.Purge();
}

void ShaderTemplate::ApplyPrepared(InternalData &newInternalData) {
    internalData.Swap(newInternalData);

    InvalidateCachedPipelines();
}

void ShaderTemplate::ClearCachedPipelines() {
    for (CachedGraphicsPipeline &cachedPipeline : cachedGraphicsPipelines) {
        if (cachedPipeline.pipelineState) {
            cachedPipeline.pipelineState->Reset();
        }
    }
    cachedGraphicsPipelines.Clear();

    for (CachedComputePipeline &cachedPipeline : cachedComputePipelines) {
        if (cachedPipeline.pipelineState) {
            cachedPipeline.pipelineState->Reset();
        }
    }
    cachedComputePipelines.Clear();
}

void ShaderTemplate::InvalidateCachedPipelines() {
    std::shared_ptr<ShaderTemplate> self = shared_from_this();

    for (CachedGraphicsPipeline &cachedPipeline : cachedGraphicsPipelines) {
        if (cachedPipeline.pipelineState) {
            cachedPipeline.pipelineState->BuildAsync(self);
        }
    }

    for (CachedComputePipeline &cachedPipeline : cachedComputePipelines) {
        if (cachedPipeline.pipelineState) {
            cachedPipeline.pipelineState->BuildAsync(self);
        }
    }
}

void ShaderTemplate::ClearPermutations() {
    permutationMap.DeleteContents(true);
}

ShaderTemplate::ShaderPermutation *ShaderTemplate::FindPermutation(ShaderPermutationKey permutationKey) const {
    const auto *entry = permutationMap.Get(permutationKey.hash);
    if (entry) {
        return entry->second;
    }
    return nullptr;
}

// baseDefines 나 featureDefines 에 define 이 있는지 확인한다.
bool ShaderTemplate::HasTemplateDefine(const char *define, const ShaderFeatureSet &featureSet) const {
    if (HasDefine(internalData.baseDefines, define)) {
        return true;
    }

    for (int i = 0; i < to_int(ShaderFeature::Count); i++) {
        ShaderFeature feature = static_cast<ShaderFeature>(i);
        if (!featureSet.HasFeature(feature)) {
            continue;
        }

        if (HasDefine(internalData.featureDefines[i], define)) {
            return true;
        }
    }

    return false;
}

bool ShaderTemplate::AppendFeatureSetDefines(BE1::StrArray &defines, const ShaderFeatureSet &featureSet) const {
    bool addedAllDefines = true;

    for (int i = 0; i < to_int(ShaderFeature::Count); i++) {
        ShaderFeature feature = static_cast<ShaderFeature>(i);
        if (!featureSet.HasFeature(feature)) {
            continue;
        }

        for (const BE1::Str &define : internalData.featureDefines[i]) {
            if (!AddDefine(defines, define.c_str())) {
                BE_ERRLOG("ShaderTemplate::AppendFeatureSetDefines: feature define name '%s' conflicts with an existing define in shader template '%s'\n", define.c_str(), hashName.c_str());
                addedAllDefines = false;
                continue;
            }
        }
    }

    return addedAllDefines;
}

bool ShaderTemplate::PreparePipelineBuildData(ShaderPermutationKey permutationKey, PipelineBuildData &outBuildData) const {
    outBuildData.Clear();

    // 원본 permutation 을 얻는다.
    ShaderPermutation *srcPermutation = FindPermutation(permutationKey);
    if (!srcPermutation) {
        BE_WARNLOG("ShaderTemplate::PreparePipelineBuildData: shader template '%s' has no permutation 0x%llx\n", GetHashName(), permutationKey.hash);
        return false;
    }

    // global defines + base defines + permutation defines + feature defines 순으로 컴파일 define 을 만든다.
    shaderTemplateManager.CopyGlobalDefines(outBuildData.compileDefines);
    AppendDefines(outBuildData.compileDefines, internalData.baseDefines);
    AppendDefines(outBuildData.compileDefines, srcPermutation->defines);
    if (!AppendFeatureSetDefines(outBuildData.compileDefines, srcPermutation->featureSet)) {
        return false;
    }

    ShaderPermutationKey::NormalizePermutationDefines(outBuildData.compileDefines);

    for (int i = 0; i < to_int(RHI::ShaderStage::Count); i++) {
        outBuildData.stageSources[i] = internalData.stageSources[i];
    }

    return true;
}

bool ShaderTemplate::PreparePipelinePSO(const ShaderPipelineState &pipelineState, const PipelineBuildData &buildData, RHI::PipelineState *&preparedPSO) {
    preparedPSO = nullptr;

    RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)] = {};

    switch (pipelineState.GetType()) {
    case ShaderPipelineState::Type::Graphics:
        preparedPSO = ShaderTemplate::CreateGraphicsPSO(buildData, pipelineState.GetGraphicsDesc(), stageShaders);
        break;
    case ShaderPipelineState::Type::Compute:
        preparedPSO = ShaderTemplate::CreateComputePSO(buildData, stageShaders);
        break;
    default:
        assert(0);
        break;
    }

    for (int i = 0; i < to_int(RHI::ShaderStage::Count); ++i) {
        if (stageShaders[i]) {
            RHI::renderer->DestroyShader(stageShaders[i], true);
            stageShaders[i] = nullptr;
        }
    }
    return preparedPSO != nullptr;
}

bool ShaderTemplate::PrepareStageShader(const PipelineBuildData &buildData, RHI::ShaderStage shaderStage, const BE1::StrArray &compileDefines, RHI::Shader *&outShader) {
    assert(shaderStage < RHI::ShaderStage::Count);

    const StageSource &stageSource = buildData.stageSources[to_int(shaderStage)];
    if (stageSource.shaderStage == RHI::ShaderStage::Count) {
        return false;
    }

    outShader = RHI::renderer->CreateShaderFromFile(stageSource.shaderModel, stageSource.shaderStage, stageSource.filename, stageSource.entryPoint, compileDefines.Count() > 0 ? &compileDefines : nullptr);
    if (!outShader) {
        BE_WARNLOG("ShaderTemplate::PrepareStageShader: failed to compile shader '%s'\n", stageSource.filename.c_str());
        return false;
    }
    return true;
}

bool ShaderTemplate::PreparePipelineGraphicsShaders(const PipelineBuildData &buildData, RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)]) {
    static constexpr RHI::ShaderStage GraphicsShaderStages[] = {
        RHI::ShaderStage::Vertex,
        RHI::ShaderStage::Hull,
        RHI::ShaderStage::Domain,
        RHI::ShaderStage::Geometry,
        RHI::ShaderStage::Fragment
    };

    bool hasGraphicsStage = false;

    for (RHI::ShaderStage shaderStage : GraphicsShaderStages) {
        const StageSource &stageSource = buildData.stageSources[to_int(shaderStage)];
        if (stageSource.shaderStage == RHI::ShaderStage::Count) {
            continue;
        }

        hasGraphicsStage = true;

        RHI::Shader *&stageShader = stageShaders[to_int(shaderStage)];
        if (!stageShader) {
            if (!PrepareStageShader(buildData, shaderStage, buildData.compileDefines, stageShader)) {
                return false;
            }
        }
    }

    if (!hasGraphicsStage) {
        BE_WARNLOG("ShaderTemplate::PreparePipelineGraphicsShaders: no graphics shader stages\n");
        return false;
    }

    return true;
}

bool ShaderTemplate::PreparePipelineComputeShader(const PipelineBuildData &buildData, RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)]) {
    const StageSource &stageSource = buildData.stageSources[to_int(RHI::ShaderStage::Compute)];
    if (stageSource.shaderStage == RHI::ShaderStage::Count) {
        return false;
    }

    RHI::Shader *&stageShader = stageShaders[to_int(RHI::ShaderStage::Compute)];
    if (!stageShader) {
        if (!PrepareStageShader(buildData, RHI::ShaderStage::Compute, buildData.compileDefines, stageShader)) {
            BE_WARNLOG("ShaderTemplate::PreparePipelineComputeShader: no compute shader stage\n");
            return false;
        }
    }

    return true;
}

uint64_t ShaderTemplate::GenerateInputLayoutHash(const RHI::InputLayout *inputLayout) {
    if (!inputLayout) {
        return 0;
    }

    size_t hash = 0;
    for (const RHI::InputLayoutElement &element : inputLayout->elements) {
        hash = hash_combine(hash, element);
    }

    return static_cast<uint64_t>(hash);
}

uint64_t ShaderTemplate::GenerateGraphicsPipelineHash(ShaderPermutationKey permutationKey, const ShaderPipelineDesc &pipelineDesc) {
    struct PipelineHashData {
        uint64_t    permutationHash = 0;
        uint64_t    inputLayoutHash = 0;
        uint64_t    renderDestHash = 0;
        uint8_t     rasterizerStateType = 0;
        uint8_t     depthStencilStateType = 0;
        uint8_t     blendStateType = 0;
        uint8_t     primitiveTopology = 0;
        uint32_t    sampleMask = 0;
    } hashData;

    hashData.permutationHash = permutationKey.hash;
    hashData.inputLayoutHash = GenerateInputLayoutHash(pipelineDesc.inputLayout);
    hashData.renderDestHash = pipelineDesc.renderDest.GetHash();
    hashData.rasterizerStateType = static_cast<uint8_t>(pipelineDesc.rasterizerStateType);
    hashData.depthStencilStateType = static_cast<uint8_t>(pipelineDesc.depthStencilStateType);
    hashData.blendStateType = static_cast<uint8_t>(pipelineDesc.blendStateType);
    hashData.primitiveTopology = static_cast<uint8_t>(pipelineDesc.primitiveTopology);
    hashData.sampleMask = pipelineDesc.sampleMask;

    return BE1::CityHash64(reinterpret_cast<const char *>(&hashData), sizeof(hashData));
}
