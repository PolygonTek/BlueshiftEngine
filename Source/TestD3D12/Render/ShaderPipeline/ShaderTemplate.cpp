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
#include "LoadingSystem.h"
#include "Asset/GuidMapper.h"
#include "IO/FileSystem.h"

ShaderTemplate::ReloadedDelegate ShaderTemplate::reloaded;

static constexpr const char *standardPropertyNames[] = {
    "alphaCutoff",
    "baseColorMap",
    "baseColorFactor",
    "normalMap",
    "emissiveMap",
    "emissionColor",
    "emissionFactor",
    "aoMap",
    "aoStrength",
    "metallicMap",
    "roughnessMap",
    "metallicFactor",
    "roughnessFactor",
    "clearCoatMap",
    "clearCoatFactor",
    "clearCoatRoughnessMap",
    "clearCoatRoughnessFactor",
    "clearCoatNormalMap",
    "heightMap",
    "heightScale",
    "shadowTexture",
    "skyCubeMap",
    "skyBackMap",
    "skyFrontMap",
    "skyRightMap",
    "skyLeftMap",
    "skyTopMap",
    "skyDownMap",
    "tcScale",
    "tcTranslation"
};
static_assert(COUNT_OF(standardPropertyNames) == static_cast<int>(ShaderTemplate::StandardProperty::Count));

ShaderTemplate::InternalData::InternalData() {
    ResetStandardPropertyIndices();
}

void ShaderTemplate::InternalData::Purge() {
    for (ShaderStageSource &stageSource : stageSources) {
        stageSource = ShaderStageSource();
    }

    inheritedShaderTemplateName.Clear();
    baseDefines.Clear();
    for (BE1::StrArray &defines : featureDefines) {
        defines.Clear();
    }
    supportedFeatureMask = 0;
    properties.Clear();
    propertyHash.Clear(64, 64);
    ResetStandardPropertyIndices();
}

void ShaderTemplate::InternalData::Swap(InternalData &rhs) {
    for (int i = 0; i < to_int(RHI::ShaderStage::Count); i++) {
        BE1::Swap(stageSources[i], rhs.stageSources[i]);
    }

    BE1::Swap(inheritedShaderTemplateName, rhs.inheritedShaderTemplateName);
    baseDefines.Swap(rhs.baseDefines);
    for (int i = 0; i < to_int(ShaderFeature::Count); i++) {
        featureDefines[i].Swap(rhs.featureDefines[i]);
    }
    BE1::Swap(supportedFeatureMask, rhs.supportedFeatureMask);
    properties.Swap(rhs.properties);
    propertyHash.Swap(rhs.propertyHash);
    for (int i = 0; i < NumStandardProperties; ++i) {
        BE1::Swap(standardPropertyIndices[i], rhs.standardPropertyIndices[i]);
    }
    BE1::Swap(flags, rhs.flags);
}

const ShaderPropertyDesc *ShaderTemplate::InternalData::FindProperty(const char *name) const {
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

bool ShaderTemplate::AddProperty(const char *name, const char *label, BE1::Variant::Type::Enum type, const BE1::Variant &defaultValue, const char *description, int flags, ShaderPropertyDesc::ShaderFlag shaderFlags) {
    ShaderPropertyDesc propertyDesc(name, label, type, defaultValue, description, flags, shaderFlags);
    if (!internalData.AddProperty(propertyDesc)) {
        BE_WARNLOG("ShaderTemplate::AddProperty: duplicated property name '%s' in shader template '%s'\n", propertyDesc.GetName(), hashName.c_str());
        return false;
    }

    return true;
}

bool ShaderTemplate::AddProperty(const ShaderPropertyDesc &propertyDesc) {
    if (!internalData.AddProperty(propertyDesc)) {
        BE_WARNLOG("ShaderTemplate::AddProperty: duplicated property name '%s' in shader template '%s'\n", propertyDesc.GetName(), hashName.c_str());
        return false;
    }

    return true;
}

bool ShaderTemplate::InternalData::AddProperty(const ShaderPropertyDesc &propertyDesc) {
    if (FindProperty(propertyDesc.GetName())) {
        assert(0);
        return false;
    }

    int propertyIndex = properties.Append(propertyDesc);
    int hash = propertyHash.GenerateHash(propertyDesc.GetName(), true);
    propertyHash.Add(hash, propertyIndex);

    for (int i = 0; i < NumStandardProperties; ++i) {
        if (!BE1::Str::Cmp(propertyDesc.GetName(), standardPropertyNames[i])) {
            standardPropertyIndices[i] = propertyIndex;
            break;
        }
    }
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
    // 중복된 프로퍼티 이름이 있다면 duplicatedPropertyName 에 기록하고 false 를 리턴한다.
    for (const ShaderPropertyDesc &propertyDesc : rhs.properties) {
        if (FindProperty(propertyDesc.GetName())) {
            assert(0);
            if (duplicatedPropertyName) {
                *duplicatedPropertyName = propertyDesc.GetName();
            }
            return false;
        }
    }

    for (const ShaderPropertyDesc &propertyDesc : rhs.properties) {
        AddProperty(propertyDesc);
    }

    return true;
}

void ShaderTemplate::BuildDefaultPropertyValues(BE1::Array<PropertyValue> &outPropertyValues) const {
    outPropertyValues.Clear();

    for (const ShaderPropertyDesc &propertyDesc : internalData.properties) {
        outPropertyValues.Append(propertyDesc.GetDefaultValue());
    }
}

bool ShaderTemplate::InternalData::AddStage(RHI::ShaderStage shaderStage, RHI::ShaderModel shaderModel, const char *filename, const char *entryPoint) {
    assert(shaderStage >= RHI::ShaderStage::Vertex && shaderStage < RHI::ShaderStage::Count);

    if (shaderStage == RHI::ShaderStage::Compute) {
        for (int stageIndex = to_int(RHI::ShaderStage::Vertex); stageIndex < to_int(RHI::ShaderStage::Compute); ++stageIndex) {
            if (!stageSources[stageIndex].filename.IsEmpty()) {
                return false;
            }
        }
    } else if (!stageSources[to_int(RHI::ShaderStage::Compute)].filename.IsEmpty()) {
        return false;
    }

    ShaderStageSource &stageSource = stageSources[to_int(shaderStage)];

    stageSource.shaderModel = shaderModel;
    stageSource.filename = filename;
    stageSource.entryPoint = entryPoint;
    return true;
}

bool ShaderTemplate::SupportsFeature(ShaderFeature feature) const {
    if (feature >= ShaderFeature::Count) {
        return false;
    }
    return internalData.featureDefines[to_int(feature)].Count() > 0;
}

ShaderFeatureSet ShaderTemplate::MaskSupportedFeatures(const ShaderFeatureSet &requestedFeatureSet) const {
    ShaderFeatureSet supportedFeatureSet = {};
    supportedFeatureSet.flags = requestedFeatureSet.flags & internalData.supportedFeatureMask;
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

static bool TryAddDefine(BE1::StrArray &targetDefines, const char *newDefine) {
    if (!newDefine || !newDefine[0]) {
        return false;
    }

    if (HasDefine(targetDefines, newDefine)) {
        return false;
    }

    targetDefines.Append(newDefine);
    return true;
}

// sourceDefines 를 targetDefines 에 추가한다. 빈 항목이나 이름이 충돌하는 항목은 건너뛴다.
// 하나라도 추가하지 못하면 false 를 반환하며, 이미 추가한 항목은 유지한다.
static bool AddDefines(BE1::StrArray &targetDefines, const BE1::StrArray &sourceDefines) {
    bool addedAllDefines = true;

    for (const BE1::Str &define : sourceDefines) {
        if (!TryAddDefine(targetDefines, define.c_str())) {
            addedAllDefines = false;
            continue;
        }
    }

    return addedAllDefines;
}

// 호출부에서 define 이름의 충돌 검사를 마쳤다는 전제로 이름과 값을 조합해 추가한다.
static void AddDefineWithValue(BE1::StrArray &targetDefines, const char *newDefineName, int value) {
    if (!newDefineName || !newDefineName[0]) {
        return;
    }

    BE1::Str newDefine = newDefineName;
    newDefine.Append("=");
    newDefine.Append(BE1::Str(value));
    targetDefines.Append(newDefine);
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
    for (int propertyIndex = 0; propertyIndex < numProperties; propertyIndex++) {
        const ShaderPropertyDesc &propertyDesc = internalData.properties[propertyIndex];
        if (!propertyDesc.IsShaderDefine()) {
            continue;
        }

        const char *propertyDefineName = propertyDesc.GetName();
        if (HasDefine(propertyDefines, propertyDefineName) || HasDefine(internalData.baseDefines, propertyDefineName) || HasFeatureDefine(featureSet, propertyDefineName)) {
            BE_ERRLOG("ShaderTemplate::RegisterPermutation: property define name '%s' conflicts with an existing shader property in shader template '%s'\n", propertyDefineName, hashName.c_str());
            continue;
        }

        const PropertyValue *propertyValue = propertyIndex < propertyValues.Count() ? &propertyValues[propertyIndex] : nullptr;
        const BE1::Variant &value = propertyValue ? *propertyValue : propertyDesc.GetDefaultValue();

        switch (propertyDesc.GetType()) {
        case BE1::Variant::Type::Bool:
            AddDefineWithValue(propertyDefines, propertyDefineName, value.As<bool>() ? 1 : 0);
            break;
        case BE1::Variant::Type::Int:
            AddDefineWithValue(propertyDefines, propertyDefineName, value.As<int>());
            break;
        default:
            BE_WARNLOG("ShaderTemplate::RegisterPermutation: shader define property '%s' has unsupported type\n", propertyDefineName);
            break;
        }
    }

    ShaderPermutationKey::NormalizeDefines(propertyDefines);

    // 프로퍼티 define 과 feature set mask 를 합쳐서 permutation key 를 만든다.
    ShaderPermutationKey permutationKey = ShaderPermutationKey::GeneratePermutationKey(propertyDefines, featureSet);
    if (FindPermutation(permutationKey)) {
        return permutationKey;
    }

    ShaderPermutation *permutation = new ShaderPermutation;
    permutation->featureSet = featureSet;
    permutation->propertyDefines.Swap(propertyDefines);
    permutationMap.Set(permutationKey.hash, permutation);
    return permutationKey;
}

bool ShaderTemplate::Create(const char *text, const char *baseDir) {
    InternalData newInternalData;

    if (!Parse(text, baseDir, hashName.c_str(), newInternalData)) {
        return false;
    }

    ApplyPrepared(newInternalData);
    return true;
}

void ShaderTemplate::ApplyPrepared(InternalData &newInternalData) {
    Purge();

    internalData.Swap(newInternalData);

    // default property 값 + empty featureSet 조합으로 permutation 을 등록한다.
    RegisterPermutation();
}

bool ShaderTemplate::Parse(const char *text, const char *baseDir, const char *shaderTemplateName, InternalData &outInternalData) {
    outInternalData.Purge();

    BE1::Lexer lexer;
    lexer.Init(BE1::Lexer::Flag::NoErrors);
    lexer.Load(text, BE1::Str::Length(text), shaderTemplateName);

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
            if (!ParseInheritProperties(lexer, baseDir, shaderTemplateName, outInternalData)) {
                return false;
            }
        } else if (!token.Icmp("baseDefines")) {
            if (!ParseBaseDefines(lexer, shaderTemplateName, outInternalData)) {
                return false;
            }
        } else if (!token.Icmp("features")) {
            if (!ParseFeatures(lexer, shaderTemplateName, outInternalData)) {
                return false;
            }
        } else if (!token.Icmp("stages")) {
            if (!ParseStages(lexer, baseDir, shaderTemplateName, outInternalData)) {
                return false;
            }
        } else if (!token.Icmp("properties")) {
            if (!ParseProperties(lexer, shaderTemplateName, outInternalData)) {
                return false;
            }
        } else {
            BE_WARNLOG("unknown token '%s' in shader template '%s'\n", token.c_str(), shaderTemplateName);
            return false;
        }
    }

    if (!closed) {
        BE_WARNLOG("ShaderTemplate::Parse: missing closing brace in shader template '%s'\n", shaderTemplateName);
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
        BE_WARNLOG("ShaderTemplate::Parse: shader template '%s' has no shader stages\n", shaderTemplateName);
        return false;
    }

    for (int featureIndex = 0; featureIndex < to_int(ShaderFeature::Count); featureIndex++) {
        if (!outInternalData.featureDefines[featureIndex].IsEmpty()) {
            ShaderFeatureSet featureSet = {};
            featureSet.EnableFeature(static_cast<ShaderFeature>(featureIndex));
            outInternalData.supportedFeatureMask |= featureSet.flags;
        }
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

    static constexpr ModelName modelNames[] = {
        { "SM_5_0", RHI::ShaderModel::SM_5_0 },
        { "SM_6_0", RHI::ShaderModel::SM_6_0 },
        { "SM_6_1", RHI::ShaderModel::SM_6_1 },
        { "SM_6_2", RHI::ShaderModel::SM_6_2 },
        { "SM_6_3", RHI::ShaderModel::SM_6_3 },
        { "SM_6_4", RHI::ShaderModel::SM_6_4 },
        { "SM_6_5", RHI::ShaderModel::SM_6_5 },
        { "SM_6_6", RHI::ShaderModel::SM_6_6 },
        { "SM_6_7", RHI::ShaderModel::SM_6_7 },
        { "SM_6_8", RHI::ShaderModel::SM_6_8 }
    };

    for (const ModelName &modelName : modelNames) {
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

bool ShaderTemplate::ParseInheritProperties(BE1::Lexer &lexer, const char *baseDir, const char *shaderTemplateName, InternalData &outInternalData) {
    BE1::Str inheritedName;
    if (!lexer.ExpectTokenType(BE1::Lexer::TokenType::String, &inheritedName)) {
        return false;
    }

    if (!outInternalData.inheritedShaderTemplateName.IsEmpty()) {
        BE_WARNLOG("ShaderTemplate::ParseInheritProperties: shader template '%s' cannot inherit properties from multiple shader templates\n", shaderTemplateName);
        return false;
    }

    SharedPtr<ShaderTemplate> inheritedShaderTemplate = shaderTemplateManager.GetShaderTemplate(inheritedName.c_str());
    if (!inheritedShaderTemplate && baseDir && baseDir[0] && BE1::FileSystem::IsRelativePath(inheritedName.c_str())) {
        BE1::Str resolvedName = baseDir;
        resolvedName.AppendPath(inheritedName, '/');
        inheritedShaderTemplate = shaderTemplateManager.GetShaderTemplate(resolvedName.c_str());
    }

    if (!inheritedShaderTemplate) {
        BE_WARNLOG("ShaderTemplate::ParseInheritProperties: shader template '%s' was not found while parsing shader template '%s'\n", inheritedName.c_str(), shaderTemplateName);
        return false;
    }

    const char *duplicatedPropertyName = nullptr;
    if (!outInternalData.CopyPropertiesFrom(inheritedShaderTemplate->internalData, &duplicatedPropertyName)) {
        BE_WARNLOG("ShaderTemplate::ParseInheritProperties: duplicated property name '%s' in shader template '%s'\n", duplicatedPropertyName ? duplicatedPropertyName : "", shaderTemplateName);
        return false;
    }

    outInternalData.inheritedShaderTemplateName = inheritedShaderTemplate->GetHashName();
    return true;
}

// 이름 토큰은 이미 읽힌 상태이며, 같은 줄의 선택적인 '=값'을 이어서 파싱한다.
static bool ParseDefine(BE1::Lexer &lexer, const BE1::Str &nameToken, BE1::Str &outDefine) {
    outDefine = nameToken;

    BE1::Str token;
    if (!lexer.ReadToken(&token, false)) {
        return true;
    }

    if (token != "=") {
        lexer.UnreadToken(&token);
        return true;
    }

    if (!lexer.ReadToken(&token, false)) {
        return false;
    }

    outDefine.Append("=");
    outDefine.Append(token);
    return true;
}

bool ShaderTemplate::ParseBaseDefines(BE1::Lexer &lexer, const char *shaderTemplateName, InternalData &outInternalData) {
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

        BE1::Str define;
        if (!ParseDefine(lexer, token, define)) {
            BE_WARNLOG("ShaderTemplate::ParseBaseDefines: missing value for base define '%s' in shader template '%s'\n", define.c_str(), shaderTemplateName);
            return false;
        }

        if (HasDefine(outInternalData.baseDefines, define.c_str())) {
            BE_WARNLOG("ShaderTemplate::ParseBaseDefines: duplicated base define name '%s' in shader template '%s'\n", define.c_str(), shaderTemplateName);
            return false;
        }

        outInternalData.baseDefines.Append(define);
    }

    if (!closed) {
        BE_WARNLOG("ShaderTemplate::ParseBaseDefines: missing closing brace in shader template '%s'\n", shaderTemplateName);
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
        { "outputRoughness",    ShaderFeature::OutputRoughness },
        { "outputVelocity",     ShaderFeature::OutputVelocity },
        { "surfacePrepass",     ShaderFeature::SurfacePrepass },
        { "transparentSSRPrepass", ShaderFeature::TransparentSSRPrepass },
        { "directLighting",     ShaderFeature::DirectLighting },
        { "indirectLighting",   ShaderFeature::IndirectLighting }
    };

    for (const FeatureName &featureName : FeatureNames) {
        if (!token.Icmp(featureName.featureName)) {
            outFeature = featureName.feature;
            return true;
        }
    }

    return false;
}

bool ShaderTemplate::ParseFeatures(BE1::Lexer &lexer, const char *shaderTemplateName, InternalData &outInternalData) {
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
            BE_WARNLOG("ShaderTemplate::ParseFeatures: unknown feature '%s' in shader template '%s'\n", token.c_str(), shaderTemplateName);
            return false;
        }

        int featureIndex = to_int(feature);
        if (declaredFeatures[featureIndex]) {
            BE_WARNLOG("ShaderTemplate::ParseFeatures: duplicated feature '%s' in shader template '%s'\n", token.c_str(), shaderTemplateName);
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

            BE1::Str define;
            if (!ParseDefine(lexer, defineToken, define)) {
                BE_WARNLOG("ShaderTemplate::ParseFeatures: missing value for feature define '%s' in shader template '%s'\n", define.c_str(), shaderTemplateName);
                return false;
            }

            if (HasDefine(featureDefines, define.c_str())) {
                BE_WARNLOG("ShaderTemplate::ParseFeatures: duplicated feature define name '%s' in shader template '%s'\n", define.c_str(), shaderTemplateName);
                return false;
            }

            featureDefines.Append(define);
        }

        if (!featureClosed) {
            BE_WARNLOG("ShaderTemplate::ParseFeatures: missing closing brace in feature '%s' in shader template '%s'\n", token.c_str(), shaderTemplateName);
            return false;
        }
    }

    if (!closed) {
        BE_WARNLOG("ShaderTemplate::ParseFeatures: missing closing brace in shader template '%s'\n", shaderTemplateName);
        return false;
    }

    return true;
}

bool ShaderTemplate::ParseStages(BE1::Lexer &lexer, const char *baseDir, const char *shaderTemplateName, InternalData &outInternalData) {
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
            BE_WARNLOG("unknown shader stage '%s' in shader template '%s'\n", token.c_str(), shaderTemplateName);
            return false;
        }

        BE1::Str shaderModelName;
        if (!lexer.ReadToken(&shaderModelName, false)) {
            return false;
        }

        RHI::ShaderModel shaderModel = RHI::Renderer::MinimumShaderModel();
        if (!ParseShaderModel(shaderModelName, shaderModel)) {
            BE_WARNLOG("unknown shader model '%s' in shader template '%s'\n", shaderModelName.c_str(), shaderTemplateName);
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
        if (!outInternalData.AddStage(shaderStage, shaderModel, resolvedFilename.c_str(), entryPoint.c_str())) {
            BE_WARNLOG("ShaderTemplate::ParseStages: cannot mix graphics and compute stages in shader template '%s'\n", shaderTemplateName);
            return false;
        }
    }

    if (!closed) {
        BE_WARNLOG("ShaderTemplate::ParseStages: missing closing brace in shader template '%s'\n", shaderTemplateName);
        return false;
    }

    return true;
}

bool ShaderTemplate::ParseProperties(BE1::Lexer &lexer, const char *shaderTemplateName, InternalData &outInternalData) {
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
                BE_WARNLOG("duplicated property name '%s' in shader template '%s'\n", token.c_str(), shaderTemplateName);
                return false;
            }

            lexer.UnreadToken(&token);

            ShaderPropertyDesc shaderPropDesc;
            if (!shaderPropDesc.Parse(lexer)) {
                BE_WARNLOG("error occurred in parsing property propInfo in shader '%s'\n", shaderTemplateName);
                lexer.SkipRestOfLine();
                continue;
            }

            if (!outInternalData.AddProperty(shaderPropDesc)) {
                BE_WARNLOG("duplicated property name '%s' in shader template '%s'\n", shaderPropDesc.GetName(), shaderTemplateName);
                return false;
            }
        }
    }

    if (!closed) {
        BE_WARNLOG("ShaderTemplate::ParseProperties: missing closing brace in shader template '%s'\n", shaderTemplateName);
        return false;
    }

    return true;
}

bool ShaderPropertyDesc::Parse(BE1::Lexer &lexer) {
    type = BE1::Variant::Type::None;
    flags = BE1::PropertyDesc::Flag::Editor;
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
        } else if (!textureTypeStr.Icmp("3D")) {
            textureType = TextureType::Texture3D;
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
                        conditions.Append(ShaderPropertyDesc::Condition(conditionName, valueList));

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
                shaderFlags |= ShaderPropertyDesc::ShaderFlag::ShaderDefine;
            } else if (token == "hidden") {
                shaderFlags |= ShaderPropertyDesc::ShaderFlag::Hidden;
            } else if (token == "normal") {
                shaderFlags |= ShaderPropertyDesc::ShaderFlag::Normal;
            } else if (token == "linear") {
                shaderFlags |= ShaderPropertyDesc::ShaderFlag::Linear;
            } else if (token == "noStreaming") {
                shaderFlags |= ShaderPropertyDesc::ShaderFlag::NoStreaming;
            } else if (token == "noMipmaps") {
                shaderFlags |= ShaderPropertyDesc::ShaderFlag::NoMipmaps;
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

bool ShaderTemplate::Prepare(const char *filename, InternalData &outInternalData) {
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

    BE1::Str shaderTemplateName;
    if (!lexer.ReadToken(&shaderTemplateName)) {
        BE1::fileSystem.FreeFile(data);
        return false;
    }

    BE1::Str baseDir = exactFilename;
    baseDir.StripFileName();
    if (!Parse(data + lexer.GetCurrentOffset(), baseDir, shaderTemplateName.c_str(), outInternalData)) {
        BE1::fileSystem.FreeFile(data);
        return false;
    }

    BE1::fileSystem.FreeFile(data);

    outInternalData.flags |= ShaderTemplate::Flag::LoadedFromFile;

    return true;
}

bool ShaderTemplate::Load(const char *filename) {
    InternalData newInternalData;
    if (!Prepare(filename, newInternalData)) {
        return false;
    }

    ApplyPrepared(newInternalData);
    return true;
}

bool ShaderTemplate::Reload() {
    if (!BE1::HasFlag(internalData.flags, Flag::LoadedFromFile)) {
        BE_WARNLOG("ShaderTemplate::Reload: cannot reload shader template '%s' because it was not loaded from a file\n", GetHashName());
        return false;
    }

    InternalData newInternalData;
    if (!Prepare(hashName.c_str(), newInternalData)) {
        return true;
    }

    loadingSystem.QueueShaderTemplateReload(shared_from_this(), newInternalData);
    loadingSystem.ProcessPendingReloads();
    return true;
}

void ShaderTemplate::Purge() {
    // 파이프라인 캐시에서 이 ShaderTemplate 을 사용하는 모든 ShaderPipeline 캐시를 제거한다.
    shaderPipelineCache.ClearPipelines(this);

    ClearPermutations();

    internalData.Purge();
}

void ShaderTemplate::ClearPermutations() {
    permutationMap.DeleteContents(true);
}

const ShaderTemplate::ShaderPermutation *ShaderTemplate::FindPermutation(ShaderPermutationKey permutationKey) const {
    const auto *entry = permutationMap.Get(permutationKey.hash);
    if (entry) {
        return entry->second;
    }
    return nullptr;
}

bool ShaderTemplate::HasFeatureDefine(const ShaderFeatureSet &featureSetToCheck, const char *defineName) const {
    for (int featureIndex = 0; featureIndex < to_int(ShaderFeature::Count); featureIndex++) {
        ShaderFeature feature = static_cast<ShaderFeature>(featureIndex);
        if (!featureSetToCheck.HasFeature(feature)) {
            continue;
        }

        if (HasDefine(internalData.featureDefines[featureIndex], defineName)) {
            return true;
        }
    }

    return false;
}

bool ShaderTemplate::CollectFeatureDefines(const ShaderFeatureSet &featureSet, BE1::StrArray &outDefines) const {
    bool addedAllDefines = true;

    for (int featureIndex = 0; featureIndex < to_int(ShaderFeature::Count); featureIndex++) {
        ShaderFeature feature = static_cast<ShaderFeature>(featureIndex);
        if (!featureSet.HasFeature(feature)) {
            continue;
        }

        for (const BE1::Str &define : internalData.featureDefines[featureIndex]) {
            if (!TryAddDefine(outDefines, define.c_str())) {
                BE_ERRLOG("ShaderTemplate::CollectFeatureDefines: feature define name '%s' conflicts with an existing define in shader template '%s'\n", define.c_str(), hashName.c_str());
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
    const ShaderPermutation *srcPermutation = FindPermutation(permutationKey);
    if (!srcPermutation) {
        BE_WARNLOG("ShaderTemplate::PrepareShaderCompileData: shader template '%s' has no permutation 0x%llx\n", GetHashName(), permutationKey.hash);
        return false;
    }

    // global defines + base defines + property defines + feature defines 순으로 합쳐서 컴파일 define 을 만든다.
    shaderTemplateManager.GetGlobalDefines(outCompileData.compileDefines);

    if (!AddDefines(outCompileData.compileDefines, internalData.baseDefines)) {
        BE_ERRLOG("ShaderTemplate::PrepareShaderCompileData: failed to append base defines in shader template '%s'\n", GetHashName());
        return false;
    }

    if (!AddDefines(outCompileData.compileDefines, srcPermutation->propertyDefines)) {
        BE_ERRLOG("ShaderTemplate::PrepareShaderCompileData: failed to append property defines in shader template '%s'\n", GetHashName());
        return false;
    }

    if (!CollectFeatureDefines(srcPermutation->featureSet, outCompileData.compileDefines)) {
        return false;
    }

    ShaderPermutationKey::NormalizeDefines(outCompileData.compileDefines);

    for (int i = 0; i < to_int(RHI::ShaderStage::Count); i++) {
        outCompileData.stageSources[i] = internalData.stageSources[i];
    }

    return true;
}
