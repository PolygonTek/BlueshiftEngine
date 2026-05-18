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

    expiredMeshHashMapCursor = 0;

    CreateEngineMeshes();
}

void MeshManager::Shutdown() {
    DestroyEngineMeshes();

    meshHashMap.Clear();
}

void MeshManager::UpdateFrame() {
    CleanExpiredMeshes();
}

void MeshManager::CreateEngineMeshes() {
    // Create default (box shape) mesh
    defaultMesh = AllocMesh("_defaultMesh");
    defaultMesh->CreateDefaultBox();

    // Create default quad mesh
    defaultQuadMesh = AllocMesh("_defaultQuadMesh");
    defaultQuadMesh->CreatePlane(BE1::Vec3::origin, BE1::Angles(0, 90, 0).ToMat3(), BE1::MeterToUnit(1.0f), 1);

    // Create default plane mesh
    defaultPlaneMesh = AllocMesh("_defaultPlaneMesh");
    defaultPlaneMesh->CreatePlane(BE1::Vec3::origin, BE1::Mat3::identity, BE1::MeterToUnit(10.0f), 10);

    // Create default box mesh
    defaultBoxMesh = AllocMesh("_defaultBoxMesh");
    defaultBoxMesh->CreateBox(BE1::Vec3::origin, BE1::Mat3::identity, BE1::Vec3(BE1::MeterToUnit(0.5f)));

    // Create default sphere mesh
    defaultSphereMesh = AllocMesh("_defaultSphereMesh");
    defaultSphereMesh->CreateSphere(BE1::Vec3::origin, BE1::Mat3::identity, BE1::MeterToUnit(0.5f), 32);

    // Create default cylinder mesh
    defaultCylinderMesh = AllocMesh("_defaultCylinderMesh");
    defaultCylinderMesh->CreateCylinder(BE1::Vec3::origin, BE1::Mat3::identity, BE1::MeterToUnit(0.5f), BE1::MeterToUnit(2.0f), 32);

    // Create default capsule mesh
    defaultCapsuleMesh = AllocMesh("_defaultCapsuleMesh");
    defaultCapsuleMesh->CreateCapsule(BE1::Vec3::origin, BE1::Mat3::identity, BE1::MeterToUnit(0.5f), BE1::MeterToUnit(1.0f), 32);
}

void MeshManager::DestroyEngineMeshes() {
    if (defaultMesh) {
        defaultMesh.reset();
    }
    if (defaultQuadMesh) {
        defaultQuadMesh.reset();
    }
    if (defaultPlaneMesh) {
        defaultPlaneMesh.reset();
    }
    if (defaultBoxMesh) {
        defaultBoxMesh.reset();
    }
    if (defaultSphereMesh) {
        defaultSphereMesh.reset();
    }
    if (defaultCylinderMesh) {
        defaultCylinderMesh.reset();
    }
    if (defaultCapsuleMesh) {
        defaultCapsuleMesh.reset();
    }
}

std::shared_ptr<Mesh> MeshManager::AllocMesh(const char *hashName) {
    if (!hashName || !hashName[0]) {
        BE_FATALERROR("MeshManager::AllocMesh: empty mesh name");
    }
    if (const auto *entry = meshHashMap.Get(hashName)) {
        if (!entry->second.expired()) {
            BE_FATALERROR("MeshManager::AllocMesh: %s mesh already allocated", hashName);
        }
        meshHashMap.Remove(hashName);
    }

    std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
    mesh->hashName = hashName;
    mesh->name = hashName;
    mesh->name.StripPath();
    mesh->name.StripFileExtension();
    meshHashMap.Set(mesh->hashName, mesh);

    return mesh;
}

std::shared_ptr<Mesh> MeshManager::FindMesh(const char *hashName) const {
    if (!hashName || !hashName[0]) {
        return nullptr;
    }

    const auto *entry = meshHashMap.Get(hashName);
    if (!entry) {
        return nullptr;
    }

    return entry->second.lock();
}

std::shared_ptr<Mesh> MeshManager::GetMesh(const char *hashName) {
    if (!hashName || !hashName[0]) {
        return defaultMesh;
    }

    std::shared_ptr<Mesh> mesh = FindMesh(hashName);
    if (mesh) {
        return mesh;
    }

    mesh = AllocMesh(hashName);
/*#ifdef USE_ASYNC_LOADING
    if (!mesh->LoadAsync(hashName)) {
#else*/
    if (!mesh->Load(hashName)) {
//#endif
        BE_WARNLOG("Couldn't load mesh '%s'\n", hashName);

        meshHashMap.Remove(mesh->hashName);
        return defaultMesh;
    }
    return mesh;
}

std::shared_ptr<Mesh> MeshManager::CreateCombinedMesh(const char *hashName, const BE1::Array<BatchSubMesh> &batchSubMeshes) {
    // Counts total verts/indices for combined mesh.
    int numTotalVerts = 0;
    int numTotalIndexes = 0;

    for (int subMeshIndex = 0; subMeshIndex < batchSubMeshes.Count(); subMeshIndex++) {
        const SubMesh *subMesh = batchSubMeshes[subMeshIndex].subMesh;

        numTotalVerts += subMesh->NumVerts();
        numTotalIndexes += subMesh->NumIndexes();
    }

    // Allocates a combined mesh.
    std::shared_ptr<Mesh> mesh = AllocMesh(hashName);
    mesh->type = MeshType::Static;
    Mesh::Surface *surf = mesh->AllocSurface(numTotalVerts, numTotalIndexes);
    mesh->surfaces.Append(surf);

    BE1::VertexGenericLit *dstVertPtr = surf->subMesh->verts;
    BE1::VertIndex *dstIndexPtr = surf->subMesh->indexes;
    int baseVertex = 0;

    for (int subMeshIndex = 0; subMeshIndex < batchSubMeshes.Count(); subMeshIndex++) {
        const SubMesh *srcSubMesh = batchSubMeshes[subMeshIndex].subMesh;

        for (int i = 0; i < srcSubMesh->numVerts; ++i) {
            *dstVertPtr = srcSubMesh->verts[i];

            dstVertPtr->Transform(batchSubMeshes[subMeshIndex].localTransform);
            dstVertPtr++;
        }

        for (int i = 0; i < srcSubMesh->numIndexes; ++i) {
            *dstIndexPtr = srcSubMesh->indexes[i] + baseVertex;
            dstIndexPtr++;
        }

        baseVertex += srcSubMesh->numVerts;
    }

    mesh->FinishSurfaces(Mesh::FinishFlag::ComputeAABB);

    return mesh;
}

void MeshManager::CleanExpiredMeshes() {
    constexpr int ExpiredMeshHashMapScanBudget = 32;
    int scansLeft = ExpiredMeshHashMapScanBudget;

    while (scansLeft-- > 0 && meshHashMap.Count() > 0) {
        if (expiredMeshHashMapCursor >= meshHashMap.Count()) {
            expiredMeshHashMapCursor = 0;
        }

        const auto *entry = meshHashMap.GetByIndex(expiredMeshHashMapCursor);
        assert(entry);

        if (entry->second.expired()) {
            meshHashMap.Remove(entry->first);
            continue;
        }

        expiredMeshHashMapCursor++;
    }
}
