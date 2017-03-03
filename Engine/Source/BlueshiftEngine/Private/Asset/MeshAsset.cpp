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
BEGIN_PROPERTIES(MeshAsset)
END_PROPERTIES

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

void MeshAsset::Reload() {
    Mesh *existingMesh = meshManager.FindMesh(GetResourceFilename());
    if (existingMesh) {
        existingMesh->Reload();
        EmitSignal(&SIG_Reloaded);
    }
}

BE_NAMESPACE_END