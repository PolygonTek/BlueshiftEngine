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
#include "Render/Mesh.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Mesh", MeshAsset, Asset)
BEGIN_EVENTS(MeshAsset)
END_EVENTS

void MeshAsset::RegisterProperties() {
}

MeshAsset::MeshAsset() {
    mesh = nullptr;
}

MeshAsset::~MeshAsset() {
    if (!mesh) {
        const Str meshPath = resourceGuidMapper.Get(GetGuid());
        mesh = meshManager.FindMesh(meshPath);
    }

    if (mesh) {
        meshManager.ReleaseMesh(mesh, true);
    }
}

Mesh *MeshAsset::GetMesh() {
    if (mesh) {
        return mesh;
    }
    const Str meshPath = resourceGuidMapper.Get(GetGuid());
    mesh = meshManager.GetMesh(meshPath);
    return mesh;
}

void MeshAsset::Rename(const Str &newName) {
    Mesh *existingMesh = meshManager.FindMesh(GetResourceFilename());
    if (existingMesh) {
        meshManager.RenameMesh(existingMesh, newName);
    }

    Asset::Rename(newName);
}

void MeshAsset::Reload() {
    Mesh *existingMesh = meshManager.FindMesh(GetResourceFilename());
    if (existingMesh) {
        existingMesh->Reload();
        EmitSignal(&SIG_Reloaded);
    }
}

BE_NAMESPACE_END
