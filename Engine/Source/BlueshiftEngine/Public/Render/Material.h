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

    Material

-------------------------------------------------------------------------------
*/

#include "Containers/HashMap.h"
#include "Shader.h"

BE_NAMESPACE_BEGIN

class Dict;
class Texture;
class Shader;

class Material {
    friend class MaterialManager;
    friend class RBSurf;

public:
    enum Flag {
        PolygonOffset           = BIT(0),
        NoShadow                = BIT(1),
        ForceShadow             = BIT(2),
        UnsmoothTangents        = BIT(4),
        Overlay                 = BIT(5),
        LoadedFromFile          = BIT(9)
    };

    enum Type {
        BadType,
        StandardType,
        LightType,
        LitSurfaceType
    };

    enum Sort {
        BadSort                 = 0,
        SubViewSort             = 1,
        SkySort                 = 2,
        OpaqueSort              = 3,    //
        AlphaTestSort           = 4,    //
        DecalSort               = 8,    //
        RefractionSort          = 9,
        AdditiveLightingSort    = 10,   //
        BlendSort               = 11,
        NearestSort             = 15
    };

    enum LightSort {
        BadLightSort            = 0,
        NormalLightSort         = 1,
        BlendLightSort          = 2,
        FogLightSort            = 3
    };

    enum Coverage {
        EmptyCoverage           = 0,
        BackgroundCoverage      = BIT(0),
        OpaqueCoverage          = BIT(1),   // completely fills the triangle
        PerforatedCoverage      = BIT(2),   // may have alpha tested holes
        TranslucentCoverage     = BIT(3)    // blended with background
    };

    enum VertexColorMode {
        IgnoreVertexColor,
        ModulateVertexColor,
        InverseModulateVertexColor
    };

    //--------------------------------------------------------------------------------------------------
    // hints for material by one texture
    //--------------------------------------------------------------------------------------------------
    enum TextureHint {
        NoHint,
        LightHint,
        VertexColorHint,
        LightmappedHint,
        SpriteHint,
        OverlayHint
    };

    struct Pass {
        int                 stateBits;
        float               alphaRef;
        VertexColorMode     vertexColorMode;
        bool                useOwnerColor;
        Color4              constantColor;
        Texture *           texture;
        Vec2                tcScale;
        Vec2                tcTranslation;
        Shader *            shader;
        Shader *            referenceShader;
        StrHashMap<Shader::Property> shaderProperties;          // define prop 이 바뀌면 reinstantiate 해야 할듯
    };

    Material();
    ~Material();

    const char *            GetName() const { return name; }
    const char *            GetHashName() const { return hashName; }
    int                     GetFlags() const { return flags; }
    Type                    GetType() const { return type; }
    int                     GetCullType() const { return cullType; }
    int                     GetSort() const { return sort; }
    int                     GetLightSort() const { return lightSort; }
    int                     GetCoverage() const { return coverage; }

    bool                    IsLitSurface() const;
    bool                    IsShadowCaster() const;

    const Pass *            GetPass() const { return pass; }
    Pass *                  GetPass() { return pass; }

    void                    SetName(const char *name) { this->name = name; }

    bool                    Create(const char *text);

    void                    Purge();

    bool                    Load(const char *filename);
    bool                    Reload();

    void                    Write(const char *filename);

    const Material *        AddRefCount() const { refCount++; return this; }

    void                    ChangeShader(Shader *shader);
    void                    EndShaderPropertiesChanged();

private:
    bool                    ParsePass(Lexer &lexer, Pass *pass);
    bool                    ParseAlphaFunc(Lexer &lexer, int *alphaFunc, Pass *pass) const;
    bool                    ParseDepthFunc(Lexer &lexer, int *depthFunc) const;
    bool                    ParseBlendFunc(Lexer &lexer, int *blendSrc, int *blendDst) const;
    bool                    ParseSort(Lexer &lexer);
    bool                    ParseLightSort(Lexer &lexer);
    //void                    MultiplyTextureMatrix(Pass *pass, int inMatrix[2][3]);    
    bool                    ParseShaderProperties(Lexer &lexer, Dict &properties);
    void                    Finish();

    Str                     hashName;
    Str                     name;
    mutable int             refCount;               // reference count
    bool                    permanence;             // is permanent material ?
    int                     index;                  // index of materialHashMap

    int                     flags;
    Type                    type;
    int                     coverage;
    int                     cullType;
    Sort                    sort;
    LightSort               lightSort;
    
    Pass *                  pass;
};

BE_INLINE Material::Material() {
    refCount                = 0;
    permanence              = false;
    index                   = -1;
    flags                   = 0;
    type                    = BadType;
    cullType                = RHI::BackCull;
    coverage                = EmptyCoverage;
    sort                    = BadSort;
    lightSort               = BadLightSort;
    pass                    = nullptr;
}

BE_INLINE Material::~Material() {
    Purge();
}

class MaterialManager {
    friend class Material;

public:
    void                    Init();
    void                    Shutdown();

    Material *              AllocMaterial(const char *name);
    Material *              FindMaterial(const char *name) const;
    Material *              GetMaterial(const char *name);
    Material *              GetTextureMaterial(const Texture *texture, Material::TextureHint hint = Material::NoHint);

    void                    ReleaseMaterial(Material *material, bool immediateDestroy = false);
    void                    DestroyMaterial(Material *material);
    void                    DestroyUnusedMaterials();

    int                     GetIndexByMaterial(const Material *material) const { return material->index; }
    Material *              GetMaterialByIndex(int index) const;

                            // pre-defined materials
    static Material *       defaultMaterial;
    static Material *       whiteMaterial;
    static Material *       blendMaterial;
    static Material *       whiteLightMaterial;
    static Material *       zeroClampLightMaterial;
    static Material *       defaultSkyboxMaterial;

private:
    static void             Cmd_ListMaterials(const CmdArgs &args);
    static void             Cmd_ReloadMaterial(const CmdArgs &args);

    StrIHashMap<Material *> materialHashMap;
};

extern MaterialManager      materialManager;

BE_NAMESPACE_END
