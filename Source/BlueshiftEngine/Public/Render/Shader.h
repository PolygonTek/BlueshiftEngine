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

#pragma once

/*
-------------------------------------------------------------------------------

    Shader

-------------------------------------------------------------------------------
*/

#include "Containers/HashMap.h"
#include "Math/Math.h"
#include "Core/Property.h"
#include "Core/CmdArgs.h"
#include "RHI/RHI.h"

BE_NAMESPACE_BEGIN

class Texture;

class Shader {
    friend class ShaderManager;
    friend class RBSurf;

public:
    enum Flag {
        LitSurface              = BIT(0),
        SkySurface              = BIT(1),
        LoadedFromFile          = BIT(8)
    };

    enum Lang {
        GlslShader,
        CgShader
    };

    enum BuiltInConstant {
        ModelViewMatrixConst,
        ModelViewMatrixTransposeConst,
        ProjectionMatrixConst,
        ProjectionMatrixTransposeConst,
        ModelViewProjectionMatrixConst,
        ModelViewProjectionMatrixTransposeConst,
        WorldMatrixSConst,
        WorldMatrixTConst,
        WorldMatrixRConst,
        TextureMatrixSConst,
        TextureMatrixTConst,
        ConstantColorConst,
        VertexColorScaleConst,
        VertexColorAddConst,
        LocalViewOriginConst,
        LocalLightOriginConst,
        LocalLightAxisConst,
        MaxBuiltInConstants
    };

    enum BuiltInSampler {
        AlbedoMapSampler,
        NormalMapSampler,
        MaxBuiltInSamplers
    };

    struct Define {
        Define() {}
        Define(const char *name, int value) : name(name), value(value) {}
        Define &operator=(const Define &rhs) { this->name = rhs.name; this->value = rhs.value; return *this;  }

        Str                 name;
        int                 value;
    };

    struct Property {
        Variant             data;
        Texture *           texture;
    };

    Shader();
    ~Shader();

    const char *            GetName() const { return name; }
    const char *            GetHashName() const { return hashName; }

    bool                    HasVertexShader() const { return hasVertexShader; }
    bool                    HasFragmentShader() const { return hasFragmentShader; }

    bool                    IsOriginalShader() const { return !originalShader; }
    bool                    IsInstantiatedShader() const { return !!originalShader; }

    int                     GetFlags() const;

                            /// Create instantiated shader
    Shader *                InstantiateShader(const Array<Define> &defineArray);

                            /// Reinstantiate itself
    void                    Reinstantiate();

    Shader *                GetPerforatedVersion();
    Shader *                GetPremulAlphaVersion();
    Shader *                GetAmbientLitVersion();
    Shader *                GetDirectLitVersion();
    Shader *                GetAmbientLitDirectLitVersion();
    Shader *                GetParallelShadowVersion();
    Shader *                GetSpotShadowVersion();
    Shader *                GetPointShadowVersion();
    Shader *                GetGPUSkinningVersion(int index);

    void                    Bind() const;

    int                     GetConstantLocation(const char *name) const;

    void                    SetConstant1i(int index, const int constant) const;
    void                    SetConstant2i(int index, const int *constant) const;
    void                    SetConstant3i(int index, const int *constant) const;
    void                    SetConstant4i(int index, const int *constant) const;
    void                    SetConstant1i(const char *name, const int constant) const;
    void                    SetConstant2i(const char *name, const int *constant) const;
    void                    SetConstant3i(const char *name, const int *constant) const;
    void                    SetConstant4i(const char *name, const int *constant) const;

    void                    SetConstant1f(int index, const float constant) const;
    void                    SetConstant2f(int index, const float *constant) const;
    void                    SetConstant3f(int index, const float *constant) const;	
    void                    SetConstant4f(int index, const float *constant) const;
    void                    SetConstant2f(int index, const Vec2 &constant) const;
    void                    SetConstant3f(int index, const Vec3 &constant) const;
    void                    SetConstant4f(int index, const Vec4 &constant) const;
    void                    SetConstant1f(const char *name, const float constant) const;
    void                    SetConstant2f(const char *name, const float *constant) const;
    void                    SetConstant3f(const char *name, const float *constant) const;
    void                    SetConstant4f(const char *name, const float *constant) const;
    void                    SetConstant2f(const char *name, const Vec2 &constant) const;
    void                    SetConstant3f(const char *name, const Vec3 &constant) const;
    void                    SetConstant4f(const char *name, const Vec4 &constant) const;

    void                    SetConstant2x2f(int index, bool rowmajor, const Mat2 &constant) const;
    void                    SetConstant3x3f(int index, bool rowmajor, const Mat3 &constant) const;
    void                    SetConstant4x4f(int index, bool rowmajor, const Mat4 &constant) const;
    void                    SetConstant2x2f(const char *name, bool rowmajor, const Mat2 &constant) const;
    void                    SetConstant3x3f(const char *name, bool rowmajor, const Mat3 &constant) const;
    void                    SetConstant4x4f(const char *name, bool rowmajor, const Mat4 &constant) const;

    void                    SetConstantArray1i(int index, int num, const int *constant) const;
    void                    SetConstantArray2i(int index, int num, const int *constant) const;
    void                    SetConstantArray3i(int index, int num, const int *constant) const;
    void                    SetConstantArray4i(int index, int num, const int *constant) const;
    void                    SetConstantArray1i(const char *name, int num, const int *constant) const;
    void                    SetConstantArray2i(const char *name, int num, const int *constant) const;
    void                    SetConstantArray3i(const char *name, int num, const int *constant) const;
    void                    SetConstantArray4i(const char *name, int num, const int *constant) const;

    void                    SetConstantArray1f(int index, int num, const float *constant) const;
    void                    SetConstantArray2f(int index, int num, const float *constant) const;
    void                    SetConstantArray3f(int index, int num, const float *constant) const;
    void                    SetConstantArray4f(int index, int num, const float *constant) const;
    void                    SetConstantArray2f(int index, int num, const Vec2 *constant) const;
    void                    SetConstantArray3f(int index, int num, const Vec3 *constant) const;
    void                    SetConstantArray4f(int index, int num, const Vec4 *constant) const;
    void                    SetConstantArray1f(const char *name, int num, const float *constant) const;
    void                    SetConstantArray2f(const char *name, int num, const float *constant) const;
    void                    SetConstantArray3f(const char *name, int num, const float *constant) const;
    void                    SetConstantArray4f(const char *name, int num, const float *constant) const;
    void                    SetConstantArray2f(const char *name, int num, const Vec2 *constant) const;
    void                    SetConstantArray3f(const char *name, int num, const Vec3 *constant) const;
    void                    SetConstantArray4f(const char *name, int num, const Vec4 *constant) const;

    void                    SetConstantArray2x2f(int index, bool rowmajor, int num, const Mat2 *constant) const;
    void                    SetConstantArray3x3f(int index, bool rowmajor, int num, const Mat3 *constant) const;
    void                    SetConstantArray4x4f(int index, bool rowmajor, int num, const Mat4 *constant) const;
    void                    SetConstantArray2x2f(const char *name, bool rowmajor, int num, const Mat2 *constant) const;
    void                    SetConstantArray3x3f(const char *name, bool rowmajor, int num, const Mat3 *constant) const;
    void                    SetConstantArray4x4f(const char *name, bool rowmajor, int num, const Mat4 *constant) const;

    void                    SetConstantBuffer(int index, RHI::Handle bufferHandle) const;
    void                    SetConstantBuffer(const char *name, RHI::Handle bufferHandle) const;

    int                     GetSamplerUnit(const char *name) const;

    void                    SetTexture(int unit, const Texture *texture) const;
    void                    SetTexture(const char *name, const Texture *texture) const;
    void                    SetTextureArray(const char *name, int num, const Texture **textures) const;

    bool                    Create(const char *text, const char *baseDir);
    const Shader *          AddRefCount() const { refCount++; return this; }

    void                    Purge();

    bool                    Load(const char *filename);
    bool                    Reload();

    const StrHashMap<PropertyInfo> &GetPropertyInfoHashMap() const;

private:
    bool                    ParseProperties(Lexer &lexer);
    Shader *                GenerateSubShader(const Str &shaderNamePostfix, const Str &vsHeaderText, const Str &fsHeaderText, int skinning);
    bool                    GenerateGpuSkinningVersion(Shader *shader, const Str &shaderNamePrefix, const Str &vpText, const Str &fpText);
    bool                    GeneratePerforatedVersion(Shader *shader, const Str &shaderNamePrefix, const Str &vpText, const Str &fpText, bool generateGpuSkinningVersion);
    bool                    GeneratePremulAlphaVersion(Shader *shader, const Str &shaderNamePrefix, const Str &vpText, const Str &fpText, bool generateGpuSkinningVersion);
    bool                    Instantiate(const Array<Define> &defineArray);  // internal function of instantiate

    bool                    Finish(bool generatePerforatedVersion, bool genereatePremulAlphaVersion, bool genereateGpuSkinningVersion, bool generateParallelShadowVersion, bool generateSpotShadowVersion, bool generatePointShadowVersion, const char *baseDir);
    bool                    ProcessShaderText(const char *text, const char *baseDir, const Array<Define> &defineArray, Str &outStr) const;
    bool                    ProcessIncludeRecursive(const char *baseDir, Str &text) const;

    static const char *     MangleNameWithDefineList(const Str &basename, const Array<Shader::Define> &defineArray, Str &mangledName);

    Str                     hashName;
    Str                     name;
    Str                     baseDir;                ///< Base directory to include other shaders
    int                     flags;
    mutable int             refCount;
    bool                    permanence;
    int                     frameCount;

    RHI::Handle             shaderHandle;
    Str                     vsText;                 ///< Vertex shader souce code text
    Str                     fsText;                 ///< Fragment shader source code text
    bool                    hasVertexShader;
    bool                    hasFragmentShader;
    //int                   interactionParms[MaxInteractionParms];
    int                     builtInConstantLocations[MaxBuiltInConstants];
    int                     builtInSamplerUnits[MaxBuiltInSamplers];

    Array<Define>           defineArray;            ///< Define list for instantiated shader

    Shader *                originalShader;         ///< Original shader pointer. Instantiated shader has a pointer to the original shader
    Array<Shader *>         instantiatedShaders;

    Shader *                perforatedVersion;
    Shader *                premulAlphaVersion;
    Shader *                ambientLitVersion;
    Shader *                directLitVersion;
    Shader *                ambientLitDirectLitVersion;
    Shader *                parallelShadowVersion;
    Shader *                spotShadowVersion;
    Shader *                pointShadowVersion;
    Shader *                gpuSkinningVersion[3];

    StrHashMap<PropertyInfo> propertyInfoHashMap;
};

BE_INLINE Shader::Shader() {
    flags                   = 0;
    refCount                = 0;
    permanence              = false;
    frameCount              = 0; 
    shaderHandle            = RHI::NullShader;
    hasVertexShader         = false;
    hasFragmentShader       = false;
    perforatedVersion       = nullptr;
    premulAlphaVersion      = nullptr;
    ambientLitVersion       = nullptr;
    directLitVersion        = nullptr;
    ambientLitDirectLitVersion = nullptr;
    parallelShadowVersion   = nullptr;
    spotShadowVersion       = nullptr;
    pointShadowVersion      = nullptr;
    gpuSkinningVersion[0]   = nullptr;
    gpuSkinningVersion[1]   = nullptr;
    gpuSkinningVersion[2]   = nullptr;
    originalShader          = nullptr;
}

BE_INLINE Shader::~Shader() {
    Purge();
}

class ShaderManager {
    friend class Shader;

public:
    enum PredefinedOriginalShader {
        DrawArrayTextureShader,
        SimpleShader,
        SelectionIdShader,
        DepthShader,
        ConstantColorShader,
        VertexColorShader,
        ObjectMotionBlurShader,
        StandardSpecShader,
        StandardShader,
        PhongShader,
        SkyboxCubemapShader,
        SkyboxSixSidedShader,
        FogLightShader,
        BlendLightShader,
        PostObjectMotionBlurShader,
        PostCameraMotionBlurShader,
        PostPassThruShader,
        PostPassThruColorShader,
        Downscale2x2Shader,
        Downscale4x4Shader,
        Downscale4x4LogLumShader,
        Downscale4x4ExpLumShader,
        Blur5xShader,
        Blur7xShader,
        Blur15xShader,
        BlurBilinear3xShader,
        BlurBilinear4xShader,
        BlurBilinear8xShader,
        BlurAlphaMaskedBilinear8xShader,
        KawaseBlurShader,
        RadialBlurShader,
        AoBlurShader,
        PostColorTransformShader,
        PostGammaCorrectionShader,
        LinearizeDepthShader,
        CopyDownscaledCocToAlphaShader,
        CopyColorAndCocShader,
        ApplyDofShader,
        SunShaftsMaskGenShader,
        SunShaftsGenShader,
        SunShaftsDisplayShader,
        LuminanceAdaptationShader,
        BrightFilterShader,
        HdrFinalShader,
        ChromaShiftShader,
        SsaoShader,
        GenerateHomShader,
        QueryHomShader,
        MaxPredefinedOriginalShaders
    };
   
    void                    Init();
    void                    Shutdown();

    Shader *                AllocShader(const char *name);
    Shader *                FindShader(const char *name) const;
    Shader *                GetShader(const char *name);

    void                    ReleaseShader(Shader *shader, bool immediateDestroy = false);
    void                    DestroyShader(Shader *shader);
    void                    DestroyUnusedShaders();

    void                    PrecacheShader(const char *name);

    void                    RenameShader(Shader *shader, const Str &newName);

    void                    ReloadShaders();
    void                    ReloadLitSurfaceShaders();

    bool                    FindGlobalHeader(const char *text) const;
    void                    AddGlobalHeader(const char *text);
    void                    RemoveGlobalHeader(const char *text);

    static Shader *         originalShaders[MaxPredefinedOriginalShaders];

    static Shader *         drawArrayTextureShader;    
    static Shader *         simpleShader;
    static Shader *         selectionIdShader;
    static Shader *         depthShader;
    static Shader *         constantColorShader;
    static Shader *         vertexColorShader;
    static Shader *         objectMotionBlurShader;
    static Shader *         standardDefaultShader;
    static Shader *         standardDefaultAmbientLitShader;
    static Shader *         standardDefaultDirectLitShader;
    static Shader *         standardDefaultAmbientLitDirectLitShader;
    static Shader *         skyboxCubemapShader;
    static Shader *         skyboxSixSidedShader;
    static Shader *         fogLightShader;
    static Shader *         blendLightShader;
    static Shader *         postObjectMotionBlurShader;
    static Shader *         postCameraMotionBlurShader;
    static Shader *         postPassThruShader;	
    static Shader *         postPassThruColorShader;	
    static Shader *         downscale2x2Shader;
    static Shader *         downscale4x4Shader;
    static Shader *         downscale4x4LogLumShader;
    static Shader *         downscale4x4ExpLumShader;
    static Shader *         blur5xShader;
    static Shader *         blur7xShader;
    static Shader *         blur15xShader;
    static Shader *         blurBilinear3xShader;
    static Shader *         blurBilinear4xShader;
    static Shader *         blurBilinear8xShader;
    static Shader *         blurAlphaMaskedBilinear8xShader;
    static Shader *         kawaseBlurShader;
    static Shader *         radialBlurShader;
    static Shader *         aoBlurShader;
    static Shader *         postColorTransformShader;	
    static Shader *         postGammaCorrectionShader;
    static Shader *         linearizeDepthShader;
    static Shader *         copyDownscaledCocToAlphaShader;
    static Shader *         copyColorAndCocShader;
    static Shader *         applyDofShader;
    static Shader *         sunShaftsMaskGenShader;
    static Shader *         sunShaftsGenShader;
    static Shader *         sunShaftsDisplayShader;
    static Shader *         brightFilterShader;
    static Shader *         luminanceAdaptationShader;
    static Shader *         hdrFinalShader;
    static Shader *         chromaShiftShader;
    static Shader *         ssaoShader;
    static Shader *         generateHomShader;
    static Shader *         queryHomShader;

private:
    void                    InitGlobalDefines();
    void                    LoadEngineShaders();
    void                    InstantiateEngineShaders();
    void                    InitShaders();
    void                    FreeShaders();

    static void             Cmd_ListShaders(const CmdArgs &args);
    static void             Cmd_ReloadShader(const CmdArgs &args);

    StrIHashMap<Shader *>   shaderHashMap;

    StrArray                globalHeaderList;
};

extern ShaderManager        shaderManager;

BE_NAMESPACE_END
