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
#include "Core/Lexer.h"
#include "File/FileSystem.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"

#if defined __ANDROID__ && ! defined __XAMARIN__
int android_progress = 0;
#endif
BE_NAMESPACE_BEGIN

static const char *directiveInclude = "$include";

// NOTE: BuiltInConstant enum 과 반드시 순서가 같아야 함
// NOTE2: 나중에 자동으로 모든 shader 에 추가되는 구조로 바꾸면 편할 듯
static const char *builtInConstantNames[] = {
    "modelViewMatrix",                      // ModelViewMatrixConst
    "modelViewMatrixTranspose",             // ModelViewMatrixTransposeConst
    "projectionMatrix",                     // ProjectionMatrixConst
    "projectionMatrixTranspose",            // ProjectionMatrixTransposeConst
    "modelViewProjectionMatrix",            // ModelViewProjectionMatrixConst
    "modelViewProjectionMatrixTranspose",   // ModelViewProjectionMatrixTransposeConst
    "worldMatrixS",                         // WorldMatrixSConst
    "worldMatrixT",                         // WorldMatrixTConst
    "worldMatrixR",                         // WorldMatrixRConst
    "textureMatrixS",                       // TextureMatrixSConst
    "textureMatrixT",                       // TextureMatrixTConst
    "constantColor",                        // ConstantColorConst
    "vertexColorScale",                     // VertexColorScaleConst
    "vertexColorAdd",                       // VertexColorAddConst
    "localViewOrigin",                      // LocalViewOriginConst
    "localLightOrigin",                     // LocalLightOriginConst
    "localLightAxis"                        // LocalLightAxisConst
};

// NOTE: BuiltInSampler enum 과 반드시 순서가 같아야 함
static const char *builtInSamplerNames[] = {
    "albedoMap",
    "normalMap"
};

int Shader::GetFlags() const {
    if (originalShader) {
        return originalShader->flags;
    }
    return flags;
}

const StrHashMap<PropertyInfo> &Shader::GetPropertyInfoHashMap() const { 
    if (originalShader) {
        return originalShader->propertyInfoHashMap; 
    }
    return propertyInfoHashMap; 
}

void Shader::Purge() {
    if (shaderHandle != RHI::NullShader) {
        rhi.DeleteShader(shaderHandle);
        shaderHandle = RHI::NullShader;
    }

    if (ambientLitVersion) {
        shaderManager.ReleaseShader(ambientLitVersion);
        ambientLitVersion = nullptr;
    }

    if (directLitVersion) {
        shaderManager.ReleaseShader(directLitVersion);
        directLitVersion = nullptr;
    }

    if (ambientLitDirectLitVersion) {
        shaderManager.ReleaseShader(ambientLitDirectLitVersion);
        ambientLitDirectLitVersion = nullptr;
    }

    if (perforatedVersion) {
        shaderManager.ReleaseShader(perforatedVersion);
        perforatedVersion = nullptr;
    }

    if (premulAlphaVersion) {
        shaderManager.ReleaseShader(premulAlphaVersion);
        premulAlphaVersion = nullptr;
    }

    if (parallelShadowVersion) {
        shaderManager.ReleaseShader(parallelShadowVersion);
        parallelShadowVersion = nullptr;
    }

    if (spotShadowVersion) {
        shaderManager.ReleaseShader(spotShadowVersion);
        spotShadowVersion = nullptr;
    }

    if (pointShadowVersion) {
        shaderManager.ReleaseShader(pointShadowVersion);
        pointShadowVersion = nullptr;
    }

    for (int i = 0; i < COUNT_OF(gpuSkinningVersion); i++) {
        if (gpuSkinningVersion[i]) {
            shaderManager.ReleaseShader(gpuSkinningVersion[i]);
            gpuSkinningVersion[i] = nullptr;
        }
    }

    defineArray.Clear();
    propertyInfoHashMap.Clear();
}

bool Shader::Create(const char *text, const char *baseDir) {
    bool generatePerforatedVersion = false;
    bool generatePremulAlphaVersion = false;
    bool generateGpuSkinningVersion = false;
    bool generateParallelShadowVersion = false;
    bool generatePointShadowVersion = false;
    bool generateSpotShadowVersion = false;

    this->baseDir = baseDir;

    Purge();

    Lexer lexer;
    lexer.Init(LexerFlag::LEXFL_NOERRORS);
    lexer.Load(text, Str::Length(text), hashName);

    // '{'
    if (!lexer.ExpectPunctuation(P_BRACEOPEN)) {
        return false;
    }

    Str token;
    while (lexer.ReadToken(&token)) {
        if (token.IsEmpty()) {
            break;
        } else if (token[0] == '}') {
            break;
        } else if (!token.Icmp("litSurface")) {
            flags |= LitSurface;
        } else if (!token.Icmp("skySurface")) {
            flags |= SkySurface;
        } else if (!token.Icmp("properties")) {
            ParseProperties(lexer);
        } else if (!token.Icmp("inheritProperties")) {
            if (lexer.ReadToken(&token)) {
                Str path = baseDir;
                path.AppendPath(token, '/');

                Shader *shader = shaderManager.FindShader(path);
                if (shader) {
                    propertyInfoHashMap = shader->propertyInfoHashMap;
                }
            } else {
                BE_WARNLOG(L"missing inheritProperties name in shader '%hs'\n", hashName.c_str());
            }
        } else if (!token.Icmp("ambientLitVersion")) {
            if (lexer.ReadToken(&token)) {
                Str path = baseDir;
                path.AppendPath(token, '/');

                ambientLitVersion = shaderManager.GetShader(path);
            } else {
                BE_WARNLOG(L"missing ambientLitVersion name in shader '%hs'\n", hashName.c_str());
            }
        } else if (!token.Icmp("directLitVersion")) {
            if (lexer.ReadToken(&token)) {
                Str path = baseDir;
                path.AppendPath(token, '/');

                directLitVersion = shaderManager.GetShader(path);
            } else {
                BE_WARNLOG(L"missing directLitVersion name in shader '%hs'\n", hashName.c_str());
            }
        } else if (!token.Icmp("ambientLitDirectLitVersion")) {
            if (lexer.ReadToken(&token)) {
                Str path = baseDir;
                path.AppendPath(token, '/');

                ambientLitDirectLitVersion = shaderManager.GetShader(path);
            } else {
                BE_WARNLOG(L"missing ambientLitDirectLitVersion name in shader '%hs'\n", hashName.c_str());
            }
        } else if (!token.Icmp("perforatedVersion")) {
            if (lexer.ReadToken(&token)) {
                Str path = baseDir;
                path.AppendPath(token, '/');

                perforatedVersion = shaderManager.GetShader(path);
            } else {
                BE_WARNLOG(L"missing perforatedVersion name in shader '%hs'\n", hashName.c_str());
            }
        } else if (!token.Icmp("parallelShadowVersion")) {
            if (lexer.ReadToken(&token)) {
                Str path = baseDir;
                path.AppendPath(token, '/');

                parallelShadowVersion = shaderManager.GetShader(path);
            } else {
                BE_WARNLOG(L"missing parallelShadowVersion name in shader '%hs'\n", hashName.c_str());
            }
        } else if (!token.Icmp("spotShadowVersion")) {
            if (lexer.ReadToken(&token)) {
                Str path = baseDir;
                path.AppendPath(token, '/');

                spotShadowVersion = shaderManager.GetShader(path);
            } else {
                BE_WARNLOG(L"missing spotShadowVersion name in shader '%hs'\n", hashName.c_str());
            }
        } else if (!token.Icmp("pointShadowVersion")) {
            if (lexer.ReadToken(&token)) {
                Str path = baseDir;
                path.AppendPath(token, '/');

                pointShadowVersion = shaderManager.GetShader(path); 
            } else {
                BE_WARNLOG(L"missing pointShadowVersion name in shader '%hs'\n", hashName.c_str());
            }
        } else if (!token.Icmp("gpuSkinningVersion")) {
            if (lexer.ReadToken(&token)) {
                Str path = baseDir;
                path.AppendPath(token, '/');

                gpuSkinningVersion[0] = shaderManager.GetShader(path);

                if (lexer.ReadToken(&token)) {
                    Str path = baseDir;
                    path.AppendPath(token, '/');

                    gpuSkinningVersion[1] = shaderManager.GetShader(path);

                    if (lexer.ReadToken(&token)) {
                        Str path = baseDir;
                        path.AppendPath(token, '/');

                        gpuSkinningVersion[2] = shaderManager.FindShader(path);
                    } else {
                        BE_WARNLOG(L"missing gpuSkinningVersion name in shader '%hs'\n", hashName.c_str());
                    }
                } else {
                    BE_WARNLOG(L"missing gpuSkinningVersion name in shader '%hs'\n", hashName.c_str());
                }
            } else {
                BE_WARNLOG(L"missing gpuSkinningVersion name in shader '%hs'\n", hashName.c_str());
            }
        } else if (!token.Icmp("generatePerforatedVersion")) {
            generatePerforatedVersion = true;
        } else if (!token.Icmp("generatePremulAlphaVersion")) {
            generatePremulAlphaVersion = true;
        } else if (!token.Icmp("generateGpuSkinningVersion")) {
            generateGpuSkinningVersion = true;
        } else if (!token.Icmp("generateParallelShadowVersion")) {
            generateParallelShadowVersion = true;
        } else if (!token.Icmp("generatePointShadowVersion")) {
            generatePointShadowVersion = true;
        } else if (!token.Icmp("generateSpotShadowVersion")) {
            generateSpotShadowVersion = true;
        } else if (!token.Icmp("glsl_vp")) {
            lexer.ParseBracedSectionExact(vsText);
        } else if (!token.Icmp("glsl_fp")) {
            lexer.ParseBracedSectionExact(fsText);
        } else {
            BE_WARNLOG(L"unknown parameter %hs in shader '%hs'\n", token.c_str(), hashName.c_str());
        }
    }

    return Finish(generatePerforatedVersion, generatePremulAlphaVersion, generateGpuSkinningVersion, generateParallelShadowVersion, generateSpotShadowVersion, generatePointShadowVersion, baseDir);
}

bool ParseShaderPropertyInfo(Lexer &lexer, PropertyInfo &propInfo) {
    propInfo.type = Variant::None;
    propInfo.flags = PropertyInfo::EditorFlag;
    propInfo.range = Rangef(0, 0, 1);
    propInfo.metaObject = nullptr;

    if (!lexer.ReadToken(&propInfo.name, false)) {
        return false;
    }

    if (!lexer.ExpectPunctuation(P_PARENTHESESOPEN)) {
        return false;
    }

    if (!lexer.ExpectTokenType(TokenType::TT_STRING, &propInfo.label)) {
        return false;
    }

    if (!lexer.ExpectPunctuation(P_PARENTHESESCLOSE)) {
        return false;
    }

    if (!lexer.ExpectPunctuation(P_COLON)) {
        return false;
    }

    Str typeStr;
    if (!lexer.ReadToken(&typeStr, false)) {
        return false;
    }

    if (!Str::Cmp(typeStr, "bool")) {
        propInfo.type = Variant::BoolType;
    } else if (!Str::Cmp(typeStr, "int")) {
        propInfo.type = Variant::IntType;
    } else if (!Str::Cmp(typeStr, "point")) {
        propInfo.type = Variant::PointType;
    } else if (!Str::Cmp(typeStr, "rect")) {
        propInfo.type = Variant::RectType;
    } else if (!Str::Cmp(typeStr, "float")) {
        propInfo.type = Variant::FloatType;
    } else if (!Str::Cmp(typeStr, "vec2")) {
        propInfo.type = Variant::Vec2Type;
    } else if (!Str::Cmp(typeStr, "vec3")) {
        propInfo.type = Variant::Vec3Type;
    } else if (!Str::Cmp(typeStr, "vec4")) {
        propInfo.type = Variant::Vec4Type;
    } else if (!Str::Cmp(typeStr, "color3")) {
        propInfo.type = Variant::Color3Type;
    } else if (!Str::Cmp(typeStr, "color4")) {
        propInfo.type = Variant::Color4Type;
    } else if (!Str::Cmp(typeStr, "enum")) {
        Str enumSequence;
        if (!lexer.ExpectTokenType(TokenType::TT_STRING, &enumSequence)) {
            return false;
        }
        propInfo.type = Variant::IntType;
        propInfo.enumeration.Clear();
        SplitStringIntoList(propInfo.enumeration, enumSequence, ";");
    } else if (!Str::Cmp(typeStr, "texture")) {
        propInfo.type = Variant::GuidType;
        propInfo.metaObject = &TextureAsset::metaObject;
    }

    if (propInfo.type == Variant::IntType ||
        propInfo.type == Variant::FloatType ||
        propInfo.type == Variant::Vec2Type ||
        propInfo.type == Variant::Vec3Type ||
        propInfo.type == Variant::Vec4Type) {
        Str token;
        lexer.ReadToken(&token, false);

        if (token == "range") {
            propInfo.range.minValue = lexer.ParseNumber();
            propInfo.range.maxValue = lexer.ParseNumber();
            propInfo.range.step = lexer.ParseNumber();
        } else {
            lexer.UnreadToken(&token);
        }
    }

    if (!lexer.ExpectPunctuation(P_ASSIGN)) {
        return false;
    }

    Str defaultValueString;
    if (!lexer.ExpectTokenType(TokenType::TT_STRING, &defaultValueString)) {
        return false;
    }

    if (!Str::Cmp(typeStr, "texture")) {
        propInfo.defaultValue = resourceGuidMapper.Get(defaultValueString);
    } else {
        propInfo.defaultValue = Variant::FromString(propInfo.type, defaultValueString);
    }

    Str token;
    lexer.ReadToken(&token, false);
    if (token == "(") {
        while (lexer.ReadToken(&token, false)) {
            if (token == ")") {
                break;
            } else if (token == "shaderDefine") {
                propInfo.flags |= PropertyInfo::ShaderDefineFlag;
            } else {
                return false;
            }
        }
    }

    return true;
}

bool Shader::ParseProperties(Lexer &lexer) {
    PropertyInfo propInfo;
    Str token;

    if (!lexer.ExpectPunctuation(P_BRACEOPEN)) {
        return false;
    }

    while (lexer.ReadToken(&token)) {
        if (token[0] == '}') {
            break;
        }
        else {
            if (propertyInfoHashMap.Get(token)) {
                BE_WARNLOG(L"same property name '%hs' ignored in shader '%hs'\n", token.c_str(), hashName.c_str());
                lexer.SkipRestOfLine();
                continue;
            }

            lexer.UnreadToken(&token);

            if (!ParseShaderPropertyInfo(lexer, propInfo)) {
                BE_WARNLOG(L"error occured in parsing property propInfo in shader '%hs'\n", hashName.c_str());
                lexer.SkipRestOfLine();
                continue;
            }

            propertyInfoHashMap.Set(token, propInfo);
        }
    }

    return true;
}

Shader *Shader::GenerateSubShader(const Str &shaderNamePostfix, const Str &vsHeaderText, const Str &fsHeaderText, int skinning) {
    Str skinningPostfix;
    Str skinningVsHeaderText;

    switch (skinning) {
    case 0:
        skinningPostfix = "";
        break;
    case 1:
        skinningPostfix = "-skinning1";
        skinningVsHeaderText = "#define GPU_SKINNING\n$include \"SkinningMatrix1.glsl\"\n";
        break;
    case 4:
        skinningPostfix = "-skinning4";
        skinningVsHeaderText = "#define GPU_SKINNING\n$include \"SkinningMatrix4.glsl\"\n";
        break;
    case 8:
        skinningPostfix = "-skinning8";
        skinningVsHeaderText = "#define GPU_SKINNING\n$include \"SkinningMatrix8.glsl\"\n";
        break;
    default:
        assert(0);
        break;
    }

    Str shaderName = name + shaderNamePostfix + skinningPostfix;
    Shader *shader = shaderManager.FindShader(shaderName);
    if (shader) {
        shader->AddRefCount();
        return shader;
    }

    shader = shaderManager.AllocShader(shaderName);

    Str text = Str("{ glsl_vp {\n") + vsHeaderText + skinningVsHeaderText + vsText + "} glsl_fp {\n" + fsHeaderText + fsText + "} }";
    if (!shader->Create(text, baseDir)) {
        return nullptr;
    }

    shader->propertyInfoHashMap = propertyInfoHashMap;

    return shader;
}

bool Shader::GenerateGpuSkinningVersion(Shader *shader, const Str &shaderNamePostfix, const Str &vsHeaderText, const Str &fsHeaderText) {
    if (!shader->gpuSkinningVersion[0]) {
        shader->gpuSkinningVersion[0] = GenerateSubShader(shaderNamePostfix, vsHeaderText, fsHeaderText, 1);
        if (!shader->gpuSkinningVersion[0]) {
            return false;
        }
    }

    if (!shader->gpuSkinningVersion[1]) {
        shader->gpuSkinningVersion[1] = GenerateSubShader(shaderNamePostfix, vsHeaderText, fsHeaderText, 4);
        if (!shader->gpuSkinningVersion[1]) {
            return false;
        }
    }

    if (!shader->gpuSkinningVersion[2]) {
        shader->gpuSkinningVersion[2] = GenerateSubShader(shaderNamePostfix, vsHeaderText, fsHeaderText, 8);
        if (!shader->gpuSkinningVersion[2]) {
            return false;
        }
    }

    return true;
}

bool Shader::GeneratePerforatedVersion(Shader *shader, const Str &shaderNamePostfix, const Str &vsHeaderText, const Str &fsHeaderText, bool genereateGpuSkinningVersion) {
    if (!shader->perforatedVersion) {
        shader->perforatedVersion = GenerateSubShader(shaderNamePostfix + "-perforated",
            vsHeaderText + "#define PERFORATED\n", fsHeaderText + "#define PERFORATED\n", 0);
        if (!shader->perforatedVersion) {
            return false;
        }
    }

    if (genereateGpuSkinningVersion) {
        if (!GenerateGpuSkinningVersion(shader->perforatedVersion,
            shaderNamePostfix + "-perforated", vsHeaderText + "#define PERFORATED\n", fsHeaderText + "#define PERFORATED\n")) {
            return false;
        }
    }

    return true;
}

bool Shader::GeneratePremulAlphaVersion(Shader *shader, const Str &shaderNamePostfix, const Str &vsHeaderText, const Str &fsHeaderText, bool genereateGpuSkinningVersion) {
    if (!shader->premulAlphaVersion) {
        shader->premulAlphaVersion = GenerateSubShader(shaderNamePostfix + "-premulAlpha",
            vsHeaderText + "#define PREMULTIPLIED_ALPHA\n", fsHeaderText + "#define PREMULTIPLIED_ALPHA\n", 0);
        if (!shader->premulAlphaVersion) {
            return false;
        }
    }

    if (genereateGpuSkinningVersion) {
        if (!GenerateGpuSkinningVersion(shader->premulAlphaVersion,
            shaderNamePostfix + "-premulAlpha", vsHeaderText + "#define PREMULTIPLIED_ALPHA\n", fsHeaderText + "#define PREMULTIPLIED_ALPHA\n")) {
            return false;
        }
    }

    return true;
}

bool Shader::Finish(bool generatePerforatedVersion, bool generatePremulAlphaVersion, bool genereateGpuSkinningVersion, bool generateParallelShadowVersion, bool generateSpotShadowVersion, bool generatePointShadowVersion, const char *baseDir) {
    if (genereateGpuSkinningVersion) {
        if (!GenerateGpuSkinningVersion(this, "", "", "")) {
            return false;
        }
    }

    if (generatePerforatedVersion) {
        if (!GeneratePerforatedVersion(this, "", "", "", genereateGpuSkinningVersion)) {
            return false;
        }
    }

    if (generatePremulAlphaVersion) {
        if (!GeneratePremulAlphaVersion(this, "", "", "", genereateGpuSkinningVersion)) {
            return false;
        }
    }

    if (generateParallelShadowVersion) {
        const Str shaderNamePostfix = "-parallelShadowed";
        const Str vsHeaderText = "#define USE_SHADOW_CASCADE\n";
        const Str fsHeaderText = "#define USE_SHADOW_CASCADE\n";

        if (!parallelShadowVersion) {
            parallelShadowVersion = GenerateSubShader(shaderNamePostfix, vsHeaderText, fsHeaderText, 0);
            if (!parallelShadowVersion) {
                return false;
            }
        }

        if (genereateGpuSkinningVersion) {
            if (!GenerateGpuSkinningVersion(parallelShadowVersion, shaderNamePostfix, vsHeaderText, fsHeaderText)) {
                return false;
            }
        }

        if (generatePerforatedVersion) {
            if (!GeneratePerforatedVersion(parallelShadowVersion, shaderNamePostfix, vsHeaderText, fsHeaderText, genereateGpuSkinningVersion)) {
                return false;
            }
        }
    }

    if (generateSpotShadowVersion) {
        const Str shaderNamePostfix = "-spotShadowed";
        const Str vsHeaderText = "#define USE_SHADOW_SPOT\n";
        const Str fsHeaderText = "#define USE_SHADOW_SPOT\n";

        if (!spotShadowVersion) {
            spotShadowVersion = GenerateSubShader(shaderNamePostfix, vsHeaderText, fsHeaderText, 0);
            if (!spotShadowVersion) {
                return false;
            }
        }

        if (genereateGpuSkinningVersion) {
            if (!GenerateGpuSkinningVersion(spotShadowVersion, shaderNamePostfix, vsHeaderText, fsHeaderText)) {
                return false;
            }
        }

        if (generatePerforatedVersion) {
            if (!GeneratePerforatedVersion(spotShadowVersion, shaderNamePostfix, vsHeaderText, fsHeaderText, genereateGpuSkinningVersion)) {
                return false;
            }
        }
    }

    if (generatePointShadowVersion) {
        const Str shaderNamePostfix = "-pointShadowed";
        const Str vsHeaderText = "#define USE_SHADOW_POINT\n";
        const Str fsHeaderText = "#define USE_SHADOW_POINT\n";

        if (!pointShadowVersion) {
            pointShadowVersion = GenerateSubShader(shaderNamePostfix, vsHeaderText, fsHeaderText, 0);
            if (!pointShadowVersion) {
                return false;
            }
        }

        if (genereateGpuSkinningVersion) {
            if (!GenerateGpuSkinningVersion(pointShadowVersion, shaderNamePostfix, vsHeaderText, fsHeaderText)) {
                return false;
            }
        }

        if (generatePerforatedVersion) {
            if (!GeneratePerforatedVersion(pointShadowVersion, shaderNamePostfix, vsHeaderText, fsHeaderText, genereateGpuSkinningVersion)) {
                return false;
            }
        }
    }

    return true;
}

const char *Shader::MangleNameWithDefineList(const Str &basename, const Array<Shader::Define> &defineArray, Str &mangledName) {
    mangledName = basename;
    
    if (defineArray.Count() > 0) {
        Array<Shader::Define> sortedDefineArray = defineArray;
        sortedDefineArray.Sort([](const Shader::Define &a, const Shader::Define &b) -> bool {
            return (a.name).Icmp(b.name) < 0;
        });
    
        for (int i = 0; i < defineArray.Count(); i++) {
            mangledName += "+" + defineArray[i].name + "=" + defineArray[i].value;
        }
    }

    return mangledName.c_str();
}

Shader *Shader::InstantiateShader(const Array<Define> &defineArray) {
    Str mangledName;
    // instantiated shader name start with '@' character
    MangleNameWithDefineList("@" + name, defineArray, mangledName);

    // return instantiated shader if it already exist
    Shader *shader = shaderManager.FindShader(mangledName);
    if (shader) {
        // instantiated shader always has pointer to original shader
        assert(shader->originalShader);
        shader->AddRefCount();
        return shader;
    }

    // allocate new instantiated shader
    shader = shaderManager.AllocShader(mangledName);
    shader->originalShader = this;
    shader->flags = flags;
    shader->defineArray = defineArray;
    shader->baseDir = baseDir;

    instantiatedShaders.Append(shader);
    
    shader->Instantiate(defineArray);

    return shader;
}

Shader *Shader::GetPerforatedVersion() {
    if (perforatedVersion) {
        return perforatedVersion;
    }

    if (originalShader) {
        if (originalShader->perforatedVersion) {
            perforatedVersion = originalShader->perforatedVersion->InstantiateShader(defineArray);
            return perforatedVersion;
        }
    }

    return nullptr;
}

Shader *Shader::GetPremulAlphaVersion() {
    if (premulAlphaVersion) {
        return premulAlphaVersion;
    }

    if (originalShader) {
        if (originalShader->premulAlphaVersion) {
            premulAlphaVersion = originalShader->premulAlphaVersion->InstantiateShader(defineArray);
            return premulAlphaVersion;
        }
    }

    return nullptr;
}

Shader *Shader::GetAmbientLitVersion() {
    if (ambientLitVersion) {
        return ambientLitVersion;
    }

    if (originalShader) {
        if (originalShader->ambientLitVersion) {
            ambientLitVersion = originalShader->ambientLitVersion->InstantiateShader(defineArray);
            return ambientLitVersion;
        }
    }

    return nullptr;
}

Shader *Shader::GetDirectLitVersion() {
    if (directLitVersion) {
        return directLitVersion;
    }

    if (originalShader) {
        if (originalShader->directLitVersion) {
            directLitVersion = originalShader->directLitVersion->InstantiateShader(defineArray);
            return directLitVersion;
        }
    }

    return nullptr;
}

Shader *Shader::GetAmbientLitDirectLitVersion() {
    if (ambientLitDirectLitVersion) {
        return ambientLitDirectLitVersion;
    }

    if (originalShader) {
        if (originalShader->ambientLitDirectLitVersion) {
            ambientLitDirectLitVersion = originalShader->ambientLitDirectLitVersion->InstantiateShader(defineArray);
            return ambientLitDirectLitVersion;
        }
    }

    return nullptr;
}

Shader *Shader::GetParallelShadowVersion() {
    if (parallelShadowVersion) {
        return parallelShadowVersion;
    }

    if (originalShader) {
        if (originalShader->parallelShadowVersion) {
            parallelShadowVersion = originalShader->parallelShadowVersion->InstantiateShader(defineArray);
            return parallelShadowVersion;
        }
    }

    return nullptr;
}

Shader *Shader::GetSpotShadowVersion() {
    if (spotShadowVersion) {
        return spotShadowVersion;
    }

    if (originalShader) {
        if (originalShader->spotShadowVersion) {
            spotShadowVersion = originalShader->spotShadowVersion->InstantiateShader(defineArray);
            return spotShadowVersion;
        }
    }

    return nullptr;
}

Shader *Shader::GetPointShadowVersion() {
    if (pointShadowVersion) {
        return pointShadowVersion;
    }

    if (originalShader) {
        if (originalShader->pointShadowVersion) {
            pointShadowVersion = originalShader->pointShadowVersion->InstantiateShader(defineArray);
            return pointShadowVersion;
        }
    }

    return nullptr;
}

Shader *Shader::GetGPUSkinningVersion(int index) {
    if (gpuSkinningVersion[index]) {
        return gpuSkinningVersion[index];
    }

    if (originalShader) {
        if (originalShader->gpuSkinningVersion[index]) {
            gpuSkinningVersion[index] = originalShader->gpuSkinningVersion[index]->InstantiateShader(defineArray);
            return gpuSkinningVersion[index];
        }
    }

    return nullptr;
}

void Shader::Reinstantiate() {
    assert(originalShader);
    Instantiate(defineArray);

    if (originalShader->ambientLitVersion) {
        if (ambientLitVersion) {
            ambientLitVersion->originalShader = originalShader->ambientLitVersion;
            ambientLitVersion->Reinstantiate();
        } else {
            ambientLitVersion = originalShader->ambientLitVersion->InstantiateShader(defineArray);
        }
    } else {
        if (ambientLitVersion) {
            shaderManager.ReleaseShader(ambientLitVersion);
            ambientLitVersion = nullptr;
        }
    }

    if (originalShader->directLitVersion) {
        if (directLitVersion) {
            directLitVersion->originalShader = originalShader->directLitVersion;
            directLitVersion->Reinstantiate();
        } else {
            directLitVersion = originalShader->directLitVersion->InstantiateShader(defineArray);
        }
    } else {
        if (directLitVersion) {
            shaderManager.ReleaseShader(directLitVersion);
            directLitVersion = nullptr;
        }
    }

    if (originalShader->ambientLitDirectLitVersion) {
        if (ambientLitDirectLitVersion) {
            ambientLitDirectLitVersion->originalShader = originalShader->ambientLitDirectLitVersion;
            ambientLitDirectLitVersion->Reinstantiate();
        } else {
            ambientLitDirectLitVersion = originalShader->ambientLitDirectLitVersion->InstantiateShader(defineArray);
        }
    } else {
        if (ambientLitDirectLitVersion) {
            shaderManager.ReleaseShader(ambientLitDirectLitVersion);
            ambientLitDirectLitVersion = nullptr;
        }
    }

    if (originalShader->perforatedVersion) {
        if (perforatedVersion) {
            perforatedVersion->originalShader = originalShader->perforatedVersion;
            perforatedVersion->Reinstantiate();
        } else {
            perforatedVersion = originalShader->perforatedVersion->InstantiateShader(defineArray);
        }
    } else {
        if (perforatedVersion) {
            shaderManager.ReleaseShader(perforatedVersion);
            perforatedVersion = nullptr;
        }
    }

    if (originalShader->premulAlphaVersion) {
        if (premulAlphaVersion) {
            premulAlphaVersion->originalShader = originalShader->premulAlphaVersion;
            premulAlphaVersion->Reinstantiate();
        } else {
            premulAlphaVersion = originalShader->premulAlphaVersion->InstantiateShader(defineArray);
        }
    } else {
        if (premulAlphaVersion) {
            shaderManager.ReleaseShader(premulAlphaVersion);
            premulAlphaVersion = nullptr;
        }
    }

    if (originalShader->parallelShadowVersion) {
        if (parallelShadowVersion) {
            parallelShadowVersion->originalShader = originalShader->parallelShadowVersion;
            parallelShadowVersion->Reinstantiate();
        } else {
            parallelShadowVersion = originalShader->parallelShadowVersion->InstantiateShader(defineArray);
        }
    } else {
        if (parallelShadowVersion) {
            shaderManager.ReleaseShader(parallelShadowVersion);
            parallelShadowVersion = nullptr;
        }
    }

    if (originalShader->spotShadowVersion) {
        if (spotShadowVersion) {
            spotShadowVersion->originalShader = originalShader->spotShadowVersion;
            spotShadowVersion->Reinstantiate();
        } else {
            spotShadowVersion = originalShader->spotShadowVersion->InstantiateShader(defineArray);
        }
    } else {
        if (spotShadowVersion) {
            shaderManager.ReleaseShader(spotShadowVersion);
            spotShadowVersion = nullptr;
        }
    }

    if (originalShader->pointShadowVersion) {
        if (pointShadowVersion) {
            pointShadowVersion->originalShader = originalShader->pointShadowVersion;
            pointShadowVersion->Reinstantiate();
        } else {
            pointShadowVersion = originalShader->pointShadowVersion->InstantiateShader(defineArray);
        }
    } else {
        if (pointShadowVersion) {
            shaderManager.ReleaseShader(pointShadowVersion);
            pointShadowVersion = nullptr;
        }
    }

    for (int i = 0; i < COUNT_OF(gpuSkinningVersion); i++) {
        if (originalShader->gpuSkinningVersion[i]) {
            if (gpuSkinningVersion[i]) {
                gpuSkinningVersion[i]->originalShader = originalShader->gpuSkinningVersion[i];
                gpuSkinningVersion[i]->Reinstantiate();
            } else {
                gpuSkinningVersion[i] = originalShader->gpuSkinningVersion[i]->InstantiateShader(defineArray);
            }
        } else {
            if (gpuSkinningVersion[i]) {
                shaderManager.ReleaseShader(gpuSkinningVersion[i]);
                gpuSkinningVersion[i] = nullptr;
            }
        }
    }
}

bool Shader::Instantiate(const Array<Define> &defineArray) {
/*#if defined __ANDROID__ && ! defined __XAMARIN__

	if (android_progress >= 0) {
		float f = android_progress * (M_PI * 0.01f);
		android_progress++;
		Color4 color;
		color.r = sinf(f) * 0.5f + 0.5f;
		color.g = sinf(f + M_PI * 2.0f / 3.0f)* 0.5f + 0.5f;
		color.b = sinf(f + M_PI * 4.0f / 3.0f)* 0.5f + 0.5f;
		color.a = 1.0f;

		rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
		rhi.Clear(RHI::ColorBit, color, 0, 0);
		rhi.SwapBuffers();
	}

    //BE_LOG(L"progress %f %f %f %f %d", color.r, color.g, color.b, f, progress);
#endif*/

    Str processedVsText;
    Str processedFsText;
    hasVertexShader = ProcessShaderText(originalShader->vsText, originalShader->baseDir, defineArray, processedVsText);
    hasFragmentShader = ProcessShaderText(originalShader->fsText, originalShader->baseDir, defineArray, processedFsText);

    if (!hasVertexShader || !hasFragmentShader) {
        return false;
    }

    if (shaderHandle != RHI::NullShader) {
        rhi.DeleteShader(shaderHandle);
    }

    shaderHandle = rhi.CreateShader(hashName, processedVsText, processedFsText);

    assert(MaxBuiltInConstants == COUNT_OF(builtInConstantNames));
    assert(MaxBuiltInSamplers == COUNT_OF(builtInSamplerNames));

    for (int i = 0; i < MaxBuiltInConstants; i++) {
        builtInConstantLocations[i] = rhi.GetShaderConstantLocation(shaderHandle, builtInConstantNames[i]);
    }

    for (int i = 0; i < MaxBuiltInSamplers; i++) {
        builtInSamplerUnits[i] = rhi.GetSamplerUnit(shaderHandle, builtInSamplerNames[i]);
    }

    return true;
}

bool Shader::ProcessShaderText(const char *text, const char *baseDir, const Array<Define> &defineArray, Str &outStr) const {
    outStr = text;

    // insert local define array
    for (int i = 0; i < defineArray.Count(); i++) {
        outStr.Insert(va("#define %s %i\n", defineArray[i].name.c_str(), defineArray[i].value), 0);
    }

    // insert global define array
    for (int i = shaderManager.globalHeaderList.Count() - 1; i >= 0; i--) {
        outStr.Insert(shaderManager.globalHeaderList[i].c_str(), 0);
    }    

    ProcessIncludeRecursive(baseDir, outStr);

    return true;
}

bool Shader::ProcessIncludeRecursive(const char *baseDir, Str &outText) const {
    Lexer lexer;
    lexer.Init(LexerFlag::LEXFL_NOERRORS);

    int pos = 0;

    do {
        pos = outText.Find(directiveInclude, true, pos);
        if (pos == -1) {
            return true;
        }

        const char *data_p = outText.c_str() + pos + Str::Length(directiveInclude);
        lexer.Load(data_p, Str::Length(data_p), hashName);

        Str relativeFileName;
        lexer.ExpectTokenType(TT_STRING, &relativeFileName);

        Str path = baseDir;
        path.AppendPath(relativeFileName);  

        char *includingText;
        size_t fileLen = fileSystem.LoadFile(path.c_str(), true, (void **)&includingText);
        if (!fileLen) {
            BE_FATALERROR(L"Shader::ProcessIncludeRecursive: Cannot open include file '%hs'", path.c_str());
            return false;
        }

        Str newBaseDir = path;
        newBaseDir.StripFileName();

        Str nestedText = includingText;
        ProcessIncludeRecursive(newBaseDir, nestedText);

        outText = outText.Left(pos) + nestedText + Str(data_p + lexer.GetCurrentOffset());

        fileSystem.FreeFile(includingText);
    } while (1);

    return true;
}

void Shader::Bind() const { 
    rhi.BindShader(shaderHandle);
}

int Shader::GetConstantLocation(const char *name) const {
    return rhi.GetShaderConstantLocation(shaderHandle, name); 
}

void Shader::SetConstant1i(int index, const int constant) const { 
    rhi.SetShaderConstant1i(index, constant);
}

void Shader::SetConstant2i(int index, const int *constant) const { 
    rhi.SetShaderConstant2i(index, constant);
}

void Shader::SetConstant3i(int index, const int *constant) const { 
    rhi.SetShaderConstant3i(index, constant);
}

void Shader::SetConstant4i(int index, const int *constant) const { 
    rhi.SetShaderConstant4i(index, constant);
}

void Shader::SetConstant1f(int index, float x) const { 
    rhi.SetShaderConstant1f(index, x);
}

void Shader::SetConstant2f(int index, const float *constant) const { 
    rhi.SetShaderConstant2f(index, constant);
}

void Shader::SetConstant3f(int index, const float *constant) const { 
    rhi.SetShaderConstant3f(index, constant);
}

void Shader::SetConstant4f(int index, const float *constant) const { 
    rhi.SetShaderConstant4f(index, constant);
}

void Shader::SetConstant2f(int index, const Vec2 &constant) const { 
    rhi.SetShaderConstant2f(index, constant);
}

void Shader::SetConstant3f(int index, const Vec3 &constant) const { 
    rhi.SetShaderConstant3f(index, constant);
}

void Shader::SetConstant4f(int index, const Vec4 &constant) const { 
    rhi.SetShaderConstant4f(index, constant);
}

void Shader::SetConstant2x2f(int index, bool rowmajor, const Mat2 &constant) const { 
    rhi.SetShaderConstant2x2f(index, rowmajor, constant);
}

void Shader::SetConstant3x3f(int index, bool rowmajor, const Mat3 &constant) const { 
    rhi.SetShaderConstant3x3f(index, rowmajor, constant);
}

void Shader::SetConstant4x4f(int index, bool rowmajor, const Mat4 &constant) const { 
    rhi.SetShaderConstant4x4f(index, rowmajor, constant);
}

void Shader::SetConstantArray1i(int index, int num, const int *constant) const {
    rhi.SetShaderConstantArray1i(index, num, constant);
}

void Shader::SetConstantArray2i(int index, int num, const int *constant) const {
    rhi.SetShaderConstantArray2i(index, num, constant);
}

void Shader::SetConstantArray3i(int index, int num, const int *constant) const {
    rhi.SetShaderConstantArray3i(index, num, constant);
}

void Shader::SetConstantArray4i(int index, int num, const int *constant) const {
    rhi.SetShaderConstantArray4i(index, num, constant);
}

void Shader::SetConstantArray1f(int index, int num, const float *constant) const {
    rhi.SetShaderConstantArray1f(index, num, constant);
}

void Shader::SetConstantArray2f(int index, int num, const float *constant) const {
    rhi.SetShaderConstantArray2f(index, num, constant);
}

void Shader::SetConstantArray3f(int index, int num, const float *constant) const {
    rhi.SetShaderConstantArray3f(index, num, constant);
}

void Shader::SetConstantArray4f(int index, int num, const float *constant) const {
    rhi.SetShaderConstantArray4f(index, num, constant);
}

void Shader::SetConstantArray2f(int index, int num, const Vec2 *constant) const {
    rhi.SetShaderConstantArray2f(index, num, constant);
}

void Shader::SetConstantArray3f(int index, int num, const Vec3 *constant) const {
    rhi.SetShaderConstantArray3f(index, num, constant);
}

void Shader::SetConstantArray4f(int index, int num, const Vec4 *constant) const {
    rhi.SetShaderConstantArray4f(index, num, constant);
}

void Shader::SetConstantArray2x2f(int index, bool rowmajor, int num, const Mat2 *constant) const { 
    rhi.SetShaderConstantArray2x2f(index, rowmajor, num, constant);
}

void Shader::SetConstantArray3x3f(int index, bool rowmajor, int num, const Mat3 *constant) const { 
    rhi.SetShaderConstantArray3x3f(index, rowmajor, num, constant);
}

void Shader::SetConstantArray4x4f(int index, bool rowmajor, int num, const Mat4 *constant) const { 
    rhi.SetShaderConstantArray4x4f(index, rowmajor, num, constant);
}

void Shader::SetConstant1i(const char *name, int x) const { 
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant1i: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant1i(index, x);
}

void Shader::SetConstant2i(const char *name, const int *constant) const { 
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant2i: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant2i(index, constant);
}

void Shader::SetConstant3i(const char *name, const int *constant) const { 
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant3i: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant3i(index, constant);
}

void Shader::SetConstant4i(const char *name, const int *constant) const { 
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant4i: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant4i(index, constant);
}

void Shader::SetConstant1f(const char *name, float x) const { 
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant1f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant1f(index, x);
}

void Shader::SetConstant2f(const char *name, const float *constant) const { 
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant2f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant2f(index, constant);
}

void Shader::SetConstant3f(const char *name, const float *constant) const { 
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant3f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant3f(index, constant);
}

void Shader::SetConstant4f(const char *name, const float *constant) const { 
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant4f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant4f(index, constant);
}

void Shader::SetConstant2f(const char *name, const Vec2 &constant) const { 
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant2f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant2f(index, constant);
}

void Shader::SetConstant3f(const char *name, const Vec3 &constant) const { 
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant3f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant3f(index, constant);
}

void Shader::SetConstant4f(const char *name, const Vec4 &constant) const { 
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant4f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant4f(index, constant);
}

void Shader::SetConstant2x2f(const char *name, bool rowmajor, const Mat2 &constant) const { 
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant2x2f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant2x2f(index, rowmajor, constant);
}

void Shader::SetConstant3x3f(const char *name, bool rowmajor, const Mat3 &constant) const { 
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant3x3f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant3x3f(index, rowmajor, constant);
}

void Shader::SetConstant4x4f(const char *name, bool rowmajor, const Mat4 &constant) const { 
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant4x4f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant4x4f(index, rowmajor, constant);
}

void Shader::SetConstantArray1i(const char *name, int num, const int *constant) const {
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray1i: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray1i(index, num, constant);
}

void Shader::SetConstantArray2i(const char *name, int num, const int *constant) const {
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray2i: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray2i(index, num, constant);
}

void Shader::SetConstantArray3i(const char *name, int num, const int *constant) const {
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray3i: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray3i(index, num, constant);
}

void Shader::SetConstantArray4i(const char *name, int num, const int *constant) const {
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray4i: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray4i(index, num, constant);
}

void Shader::SetConstantArray1f(const char *name, int num, const float *constant) const {
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray1f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray1f(index, num, constant);
}

void Shader::SetConstantArray2f(const char *name, int num, const float *constant) const {
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray2f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray2f(index, num, constant);
}

void Shader::SetConstantArray3f(const char *name, int num, const float *constant) const {
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray3f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray3f(index, num, constant);
}

void Shader::SetConstantArray4f(const char *name, int num, const float *constant) const {
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray4f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray4f(index, num, constant);
}

void Shader::SetConstantArray2f(const char *name, int num, const Vec2 *constant) const {
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray2f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray2f(index, num, constant);
}

void Shader::SetConstantArray3f(const char *name, int num, const Vec3 *constant) const {
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray3f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray3f(index, num, constant);
}

void Shader::SetConstantArray4f(const char *name, int num, const Vec4 *constant) const {
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray4f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray4f(index, num, constant);
}

void Shader::SetConstantArray2x2f(const char *name, bool rowmajor, int num, const Mat2 *constant) const { 
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray2x2f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray2x2f(index, rowmajor, num, constant);
}

void Shader::SetConstantArray3x3f(const char *name, bool rowmajor, int num, const Mat3 *constant) const { 
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray3x3f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray3x3f(index, rowmajor, num, constant);
}

void Shader::SetConstantArray4x4f(const char *name, bool rowmajor, int num, const Mat4 *constant) const { 
    int index = rhi.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray4x4f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray4x4f(index, rowmajor, num, constant);
}

int Shader::GetSamplerUnit(const char *name) const {
    return rhi.GetSamplerUnit(shaderHandle, name);
}

void Shader::SetTexture(int unit, const Texture *texture) const {
    if (unit < 0) {
        return;
    }

    rhi.SetTexture(unit, texture->textureHandle);
}

void Shader::SetTexture(const char *name, const Texture *texture) const {
    int unit = rhi.GetSamplerUnit(shaderHandle, name);
    if (unit < 0) {
        //BE_WARNLOG(L"Shader::SetTexture: invalid texture name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }

    rhi.SetTexture(unit, texture->textureHandle);
}

void Shader::SetTextureArray(const char *name, int num, const Texture **textures) const {
    char temp[256];

    strcpy(temp, name);
    char *indexPtr = temp + strlen(temp) + 1;
    strcat(temp, "[0]");

    for (int i = 0; i < num; i++) {
        *indexPtr = '0' + i;

        int unit = rhi.GetSamplerUnit(shaderHandle, temp);
        if (unit < 0) {
            //BE_WARNLOG(L"Shader::SetTextureArray: invalid texture name '%hs' in shader '%hs'\n", temp, this->hashName.c_str());
            return;
        }

        rhi.SetTexture(unit, textures[i]->textureHandle);
    }
}

bool Shader::Load(const char *hashName) {
    char *data;
    size_t size = fileSystem.LoadFile(hashName, true, (void **)&data);
    if (!data) {
        return false;
    }

    Lexer lexer;
    lexer.Init(LexerFlag::LEXFL_NOERRORS);
    lexer.Load(data, (int)size, hashName);

    if (!lexer.ExpectTokenString("shader")) {
        fileSystem.FreeFile(data);
        return false;
    }

    Str name;
    if (!lexer.ReadToken(&name)) {
        fileSystem.FreeFile(data);
        return false;
    }

    Str baseDir = hashName;
    baseDir.StripFileName();
    Create(data + lexer.GetCurrentOffset(), baseDir);

    fileSystem.FreeFile(data);

    this->flags |= LoadedFromFile;

    return true;
}

bool Shader::Reload() {
    Shader *shader = this;
    if (originalShader) {
        shader = originalShader;
    }

    if (!(shader->flags & LoadedFromFile)) {
        return false;
    }

    Str _hashName = shader->hashName;
    bool ret = shader->Load(_hashName);

    for (int i = 0; i < shader->instantiatedShaders.Count(); i++) {
        instantiatedShaders[i]->Reinstantiate();
    }

    return ret;
}

BE_NAMESPACE_END
