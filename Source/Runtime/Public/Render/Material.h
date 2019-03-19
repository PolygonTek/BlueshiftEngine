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
    friend class Batch;

public:
    struct Flag {
        enum Enum {
            PolygonOffset       = BIT(0),
            NoShadow            = BIT(1),
            ForceShadow         = BIT(2),
            UnsmoothTangents    = BIT(4),
            LoadedFromFile      = BIT(9)
        };
    };

    struct Type {
        enum Enum {
            Surface             = 0,
            Decal               = 1,
            Light               = 2,
            BlendLight          = 3,
            FogLight            = 4
        };
    };

    struct Sort {
        enum Enum {
            Bad                 = 0,
            SubView             = 1,
            Opaque              = 2,
            AlphaTest           = 3,
            Sky                 = 4,
            Translucent         = 10,
            Overlay             = 11,
            Nearest             = 15
        };
    };

    struct VertexColorMode {
        enum Enum {
            Ignore,
            Modulate,
            InverseModulate
        };
    };

    // Hints for material by one texture
    struct TextureHint {
        enum Enum {
            None,
            Light,
            VertexColor,
            Lightmapped,
            Sprite,
            Overlay,
            EnvCubeMap
        };
    };

    struct RenderingMode {
        enum Enum {
            Opaque,
            AlphaCutoff,
            AlphaBlend
        };
    };

    // Controls how transparent objects are rendered.
    // This is only valid when the rendering mode is alpha blend.
    struct Transparency {
        enum Enum {
            Default,            ///< Render normally.
            TwoPassesOneSide,   ///< Render in the depth buffer, then again in color buffer.
            TwoPassesTwoSides   ///< Render twice in the color buffer first with its back faces, then with its front faces.
        };
    };

    struct ShaderPass {
        RenderingMode::Enum     renderingMode;
        Transparency::Enum      transparency;
        int                     cullType;
        int                     stateBits;
        float                   cutoffAlpha;
        VertexColorMode::Enum   vertexColorMode;
        bool                    useOwnerColor;
        Color4                  constantColor;
        Texture *               texture;
        Vec2                    tcScale;
        Vec2                    tcTranslation;
        bool                    instancingEnabled;
        Shader *                referenceShader;
        Shader *                shader;
        StrHashMap<Shader::Property> shaderProperties;
    };

    Material();
    ~Material();

    const char *                GetName() const { return name; }
    const char *                GetHashName() const { return hashName; }
    int                         GetFlags() const { return flags; }
    int                         GetType() const { return type; }
    RenderingMode::Enum         GetRenderingMode() const { return pass->renderingMode; }
    void                        SetRenderingMode(RenderingMode::Enum mode);
    int                         GetCullType() const { return pass->cullType; }
    int                         GetSort() const { return sort; }

    bool                        IsLitSurface() const;
    bool                        IsSkySurface() const;
    bool                        IsShadowCaster() const;

    const ShaderPass *          GetPass() const { return pass; }
    ShaderPass *                GetPass() { return pass; }

    void                        SetName(const char *name) { this->name = name; }

    bool                        Create(const char *text);

    void                        Purge();

    bool                        Load(const char *filename);
    bool                        Reload();

    void                        Write(const char *filename);

    const Material *            AddRefCount() const { refCount++; return this; }
    int                         GetRefCount() const { return refCount; }

    void                        ChangeShader(Shader *shader);
    void                        CommitShaderPropertiesChanged();

private:
    void                        Finish();
    bool                        ParsePass(Lexer &lexer, ShaderPass *pass);
    bool                        ParseRenderingMode(Lexer &lexer, RenderingMode::Enum *renderingMode) const;
    bool                        ParseBlendFunc(Lexer &lexer, int *blendSrc, int *blendDst) const;
    //void                      MultiplyTextureMatrix(Pass *pass, int inMatrix[2][3]);
    bool                        ParseShaderProperties(Lexer &lexer, Dict &properties);

    Str                         hashName;
    Str                         name;
    mutable int                 refCount;               // reference count
    bool                        permanence;             // is permanent material ?
    int                         index;                  // index for sorting materials when rendering

    int                         flags;
    Type::Enum                  type;
    Sort::Enum                  sort;

    ShaderPass *                pass;
};

BE_INLINE Material::Material() {
    refCount    = 0;
    permanence  = false;
    index       = -1;
    flags       = 0;
    type        = Type::Surface;
    sort        = Sort::Bad;
    pass        = nullptr;
}

BE_INLINE Material::~Material() {
    Purge();
}

class MaterialManager {
    friend class Material;

public:
    void                        Init();
    void                        Shutdown();

    Material *                  AllocMaterial(const char *name);
    Material *                  FindMaterial(const char *name) const;
    Material *                  GetMaterial(const char *name);
    Material *                  GetSingleTextureMaterial(const Texture *texture, Material::TextureHint::Enum hint = Material::TextureHint::None);

    void                        ReleaseMaterial(Material *material, bool immediateDestroy = false);
    void                        DestroyMaterial(Material *material);
    void                        DestroyUnusedMaterials();

    void                        PrecacheMaterial(const char *filename);

    void                        RenameMaterial(Material *material, const Str &newName);

    int                         GetIndexByMaterial(const Material *material) const { return material->index; }

                                // pre-defined materials
    static Material *           defaultMaterial;
    static Material *           whiteMaterial;
    static Material *           unlitMaterial;
    static Material *           blendMaterial;
    static Material *           whiteLightMaterial;
    static Material *           zeroClampLightMaterial;
    static Material *           defaultSkyboxMaterial;

    static const int            MaxCount = 65536;

private:
    static void                 Cmd_ListMaterials(const CmdArgs &args);
    static void                 Cmd_ReloadMaterial(const CmdArgs &args);

    void                        CreateEngineMaterials();

    StrIHashMap<Material *>     materialHashMap;
};

extern MaterialManager          materialManager;

BE_NAMESPACE_END
