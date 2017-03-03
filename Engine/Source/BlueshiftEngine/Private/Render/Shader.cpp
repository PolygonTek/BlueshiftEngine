#include "Precompiled.h"
#include "Render/Render.h"
#include "RenderInternal.h"
#include "Core/Lexer.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

static const char *directiveInclude = "$include";

// NOTE: BuiltInConstant enum 과 반드시 순서가 같아야 함
// NOTE2: 나중에 자동으로 모든 shader 에 추가되는 구조로 바꾸면 편할 듯
static const char *builtInConstantNames[] = {
    "modelViewMatrix",
    "modelViewMatrixTranspose",
    "projectionMatrix",
    "projectionMatrixTranspose",
    "modelViewProjectionMatrix",
    "modelViewProjectionMatrixTranspose",
    "worldMatrixS",
    "worldMatrixT",
    "worldMatrixR",
    "textureMatrixS",
    "textureMatrixT",
    "constantColor",
    "vertexColorScale",
    "vertexColorAdd",
    "localViewOrigin",
    "localLightOrigin",
    "localLightAxis"
};

// NOTE: BuiltInSampler enum 과 반드시 순서가 같아야 함
static const char *builtInSamplerNames[] = {
    "diffuseMap",
    "specularMap",
    "bumpMap"
};

const StrHashMap<PropertySpec> &Shader::GetSpecHashMap() const { 
    if (originalShader) {
        return originalShader->specHashMap; 
    }
    return specHashMap; 
}

void Shader::Purge() {
    if (shaderHandle != Renderer::NullShader) {
        glr.DeleteShader(shaderHandle);
        shaderHandle = Renderer::NullShader;
    }

    if (perforatedVersion) {
        shaderManager.ReleaseShader(perforatedVersion);
        perforatedVersion = nullptr;
    }

    if (ambientLitVersion) {
        shaderManager.ReleaseShader(ambientLitVersion);
        ambientLitVersion = nullptr;
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

    if (originalShader) {
        originalShader->instantiatedShaders.Remove(this);
        originalShader = nullptr;
    }

    defineArray.Clear();
    specHashMap.Clear();
}

bool Shader::Create(const char *text, const char *baseDir) {
    bool generatePerforatedVersion = false;
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
        } else if (!token.Icmp("lighting")) {
            flags |= Lighting;
        } else if (!token.Icmp("properties")) {
            ParseProperties(lexer);
        } else if (!token.Icmp("perforatedVersion")) {
            if (lexer.ReadToken(&token)) {
                Str path = baseDir;
                path.AppendPath(token, '/');

                perforatedVersion = shaderManager.GetShader(path);
            } else {
                BE_WARNLOG(L"missing perforatedVersion name in shader '%hs'\n", hashName.c_str());
            }
        } else if (!token.Icmp("ambientLitVersion")) {
            if (lexer.ReadToken(&token)) {
                Str path = baseDir;
                path.AppendPath(token, '/');

                ambientLitVersion = shaderManager.GetShader(path);
            } else {
                BE_WARNLOG(L"missing ambientLitVersion name in shader '%hs'\n", hashName.c_str());
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

    return Finish(generatePerforatedVersion, generateGpuSkinningVersion, generateParallelShadowVersion, generateSpotShadowVersion, generatePointShadowVersion, baseDir);
}

bool Shader::ParseProperties(Lexer &lexer) {
    PropertySpec spec;
    Str token;

    if (!lexer.ExpectPunctuation(P_BRACEOPEN)) {
        return false;
    }

    while (lexer.ReadToken(&token)) {
        if (token[0] == '}') {
            break;
        }
        else {
            if (specHashMap.Get(token)) {
                BE_WARNLOG(L"same property name '%hs' ignored in shader '%hs'\n", token.c_str(), hashName.c_str());
                lexer.SkipRestOfLine();
                continue;
            }

            lexer.UnreadToken(&token);

            if (!spec.ParseSpec(lexer)) {
                BE_WARNLOG(L"error occured in parsing property spec in shader '%hs'\n", hashName.c_str());
                lexer.SkipRestOfLine();
                continue;
            }

            specHashMap.Set(token, spec);
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
        skinningVsHeaderText = "#define GPU_SKINNING\n$include \"skinning_matrix1.glsl\"\n";
        break;
    case 4:
        skinningPostfix = "-skinning4";
        skinningVsHeaderText = "#define GPU_SKINNING\n$include \"skinning_matrix4.glsl\"\n";
        break;
    case 8:
        skinningPostfix = "-skinning8";
        skinningVsHeaderText = "#define GPU_SKINNING\n$include \"skinning_matrix8.glsl\"\n";
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

    shader->specHashMap = specHashMap;

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

bool Shader::Finish(bool generatePerforatedVersion, bool genereateGpuSkinningVersion, bool generateParallelShadowVersion, bool generateSpotShadowVersion, bool generatePointShadowVersion, const char *baseDir) {
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

const char *MangleNameWithDefineList(const Str &basename, const Array<Shader::Define> &defineArray, Str &mangledName) {
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
    shader->defineArray = defineArray;
    shader->baseDir = baseDir;

    instantiatedShaders.Append(shader);
    
    shader->Instantiate(defineArray);

    if (perforatedVersion) {
        shader->perforatedVersion = perforatedVersion->InstantiateShader(defineArray);
    }
    
    if (ambientLitVersion) {
        shader->ambientLitVersion = ambientLitVersion->InstantiateShader(defineArray);
    }

    if (parallelShadowVersion) {
        shader->parallelShadowVersion = parallelShadowVersion->InstantiateShader(defineArray);
    }

    if (spotShadowVersion) {
        shader->spotShadowVersion = spotShadowVersion->InstantiateShader(defineArray);
    }

    if (pointShadowVersion) {
        shader->pointShadowVersion = pointShadowVersion->InstantiateShader(defineArray);
    }

    for (int i = 0; i < COUNT_OF(gpuSkinningVersion); i++) {
        if (gpuSkinningVersion[i]) {
            shader->gpuSkinningVersion[i] = gpuSkinningVersion[i]->InstantiateShader(defineArray);
        }
    }

    return shader;
}

void Shader::Reinstantiate() {
    assert(originalShader);
    Instantiate(defineArray);

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
	
#if defined __ANDROID__ && ! defined __XAMARIN__
	static int progress = 0;

	float f = progress * (M_PI * 0.01f);
	progress++;
	Color4 color;
	color.r = sinf(f) * 0.5f + 0.5f;
	color.g = sinf(f + M_PI * 2.0f / 3.0f)* 0.5f + 0.5f;
	color.b = sinf(f + M_PI * 4.0f / 3.0f)* 0.5f + 0.5f;
	color.a = 1.0f;

	glr.SetStateBits(Renderer::ColorWrite | Renderer::AlphaWrite);
	glr.Clear(Renderer::ColorBit, color, 0, 0);
	glr.SwapBuffers();

	//BE_LOG(L"progress %f %f %f %f %d", color.r, color.g, color.b, f, progress);
#endif

    Str processedVsText;
    Str processedFsText;
    hasVertexShader = ProcessShaderText(originalShader->vsText, originalShader->baseDir, defineArray, processedVsText);
    hasFragmentShader = ProcessShaderText(originalShader->fsText, originalShader->baseDir, defineArray, processedFsText);

    if (!hasVertexShader || !hasFragmentShader) {
        return false;
    }

    if (shaderHandle != Renderer::NullShader) {
        glr.DeleteShader(shaderHandle);
    }

    shaderHandle = glr.CreateShader(hashName, processedVsText, processedFsText);

    assert(MaxBuiltInConstants == COUNT_OF(builtInConstantNames));
    assert(MaxBuiltInSamplers == COUNT_OF(builtInSamplerNames));

    for (int i = 0; i < MaxBuiltInConstants; i++) {
        builtInConstantLocations[i] = glr.GetShaderConstantLocation(shaderHandle, builtInConstantNames[i]);
    }

    for (int i = 0; i < MaxBuiltInSamplers; i++) {
        builtInSamplerUnits[i] = glr.GetSamplerUnit(shaderHandle, builtInSamplerNames[i]);
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
    glr.BindShader(shaderHandle);
}

int Shader::GetConstantLocation(const char *name) const {
    return glr.GetShaderConstantLocation(shaderHandle, name); 
}

void Shader::SetConstant1i(int index, const int constant) const { 
    glr.SetShaderConstant1i(index, constant);
}

void Shader::SetConstant2i(int index, const int *constant) const { 
    glr.SetShaderConstant2i(index, constant);
}

void Shader::SetConstant3i(int index, const int *constant) const { 
    glr.SetShaderConstant3i(index, constant);
}

void Shader::SetConstant4i(int index, const int *constant) const { 
    glr.SetShaderConstant4i(index, constant);
}

void Shader::SetConstant1f(int index, float x) const { 
    glr.SetShaderConstant1f(index, x);
}

void Shader::SetConstant2f(int index, const float *constant) const { 
    glr.SetShaderConstant2f(index, constant);
}

void Shader::SetConstant3f(int index, const float *constant) const { 
    glr.SetShaderConstant3f(index, constant);
}

void Shader::SetConstant4f(int index, const float *constant) const { 
    glr.SetShaderConstant4f(index, constant);
}

void Shader::SetConstant2f(int index, const Vec2 &constant) const { 
    glr.SetShaderConstant2f(index, constant);
}

void Shader::SetConstant3f(int index, const Vec3 &constant) const { 
    glr.SetShaderConstant3f(index, constant);
}

void Shader::SetConstant4f(int index, const Vec4 &constant) const { 
    glr.SetShaderConstant4f(index, constant);
}

void Shader::SetConstant2x2f(int index, bool rowmajor, const Mat2 &constant) const { 
    glr.SetShaderConstant2x2f(index, rowmajor, constant);
}

void Shader::SetConstant3x3f(int index, bool rowmajor, const Mat3 &constant) const { 
    glr.SetShaderConstant3x3f(index, rowmajor, constant);
}

void Shader::SetConstant4x4f(int index, bool rowmajor, const Mat4 &constant) const { 
    glr.SetShaderConstant4x4f(index, rowmajor, constant);
}

void Shader::SetConstantArray1i(int index, int num, const int *constant) const {
    glr.SetShaderConstantArray1i(index, num, constant);
}

void Shader::SetConstantArray2i(int index, int num, const int *constant) const {
    glr.SetShaderConstantArray2i(index, num, constant);
}

void Shader::SetConstantArray3i(int index, int num, const int *constant) const {
    glr.SetShaderConstantArray3i(index, num, constant);
}

void Shader::SetConstantArray4i(int index, int num, const int *constant) const {
    glr.SetShaderConstantArray4i(index, num, constant);
}

void Shader::SetConstantArray1f(int index, int num, const float *constant) const {
    glr.SetShaderConstantArray1f(index, num, constant);
}

void Shader::SetConstantArray2f(int index, int num, const float *constant) const {
    glr.SetShaderConstantArray2f(index, num, constant);
}

void Shader::SetConstantArray3f(int index, int num, const float *constant) const {
    glr.SetShaderConstantArray3f(index, num, constant);
}

void Shader::SetConstantArray4f(int index, int num, const float *constant) const {
    glr.SetShaderConstantArray4f(index, num, constant);
}

void Shader::SetConstantArray2f(int index, int num, const Vec2 *constant) const {
    glr.SetShaderConstantArray2f(index, num, constant);
}

void Shader::SetConstantArray3f(int index, int num, const Vec3 *constant) const {
    glr.SetShaderConstantArray3f(index, num, constant);
}

void Shader::SetConstantArray4f(int index, int num, const Vec4 *constant) const {
    glr.SetShaderConstantArray4f(index, num, constant);
}

void Shader::SetConstantArray2x2f(int index, bool rowmajor, int num, const Mat2 *constant) const { 
    glr.SetShaderConstantArray2x2f(index, rowmajor, num, constant);
}

void Shader::SetConstantArray3x3f(int index, bool rowmajor, int num, const Mat3 *constant) const { 
    glr.SetShaderConstantArray3x3f(index, rowmajor, num, constant);
}

void Shader::SetConstantArray4x4f(int index, bool rowmajor, int num, const Mat4 *constant) const { 
    glr.SetShaderConstantArray4x4f(index, rowmajor, num, constant);
}

void Shader::SetConstant1i(const char *name, int x) const { 
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant1i: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstant1i(index, x);
}

void Shader::SetConstant2i(const char *name, const int *constant) const { 
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant2i: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstant2i(index, constant);
}

void Shader::SetConstant3i(const char *name, const int *constant) const { 
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant3i: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstant3i(index, constant);
}

void Shader::SetConstant4i(const char *name, const int *constant) const { 
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant4i: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstant4i(index, constant);
}

void Shader::SetConstant1f(const char *name, float x) const { 
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant1f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstant1f(index, x);
}

void Shader::SetConstant2f(const char *name, const float *constant) const { 
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant2f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstant2f(index, constant);
}

void Shader::SetConstant3f(const char *name, const float *constant) const { 
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant3f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstant3f(index, constant);
}

void Shader::SetConstant4f(const char *name, const float *constant) const { 
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant4f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstant4f(index, constant);
}

void Shader::SetConstant2f(const char *name, const Vec2 &constant) const { 
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant2f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstant2f(index, constant);
}

void Shader::SetConstant3f(const char *name, const Vec3 &constant) const { 
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant3f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstant3f(index, constant);
}

void Shader::SetConstant4f(const char *name, const Vec4 &constant) const { 
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant4f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstant4f(index, constant);
}

void Shader::SetConstant2x2f(const char *name, bool rowmajor, const Mat2 &constant) const { 
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant2x2f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstant2x2f(index, rowmajor, constant);
}

void Shader::SetConstant3x3f(const char *name, bool rowmajor, const Mat3 &constant) const { 
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant3x3f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstant3x3f(index, rowmajor, constant);
}

void Shader::SetConstant4x4f(const char *name, bool rowmajor, const Mat4 &constant) const { 
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstant4x4f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstant4x4f(index, rowmajor, constant);
}

void Shader::SetConstantArray1i(const char *name, int num, const int *constant) const {
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray1i: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstantArray1i(index, num, constant);
}

void Shader::SetConstantArray2i(const char *name, int num, const int *constant) const {
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray2i: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstantArray2i(index, num, constant);
}

void Shader::SetConstantArray3i(const char *name, int num, const int *constant) const {
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray3i: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstantArray3i(index, num, constant);
}

void Shader::SetConstantArray4i(const char *name, int num, const int *constant) const {
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray4i: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstantArray4i(index, num, constant);
}

void Shader::SetConstantArray1f(const char *name, int num, const float *constant) const {
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray1f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstantArray1f(index, num, constant);
}

void Shader::SetConstantArray2f(const char *name, int num, const float *constant) const {
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray2f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstantArray2f(index, num, constant);
}

void Shader::SetConstantArray3f(const char *name, int num, const float *constant) const {
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray3f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstantArray3f(index, num, constant);
}

void Shader::SetConstantArray4f(const char *name, int num, const float *constant) const {
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray4f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstantArray4f(index, num, constant);
}

void Shader::SetConstantArray2f(const char *name, int num, const Vec2 *constant) const {
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray2f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstantArray2f(index, num, constant);
}

void Shader::SetConstantArray3f(const char *name, int num, const Vec3 *constant) const {
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray3f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstantArray3f(index, num, constant);
}

void Shader::SetConstantArray4f(const char *name, int num, const Vec4 *constant) const {
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray4f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstantArray4f(index, num, constant);
}

void Shader::SetConstantArray2x2f(const char *name, bool rowmajor, int num, const Mat2 *constant) const { 
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray2x2f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstantArray2x2f(index, rowmajor, num, constant);
}

void Shader::SetConstantArray3x3f(const char *name, bool rowmajor, int num, const Mat3 *constant) const { 
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray3x3f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstantArray3x3f(index, rowmajor, num, constant);
}

void Shader::SetConstantArray4x4f(const char *name, bool rowmajor, int num, const Mat4 *constant) const { 
    int index = glr.GetShaderConstantLocation(shaderHandle, name);
    if (index < 0) {
        //BE_WARNLOG(L"Shader::SetConstantArray4x4f: invalid constant name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }
    glr.SetShaderConstantArray4x4f(index, rowmajor, num, constant);
}

int Shader::GetSamplerUnit(const char *name) const {
    return glr.GetSamplerUnit(shaderHandle, name);
}

void Shader::SetTexture(int unit, const Texture *texture) const {
    if (unit < 0) {
        return;
    }

    glr.SetTexture(unit, texture->textureHandle);
}

void Shader::SetTexture(const char *name, const Texture *texture) const {
    int unit = glr.GetSamplerUnit(shaderHandle, name);
    if (unit < 0) {
        //BE_WARNLOG(L"Shader::SetTexture: invalid texture name '%hs' in shader '%hs'\n", name, this->hashName.c_str());
        return;
    }

    glr.SetTexture(unit, texture->textureHandle);
}

void Shader::SetTextureArray(const char *name, int num, const Texture **textures) const {
    char temp[256];

    strcpy(temp, name);
    char *indexPtr = temp + strlen(temp) + 1;
    strcat(temp, "[0]");

    for (int i = 0; i < num; i++) {
        *indexPtr = '0' + i;

        int unit = glr.GetSamplerUnit(shaderHandle, temp);
        if (unit < 0) {
            //BE_WARNLOG(L"Shader::SetTextureArray: invalid texture name '%hs' in shader '%hs'\n", temp, this->hashName.c_str());
            return;
        }

        glr.SetTexture(unit, textures[i]->textureHandle);
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
