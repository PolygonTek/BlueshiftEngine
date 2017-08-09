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

class MaterialEditor;

BE_NAMESPACE_BEGIN

class Dict;
class Texture;
class Shader;

class Material {
    friend class MaterialManager;
    friend class ::MaterialEditor;
    friend class RBSurf;

public:
    enum Flag {
        PolygonOffset           = BIT(0),
        NoShadow                = BIT(1),
        ForceShadow             = BIT(2),
        UnsmoothTangents        = BIT(4),
        LoadedFromFile          = BIT(9)
    };

    enum Type {
        LitSurface,
        UnlitSurface,
        SkySurface,
        Light,
    };

    enum LightMaterialType {
        LightMaterial,
        BlendLightMaterial,
        FogLightMaterial
    };

    enum Sort {
        BadSort                 = 0,
        SubViewSort             = 1,
        OpaqueSort              = 2,    //
        SkySort                 = 3,
        AlphaTestSort           = 4,    //
        TranslucentSort         = 10,   //
        UnlitBlendSort          = 11,
        NearestSort             = 15
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

    enum RenderingMode {
        Opaque,
        AlphaCutoff,
        AlphaBlend
    };

    struct ShaderPass {
        RenderingMode       renderingMode;
        int                 cullType;
        int                 stateBits;
        float               cutoffAlpha;
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
    void                    SetType(Type type);
    int                     GetLightMaterialType() const { return lightMaterialType; }
    RenderingMode           GetRenderingMode() const { return pass->renderingMode; }
    void                    SetRenderingMode(RenderingMode mode);
    int                     GetCullType() const { return pass->cullType; }
    int                     GetSort() const { return sort; }

    bool                    IsLitSurface() const;
    bool                    IsShadowCaster() const;

    const ShaderPass *      GetPass() const { return pass; }
    ShaderPass *            GetPass() { return pass; }

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
    void                    Finish();
    bool                    ParsePass(Lexer &lexer, ShaderPass *pass);
    bool                    ParseRenderingMode(Lexer &lexer, RenderingMode *renderingMode) const;
    bool                    ParseDepthFunc(Lexer &lexer, int *depthFunc) const; // to be removed
    bool                    ParseBlendFunc(Lexer &lexer, int *blendSrc, int *blendDst) const;
    bool                    ParseLightMaterialType(Lexer &lexer);
    //void                  MultiplyTextureMatrix(Pass *pass, int inMatrix[2][3]);    
    bool                    ParseShaderProperties(Lexer &lexer, Dict &properties);

    Str                     hashName;
    Str                     name;
    mutable int             refCount;               // reference count
    bool                    permanence;             // is permanent material ?
    int                     index;                  // index of materialHashMap

    int                     flags;
    Type                    type;
    LightMaterialType       lightMaterialType;
    Sort                    sort;

    ShaderPass *            pass;
};

BE_INLINE Material::Material() {
    refCount                = 0;
    permanence              = false;
    index                   = -1;
    flags                   = 0;
    type                    = LitSurface;
    lightMaterialType       = LightMaterial;
    sort                    = BadSort;
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
