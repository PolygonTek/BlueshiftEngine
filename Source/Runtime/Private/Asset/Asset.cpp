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
#include "Asset/Asset.h"
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Asset", Asset, Object)
BEGIN_EVENTS(Asset)
END_EVENTS

const SignalDef Asset::SIG_Reloaded("Asset::Reloaded");
const SignalDef Asset::SIG_Modified("Asset::Modified", "i");

void Asset::RegisterProperties() {
}

Asset::Asset() {
    node.SetOwner(this);
    assetImporter = nullptr;
    resource = nullptr;
    isStoredInDisk = false;
}

Asset::~Asset() {
    if (assetImporter) {
        AssetImporter::DestroyInstanceImmediate(assetImporter);
    }
    if (resource) {
        Resource::DestroyInstanceImmediate(resource);
    }
}

bool Asset::IsRedundantAsset() const { 
    return resource->IsRedundant(); 
}

void Asset::CreateResource(const MetaObject &metaObject, const Guid &guid) {
    if (resource) {
        if (resource->GetMetaObject() == &metaObject) {
            // No need to recreate resource object if it's type is the same.
            return;
        }
        DestroyInstanceImmediate(resource);
    }

    resource = static_cast<Resource *>(metaObject.CreateInstance(guid));
    resource->asset = this;
}

Anim *Asset::GetAnim() const {
    AnimResource *animResource = resource->Cast<AnimResource>();
    if (animResource) {
        return animResource->GetAnim();
    }
    return nullptr;
}

AnimController *Asset::GetAnimController() const {
    AnimControllerResource *animControllerResource = resource->Cast<AnimControllerResource>();
    if (animControllerResource) {
        return animControllerResource->GetAnimController();
    }
    return nullptr;
}

Material *Asset::GetMaterial() const {
    MaterialResource *materialResource = resource->Cast<MaterialResource>();
    if (materialResource) {
        return materialResource->GetMaterial();
    }
    return nullptr;
}

Mesh *Asset::GetMesh() const {
    MeshResource *meshResource = resource->Cast<MeshResource>();
    if (meshResource) {
        return meshResource->GetMesh();
    }
    return nullptr;
}

Prefab *Asset::GetPrefab() const {
    PrefabResource *prefabResource = resource->Cast<PrefabResource>();
    if (prefabResource) {
        return prefabResource->GetPrefab();
    }
    return nullptr;
}

Shader *Asset::GetShader() const {
    ShaderResource *shaderResource = resource->Cast<ShaderResource>();
    if (shaderResource) {
        return shaderResource->GetShader();
    }
    return nullptr;
}

Skeleton *Asset::GetSkeleton() const {
    SkeletonResource *skeletonResource = resource->Cast<SkeletonResource>();
    if (skeletonResource) {
        return skeletonResource->GetSkeleton();
    }
    return nullptr;
}

Sound *Asset::GetSound() const {
    SoundResource *soundResource = resource->Cast<SoundResource>();
    if (soundResource) {
        return soundResource->GetSound();
    }
    return nullptr;
}

Texture *Asset::GetTexture() const {
    TextureResource *textureResource = resource->Cast<TextureResource>();
    if (textureResource) {
        return textureResource->GetTexture();
    }
    return nullptr;
}

const Str Asset::NormalizeAssetPath(const Str &assetPath) {
    Str normalizedAssetPath = assetPath;

    Str::ConvertPathSeperator(normalizedAssetPath, PATHSEPERATOR_CHAR);

    return normalizedAssetPath;
}

const Str Asset::GetMetaFileNameFromAssetPath(const char *assetPath) {
    Str metaFileName = Asset::NormalizeAssetPath(assetPath);
    metaFileName += ".meta";

    return metaFileName;
}

const Str Asset::GetCacheDirectory(const char *baseDir) const {
    Str cacheDir = baseDir;
    cacheDir.AppendPath("metadata");
    cacheDir.AppendPath(va("%s", GetGuid().ToString(Guid::Format::DigitsWithHyphens)));

    Str::ConvertPathSeperator(cacheDir, PATHSEPERATOR_CHAR);
    
    return cacheDir;
}

const Str Asset::GetAssetFilename() const {
    Str assetPath;
    const Asset *asset = this;
    
    while (asset) {
        Str tempPath = asset->name;
        tempPath.AppendPath(assetPath, PATHSEPERATOR_CHAR);
        assetPath = tempPath;

        asset = asset->node.GetParent();
    }

    return assetPath;
}

const Str Asset::GetResourceFilename() const {
    Str name;

    if (assetImporter) {
        name = Asset::NormalizeAssetPath(assetImporter->GetResourceFilename());
    } else {
        name = Asset::NormalizeAssetPath(GetAssetFilename());
    }

    return name;
}

void Asset::Rename(const Str &newName) {
    resource->Rename(newName);

    name = newName;
    name.StripPath();
}

void Asset::Reload() {
    if (resource->Reload()) {
        EmitSignal(&SIG_Reloaded);
    }
}

void Asset::Save() {
    if (resource->Save()) {
        EmitSignal(&SIG_Modified, 0);
    }
}

void Asset::GetChildren(Array<Asset *> &children) const {
    for (Asset *child = node.GetFirstChild(); child; child = child->node.GetNextSibling()) {
        children.Append(child);
        child->GetChildren(children);
    }
}

void Asset::WriteMetaDataFile() const {
    Str metaFileName = Asset::GetMetaFileNameFromAssetPath(GetAssetFilename());

    Json::Value metaDataValue;
    Serialize(metaDataValue);

    if (assetImporter) {
        Json::Value importerValue;
        assetImporter->Serialize(importerValue);

        metaDataValue["importer"] = importerValue;
    }

    if (resource) {
        Json::Value resourceValue;
        resource->Serialize(resourceValue);

        metaDataValue["resource"] = resourceValue;
    }
 
    Json::StyledWriter jsonWriter;
    Str jsonText = jsonWriter.write(metaDataValue).c_str();

    fileSystem.WriteFile(metaFileName, jsonText.c_str(), jsonText.Length());
}

void Asset::SetModified(bool modified) {
    EmitSignal(&SIG_Modified, modified ? 1 : 0);
}

BE_NAMESPACE_END
