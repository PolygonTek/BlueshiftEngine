#pragma once

class AssetDatabase;

#include "Core/Object.h"

BE_NAMESPACE_BEGIN

class AssetImporter;

class Asset : public Object {
    friend class ::AssetDatabase;

public:
    ABSTRACT_PROTOTYPE(Asset);

    Asset();
    virtual ~Asset() = 0;

    virtual const Str           ToString() const override { return name; }

    bool                        IsStoredInDisk() const { return isStoredInDisk; }
    bool                        IsRedundantAsset() const { return isRedundantAsset;  }

    AssetImporter *             GetAssetImporter() const { return assetImporter; }

                                // Get asset name
    const Str                   GetName() const { return name; }

                                // Get source asset file path (DCC file)
    const Str                   GetAssetFilename() const;

                                // Get resource file path for use in engine
    const Str                   GetResourceFilename() const;

                                // Get cache directory in the "Library" directory
    const Str                   GetCacheDirectory(const char *baseLibraryDir) const;

    const Hierarchy<Asset> &    GetNode() const { return node; }
    void                        GetChildren(Array<Asset *> &children) const;

    void                        WriteMetaDataFile() const;

    virtual void                Reload() = 0;

    virtual void                Save() = 0;

    void                        SetModified(bool modified);

                                // Asset path to relative to base dir
    static const Str            NormalizeAssetPath(const Str &assetPath);

                                // Gets .meta filename associated with an asset
    static const Str            GetMetaFilenameFromAssetPath(const char *assetPath);

protected:
    AssetImporter *             assetImporter;
    Str                         name;
    Hierarchy<Asset>            node;
    bool                        isStoredInDisk;
    bool                        isRedundantAsset;
};

class Texture;
class Shader;
class Material;
class Skeleton;
class Mesh;
class Anim;
class AnimController;
class Prefab;
class Sound;

class FolderAsset : public Asset {
public:
    OBJECT_PROTOTYPE(FolderAsset);

    FolderAsset();
    ~FolderAsset();

    virtual void                Reload() override {}

    virtual void                Save() override {}
};

class TextureAsset : public Asset {
public:
    OBJECT_PROTOTYPE(TextureAsset);

    TextureAsset();
    ~TextureAsset();

    Texture *                   GetTexture();

    virtual void                Reload() override;

    virtual void                Save() override {}

private:
    Texture *                   texture;
};

class ShaderAsset : public Asset {
public:
    OBJECT_PROTOTYPE(ShaderAsset);

    ShaderAsset();
    ~ShaderAsset();

    Shader *                    GetShader();

    virtual void                Reload() override;

    virtual void                Save() override {}

private:
    Shader *                    shader;
};

class MaterialAsset : public Asset {
public:
    OBJECT_PROTOTYPE(MaterialAsset);

    MaterialAsset();
    ~MaterialAsset();

    Material *                  GetMaterial();

    virtual void                Reload() override;

    virtual void                Save() override;

private:
    Material *                  material;
};

class FontAsset : public Asset {
public:
    OBJECT_PROTOTYPE(FontAsset);

    FontAsset();
    ~FontAsset();

    virtual void                Reload() override {}

    virtual void                Save() override {}
};

class SkeletonAsset : public Asset {
public:
    OBJECT_PROTOTYPE(SkeletonAsset);

    SkeletonAsset();
    ~SkeletonAsset();

    Skeleton *                  GetSkeleton();

    virtual void                Reload() override;

    virtual void                Save() override {}

private:
    Skeleton *                  skeleton;
};

class MeshAsset : public Asset {
public:
    OBJECT_PROTOTYPE(MeshAsset);

    MeshAsset();
    ~MeshAsset();

    Mesh *                      GetMesh();

    virtual void                Reload() override;

    virtual void                Save() override {}

private:
    Mesh *                      mesh;
};

class AnimAsset : public Asset {
public:
    OBJECT_PROTOTYPE(AnimAsset);

    AnimAsset();
    ~AnimAsset();

    Anim *                      GetAnim();

    virtual void                Reload() override;

    virtual void                Save() override {}

private:
    Anim *                      anim;
};

class FbxAsset : public Asset {
public:
    OBJECT_PROTOTYPE(FbxAsset);

    FbxAsset();
    ~FbxAsset();

    virtual void                Reload() override {}

    virtual void                Save() override {}
};

class JointMaskAsset : public Asset {
public:
    OBJECT_PROTOTYPE(JointMaskAsset);

    JointMaskAsset();
    ~JointMaskAsset();

    virtual void                Reload() override;

    virtual void                Save() override {}
};

class AnimControllerAsset : public Asset {
public:
    OBJECT_PROTOTYPE(AnimControllerAsset);

    AnimControllerAsset();
    ~AnimControllerAsset();

    AnimController *            GetAnimController();

    virtual void                Reload() override;

    virtual void                Save() override;

private:
    AnimController *            animController;
};

class PrefabAsset : public Asset {
public:
    OBJECT_PROTOTYPE(PrefabAsset);

    PrefabAsset();
    ~PrefabAsset();

    Prefab *                    GetPrefab();

    virtual void                Reload() override;

    virtual void                Save() override;

private:
    Prefab *                    prefab;
};

class SoundAsset : public Asset {
public:
    OBJECT_PROTOTYPE(SoundAsset);

    SoundAsset();
    ~SoundAsset();

    Sound *                     GetSound();

    virtual void                Reload() override;

    virtual void                Save() override {}

private:
    Sound *                     sound;
};

class MapAsset : public Asset {
public:
    OBJECT_PROTOTYPE(MapAsset);

    MapAsset();
    ~MapAsset();

    virtual void                Reload() override {}

    virtual void                Save() override {}
};

class ScriptAsset : public Asset {
public:
    OBJECT_PROTOTYPE(ScriptAsset);

    ScriptAsset();
    ~ScriptAsset();

    virtual void                Reload() override;

    virtual void                Save() override {}
};

extern const SignalDef          SIG_Reloaded;
extern const SignalDef          SIG_Modified;

BE_NAMESPACE_END