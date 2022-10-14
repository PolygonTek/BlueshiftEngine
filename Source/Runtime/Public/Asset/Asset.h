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

class AssetDatabase;

#include "Core/Object.h"

BE_NAMESPACE_BEGIN

class AssetImporter;
class Resource;
class Anim;
class AnimController;
class Material;
class Mesh;
class Prefab;
class Shader;
class Skeleton;
class Sound;
class Texture;
class ParticleSystem;

class Asset : public Object {
    friend class ::AssetDatabase;

public:
    OBJECT_PROTOTYPE(Asset);

    Asset();
    virtual ~Asset();

    virtual Str                 ToString() const override { return name; }

    bool                        IsStoredInDisk() const { return isStoredInDisk; }

    bool                        IsRedundantAsset() const;

    AssetImporter *             GetAssetImporter() const { return assetImporter; }

    Resource *                  GetResource() const { return resource; }

                                /// Gets asset name.
    const Str                   GetName() const { return name; }

                                /// Gets the cache directory of this asset. 
    const Str                   GetCacheDirectory(const char *baseDir) const;

                                /// Gets the filename of the original asset.
    const Str                   GetAssetFilename() const;

                                /// Gets the filename ready for use by engine.
    const Str                   GetResourceFilename() const;

                                /// Creates resource object when importing is completed.
    void                        CreateResource(const MetaObject &metaObject, const Guid &guid = Guid::zero);
    template <typename T>
    void                        CreateResource(const Guid &guid = Guid::zero) { CreateResource(T::metaObject, guid); }

    Anim *                      GetAnim() const;
    AnimController *            GetAnimController() const;
    Material *                  GetMaterial() const;
    Mesh *                      GetMesh() const;
    Prefab *                    GetPrefab() const;
    Shader *                    GetShader() const;
    Skeleton *                  GetSkeleton() const;
    Sound *                     GetSound() const;
    Texture *                   GetTexture() const;
    ParticleSystem *            GetParticleSystem() const;

                                /// Gets the hierarchy node.
    const Hierarchy<Asset> &    GetNode() const { return node; }

                                /// Gets the children of this node.
    void                        GetChildrenRecursive(Array<Asset *> &children) const;

    void                        WriteMetaDataFile() const;

    void                        Rename(const Str &newName);

    void                        Reload();

    void                        Save();

    void                        SetModified(bool modified);

                                /// Asset path to relative to base dir.
    static const Str            NormalizeAssetPath(const Str &assetPath);

                                /// Gets .meta filename associated with an asset path.
    static const Str            GetMetaFileNameFromAssetPath(const char *assetPath);

    static const SignalDef      SIG_Reloaded;
    static const SignalDef      SIG_Modified;

protected:
    AssetImporter *             assetImporter;
    Resource *                  resource;
    Str                         name;
    Hierarchy<Asset>            node;
    bool                        isStoredInDisk;
};

BE_NAMESPACE_END
