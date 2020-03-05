// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Render/Render.h"
#include "RenderInternal.h"
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"
#include "IO/FileSystem.h"

BE_NAMESPACE_BEGIN

void Material::Purge() {
    if (pass) {
        if (pass->shader) {
            shaderManager.ReleaseShader(pass->shader);
            pass->shader = nullptr;
        }

        if (pass->referenceShader) {
            shaderManager.ReleaseShader(pass->referenceShader);
            pass->referenceShader = nullptr;
        }

        if (pass->texture) {
            textureManager.ReleaseTexture(pass->texture);
            pass->texture = nullptr;
        }

        for (int i = 0; i < pass->shaderProperties.Count(); i++) {
            const auto entry = pass->shaderProperties.GetByIndex(i);
            if (entry->second.texture) {
                textureManager.ReleaseTexture(entry->second.texture);
                entry->second.texture = nullptr;
            }
        }

        delete pass;
        pass = nullptr;
    }
}

bool Material::Create(const char *text) {
    Str token;

    Purge();

    flags = 0;

    Lexer lexer; 
    lexer.Init(Lexer::Flag::NoErrors);
    lexer.Load(text, Str::Length(text), hashName.c_str());

    while (lexer.ReadToken(&token)) {
        if (token.IsEmpty()) {
            break;
        } else if (!token.Icmp("noShadow")) {
            flags |= Flag::NoShadow;
        } else if (!token.Icmp("forceShadow")) {
            flags |= Flag::ForceShadow;
        } else if (!token.Icmp("unsmoothTangents")) {
            flags |= Flag::UnsmoothTangents;
        } else if (!token.Icmp("polygonOffset")) {
            flags |= Flag::PolygonOffset;
        } else if (!token.Icmp("decal")) {
            type = Type::Decal;
        } else if (!token.Icmp("light")) {
            type = Type::Light;
        } else if (!token.Icmp("blendLight")) {
            type = Type::BlendLight;
        } else if (!token.Icmp("fogLight")) {
            type = Type::FogLight;
        } else if (!token.Icmp("pass")) {
            pass = new ShaderPass;
            if (!ParsePass(lexer, pass)) {
                delete pass;
                pass = nullptr;
                return false;
            }
        } else {
            BE_WARNLOG("unknown general material parameter '%s' in material '%s'\n", token.c_str(), hashName.c_str());
            lexer.SkipRestOfLine();
        }
    }
      
    Finish();

    return true;
}

bool Material::ParsePass(Lexer &lexer, ShaderPass *pass) {
    int blendSrc = 0;
    int blendDst = 0;
    int colorWrite = RHI::RedWrite | RHI::GreenWrite | RHI::BlueWrite;
    int depthWrite = RHI::DepthWrite;
    int depthTestBits = RHI::DF_LEqual;

    pass->renderingMode     = RenderingMode::Opaque;
    pass->transparency      = Transparency::Default;
    pass->cullType          = RHI::CullType::Back;
    pass->stateBits         = 0;
    pass->cutoffAlpha       = 0.004f;
    pass->vertexColorMode   = VertexColorMode::VertexColorMode::Ignore;
    pass->useOwnerColor     = false;
    pass->texture           = nullptr;
    pass->shader            = nullptr;
    pass->referenceShader   = nullptr;
    pass->constantColor     = Color4(1.0f, 1.0f, 1.0f, 1.0f);
    pass->tcScale           = Vec2(1.0f, 1.0f);
    pass->tcTranslation     = Vec2(0.0f, 0.0f);
    pass->instancingEnabled = false;

    if (!lexer.ExpectPunctuation(Lexer::PuncType::BraceOpen)) {
        return false;
    }

    Str token;

    while (1) {
        lexer.ReadToken(&token);

        if (token.IsEmpty()) {
            BE_WARNLOG("no matching '}' found\n");
            return false;
        } else if (token[0] == '}') {
            break;
        } else if (!token.Icmp("renderingMode")) {
            ParseRenderingMode(lexer, &pass->renderingMode);
        } else if (!token.Icmp("cull")) {
            if (lexer.ReadToken(&token, false)) {
                if (!token.Icmp("none") || !token.Icmp("disable") || !token.Icmp("twoSided")) {
                    pass->cullType = RHI::CullType::None;
                } else if (!token.Icmp("back") || !token.Icmp("backSide") || !token.Icmp("backSided")) {
                    pass->cullType = RHI::CullType::Back;
                } else if (!token.Icmp("front") || !token.Icmp("frontSide") || !token.Icmp("frontSided")) {
                    pass->cullType = RHI::CullType::Front;
                } else {
                    BE_WARNLOG("invalid cull parm '%s' in material '%s'\n", token.c_str(), hashName.c_str());
                }
            } else {
                BE_WARNLOG("missing parameter cull keyword in material '%s'\n", hashName.c_str());
            }
        } else if (!token.Icmp("transparency")) {
            if (lexer.ReadToken(&token, false)) {
                if (!token.Icmp("default")) {
                    pass->transparency = Transparency::Default;
                } else if (!token.Icmp("twoPassesOneSide")) {
                    pass->transparency = Transparency::TwoPassesOneSide;
                } else if (!token.Icmp("twoPassesTwoSides")) {
                    pass->transparency = Transparency::TwoPassesTwoSides;
                } else {
                    BE_WARNLOG("invalid transparency parm '%s' in material '%s'\n", token.c_str(), hashName.c_str());
                }
            } else {
                BE_WARNLOG("missing transparency cull keyword in material '%s'\n", hashName.c_str());
            }
        } else if (!token.Icmp("depthTest")) {
            ParseDepthTest(lexer, &depthTestBits);
        } else if (!token.Icmp("shader")) {
            if (ParseShader(lexer, pass->referenceShader, pass->shaderProperties)) {
                CommitShaderPropertiesChanged();
            }
        } else if (!token.Icmp("map")) {
            if (lexer.ReadToken(&token, false)) {
                const Guid textureGuid = Guid::FromString(token);
                const Str texturePath = resourceGuidMapper.Get(textureGuid);
                pass->texture = textureManager.GetTexture(texturePath);
            } else {
                BE_WARNLOG("missing map GUID in material '%s'\n", hashName.c_str());
            }
        } else if (!token.Icmp("mapPath")) {
            if (lexer.ReadToken(&token, false)) {
                pass->texture = textureManager.FindTexture(token);
                if (!pass->texture) {
                    pass->texture = textureManager.defaultTexture;
                }
                pass->texture->AddRefCount();
            } else {
                BE_WARNLOG("missing map name in material '%s'\n", hashName.c_str());
            }
        } else if (!token.Icmp("tc")) {
            lexer.ParseVec(2, pass->tcScale);
            lexer.ParseVec(2, pass->tcTranslation);
        } else if (!token.Icmp("imageBorders")) {
            pass->imageBorderLT.x = lexer.ParseInt();
            pass->imageBorderLT.y = lexer.ParseInt();
            pass->imageBorderRB.x = lexer.ParseInt();
            pass->imageBorderRB.y = lexer.ParseInt();
        } else if (!token.Icmp("cutoffAlpha")) {
            pass->cutoffAlpha = lexer.ParseFloat(); 
        } else if (!token.Icmp("blendFunc")) {
            if (ParseBlendFunc(lexer, &blendSrc, &blendDst)) {
                depthWrite = 0; // depth write off when blendFunc is valid
            }
        } else if (!token.Icmp("colorMask")) {
            if (lexer.ReadToken(&token, false)) {
                for (int i = 0; i < token.Length(); i++) {
                    switch (token[i]) {
                    case 'R':
                        colorWrite |= RHI::RedWrite;
                        break;
                    case 'G':
                        colorWrite |= RHI::GreenWrite;
                        break;
                    case 'B':
                        colorWrite |= RHI::BlueWrite;
                        break;
                    case 'A':
                        colorWrite |= RHI::AlphaWrite;
                        break;
                    }
                }
            } else {
                BE_WARNLOG("missing color mask in material '%s'\n", hashName.c_str());
            }
        }/* else if (!token.Icmp("scale")) {
            exprChunk->ParseExpressions(lexer, 2, registers);

            textureMatrixRegisters[0][0] = registers[0];
            exprChunk->ParseExpressions("0.0f", 1, &textureMatrixRegisters[0][1]);
            exprChunk->ParseExpressions("0.0f", 1, &textureMatrixRegisters[0][2]);

            exprChunk->ParseExpressions("0.0f", 1, &textureMatrixRegisters[1][0]);
            textureMatrixRegisters[1][1] = registers[1];
            exprChunk->ParseExpressions("0.0f", 1, &textureMatrixRegisters[1][2]);

            MultiplyTextureMatrix(pass, textureMatrixRegisters);
        } else if (!token.Icmp("rotate")) {
            exprChunk->ParseExpressions(lexer, 1, registers);

            exprChunk->ParseExpressions(va("sin r%i", GET_REG_INDEX(registers[0])), 1, &registers[1]);
            exprChunk->ParseExpressions(va("cos r%i", GET_REG_INDEX(registers[0])), 1, &registers[2]);
            exprChunk->ParseExpressions(va("-r%i", GET_REG_INDEX(registers[1])), 1, &registers[3]);

            textureMatrixRegisters[0][0] = registers[2];
            textureMatrixRegisters[0][1] = registers[3];
            exprChunk->ParseExpressions("0.0f", 1, &textureMatrixRegisters[0][2]);

            textureMatrixRegisters[1][0] = registers[1];
            textureMatrixRegisters[1][1] = registers[2];
            exprChunk->ParseExpressions("0.0f", 1, &textureMatrixRegisters[1][2]);

            exprChunk->ParseExpressions("1.0f, 0.0f, -0.5f", 3, &textureMatrixRegisters2[0][0]);
            exprChunk->ParseExpressions("0.0f, 1.0f, -0.5f", 3, &textureMatrixRegisters2[1][0]);
            MultiplyTextureMatrix(pass, textureMatrixRegisters2);

            MultiplyTextureMatrix(pass, textureMatrixRegisters);

            exprChunk->ParseExpressions("1.0f, 0.0f, 0.5f", 3, &textureMatrixRegisters2[0][0]);
            exprChunk->ParseExpressions("0.0f, 1.0f, 0.5f", 3, &textureMatrixRegisters2[1][0]);
            MultiplyTextureMatrix(pass, textureMatrixRegisters2);
        } else if (!token.Icmp("scroll") || !token.Icmp("translate")) {
            exprChunk->ParseExpressions(lexer, 2, registers);

            exprChunk->ParseExpressions("1.0f", 1, &textureMatrixRegisters[0][0]);
            exprChunk->ParseExpressions("0.0f", 1, &textureMatrixRegisters[0][1]);
            textureMatrixRegisters[0][2] = registers[0];

            exprChunk->ParseExpressions("0.0f", 1, &textureMatrixRegisters[1][0]);
            exprChunk->ParseExpressions("1.0f", 1, &textureMatrixRegisters[1][1]);
            textureMatrixRegisters[1][2] = registers[1];

            MultiplyTextureMatrix(pass, textureMatrixRegisters);
        }*/else if (!token.Icmp("vertexColor")) {
            pass->vertexColorMode = VertexColorMode::Modulate;
        } else if (!token.Icmp("inverseVertexColor")) {
            pass->vertexColorMode = VertexColorMode::InverseModulate;
        } else if (!token.Icmp("color")) {
            lexer.ParseVec(4, pass->constantColor);
        } else if (!token.Icmp("useOwnerColor")) {
            pass->useOwnerColor = true;
        } else if (!token.Icmp("instancingEnabled")) {
            pass->instancingEnabled = true;
        } else {
            BE_WARNLOG("unknown material pass parameter '%s' in material '%s'\n", token.c_str(), hashName.c_str());
            lexer.SkipRestOfLine();
        }
    }

    // We don't use DST_ALPHA
    if (blendSrc == RHI::BS_OneMinusDstAlpha) {
        blendSrc = RHI::BS_Zero;
    }

    if (blendDst == RHI::BD_OneMinusDstAlpha) {
        blendDst = RHI::BD_Zero;
    }

    pass->stateBits = blendSrc | blendDst | colorWrite | depthWrite;
    pass->depthTestBits = depthTestBits;

    return true;
}

void Material::ChangeShader(Shader *shader) {
    const auto &oldPropInfoHashMap = pass->referenceShader ? pass->referenceShader->GetPropertyInfoHashMap() : StrHashMap<Shader::ShaderPropertyInfo>();
    const auto &newPropInfoHashMap = shader->GetPropertyInfoHashMap();

    // Release textures of old shader properties.
    for (int i = 0; i < pass->shaderProperties.Count(); i++) {
        const auto entry = pass->shaderProperties.GetByIndex(i);
        if (entry->second.texture) {
            textureManager.ReleaseTexture(entry->second.texture);
            entry->second.texture = nullptr;
        }
    }

    // Change new reference shader.
    if (pass->referenceShader) {
        shaderManager.ReleaseShader(pass->referenceShader);
    }
    pass->referenceShader = shader;

    // Set shader properties with reusing old shader properties.
    StrHashMap<Shader::Property> newShaderProperties;

    for (int i = 0; i < newPropInfoHashMap.Count(); i++) {
        const auto newEntry = newPropInfoHashMap.GetByIndex(i);
        const auto &key = newEntry->first;
        const auto &propInfo = newEntry->second;

        const auto oldEntry = oldPropInfoHashMap.Get(key);

        if (oldEntry && oldEntry->second.GetType() == newEntry->second.GetType()) {
            const auto &oldProp = pass->shaderProperties.Get(key)->second;

            newShaderProperties.Set(key, oldProp);
        } else {
            Shader::Property shaderProperty;

            if (propInfo.GetType() == Variant::Type::Guid && propInfo.GetMetaObject()->IsTypeOf(TextureResource::metaObject)) {
                const Str defaultTextureName = resourceGuidMapper.Get(propInfo.GetDefaultValue().As<Guid>());
                const Texture *defaultTexture = textureManager.FindTexture(defaultTextureName);
                const Guid defaultTextureGuid = resourceGuidMapper.Get(defaultTexture->GetHashName());

                shaderProperty.data = defaultTextureGuid;
            } else {
                shaderProperty.data = propInfo.GetDefaultValue();
            }

            shaderProperty.texture = nullptr;

            newShaderProperties.Set(key, shaderProperty);
        }
    }

    pass->shaderProperties.Swap(newShaderProperties);

    // Instantiate shader with changed define properites.
    CommitShaderPropertiesChanged();

    Finish();
}

void Material::CommitShaderPropertiesChanged() {
    Array<Shader::Define> defineArray;

    const auto &shaderPropertyInfos = pass->referenceShader->GetPropertyInfoHashMap();

    // List up define list for re-instantiating shader.
    for (int i = 0; i < shaderPropertyInfos.Count(); i++) {
        const auto entry = shaderPropertyInfos.GetByIndex(i);
        const auto &propName = entry->first;
        const auto &propInfo = entry->second;

        // property propInfo with shaderDefine allows only bool/enum type.
        if (propInfo.GetFlags() & PropertyInfo::Flag::ShaderDefine) {
            const auto *entry = pass->shaderProperties.Get(propName);
            const Shader::Property &shaderProp = entry->second;

            if (propInfo.GetType() == Variant::Type::Bool && shaderProp.data.As<bool>()) {
                defineArray.Append(Shader::Define(propName, 1));
            } else if (propInfo.GetType() == Variant::Type::Int && propInfo.GetEnum().Count() > 0) {
                defineArray.Append(Shader::Define(propName, shaderProp.data.As<int>()));
            }
        }
    }
    
    // Release previous one
    if (pass->shader) {
        shaderManager.ReleaseShader(pass->shader);
    }
    // Instantiate shader with the given define list.
    pass->shader = pass->referenceShader->InstantiateShader(defineArray);

    // Reload shader's texture.
    for (int i = 0; i < shaderPropertyInfos.Count(); i++) {
        const auto entry = shaderPropertyInfos.GetByIndex(i);
        const auto &propName = entry->first;
        const auto &propInfo = entry->second;

        if (propInfo.GetType() == Variant::Type::Guid && propInfo.GetMetaObject()->IsTypeOf(TextureResource::metaObject)) {
            auto *entry = pass->shaderProperties.Get(propName);
            Shader::Property &shaderProperty = entry->second;

            if (shaderProperty.texture) {
                textureManager.ReleaseTexture(shaderProperty.texture);
            }

            const Guid textureGuid = shaderProperty.data.As<Guid>();
            const Str texturePath = resourceGuidMapper.Get(textureGuid);
            shaderProperty.texture = textureManager.GetTexture(texturePath);
        }
    }
}

bool Material::ParseRenderingMode(Lexer &lexer, RenderingMode::Enum *renderingMode) const {
    Str	token;

    if (lexer.ReadToken(&token, false)) {
        if (!token.Icmp("opaque")) {
            *renderingMode = RenderingMode::Opaque;
        } else if (!token.Icmp("alphaCutoff")) {
            *renderingMode = RenderingMode::AlphaCutoff;
        } else if (!token.Icmp("alphaBlend")) {
            *renderingMode = RenderingMode::AlphaBlend;
        } else {
            BE_WARNLOG("unknown renderingMode '%s' in material '%s'\n", token.c_str(), hashName.c_str());
        }

        return true;
    }

    BE_WARNLOG("missing parameter for renderingMode keyword in material '%s\n", hashName.c_str());
    return false;
}

bool Material::ParseDepthTest(Lexer &lexer, int *depthTest) const {
    Str	token;

    if (lexer.ReadToken(&token, false)) {
        if (!token.Icmp("less")) {
            *depthTest = RHI::DF_Less;
        } else if (!token.Icmp("greater")) {
            *depthTest = RHI::DF_Greater;
        } else if (!token.Icmp("lequal")) {
            *depthTest = RHI::DF_LEqual;
        } else if (!token.Icmp("gequal")) {
            *depthTest = RHI::DF_GEqual;
        } else if (!token.Icmp("equal")) {
            *depthTest = RHI::DF_Equal;
        } else if (!token.Icmp("notequal")) {
            *depthTest = RHI::DF_NotEqual;
        } else if (!token.Icmp("always")) {
            *depthTest = RHI::DF_Always;
        } else {
            *depthTest = RHI::DF_LEqual;
            BE_WARNLOG("unknown depthTest '%s' in material '%s', substituting LEQUAL\n", token.c_str(), hashName.c_str());
        }

        return true;
    }

    BE_WARNLOG("missing parameter for depthTest keyword in material '%s'\n", hashName.c_str());
    return false;
}

bool Material::ParseShaderProperties(Lexer &lexer, Dict &properties) const {
    Str token;
    Str value;

    if (!lexer.ExpectPunctuation(Lexer::PuncType::BraceOpen)) {
        return false;
    }

    while (1) {
        lexer.ReadToken(&token);

        if (token.IsEmpty()) {
            BE_WARNLOG("no matching '}' found\n");
            return false;
        } else if (token[0] == '}') {
            break;
        } else {
            if (lexer.ReadToken(&value, false)) {
                properties.Set(token, value);
            } else {
                BE_WARNLOG("missing property value for property '%s' in material '%s'\n", token.c_str(), hashName.c_str());
            }
        }
    }

    return true;
}

bool Material::ParseShader(Lexer &lexer, Shader *&referenceShader, StrHashMap<Shader::Property> &shaderProperties) const {
    Str token;

    // Parse shader name.
    if (!lexer.ReadToken(&token, false)) {
        BE_WARNLOG("missing shader name in material '%s'\n", hashName.c_str());
        return false;
    }

    const Guid shaderGuid = Guid::FromString(token);
    const Str shaderPath = resourceGuidMapper.Get(shaderGuid);

    // Get the refernece shader by shader path.
    referenceShader = shaderManager.GetShader(shaderPath);
    if (!referenceShader) {
        return false;
    }

    // Parse the shader properties into a string and put it in the dictionary.
    Dict propDict;
    if (!ParseShaderProperties(lexer, propDict)) {
        if (referenceShader) {
            shaderManager.ReleaseShader(referenceShader);
            referenceShader = nullptr;
        }
        return false;
    }

    const auto &shaderPropertyInfos = referenceShader->GetPropertyInfoHashMap();

    // Set shader property values.
    for (int i = 0; i < shaderPropertyInfos.Count(); i++) {
        const auto entry = shaderPropertyInfos.GetByIndex(i);
        const auto &propName = entry->first;
        const auto &propInfo = entry->second;

        Shader::Property shaderProperty;

        if (propInfo.GetType() == Variant::Type::Guid && propInfo.GetMetaObject()->IsTypeOf(TextureResource::metaObject)) {
            // Get the value as a string.
            Str value = propDict.GetString(propName, propInfo.GetDefaultValue().As<Guid>().ToString(Guid::Format::DigitsWithHyphensInBraces));

            if (version >= 2) {
                if (value.Length() == 38/*Str::Length("{00000000-0000-0000-0000-000000000000}")*/ && 
                    value[0] == '{' && value[value.Length() - 1] == '}') {
                    shaderProperty.data = Variant::FromString(Variant::Type::Guid, value);
                } else {
                    shaderProperty.data = Guid::CreateGuid();
                    resourceGuidMapper.Set(shaderProperty.data.As<Guid>(), value);
                }
            } else {
                shaderProperty.data = Variant::FromString(Variant::Type::Guid, value);
            }

            // Get texture path by GUID.
            const Guid textureGuid = shaderProperty.data.As<Guid>();
            const Str texturePath = resourceGuidMapper.Get(textureGuid);

            // Set texture.
            shaderProperty.texture = textureManager.GetTexture(texturePath);
        } else {
            // Get the value as a string.
            Str value = propDict.GetString(propName, propInfo.GetDefaultValue().ToString());

            // Set value.
            shaderProperty.data = Variant::FromString(propInfo.GetType(), value);
            shaderProperty.texture = nullptr;
        }

        shaderProperties.Set(propName, shaderProperty);
    }

    return true;
}

bool Material::ParseBlendFunc(Lexer &lexer, int *blendSrc, int *blendDst) const {
    Str	token;

    if (lexer.ReadToken(&token, false)) {
        if (!token.Icmp("add")) {
            *blendSrc = RHI::BS_One;
            *blendDst = RHI::BD_One;
        } else if (!token.Icmp("addBlended")) {
            *blendSrc = RHI::BS_SrcAlpha;
            *blendDst = RHI::BD_One;
        } else if (!token.Icmp("blend")) {
            *blendSrc = RHI::BS_SrcAlpha;
            *blendDst = RHI::BD_OneMinusSrcAlpha;
        } else if (!token.Icmp("filter") || !token.Icmp("modulate")) {
            *blendSrc = RHI::BS_DstColor;
            *blendDst = RHI::BD_Zero;
        } else if (!token.Icmp("modulate2x")) {
            *blendSrc = RHI::BS_DstColor;
            *blendDst = RHI::BD_SrcColor;
        } else {
            if (!token.Icmp("ZERO") || !token.Icmp("GL_ZERO")) {
                *blendSrc = RHI::BS_Zero;
            } else if (!token.Icmp("ONE") || !token.Icmp("GL_ONE")) {
                *blendSrc = RHI::BS_One;
            } else if (!token.Icmp("DST_COLOR") || !token.Icmp("GL_DST_COLOR")) {
                *blendSrc = RHI::BS_DstColor;
            } else if (!token.Icmp("ONE_MINUS_DST_COLOR") || !token.Icmp("GL_ONE_MINUS_DST_COLOR")) {
                *blendSrc = RHI::BS_OneMinusDstColor;
            } else if (!token.Icmp("SRC_ALPHA") || !token.Icmp("GL_SRC_ALPHA")) {
                *blendSrc = RHI::BS_SrcAlpha;
            } else if (!token.Icmp("ONE_MINUS_SRC_ALPHA") || !token.Icmp("GL_ONE_MINUS_SRC_ALPHA")) {
                *blendSrc = RHI::BS_OneMinusSrcAlpha;
            } else if (!token.Icmp("DST_ALPHA") || !token.Icmp("GL_DST_ALPHA")) {
                *blendSrc = RHI::BS_DstAlpha;
            } else if (!token.Icmp("ONE_MINUS_DST_ALPHA") || !token.Icmp("GL_ONE_MINUS_DST_ALPHA")) {
                *blendSrc = RHI::BS_OneMinusDstAlpha;
            } else if (!token.Icmp("SRC_ALPHA_SATURATE") || !token.Icmp("GL_SRC_ALPHA_SATURATE")) {
                *blendSrc = RHI::BS_SrcAlphaSaturate;
            } else {
                *blendSrc = RHI::BS_One;
                BE_WARNLOG("unknown blend mode '%s' in material '%s', \nsubstituting GL_ONE\n", token.c_str(), hashName.c_str());
            }

            if (lexer.ReadToken(&token, false)) {
                if (!token.Icmp("ZERO") || !token.Icmp("GL_ZERO")) {
                    *blendDst = RHI::BD_Zero;
                } else if (!token.Icmp("ONE") || !token.Icmp("GL_ONE")) {
                    *blendDst = RHI::BD_One;
                } else if (!token.Icmp("SRC_COLOR") || !token.Icmp("GL_SRC_COLOR")) {
                    *blendDst = RHI::BD_SrcColor;
                } else if (!token.Icmp("ONE_MINUS_SRC_COLOR") || !token.Icmp("GL_ONE_MINUS_SRC_COLOR")) {
                    *blendDst = RHI::BD_OneMinusSrcColor;
                } else if (!token.Icmp("SRC_ALPHA") || !token.Icmp("GL_SRC_ALPHA")) {
                    *blendDst = RHI::BD_SrcAlpha;
                } else if (!token.Icmp("ONE_MINUS_SRC_ALPHA") || !token.Icmp("GL_ONE_MINUS_SRC_ALPHA")) {
                    *blendDst = RHI::BD_OneMinusSrcAlpha;
                } else if (!token.Icmp("DST_ALPHA") || !token.Icmp("GL_DST_ALPHA")) {
                    *blendDst = RHI::BD_DstAlpha;
                } else if (!token.Icmp("ONE_MINUS_DST_ALPHA") || !token.Icmp("GL_ONE_MINUS_DST_ALPHA")) {
                    *blendDst = RHI::BD_OneMinusDstAlpha;
                } else {
                    *blendDst = RHI::BD_One;
                    BE_WARNLOG("unknown blend mode '%s' in material '%s', substituting GL_ONE\n", token.c_str(), hashName.c_str());
                }
            } else {
                BE_WARNLOG("missing parameter for blendFunc keyword in material '%s'\n", hashName.c_str());
            }
        }

        return true;
    }

    BE_WARNLOG("missing parameter for blendFunc keyword in material '%s'\n", hashName.c_str());
    return false;
}

void Material::Finish() {
    if (type == Type::Surface) {
        if (!pass->shader) {
            if (pass->renderingMode == RenderingMode::AlphaBlend) {
                sort = Sort::Overlay;
            } else if (pass->renderingMode == RenderingMode::AlphaCutoff) {
                sort = Sort::AlphaTest;
            } else {
                sort = Sort::Opaque;
            }
        } else if (pass->shader->GetFlags() & Shader::Flag::SkySurface) {
            sort = Sort::Sky;
        } else if (pass->shader->GetFlags() & Shader::Flag::LitSurface) {
            if (pass->renderingMode == RenderingMode::AlphaBlend) {
                sort = Sort::Translucent;
            } else if (pass->renderingMode == RenderingMode::AlphaCutoff) {
                sort = Sort::AlphaTest;
            } else {
                sort = Sort::Opaque;
            }
        } else {
            if (pass->renderingMode == RenderingMode::AlphaBlend) {
                sort = Sort::Overlay;
            } else if (pass->renderingMode == RenderingMode::AlphaCutoff) {
                sort = Sort::AlphaTest;
            } else {
                sort = Sort::Opaque;
            }
        }
    } else {
        sort = Sort::Bad;
    }
}

void Material::Write(const char *filename) {
    File *fp = fileSystem.OpenFile(filename, File::Mode::Write);
    if (!fp) {
        BE_WARNLOG("Material::Write: file open error\n");
        return;
    }

    Str indentSpace;

    fp->Printf("material %i\n", Material::Version);

    Str lightMaterialTypeStr;
    if (type == Type::Surface) {
    } else if (type == Type::Decal) {
        fp->Printf("%sdecal\n", indentSpace.c_str());
    } else if (type == Type::Light) {
        fp->Printf("%slight\n", indentSpace.c_str());
    } else if (type == Type::BlendLight) {
        fp->Printf("%sblendLight\n", indentSpace.c_str());
    } else if (type == Type::FogLight) {
        fp->Printf("%sfogLight\n", indentSpace.c_str());
    }

    if (flags & Flag::NoShadow) {
        fp->Printf("%snoShadow\n", indentSpace.c_str());
    }

    if (flags & Flag::ForceShadow) {
        fp->Printf("%sforceShadow\n", indentSpace.c_str());
    }

    if (flags & Flag::UnsmoothTangents) {
        fp->Printf("%sunsmoothTangents\n", indentSpace.c_str());
    }

    if (flags & Flag::PolygonOffset) {
        fp->Printf("%spolygonOffset\n", indentSpace.c_str());
    }

    fp->Printf("%spass {\n", indentSpace.c_str());
    indentSpace += "  ";

    Str renderingModeStr;
    switch (pass->renderingMode) {
    case RenderingMode::Opaque: 
        renderingModeStr = "opaque"; 
        break;
    case RenderingMode::AlphaCutoff: 
        renderingModeStr = "alphaCutoff"; 
        break;
    case RenderingMode::AlphaBlend: 
        renderingModeStr = "alphaBlend"; 
        break;
    }
    fp->Printf("%srenderingMode %s\n", indentSpace.c_str(), renderingModeStr.c_str());

    Str cullStr;
    switch (pass->cullType) {
    case RHI::CullType::Back:
        cullStr = "back";
        break;
    case RHI::CullType::Front:
        cullStr = "front";
        break;
    case RHI::CullType::None:
    default: 
        cullStr = "none";
        break;
    }
    fp->Printf("%scull %s\n", indentSpace.c_str(), cullStr.c_str());

    Str transparencyStr;
    switch (pass->transparency) {
    case Transparency::Default:
        transparencyStr = "default";
        break;
    case Transparency::TwoPassesOneSide:
        transparencyStr = "twoPassesOneSide";
        break;
    case Transparency::TwoPassesTwoSides:
    default: 
        transparencyStr = "twoPassesTwoSides";
        break;
    }
    fp->Printf("%stransparency %s\n", indentSpace.c_str(), transparencyStr.c_str());

    if (pass->shader) {
        const Guid shaderGuid = resourceGuidMapper.Get(pass->shader->GetOriginalShader()->GetHashName());
        fp->Printf("%sshader \"%s\" {\n", indentSpace.c_str(), shaderGuid.ToString(Guid::Format::DigitsWithHyphensInBraces));
        indentSpace += "  ";

        const auto &shaderPropertyInfos = pass->shader->GetOriginalShader()->GetPropertyInfoHashMap();
        
        for (int i = 0; i < shaderPropertyInfos.Count(); i++) {
            const auto *keyValue = shaderPropertyInfos.GetByIndex(i);
            const PropertyInfo &propInfo = keyValue->second;
            const char *name = propInfo.GetName();
            const auto *shaderPropEntry = pass->shaderProperties.Get(name);
            const Variant &value = shaderPropEntry->second.data;

            // Skip writing useless properties in shader.
            if (!(propInfo.GetFlags() & PropertyInfo::Flag::ShaderDefine)) {
                if (!pass->shader->IsPropertyUsed(name, pass->shaderProperties)) {
                    continue;
                }
            }

            switch (propInfo.GetType()) {
            case Variant::Type::Float:
                fp->Printf("%s%s \"%.4f\"\n", indentSpace.c_str(), name, value.As<float>());
                break;
            case Variant::Type::Int:
                fp->Printf("%s%s \"%i\"\n", indentSpace.c_str(), name, value.As<int>());
                break;
            case Variant::Type::Guid:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<Guid>().ToString(Guid::Format::DigitsWithHyphensInBraces));
                break;
            case Variant::Type::Bool:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<bool>() ? "true" : "false");
                break;
            case Variant::Type::Point:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<Point>().ToString());
                break;
            case Variant::Type::Size:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<Size>().ToString());
                break;
            case Variant::Type::Rect:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<Rect>().ToString());
                break;
            case Variant::Type::Vec2:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<Vec2>().ToString());
                break;
            case Variant::Type::Vec3:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<Vec3>().ToString());
                break;
            case Variant::Type::Vec4:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<Vec4>().ToString());
                break;
            case Variant::Type::Color3:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<Color3>().ToString());
                break;
            case Variant::Type::Color4:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<Color4>().ToString());
                break;
            default:
                break;
            }
        }

        indentSpace.Truncate(indentSpace.Length() - 2);
        fp->Printf("%s}\n", indentSpace.c_str());
    } else if (pass->texture) {
        const Guid textureGuid = resourceGuidMapper.Get(pass->texture->GetHashName());
        assert(!textureGuid.IsZero());
        fp->Printf("%smap \"%s\"\n", indentSpace.c_str(), textureGuid.ToString());
    }

    fp->Printf("%stc (%.3f %.3f) (%.3f %.3f)\n", indentSpace.c_str(), pass->tcScale[0], pass->tcScale[1], pass->tcTranslation[0], pass->tcTranslation[1]);     

    fp->Printf("%simageBorders %i %i %i %i\n", indentSpace.c_str(), pass->imageBorderLT.x, pass->imageBorderLT.y, pass->imageBorderRB.x, pass->imageBorderRB.y);

    int colorMask = pass->stateBits & RHI::MaskColor;
    if (colorMask) {
        Str colorMaskStr;
        if (colorMask & RHI::RedWrite) {
            colorMaskStr += "R";
        }
        if (colorMask & RHI::GreenWrite) {
            colorMaskStr += "G";
        }
        if (colorMask & RHI::BlueWrite) {
            colorMaskStr += "B";
        }
        if (colorMask & RHI::AlphaWrite) {
            colorMaskStr += "A";
        }        
        fp->Printf("%scolorMask %s\n", indentSpace.c_str(), colorMaskStr.c_str());
    }

    if (pass->renderingMode == RenderingMode::AlphaCutoff) {
        fp->Printf("%scutoffAlpha %.3f\n", indentSpace.c_str(), pass->cutoffAlpha);
    }

    if (pass->depthTestBits != RHI::DF_LEqual) {
        Str depthTestStr;
        switch (pass->depthTestBits) {
        case RHI::DF_Less: depthTestStr = "LESS"; break;
        case RHI::DF_Greater: depthTestStr = "GREATER"; break;
        case RHI::DF_LEqual: depthTestStr = "LEQUAL"; break;
        case RHI::DF_GEqual: depthTestStr = "GEQUAL"; break;
        case RHI::DF_Equal: depthTestStr = "EQUAL"; break;
        case RHI::DF_NotEqual: depthTestStr = "NOTEQUAL"; break;
        case RHI::DF_Always: depthTestStr = "ALWAYS"; break;
        }

        fp->Printf("%sdepthTest %s\n", indentSpace.c_str(), depthTestStr.c_str());
    }

    int blendFuncMask = pass->stateBits & RHI::MaskBF;
    if (blendFuncMask) {
        int blendSrc = blendFuncMask & RHI::MaskBS;
        int blendDst = blendFuncMask & RHI::MaskBD;

        Str blendSrcStr;
        switch (blendSrc) {
        case RHI::BS_Zero: blendSrcStr = "ZERO"; break;
        case RHI::BS_One: blendSrcStr = "ONE"; break;
        case RHI::BS_DstColor: blendSrcStr = "DST_COLOR"; break;
        case RHI::BS_OneMinusDstColor: blendSrcStr = "ONE_MINUS_DST_COLOR"; break;
        case RHI::BS_SrcAlpha: blendSrcStr = "SRC_ALPHA"; break;
        case RHI::BS_OneMinusSrcAlpha: blendSrcStr = "ONE_MINUS_SRC_ALPHA"; break;
        case RHI::BS_DstAlpha: blendSrcStr = "DST_ALPHA"; break;
        case RHI::BS_OneMinusDstAlpha: blendSrcStr = "ONE_MINUS_DST_ALPHA"; break;
        case RHI::BS_SrcAlphaSaturate: blendSrcStr = "SRC_ALPHA_SATURATE"; break;
        }

        Str blendDstStr;
        switch (blendDst) {
        case RHI::BD_Zero: blendDstStr = "ZERO"; break;
        case RHI::BD_One: blendDstStr = "ONE"; break;
        case RHI::BD_SrcColor: blendDstStr = "SRC_COLOR"; break;
        case RHI::BD_OneMinusSrcColor: blendDstStr = "ONE_MINUS_SRC_COLOR"; break;
        case RHI::BD_SrcAlpha: blendDstStr = "SRC_ALPHA"; break;
        case RHI::BD_OneMinusSrcAlpha: blendDstStr = "ONE_MINUS_SRC_ALPHA"; break;
        case RHI::BD_DstAlpha: blendDstStr = "DST_ALPHA"; break;
        case RHI::BD_OneMinusDstAlpha: blendDstStr = "ONE_MINUS_DST_ALPHA"; break;
        }

        fp->Printf("%sblendFunc %s %s\n", indentSpace.c_str(), blendSrcStr.c_str(), blendDstStr.c_str());
    }

    if (pass->vertexColorMode != VertexColorMode::Ignore) {
        Str vertexColorModeStr;
        switch (pass->vertexColorMode) {
        case VertexColorMode::Modulate: vertexColorModeStr = "vertexColor"; break;
        case VertexColorMode::InverseModulate: vertexColorModeStr = "inverseVertexColor"; break;
        default: assert(0); break;
        }
        fp->Printf("%s%s\n", indentSpace.c_str(), vertexColorModeStr.c_str());
    }

    if (pass->useOwnerColor) {
        fp->Printf("%suseOwnerColor\n", indentSpace.c_str());
    }

    if (pass->instancingEnabled) {
        fp->Printf("%sinstancingEnabled\n", indentSpace.c_str());
    }

    fp->Printf("%scolor (%s)\n", indentSpace.c_str(), pass->constantColor.ToString());
    indentSpace.Truncate(indentSpace.Length() - 2);

    fp->Printf("%s}\n", indentSpace.c_str());

    fileSystem.CloseFile(fp);
}

void Material::SetRenderingMode(RenderingMode::Enum mode) {
    pass->renderingMode = mode;

    Finish();
}

bool Material::IsLitSurface() const {
    if (pass->shader && (pass->shader->GetFlags() & Shader::Flag::LitSurface)) {
        return true;
    }

    return false;
}

bool Material::IsSkySurface() const {
    if (pass->shader && (pass->shader->GetFlags() & Shader::Flag::SkySurface)) {
        return true;
    }

    return false;
}

bool Material::IsShadowCaster() const {
    if (flags & Flag::NoShadow) {
        return false;
    }

    if (!pass->shader) {
        return false;
    }

    if (!(pass->shader->GetFlags() & Shader::Flag::LitSurface) || (pass->shader->GetFlags() & Shader::Flag::SkySurface)) {
        return false;
    }

    if (pass->renderingMode == RenderingMode::AlphaBlend) {
        return false;
    }

    return true;
}
/*
void Material::MultiplyTextureMatrix(Pass *pass, int inMatrix[2][3]) {
    int resultMatrix[2][3];
    char *expr;

    if (pass->hasTextureMatrix) {
        expr = va("r%i * r%i + r%i * r%i", 
            GET_REG_INDEX(inMatrix[0][0]), GET_REG_INDEX(pass->textureMatrix[0][0]), 
            GET_REG_INDEX(inMatrix[0][1]), GET_REG_INDEX(pass->textureMatrix[1][0]));
        exprChunk->ParseExpressions(expr, 1, &resultMatrix[0][0]);

        expr = va("r%i * r%i + r%i * r%i", 
            GET_REG_INDEX(inMatrix[0][0]), GET_REG_INDEX(pass->textureMatrix[0][1]), 
            GET_REG_INDEX(inMatrix[0][1]), GET_REG_INDEX(pass->textureMatrix[1][1]));
        exprChunk->ParseExpressions(expr, 1, &resultMatrix[0][1]);

        expr = va("r%i * r%i + r%i * r%i + r%i", 
            GET_REG_INDEX(inMatrix[0][0]), GET_REG_INDEX(pass->textureMatrix[0][2]), 
            GET_REG_INDEX(inMatrix[0][1]), GET_REG_INDEX(pass->textureMatrix[1][2]), GET_REG_INDEX(inMatrix[0][2]));
        exprChunk->ParseExpressions(expr, 1, &resultMatrix[0][2]);

        expr = va("r%i * r%i + r%i * r%i", 
            GET_REG_INDEX(inMatrix[1][0]), GET_REG_INDEX(pass->textureMatrix[0][0]), 
            GET_REG_INDEX(inMatrix[1][1]), GET_REG_INDEX(pass->textureMatrix[1][0]));
        exprChunk->ParseExpressions(expr, 1, &resultMatrix[1][0]);

        expr = va("r%i * r%i + r%i * r%i", 
            GET_REG_INDEX(inMatrix[1][0]), GET_REG_INDEX(pass->textureMatrix[0][1]), 
            GET_REG_INDEX(inMatrix[1][1]), GET_REG_INDEX(pass->textureMatrix[1][1]));
        exprChunk->ParseExpressions(expr, 1, &resultMatrix[1][1]);

        expr = va("r%i * r%i + r%i * r%i + r%i", 
            GET_REG_INDEX(inMatrix[1][0]), GET_REG_INDEX(pass->textureMatrix[0][2]), 
            GET_REG_INDEX(inMatrix[1][1]), GET_REG_INDEX(pass->textureMatrix[1][2]), GET_REG_INDEX(inMatrix[1][2]));
        exprChunk->ParseExpressions(expr, 1, &resultMatrix[1][2]);

        memcpy(pass->textureMatrix, resultMatrix, sizeof(resultMatrix));
    } else {
        memcpy(pass->textureMatrix, inMatrix, sizeof(pass->textureMatrix));	// NOTE: sizeof(inMatrix) == 4 로 나옴
        pass->hasTextureMatrix = true;
    }
}

//--------------------------------------------------------------------------------------------------
//
//                  | [0] [1]  0  [2] |
// texture matrix = | [3] [4]  0  [5] |
//                  |  0   0   1   0  |
//                  |  0   0   0   1  |
//    
//--------------------------------------------------------------------------------------------------
bool Material::ShaderPass::GetTextureMatrix(const float *evalShaderRegisters, float outMatrix[2][4]) const {
    if (!this->hasTextureMatrix) {
        outMatrix[0][0] = 1.0f;
        outMatrix[0][1] = 0.0f;
        outMatrix[0][2] = 0.0f;
        outMatrix[0][3] = 0.0f;

        outMatrix[1][0] = 0.0f;
        outMatrix[1][1] = 1.0f;
        outMatrix[1][2] = 0.0f;
        outMatrix[1][3] = 0.0f;

        return false;
    }

    for (int i = 0; i < 3; i++) {
        outMatrix[0][i] = evalShaderRegisters[GET_REG_INDEX(this->textureMatrix[0][i])];	// [0] [1] [2]
    }
    outMatrix[0][3] = outMatrix[0][2];
    outMatrix[0][2] = 0.0f;

    for (int i = 0; i < 3; i++) {
        outMatrix[1][i] = evalShaderRegisters[GET_REG_INDEX(this->textureMatrix[1][i])];	// [0] [1] [2]
    }
    outMatrix[1][3] = outMatrix[1][2];
    outMatrix[1][2] = 0.0f;

    return true;
}
*/

bool Material::Load(const char *hashName) {
    char *data;
    int size = (int)fileSystem.LoadFile(hashName, true, (void **)&data);
    if (!data) {
        return false;
    }

    Lexer lexer;
    lexer.Init(Lexer::Flag::NoErrors);
    lexer.Load(data, size, hashName);

    if (!lexer.ExpectTokenString("material")) {
        fileSystem.FreeFile(data);
        return false;
    }

    this->version = lexer.ParseInt();

    if (version < 1) {
        lexer.Error("Material::Load: Invalid version %d\n", version);
        fileSystem.FreeFile(data);
        return false;
    }

    if (!Create(data + lexer.GetCurrentOffset())) {
        return false;
    }

    fileSystem.FreeFile(data);

    this->flags |= Flag::LoadedFromFile;

    return true;
}

bool Material::Reload() {
    Str _hashName = this->hashName;
    bool ret = Load(_hashName);

    return ret;
}

BE_NAMESPACE_END
