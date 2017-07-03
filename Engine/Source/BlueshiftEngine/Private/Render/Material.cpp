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
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Render/Render.h"
#include "RenderInternal.h"
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
    coverage = EmptyCoverage;

    Lexer lexer; 
    lexer.Init(LexerFlag::LEXFL_NOERRORS);
    lexer.Load(text, Str::Length(text), hashName.c_str());

    while (lexer.ReadToken(&token)) {
        if (token.IsEmpty()) {
            break;
        } else if (!token.Icmp("pass")) {
            pass = new Pass;
            if (!ParsePass(lexer, pass)) {
                delete pass;
                pass = nullptr;
                return false;
            }

            if (pass->stateBits & RHI::MaskBF) {
                coverage |= TranslucentCoverage;
            } else if (pass->stateBits & RHI::MaskAF) {
                coverage |= PerforatedCoverage;
            } else {
                coverage |= OpaqueCoverage;
            }
        } else if (!token.Icmp("cull")) {
            if (lexer.ReadToken(&token, false)) {
                if (!token.Icmp("none") || !token.Icmp("disable") || !token.Icmp("twoSided")) {
                    cullType = RHI::NoCull;
                } else if (!token.Icmp("back") || !token.Icmp("backSide") || !token.Icmp("backSided")) {
                    cullType = RHI::BackCull;
                } else if (!token.Icmp("front") || !token.Icmp("frontSide") || !token.Icmp("frontSided")) {
                    cullType = RHI::FrontCull;
                } else {
                    BE_WARNLOG(L"invalid cull parm '%hs' in material '%hs'\n", token.c_str(), hashName.c_str());
                }
            } else {
                BE_WARNLOG(L"missing parameter cull keyword in material '%hs'\n", hashName.c_str());
            }
        } else if (!token.Icmp("overlay")) {
            flags |= (Overlay | NoShadow);
        } else if (!token.Icmp("noShadow")) {
            flags |= NoShadow;
        } else if (!token.Icmp("forceShadow")) {
            flags |= ForceShadow;
        } else if (!token.Icmp("unsmoothTangents")) {
            flags |= UnsmoothTangents;
        } else if (!token.Icmp("polygonOffset")) {
            flags |= PolygonOffset;
        } else if (!token.Icmp("sort")) {
            ParseSort(lexer);
        } else if (!token.Icmp("lightSort")) {
            ParseLightSort(lexer);
        } else {
            BE_WARNLOG(L"unknown general material parameter '%hs' in material '%hs'\n", token.c_str(), hashName.c_str());
            lexer.SkipRestOfLine();
        }
    }
      
    Finish();

    return true;
}

bool Material::ParsePass(Lexer &lexer, Pass *pass) {
    int blendSrc = 0;
    int blendDst = 0;
    int alphaFunc = 0;
    int depthFunc = RHI::DF_LEqual;
    int colorWrite = RHI::RedWrite | RHI::GreenWrite | RHI::BlueWrite;
    int depthWrite = RHI::DepthWrite;
    Dict propDict;
    Str token;

    pass->stateBits         = 0;
    pass->alphaRef          = 0;
    pass->vertexColorMode   = IgnoreVertexColor;
    pass->useOwnerColor     = false;
    pass->constantColor[0]  = 1.0f;
    pass->constantColor[1]  = 1.0f;
    pass->constantColor[2]  = 1.0f;
    pass->constantColor[3]  = 1.0f;
    pass->texture           = nullptr;
    pass->tcScale[0]        = 1.0f;
    pass->tcScale[1]        = 1.0f;
    pass->tcTranslation[0]  = 0.0f;
    pass->tcTranslation[1]  = 0.0f;
    pass->shader            = nullptr;
    pass->referenceShader   = nullptr;

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
        } else if (!token.Icmp("shader")) {
            if (lexer.ReadToken(&token, false)) {
                const Guid shaderGuid = Guid::ParseString(token);
                const Str shaderPath = resourceGuidMapper.Get(shaderGuid);
                pass->referenceShader = shaderManager.GetShader(shaderPath);

                if (!ParseShaderProperties(lexer, propDict)) {
                    if (pass->referenceShader) {
                        shaderManager.ReleaseShader(pass->referenceShader);
                    }
                    return false;
                }

                if (pass->referenceShader) {
                    const auto &shaderSpecs = pass->referenceShader->GetSpecHashMap();

                    for (int i = 0; i < shaderSpecs.Count(); i++) {
                        const auto entry = shaderSpecs.GetByIndex(i);
                        const auto &shaderSpecKey = entry->first;
                        const auto &shaderSpec = entry->second;

                        Shader::Property shaderProp;

                        if (shaderSpec.GetType() == PropertySpec::ObjectType) {
                            if (shaderSpec.GetMetaObject() == &TextureAsset::metaObject) {
                                const Texture *defaultTexture = textureManager.FindTexture(shaderSpec.GetDefaultValue());
                                assert(defaultTexture);
                                const Guid defaultTextureGuid = resourceGuidMapper.Get(defaultTexture->GetHashName());

                                shaderProp.data = PropertySpec::ToVariant(shaderSpec.GetType(), propDict.GetString(shaderSpecKey, defaultTextureGuid.ToString()));
                                const Guid textureGuid = shaderProp.data.As<Guid>();
                                const Str texturePath = resourceGuidMapper.Get(textureGuid);
                                shaderProp.texture = textureManager.GetTexture(texturePath);
                            }
                        } else {
                            shaderProp.data = PropertySpec::ToVariant(shaderSpec.GetType(), propDict.GetString(shaderSpecKey, shaderSpec.GetDefaultValue()));
                            shaderProp.texture = nullptr;
                        }

                        pass->shaderProperties.Set(shaderSpecKey, shaderProp);
                    }

                    EndShaderPropertiesChanged();
                }
            } else {
                BE_WARNLOG(L"missing shader name in material '%hs'\n", hashName.c_str());
            }
        } else if (!token.Icmp("map")) {
            if (lexer.ReadToken(&token, false)) {
                const Guid textureGuid = Guid::ParseString(token);
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
        } else if (!token.Icmp("alphaFunc")) {
            ParseAlphaFunc(lexer, &alphaFunc, pass);
        } else if (!token.Icmp("depthFunc")) {
            ParseDepthFunc(lexer, &depthFunc);
        } else if (!token.Icmp("blendFunc")) {
            if (ParseBlendFunc(lexer, &blendSrc, &blendDst)) {
                depthWrite = 0; // depth write off when blendFunc is valid
            }
        } else if (!token.Icmp("noDepthWrite")) {
            depthWrite = 0;
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

    pass->stateBits = blendSrc | blendDst | alphaFunc | depthFunc | colorWrite | depthWrite;

    return true;
}

void Material::ChangeShader(Shader *shader) {
    const auto &oldSpecHashMap = pass->referenceShader ? pass->referenceShader->GetSpecHashMap() : StrHashMap<PropertySpec>();
    const auto &newSpecHashMap = shader->GetSpecHashMap();

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

    for (int i = 0; i < newSpecHashMap.Count(); i++) {
        const auto newEntry = newSpecHashMap.GetByIndex(i);
        const auto &key = newEntry->first;
        const auto &spec = newEntry->second;

        const auto oldEntry = oldSpecHashMap.Get(key);
        if (oldEntry && oldEntry->second.GetType() == newEntry->second.GetType()) {
            const auto &oldProp = pass->shaderProperties.Get(key)->second;

            newShaderProperties.Set(key, oldProp);
        } else {
            Shader::Property prop;

            if (spec.GetType() == PropertySpec::ObjectType) {
                if (spec.GetMetaObject() == &TextureAsset::metaObject) {
                    Texture *defaultTexture = textureManager.FindTexture(spec.GetDefaultValue());
                    assert(defaultTexture);
                    const Guid defaultTextureGuid = resourceGuidMapper.Get(defaultTexture->GetHashName());

                    prop.data = defaultTextureGuid;
                }
            } else {
                prop.data = PropertySpec::ToVariant(spec.GetType(), spec.GetDefaultValue());
            }

            prop.texture = nullptr;

            newShaderProperties.Set(key, prop);
        }
    }

    pass->shaderProperties.Swap(newShaderProperties);

    const Texture *tt = pass->shaderProperties.Get("diffuseMap")->second.texture;

    // Instantiate shader with changed define properites 
    EndShaderPropertiesChanged();
}

void Material::EndShaderPropertiesChanged() {
    if (pass->shader) {
        shaderManager.ReleaseShader(pass->shader);
    }

    const auto &shaderSpecs = pass->referenceShader->GetSpecHashMap();

    Array<Shader::Define> defineArray;

    // List up define list for re-instantiating shader
    for (int i = 0; i < shaderSpecs.Count(); i++) {
        const auto entry = shaderSpecs.GetByIndex(i);
        const auto &shaderSpecKey = entry->first;
        const auto &shaderSpec = entry->second;

        // property spec with shaderDefine allows only bool/enum type
        if (shaderSpec.GetFlags() & PropertySpec::ShaderDefine) {
            const auto *entry = pass->shaderProperties.Get(shaderSpecKey);
            const Shader::Property &shaderProp = entry->second;

            if (shaderSpec.GetType() == PropertySpec::BoolType) {
                if (shaderProp.data.As<bool>()) {
                    defineArray.Append(Shader::Define(shaderSpecKey, 1));
                }
            } else if (shaderSpec.GetType() == PropertySpec::EnumType) {
                int enumIndex = shaderProp.data.As<int>();
                defineArray.Append(Shader::Define(shaderSpecKey, enumIndex));
            }
        }
    }    
    
    // 작성된 shader define list 로 instantiate
    pass->shader = pass->referenceShader->InstantiateShader(defineArray);

    // Reload shader's texture
    for (int i = 0; i < shaderSpecs.Count(); i++) {
        const auto entry = shaderSpecs.GetByIndex(i);
        const auto &shaderSpecKey = entry->first;
        const auto &shaderSpec = entry->second;

        if (shaderSpec.GetType() == PropertySpec::ObjectType) {
            if (shaderSpec.GetMetaObject() == &TextureAsset::metaObject) {
                auto *entry = pass->shaderProperties.Get(shaderSpecKey);
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

bool Material::ParseAlphaFunc(Lexer &lexer, int *alphaFunc, Pass *pass) const {
    Str	token;

    if (lexer.ReadToken(&token, false)) {
        if (!token.Icmp("Greater") || !token.Icmp("GT")) {
            *alphaFunc = RHI::AF_Greater;
        } else if (!token.Icmp("GreaterEqual") || !token.Icmp("GE")) {
            *alphaFunc = RHI::AF_GEqual;
        } else if (!token.Icmp("Less") || !token.Icmp("LT")) {
            *alphaFunc = RHI::AF_Less;
        } else if (!token.Icmp("LessEqual") || !token.Icmp("LE")) {
            *alphaFunc = RHI::AF_LEqual;
        } else {
            BE_WARNLOG(L"unknown alphaFunc name '%hs' in material '%hs'\n", token.c_str(), hashName.c_str());
        }

        pass->alphaRef = lexer.ParseFloat();

        return true;
    }

    BE_WARNLOG(L"missing parameter for alphaFunc keyword in material '%hs'\n", hashName.c_str());
    return false;
}

bool Material::ParseDepthFunc(Lexer &lexer, int *depthFunc) const {
    Str	token;

    if (lexer.ReadToken(&token, false)) {
        if (!token.Icmp("Always")) {
            *depthFunc = RHI::DF_Always;
        } else if (!token.Icmp("LessEqual") || !token.Icmp("LE")) {
            *depthFunc = RHI::DF_LEqual;
        } else if (!token.Icmp("Equal") || !token.Icmp("EQ")) {
            *depthFunc = RHI::DF_Equal;
        } else if (!token.Icmp("Less") || !token.Icmp("LT")) {
            *depthFunc = RHI::DF_Less;
        } else if (!token.Icmp("GreaterEqual") || !token.Icmp("GE")) {
            *depthFunc = RHI::DF_GEqual;
        } else if (!token.Icmp("Greater") || !token.Icmp("GT")) {
            *depthFunc = RHI::DF_Greater;
        } else {
            BE_WARNLOG(L"unknown depthFunc '%hs' in material '%hs'\n", token.c_str(), hashName.c_str());
        }

        return true;
    }

    BE_WARNLOG(L"missing parameter for depthFunc keyword in material '%hs\n", hashName.c_str());
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

bool Material::ParseSort(Lexer &lexer) {
    Str	token;

    if (lexer.ReadToken(&token, false)) {
        if (!token.Icmp("subview")) {
            sort = SubViewSort;
        } else if (!token.Icmp("sky")) {
            sort = SkySort;
            coverage |= BackgroundCoverage;
        } else if (!token.Icmp("opaque")) {
            sort = OpaqueSort;
        } else if (!token.Icmp("decal")) {
            sort = DecalSort;
        } else if (!token.Icmp("additiveLighting")) {
            sort = AdditiveLightingSort;
        } else if (!token.Icmp("blend")) {
            sort = BlendSort;
        } else if (!token.Icmp("nearest")) {
            sort = NearestSort;
        } else {
            sort = (Sort)atoi(token.c_str());
        }
    } else {
        BE_WARNLOG(L"missing parameter sort keyword in material '%hs'\n", hashName.c_str());
        return false;
    }

    return true;
}

bool Material::ParseLightSort(Lexer &lexer) {
    Str	token;

    if (lexer.ReadToken(&token, false)) {
        if (!token.Icmp("light")) {
            lightSort = NormalLightSort;
        } else if (!token.Icmp("blendLight")) {
            lightSort = BlendLightSort;
        } else if (!token.Icmp("fogLight")) {
            lightSort = FogLightSort;
        }
    } else {
        BE_WARNLOG(L"missing parameter lightSort keyword in material '%hs'\n", hashName.c_str());
        return false;
    }

    return true;
}

void Material::Finish() {
    if (lightSort != BadLightSort) {
        type = LightType;
    } else if (pass->shader) {
        type = LitSurfaceType;
    } else if (pass->texture) {
        type = StandardType;
    } else {
        type = BadType;
    }

    if (flags & PolygonOffset) {
        if (!sort) {
            sort = DecalSort;
        }
    }

    if (sort == BadSort) {
        if (pass->stateBits & RHI::MaskBF) {
            sort = BlendSort;
        } /*else if (pass.stateBits & RHI::MaskAF) {
          sort = AlphaTestSort;
        } */else {
              sort = OpaqueSort;
        }
    }		

    if (!cullType) {
        cullType = RHI::BackCull;
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

    Str sortStr;
    switch (sort) {
    case SubViewSort: sortStr = "subView"; break;
    case SkySort: sortStr = "sky"; break;
    case OpaqueSort: sortStr = "opaque"; break;
    case DecalSort: sortStr = "decal"; break;
    case AdditiveLightingSort: sortStr = "additiveLighting"; break;
    case BlendSort: sortStr = "blend"; break;
    case NearestSort: sortStr = "nearest"; break;
    default: sortStr = Str((int)sort); break;
    }
    fp->Printf("%ssort %s\n", indentSpace.c_str(), sortStr.c_str());

    Str lightSortStr;
    switch (lightSort) {
    case NormalLightSort: lightSortStr = "light"; break;
    case BlendLightSort: lightSortStr = "blendLight"; break;
    case FogLightSort: lightSortStr = "fogLight"; break;
    default: lightSortStr = Str((int)sort); break;
    }
    fp->Printf("%slightSort %s\n", indentSpace.c_str(), lightSortStr.c_str());

    Str cullStr;
    switch (cullType) {
    case RHI::BackCull: cullStr = "back"; break;
    case RHI::FrontCull: cullStr = "front"; break;
    case RHI::NoCull: default: cullStr = "none"; break;
    }
    fp->Printf("%scull %s\n", indentSpace.c_str(), cullStr.c_str());

    if (flags & Overlay) {
        fp->Printf("%soverlay\n", indentSpace.c_str());
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

    if (pass->referenceShader) {
        const Guid shaderGuid = resourceGuidMapper.Get(pass->referenceShader->GetHashName());
        fp->Printf("%sshader \"%s\" {\n", indentSpace.c_str(), shaderGuid.ToString());
        indentSpace += "  ";

        const auto &specHashMap = pass->referenceShader->GetSpecHashMap();
        
        for (int i = 0; i < specHashMap.Count(); i++) {
            const auto *keyValue = specHashMap.GetByIndex(i);
            const PropertySpec &spec = keyValue->second;
            const char *name = spec.GetName();
            const auto *shaderPropEntry = pass->shaderProperties.Get(name);
            const auto &value = shaderPropEntry->second.data;
            
            switch (spec.GetType()) {
            case PropertySpec::FloatType:
                fp->Printf("%s%s \"%.4f\"\n", indentSpace.c_str(), name, value.As<float>());
                break;
            case PropertySpec::IntType:
            case PropertySpec::EnumType:
                fp->Printf("%s%s \"%i\"\n", indentSpace.c_str(), name, value.As<int>());
                break;
            case PropertySpec::ObjectType:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<Guid>().ToString());
                break;
            case PropertySpec::BoolType:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<bool>() ? "true" : "false");
                break;
            case PropertySpec::PointType:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<Point>().ToString());
                break;
            case PropertySpec::RectType:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<Rect>().ToString());
                break;
            case PropertySpec::Vec2Type:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<Vec2>().ToString());
                break;
            case PropertySpec::Vec3Type:
            case PropertySpec::Color3Type:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<Vec3>().ToString());
                break;
            case PropertySpec::Vec4Type:
            case PropertySpec::Color4Type:
                fp->Printf("%s%s \"%s\"\n", indentSpace.c_str(), name, value.As<Vec4>().ToString());
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

    int alphaFuncMask = pass->stateBits & RHI::MaskAF;
    if (alphaFuncMask) {
        Str alphaFuncStr;
        switch (alphaFuncMask) {
        case RHI::AF_LEqual: alphaFuncStr = "LE"; break;
        case RHI::AF_Less: alphaFuncStr = "LT"; break;
        case RHI::AF_GEqual: alphaFuncStr = "GE"; break;
        case RHI::AF_Greater: alphaFuncStr = "GT"; break;
        }
        fp->Printf("%salphaFunc %s %.3f\n", indentSpace.c_str(), alphaFuncStr.c_str(), pass->alphaRef);
    }

    int depthFuncMask = pass->stateBits & RHI::MaskDF;
    if (depthFuncMask) {
        Str depthFuncStr;
        switch (depthFuncMask) {
        case RHI::DF_Equal: depthFuncStr = "EQ"; break;
        case RHI::DF_LEqual: depthFuncStr = "LE"; break;
        case RHI::DF_Less: depthFuncStr = "LT"; break;
        case RHI::DF_GEqual: depthFuncStr = "GE"; break;
        case RHI::DF_Greater: depthFuncStr = "GT"; break;
        }
        fp->Printf("%sdepthFunc %s\n", indentSpace.c_str(), depthFuncStr.c_str());
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

    if (!(pass->stateBits & RHI::DepthWrite)) {
        fp->Printf("%snoDepthWrite\n", indentSpace.c_str());
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

    fp->Printf("%scolor (%s)\n", indentSpace.c_str(), pass->constantColor.ToString());
    indentSpace.Truncate(indentSpace.Length() - 2);

    fp->Printf("%s}\n", indentSpace.c_str());

    fileSystem.CloseFile(fp);
}

bool Material::IsLitSurface() const {
    if (type != StandardType && type != LitSurfaceType) {
        return false;
    }

    return true;
}

bool Material::IsShadowCaster() const {
    if (flags & NoShadow) {
        return false;
    }

    if (type != StandardType && type != LitSurfaceType) {
        return false;
    }

    if (pass->stateBits & RHI::MaskBF) {
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
bool Material::Pass::GetTextureMatrix(const float *evalShaderRegisters, float outMatrix[2][4]) const {
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
    size_t size = fileSystem.LoadFile(hashName, true, (void **)&data);
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
