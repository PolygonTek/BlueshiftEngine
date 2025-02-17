// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Mesh.h"

MeshManager     meshManager;

void MeshManager::Init() {
    meshHashMap.Init(1024, 64, 64);

    CreateEngineMeshes();
}

void MeshManager::CreateEngineMeshes() {
    // Create default (box shape) mesh
    defaultMesh = AllocMesh("_defaultMesh");
    defaultMesh->flags |= Mesh::Flag::Permanent;
    defaultMesh->CreateDefaultBox();

    // Create default quad mesh
    defaultQuadMesh = AllocMesh("_defaultQuadMesh");
    defaultQuadMesh->flags |= Mesh::Flag::Permanent;
    defaultQuadMesh->CreatePlane(BE1::Vec3::origin, BE1::Angles(0, 90, 0).ToMat3(), BE1::MeterToUnit(1.0f), 1);

    // Create default plane mesh
    defaultPlaneMesh = AllocMesh("_defaultPlaneMesh");
    defaultPlaneMesh->flags |= Mesh::Flag::Permanent;
    defaultPlaneMesh->CreatePlane(BE1::Vec3::origin, BE1::Mat3::identity, BE1::MeterToUnit(10.0f), 10);

    // Create default box mesh
    defaultBoxMesh = AllocMesh("_defaultBoxMesh");
    defaultBoxMesh->flags |= Mesh::Flag::Permanent;
    defaultBoxMesh->CreateBox(BE1::Vec3::origin, BE1::Mat3::identity, BE1::Vec3(BE1::MeterToUnit(0.5f)));

    // Create default sphere mesh
    defaultSphereMesh = AllocMesh("_defaultSphereMesh");
    defaultSphereMesh->flags |= Mesh::Flag::Permanent;
    defaultSphereMesh->CreateSphere(BE1::Vec3::origin, BE1::Mat3::identity, BE1::MeterToUnit(0.5f), 32);

    // Create default cylinder mesh
    defaultCylinderMesh = AllocMesh("_defaultCylinderMesh");
    defaultCylinderMesh->flags |= Mesh::Flag::Permanent;
    defaultCylinderMesh->CreateCylinder(BE1::Vec3::origin, BE1::Mat3::identity, BE1::MeterToUnit(0.5f), BE1::MeterToUnit(2.0f), 32);

    // Create default capsule mesh
    defaultCapsuleMesh = AllocMesh("_defaultCapsuleMesh");
    defaultCapsuleMesh->flags |= Mesh::Flag::Permanent;
    defaultCapsuleMesh->CreateCapsule(BE1::Vec3::origin, BE1::Mat3::identity, BE1::MeterToUnit(0.5f), BE1::MeterToUnit(1.0f), 32);
}

void MeshManager::Shutdown() {
    for (const auto &entry : meshHashMap) {
        Mesh *mesh = entry.second;
        delete mesh;
    }
    meshHashMap.Clear();
}

Mesh *MeshManager::AllocMesh(const char *hashName) {
    if (meshHashMap.Get(hashName)) {
        BE_FATALERROR("%s mesh already allocated", hashName);
    }

    Mesh *mesh = new Mesh;
    mesh->refCount = 1;
    mesh->hashName = hashName;
    mesh->name = hashName;
    mesh->name.StripPath();
    mesh->name.StripFileExtension();
    meshHashMap.Set(mesh->hashName, mesh);

    return mesh;
}

Mesh *MeshManager::FindMesh(const char *hashName) const {
    const auto *entry = meshHashMap.Get(BE1::Str(hashName));
    if (entry) {
        return entry->second;
    }
    return nullptr;
}

Mesh *MeshManager::GetMesh(const char *hashName) {
    if (!hashName || !hashName[0]) {
        return defaultMesh;
    }

    Mesh *mesh = FindMesh(hashName);
    if (mesh) {
        mesh->refCount++;
        return mesh;
    }

    mesh = AllocMesh(hashName);
    if (!mesh->Load(hashName)) {
        BE_WARNLOG("Couldn't load mesh '%s'\n", hashName);
        DestroyMesh(mesh);
        return defaultMesh;
    }
    return mesh;
}

void MeshManager::ReleaseMesh(Mesh *mesh, bool immediateDestroy) {
    if (mesh->refCount > 0) {
        if (--mesh->refCount > 0) {
            return;
        }
    }

    if (BE1::HasFlag(mesh->flags, Mesh::Flag::Permanent)) {
        return;
    }

    if (immediateDestroy) {
        DestroyMesh(mesh);
    }
}

void MeshManager::DestroyMesh(Mesh *mesh) {
    if (mesh->refCount > 1) {
        BE_WARNLOG("MeshManager::DestroyMesh: mesh '%s' has %i reference count\n", mesh->name.c_str(), mesh->refCount);
    }
    meshHashMap.Remove(mesh->hashName);

    delete mesh;
}

void MeshManager::DestroyUnusedMeshes() {
    BE1::Array<Mesh *> removeArray;

    for (const auto &entry : meshHashMap) {
        Mesh *mesh = entry.second;
        if (!mesh) {
            continue;
        }

        if (!BE1::HasFlag(mesh->flags, Mesh::Flag::Permanent) && mesh->refCount == 0) {
            removeArray.Append(mesh);
        }
    }

    for (Mesh *mesh : removeArray) {
        DestroyMesh(mesh);
    }
}

Mesh *MeshManager::CreateCombinedMesh(const char *hashName, const BE1::Array<BatchSubMesh> &batchSubMeshes) {
    // Counts total verts/indices for combined mesh.
    int numTotalVerts = 0;
    int numTotalIndexes = 0;

    for (int subMeshIndex = 0; subMeshIndex < batchSubMeshes.Count(); subMeshIndex++) {
        const SubMesh *subMesh = batchSubMeshes[subMeshIndex].subMesh;

        numTotalVerts += subMesh->NumVerts();
        numTotalIndexes += subMesh->NumIndexes();
    }

    // Allocates a combined mesh.
    Mesh *mesh = AllocMesh(hashName);
    mesh->type = MeshType::Static;
    Mesh::Surface *surf = mesh->AllocSurface(numTotalVerts, numTotalIndexes);
    mesh->surfaces.Append(surf);

    BE1::VertexGenericLit *dstVertPtr = surf->subMesh->verts;
    BE1::VertIndex *dstIndexPtr = surf->subMesh->indexes;
    int baseVertex = 0;

    for (int subMeshIndex = 0; subMeshIndex < batchSubMeshes.Count(); subMeshIndex++) {
        const SubMesh *srcSubMesh = batchSubMeshes[subMeshIndex].subMesh;

        for (int i = 0; i < srcSubMesh->numVerts; i++) {
            *dstVertPtr = srcSubMesh->verts[i];

            dstVertPtr->Transform(batchSubMeshes[subMeshIndex].localTransform);
            dstVertPtr++;
        }

        for (int i = 0; i < srcSubMesh->numIndexes; i++) {
            *dstIndexPtr = srcSubMesh->indexes[i] + baseVertex;
            dstIndexPtr++;
        }

        baseVertex += srcSubMesh->numVerts;
    }

    mesh->FinishSurfaces(Mesh::FinishFlag::ComputeAABB);

    return mesh;
}
