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

#include "Core/Object.h"

BE_NAMESPACE_BEGIN

class Asset;
class Texture;
class Shader;
class Material;
class Skeleton;
class Mesh;
class ParticleSystem;
class Anim;
class AnimController;
class Prefab;
class Sound;

class Resource : public Object {
    friend class Asset;

public:
    ABSTRACT_PROTOTYPE(Resource);

    Resource();
    virtual ~Resource() = 0;

    virtual bool                IsRedundant() const { return false; }

    virtual void                Rename(const Str &newName) = 0;

    virtual bool                Reload() = 0;

    virtual bool                Save() = 0;

protected:
    Asset *                     asset;
};

class FolderResource : public Resource {
public:
    OBJECT_PROTOTYPE(FolderResource);

    FolderResource();
    ~FolderResource();

    virtual void                Rename(const Str &newName) override;

    virtual bool                Reload() override;

    virtual bool                Save() override;
};

class TextureResource : public Resource {
public:
    OBJECT_PROTOTYPE(TextureResource);

    TextureResource();
    ~TextureResource();

    Texture *                   GetTexture();

    virtual void                Rename(const Str &newName) override;

    virtual bool                Reload() override;

    virtual bool                Save() override;

private:
    Texture *                   texture;
};

class Texture2DResource : public TextureResource {
public:
    OBJECT_PROTOTYPE(Texture2DResource);

    Texture2DResource();
    ~Texture2DResource();
};

class TextureCubeMapResource : public TextureResource {
public:
    OBJECT_PROTOTYPE(TextureCubeMapResource);

    TextureCubeMapResource();
    ~TextureCubeMapResource();
};

class TextureSpriteResource : public TextureResource {
public:
    OBJECT_PROTOTYPE(TextureSpriteResource);

    TextureSpriteResource();
    ~TextureSpriteResource();
};

class ShaderResource : public Resource {
public:
    OBJECT_PROTOTYPE(ShaderResource);

    ShaderResource();
    ~ShaderResource();

    Shader *                    GetShader();

    virtual void                Rename(const Str &newName) override;

    virtual bool                Reload() override;

    virtual bool                Save() override;

private:
    Shader *                    shader;
};

class MaterialResource : public Resource {
public:
    OBJECT_PROTOTYPE(MaterialResource);

    MaterialResource();
    ~MaterialResource();

    Material *                  GetMaterial();

    virtual void                Rename(const Str &newName) override;

    virtual bool                Reload() override;

    virtual bool                Save() override;

private:
    Material *                  material;
};

class FontResource : public Resource {
public:
    OBJECT_PROTOTYPE(FontResource);

    FontResource();
    ~FontResource();

    virtual void                Rename(const Str &newName) override;

    virtual bool                Reload() override;

    virtual bool                Save() override;
};

class SkeletonResource : public Resource {
public:
    OBJECT_PROTOTYPE(SkeletonResource);

    SkeletonResource();
    ~SkeletonResource();

    Skeleton *                  GetSkeleton();

    virtual void                Rename(const Str &newName) override;

    virtual bool                Reload() override;

    virtual bool                Save() override;

private:
    Skeleton *                  skeleton;
};

class MeshResource : public Resource {
public:
    OBJECT_PROTOTYPE(MeshResource);

    MeshResource();
    ~MeshResource();

    Mesh *                      GetMesh();

    virtual void                Rename(const Str &newName) override;

    virtual bool                Reload() override;

    virtual bool                Save() override;

private:
    Mesh *                      mesh;
};

class ParticleSystemResource : public Resource {
public:
    OBJECT_PROTOTYPE(ParticleSystemResource);

    ParticleSystemResource();
    ~ParticleSystemResource();

    ParticleSystem *            GetParticleSystem();

    virtual void                Rename(const Str &newName) override;

    virtual bool                Reload() override;

    virtual bool                Save() override;

private:
    ParticleSystem *            particleSystem;
};

class AnimResource : public Resource {
public:
    OBJECT_PROTOTYPE(AnimResource);

    AnimResource();
    ~AnimResource();

    Anim *                      GetAnim();

    virtual void                Rename(const Str &newName) override;

    virtual bool                Reload() override;

    virtual bool                Save() override;

private:
    Anim *                      anim;
};

class FbxResource : public Resource {
public:
    OBJECT_PROTOTYPE(FbxResource);

    FbxResource();
    ~FbxResource();

    virtual bool                IsRedundant() const override { return true; }

    virtual void                Rename(const Str &newName) override;

    virtual bool                Reload() override;

    virtual bool                Save() override;
};

class JointMaskResource : public Resource {
public:
    OBJECT_PROTOTYPE(JointMaskResource);

    JointMaskResource();
    ~JointMaskResource();

    virtual void                Rename(const Str &newName) override;

    virtual bool                Reload() override;

    virtual bool                Save() override;
};

class AnimControllerResource : public Resource {
public:
    OBJECT_PROTOTYPE(AnimControllerResource);

    AnimControllerResource();
    ~AnimControllerResource();

    AnimController *            GetAnimController();

    virtual void                Rename(const Str &newName) override;

    virtual bool                Reload() override;

    virtual bool                Save() override;

private:
    AnimController *            animController;
};

class PrefabResource : public Resource {
public:
    OBJECT_PROTOTYPE(PrefabResource);

    PrefabResource();
    ~PrefabResource();

    Prefab *                    GetPrefab();

    virtual void                Rename(const Str &newName) override;

    virtual bool                Reload() override;

    virtual bool                Save() override;

private:
    Prefab *                    prefab;
};

class SoundResource : public Resource {
public:
    OBJECT_PROTOTYPE(SoundResource);

    SoundResource();
    ~SoundResource();

    Sound *                     GetSound();

    virtual void                Rename(const Str &newName) override;

    virtual bool                Reload() override;

    virtual bool                Save() override;

private:
    Sound *                     sound;
};

class MapResource : public Resource {
public:
    OBJECT_PROTOTYPE(MapResource);

    MapResource();
    ~MapResource();

    virtual void                Rename(const Str &newName) override;

    virtual bool                Reload() override;

    virtual bool                Save() override;
};

class ScriptResource : public Resource {
public:
    OBJECT_PROTOTYPE(ScriptResource);

    ScriptResource();
    ~ScriptResource();

    virtual void                Rename(const Str &newName) override;

    virtual bool                Reload() override;

    virtual bool                Save() override;
};

BE_NAMESPACE_END
