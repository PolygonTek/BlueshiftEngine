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
#include "ShaderTemplateManager.h"
#include "ShaderPipelineCache.h"
#include "Asset/GuidMapper.h"
#include "IO/FileSystem.h"

static constexpr const char *StandardPropertyNames[] = {
    "alphaCutoff",
    "baseColorTexture",
    "normalTexture",
    "shadowTexture",
    "skyCubeMap",
    "skyBackMap",
    "skyFrontMap",
    "skyRightMap",
    "skyLeftMap",
    "skyTopMap",
    "skyDownMap"
};
static_assert(COUNT_OF(StandardPropertyNames) == static_cast<int>(ShaderTemplate::StandardProperty::Count));

ShaderTemplate::InternalData::InternalData() {
    ResetStandardPropertyIndices();
}

void ShaderTemplate::InternalData::Purge() {
    for (ShaderStageSource &stageSource : stageSources) {
        stageSource = ShaderStageSource();
    }

    baseDefines.Clear();
    for (BE1::StrArray &defines : featureDefines) {
        defines.Clear();
    }
    properties.Clear();
    propertyHash.Clear(64, 64);
    ResetStandardPropertyIndices();
}

void ShaderTemplate::InternalData::Swap(InternalData &rhs) {
    for (int i = 0; i < to_int(RHI::ShaderStage::Count); i++) {
        BE1::Swap(stageSources[i], rhs.stageSources[i]);
    }

    baseDefines.Swap(rhs.baseDefines);
    for (int i = 0; i < to_int(ShaderFeature::Count); i++) {
        featureDefines[i].Swap(rhs.featureDefines[i]);
    }
    properties.Swap(rhs.properties);
    propertyHash.Swap(rhs.propertyHash);
    for (int i = 0; i < NumStandardProperties; ++i) {
        BE1::Swap(standardPropertyIndices[i], rhs.standardPropertyIndices[i]);
    }
    BE1::Swap(flags, rhs.flags);
}

const ShaderPropertyInfo *ShaderTemplate::InternalData::FindProperty(const char *name) const {
    int propertyIndex = FindPropertyIndex(name);
    if (propertyIndex >= 0) {
        return &properties[propertyIndex];
    }
    return nullptr;
}

int ShaderTemplate::InternalData::FindPropertyIndex(const char *name) const {
    if (!name || !name[0]) {
        return -1;
    }

    int hash = propertyHash.GenerateHash(name, true);
    for (int index = propertyHash.First(hash); index != -1; index = propertyHash.Next(index)) {
        if (!BE1::Str::Cmp(properties[index].GetName(), name)) {
            return index;
        }
    }

    return -1;
}

int ShaderTemplate::InternalData::GetStandardPropertyIndex(StandardProperty standardProperty) const {
    const int standardPropertyIndex = static_cast<int>(standardProperty);
    assert(standardPropertyIndex >= 0 && standardPropertyIndex < NumStandardProperties);

    return standardPropertyIndices[standardPropertyIndex];
}

void ShaderTemplate::InternalData::ResetStandardPropertyIndices() {
    for (int i = 0; i < NumStandardProperties; ++i) {
        standardPropertyIndices[i] = -1;
    }
}

void ShaderTemplate::InternalData::RefreshStandardPropertyIndices() {
    ResetStandardPropertyIndices();

    for (int i = 0; i < NumStandardProperties; ++i) {
        standardPropertyIndices[i] = FindPropertyIndex(StandardPropertyNames[i]);
    }
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

    int propertyIndex = properties.Append(propertyInfo);
    int hash = propertyHash.GenerateHash(propertyInfo.GetName(), true);
    propertyHash.Add(hash, propertyIndex);
    RefreshStandardPropertyIndices();
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
        AddProperty(propertyInfo);
    }

    return true;
}

void ShaderTemplate::InitDefaultPropertyValues(BE1::Array<PropertyValue> &outPropertyValues) const {
    outPropertyValues.Clear();

    for (const ShaderPropertyInfo &propertyInfo : internalData.properties) {
        outPropertyValues.Append(propertyInfo.GetDefaultValue());
    }
}

void ShaderTemplate::InternalData::AddStage(RHI::ShaderStage shaderStage, RHI::ShaderModel shaderModel, const char *filename, const char *entryPoint) {
    assert(shaderStage >= RHI::ShaderStage::Vertex && shaderStage < RHI::ShaderStage::Count);

    ShaderStageSource &stageSource = stageSources[to_int(shaderStage)];

    stageSource.shaderModel = shaderModel;
    stageSource.filename = filename;
    stageSource.entryPoint = entryPoint;
}

bool ShaderTemplate::SupportsFeature(ShaderFeature feature) const {
    if (feature >= ShaderFeature::Count) {
        return false;
    }
    return internalData.featureDefines[to_int(feature)].Count() > 0;
}

ShaderFeatureSet ShaderTemplate::MaskSupportedFeatures(const ShaderFeatureSet &featureSet) const {
    ShaderFeatureSet supportedFeatureSet;

    for (int featureIndex = 0; featureIndex < to_int(ShaderFeature::Count); featureIndex++) {
        ShaderFeature feature = static_cast<ShaderFeature>(featureIndex);

        if (featureSet.HasFeature(feature) && SupportsFeature(feature)) {
            supportedFeatureSet.EnableFeature(feature);
        }
    }

    return supportedFeatureSet;
}

// 같은 이름의 define 이 있는지 검사한다. define name 만 비교하고 value 는 비교하지 않는다.
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

ShaderPermutationKey ShaderTemplate::RegisterPermutation() {
    return RegisterPermutation(BE1::Array<PropertyValue>(), ShaderFeatureSet());
}

ShaderPermutationKey ShaderTemplate::RegisterPermutation(const BE1::Array<PropertyValue> &propertyValues) {
    return RegisterPermutation(propertyValues, ShaderFeatureSet());
}

ShaderPermutationKey ShaderTemplate::RegisterPermutation(const BE1::Array<PropertyValue> &propertyValues, const ShaderFeatureSet &featureSet) {
    BE1::StrArray propertyDefines;

    // 프로퍼티 중에 shader define 프로퍼티로 define 목록을 만든다.
    const int numProperties = internalData.properties.Count();
    for (int i = 0; i < numProperties; i++) {
        const ShaderPropertyInfo &propertyInfo = internalData.properties[i];
        if (!propertyInfo.IsShaderDefine()) {
            continue;
        }

        const char *propertyDefineName = propertyInfo.GetName();
        if (HasDefine(internalData.baseDefines, propertyDefineName) || HasFeatureSetDefine(featureSet, propertyDefineName) || HasDefine(propertyDefines, propertyDefineName)) {
            // permutation 의 define name 과 프로퍼티의 define name 은 중복되면 안된다.
            BE_ERRLOG("ShaderTemplate::RegisterPermutation: permutation define name '%s' conflicts with an existing shader property in shader template '%s'\n", propertyDefineName, hashName.c_str());
            continue;
        }

        const PropertyValue *propertyValue = i < propertyValues.Count() ? &propertyValues[i] : nullptr;
        const BE1::Variant &value = propertyValue ? *propertyValue : propertyInfo.GetDefaultValue();

        switch (propertyInfo.GetType()) {
        case BE1::Variant::Type::Bool:
            AddDefine(propertyDefines, propertyDefineName, value.As<bool>() ? 1 : 0);
            break;
        case BE1::Variant::Type::Int:
            AddDefine(propertyDefines, propertyDefineName, value.As<int>());
            break;
        default:
            BE_WARNLOG("ShaderTemplate::RegisterPermutation: shader define property '%s' has unsupported type\n", propertyDefineName);
            break;
        }
    }

    ShaderPermutationKey::NormalizeDefines(propertyDefines);

    ShaderPermutationKey permutationKey = ShaderPermutationKey::GeneratePermutationKey(propertyDefines, featureSet);
    ShaderPermutation *permutation = FindPermutation(permutationKey);
    if (permutation) {
        return permutationKey;
    }

    permutation = new ShaderPermutation;
    permutation->key = permutationKey;
    permutation->featureSet = featureSet;
    permutation->propertyDefines = propertyDefines;
    permutationMap.Set(permutationKey.hash, permutation);
    return permutationKey;
}

bool ShaderTemplate::Create(const char *text, const char *baseDir) {
    InternalData newInternalData;

    if (!Parse(text, baseDir, newInternalData)) {
        return false;
    }

    Purge();

    internalData.Swap(newInternalData);

    // default property 값 + empty featureSet 조합으로 permutation 을 등록한다.
    RegisterPermutation();
    return true;
}

bool ShaderTemplate::Parse(const char *text, const char *baseDir, InternalData &outInternalData) {
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
        } else if (!token.Icmp("litSurface")) {
            outInternalData.flags |= Flag::LitSurface;
        } else if (!token.Icmp("skySurface")) {
            outInternalData.flags |= Flag::SkySurface;
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
        BE_WARNLOG("ShaderTemplate::Parse: missing closing brace in shader template '%s'\n", hashName.c_str());
        return false;
    }

    bool hasShaderStage = false;
    for (const ShaderStageSource &stageSource : outInternalData.stageSources) {
        if (!stageSource.filename.IsEmpty()) {
            hasShaderStage = true;
            break;
        }
    }

    if (!hasShaderStage) {
        BE_WARNLOG("ShaderTemplate::Parse: shader template '%s' has no shader stages\n", hashName.c_str());
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

    SharedPtr<ShaderTemplate> inheritedShaderTemplate = shaderTemplateManager.GetShaderTemplate(inheritedName.c_str());
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

static bool ParseShaderFeature(const BE1::Str &token, ShaderFeature &outFeature) {
    struct FeatureName {
        const char *featureName;
        ShaderFeature feature;
    };

    static constexpr FeatureName FeatureNames[] = {
        { "perforated",         ShaderFeature::Perforated },
        { "gpuSkinningWeight1", ShaderFeature::GpuSkinningWeight1 },
        { "gpuSkinningWeight4", ShaderFeature::GpuSkinningWeight4 },
        { "gpuSkinningWeight8", ShaderFeature::GpuSkinningWeight8 },
        { "outputNormal",       ShaderFeature::OutputNormal },
        { "outputVelocity",     ShaderFeature::OutputVelocity }
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
    bool declaredFeatures[to_int(ShaderFeature::Count)] = {};

    if (!lexer.ExpectPunctuation(BE1::Lexer::PuncType::BraceOpen)) {
        return false;
    }

    while (lexer.ReadToken(&token)) {
        if (token[0] == '}') {
            closed = true;
            break;
        }

        ShaderFeature feature = ShaderFeature::Count;
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
    textureType = TextureType::None;
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

        if (!textureTypeStr.Icmp("2D")) {
            textureType = TextureType::Texture2D;
        } else if (!textureTypeStr.Icmp("CUBE")) {
            textureType = TextureType::TextureCube;
        } else {
            BE_WARNLOG("unknown texture type '%s' in shader property '%s'\n", textureTypeStr.c_str(), name.c_str());
            return false;
        }
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
        defaultValue = BE1::resourceGuidMapper.GetOrCreateGuid(defaultValueString);
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
            } else if (token == "noStreaming") {
                shaderFlags |= ShaderPropertyInfo::ShaderFlag::NoStreaming;
            } else if (token == "noMipmaps") {
                shaderFlags |= ShaderPropertyInfo::ShaderFlag::NoMipmaps;
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

    BE1::Array<ShaderPropertyInfo> oldProperties = internalData.properties;

    if (!Load(hashName)) {
        return false;
    }

    // 외부 material shader template reload 대응용 알림이다.
    // Reload 중 Purge 로 pipeline cache 는 이미 비워졌으므로 material 은 필요할 때 GetGraphicsPipeline 를 통해 새 pipeline 을 다시 얻는다.
    shaderTemplateManager.shaderTemplateReloaded(this, oldProperties);

    return true;
}

void ShaderTemplate::Purge() {
    // 파이프라인 캐시에서 이 ShaderTemplate 을 사용하는 모든 ShaderPipeline 캐시를 제거한다.
    shaderPipelineCache.ClearShaderTemplatePipelines(this);

    ClearPermutations();

    internalData.Purge();
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

// featureSet 에 해당하는 featureDefines 에 define 이 있는지 확인한다.
bool ShaderTemplate::HasFeatureSetDefine(const ShaderFeatureSet &featureSet, const char *define) const {
    for (int featureIndex = 0; featureIndex < to_int(ShaderFeature::Count); featureIndex++) {
        ShaderFeature feature = static_cast<ShaderFeature>(featureIndex);
        if (!featureSet.HasFeature(feature)) {
            continue;
        }

        if (HasDefine(internalData.featureDefines[featureIndex], define)) {
            return true;
        }
    }

    return false;
}

bool ShaderTemplate::AppendFeatureSetDefines(BE1::StrArray &outDefines, const ShaderFeatureSet &featureSet) const {
    bool addedAllDefines = true;

    for (int featureIndex = 0; featureIndex < to_int(ShaderFeature::Count); featureIndex++) {
        ShaderFeature feature = static_cast<ShaderFeature>(featureIndex);
        if (!featureSet.HasFeature(feature)) {
            continue;
        }

        for (const BE1::Str &define : internalData.featureDefines[featureIndex]) {
            if (!AddDefine(outDefines, define.c_str())) {
                BE_ERRLOG("ShaderTemplate::AppendFeatureSetDefines: feature define name '%s' conflicts with an existing define in shader template '%s'\n", define.c_str(), hashName.c_str());
                addedAllDefines = false;
                continue;
            }
        }
    }

    return addedAllDefines;
}

bool ShaderTemplate::PrepareShaderCompileData(ShaderPermutationKey permutationKey, ShaderCompileData &outCompileData) const {
    outCompileData.Clear();

    // 원본 permutation 을 얻는다.
    ShaderPermutation *srcPermutation = FindPermutation(permutationKey);
    if (!srcPermutation) {
        BE_WARNLOG("ShaderTemplate::PrepareShaderCompileData: shader template '%s' has no permutation 0x%llx\n", GetHashName(), permutationKey.hash);
        return false;
    }

    // global defines + base defines + property defines + feature defines 순으로 합쳐서 컴파일 define 을 만든다.
    shaderTemplateManager.CopyGlobalDefines(outCompileData.compileDefines);
    AppendDefines(outCompileData.compileDefines, internalData.baseDefines);
    AppendDefines(outCompileData.compileDefines, srcPermutation->propertyDefines);
    if (!AppendFeatureSetDefines(outCompileData.compileDefines, srcPermutation->featureSet)) {
        return false;
    }

    ShaderPermutationKey::NormalizeDefines(outCompileData.compileDefines);

    for (int i = 0; i < to_int(RHI::ShaderStage::Count); i++) {
        outCompileData.stageSources[i] = internalData.stageSources[i];
    }

    return true;
}
