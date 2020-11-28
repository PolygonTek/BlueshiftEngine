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
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Mesh", MeshResource, Resource)
BEGIN_EVENTS(MeshResource)
END_EVENTS

void MeshResource::RegisterProperties() {
}

MeshResource::MeshResource() {
    mesh = nullptr;
}

MeshResource::~MeshResource() {
    if (mesh) {
        meshManager.ReleaseMesh(mesh);
    }
}

Mesh *MeshResource::GetMesh() {
    if (mesh) {
        return mesh;
    }
    const Str meshPath = resourceGuidMapper.Get(asset->GetGuid());
    mesh = meshManager.GetMesh(meshPath);
    return mesh;
}

void MeshResource::Rename(const Str &newName) {
    const Str meshPath = resourceGuidMapper.Get(asset->GetGuid());
    Mesh *existingMesh = meshManager.FindMesh(meshPath);
    if (existingMesh) {
        meshManager.RenameMesh(existingMesh, newName);
    }
}

bool MeshResource::Reload() {
    const Str meshPath = resourceGuidMapper.Get(asset->GetGuid());
    Mesh *existingMesh = meshManager.FindMesh(meshPath);
    if (existingMesh) {
        existingMesh->Reload();
        return true;
    }
    return false;
}

bool MeshResource::Save() {
    return false;
}

BE_NAMESPACE_END
