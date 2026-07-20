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
#include "Material.h"
#include "MaterialManager.h"
#include "LoadingSystem.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"
#include "Render/ShaderPipeline/ShaderPipelineCache.h"
#include "Render/RenderSystem.h"
#include "Asset/GuidMapper.h"
#include "IO/FileSystem.h"
#include "Render/Texture/TextureManager.h"

void Material::InternalData::Purge() {
    shaderTemplate.reset();
    propertyValues.Clear();
    textureProperties.Clear();
    cullMode = RHI::CullMode::Back;
    renderingMode = RenderingMode::Opaque;
    blendType = RHI::BlendStateType::Disabled;
    flags = Flag::None;
}

void Material::InternalData::Swap(InternalData &rhs) {
    shaderTemplate.swap(rhs.shaderTemplate);
    propertyValues.Swap(rhs.propertyValues);
    textureProperties.Swap(rhs.textureProperties);
    BE1::Swap(cullMode, rhs.cullMode);
    BE1::Swap(renderingMode, rhs.renderingMode);
    BE1::Swap(blendType, rhs.blendType);
    BE1::Swap(flags, rhs.flags);
}

void Material::Purge() {
    materialManager.UnregisterStreamingMaterial(this);
    internalData.Purge();
    InvalidatePermutationKeys();
}

static BE1::Str ResolveTextureName(const ShaderTemplate::PropertyValue &propertyValue) {
    if (propertyValue.GetType() == BE1::Variant::Type::Guid) {
        BE1::Guid textureGuid = propertyValue.As<BE1::Guid>();
        return textureGuid.IsZero() ? "" : BE1::resourceGuidMapper.Get(textureGuid);
    }

    if (propertyValue.GetType() == BE1::Variant::Type::Str) {
        return propertyValue.As<BE1::Str>();
    }

    return "";
}

int Material::FindPropertyIndex(const char *propertyName) const {
    if (!propertyName || !propertyName[0] || !internalData.shaderTemplate) {
        return -1;
    }

    return internalData.shaderTemplate->FindPropertyIndex(propertyName);
}

int Material::FindPropertyIndex(ShaderTemplate::StandardProperty standardProperty) const {
    if (!internalData.shaderTemplate) {
        return -1;
    }

    return internalData.shaderTemplate->GetStandardPropertyIndex(standardProperty);
}

const ShaderTemplate::PropertyValue *Material::GetPropertyValue(int propertyIndex) const {
    if (propertyIndex < 0 || propertyIndex >= internalData.propertyValues.Count()) {
        return nullptr;
    }

    return &internalData.propertyValues[propertyIndex];
}

const ShaderTemplate::PropertyValue *Material::GetPropertyValue(const char *propertyName) const {
    return GetPropertyValue(FindPropertyIndex(propertyName));
}

const ShaderTemplate::PropertyValue *Material::GetPropertyValue(ShaderTemplate::StandardProperty standardProperty) const {
    if (!internalData.shaderTemplate) {
        return nullptr;
    }
    return GetPropertyValue(FindPropertyIndex(standardProperty));
}

const SharedPtr<Texture> *Material::GetPropertyTexture(int propertyIndex) const {
    for (const TextureProperty &textureProperty : internalData.textureProperties) {
        if (textureProperty.propertyIndex == propertyIndex) {
            return &textureProperty.texture;
        }
    }

    return nullptr;
}

const SharedPtr<Texture> *Material::GetPropertyTexture(const char *propertyName) const {
    return GetPropertyTexture(FindPropertyIndex(propertyName));
}

const SharedPtr<Texture> *Material::GetPropertyTexture(ShaderTemplate::StandardProperty standardProperty) const {
    if (!internalData.shaderTemplate) {
        return nullptr;
    }
    return GetPropertyTexture(FindPropertyIndex(standardProperty));
}

void Material::InvalidatePermutationKeys() const {
    cachedPermutationKeys.Clear();
}

bool Material::SetPropertyValue(int propertyIndex, const ShaderTemplate::PropertyValue &value) {
    assert(BE1::Engine::IsInMainThread());

    if (!internalData.shaderTemplate) {
        return false;
    }

    const BE1::Array<ShaderPropertyInfo> &properties = internalData.shaderTemplate->GetProperties();
    if (propertyIndex < 0 || propertyIndex >= properties.Count() || propertyIndex >= internalData.propertyValues.Count()) {
        return false;
    }

    const ShaderPropertyInfo &propertyInfo = properties[propertyIndex];
    ShaderTemplate::PropertyValue newValue;
    if (propertyInfo.IsTexture()) {
        if (value.GetType() == BE1::Variant::Type::Str) {
            newValue = BE1::resourceGuidMapper.GetOrCreateGuid(value.As<BE1::Str>());
        } else if (value.GetType() == BE1::Variant::Type::Guid) {
            newValue = value;
        } else {
            BE_WARNLOG("Material::SetPropertyValue: texture property '%s' in material '%s' requires a Guid or Str value\n", propertyInfo.GetName(), hashName.c_str());
            return false;
        }
    } else {
        if (value.GetType() != propertyInfo.GetType()) {
            BE_WARNLOG("Material::SetPropertyValue: property '%s' type mismatch in material '%s'\n", propertyInfo.GetName(), hashName.c_str());
            return false;
        }
        newValue = value;
    }

    if (internalData.propertyValues[propertyIndex] == newValue) {
        return true;
    }

    // 렌더 백엔드에서 이전 프레임의 material 데이터 사용을 마칠 때까지 기다린다.
    renderSystem->WaitAllRenderCompleted();

    internalData.propertyValues[propertyIndex] = newValue;

    if (propertyInfo.IsShaderDefine()) {
        InvalidatePermutationKeys();
    }
    if (propertyInfo.IsTexture()) {
        UpdateTextureProperties();
    }

    return true;
}

bool Material::SetPropertyValue(const char *propertyName, const ShaderTemplate::PropertyValue &value) {
    return SetPropertyValue(FindPropertyIndex(propertyName), value);
}

static Texture::Flag GetTextureFlags(const ShaderPropertyInfo &propertyInfo) {
    Texture::Flag flags = Texture::Flag::None;

    if (!propertyInfo.IsNoStreaming()) {
        flags |= Texture::Flag::Streaming;
    }
    if (propertyInfo.IsNormal()) {
        flags |= Texture::Flag::NormalMap;
    }
    if (propertyInfo.IsNoMipmaps()) {
        flags |= Texture::Flag::NoMipmaps;
    }

    return flags;
}

static SharedPtr<Texture> GetFallbackTexture(const ShaderPropertyInfo &propertyInfo) {
    if (propertyInfo.IsNormal()) {
        return textureManager.flatNormalTexture ? textureManager.flatNormalTexture : textureManager.defaultTexture;
    }

    return textureManager.whiteTexture ? textureManager.whiteTexture : textureManager.defaultTexture;
}

// 프로퍼티 중에 텍스쳐 프로퍼티만 골라서 internalData.textureProperties 배열을 업데이트한다.
void Material::UpdateTextureProperties() {
    internalData.textureProperties.Clear();

    if (!internalData.shaderTemplate) {
        materialManager.UnregisterStreamingMaterial(this);
        return;
    }

    const BE1::Array<ShaderPropertyInfo> &properties = internalData.shaderTemplate->GetProperties();
    const int numProperties = properties.Count();
    for (int propertyIndex = 0; propertyIndex < numProperties; propertyIndex++) {
        const ShaderPropertyInfo &propertyInfo = properties[propertyIndex];
        if (!propertyInfo.IsTexture()) {
            continue;
        }

        TextureProperty &textureProperty = internalData.textureProperties.Alloc();
        textureProperty.propertyIndex = propertyIndex;

        const ShaderTemplate::PropertyValue *propertyValue = propertyIndex < internalData.propertyValues.Count() ? &internalData.propertyValues[propertyIndex] : nullptr;
        if (propertyValue) {
            BE1::Str textureName = ResolveTextureName(*propertyValue);
            if (!textureName.IsEmpty()) {
                textureProperty.texture = textureManager.GetTexture(textureName.c_str(), GetTextureFlags(propertyInfo), true);
            }
        }

        if (!textureProperty.texture) {
            textureProperty.texture = GetFallbackTexture(propertyInfo);
        }
    }

    materialManager.RefreshStreamingMaterial(shared_from_this());
}

bool Material::HasStreamingTextureProperties() const {
    if (!internalData.shaderTemplate) {
        return false;
    }

    const BE1::Array<ShaderPropertyInfo> &properties = internalData.shaderTemplate->GetProperties();
    for (const TextureProperty &textureProperty : internalData.textureProperties) {
        if (textureProperty.propertyIndex < 0 || textureProperty.propertyIndex >= properties.Count()) {
            continue;
        }

        if (!properties[textureProperty.propertyIndex].IsNoStreaming()) {
            return true;
        }
    }

    return false;
}

void Material::RequestStreamingMaxDimension(uint32_t requestedMaxDimension) const {
    for (const TextureProperty &textureProperty : internalData.textureProperties) {
        const SharedPtr<Texture> &texture = textureProperty.texture;

        if (texture && textureManager.IsRegisteredStreamingTexture(texture.get())) {
            const Texture::Streaming *streaming = texture->GetStreaming();

            // 실제 원본 텍스쳐의 최대 축 크기
            uint32_t originalMaxDimension = BE1::Max3(streaming->originalWidth, streaming->originalHeight, streaming->originalDepth);

            // 요청된 해상도와 원본 텍스쳐의 최대 축 크기를 비교하여, 스트리밍 시스템에 요청할 최대 해상도를 결정한다.
            texture->RequestStreamingMaxDimension(BE1::Min(requestedMaxDimension, originalMaxDimension));
        }
    }
}

ShaderPermutationKey Material::EnsurePermutation(const ShaderFeatureSet &featureSet) const {
    if (!internalData.shaderTemplate) {
        return ShaderPermutationKey();
    }

    ShaderFeatureSet supportedFeatureSet = featureSet;
    if (!featureSet.IsEmpty()) {
        // shader template 에서 지원하지 않는 features 는 거른다.
        supportedFeatureSet = internalData.shaderTemplate->MaskSupportedFeatures(featureSet);
    }

    // permutation 캐시에서 실제로 사용할 featureSet 에 대한 permutation 을 찾아본다.
    for (const CachedPermutationKey &cachedPermutationKey : cachedPermutationKeys) {
        // propertyValues 가 바뀌면 캐시가 무효화되므로, 여기서는 featureSet 만 비교하면 된다.
        if (cachedPermutationKey.featureSet == supportedFeatureSet) {
            return cachedPermutationKey.key;
        }
    }

    if (supportedFeatureSet != featureSet) {
        BE_WARNLOG("Material::EnsurePermutation: material '%s' requested unsupported shader features 0x%08x in shader template '%s'\n", hashName.c_str(), featureSet.flags & ~supportedFeatureSet.flags, internalData.shaderTemplate->GetHashName());
    }

    // 프로퍼티 + 피쳐셋에 대한 permutation 을 등록한다.
    ShaderPermutationKey permutationKey = internalData.shaderTemplate->RegisterPermutation(internalData.propertyValues, supportedFeatureSet);

    CachedPermutationKey &cachedPermutationKey = cachedPermutationKeys.Alloc();
    cachedPermutationKey.key = permutationKey;
    cachedPermutationKey.featureSet = supportedFeatureSet;

    return permutationKey;
}

void Material::EnsurePermutations(const BE1::Array<ShaderFeatureSet> &featureSets) const {
    if (!internalData.shaderTemplate) {
        return;
    }

    for (const ShaderFeatureSet &featureSet : featureSets) {
        EnsurePermutation(featureSet);
    }
}

static RHI::RasterizerStateType GetMaterialRasterizerStateType(RHI::RasterizerStateType baseRasterizerStateType, RHI::CullMode cullMode) {
    switch (baseRasterizerStateType) {
    case RHI::RasterizerStateType::SolidFrontSided:
    case RHI::RasterizerStateType::SolidBackSided:
    case RHI::RasterizerStateType::SolidTwoSided:
        switch (cullMode) {
        case RHI::CullMode::Back:
            return RHI::RasterizerStateType::SolidFrontSided;
        case RHI::CullMode::Front:
            return RHI::RasterizerStateType::SolidBackSided;
        case RHI::CullMode::None:
            return RHI::RasterizerStateType::SolidTwoSided;
        default:
            assert(0);
            break;
        }
        break;
    default:
        break;
    }

    return baseRasterizerStateType;
}

ShaderPipeline *Material::GetGraphicsPipeline(const ShaderFeatureSet &featureSet, const GraphicsPipelineDesc &baseGraphicsPipelineDesc, bool async) const {
    if (!internalData.shaderTemplate) {
        return nullptr;
    }

    GraphicsPipelineDesc graphicsPipelineDesc = baseGraphicsPipelineDesc;
    graphicsPipelineDesc.rasterizerStateType = GetMaterialRasterizerStateType(baseGraphicsPipelineDesc.rasterizerStateType, internalData.cullMode);
    graphicsPipelineDesc.blendStateType = internalData.renderingMode == RenderingMode::Blend ? internalData.blendType : RHI::BlendStateType::Disabled;

    return shaderPipelineCache.GetGraphicsPipeline(internalData.shaderTemplate, EnsurePermutation(featureSet), graphicsPipelineDesc, async).get();
}

bool Material::Create(const char *text, const char *baseDir) {
    InternalData newInternalData;
    if (!Parse(text, baseDir, hashName.c_str(), newInternalData)) {
        return false;
    }

    ApplyPrepared(newInternalData);
    return true;
}

void Material::ApplyPrepared(InternalData &newInternalData) {
    Purge();

    internalData.Swap(newInternalData);
    UpdateTextureProperties();
}

bool Material::Parse(const char *text, const char *baseDir, const char *materialName, InternalData &outInternalData) {
    outInternalData.Purge();

    if (!text) {
        return false;
    }

    BE1::Lexer lexer;
    lexer.Init(BE1::Lexer::Flag::NoErrors);
    lexer.Load(text, BE1::Str::Length(text), materialName);

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
        } else if (!token.Icmp("noShadow")) {
            outInternalData.flags |= Flag::NoShadow;
        } else if (!token.Icmp("cull")) {
            if (!ParseCull(lexer, materialName, outInternalData)) {
                return false;
            }
        } else if (!token.Icmp("renderingMode")) {
            if (!ParseRenderingMode(lexer, materialName, outInternalData)) {
                return false;
            }
        } else if (!token.Icmp("blendType")) {
            if (!ParseBlendType(lexer, materialName, outInternalData)) {
                return false;
            }
        } else if (!token.Icmp("shaderTemplate")) {
            if (!ParseShaderTemplate(lexer, baseDir, materialName, outInternalData)) {
                return false;
            }
        } else if (!token.Icmp("properties")) {
            if (!ParseProperties(lexer, materialName, outInternalData)) {
                return false;
            }
        } else {
            BE_WARNLOG("unknown token '%s' in material '%s'\n", token.c_str(), materialName);
            lexer.SkipRestOfLine();
        }
    }

    if (!closed) {
        BE_WARNLOG("Material::Parse: missing closing brace in material '%s'\n", materialName);
        return false;
    }

    if (!outInternalData.shaderTemplate) {
        BE_WARNLOG("Material::Parse: material '%s' has no shader template\n", materialName);
        return false;
    }

    return true;
}

bool Material::ParseCull(BE1::Lexer &lexer, const char *materialName, InternalData &outInternalData) {
    BE1::Str token;
    if (!lexer.ReadToken(&token, false)) {
        BE_WARNLOG("Material::ParseCull: missing cullMode value in material '%s'\n", materialName);
        return false;
    }

    if (!token.Icmp("none") || !token.Icmp("disable") || !token.Icmp("twoSided")) {
        outInternalData.cullMode = RHI::CullMode::None;
    } else if (!token.Icmp("back") || !token.Icmp("backSide") || !token.Icmp("backSided")) {
        outInternalData.cullMode = RHI::CullMode::Back;
    } else if (!token.Icmp("front") || !token.Icmp("frontSide") || !token.Icmp("frontSided")) {
        outInternalData.cullMode = RHI::CullMode::Front;
    } else {
        BE_WARNLOG("Material::ParseCull: unknown cullMode '%s' in material '%s'\n", token.c_str(), materialName);
        return false;
    }

    return true;
}

bool Material::ParseRenderingMode(BE1::Lexer &lexer, const char *materialName, InternalData &outInternalData) {
    BE1::Str renderingModeString;
    if (!lexer.ReadToken(&renderingModeString, false)) {
        BE_WARNLOG("Material::ParseRenderingMode: missing renderingMode value in material '%s'\n", materialName);
        return false;
    }

    if (!renderingModeString.Icmp("Opaque")) {
        outInternalData.renderingMode = RenderingMode::Opaque;
    } else if (!renderingModeString.Icmp("AlphaCutoff")) {
        outInternalData.renderingMode = RenderingMode::AlphaCutoff;
    } else if (!renderingModeString.Icmp("Blend")) {
        outInternalData.renderingMode = RenderingMode::Blend;
        outInternalData.blendType = RHI::BlendStateType::AlphaBlend; // 기본 블렌딩 타입 설정
    } else {
        BE_WARNLOG("Material::ParseRenderingMode: unknown renderingMode '%s' in material '%s'\n", renderingModeString.c_str(), materialName);
        return false;
    }

    return true;
}

bool Material::ParseBlendType(BE1::Lexer &lexer, const char *materialName, InternalData &outInternalData) {
    BE1::Str token;
    if (!lexer.ReadToken(&token, false)) {
        BE_WARNLOG("Material::ParseBlendType: missing blendType value in material '%s'\n", materialName);
        return false;
    }

    if (!token.Icmp("alphaBlend")) {
        outInternalData.blendType = RHI::BlendStateType::AlphaBlend;
    } else if (!token.Icmp("addBlend")) {
        outInternalData.blendType = RHI::BlendStateType::AddBlend;
    } else if (!token.Icmp("add")) {
        outInternalData.blendType = RHI::BlendStateType::Add;
    } else if (!token.Icmp("filter") || !token.Icmp("modulate")) {
        outInternalData.blendType = RHI::BlendStateType::Modulate;
    } else if (!token.Icmp("modulate2x")) {
        outInternalData.blendType = RHI::BlendStateType::Modulate2X;
    } else {
        BE_WARNLOG("Material::ParseBlendType: unknown blendType '%s' in material '%s'\n", token.c_str(), materialName);
        return false;
    }

    return true;
}

bool Material::ParseShaderTemplate(BE1::Lexer &lexer, const char *baseDir, const char *materialName, InternalData &outInternalData) {
    if (outInternalData.shaderTemplate) {
        BE_WARNLOG("Material::ParseShaderTemplate: duplicated shaderTemplate in material '%s'\n", materialName);
        return false;
    }

    BE1::Str shaderTemplateName;
    if (!lexer.ExpectTokenType(BE1::Lexer::TokenType::String, &shaderTemplateName)) {
        return false;
    }

    SharedPtr<ShaderTemplate> shaderTemplate = shaderTemplateManager.GetShaderTemplate(shaderTemplateName.c_str());
    if (!shaderTemplate && baseDir && baseDir[0] && BE1::FileSystem::IsRelativePath(shaderTemplateName.c_str())) {
        BE1::Str resolvedName = baseDir;
        resolvedName.AppendPath(shaderTemplateName, '/');
        shaderTemplate = shaderTemplateManager.GetShaderTemplate(resolvedName.c_str());
    }

    if (!shaderTemplate) {
        BE_WARNLOG("Material::ParseShaderTemplate: shader template '%s' was not found while parsing material '%s'\n", shaderTemplateName.c_str(), materialName);
        return false;
    }

    outInternalData.shaderTemplate = shaderTemplate;
    outInternalData.shaderTemplate->InitDefaultPropertyValues(outInternalData.propertyValues);

    return true;
}

bool Material::ParseProperties(BE1::Lexer &lexer, const char *materialName, InternalData &outInternalData) {
    BE1::Str token;
    bool closed = false;

    if (!outInternalData.shaderTemplate) {
        BE_WARNLOG("Material::ParseProperties: properties block must appear after shaderTemplate in material '%s'\n", materialName);
        return false;
    }

    if (!lexer.ExpectPunctuation(BE1::Lexer::PuncType::BraceOpen)) {
        return false;
    }

    const BE1::Array<ShaderPropertyInfo> &properties = outInternalData.shaderTemplate->GetProperties();

    while (lexer.ReadToken(&token)) {
        if (token[0] == '}') {
            closed = true;
            break;
        }

        if (token == "," || token == ";") {
            continue;
        }

        BE1::Str propertyName = token;
        BE1::Str valueString;
        if (!lexer.ReadToken(&valueString, false)) {
            BE_WARNLOG("Material::ParseProperties: missing value for property '%s' in material '%s'\n", propertyName.c_str(), materialName);
            return false;
        }

        int propertyIndex = outInternalData.shaderTemplate->FindPropertyIndex(propertyName.c_str());
        if (propertyIndex < 0) {
            BE_WARNLOG("Material::ParseProperties: unknown property '%s' in material '%s'\n", propertyName.c_str(), materialName);
            continue;
        }

        if (properties[propertyIndex].IsTexture()) {
            outInternalData.propertyValues[propertyIndex] = BE1::resourceGuidMapper.GetOrCreateGuid(valueString);
        } else {
            outInternalData.propertyValues[propertyIndex] = BE1::Variant::FromString(properties[propertyIndex].GetType(), valueString.c_str());
        }
    }

    if (!closed) {
        BE_WARNLOG("Material::ParseProperties: missing closing brace in material '%s'\n", materialName);
        return false;
    }

    return true;
}

void Material::RemapShaderTemplateProperties(const BE1::Array<ShaderPropertyInfo> &oldProperties) {
    if (!internalData.shaderTemplate) {
        return;
    }

    const int numOldProperties = oldProperties.Count();

    // 예전 프로퍼티 값들을 백업해둔다.
    BE1::Array<ShaderTemplate::PropertyValue> oldPropertyValues;
    oldPropertyValues.Swap(internalData.propertyValues);

    // 기본 프로퍼티 값들로 세팅한다.
    internalData.shaderTemplate->InitDefaultPropertyValues(internalData.propertyValues);

    // 이름/타입이 예전과 같은 프로퍼티가 있다면 값을 옮겨온다.
    const BE1::Array<ShaderPropertyInfo> &newProperties = internalData.shaderTemplate->GetProperties();
    const int numNewProperties = newProperties.Count();
    for (int newIndex = 0; newIndex < numNewProperties; newIndex++) {
        const ShaderPropertyInfo &newProperty = newProperties[newIndex];

        for (int oldIndex = 0; oldIndex < numOldProperties; oldIndex++) {
            const ShaderPropertyInfo &oldProperty = oldProperties[oldIndex];
            if (BE1::Str::Cmp(newProperty.GetName(), oldProperty.GetName())) {
                continue;
            }

            if (oldIndex >= oldPropertyValues.Count()) {
                break;
            }

            if (newProperty.GetType() != oldProperty.GetType()) {
                BE_WARNLOG("Material::RemapShaderTemplateProperties: property '%s' type changed in material '%s'\n", newProperty.GetName(), hashName.c_str());
                break;
            }

            internalData.propertyValues[newIndex] = oldPropertyValues[oldIndex];
            break;
        }
    }

    UpdateTextureProperties();
    InvalidatePermutationKeys();
}

bool Material::Prepare(const char *filename, InternalData &outInternalData) {
    BE1::Str exactFilename = filename;
    exactFilename.DefaultFileExtension(".material");

    char *data;
    size_t size = BE1::fileSystem.LoadFile(exactFilename, true, (void **)&data);
    if (!data) {
        return false;
    }

    BE1::Lexer lexer;
    lexer.Init(BE1::Lexer::Flag::NoErrors);
    lexer.Load(data, (int)size, exactFilename);

    if (!lexer.ExpectTokenString("material")) {
        BE1::fileSystem.FreeFile(data);
        return false;
    }

    BE1::Str materialName;
    if (!lexer.ReadToken(&materialName)) {
        BE1::fileSystem.FreeFile(data);
        return false;
    }

    BE1::Str baseDir = exactFilename;
    baseDir.StripFileName();
    if (!Parse(data + lexer.GetCurrentOffset(), baseDir, materialName.c_str(), outInternalData)) {
        BE1::fileSystem.FreeFile(data);
        return false;
    }

    BE1::fileSystem.FreeFile(data);

    outInternalData.flags |= Flag::LoadedFromFile;

    return true;
}

bool Material::Load(const char *filename) {
    InternalData newInternalData;
    if (!Prepare(filename, newInternalData)) {
        return false;
    }

    ApplyPrepared(newInternalData);
    return true;
}

bool Material::Reload() {
    if (!BE1::HasFlag(internalData.flags, Flag::LoadedFromFile)) {
        BE_WARNLOG("Material::Reload: cannot reload material '%s' because it was not loaded from a file\n", GetHashName());
        return false;
    }

    InternalData newInternalData;
    if (!Prepare(hashName.c_str(), newInternalData)) {
        return true;
    }

    loadingSystem.QueueMaterialReload(shared_from_this(), newInternalData);
    loadingSystem.ProcessPendingReloads();
    return true;
}
