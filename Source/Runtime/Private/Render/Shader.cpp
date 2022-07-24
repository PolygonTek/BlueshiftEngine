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
#include "IO/FileSystem.h"
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

static const char *directiveInclude = "$include";

// NOTE: must be same order with Shader::BuiltInConstant enum.
static const char *builtInConstantNames[] = {
    "modelViewMatrix",                      // ModelViewMatrix
    "modelViewMatrixTranspose",             // ModelViewMatrixTranspose
    "viewMatrix",                           // ViewMatrix
    "viewMatrixTranspose",                  // ViewMatrixTranspose
    "projectionMatrix",                     // ProjectionMatrix
    "projectionMatrixTranspose",            // ProjectionMatrixTranspose
    "viewProjectionMatrix",                 // ViewProjectionMatrix
    "viewProjectionMatrixTranspose",        // ViewProjectionMatrixTranspose
    "modelViewProjectionMatrix",            // ModelViewProjectionMatrix
    "modelViewProjectionMatrixTranspose",   // ModelViewProjectionMatrixTranspose
    "prevModelViewProjectionMatrix",        // PrevModelViewProjectionMatrix
    "instanceDataBuffer",                   // InstanceDataBuffer
    "instanceIndexes",                      // InstanceIndexes
    "localToWorldMatrix",                   // LocalToWorldMatrix
    "worldToLocalMatrix",                   // WorldToLocalMatrix
    "textureMatrixS",                       // TextureMatrixS
    "textureMatrixT",                       // TextureMatrixT
    "constantColor",                        // ConstantColor
    "intensity",                            // Intensity
    "vertexColorScale",                     // VertexColorScale
    "vertexColorAdd",                       // VertexColorAdd
    "perforatedAlpha",                      // PerforatedAlpha
    "viewOrigin",                           // ViewOrigin
    "lightVec",                             // LightVec
    "lightTextureMatrix",                   // LightTextureMatrix
    "lightColor",                           // LightColor
    "lightFallOffMatrix",                   // LightFallOffMatrix
    "lightFallOffExponent",                 // LightFallOffExponent
    "joints",                               // Joints
    "invJointsMapSize",                     // InvJointsMapSize
    "skinningBaseTc",                       // SkinningBaseTc
    "jointIndexOffset",                     // JointIndexOffset
    "shadowProjMatrix",                     // ShadowProjMatrix
    "shadowCascadeProjMatrix",              // ShadowCascadeProjMatrix
    "shadowSplitFar",                       // ShadowSplitFar
    "probe0SpecularCubeMapMaxMipLevel",     // Probe0SpecularCubeMapMaxMipLevel
    "probe0Position",                       // Probe0Position
    "probe0Mins",                           // Probe0Mins
    "probe0Maxs",                           // Probe0Maxs
    "probe1SpecularCubeMapMaxMipLevel",     // Probe1SpecularCubeMapMaxMipLevel
    "probe1Position",                       // Probe1Position
    "probe1Mins",                           // Probe1Mins
    "probe1Maxs",                           // Probe1Maxs
    "probeLerp"                             // ProbeLerp
};

// NOTE: must be same order with Shader::BuiltInTexture enum.
static const char *builtInTextureNames[] = {
    "cubicNormalCubeMap",                   // CubicNormalCubeMap
    "indirectionCubeMap",                   // IndirectionCubeMap
    "albedoMap",                            // AlbedoMap
    "normalMap",                            // NormalMap
    "jointsMap",                            // JointsMap
    "lightProjectionMap",                   // LightProjectionMap
    "lightCubeMap",                         // LightCubeMap
    "shadowMap",                            // ShadowMap
    "shadowArrayMap",                       // ShadowArrayMap
    "prefilteredDfgMap",                    // PrefilteredDfgMap
    "probe0DiffuseCubeMap",                 // Probe0DiffuseCubeMap
    "probe0SpecularCubeMap",                // Probe0SpecularCubeMap
    "probe1DiffuseCubeMap",                 // Probe1DiffuseCubeMap
    "probe1SpecularCubeMap",                // Probe1SpecularCubeMap
};

const StrHashMap<Shader::ShaderPropertyInfo> &Shader::GetPropertyInfoHashMap() const {
    if (originalShader) {
        return originalShader->propertyInfoHashMap;
    }
    return propertyInfoHashMap;
}

bool Shader::IsPropertyUsed(const Str &propName, const StrHashMap<Shader::Property> &shaderProperties) const {
    const auto &propInfos = GetPropertyInfoHashMap();
    const auto *keyValue = propInfos.Get(propName);
    const ShaderPropertyInfo &propInfo = keyValue->second;

    // No conditions.
    if (propInfo.conditions.Count() == 0) {
        return true;
    }

    for (int conditionIndex = 0; conditionIndex < propInfo.conditions.Count(); conditionIndex++) {
        const Condition &condition = propInfo.conditions[conditionIndex];

        const BE1::Variant &value = shaderProperties.Get(condition.name)->second.data;

        if (!condition.valueList.Find(value.As<int>())) {
            // Found unmatched condition.
            return false;
        }
    }

    return true;
}

void Shader::Purge() {
    if (shaderHandle != RHI::NullShader) {
        rhi.DestroyShader(shaderHandle);
        shaderHandle = RHI::NullShader;
    }

    if (indirectLitVersion) {
        shaderManager.ReleaseShader(indirectLitVersion);
        indirectLitVersion = nullptr;
    }

    if (directLitVersion) {
        shaderManager.ReleaseShader(directLitVersion);
        directLitVersion = nullptr;
    }

    if (indirectLitDirectLitVersion) {
        shaderManager.ReleaseShader(indirectLitDirectLitVersion);
        indirectLitDirectLitVersion = nullptr;
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

    if (gpuInstancingVersion) {
        shaderManager.ReleaseShader(gpuInstancingVersion);
        gpuInstancingVersion = nullptr;
    }

    defineArray.Clear();
    propertyInfoHashMap.Clear();
}

bool Shader::Create(const char *text, const char *baseDir) {
    bool generatePerforatedVersion = false;
    bool generatePremulAlphaVersion = false;
    bool generateGpuSkinningVersion = false;
    bool generateGpuInstancingVersion = false;
    bool generateParallelShadowVersion = false;
    bool generatePointShadowVersion = false;
    bool generateSpotShadowVersion = false;

    this->baseDir = baseDir;

    Purge();

    Lexer lexer;
    lexer.Init(Lexer::Flag::NoErrors);
    lexer.Load(text, Str::Length(text), hashName);

    // '{'
    if (!lexer.ExpectPunctuation(Lexer::PuncType::BraceOpen)) {
        return false;
    }

    Str token;
    while (lexer.ReadToken(&token)) {
        if (token.IsEmpty()) {
            break;
        } else if (token[0] == '}') {
            break;
        } else if (!token.Icmp("litSurface")) {
            flags |= Flag::LitSurface;
        } else if (!token.Icmp("skySurface")) {
            flags |= Flag::SkySurface;
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
                BE_WARNLOG("missing inheritProperties name in shader '%s'\n", hashName.c_str());
            }
        } else if (!token.Icmp("indirectLitVersion")) {
            if (lexer.ReadToken(&token)) {
                Str path = baseDir;
                path.AppendPath(token, '/');

                indirectLitVersion = shaderManager.GetShader(path);
            } else {
                BE_WARNLOG("missing indirectLitVersion name in shader '%s'\n", hashName.c_str());
            }
        } else if (!token.Icmp("directLitVersion")) {
            if (lexer.ReadToken(&token)) {
                Str path = baseDir;
                path.AppendPath(token, '/');

                directLitVersion = shaderManager.GetShader(path);
            } else {
                BE_WARNLOG("missing directLitVersion name in shader '%s'\n", hashName.c_str());
            }
        } else if (!token.Icmp("indirectLitDirectLitVersion")) {
            if (lexer.ReadToken(&token)) {
                Str path = baseDir;
                path.AppendPath(token, '/');

                indirectLitDirectLitVersion = shaderManager.GetShader(path);
            } else {
                BE_WARNLOG("missing indirectLitDirectLitVersion name in shader '%s'\n", hashName.c_str());
            }
        } else if (!token.Icmp("perforatedVersion")) {
            if (lexer.ReadToken(&token)) {
                Str path = baseDir;
                path.AppendPath(token, '/');

                perforatedVersion = shaderManager.GetShader(path);
            } else {
                BE_WARNLOG("missing perforatedVersion name in shader '%s'\n", hashName.c_str());
            }
        } else if (!token.Icmp("parallelShadowVersion")) {
            if (lexer.ReadToken(&token)) {
                Str path = baseDir;
                path.AppendPath(token, '/');

                parallelShadowVersion = shaderManager.GetShader(path);
            } else {
                BE_WARNLOG("missing parallelShadowVersion name in shader '%s'\n", hashName.c_str());
            }
        } else if (!token.Icmp("spotShadowVersion")) {
            if (lexer.ReadToken(&token)) {
                Str path = baseDir;
                path.AppendPath(token, '/');

                spotShadowVersion = shaderManager.GetShader(path);
            } else {
                BE_WARNLOG("missing spotShadowVersion name in shader '%s'\n", hashName.c_str());
            }
        } else if (!token.Icmp("pointShadowVersion")) {
            if (lexer.ReadToken(&token)) {
                Str path = baseDir;
                path.AppendPath(token, '/');

                pointShadowVersion = shaderManager.GetShader(path);
            } else {
                BE_WARNLOG("missing pointShadowVersion name in shader '%s'\n", hashName.c_str());
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
                        BE_WARNLOG("missing gpuSkinningVersion name in shader '%s'\n", hashName.c_str());
                    }
                } else {
                    BE_WARNLOG("missing gpuSkinningVersion name in shader '%s'\n", hashName.c_str());
                }
            } else {
                BE_WARNLOG("missing gpuSkinningVersion name in shader '%s'\n", hashName.c_str());
            }
        } else if (!token.Icmp("generatePerforatedVersion")) {
            generatePerforatedVersion = true;
        } else if (!token.Icmp("generatePremulAlphaVersion")) {
            generatePremulAlphaVersion = true;
        } else if (!token.Icmp("generateGpuSkinningVersion")) {
            generateGpuSkinningVersion = true;
        } else if (!token.Icmp("generateGpuInstancingVersion")) {
            generateGpuInstancingVersion = true;
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
            BE_WARNLOG("unknown parameter %s in shader '%s'\n", token.c_str(), hashName.c_str());
        }
    }

    if (renderGlobal.instancingMethod == Mesh::InstancingMethod::NoInstancing) {
        generateGpuInstancingVersion = false;
    }

    return Finish(generatePerforatedVersion, generateGpuSkinningVersion, generateGpuInstancingVersion, generateParallelShadowVersion, generateSpotShadowVersion, generatePointShadowVersion);
}

bool Shader::ShaderPropertyInfo::Parse(Lexer &lexer) {
    type = Variant::Type::None;
    flags = PropertyInfo::Flag::Editor;
    shaderFlags = 0;
    range = Rangef(0, 0, 1);
    metaObject = nullptr;

    if (!lexer.ReadToken(&name, false)) {
        return false;
    }

    if (!lexer.ExpectPunctuation(Lexer::PuncType::ParenthesesOpen)) {
        return false;
    }

    if (!lexer.ExpectTokenType(Lexer::TokenType::String, &label)) {
        return false;
    }

    if (!lexer.ExpectPunctuation(Lexer::PuncType::ParenthesesClose)) {
        return false;
    }

    if (!lexer.ExpectPunctuation(Lexer::PuncType::Colon)) {
        return false;
    }

    Str typeStr;
    if (!lexer.ReadToken(&typeStr, false)) {
        return false;
    }

    if (!Str::Cmp(typeStr, "bool")) {
        type = Variant::Type::Bool;
    } else if (!Str::Cmp(typeStr, "int")) {
        type = Variant::Type::Int;
    } else if (!Str::Cmp(typeStr, "point")) {
        type = Variant::Type::Point;
    } else if (!Str::Cmp(typeStr, "size")) {
        type = Variant::Type::Size;
    } else if (!Str::Cmp(typeStr, "rect")) {
        type = Variant::Type::Rect;
    } else if (!Str::Cmp(typeStr, "float")) {
        type = Variant::Type::Float;
    } else if (!Str::Cmp(typeStr, "vec2")) {
        type = Variant::Type::Vec2;
    } else if (!Str::Cmp(typeStr, "vec3")) {
        type = Variant::Type::Vec3;
    } else if (!Str::Cmp(typeStr, "vec4")) {
        type = Variant::Type::Vec4;
    } else if (!Str::Cmp(typeStr, "color3")) {
        type = Variant::Type::Color3;
    } else if (!Str::Cmp(typeStr, "color4")) {
        type = Variant::Type::Color4;
    } else if (!Str::Cmp(typeStr, "enum")) {
        Str enumSequence;
        if (!lexer.ExpectTokenType(Lexer::TokenType::String, &enumSequence)) {
            return false;
        }
        type = Variant::Type::Int;
        enumeration.Clear();
        SplitStringIntoList(enumeration, enumSequence, ";");
    } else if (!Str::Cmp(typeStr, "texture")) {
        type = Variant::Type::Guid;

        Str textureTypeStr;
        if (!lexer.ReadToken(&textureTypeStr, false)) {
            return false;
        }

        if (!Str::Icmp(textureTypeStr, "2D")) {
            metaObject = &Texture2DResource::metaObject;
        } else if (!Str::Icmp(textureTypeStr, "CUBE")) {
            metaObject = &TextureCubeMapResource::metaObject;
        } else if (!Str::Icmp(textureTypeStr, "SPRITE")) {
            metaObject = &TextureSpriteResource::metaObject;
        } else {
            BE_WARNLOG("unknown texture type %s in shader '%s'\n", textureTypeStr.c_str(), lexer.GetFilename());
            metaObject = &Texture2DResource::metaObject;
        }
    }

    if (type == Variant::Type::Int ||
        type == Variant::Type::Float ||
        type == Variant::Type::Vec2 ||
        type == Variant::Type::Vec3 ||
        type == Variant::Type::Vec4) {
        Str token;
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

    if (!lexer.ExpectPunctuation(Lexer::PuncType::Assign)) {
        return false;
    }

    Str defaultValueString;
    if (!lexer.ExpectTokenType(Lexer::TokenType::String, &defaultValueString)) {
        return false;
    }

    if (!Str::Cmp(typeStr, "texture")) {
        defaultValue = resourceGuidMapper.Get(defaultValueString);
    } else {
        defaultValue = Variant::FromString(type, defaultValueString);
    }

    Str token;
    lexer.ReadToken(&token, false);
    if (token == "(") {
        while (lexer.ReadToken(&token, false)) {
            if (token == ")") {
                break;
            } else if (token == "condition") {
                Str name;
                Array<int> valueList;

                while (lexer.ReadToken(&token, false)) {
                    if (token == ")") {
                        conditions.Append(Condition(name, valueList));

                        lexer.UnreadToken(&token);
                        break;
                    } else if (lexer.GetTokenType() & Lexer::TokenType::Identifier) {
                        name = token;
                    } else if (lexer.GetTokenType() & Lexer::TokenType::Integer) {
                        int value = Str::ToI32(token);
                        valueList.Append(value);
                    }
                }
            } else {
                return false;
            }
        }
    } else {
        if (!token.IsEmpty()) {
            lexer.UnreadToken(&token);
        }
    }

    lexer.ReadToken(&token, false);
    if (token == "[") {
        while (lexer.ReadToken(&token, false)) {
            if (token == "]") {
                break;
            } else if (token == "shaderDefine") {
                shaderFlags |= ShaderPropertyInfo::Flag::ShaderDefine;
            } else if (token == "normal") {
                shaderFlags |= ShaderPropertyInfo::Flag::Normal;
            } else if (token == "linear") {
                shaderFlags |= ShaderPropertyInfo::Flag::Linear;
            }
        }
    } else {
        if (!token.IsEmpty()) {
            lexer.UnreadToken(&token);
        }
    }

    return true;
}

bool Shader::ParseProperties(Lexer &lexer) {
    Str token;

    if (!lexer.ExpectPunctuation(Lexer::PuncType::BraceOpen)) {
        return false;
    }

    while (lexer.ReadToken(&token)) {
        if (token[0] == '}') {
            break;
        } else {
            if (propertyInfoHashMap.Get(token)) {
                BE_WARNLOG("same property name '%s' ignored in shader '%s'\n", token.c_str(), hashName.c_str());
                lexer.SkipRestOfLine();
                continue;
            }

            lexer.UnreadToken(&token);

            ShaderPropertyInfo shaderPropInfo;
            if (!shaderPropInfo.Parse(lexer)) {
                BE_WARNLOG("error occured in parsing property propInfo in shader '%s'\n", hashName.c_str());
                lexer.SkipRestOfLine();
                continue;
            }

            propertyInfoHashMap.Set(token, shaderPropInfo);
        }
    }

    return true;
}

Shader *Shader::GenerateSubShader(const Str &shaderNamePostfix, const Str &vsHeaderText, const Str &fsHeaderText, bool shadowing, int skinningWeightCount, bool instancing) {
    Str skinningPostfix;
    Str skinningVsHeaderText;
    Str skinningVsHeaderTextCommon;

    skinningVsHeaderTextCommon = "#ifdef VTF_SKINNING\n"
        "#ifdef USE_BUFFER_TEXTURE\n"
        "#define VTF_SKINNING_TC_TYPE int\n"
        "#else\n"
        "#define VTF_SKINNING_TC_TYPE vec2\n"
        "#endif\n"
        "#endif\n";

    switch (skinningWeightCount) {
    case 0:
        skinningPostfix = "";
        break;
    case 1:
        skinningPostfix = "-skinning1";
        skinningVsHeaderText = "#define GPU_SKINNING\n" "#define GPU_SKINNING_1_WEIGHTS\n" + skinningVsHeaderTextCommon;
        break;
    case 4:
        skinningPostfix = "-skinning4";
        skinningVsHeaderText = "#define GPU_SKINNING\n" "#define GPU_SKINNING_4_WEIGHTS\n" + skinningVsHeaderTextCommon;
        break;
    case 8:
        skinningPostfix = "-skinning8";
        skinningVsHeaderText = "#define GPU_SKINNING\n" "#define GPU_SKINNING_8_WEIGHTS\n" + skinningVsHeaderTextCommon;
        break;
    default:
        assert(0);
        break;
    }

    Str instancingPostfix;
    Str instancingVsHeaderText;

    if (instancing) {
        instancingPostfix = "-instancing";
        instancingVsHeaderText = "#define INSTANCING\n";
    }

    Str shaderName = hashName + shaderNamePostfix + skinningPostfix + instancingPostfix;
    Shader *shader = shaderManager.FindShader(shaderName);
    if (shader) {
        shader->AddRefCount();
        return shader;
    }

    shader = shaderManager.AllocShader(shaderName);

    Str text = "{\n";
    text += "glsl_vp { " + vsHeaderText + instancingVsHeaderText + skinningVsHeaderText + vsText + " }\n";
    text += "glsl_fp { " + fsHeaderText + fsText + " }\n";
    text += "}";
    if (!shader->Create(text, baseDir)) {
        return nullptr;
    }

    if (flags & Flag::LitSurface) {
        shader->flags |= Flag::LitSurface;
    }

    if (shadowing) {
        shader->flags |= Flag::Shadowing;
    }

    shader->propertyInfoHashMap = propertyInfoHashMap;

    return shader;
}

bool Shader::GenerateGpuSkinningVersion(Shader *shader, const Str &shaderNamePostfix, const Str &vsHeaderText, const Str &fsHeaderText,
    bool shadowing, bool generateGpuInstancingVersion) {
    if (!shader->gpuSkinningVersion[0]) {
        shader->gpuSkinningVersion[0] = GenerateSubShader(shaderNamePostfix, vsHeaderText, fsHeaderText, shadowing, 1, false);
        if (!shader->gpuSkinningVersion[0]) {
            return false;
        }
    }

    if (!shader->gpuSkinningVersion[1]) {
        shader->gpuSkinningVersion[1] = GenerateSubShader(shaderNamePostfix, vsHeaderText, fsHeaderText, shadowing, 4, false);
        if (!shader->gpuSkinningVersion[1]) {
            return false;
        }
    }

    if (!shader->gpuSkinningVersion[2]) {
        shader->gpuSkinningVersion[2] = GenerateSubShader(shaderNamePostfix, vsHeaderText, fsHeaderText, shadowing, 8, false);
        if (!shader->gpuSkinningVersion[2]) {
            return false;
        }
    }

    if (generateGpuInstancingVersion) {
        if (!shader->gpuSkinningVersion[0]->gpuInstancingVersion) {
            shader->gpuSkinningVersion[0]->gpuInstancingVersion = GenerateSubShader(shaderNamePostfix, vsHeaderText, fsHeaderText, shadowing, 1, true);
            if (!shader->gpuSkinningVersion[0]->gpuInstancingVersion) {
                return false;
            }
        }

        if (!shader->gpuSkinningVersion[1]->gpuInstancingVersion) {
            shader->gpuSkinningVersion[1]->gpuInstancingVersion = GenerateSubShader(shaderNamePostfix, vsHeaderText, fsHeaderText, shadowing, 4, true);
            if (!shader->gpuSkinningVersion[1]->gpuInstancingVersion) {
                return false;
            }
        }

        if (!shader->gpuSkinningVersion[2]->gpuInstancingVersion) {
            shader->gpuSkinningVersion[2]->gpuInstancingVersion = GenerateSubShader(shaderNamePostfix, vsHeaderText, fsHeaderText, shadowing, 8, true);
            if (!shader->gpuSkinningVersion[2]->gpuInstancingVersion) {
                return false;
            }
        }
    }

    return true;
}

bool Shader::GeneratePerforatedVersion(Shader *shader, const Str &shaderNamePostfix, const Str &vsHeaderText, const Str &fsHeaderText,
    bool shadowing, bool genereateGpuSkinningVersion, bool generateGpuInstancingVersion) {
    if (!shader->perforatedVersion) {
        shader->perforatedVersion = GenerateSubShader(shaderNamePostfix + "-perforated",
            vsHeaderText + "#define PERFORATED\n", fsHeaderText + "#define PERFORATED\n", shadowing, 0, false);
        if (!shader->perforatedVersion) {
            return false;
        }
    }

    if (genereateGpuSkinningVersion) {
        if (!GenerateGpuSkinningVersion(shader->perforatedVersion,
            shaderNamePostfix + "-perforated", vsHeaderText + "#define PERFORATED\n", fsHeaderText + "#define PERFORATED\n", shadowing, false)) {
            return false;
        }
    }

    if (generateGpuInstancingVersion) {
        if (!shader->perforatedVersion->gpuInstancingVersion) {
            shader->perforatedVersion->gpuInstancingVersion = GenerateSubShader(shaderNamePostfix + "-perforated",
                vsHeaderText + "#define PERFORATED\n", fsHeaderText + "#define PERFORATED\n", shadowing, 0, true);
            if (!shader->perforatedVersion->gpuInstancingVersion) {
                return false;
            }
        }

        if (genereateGpuSkinningVersion) {
            if (!GenerateGpuSkinningVersion(shader->perforatedVersion->gpuInstancingVersion,
                shaderNamePostfix + "-perforated", vsHeaderText + "#define PERFORATED\n", fsHeaderText + "#define PERFORATED\n", shadowing, true)) {
                return false;
            }
        }
    }

    return true;
}

bool Shader::GeneratePremulAlphaVersion(Shader *shader, const Str &shaderNamePostfix, const Str &vsHeaderText, const Str &fsHeaderText,
    bool shadowing, bool genereateGpuSkinningVersion, bool generateGpuInstancingVersion) {
    if (!shader->premulAlphaVersion) {
        shader->premulAlphaVersion = GenerateSubShader(shaderNamePostfix + "-premulAlpha",
            vsHeaderText + "#define PREMULTIPLIED_ALPHA\n", fsHeaderText + "#define PREMULTIPLIED_ALPHA\n", shadowing, 0, false);
        if (!shader->premulAlphaVersion) {
            return false;
        }
    }

    if (genereateGpuSkinningVersion) {
        if (!GenerateGpuSkinningVersion(shader->premulAlphaVersion,
            shaderNamePostfix + "-premulAlpha", vsHeaderText + "#define PREMULTIPLIED_ALPHA\n", fsHeaderText + "#define PREMULTIPLIED_ALPHA\n", shadowing, false)) {
            return false;
        }
    }

    if (generateGpuInstancingVersion) {
        if (!shader->premulAlphaVersion->gpuInstancingVersion) {
            shader->premulAlphaVersion->gpuInstancingVersion = GenerateSubShader(shaderNamePostfix + "-premulAlpha",
                vsHeaderText + "#define PREMULTIPLIED_ALPHA\n", fsHeaderText + "#define PREMULTIPLIED_ALPHA\n", shadowing, 0, true);
            if (!shader->premulAlphaVersion->gpuInstancingVersion) {
                return false;
            }
        }

        if (genereateGpuSkinningVersion) {
            if (!GenerateGpuSkinningVersion(shader->premulAlphaVersion->gpuInstancingVersion,
                shaderNamePostfix + "-premulAlpha", vsHeaderText + "#define PREMULTIPLIED_ALPHA\n", fsHeaderText + "#define PREMULTIPLIED_ALPHA\n", shadowing, true)) {
                return false;
            }
        }
    }

    return true;
}

bool Shader::Finish(bool generatePerforatedVersion,
    bool genereateGpuSkinningVersion, bool generateGpuInstancingVersion,
    bool generateParallelShadowVersion, bool generateSpotShadowVersion, bool generatePointShadowVersion) {
    if (generateGpuInstancingVersion) {
        if (!gpuInstancingVersion) {
            gpuInstancingVersion = GenerateSubShader("", "", "", false, 0, true);
            if (!gpuInstancingVersion) {
                return false;
            }
        }
    }

    if (genereateGpuSkinningVersion) {
        if (!GenerateGpuSkinningVersion(this, "", "", "", false, generateGpuInstancingVersion)) {
            return false;
        }
    }

    if (generatePerforatedVersion) {
        if (!GeneratePerforatedVersion(this, "", "", "", false, genereateGpuSkinningVersion, generateGpuInstancingVersion)) {
            return false;
        }
    }

    if (generateParallelShadowVersion) {
        const Str shaderNamePostfix = "-dirShadowed";
        const Str vsHeaderText = "#define USE_SHADOW_CASCADE\n";
        const Str fsHeaderText = "#define USE_SHADOW_CASCADE\n";

        if (!parallelShadowVersion) {
            parallelShadowVersion = GenerateSubShader(shaderNamePostfix, vsHeaderText, fsHeaderText, true, 0, false);
            if (!parallelShadowVersion) {
                return false;
            }

            if (generateGpuInstancingVersion && !parallelShadowVersion->gpuInstancingVersion) {
                parallelShadowVersion->gpuInstancingVersion = GenerateSubShader(shaderNamePostfix, vsHeaderText, fsHeaderText, true, 0, false);
                if (!parallelShadowVersion->gpuInstancingVersion) {
                    return false;
                }
            }
        }

        if (genereateGpuSkinningVersion) {
            if (!GenerateGpuSkinningVersion(parallelShadowVersion, shaderNamePostfix, vsHeaderText, fsHeaderText, true, generateGpuInstancingVersion)) {
                return false;
            }
        }

        if (generatePerforatedVersion) {
            if (!GeneratePerforatedVersion(parallelShadowVersion, shaderNamePostfix, vsHeaderText, fsHeaderText, true, genereateGpuSkinningVersion, generateGpuInstancingVersion)) {
                return false;
            }
        }
    }

    if (generateSpotShadowVersion) {
        const Str shaderNamePostfix = "-spotShadowed";
        const Str vsHeaderText = "#define USE_SHADOW_SPOT\n";
        const Str fsHeaderText = "#define USE_SHADOW_SPOT\n";

        if (!spotShadowVersion) {
            spotShadowVersion = GenerateSubShader(shaderNamePostfix, vsHeaderText, fsHeaderText, true, 0, false);
            if (!spotShadowVersion) {
                return false;
            }

            if (generateGpuInstancingVersion && !spotShadowVersion->gpuInstancingVersion) {
                spotShadowVersion->gpuInstancingVersion = GenerateSubShader(shaderNamePostfix, vsHeaderText, fsHeaderText, true, 0, false);
                if (!spotShadowVersion->gpuInstancingVersion) {
                    return false;
                }
            }
        }

        if (genereateGpuSkinningVersion) {
            if (!GenerateGpuSkinningVersion(spotShadowVersion, shaderNamePostfix, vsHeaderText, fsHeaderText, true, generateGpuInstancingVersion)) {
                return false;
            }
        }

        if (generatePerforatedVersion) {
            if (!GeneratePerforatedVersion(spotShadowVersion, shaderNamePostfix, vsHeaderText, fsHeaderText, true, genereateGpuSkinningVersion, generateGpuInstancingVersion)) {
                return false;
            }
        }
    }

    if (generatePointShadowVersion) {
        const Str shaderNamePostfix = "-pointShadowed";
        const Str vsHeaderText = "#define USE_SHADOW_POINT\n";
        const Str fsHeaderText = "#define USE_SHADOW_POINT\n";

        if (!pointShadowVersion) {
            pointShadowVersion = GenerateSubShader(shaderNamePostfix, vsHeaderText, fsHeaderText, true, 0, false);
            if (!pointShadowVersion) {
                return false;
            }

            if (generateGpuInstancingVersion && !pointShadowVersion->gpuInstancingVersion) {
                pointShadowVersion->gpuInstancingVersion = GenerateSubShader(shaderNamePostfix, vsHeaderText, fsHeaderText, true, 0, false);
                if (!pointShadowVersion->gpuInstancingVersion) {
                    return false;
                }
            }
        }

        if (genereateGpuSkinningVersion) {
            if (!GenerateGpuSkinningVersion(pointShadowVersion, shaderNamePostfix, vsHeaderText, fsHeaderText, true, generateGpuInstancingVersion)) {
                return false;
            }
        }

        if (generatePerforatedVersion) {
            if (!GeneratePerforatedVersion(pointShadowVersion, shaderNamePostfix, vsHeaderText, fsHeaderText, true, genereateGpuSkinningVersion, generateGpuInstancingVersion)) {
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
        sortedDefineArray.StableSort([](const Shader::Define &a, const Shader::Define &b) -> bool {
            return (a.name).Icmp(b.name) < 0;
        });

        for (int i = 0; i < defineArray.Count(); i++) {
            // Skip define value is 0
            if (defineArray[i].value == 0) {
                continue;
            }
            mangledName += "+" + defineArray[i].name + "=" + defineArray[i].value;
        }
    }

    return mangledName.c_str();
}

Shader *Shader::InstantiateShader(const Array<Define> &defineArray) {
    Str mangledName;
    // Instantiated shader name start with '@' character.
    MangleNameWithDefineList("@" + name, defineArray, mangledName);

    // Return instantiated shader if it already exist.
    Shader *shader = shaderManager.FindShader(mangledName);
    if (shader) {
        // Instantiated shader always has pointer to original shader.
        assert(shader->originalShader);
        shader->AddRefCount();
        return shader;
    }

    // Allocate new instantiated shader.
    shader = shaderManager.AllocShader(mangledName);
    shader->originalShader = this;
    shader->flags = flags;
    shader->defineArray = defineArray;
    shader->baseDir = baseDir;

    instantiatedShaders.Append(shader);

    shader->InstantiateShaderInternal(defineArray);

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

Shader *Shader::GetIndirectLitVersion() {
    if (indirectLitVersion) {
        return indirectLitVersion;
    }

    if (originalShader) {
        if (originalShader->indirectLitVersion) {
            indirectLitVersion = originalShader->indirectLitVersion->InstantiateShader(defineArray);
            return indirectLitVersion;
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

Shader *Shader::GetIndirectLitDirectLitVersion() {
    if (indirectLitDirectLitVersion) {
        return indirectLitDirectLitVersion;
    }

    if (originalShader) {
        if (originalShader->indirectLitDirectLitVersion) {
            indirectLitDirectLitVersion = originalShader->indirectLitDirectLitVersion->InstantiateShader(defineArray);
            return indirectLitDirectLitVersion;
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

Shader *Shader::GetGPUInstancingVersion() {
    if (gpuInstancingVersion) {
        return gpuInstancingVersion;
    }

    if (originalShader) {
        if (originalShader->gpuInstancingVersion) {
            gpuInstancingVersion = originalShader->gpuInstancingVersion->InstantiateShader(defineArray);
            return gpuInstancingVersion;
        }
    }

    return nullptr;
}

void Shader::Reinstantiate() {
    assert(originalShader);
    InstantiateShaderInternal(defineArray);

    flags ^= Flag::NeedReinstatiate;

    if (originalShader->indirectLitVersion) {
        if (indirectLitVersion) {
            indirectLitVersion->originalShader = originalShader->indirectLitVersion;
            indirectLitVersion->Reinstantiate();
        } else {
            //indirectLitVersion = originalShader->indirectLitVersion->InstantiateShader(defineArray);
        }
    } else {
        if (indirectLitVersion) {
            shaderManager.ReleaseShader(indirectLitVersion);
            indirectLitVersion = nullptr;
        }
    }

    if (originalShader->directLitVersion) {
        if (directLitVersion) {
            directLitVersion->originalShader = originalShader->directLitVersion;
            directLitVersion->Reinstantiate();
        } else {
            //directLitVersion = originalShader->directLitVersion->InstantiateShader(defineArray);
        }
    } else {
        if (directLitVersion) {
            shaderManager.ReleaseShader(directLitVersion);
            directLitVersion = nullptr;
        }
    }

    if (originalShader->indirectLitDirectLitVersion) {
        if (indirectLitDirectLitVersion) {
            indirectLitDirectLitVersion->originalShader = originalShader->indirectLitDirectLitVersion;
            indirectLitDirectLitVersion->Reinstantiate();
        } else {
            //indirectLitDirectLitVersion = originalShader->indirectLitDirectLitVersion->InstantiateShader(defineArray);
        }
    } else {
        if (indirectLitDirectLitVersion) {
            shaderManager.ReleaseShader(indirectLitDirectLitVersion);
            indirectLitDirectLitVersion = nullptr;
        }
    }

    if (originalShader->perforatedVersion) {
        if (perforatedVersion) {
            perforatedVersion->originalShader = originalShader->perforatedVersion;
            perforatedVersion->Reinstantiate();
        } else {
            //perforatedVersion = originalShader->perforatedVersion->InstantiateShader(defineArray);
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
            //premulAlphaVersion = originalShader->premulAlphaVersion->InstantiateShader(defineArray);
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
            //parallelShadowVersion = originalShader->parallelShadowVersion->InstantiateShader(defineArray);
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
            //spotShadowVersion = originalShader->spotShadowVersion->InstantiateShader(defineArray);
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
            //pointShadowVersion = originalShader->pointShadowVersion->InstantiateShader(defineArray);
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
                //gpuSkinningVersion[i] = originalShader->gpuSkinningVersion[i]->InstantiateShader(defineArray);
            }
        } else {
            if (gpuSkinningVersion[i]) {
                shaderManager.ReleaseShader(gpuSkinningVersion[i]);
                gpuSkinningVersion[i] = nullptr;
            }
        }
    }

    if (originalShader->gpuInstancingVersion) {
        if (gpuInstancingVersion) {
            gpuInstancingVersion->originalShader = originalShader->gpuInstancingVersion;
            gpuInstancingVersion->Reinstantiate();
        } else {
            //gpuInstancingVersion = originalShader->gpuInstancingVersion->InstantiateShader(defineArray);
        }
    } else {
        if (gpuInstancingVersion) {
            shaderManager.ReleaseShader(gpuInstancingVersion);
            gpuInstancingVersion = nullptr;
        }
    }
}

bool Shader::InstantiateShaderInternal(const Array<Define> &defineArray) {
    Str processedVsText;
    Str processedFsText;

    flags |= ProcessShaderText(originalShader->vsText, originalShader->baseDir, defineArray, processedVsText) ? Flag::HasVertexShader : 0;
    flags |= ProcessShaderText(originalShader->fsText, originalShader->baseDir, defineArray, processedFsText) ? Flag::HasFragmentShader : 0;

    if (!(flags & Flag::HasVertexShader) || !(flags & Flag::HasFragmentShader)) {
        return false;
    }

    if (shaderHandle != RHI::NullShader) {
        rhi.DestroyShader(shaderHandle);
    }

    shaderHandle = rhi.CreateShader(hashName, processedVsText, processedFsText);

    assert(BuiltInConstant::Count == COUNT_OF(builtInConstantNames));
    assert(BuiltInTexture::Count == COUNT_OF(builtInTextureNames));

    for (int i = 0; i < BuiltInConstant::Count; i++) {
        builtInConstantIndices[i] = rhi.GetShaderConstantIndex(shaderHandle, builtInConstantNames[i]);
    }

    for (int i = 0; i < BuiltInTexture::Count; i++) {
        builtInTextureUnits[i] = rhi.GetShaderTextureUnit(shaderHandle, builtInTextureNames[i]);
    }

    return true;
}

bool Shader::ProcessShaderText(const char *text, const char *baseDir, const Array<Define> &defineArray, Str &outStr) const {
    outStr = text;

    // Insert local define array.
    for (int i = 0; i < defineArray.Count(); i++) {
        outStr.Insert(va("#define %s %i\n", defineArray[i].name.c_str(), defineArray[i].value), 0);
    }

    // Insert global define array.
    for (int i = shaderManager.globalHeaderList.Count() - 1; i >= 0; i--) {
        outStr.Insert(shaderManager.globalHeaderList[i].c_str(), 0);
    }

    ProcessIncludeRecursive(baseDir, outStr);

    return true;
}

bool Shader::ProcessIncludeRecursive(const char *baseDir, Str &outText) const {
    Lexer lexer;
    lexer.Init(Lexer::Flag::NoErrors);

    int pos = 0;

    do {
        pos = outText.Find(directiveInclude, true, pos);
        if (pos == -1) {
            return true;
        }

        const char *data_p = outText.c_str() + pos + Str::Length(directiveInclude);
        lexer.Load(data_p, Str::Length(data_p), hashName);

        Str relativeFileName;
        lexer.ExpectTokenType(Lexer::TokenType::String, &relativeFileName);

        Str path = baseDir;
        path.AppendPath(relativeFileName);

        char *includingText;
        size_t fileLen = fileSystem.LoadFile(path.c_str(), true, (void **)&includingText);
        if (!fileLen) {
            BE_FATALERROR("Shader::ProcessIncludeRecursive: Cannot open include file '%s'", path.c_str());
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

void Shader::Bind() {
    if (flags & Shader::Flag::NeedReinstatiate) {
        Reinstantiate();
    }

    rhi.BindShader(shaderHandle);
}

int Shader::GetConstantIndex(const char *name) const {
    return rhi.GetShaderConstantIndex(shaderHandle, name);
}

int Shader::GetConstantBlockIndex(const char *name) const {
    return rhi.GetShaderConstantBlockIndex(shaderHandle, name);
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

void Shader::SetConstant1ui(int index, const unsigned int constant) const {
    rhi.SetShaderConstant1ui(index, constant);
}

void Shader::SetConstant2ui(int index, const unsigned int *constant) const {
    rhi.SetShaderConstant2ui(index, constant);
}

void Shader::SetConstant3ui(int index, const unsigned int *constant) const {
    rhi.SetShaderConstant3ui(index, constant);
}

void Shader::SetConstant4ui(int index, const unsigned int *constant) const {
    rhi.SetShaderConstant4ui(index, constant);
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

void Shader::SetConstant2x2f(int index, bool rowMajor, const Mat2 &constant) const {
    rhi.SetShaderConstant2x2f(index, rowMajor, constant);
}

void Shader::SetConstant3x3f(int index, bool rowMajor, const Mat3 &constant) const {
    rhi.SetShaderConstant3x3f(index, rowMajor, constant);
}

void Shader::SetConstant4x4f(int index, bool rowMajor, const Mat4 &constant) const {
    rhi.SetShaderConstant4x4f(index, rowMajor, constant);
}

void Shader::SetConstant4x3f(int index, bool rowMajor, const Mat3x4 &constant) const {
    rhi.SetShaderConstant4x3f(index, rowMajor, constant);
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

void Shader::SetConstantArray2x2f(int index, bool rowMajor, int num, const Mat2 *constant) const {
    rhi.SetShaderConstantArray2x2f(index, rowMajor, num, constant);
}

void Shader::SetConstantArray3x3f(int index, bool rowMajor, int num, const Mat3 *constant) const {
    rhi.SetShaderConstantArray3x3f(index, rowMajor, num, constant);
}

void Shader::SetConstantArray4x4f(int index, bool rowMajor, int num, const Mat4 *constant) const {
    rhi.SetShaderConstantArray4x4f(index, rowMajor, num, constant);
}

void Shader::SetConstantArray4x3f(int index, bool rowMajor, int num, const Mat3x4 *constant) const {
    rhi.SetShaderConstantArray4x3f(index, rowMajor, num, constant);
}

void Shader::SetConstantBuffer(int index, int bindingIndex) const {
    rhi.SetShaderConstantBlock(index, bindingIndex);
}

void Shader::SetConstant1i(const char *name, int x) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstant1i: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant1i(index, x);
}

void Shader::SetConstant2i(const char *name, const int *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstant2i: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant2i(index, constant);
}

void Shader::SetConstant3i(const char *name, const int *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstant3i: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant3i(index, constant);
}

void Shader::SetConstant4i(const char *name, const int *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstant4i: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant4i(index, constant);
}

void Shader::SetConstant1ui(const char *name, unsigned int x) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstant1ui: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant1ui(index, x);
}

void Shader::SetConstant2ui(const char *name, const unsigned int *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstant2ui: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant2ui(index, constant);
}

void Shader::SetConstant3ui(const char *name, const unsigned int *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstant3ui: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant3ui(index, constant);
}

void Shader::SetConstant4ui(const char *name, const unsigned int *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstant4ui: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant4ui(index, constant);
}

void Shader::SetConstant1f(const char *name, float x) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstant1f: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant1f(index, x);
}

void Shader::SetConstant2f(const char *name, const float *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstant2f: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant2f(index, constant);
}

void Shader::SetConstant3f(const char *name, const float *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstant3f: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant3f(index, constant);
}

void Shader::SetConstant4f(const char *name, const float *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstant4f: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant4f(index, constant);
}

void Shader::SetConstant2f(const char *name, const Vec2 &constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstant2f: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant2f(index, constant);
}

void Shader::SetConstant3f(const char *name, const Vec3 &constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstant3f: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant3f(index, constant);
}

void Shader::SetConstant4f(const char *name, const Vec4 &constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstant4f: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant4f(index, constant);
}

void Shader::SetConstant2x2f(const char *name, bool rowMajor, const Mat2 &constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstant2x2f: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant2x2f(index, rowMajor, constant);
}

void Shader::SetConstant3x3f(const char *name, bool rowMajor, const Mat3 &constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstant3x3f: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant3x3f(index, rowMajor, constant);
}

void Shader::SetConstant4x4f(const char *name, bool rowMajor, const Mat4 &constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstant4x4f: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant4x4f(index, rowMajor, constant);
}

void Shader::SetConstant4x3f(const char *name, bool rowMajor, const Mat3x4 &constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstant4x3f: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstant4x3f(index, rowMajor, constant);
}

void Shader::SetConstantArray1i(const char *name, int num, const int *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstantArray1i: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray1i(index, num, constant);
}

void Shader::SetConstantArray2i(const char *name, int num, const int *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstantArray2i: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray2i(index, num, constant);
}

void Shader::SetConstantArray3i(const char *name, int num, const int *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstantArray3i: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray3i(index, num, constant);
}

void Shader::SetConstantArray4i(const char *name, int num, const int *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstantArray4i: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray4i(index, num, constant);
}

void Shader::SetConstantArray1f(const char *name, int num, const float *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstantArray1f: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray1f(index, num, constant);
}

void Shader::SetConstantArray2f(const char *name, int num, const float *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstantArray2f: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray2f(index, num, constant);
}

void Shader::SetConstantArray3f(const char *name, int num, const float *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstantArray3f: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray3f(index, num, constant);
}

void Shader::SetConstantArray4f(const char *name, int num, const float *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstantArray4f: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray4f(index, num, constant);
}

void Shader::SetConstantArray2f(const char *name, int num, const Vec2 *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstantArray2f: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray2f(index, num, constant);
}

void Shader::SetConstantArray3f(const char *name, int num, const Vec3 *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstantArray3f: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray3f(index, num, constant);
}

void Shader::SetConstantArray4f(const char *name, int num, const Vec4 *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstantArray4f: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray4f(index, num, constant);
}

void Shader::SetConstantArray2x2f(const char *name, bool rowMajor, int num, const Mat2 *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstantArray2x2f: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray2x2f(index, rowMajor, num, constant);
}

void Shader::SetConstantArray3x3f(const char *name, bool rowMajor, int num, const Mat3 *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstantArray3x3f: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray3x3f(index, rowMajor, num, constant);
}

void Shader::SetConstantArray4x4f(const char *name, bool rowMajor, int num, const Mat4 *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstantArray4x4f: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray4x4f(index, rowMajor, num, constant);
}

void Shader::SetConstantArray4x3f(const char *name, bool rowMajor, int num, const Mat3x4 *constant) const {
    int index = rhi.GetShaderConstantIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstantArray4x3f: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantArray4x3f(index, rowMajor, num, constant);
}

void Shader::SetConstantBuffer(const char *name, int bindingIndex) const {
    int index = rhi.GetShaderConstantBlockIndex(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG("Shader::SetConstantBuffer: invalid constant name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }
    rhi.SetShaderConstantBlock(index, bindingIndex);
}

int Shader::GetShaderTextureUnit(const char *name) const {
    return rhi.GetShaderTextureUnit(shaderHandle, name);
}

void Shader::SetTexture(int textureUnit, const Texture *texture) const {
    if (textureUnit < 0) {
        return;
    }

    rhi.SetTexture(textureUnit, texture->textureHandle);
}

void Shader::SetTexture(const char *name, const Texture *texture) const {
    int textureUnit = rhi.GetShaderTextureUnit(shaderHandle, name);
    if (textureUnit < 0) {
        //BE_WARNLOG("Shader::SetTexture: invalid texture name '%s' in shader '%s'\n", name, this->hashName.c_str());
        return;
    }

    rhi.SetTexture(textureUnit, texture->textureHandle);
}

void Shader::SetTextureArray(const char *name, int num, const Texture **textures) const {
    char temp[256];

    strcpy(temp, name);
    char *indexPtr = temp + strlen(temp) + 1;
    strcat(temp, "[0]");

    for (int i = 0; i < num; i++) {
        *indexPtr = '0' + i;

        int textureUnit = rhi.GetShaderTextureUnit(shaderHandle, temp);
        if (textureUnit < 0) {
            //BE_WARNLOG("Shader::SetTextureArray: invalid texture name '%s' in shader '%s'\n", temp, this->hashName.c_str());
            return;
        }

        rhi.SetTexture(textureUnit, textures[i]->textureHandle);
    }
}

bool Shader::Load(const char *hashName) {
    Str filename = hashName;
    filename.DefaultFileExtension(".shader");

    char *data;
    size_t size = fileSystem.LoadFile(filename, true, (void **)&data);
    if (!data) {
        return false;
    }

    Lexer lexer;
    lexer.Init(Lexer::Flag::NoErrors);
    lexer.Load(data, (int)size, filename);

    if (!lexer.ExpectTokenString("shader")) {
        fileSystem.FreeFile(data);
        return false;
    }

    Str name;
    if (!lexer.ReadToken(&name)) {
        fileSystem.FreeFile(data);
        return false;
    }

    Str baseDir = filename;
    baseDir.StripFileName();
    Create(data + lexer.GetCurrentOffset(), baseDir);

    fileSystem.FreeFile(data);

    this->flags |= Flag::LoadedFromFile;

    return true;
}

bool Shader::Reload() {
    Shader *shader = this;
    if (originalShader) {
        shader = originalShader;
    }

    bool ret = true;

    if (shader->flags & Flag::LoadedFromFile) {
        Str _hashName = shader->hashName;
        ret = shader->Load(_hashName);
    }

    for (int i = 0; i < shader->instantiatedShaders.Count(); i++) {
        instantiatedShaders[i]->flags |= Flag::NeedReinstatiate;
    }

    return ret;
}

BE_NAMESPACE_END
