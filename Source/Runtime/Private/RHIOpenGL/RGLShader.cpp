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
#include "RHI/RHIOpenGL.h"
#include "RGLInternal.h"
#include "Platform/PlatformFile.h"
#include "Containers/BinSearch.h"
#include "Simd/Simd.h"
#include "Core/Checksum_MD5.h"
#include "Core/Heap.h"
#include "Core/Lexer.h"

BE_NAMESPACE_BEGIN

struct InOut {
    Str precision;
    Str type;
    Str name;
    int index;
    int location;
};

struct InOutSemantic {
    int index;
    int location;
    const char *name;
};

static const InOutSemantic inOutSemantics[] = {
    // vertex shader input semantics
    { 0, RHI::VertexElement::Position, "POSITION" }, 
    { 1, RHI::VertexElement::Normal, "NORMAL" },
    { 2, RHI::VertexElement::Color, "COLOR" },
    { 3, RHI::VertexElement::SecondaryColor, "SECONDARY_COLOR" },
    { 4, RHI::VertexElement::Tangent, "TANGENT" },
    { 5, RHI::VertexElement::WeightIndex, "WEIGHT_INDEX" },
    { 6, RHI::VertexElement::WeightIndex0, "WEIGHT_INDEX0" },
    { 7, RHI::VertexElement::WeightIndex1, "WEIGHT_INDEX1" },
    { 8, RHI::VertexElement::WeightValue, "WEIGHT_VALUE" },
    { 9, RHI::VertexElement::WeightValue0, "WEIGHT_VALUE0" },
    { 10, RHI::VertexElement::WeightValue1, "WEIGHT_VALUE1" },
    { 11, RHI::VertexElement::TexCoord, "TEXCOORD" },
    { 12, RHI::VertexElement::TexCoord0, "TEXCOORD0" },
    { 13, RHI::VertexElement::TexCoord1, "TEXCOORD1" },
    { 14, RHI::VertexElement::TexCoord2, "TEXCOORD2" },
    { 15, RHI::VertexElement::TexCoord3, "TEXCOORD3" },
    { 16, RHI::VertexElement::TexCoord4, "TEXCOORD4" },
    { 17, RHI::VertexElement::TexCoord5, "TEXCOORD5" },
    { 18, RHI::VertexElement::TexCoord6, "TEXCOORD6" },
    { 19, RHI::VertexElement::TexCoord7, "TEXCOORD7" },

    // fragment shader output semantics
    { 0, 0, "FRAG_COLOR" },
    { 1, 0, "FRAG_COLOR0" },
    { 2, 1, "FRAG_COLOR1" },
    { 3, 2, "FRAG_COLOR2" },
    { 4, 3, "FRAG_COLOR3" },
    { 5, 3, "FRAG_DEPTH" },
};

struct Uniform {
    Str precision;
    Str type;
    Str name;
};

struct FsOutBuiltInVar {
    const char *name;
    int deprecatedVersion;
};

static FsOutBuiltInVar fsOutBuiltInVars[] = {
    { "gl_FragColor", 130 },
    { "gl_FragData[0]", 130 },
    { "gl_FragData[1]", 130 },
    { "gl_FragData[2]", 130 },
    { "gl_FragData[3]", 130 },
    { "gl_FragDepth", 999 } // not deprecated yet
};

static const char *vsInsert = {
    "float saturate(float v) { return clamp(v, 0.0, 1.0); }\n"
    "vec2 saturate(vec2 v) { return clamp(v, 0.0, 1.0); }\n"
    "vec3 saturate(vec3 v) { return clamp(v, 0.0, 1.0); }\n"
    "vec4 saturate(vec4 v) { return clamp(v, 0.0, 1.0); }\n"
    "vec4 tex2Dlod(sampler2D sampler, vec4 texcoord) { return textureLod(sampler, texcoord.xy, texcoord.w); }\n"
    "\n"
};

static const char *fsInsert = {
    "\n"
    "void clip(float v) { if (v < 0.0) { discard; } }\n"
    "void clip(vec2 v) { if (any(lessThan(v, vec2(0.0)))) { discard; } }\n"
    "void clip(vec3 v) { if (any(lessThan(v, vec3(0.0)))) { discard; } }\n"
    "void clip(vec4 v) { if (any(lessThan(v, vec4(0.0)))) { discard; } }\n"
    "\n"
    "float saturate(float v) { return clamp(v, 0.0, 1.0); }\n"
    "vec2 saturate(vec2 v) { return clamp(v, 0.0, 1.0); }\n"
    "vec3 saturate(vec3 v) { return clamp(v, 0.0, 1.0); }\n"
    "vec4 saturate(vec4 v) { return clamp(v, 0.0, 1.0); }\n"
    "\n"
    "vec4 tex2D(sampler2D sampler, vec2 texcoord) { return texture(sampler, texcoord.xy); }\n"
    "vec4 tex2D(sampler2D sampler, vec2 texcoord, vec2 dx, vec2 dy) { return textureGrad(sampler, texcoord.xy, dx, dy); }\n"
    "vec4 tex2Dbias(sampler2D sampler, vec4 texcoord) { return texture(sampler, texcoord.xy, texcoord.w); }\n"
    "vec4 tex2Dproj(sampler2D sampler, vec3 texcoord) { return textureProj(sampler, texcoord); }\n"
    "vec4 tex2Dproj(sampler2D sampler, vec4 texcoord) { return textureProj(sampler, texcoord); }\n"
    "vec4 tex2Dlod(sampler2D sampler, vec4 texcoord) { return textureLod(sampler, texcoord.xy, texcoord.w); }\n"
    "\n"
    "vec4 tex2D(sampler2DShadow sampler, vec3 texcoord) { return vec4(texture(sampler, texcoord.xyz)); }\n"
    "vec4 tex2D(sampler2DShadow sampler, vec3 texcoord, vec2 dx, vec2 dy) { return vec4(textureGrad(sampler, texcoord.xyz, dx, dy)); }\n"
    "\n"
    "vec4 texCUBE(samplerCube sampler, vec3 texcoord) { return texture(sampler, texcoord.xyz); }\n"
    "vec4 texCUBE(samplerCubeShadow sampler, vec4 texcoord) { return vec4(texture(sampler, texcoord.xyzw)); }\n"
    "vec4 texCUBEbias(samplerCube sampler, vec4 texcoord) { return texture(sampler, texcoord.xyz, texcoord.w); }\n"
    "vec4 texCUBElod(samplerCube sampler, vec4 texcoord) { return textureLod(sampler, texcoord.xyz, texcoord.w); }\n"
    "\n"
    "vec4 tex3D(sampler3D sampler, vec3 texcoord) { return texture(sampler, texcoord.xyz); }\n"
    "vec4 tex3Dproj(sampler3D sampler, vec4 texcoord) { return textureProj(sampler, texcoord); }\n"
    "vec4 tex3Dbias(sampler3D sampler, vec4 texcoord) { return texture(sampler, texcoord.xyz, texcoord.w); }\n"
    "vec4 tex3Dlod(sampler3D sampler, vec4 texcoord) { return textureLod(sampler, texcoord.xyz, texcoord.w); }\n"
    "\n"
    "vec4 tex2Darray(sampler2DArray sampler, vec3 texcoord) { return texture(sampler, texcoord.xyz); }\n"
    "vec4 tex2Darray(sampler2DArrayShadow sampler, vec4 texcoord) { return vec4(texture(sampler, texcoord.xyzw)); }\n"
    "\n"
    "#ifdef TEXTURE_RECT\n"
    "vec4 texRECT(sampler2DRect sampler, vec2 texcoord) { return texture(sampler, texcoord.xy); }\n"
    "vec4 texRECTproj(sampler2DRect sampler, vec3 texcoord) { return textureProj(sampler, texcoord); }\n"
    "vec4 texRECTproj(sampler2DRect sampler, vec4 texcoord) { return textureProj(sampler, texcoord); }\n"
    "#endif\n"
};

static const InOutSemantic *FindInOutSemantic(const char *semanticName) {
    for (int i = 0; i < COUNT_OF(inOutSemantics); i++) {
        if (!Str::Cmp(semanticName, inOutSemantics[i].name)) {
            return &inOutSemantics[i];
        }
    }

    return nullptr;
}

static void ParseInOut(Lexer &lexer, bool hasSemantic, InOut &inOut) {
    Str token;
    Str semanticName;

    lexer.ReadToken(&token); // type or precision qualifier

    if (token == "LOWP" || token == "MEDIUMP" || token == "HIGHP") {
        inOut.precision = token;

        lexer.ReadToken(&token); // type
    } else {
        inOut.precision = "";
    }

    inOut.type = token;

    lexer.ReadToken(&token); // name
    inOut.name = token;

    inOut.index = -1;
    inOut.location = -1;

    if (hasSemantic) {
        if (lexer.ExpectPunctuation(P_COLON)) { // :
            if (lexer.ReadToken(&semanticName)) { // semantic name
                const InOutSemantic *inOutSemantic = FindInOutSemantic(semanticName);
                inOut.index = inOutSemantic->index;
                inOut.location = inOutSemantic->location;
            } else {
                lexer.Warning("ParseInOut: missing semantic for inOut %s", inOut.name.c_str());
            }
        }
    }
}

static void ParseUniform(Lexer &lexer, Uniform &uniform) {
    Str token;

    lexer.ReadToken(&token); // type or precision qualifier

    if (token == "LOWP" || token == "MEDIUMP" || token == "HIGHP") {
        uniform.precision = token;

        lexer.ReadToken(&token); // type
    } else {
        uniform.precision = "";
    }

    uniform.type = token;

    lexer.ReadToken(&token); // name
    uniform.name = token;

    while (lexer.ReadToken(&token)) {
        if (token == ";") {
            break;
        }
        uniform.name += token;
    }

    lexer.UnreadToken(&token);
}

static Str PreprocessShaderText(const char *shaderName, bool isVertexShader, const Str &sourceText, Array<InOut> &inOutList) {
    Str processedText;
    processedText.ReAllocate(sourceText.Length() * 2, false);

    Lexer lexer(sourceText.c_str(), sourceText.Length(), shaderName);

    char newline[128] = { "\n" };
    int parentheses = 0;
    Str token;
    InOut inOut;
    Uniform uniform;

    while (lexer.ReadToken(&token, true)) {
        if (token.IsEmpty()) {
            break;
        }

        // maintain indentation
        if (token == "{") {
            processedText += (lexer.LinesCrossed() > 0) ? newline : (lexer.WhiteSpaceBeforeToken() > 0 ? " " : "");
            processedText += "{";

            int len = Min(Str::Length(newline) + 1, (int)sizeof(newline) - 1);
            newline[len - 1] = '\t';
            newline[len - 0] = '\0';
            continue;
        }

        if (token == "}") {
            int len = Max(Str::Length(newline) - 1, 0);
            newline[len] = '\0';

            processedText += (lexer.LinesCrossed() > 0) ? newline : (lexer.WhiteSpaceBeforeToken() > 0 ? " " : "");
            processedText += "}";
            continue;
        }

        if (token == "(") {
            parentheses++;

            processedText += (lexer.LinesCrossed() > 0) ? newline : (lexer.WhiteSpaceBeforeToken() > 0 ? " " : "");
            processedText += "(";
            continue;
        }

        if (token == ")") {
            parentheses--;

            processedText += (lexer.LinesCrossed() > 0) ? newline : (lexer.WhiteSpaceBeforeToken() > 0 ? " " : "");
            processedText += ")";
            continue;
        }

        if (token == "in" && parentheses == 0) {
            processedText += (lexer.LinesCrossed() > 0) ? newline : (lexer.WhiteSpaceBeforeToken() > 0 ? " " : "");

            ParseInOut(lexer, isVertexShader, inOut);
            lexer.SkipUntilString(";");

            if (isVertexShader) {
                inOutList.Append(inOut);

                if (OpenGL::GLSL_VERSION < 130) {
                    processedText += "attribute ";
                } else if (OpenGL::GLSL_VERSION <= 150) {
                    processedText += "in ";
                } else {
                    processedText += va("layout (location = %i) in ", inOut.location);
                }
            } else {
                if (OpenGL::GLSL_VERSION < 130) {
                    processedText += "varying ";
                } else {
                    processedText += "in ";
                }
            }

            if (!inOut.precision.IsEmpty()) {
                processedText += inOut.precision + " ";
            }
            processedText += inOut.type + " " + inOut.name + ";";
            continue;
        }
        
        if (token == "out" && parentheses == 0) {
            processedText += (lexer.LinesCrossed() > 0) ? newline : (lexer.WhiteSpaceBeforeToken() > 0 ? " " : "");

            ParseInOut(lexer, !isVertexShader, inOut);
            lexer.SkipUntilString(";");

            if (isVertexShader) {
                if (OpenGL::GLSL_VERSION < 130) {
                    processedText += "varying ";
                } else {
                    processedText += "out ";
                }
            } else {
                inOutList.Append(inOut);

                if (OpenGL::GLSL_VERSION < 130) {
                } else {
                    processedText += va("layout (location = %i) out ", inOut.location);
                }
            }

            if (!inOut.precision.IsEmpty()) {
                processedText += inOut.precision + " ";
            }
            processedText += inOut.type + " " + inOut.name + ";";
            continue;
        }

        if (token == "uniform" && parentheses == 0) {
            processedText += (lexer.LinesCrossed() > 0) ? newline : (lexer.WhiteSpaceBeforeToken() > 0 ? " " : "");

            ParseUniform(lexer, uniform);
            lexer.SkipUntilString(";");

            processedText += "uniform ";
                
            if (!uniform.precision.IsEmpty()) {
                processedText += uniform.precision + " ";
            }
            processedText += uniform.type + " " + uniform.name + ";";
            continue;
        }

        if (token == "UNIFORM_BLOCK" && parentheses == 0) {
            processedText += (lexer.LinesCrossed() > 0) ? newline : (lexer.WhiteSpaceBeforeToken() > 0 ? " " : "");

            Str blockName;
            lexer.ReadToken(&blockName);
            lexer.ExpectPunctuation(P_BRACEOPEN);

            processedText += "layout (std140) uniform " + blockName + " {\n";

            while (1) {
                lexer.ReadToken(&token);

                if (token.IsEmpty()) {
                    BE_WARNLOG(L"no matching '}' found\n");
                    break;
                } else if (token[0] == '}') {
                    break;
                } else {
                    lexer.UnreadToken(&token);
                    ParseUniform(lexer, uniform);
                    lexer.SkipUntilString(";");

                    if (!uniform.precision.IsEmpty()) {
                        processedText += uniform.precision + " ";
                    }
                    processedText += uniform.type + " " + uniform.name + ";\n";
                }
            }

            processedText += "}";
            continue;
        }

        if (!isVertexShader) {
            for (int i = 0; i < inOutList.Count(); i++) {
                const InOut &fsOut = inOutList[i];

                if (token == fsOut.name && fsOut.index != -1) {
                    if (fsOutBuiltInVars[fsOut.index].deprecatedVersion > OpenGL::GLSL_VERSION) {
                        token = fsOutBuiltInVars[fsOut.index].name;
                        break;
                    }
                }
            }
        }
    
        processedText += (lexer.LinesCrossed() > 0) ? newline : (lexer.WhiteSpaceBeforeToken() > 0 ? " " : "");
        processedText += token;
    }

    Str headerText;
    headerText.ReAllocate(2048, false);

    static char versionString[32];
    static char versionDefineString[32];
    static bool first = true;
    
    if (first) {
        Str::snPrintf(versionString, COUNT_OF(versionString), "#version %s\n", OpenGL::GLSL_VERSION_STRING);
        Str::snPrintf(versionDefineString, COUNT_OF(versionDefineString), "#define GLSL_VERSION %d\n", OpenGL::GLSL_VERSION);
        first = false;
    }
    headerText += versionString;
    headerText += versionDefineString;
    
#if defined(GL_EXT_gpu_shader4) || defined(GL_ARB_gpu_shader5)
    if (gglext._GL_ARB_gpu_shader5) {
        headerText += "#define GPU_SHADER 5\n";
        //headerText += "#extension GL_ARB_gpu_shader5 : enable\n";
    } else if (gglext._GL_EXT_gpu_shader4) {
        headerText += "#define GPU_SHADER 4\n";
        headerText += "#extension GL_EXT_gpu_shader4 : enable\n";
        //headerText += "#extension GL_ARB_geometry_shader4 : enable\n";
    } else {
        headerText += "#define GPU_SHADER 3\n";
    }
#endif

#ifdef GL_ARB_explicit_attrib_location
    if (OpenGL::GLSL_VERSION < 330) {
        if (gglext._GL_ARB_explicit_attrib_location) {
            headerText += "#extension GL_ARB_explicit_attrib_location : enable\n";
        }
    }
#endif

#ifdef GL_ARB_shader_texture_lod
    if (gglext._GL_ARB_shader_texture_lod) {
        headerText += "#extension GL_ARB_shader_texture_lod : enable\n";
    }
#endif

#ifdef GL_EXT_shader_texture_lod
    if (gglext._GL_EXT_shader_texture_lod) {
        //headerText += "#extension GL_EXT_shader_texture_lod : enable\n";
    }
#endif
    
    headerText += "#ifdef GL_ES\n";

    headerText += "#define LOWP lowp\n";
    headerText += "#define MEDIUMP mediump\n";
    headerText += "#define HIGHP highp\n";

    // Set default precisions
    if (isVertexShader) {
        headerText += "precision highp float;\n";
        headerText += "precision highp int;\n";
    } else {
        headerText += "precision highp float;\n";
        headerText += "precision highp int;\n";
        headerText += "precision lowp sampler2D;\n";
        headerText += "#ifdef TEXTURE_RECT\n";
        headerText += "precision lowp sampler2DRect;\n";
        headerText += "#endif\n";
        headerText += "precision lowp sampler3D;\n";
        headerText += "precision lowp samplerCube;\n";
        headerText += "precision lowp sampler2DArray;\n";
        headerText += "precision highp sampler2DShadow;\n";
        headerText += "precision highp samplerCubeShadow;\n";
        headerText += "precision highp sampler2DArrayShadow;\n";
    }

    headerText += "#else\n";

    headerText += "#define LOWP\n";
    headerText += "#define MEDIUMP\n";
    headerText += "#define HIGHP\n";
    
    headerText += "#endif\n";

    Str outputText;

    if (isVertexShader) {
        outputText.ReAllocate(headerText.Length() + Str::Length(vsInsert) + processedText.Length(), false);
        outputText += headerText;
        outputText += vsInsert;
    } else {
        outputText.ReAllocate(headerText.Length() + Str::Length(fsInsert) + processedText.Length(), false);
        outputText += headerText;
        outputText += fsInsert;
    }

    outputText += processedText;

    return outputText;
}

static void TextToLineList(const char *text, Array<Str> &lines) {
    const char separator = '\n';

    Str source(text);
    int ofs = 0;

    for (int lineIndex = 0; ofs < source.Length(); lineIndex++) {
        int start = ofs;
        int last = source.Find(separator, ofs);
        int n = last >= 0 ? last - start : source.Length() - start;

        lines.Append(source.Mid(start, n));

        ofs = start + n + 1;
    }
}

static int CompareSampler(const void *s0, const void *s1) {
    return strcmp(((GLSampler *)s0)->name, ((GLSampler *)s1)->name);
}

static int CompareUniform(const void *s0, const void *s1) {
    return strcmp(((GLUniform *)s0)->name, ((GLUniform *)s1)->name);
}

static int CompareUniformBlock(const void *s0, const void *s1) {
    return strcmp(((GLUniformBlock *)s0)->name, ((GLUniformBlock *)s1)->name);
}

static bool IsValidSamplerType(GLenum type) {
    switch (type) {
    case GL_SAMPLER_2D:
    case GL_SAMPLER_CUBE:
    case GL_SAMPLER_3D:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_CUBE_SHADOW:
    case GL_SAMPLER_2D_ARRAY:
    case GL_SAMPLER_2D_ARRAY_SHADOW:
        return true;
    }
    
    return false;
}

static bool VerifyCompiledShader(GLuint shaderObject, const char *shaderName, const Str &shaderText) {
    GLint infoLogLength;
    GLint status;

    gglGetShaderiv(shaderObject, GL_COMPILE_STATUS, &status);
    
    if (!status) {
        gglGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &infoLogLength);

        if (infoLogLength > 0) {
            char *infoLog = (char *)Mem_Alloc(infoLogLength);
            gglGetShaderInfoLog(shaderObject, infoLogLength, &infoLogLength, infoLog);

            BE_WARNLOG(L"SHADER COMPILE ERROR : '%hs'\n%hs\n", shaderName, infoLog);

            Array<Str> lines;
            TextToLineList(shaderText, lines);
            BE_LOG(L"-----------------\n");
            for (int i = 0; i < lines.Count(); i++) {
                BE_LOG(L"%3d: %hs\n", i + 1, lines[i].c_str());
            }
            BE_LOG(L"-----------------\n");

            Mem_Free(infoLog);
        }
        return false;
    }
    return true;
}

static bool VerifyLinkedProgram(GLuint programObject, const char *shaderName) {
    GLint infoLogLength;
    GLint status;

    gglGetProgramiv(programObject, GL_LINK_STATUS, &status);

    if (!status) {
        gglGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLogLength);

        if (infoLogLength > 0) {
            char *infoLog = (char *)Mem_Alloc(infoLogLength);
            gglGetProgramInfoLog(programObject, infoLogLength, nullptr, infoLog);

            BE_WARNLOG(L"SHADER LINK ERROR : '%hs'\n%hs\n", shaderName, infoLog);
            Mem_Free(infoLog);
        }
        return false;
    }
    return true;
}

static bool CompileAndLinkProgram(const char *name, const char *vsText, const Array<InOut> &vsInArray, const char *fsText, const Array<InOut> &fsOutArray, GLuint programObject) {
    GLuint vs;
    GLuint fs;

    if ((!vsText || !vsText[0]) && (!fsText || !fsText[0])) {
        return false;
    }

    if (vsText && vsText[0]) {
        const GLcharARB *shaderStrings[1] = { vsText };

        vs = gglCreateShader(GL_VERTEX_SHADER);
        gglShaderSource(vs, 1, shaderStrings, nullptr);
        gglCompileShader(vs);

        if (!VerifyCompiledShader(vs, name, vsText)) {
            gglDeleteShader(vs);
            return false;
        }
    }

    if (fsText && fsText[0]) {
        const GLcharARB *shaderStrings[1] = { fsText };

        fs = gglCreateShader(GL_FRAGMENT_SHADER);
        gglShaderSource(fs, 1, shaderStrings, nullptr);
        gglCompileShader(fs);

        if (!VerifyCompiledShader(fs, name, fsText)) {
            gglDeleteShader(fs);
            gglDeleteShader(vs);
            return false;
        }
    }

    if (vsText && vsText[0]) {
        gglAttachShader(programObject, vs);

        if (OpenGL::GLSL_VERSION <= 150) {
            for (int i = 0; i < vsInArray.Count(); i++) {
                const InOut &vsIn = vsInArray[i];
                gglBindAttribLocation(programObject, vsIn.location, vsIn.name.c_str());
            }
        }
    }

    if (fsText && fsText[0]) {
        gglAttachShader(programObject, fs);

#ifdef GL_VERSION_3_0
        if (OpenGL::GLSL_VERSION >= 130 && OpenGL::GLSL_VERSION <= 150) {
            for (int i = 0; i < fsOutArray.Count(); i++) {
                const InOut &fsOut = fsOutArray[i];
                gglBindFragDataLocation(programObject, fsOut.location, fsOut.name.c_str());
            }
        }
#endif
    }

    gglLinkProgram(programObject);

    // delete shader objects after linking GLSL program to save memory
    if (vsText && vsText[0]) {
        gglDeleteShader(vs);
    }
    if (fsText && fsText[0]) {
        gglDeleteShader(fs);
    }

#ifdef _DEBUG
    if (!VerifyLinkedProgram(programObject, name)) {
        return false;
    }
#endif
    return true;
}

static void CacheProgram(const char *name, const uint32_t hash, GLuint programObject) {
    GLint binaryLength;
    gglGetProgramiv(programObject, GL_PROGRAM_BINARY_LENGTH, &binaryLength);

    if (binaryLength > 0) {
        byte *programBinary = (byte *)Mem_Alloc16(binaryLength + sizeof(uint32_t) + sizeof(GLenum));
        
        *(uint32_t *)programBinary = hash;
        gglGetProgramBinary(programObject, binaryLength, nullptr, (GLenum *)(programBinary + sizeof(uint32_t)), programBinary + sizeof(uint32_t) + sizeof(GLenum));        

        Str filename = GLShader::programCacheDir;
        filename.AppendPath(name);
        filename.SetFileExtension(".programbin");
        PlatformFile *file = PlatformFile::OpenFileWrite(filename);
        if (file) {
            file->Write(programBinary, binaryLength + sizeof(uint32_t) + sizeof(GLenum));
            delete file;
        }

        Mem_AlignedFree(programBinary);
    }
}

static bool UseCachedProgram(const char *name, const uint32_t hash, GLuint programObject) {
    Str filename = GLShader::programCacheDir;
    filename.AppendPath(name);
    filename.SetFileExtension(".programbin");
    PlatformFile *file = PlatformFile::OpenFileRead(filename);
    if (!file) {
        return false;
    }

    int fileSize = file->Size();
    byte *programBinary = (byte *)Mem_Alloc16(fileSize);
    file->Read(programBinary, fileSize);
    delete file;

    if (*(uint32_t *)programBinary != hash) {
        Mem_AlignedFree(programBinary);
        return false;
    }

    gglProgramBinary(programObject, *(GLenum *)(programBinary + sizeof(uint32_t)), programBinary + sizeof(uint32_t) + sizeof(GLenum), fileSize - (sizeof(uint32_t) + sizeof(GLenum)));
    
    Mem_AlignedFree(programBinary);

    GLint status;
    gglGetProgramiv(programObject, GL_LINK_STATUS, &status);
    if (!status) {
        return false;
    }

    return true;
}

RHI::Handle OpenGLRHI::CreateShader(const char *name, const char *vsText, const char *fsText) {
    GLuint programObject = gglCreateProgram();
    uint32_t programHash = 0;
    Array<InOut> vsInArray(32);
    Array<InOut> fsOutArray(32);

    Str vsppText = PreprocessShaderText(name, true, vsText, vsInArray);
    Str fsppText = PreprocessShaderText(name, false, fsText, fsOutArray);

    bool shouldCompileProgram = true;

    if (OpenGL::SupportsProgramBinary()) {
        const uint32_t vsHash = MD5_BlockChecksum(vsppText.c_str(), vsppText.Length());
        const uint32_t fsHash = MD5_BlockChecksum(fsppText.c_str(), fsppText.Length());

        programHash = vsHash ^ fsHash;

        shouldCompileProgram = !UseCachedProgram(name, programHash, programObject);
    }

    if (shouldCompileProgram) {
        if (!CompileAndLinkProgram(name, vsppText, vsInArray, fsppText, fsOutArray, programObject)) {
            gglDeleteProgram(programObject);
            return NullShader;
        }

        if (OpenGL::SupportsProgramBinary()) {
            CacheProgram(name, programHash, programObject);
        }
    } 

    GLint uniformCount, uniformMaxNameLength;
    gglGetProgramiv(programObject, GL_ACTIVE_UNIFORMS, &uniformCount);
    gglGetProgramiv(programObject, GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniformMaxNameLength); // maximum length of the uniform name

    GLint uniformBlockCount, uniformBlockMaxNameLength;
    gglGetProgramiv(programObject, GL_ACTIVE_UNIFORM_BLOCKS, &uniformBlockCount);
    gglGetProgramiv(programObject, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, &uniformBlockMaxNameLength);
    
    char *uniformName = (char *)_alloca(uniformMaxNameLength);
    char *uniformBlockName = (char *)_alloca(uniformBlockMaxNameLength);

    static const int MAX_SAMPLERS = 64;
    static const int MAX_UNIFORMS = 4096;
    static const int MAX_UNIFORM_BLOCKS = 16;

    GLSampler *tempSamplers = (GLSampler *)_alloca(sizeof(GLSampler) * MAX_SAMPLERS);
    GLUniform *tempUniforms = (GLUniform *)_alloca(sizeof(GLUniform) * MAX_UNIFORMS);
    GLUniformBlock *tempUniformBlocks = (GLUniformBlock *)_alloca(sizeof(GLUniformBlock) * MAX_UNIFORM_BLOCKS);

    int numSamplers = 0;
    int numUniforms = 0;
    int numUniformBlocks = 0;
    
    gglUseProgram(programObject);

    for (int uniformIndex = 0; uniformIndex < uniformCount; uniformIndex++) {
        GLenum type;
        GLint size;

        // A uniform variable (either built-in or user-defined) is considered active if it is determined during 
        // the link operation that it may be accessed during program execution. Therefore, program should have 
        // previously been the target of a call to glLinkProgram
        //
        // If one or more elements of an array are active, the name of the array is returned in name, the type is 
        // returned in type, and the size parameter returns the highest array element index used, plus one, 
        // as determined by the compiler and/or linker
        // http://www.khronos.org/opengles/sdk/docs/man/xhtml/glGetActiveUniform.xml
        gglGetActiveUniform(programObject, uniformIndex, uniformMaxNameLength, nullptr, &size, &type, uniformName);
        // NOTE: size 는 highest array element index used 가 아닌 것 같다. 그냥 max array count 가 나옴

        if (IsValidSamplerType(type)) {
            if (size > 1) {
                char *bracket = strchr(uniformName, '[');
                if (bracket) {
                    for (int arrayIndex = 0; arrayIndex < size; arrayIndex++) {
                        bracket[1] = '0' + arrayIndex;

                        GLint location = gglGetUniformLocation(programObject, uniformName);
                        gglUniform1i(location, numSamplers);

                        tempSamplers[numSamplers].name = Mem_AllocString(uniformName);
                        tempSamplers[numSamplers].unit = numSamplers; // TMU

                        numSamplers++;
                    }
                }
            } else {
                GLint location = gglGetUniformLocation(programObject, uniformName);
                gglUniform1i(location, numSamplers);

                tempSamplers[numSamplers].name = Mem_AllocString(uniformName);
                tempSamplers[numSamplers].unit = numSamplers; // TMU

                numSamplers++;
            }
        } else {
            if (Str::Cmpn(uniformName, "gl_", 3)) { // built-in uniform 은 제외한다
                char *bracket = strchr(uniformName, '[');
                if (bracket == nullptr || (bracket[1] == '0' && bracket[2] == ']')) {
                    if (bracket) {
                        *bracket = '\0';
                    }

                    tempUniforms[numUniforms].name = Mem_AllocString(uniformName);
                    tempUniforms[numUniforms].location = gglGetUniformLocation(programObject, uniformName);
                    tempUniforms[numUniforms].type = type;
                    tempUniforms[numUniforms].count = size;

                    numUniforms++;
                }
            }
        }
    }

    for (int uniformBlockIndex = 0; uniformBlockIndex < uniformBlockCount; uniformBlockIndex++) {
        GLint params;

        gglGetActiveUniformBlockName(programObject, uniformBlockIndex, uniformBlockMaxNameLength, nullptr, uniformBlockName);

        tempUniformBlocks[numUniformBlocks].name = Mem_AllocString(uniformBlockName);
        tempUniformBlocks[numUniformBlocks].index = gglGetUniformBlockIndex(programObject, uniformBlockName);

        gglGetActiveUniformBlockiv(programObject, uniformBlockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &params);
        tempUniformBlocks[numUniformBlocks].size = params;

        gglGetActiveUniformBlockiv(programObject, uniformBlockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &params);
        tempUniformBlocks[numUniformBlocks].numUniforms = params; // occupies 16 bytes per uniform

        gglGetActiveUniformBlockiv(programObject, uniformBlockIndex, GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER, &params);
        tempUniformBlocks[numUniformBlocks].referencedByVS = params > 0;

        gglGetActiveUniformBlockiv(programObject, uniformBlockIndex, GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER, &params);
        tempUniformBlocks[numUniformBlocks].referencedByFS = params > 0;

        numUniformBlocks++;
    }
    
    gglUseProgram(0);

    GLSampler *samplers = nullptr;
    GLUniform *uniforms = nullptr;
    GLUniformBlock *uniformBlocks = nullptr;

    if (numSamplers > 0) {
        samplers = (GLSampler *)Mem_Alloc(sizeof(GLSampler) * numSamplers);
        simdProcessor->Memcpy(samplers, tempSamplers, sizeof(GLSampler) * numSamplers);

        // binary search 를 위해 정렬
        qsort(samplers, numSamplers, sizeof(samplers[0]), CompareSampler);
    }

    if (numUniforms > 0) {
        uniforms = (GLUniform *)Mem_Alloc(sizeof(GLUniform) * numUniforms);
        simdProcessor->Memcpy(uniforms, tempUniforms, sizeof(GLUniform) * numUniforms);

        // binary search 를 위해 정렬
        qsort(uniforms, numUniforms, sizeof(uniforms[0]), CompareUniform);
    }

    if (numUniformBlocks > 0) {
        uniformBlocks = (GLUniformBlock *)Mem_Alloc(sizeof(GLUniformBlock) * numUniformBlocks);
        simdProcessor->Memcpy(uniformBlocks, tempUniformBlocks, sizeof(GLUniform) * numUniformBlocks);

        // binary search 를 위해 정렬
        qsort(uniformBlocks, numUniformBlocks, sizeof(uniformBlocks[0]), CompareUniformBlock);
    }

    GLShader *shader = new GLShader;
    Str::Copynz(shader->name, name, COUNT_OF(shader->name));
    shader->programObject       = programObject;
    shader->numSamplers         = numSamplers;
    shader->samplers            = samplers;
    shader->numUniforms         = numUniforms;
    shader->uniforms            = uniforms;
    shader->numUniformBlocks    = numUniformBlocks;
    shader->uniformBlocks       = uniformBlocks;

    int handle = shaderList.FindNull();
    if (handle == -1) {
        handle = shaderList.Append(shader);
    } else {
        shaderList[handle] = shader;
    }

    return (Handle)handle;
}

void OpenGLRHI::DestroyShader(Handle shaderHandle) {
    if (currentContext->state->shaderHandle == shaderHandle) {
        BindShader(NullShader);
    }

    GLShader *shader = shaderList[shaderHandle];
    gglDeleteProgram(shader->programObject);
    for (int i = 0; i < shader->numSamplers; i++) {
        Mem_Free(shader->samplers[i].name);
    }
    for (int i = 0; i < shader->numUniforms; i++) {
        Mem_Free(shader->uniforms[i].name);
    }
    Mem_Free(shader->samplers);
    Mem_Free(shader->uniforms);

    delete shader;
    shaderList[shaderHandle] = nullptr;
}

void OpenGLRHI::BindShader(Handle shaderHandle) {
    if (currentContext->state->shaderHandle == shaderHandle) {
        return;
    }

    const GLShader *shader = shaderList[shaderHandle];
    gglUseProgram(shader->programObject);

    currentContext->state->shaderHandle = shaderHandle;
}

int OpenGLRHI::GetSamplerUnit(Handle shaderHandle, const char *name) const {
    const GLShader *shader = shaderList[shaderHandle];
    GLSampler find;
    find.name = const_cast<char *>(name);
    int index = BinSearch_Equal<GLSampler>(shader->samplers, shader->numSamplers, find);
    if (index >= 0) {
        return shader->samplers[index].unit;
    }
    return -1;
}

void OpenGLRHI::SetTexture(int unit, Handle textureHandle) {
    SelectTextureUnit(unit);
    BindTexture(textureHandle);
}

int OpenGLRHI::GetShaderConstantIndex(int shaderHandle, const char *name) const {
    const GLShader *shader = shaderList[shaderHandle];
    GLUniform find;
    find.name = const_cast<char *>(name);
    int index = BinSearch_Equal<GLUniform>(shader->uniforms, shader->numUniforms, find);
    if (index < 0) {
        return -1;
    }

    return index;
}

int OpenGLRHI::GetShaderConstantBlockIndex(int shaderHandle, const char *name) const {
    const GLShader *shader = shaderList[shaderHandle];
    GLUniformBlock find;
    find.name = const_cast<char *>(name);
    int index = BinSearch_Equal<GLUniformBlock>(shader->uniformBlocks, shader->numUniformBlocks, find);
    if (index < 0) {
        return -1;
    }

    return index;
}

void OpenGLRHI::SetShaderConstantGeneric(int index, bool rowMajor, int count, const void *data) const {
    if (index < 0) {
        return;
    }

    const GLShader *shader = shaderList[currentContext->state->shaderHandle];
    const GLUniform *uniform = &shader->uniforms[index];
    switch (uniform->type) {
    case GL_FLOAT:
        gglUniform1fv(uniform->location, count, (const GLfloat *)data);
        break;
    case GL_FLOAT_VEC2:
        gglUniform2fv(uniform->location, count, (const GLfloat *)data);
        break;
    case GL_FLOAT_VEC3:
        gglUniform3fv(uniform->location, count, (const GLfloat *)data);
        break;
    case GL_FLOAT_VEC4:
        gglUniform4fv(uniform->location, count, (const GLfloat *)data);
        break;
    case GL_INT:
    case GL_UNSIGNED_INT:
    case GL_BOOL:
        gglUniform1iv(uniform->location, count, (const GLint *)data);
        break;
    case GL_INT_VEC2:
    case GL_UNSIGNED_INT_VEC2:
    case GL_BOOL_VEC2:
        gglUniform2iv(uniform->location, count, (const GLint *)data);
        break;
    case GL_INT_VEC3:
    case GL_UNSIGNED_INT_VEC3:
    case GL_BOOL_VEC3:
        gglUniform3iv(uniform->location, count, (const GLint *)data);
        break;
    case GL_INT_VEC4:
    case GL_UNSIGNED_INT_VEC4:
    case GL_BOOL_VEC4:
        gglUniform4iv(uniform->location, count, (const GLint *)data);
        break;
    case GL_FLOAT_MAT2:
        gglUniformMatrix2fv(uniform->location, count, rowMajor, (const GLfloat *)data);
        break;
    case GL_FLOAT_MAT3:
        gglUniformMatrix3fv(uniform->location, count, rowMajor, (const GLfloat *)data);
        break;
    case GL_FLOAT_MAT4:
        gglUniformMatrix4fv(uniform->location, count, rowMajor, (const GLfloat *)data);
        break;
    case GL_FLOAT_MAT4x3: // columns = 4, rows = 3
        gglUniformMatrix4x3fv(uniform->location, count, rowMajor, (const GLfloat *)data);
        break;
    }
}

void OpenGLRHI::SetShaderConstant1i(int index, const int constant) const {
    SetShaderConstantGeneric(index, false, 1, &constant);
}

void OpenGLRHI::SetShaderConstant2i(int index, const int *constant) const {
    SetShaderConstantGeneric(index, false, 1, constant);
}

void OpenGLRHI::SetShaderConstant3i(int index, const int *constant) const {
    SetShaderConstantGeneric(index, false, 1, constant);
}

void OpenGLRHI::SetShaderConstant4i(int index, const int *constant) const {
    SetShaderConstantGeneric(index, false, 1, constant);
}

void OpenGLRHI::SetShaderConstant1f(int index, const float constant) const {
    SetShaderConstantGeneric(index, false, 1, &constant);
}

void OpenGLRHI::SetShaderConstant2f(int index, const float *constant) const {
    SetShaderConstantGeneric(index, false, 1, constant);
}

void OpenGLRHI::SetShaderConstant3f(int index, const float *constant) const {
    SetShaderConstantGeneric(index, false, 1, constant);
}

void OpenGLRHI::SetShaderConstant4f(int index, const float *constant) const {
    SetShaderConstantGeneric(index, false, 1, constant);
}

void OpenGLRHI::SetShaderConstant2f(int index, const Vec2 &constant) const {
    SetShaderConstantGeneric(index, false, 1, &constant);
}

void OpenGLRHI::SetShaderConstant3f(int index, const Vec3 &constant) const {
    SetShaderConstantGeneric(index, false, 1, &constant);
}

void OpenGLRHI::SetShaderConstant4f(int index, const Vec4 &constant) const {
    SetShaderConstantGeneric(index, false, 1, &constant);
}

void OpenGLRHI::SetShaderConstant2x2f(int index, bool rowMajor, const Mat2 &constant) const {
    SetShaderConstantGeneric(index, rowMajor, 1, &constant);
}

void OpenGLRHI::SetShaderConstant3x3f(int index, bool rowMajor, const Mat3 &constant) const {
    SetShaderConstantGeneric(index, rowMajor, 1, &constant);
}

void OpenGLRHI::SetShaderConstant4x4f(int index, bool rowMajor, const Mat4 &constant) const {
    SetShaderConstantGeneric(index, rowMajor, 1, &constant);
}

void OpenGLRHI::SetShaderConstant4x3f(int index, bool rowMajor, const Mat3x4 &constant) const {
    SetShaderConstantGeneric(index, rowMajor, 1, &constant);
}

void OpenGLRHI::SetShaderConstantArray1i(int index, int count, const int *constant) const {
    SetShaderConstantGeneric(index, false, count, constant);
}

void OpenGLRHI::SetShaderConstantArray2i(int index, int count, const int *constant) const {
    SetShaderConstantGeneric(index, false, count, constant);
}

void OpenGLRHI::SetShaderConstantArray3i(int index, int count, const int *constant) const {
    SetShaderConstantGeneric(index, false, count, constant);
}

void OpenGLRHI::SetShaderConstantArray4i(int index, int count, const int *constant) const {
    SetShaderConstantGeneric(index, false, count, constant);
}

void OpenGLRHI::SetShaderConstantArray1f(int index, int count, const float *constant) const {
    SetShaderConstantGeneric(index, false, count, constant);
}

void OpenGLRHI::SetShaderConstantArray2f(int index, int count, const float *constant) const {
    SetShaderConstantGeneric(index, false, count, constant);
}

void OpenGLRHI::SetShaderConstantArray3f(int index, int count, const float *constant) const {
    SetShaderConstantGeneric(index, false, count, constant);
}

void OpenGLRHI::SetShaderConstantArray4f(int index, int count, const float *constant) const {
    SetShaderConstantGeneric(index, false, count, constant);
}

void OpenGLRHI::SetShaderConstantArray2f(int index, int count, const Vec2 *constant) const {
    SetShaderConstantGeneric(index, false, count, constant);
}

void OpenGLRHI::SetShaderConstantArray3f(int index, int count, const Vec3 *constant) const {
    SetShaderConstantGeneric(index, false, count, constant);
}

void OpenGLRHI::SetShaderConstantArray4f(int index, int count, const Vec4 *constant) const {
    SetShaderConstantGeneric(index, false, count, constant);
}

void OpenGLRHI::SetShaderConstantArray2x2f(int index, bool rowMajor, int count, const Mat2 *constant) const {
    SetShaderConstantGeneric(index, rowMajor, count, constant);
}

void OpenGLRHI::SetShaderConstantArray3x3f(int index, bool rowMajor, int count, const Mat3 *constant) const {
    SetShaderConstantGeneric(index, rowMajor, count, constant);
}

void OpenGLRHI::SetShaderConstantArray4x4f(int index, bool rowMajor, int count, const Mat4 *constant) const {
    SetShaderConstantGeneric(index, rowMajor, count, constant);
}

void OpenGLRHI::SetShaderConstantArray4x3f(int index, bool rowMajor, int count, const Mat3x4 *constant) const {
    SetShaderConstantGeneric(index, rowMajor, count, constant);
}

void OpenGLRHI::SetShaderConstantBlock(int index, int bindingIndex) {
    const GLShader *shader = shaderList[currentContext->state->shaderHandle];
    const GLUniformBlock *uniformBlock = &shader->uniformBlocks[index];

    gglUniformBlockBinding(shader->programObject, uniformBlock->index, bindingIndex);
}

BE_NAMESPACE_END
