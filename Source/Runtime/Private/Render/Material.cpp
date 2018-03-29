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
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

#define MATERIAL_VERSION 1

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

        delete pass;
        pass = nullptr;
    }
}

bool Material::Create(const char *text) {
    Str token;

    Purge();

    flags = 0;

    Lexer lexer; 
    lexer.Init(LexerFlag::LEXFL_NOERRORS);
    lexer.Load(text, Str::Length(text), hashName.c_str());

    while (lexer.ReadToken(&token)) {
        if (token.IsEmpty()) {
            break;
        } else if (!token.Icmp("noShadow")) {
            flags |= NoShadow;
        } else if (!token.Icmp("forceShadow")) {
            flags |= ForceShadow;
        } else if (!token.Icmp("unsmoothTangents")) {
            flags |= UnsmoothTangents;
        } else if (!token.Icmp("polygonOffset")) {
            flags |= PolygonOffset;
        } else if (!token.Icmp("decal")) {
            type = DecalMaterialType;
        } else if (!token.Icmp("light")) {
            type = LightMaterialType;
        } else if (!token.Icmp("blendLight")) {
            type = BlendLightMaterialType;
        } else if (!token.Icmp("fogLight")) {
            type = FogLightMaterialType;
        } else if (!token.Icmp("pass")) {
            pass = new ShaderPass;
            if (!ParsePass(lexer, pass)) {
                delete pass;
                pass = nullptr;
                return false;
            }
        } else {
            BE_WARNLOG(L"unknown general material parameter '%hs' in material '%hs'\n", token.c_str(), hashName.c_str());
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

    pass->renderingMode     = RenderingMode::Opaque;
    pass->cullType          = RHI::BackCull;
    pass->stateBits         = 0;
    pass->cutoffAlpha       = 0.004f;
    pass->vertexColorMode   = VertexColorMode::IgnoreVertexColor;
    pass->useOwnerColor     = false;
    pass->texture           = nullptr;
    pass->shader            = nullptr;
    pass->referenceShader   = nullptr;
    pass->constantColor.Set(1.0f, 1.0f, 1.0f, 1.0f);
    pass->tcScale.Set(1.0f, 1.0f);
    pass->tcTranslation.Set(0.0f, 0.0f);

    if (!lexer.ExpectPunctuation(P_BRACEOPEN)) {
        return false;
    }

    Str token;

    while (1) {
        lexer.ReadToken(&token);

        if (token.IsEmpty()) {
            BE_WARNLOG(L"no matching '}' found\n");
            return false;
        } else if (token[0] == '}') {
            break;
        } else if (!token.Icmp("renderingMode")) {
            ParseRenderingMode(lexer, &pass->renderingMode);
        } else if (!token.Icmp("cull")) {
            if (lexer.ReadToken(&token, false)) {
                if (!token.Icmp("none") || !token.Icmp("disable") || !token.Icmp("twoSided")) {
                    pass->cullType = RHI::NoCull;
                } else if (!token.Icmp("back") || !token.Icmp("backSide") || !token.Icmp("backSided")) {
                    pass->cullType = RHI::BackCull;
                } else if (!token.Icmp("front") || !token.Icmp("frontSide") || !token.Icmp("frontSided")) {
                    pass->cullType = RHI::FrontCull;
                } else {
                    BE_WARNLOG(L"invalid cull parm '%hs' in material '%hs'\n", token.c_str(), hashName.c_str());
                }
            } else {
                BE_WARNLOG(L"missing parameter cull keyword in material '%hs'\n", hashName.c_str());
            }
        } else if (!token.Icmp("shader")) {
            if (lexer.ReadToken(&token, false)) {
                const Guid shaderGuid = Guid::FromString(token);
                const Str shaderPath = resourceGuidMapper.Get(shaderGuid);
                pass->referenceShader = shaderManager.GetShader(shaderPath);

                Dict propDict;
                if (!ParseShaderProperties(lexer, propDict)) {
                    if (pass->referenceShader) {
                        shaderManager.ReleaseShader(pass->referenceShader);
                    }
                    return false;
                }

                if (pass->referenceShader) {
                    const auto &shaderPropInfoHashMap = pass->referenceShader->GetPropertyInfoHashMap();

                    for (int i = 0; i < shaderPropInfoHashMap.Count(); i++) {
                        const auto entry = shaderPropInfoHashMap.GetByIndex(i);
                        const auto &propName = entry->first;
                        const auto &propInfo = entry->second;

                        Shader::Property shaderProp;

                        if (propInfo.GetType() == Variant::GuidType) {
                            if (propInfo.GetMetaObject() == &TextureAsset::metaObject) {
                                Str name = resourceGuidMapper.Get(propInfo.GetDefaultValue().As<Guid>());
                                shaderProp.data = Variant::FromString(propInfo.GetType(), propDict.GetString(propName, name));
                                const Guid textureGuid = shaderProp.data.As<Guid>();
                                const Str texturePath = resourceGuidMapper.Get(textureGuid);
                                shaderProp.texture = textureManager.GetTexture(texturePath);
                            }
                        } else {
                            shaderProp.data = Variant::FromString(propInfo.GetType(), propDict.GetString(propName, propInfo.GetDefaultValue().ToString()));
                            shaderProp.texture = nullptr;
                        }

                        pass->shaderProperties.Set(propName, shaderProp);
                    }

                    EndShaderPropertiesChanged();
                }
            } else {
                BE_WARNLOG(L"missing shader name in material '%hs'\n", hashName.c_str());
            }
        } else if (!token.Icmp("map")) {
            if (lexer.ReadToken(&token, false)) {
                const Guid textureGuid = Guid::FromString(token);
                const Str texturePath = resourceGuidMapper.Get(textureGuid);
                pass->texture = textureManager.GetTexture(texturePath);
            } else {
                BE_WARNLOG(L"missing map GUID in material '%hs'\n", hashName.c_str());
            }
        } else if (!token.Icmp("mapPath")) {
            if (lexer.ReadToken(&token, false)) {
                pass->texture = textureManager.FindTexture(token);
                if (!pass->texture) {
                    pass->texture = textureManager.defaultTexture;
                }
                pass->texture->AddRefCount();
            } else {
                BE_WARNLOG(L"missing map name in material '%hs'\n", hashName.c_str());
            }
        } else if (!token.Icmp("tc")) {
            lexer.ParseVec(2, pass->tcScale);
            lexer.ParseVec(2, pass->tcTranslation);
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
                BE_WARNLOG(L"missing color mask in material '%hs'\n", hashName.c_str());
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
            pass->vertexColorMode = ModulateVertexColor;
        } else if (!token.Icmp("inverseVertexColor")) {
            pass->vertexColorMode = InverseModulateVertexColor;
        } else if (!token.Icmp("color")) {
            lexer.ParseVec(4, pass->constantColor);
        } else if (!token.Icmp("useOwnerColor")) {
            pass->useOwnerColor = true;
        } else {
            BE_WARNLOG(L"unknown material pass parameter '%hs' in material '%hs'\n", token.c_str(), hashName.c_str());
            lexer.SkipRestOfLine();
        }
    }

    // TEMP: DST_ALPHA 는 안쓴다
    if (blendSrc == RHI::BS_OneMinusDstAlpha) {
        blendSrc = RHI::BS_Zero;
    }

    if (blendDst == RHI::BD_OneMinusDstAlpha) {
        blendDst = RHI::BD_Zero;
    }

    pass->stateBits = blendSrc | blendDst | colorWrite | depthWrite;

    return true;
}

void Material::ChangeShader(Shader *shader) {
    const auto &oldPropInfoHashMap = pass->referenceShader ? pass->referenceShader->GetPropertyInfoHashMap() : StrHashMap<PropertyInfo>();
    const auto &newPropInfoHashMap = shader->GetPropertyInfoHashMap();

    // Release textures of old shader property
    for (int i = 0; i < pass->shaderProperties.Count(); i++) {
        const auto entry = pass->shaderProperties.GetByIndex(i);
        if (entry->second.texture) {
            textureManager.ReleaseTexture(entry->second.texture);
            entry->second.texture = nullptr;
        }
    }

    // Change new reference shader
    if (pass->referenceShader) {
        shaderManager.ReleaseShader(pass->referenceShader);
    }
    pass->referenceShader = shader;

    // Set shader properties with reusing old shader properties
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
            Shader::Property prop;

            if (propInfo.GetType() == Variant::GuidType) {
                if (propInfo.GetMetaObject() == &TextureAsset::metaObject) {
                    Str name = resourceGuidMapper.Get(propInfo.GetDefaultValue().As<Guid>());
                    Texture *defaultTexture = textureManager.FindTexture(name);
                    assert(defaultTexture);
                    const Guid defaultTextureGuid = resourceGuidMapper.Get(defaultTexture->GetHashName());

                    prop.data = defaultTextureGuid;
                }
            } else {
                prop.data = Variant::FromString(propInfo.GetType(), propInfo.GetDefaultValue().As<Str>());
            }

            prop.texture = nullptr;

            newShaderProperties.Set(key, prop);
        }
    }

    pass->shaderProperties.Swap(newShaderProperties);

    // Instantiate shader with changed define properites 
    EndShaderPropertiesChanged();

    Finish();
}

void Material::EndShaderPropertiesChanged() {
    if (pass->shader) {
        shaderManager.ReleaseShader(pass->shader);
    }

    const auto &shaderPropInfoHashMap = pass->referenceShader->GetPropertyInfoHashMap();

    Array<Shader::Define> defineArray;

    // List up define list for re-instantiating shader
    for (int i = 0; i < shaderPropInfoHashMap.Count(); i++) {
        const auto entry = shaderPropInfoHashMap.GetByIndex(i);
        const auto &propName = entry->first;
        const auto &propInfo = entry->second;

        // property propInfo with shaderDefine allows only bool/enum type
        if (propInfo.GetFlags() & PropertyInfo::ShaderDefineFlag) {
            const auto *entry = pass->shaderProperties.Get(propName);
            const Shader::Property &shaderProp = entry->second;

            if (propInfo.GetType() == Variant::BoolType) {
                if (shaderProp.data.As<bool>()) {
                    defineArray.Append(Shader::Define(propName, 1));
                }
            } else if (propInfo.GetType() == Variant::IntType && propInfo.GetEnum().Count() > 0) {
                int enumIndex = shaderProp.data.As<int>();
                defineArray.Append(Shader::Define(propName, enumIndex));
            }
        }
    }    
    
    // 작성된 shader define list 로 instantiate
    pass->shader = pass->referenceShader->InstantiateShader(defineArray);

    // Reload shader's texture
    for (int i = 0; i < shaderPropInfoHashMap.Count(); i++) {
        const auto entry = shaderPropInfoHashMap.GetByIndex(i);
        const auto &propName = entry->first;
        const auto &propInfo = entry->second;

        if (propInfo.GetType() == Variant::GuidType) {
            if (propInfo.GetMetaObject() == &TextureAsset::metaObject) {
                auto *entry = pass->shaderProperties.Get(propName);
                Shader::Property &shaderProp = entry->second;

                if (shaderProp.texture) {
                    textureManager.ReleaseTexture(shaderProp.texture);
                }
                const Guid textureGuid = shaderProp.data.As<Guid>();
                const Str texturePath = resourceGuidMapper.Get(textureGuid);
                shaderProp.texture = textureManager.GetTexture(texturePath);
            }
        }
    }
}

bool Material::ParseShaderProperties(Lexer &lexer, Dict &properties) {
    Str token;
    Str propValue;

    if (!pass->referenceShader) {
        BE_WARNLOG(L"shader must be specified before shader properties in material '%hs'\n", hashName.c_str());
    }

    if (!lexer.ExpectPunctuation(P_BRACEOPEN)) {
        return false;
    }

    while (1) {
        lexer.ReadToken(&token);

        if (token.IsEmpty()) {
            BE_WARNLOG(L"no matching '}' found\n");
            return false;
        } else if (token[0] == '}') {
            break;
        } else {
            if (lexer.ReadToken(&propValue, false)) {
                properties.Set(token, propValue);
            } else {
                BE_WARNLOG(L"missing property value for property '%hs' in material '%hs'\n", token.c_str(), hashName.c_str());
            }
        }
    }

    return true;
}

bool Material::ParseRenderingMode(Lexer &lexer, RenderingMode *renderingMode) const {
    Str	token;

    if (lexer.ReadToken(&token, false)) {
        if (!token.Icmp("opaque")) {
            *renderingMode = RenderingMode::Opaque;
        } else if (!token.Icmp("alphaCutoff")) {
            *renderingMode = RenderingMode::AlphaCutoff;
        } else if (!token.Icmp("alphaBlend")) {
            *renderingMode = RenderingMode::AlphaBlend;
        } else {
            BE_WARNLOG(L"unknown renderingMode '%hs' in material '%hs'\n", token.c_str(), hashName.c_str());
        }

        return true;
    }

    BE_WARNLOG(L"missing parameter for renderingMode keyword in material '%hs\n", hashName.c_str());
    return false;
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
                BE_WARNLOG(L"unknown blend mode '%hs' in material '%hs', \nsubstituting GL_ONE\n", token.c_str(), hashName.c_str());
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
                    BE_WARNLOG(L"unknown blend mode '%hs' in material '%hs', substituting GL_ONE\n", token.c_str(), hashName.c_str());
                }
            } else {
                BE_WARNLOG(L"missing parameter for blendFunc keyword in material '%hs'\n", hashName.c_str());
            }
        }

        return true;
    }

    BE_WARNLOG(L"missing parameter for blendFunc keyword in material '%hs'\n", hashName.c_str());
    return false;
}

void Material::Finish() {
    if (type == SurfaceMaterialType) {
        if (!pass->shader) {
            if (pass->renderingMode == RenderingMode::AlphaBlend) {
                sort = OverlaySort;
            } else if (pass->renderingMode == RenderingMode::AlphaCutoff) {
                sort = AlphaTestSort;
            } else {
                sort = OpaqueSort;
            }
        } else if (pass->shader->GetFlags() & Shader::SkySurface) {
            sort = SkySort;
        } else if (pass->shader->GetFlags() & Shader::LitSurface) {
            if (pass->renderingMode == RenderingMode::AlphaBlend) {
                sort = TranslucentSort;
            } else if (pass->renderingMode == RenderingMode::AlphaCutoff) {
                sort = AlphaTestSort;
            } else {
                sort = OpaqueSort;
            }
        } else {
            if (pass->renderingMode == RenderingMode::AlphaBlend) {
                sort = OverlaySort;
            } else if (pass->renderingMode == RenderingMode::AlphaCutoff) {
                sort = AlphaTestSort;
            } else {
                sort = OpaqueSort;
            }
        }
    } else {
        sort = BadSort;
    }
}

void Material::Write(const char *filename) {
    File *fp = fileSystem.OpenFile(filename, File::WriteMode);
    if (!fp) {
        BE_WARNLOG(L"Material::Write: file open error\n");
        return;
    }

    Str indentSpace;

    fp->Printf("material %i\n", MATERIAL_VERSION);

    Str lightMaterialTypeStr;
    if (type == SurfaceMaterialType) {
    } else if (type == DecalMaterialType) {
        fp->Printf("%sdecal\n", indentSpace.c_str());
    } else if (type == LightMaterialType) {
        fp->Printf("%slight\n", indentSpace.c_str());
    } else if (type == BlendLightMaterialType) {
        fp->Printf("%sblendLight\n", indentSpace.c_str());
    } else if (type == FogLightMaterialType) {
        fp->Printf("%sfogLight\n", indentSpace.c_str());
    }

    if (flags & NoShadow) {
        fp->Printf("%snoShadow\n", indentSpace.c_str());
    }

    if (flags & ForceShadow) {
        fp->Printf("%sforceShadow\n", indentSpace.c_str());
    }

    if (flags & UnsmoothTangents) {
        fp->Printf("%sunsmoothTangents\n", indentSpace.c_str());
    }

    if (flags & PolygonOffset) {
        fp->Printf("%spolygonOffset\n", indentSpace.c_str());
    }

    fp->Printf("%spass {\n", indentSpace.c_str());
    indentSpace += "  ";

    Str renderingModeStr;
    switch (pass->renderingMode) {
    case RenderingMode::Opaque: renderingModeStr = "opaque"; break;
    case RenderingMode::AlphaCutoff: renderingModeStr = "alphaCutoff"; break;
    case RenderingMode::AlphaBlend: renderingModeStr = "alphaBlend"; break;
    }
    fp->Printf("%srenderingMode %s\n", indentSpace.c_str(), renderingModeStr.c_str());

    Str cullStr;
    switch (pass->cullType) {
    case RHI::BackCull: cullStr = "back"; break;
    case RHI::FrontCull: cullStr = "front"; break;
    case RHI::NoCull: default: cullStr = "none"; break;
    }
    fp->Printf("%scull %s\n", indentSpace.c_str(), cullStr.c_str());

    if (pass->referenceShader) {
        const Guid shaderGuid = resourceGuidMapper.Get(pass->referenceShader->GetHashName());
        fp->Printf("%sshader \"%s\" {\n", indentSpace.c_str(), shaderGuid.ToString());
        indentSpace += "  ";

        const auto &propertyInfoHashMap = pass->referenceShader->GetPropertyInfoHashMap();
        
        for (int i = 0; i < propertyInfoHashMap.Count(); i++) {
            const auto *keyValue = propertyInfoHashMap.GetByIndex(i);
            const PropertyInfo &propInfo = keyValue->second;
            const char *name = propInfo.GetName();
            const auto *shaderPropEntry = pass->shaderProperties.Get(name);
            const auto &value = shaderPropEntry->second.data;
            
            switch (propInfo.GetType()) {
            case Variant::FloatType:
                fp->Printf("%s%s \"%.4f\"\n", indentSpace.c_str(), name, value.As<float>());
                break;
            case Variant::IntType:
                fp->Printf("%s%s \"%i\"\n", indentSpace.c_str(), name, value.As<int>());
                break;
            case Variant::GuidType:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<Guid>().ToString());
                break;
            case Variant::BoolType:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<bool>() ? "true" : "false");
                break;
            case Variant::PointType:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<Point>().ToString());
                break;
            case Variant::RectType:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<Rect>().ToString());
                break;
            case Variant::Vec2Type:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<Vec2>().ToString());
                break;
            case Variant::Vec3Type:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<Vec3>().ToString());
                break;
            case Variant::Vec4Type:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<Vec4>().ToString());
                break;
            case Variant::Color3Type:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<Color3>().ToString());
                break;
            case Variant::Color4Type:
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

    if (pass->vertexColorMode != IgnoreVertexColor) {
        Str vertexColorModeStr;
        switch (pass->vertexColorMode) {
        case ModulateVertexColor: vertexColorModeStr = "vertexColor"; break;
        case InverseModulateVertexColor: vertexColorModeStr = "inverseVertexColor"; break;
        default: assert(0); break;
        }
        fp->Printf("%s%s\n", indentSpace.c_str(), vertexColorModeStr.c_str());
    }

    if (pass->useOwnerColor) {
        fp->Printf("%suseOwnerColor\n", indentSpace.c_str());
    }

    fp->Printf("%scolor (%s)\n", indentSpace.c_str(), pass->constantColor.ToString());
    indentSpace.Truncate(indentSpace.Length() - 2);

    fp->Printf("%s}\n", indentSpace.c_str());

    fileSystem.CloseFile(fp);
}

void Material::SetRenderingMode(RenderingMode mode) {
    pass->renderingMode = mode;

    Finish();
}

bool Material::IsLitSurface() const {
    if (pass->shader && pass->shader->GetFlags() & Shader::LitSurface) {
        return true;
    }

    return false;
}

bool Material::IsSkySurface() const {
    if (pass->shader && pass->shader->GetFlags() & Shader::SkySurface) {
        return true;
    }

    return false;
}

bool Material::IsShadowCaster() const {
    if (flags & NoShadow) {
        return false;
    }

    if (!pass->shader) {
        return false;
    }

    if (!(pass->shader->GetFlags() & Shader::LitSurface) || (pass->shader->GetFlags() & Shader::SkySurface)) {
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
    lexer.Init(LexerFlag::LEXFL_NOERRORS);
    lexer.Load(data, size, hashName);

    if (!lexer.ExpectTokenString("material")) {
        fileSystem.FreeFile(data);
        return false;
    }

    int version = lexer.ParseInt();
    if (version != MATERIAL_VERSION) {
        lexer.Error("Invalid version %d. Should be version %d\n", version, MATERIAL_VERSION);
        fileSystem.FreeFile(data);
        return false;
    }

    Str baseDir = hashName;
    baseDir.StripFileName();
    Create(data + lexer.GetCurrentOffset());

    fileSystem.FreeFile(data);

    this->flags |= LoadedFromFile;

    return true;
}

bool Material::Reload() {
    Str _hashName = this->hashName;
    bool ret = Load(_hashName);

    return ret;
}

BE_NAMESPACE_END
