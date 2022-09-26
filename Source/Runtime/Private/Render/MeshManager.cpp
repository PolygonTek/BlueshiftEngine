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
#include "Render/Render.h"
#include "Core/Cmds.h"

BE_NAMESPACE_BEGIN
    
Mesh *          MeshManager::defaultMesh;
Mesh *          MeshManager::defaultQuadMesh;
Mesh *          MeshManager::defaultPlaneMesh;
Mesh *          MeshManager::defaultBoxMesh;
Mesh *          MeshManager::defaultSphereMesh;
Mesh *          MeshManager::defaultGeoSphereMesh;
Mesh *          MeshManager::defaultCylinderMesh;
Mesh *          MeshManager::defaultCapsuleMesh;
Mesh *          MeshManager::defaultRoundedBoxMesh;
//Mesh *          MeshManager::testSlicedMesh;

MeshManager     meshManager;

void MeshManager::Init() {
    cmdSystem.AddCommand("listMeshes", Cmd_ListMeshes);
    cmdSystem.AddCommand("reloadMesh", Cmd_ReloadMesh);

    meshHashMap.Init(1024, 64, 64);
    instantiatedMeshList.Resize(64, 64);

    CreateEngineMeshes();
}

void MeshManager::CreateEngineMeshes() {
    // Create default (box shape) mesh
    defaultMesh = AllocMesh("_defaultMesh");
    defaultMesh->permanence = true;
    defaultMesh->CreateDefaultBox();

    // Create default quad mesh
    defaultQuadMesh = AllocMesh("_defaultQuadMesh");
    defaultQuadMesh->permanence = true;
    defaultQuadMesh->CreatePlane(Vec3::origin, Angles(0, 90, 0).ToMat3(), MeterToUnit(1.0f), 1);

    // Create default plane mesh
    defaultPlaneMesh = AllocMesh("_defaultPlaneMesh");
    defaultPlaneMesh->permanence = true;
    defaultPlaneMesh->CreatePlane(Vec3::origin, Mat3::identity, MeterToUnit(10.0f), 10);

    // Create default box mesh
    defaultBoxMesh = AllocMesh("_defaultBoxMesh");
    defaultBoxMesh->permanence = true;
    defaultBoxMesh->CreateBox(Vec3::origin, Mat3::identity, Vec3(MeterToUnit(0.5f)));

    // Create default sphere mesh
    defaultSphereMesh = AllocMesh("_defaultSphereMesh");
    defaultSphereMesh->permanence = true;
    defaultSphereMesh->CreateSphere(Vec3::origin, Mat3::identity, MeterToUnit(0.5f), 32);

    // Create default geosphere mesh
    defaultGeoSphereMesh = AllocMesh("_defaultGeoSphereMesh");
    defaultGeoSphereMesh->permanence = true;
    defaultGeoSphereMesh->CreateGeosphere(Vec3::origin, MeterToUnit(0.5f), 3);

    // Create default cone mesh
    //defaultConeMesh = AllocMesh("_defaultConeMesh");
    //defaultConeMesh->permanence = true;
    //defaultConeMesh->CreateCone(Vec3::origin, Mat3::identity, MeterToUnit(0.5), MeterToUnit(2), 32);

    // Create default cylinder mesh
    defaultCylinderMesh = AllocMesh("_defaultCylinderMesh");
    defaultCylinderMesh->permanence = true;
    defaultCylinderMesh->CreateCylinder(Vec3::origin, Mat3::identity, MeterToUnit(0.5f), MeterToUnit(2.0f), 32);

    // Create default capsule mesh
    defaultCapsuleMesh = AllocMesh("_defaultCapsuleMesh");
    defaultCapsuleMesh->permanence = true;
    defaultCapsuleMesh->CreateCapsule(Vec3::origin, Mat3::identity, MeterToUnit(0.5f), MeterToUnit(1.0f), 32);

    // Create default rounded box mesh
    defaultRoundedBoxMesh = AllocMesh("_defaultRoundedBoxMesh");
    defaultRoundedBoxMesh->permanence = true;
    defaultRoundedBoxMesh->CreateRoundedBox(Vec3::origin, Mat3::identity, Vec3(MeterToUnit(0.5f) - MeterToUnit(0.05f)), MeterToUnit(0.05f), 2);

    // ------------- TEST CODE -------------
    /*Quat quat = Quat::FromRotationX(DEG2RAD(30));
    Plane slicePlane = Plane(quat * Vec3(0, 0, 1), MeterToUnit(0.25f));
    //const Str planeString = slicePlane.ToString();
    //const Str slicedMeshName = Str(defaultBoxMesh->GetHashName()) + "-SI-" + planeString;
    testSlicedMesh = meshManager.AllocMesh("_testSlicedMesh");

    if (!Mesh::TrySliceMesh(*defaultRoundedBoxMesh, slicePlane, true, false, testSlicedMesh, nullptr)) {
        meshManager.DestroyMesh(testSlicedMesh);
        testSlicedMesh = nullptr;
    }
    // --------------------------------------
    */
}

void MeshManager::Shutdown() {
    cmdSystem.RemoveCommand("listMeshes");
    cmdSystem.RemoveCommand("reloadMesh");

    for (int i = 0; i < meshHashMap.Count(); i++) {
        const auto *entry = meshManager.meshHashMap.GetByIndex(i);
        Mesh *mesh = entry->second;
        mesh->Purge();
        delete mesh;
    }

    meshHashMap.Clear();

    for (int i = 0; i < instantiatedMeshList.Count(); i++) {
        Mesh *mesh = instantiatedMeshList[i];
        if (mesh) {
            mesh->Purge();
            delete mesh;
        }
    }
    
    instantiatedMeshList.Clear();
}

void MeshManager::DestroyUnusedMeshes() {
    Array<Mesh *> removeArray;

    for (int i = 0; i < meshHashMap.Count(); i++) {
        const auto *entry = meshHashMap.GetByIndex(i);
        Mesh *mesh = entry->second;

        if (mesh && !mesh->permanence && mesh->refCount == 0) {
            removeArray.Append(mesh);
        }
    }

    for (int i = 0; i < removeArray.Count(); i++) {
        DestroyMesh(removeArray[i]);
    }
}

Mesh *MeshManager::AllocMesh(const char *hashName) {
    if (meshHashMap.Get(hashName)) {
        BE_FATALERROR("%s mesh already allocated", hashName);
    }
    
    Mesh *mesh = new Mesh;
    mesh->refCount          = 1;
    mesh->hashName          = hashName;
    mesh->name              = hashName;
    mesh->name.StripPath();
    mesh->name.StripFileExtension();
    meshHashMap.Set(mesh->hashName, mesh);

    return mesh;
}

Mesh *MeshManager::AllocInstantiatedMesh(Mesh *refMesh) {
    Mesh *mesh = new Mesh;
    mesh->refCount          = 1;
    mesh->hashName          = refMesh->hashName;
    mesh->name              = refMesh->name;
    mesh->isInstantiated    = true;
    mesh->originalMesh      = refMesh;
    mesh->useGpuSkinning    = refMesh->useGpuSkinning;
    mesh->numJoints         = refMesh->numJoints;
    mesh->joints            = refMesh->joints;
    mesh->aabb              = refMesh->aabb;
    instantiatedMeshList.Append(mesh);
    
    return mesh;
}

void MeshManager::DestroyMesh(Mesh *mesh) {
    if (mesh->isInstantiated) {
        instantiatedMeshList.RemoveFast(mesh);
    } else {
        if (mesh->refCount > 1) {
            BE_WARNLOG("MeshManager::DestroyMesh: mesh '%s' has %i reference count\n", mesh->name.c_str(), mesh->refCount);
        }

        meshHashMap.Remove(mesh->hashName);
    }

    delete mesh;
}

void MeshManager::ReleaseMesh(Mesh *mesh, bool immediateDestroy) {
    if (mesh->permanence) {
        return;
    }

    if (mesh->isInstantiated) {
        instantiatedMeshList.RemoveFast(mesh);
        delete mesh;
        return;
    }

    if (mesh->refCount > 0) {
        mesh->refCount--;
    }

    if (immediateDestroy && mesh->refCount == 0) {
        DestroyMesh(mesh);
    }
}

void MeshManager::PrecacheMesh(const char *filename) {
    Mesh *mesh = GetMesh(filename);
    ReleaseMesh(mesh);
}

void MeshManager::RenameMesh(Mesh *mesh, const Str &newName) {
    const auto *entry = meshHashMap.Get(mesh->hashName);
    if (entry) {
        meshHashMap.Remove(mesh->hashName);

        mesh->hashName = newName;
        mesh->name = newName;
        mesh->name.StripPath();
        mesh->name.StripFileExtension();

        meshHashMap.Set(newName, mesh);
    }
}

Mesh *MeshManager::FindMesh(const char *hashName) const {
    const auto *entry = meshHashMap.Get(Str(hashName));
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

Mesh *MeshManager::CreateCombinedMesh(const char *hashName, const Array<BatchSubMesh> &batchSubMeshes) {
    // Counts total verts/indices for combined mesh.
    int numTotalVerts = 0;
    int numTotalIndexes = 0;

    for (int subMeshIndex = 0; subMeshIndex < batchSubMeshes.Count(); subMeshIndex++) {
        const SubMesh *subMesh = batchSubMeshes[subMeshIndex].subMesh;

        numTotalVerts += subMesh->NumVerts();
        numTotalIndexes += subMesh->NumIndexes();
    }

    // Allocates a combiend mesh.
    Mesh *mesh = AllocMesh(hashName);
    MeshSurf *surf = mesh->AllocSurface(numTotalVerts, numTotalIndexes);
    mesh->surfaces.Append(surf);

    VertexGenericLit *dstVertPtr = surf->subMesh->verts;
    TriIndex *dstIndexPtr = surf->subMesh->indexes;
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

void MeshManager::EndLevelLoad() {
#if 0
    for (int i = 0; i < meshHashMap.Count(); i++) {
        const auto *entry = meshManager.meshHashMap.GetByIndex(i);
        Mesh *mesh = entry->second;

        mesh->CacheStaticDataToGpu();
    }
#endif
}

void MeshManager::ReinstantiateSkinnedMeshes() {
    for (int i = 0; i < instantiatedMeshList.Count(); i++) {
        Mesh *mesh = instantiatedMeshList[i];

        if (mesh->IsSkinnedMesh()) {
            mesh->Reinstantiate();
        }
    }
}

//--------------------------------------------------------------------------------------------------

void MeshManager::Cmd_ListMeshes(const CmdArgs &args) {
    int count = 0;

    for (int i = 0; i < meshManager.meshHashMap.Count(); i++) {
        const auto *entry = meshManager.meshHashMap.GetByIndex(i);
        Mesh *mesh = entry->second;

        int numVerts = 0;
        int numTris = 0;

        for (int j = 0; j < mesh->NumSurfaces(); j++) {
            numVerts += mesh->surfaces[j]->subMesh->numVerts;
            numTris += mesh->surfaces[j]->subMesh->numIndexes / 3;
        }

        BE_LOG("%3d refs %3d surfs, %6d verts, %6d tris, %3d joints : %s\n",
            mesh->refCount,
            mesh->surfaces.Count(),
            numVerts, numTris,
            mesh->numJoints,
            mesh->hashName.c_str());

        count++;
    }

    BE_LOG("%i total meshes\n", count);
}

void MeshManager::Cmd_ReloadMesh(const CmdArgs &args) {
    if (args.Argc() != 2) {
        BE_LOG("reloadMesh <filename>\n");
        return;
    }

    if (!Str::Icmp(args.Argv(1), "all")) {
        int count = meshManager.meshHashMap.Count();

        for (int i = 0; i < count; i++) {
            const auto *entry = meshManager.meshHashMap.GetByIndex(i);
            Mesh *mesh = entry->second;
            mesh->Reload();
        }
    } else {
        Mesh *mesh = meshManager.FindMesh(args.Argv(1));
        if (!mesh) {
            BE_WARNLOG("Couldn't find mesh to reload \"%s\"\n", args.Argv(1));
            return;
        }

        mesh->Reload();
    }
}

BE_NAMESPACE_END
