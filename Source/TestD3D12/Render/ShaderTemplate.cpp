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
    permutationKey = ShaderPermutationKey();

    for (StageSource &stageSource : stageSources) {
        stageSource = StageSource();
    }

    permutationDefines.Clear();
    globalDefines.Clear();
}

void ShaderTemplate::InternalData::Purge() {
    for (StageSource &stageSource : stageSources) {
        stageSource = StageSource();
    }

    properties.Clear();
}

void ShaderTemplate::InternalData::Swap(InternalData &rhs) {
    for (int i = 0; i < to_int(RHI::ShaderStage::Count); i++) {
        BE1::Swap(stageSources[i], rhs.stageSources[i]);
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

void ShaderTemplate::ShaderPermutation::ClearShaders() {
    for (int i = 0; i < to_int(RHI::ShaderStage::Count); ++i) {
        if (shaders[i]) {
            RHI::renderer->DestroyShader(shaders[i], true);
            shaders[i] = nullptr;
        }
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

    for (ShaderPermutation *permutation : permutations) {
        RHI::Shader *&shader = permutation->shaders[to_int(shaderStage)];
        if (shader) {
            RHI::renderer->DestroyShader(shader, true);
            shader = nullptr;
        }
    }

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

ShaderPermutationKey ShaderTemplate::AddPermutation(std::initializer_list<const char *> defines) {
    BE1::StrArray permutationDefines;
    for (const char *define : defines) {
        if (define && define[0]) {
            permutationDefines.Append(define);
        }
    }

    return AddPermutation(permutationDefines);
}

ShaderPermutationKey ShaderTemplate::AddPermutation(const BE1::StrArray &defines) {
    BE1::StrArray permutationDefines = defines;
    ShaderPermutationKey::NormalizePermutationDefines(permutationDefines);

    ShaderPermutationKey permutationKey = ShaderPermutationKey::GeneratePermutationKey(permutationDefines);
    ShaderPermutation *permutation = FindPermutation(permutationKey);
    if (permutation) {
        return permutationKey;
    }

    permutation = new ShaderPermutation;
    permutation->key = permutationKey;
    permutation->defines = permutationDefines;
    permutations.Append(permutation);
    return permutationKey;
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

// 같은 이름의 define 이 있는지 검사한다. define name 만 비교하며, value 는 비교하지 않는다.
static bool HasDefine(const BE1::StrArray &defines, const char *keyDefine) {
    if (!keyDefine || !keyDefine[0]) {
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

static void AppendDefines(BE1::StrArray &defines, const BE1::StrArray &appendDefines) {
    for (const BE1::Str &define : appendDefines) {
        assert(!HasDefine(defines, define.c_str()));
        defines.Append(define);
    }
}

ShaderPermutationKey ShaderTemplate::AddPermutation(const BE1::Array<PropertyValue> &propertyValues, const BE1::StrArray &defines) {
    BE1::StrArray permutationDefines = defines;

    for (int i = 0; i < internalData.properties.Count(); i++) {
        const ShaderPropertyInfo &propertyInfo = internalData.properties[i];
        if (!propertyInfo.IsShaderDefine()) {
            continue;
        }

        const char *propertyDefineName = propertyInfo.GetName();
        if (HasDefine(permutationDefines, propertyDefineName)) {
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

    ShaderPermutationKey permutationKey = ShaderPermutationKey::GeneratePermutationKey(permutationDefines);
    ShaderPermutation *permutation = FindPermutation(permutationKey);
    if (permutation) {
        return permutationKey;
    }

    permutation = new ShaderPermutation;
    permutation->key = permutationKey;
    permutation->defines = permutationDefines;
    permutations.Append(permutation);
    return permutationKey;
}

ShaderPermutationKey ShaderTemplate::AddPermutation(const BE1::Array<PropertyValue> &propertyValues) {
    return AddPermutation(propertyValues, {});
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
    std::shared_ptr<ShaderPipelineState> pipelineState = FindCachedGraphicsPipeline(permutationKey, pipelineDesc);
    if (!pipelineState) {
        uint64_t hash = GenerateGraphicsPipelineHash(permutationKey, pipelineDesc);

        CachedGraphicsPipeline &cachedPipeline = cachedGraphicsPipelines.Alloc();
        cachedPipeline.hash = hash;
        cachedPipeline.pipelineState = std::shared_ptr<ShaderPipelineState>(new ShaderPipelineState(ShaderPipelineState::Type::Graphics, hash, permutationKey, pipelineDesc));

        pipelineState = cachedPipeline.pipelineState;
    }

    if (!pipelineState->IsReady()) {
        pipelineState->EnsureBuildAsync(shared_from_this());
    }

    return pipelineState;
}

std::shared_ptr<ShaderPipelineState> ShaderTemplate::GetComputePipeline(ShaderPermutationKey permutationKey) {
    std::shared_ptr<ShaderPipelineState> pipelineState = FindCachedComputePipeline(permutationKey);
    if (!pipelineState) {
        CachedComputePipeline &cachedPipeline = cachedComputePipelines.Alloc();
        cachedPipeline.permutationKey = permutationKey;
        cachedPipeline.pipelineState = std::shared_ptr<ShaderPipelineState>(new ShaderPipelineState(ShaderPipelineState::Type::Compute, permutationKey.hash, permutationKey));

        pipelineState = cachedPipeline.pipelineState;
    }

    if (!pipelineState->IsReady()) {
        pipelineState->EnsureBuildAsync(shared_from_this());
    }

    return pipelineState;
}

RHI::PipelineState *ShaderTemplate::CreateGraphicsPSO(const PipelineBuildData &buildData, ShaderPermutation &permutation, const ShaderPipelineDesc &pipelineDesc) const {
    if (!PreparePermutationGraphicsShaders(buildData, &permutation)) {
        return nullptr;
    }

    RHI::PipelineStateDesc psoDesc;
    psoDesc.vs = permutation.shaders[to_int(RHI::ShaderStage::Vertex)];
    psoDesc.ps = permutation.shaders[to_int(RHI::ShaderStage::Fragment)];
    psoDesc.ds = permutation.shaders[to_int(RHI::ShaderStage::Domain)];
    psoDesc.hs = permutation.shaders[to_int(RHI::ShaderStage::Hull)];
    psoDesc.gs = permutation.shaders[to_int(RHI::ShaderStage::Geometry)];
    psoDesc.inputLayout = pipelineDesc.inputLayout;
    psoDesc.rasterizerState = RHI::renderer->GetRasterizerState(pipelineDesc.rasterizerStateType);
    psoDesc.depthStencilState = RHI::renderer->GetDepthStencilState(pipelineDesc.depthStencilStateType);
    psoDesc.blendState = RHI::renderer->GetBlendState(pipelineDesc.blendStateType);
    psoDesc.renderDest = &pipelineDesc.renderDest;
    psoDesc.primitiveTopology = pipelineDesc.primitiveTopology;
    psoDesc.sampleMask = pipelineDesc.sampleMask;

    return RHI::renderer->CreateGraphicsPSO(&psoDesc);
}

RHI::PipelineState *ShaderTemplate::CreateComputePSO(const PipelineBuildData &buildData, ShaderPermutation &permutation) const {
    if (!PreparePermutationComputeShader(buildData, &permutation)) {
        return nullptr;
    }

    RHI::Shader *computeShader = permutation.shaders[to_int(RHI::ShaderStage::Compute)];
    return RHI::renderer->CreateComputePSO(computeShader);
}

void ShaderTemplate::Purge() {
    ClearCachedPipelines();
    ClearPermutations();

    internalData.Purge();
}

void ShaderTemplate::ApplyPrepared(InternalData &newInternalData) {
    ClearPermutationShaders();

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

void ShaderTemplate::ClearPermutationShaders() {
    for (ShaderPermutation *permutation : permutations) {
        permutation->ClearShaders();
    }
}

void ShaderTemplate::ClearPermutations() {
    ClearPermutationShaders();

    permutations.DeleteContents(true);
}

ShaderTemplate::ShaderPermutation *ShaderTemplate::FindPermutation(ShaderPermutationKey permutationKey) const {
    for (ShaderPermutation *permutation : permutations) {
        if (permutation->key == permutationKey) {
            return permutation;
        }
    }
    return nullptr;
}

bool ShaderTemplate::PreparePipelineBuildData(ShaderPermutationKey permutationKey, PipelineBuildData &outBuildData) const {
    outBuildData.Clear();

    // 원본 permutation 을 얻는다.
    ShaderPermutation *srcPermutation = FindPermutation(permutationKey);
    if (!srcPermutation) {
        BE_WARNLOG("ShaderTemplate::PreparePipelineBuildData: shader template '%s' has no permutation 0x%llx\n", GetHashName(), permutationKey.hash);
        return false;
    }

    outBuildData.permutationKey = srcPermutation->key;
    outBuildData.permutationDefines = srcPermutation->defines;
    shaderTemplateManager.CopyGlobalDefines(outBuildData.globalDefines);

    for (int i = 0; i < to_int(RHI::ShaderStage::Count); i++) {
        outBuildData.stageSources[i] = internalData.stageSources[i];
    }

    return true;
}

bool ShaderTemplate::PreparePipelinePSO(const ShaderPipelineState &pipelineState, const PipelineBuildData &buildData, RHI::PipelineState *&preparedPSO) const {
    preparedPSO = nullptr;

    if (buildData.permutationKey != pipelineState.permutationKey) {
        BE_WARNLOG("ShaderTemplate::PreparePipelinePSO: shader template '%s' has mismatched permutation 0x%llx, expected 0x%llx\n", GetHashName(), buildData.permutationKey.hash, pipelineState.permutationKey.hash);
        return false;
    }

    ShaderPermutation buildPermutation;
    buildPermutation.key = buildData.permutationKey;
    buildPermutation.defines = buildData.permutationDefines;

    // NOTE: stage source 와 define 은 enqueue 시점에 PipelineBuildData 로 복사해둔 값을 사용한다.
    // live permutation 의 shader 포인터를 로딩 스레드에서 그대로 가져다 쓰면, 메인 스레드가 permutation 을 무효화하는 순간 shader 수명이 엇갈릴 수 있다.
    // 그래서 PSO 생성에 필요한 shader 는 이 함수 안에서 임시로 만들고, PSO 생성이 끝나면 바로 해제한다.
    // 같은 permutation 내에서 pipeline state 만 다르게 해서 PSO 를 여러 개 만드는 경우, 매번 같은 shader 를 생성해야 한다.
    // 비효율적이지만 어차피 CSO 를 이용하기도 하고 재활용할 때 필요한 동기화 로직까지 고려하면, 이방법도 나쁘지 않은 것 같다.
    switch (pipelineState.type) {
    case ShaderPipelineState::Type::Graphics:
        preparedPSO = CreateGraphicsPSO(buildData, buildPermutation, pipelineState.graphicsDesc);
        break;
    case ShaderPipelineState::Type::Compute:
        preparedPSO = CreateComputePSO(buildData, buildPermutation);
        break;
    default:
        assert(0);
        break;
    }

    buildPermutation.ClearShaders();
    return preparedPSO != nullptr;
}

// 특정 shader stage 의 shader 를 준비한다.
bool ShaderTemplate::PrepareStageShader(const PipelineBuildData &buildData, RHI::Shader *&shader, RHI::ShaderStage shaderStage, const BE1::StrArray &compileDefines) const {
    assert(shaderStage < RHI::ShaderStage::Count);

    const StageSource &stageSource = buildData.stageSources[to_int(shaderStage)];
    if (stageSource.shaderStage == RHI::ShaderStage::Count) {
        return false;
    }

    shader = RHI::renderer->CreateShaderFromFile(stageSource.shaderModel, stageSource.shaderStage, stageSource.filename, stageSource.entryPoint, compileDefines.Count() > 0 ? &compileDefines : nullptr);
    if (!shader) {
        BE_WARNLOG("ShaderTemplate::PrepareStageShader: failed to compile shader '%s'\n", stageSource.filename.c_str());
        return false;
    }
    return true;
}

// permutation 의 모든 graphics shaders 들을 준비한다.
bool ShaderTemplate::PreparePermutationGraphicsShaders(const PipelineBuildData &buildData, ShaderPermutation *permutation) const {
    static constexpr RHI::ShaderStage GraphicsShaderStages[] = {
        RHI::ShaderStage::Vertex,
        RHI::ShaderStage::Hull,
        RHI::ShaderStage::Domain,
        RHI::ShaderStage::Geometry,
        RHI::ShaderStage::Fragment
    };

    bool hasGraphicsStage = false;

    // 컴파일 define 목록은 글로벌 define 과 permutation 의 define 을 합쳐서 만든다.
    BE1::StrArray compileDefines = buildData.globalDefines;
    AppendDefines(compileDefines, permutation->defines);
    ShaderPermutationKey::NormalizePermutationDefines(compileDefines);

    for (RHI::ShaderStage shaderStage : GraphicsShaderStages) {
        const StageSource &stageSource = buildData.stageSources[to_int(shaderStage)];
        if (stageSource.shaderStage == RHI::ShaderStage::Count) {
            continue;
        }

        hasGraphicsStage = true;

        RHI::Shader *&permutationShader = permutation->shaders[to_int(shaderStage)];
        if (!permutationShader) {
            if (!PrepareStageShader(buildData, permutationShader, shaderStage, compileDefines)) {
                return false;
            }
        }
    }

    if (!hasGraphicsStage) {
        BE_WARNLOG("ShaderTemplate::PreparePermutationGraphicsShaders: shader template '%s' has no graphics shader stages for permutation 0x%llx\n", GetHashName(), permutation->key.hash);
        return false;
    }

    return true;
}

// permutation 의 compute shader 를 준비한다.
bool ShaderTemplate::PreparePermutationComputeShader(const PipelineBuildData &buildData, ShaderPermutation *permutation) const {
    const StageSource &stageSource = buildData.stageSources[to_int(RHI::ShaderStage::Compute)];
    if (stageSource.shaderStage == RHI::ShaderStage::Count) {
        BE_WARNLOG("ShaderTemplate::PreparePermutationComputeShader: shader template '%s' has no compute shader stage for permutation 0x%llx\n", GetHashName(), permutation->key.hash);
        return false;
    }

    // 컴파일 define 목록은 글로벌 define 과 permutation 의 define 을 합쳐서 만든다.
    BE1::StrArray compileDefines = buildData.globalDefines;
    AppendDefines(compileDefines, permutation->defines);
    ShaderPermutationKey::NormalizePermutationDefines(compileDefines);

    RHI::Shader *&permutationShader = permutation->shaders[to_int(RHI::ShaderStage::Compute)];
    if (!permutationShader) {
        if (!PrepareStageShader(buildData, permutationShader, RHI::ShaderStage::Compute, compileDefines)) {
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
