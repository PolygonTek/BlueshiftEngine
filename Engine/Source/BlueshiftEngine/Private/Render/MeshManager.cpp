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
MeshManager     meshManager;

void MeshManager::Init() {
    cmdSystem.AddCommand(L"listMeshes", Cmd_ListMeshes);
    cmdSystem.AddCommand(L"reloadMesh", Cmd_ReloadMesh);

    meshHashMap.Init(1024, 64, 64);
    instantiatedMeshList.Resize(64, 64);

    // Create default (box shape) mesh
    defaultMesh = AllocMesh("_defaultMesh");
    defaultMesh->permanence = true;
    defaultMesh->CreateDefaultBox();

    // Create default quad mesh
    defaultQuadMesh = AllocMesh("_defaultQuadMesh");
    defaultQuadMesh->permanence = true;
    defaultQuadMesh->CreatePlane(Vec3::origin, Angles(0, 90, 0).ToMat3(), MeterToUnit(1), 1);

    // Create default plane mesh
    defaultPlaneMesh = AllocMesh("_defaultPlaneMesh");
    defaultPlaneMesh->permanence = true;
    defaultPlaneMesh->CreatePlane(Vec3::origin, Mat3::identity, MeterToUnit(10), 10);

    // Create default box mesh
    defaultBoxMesh = AllocMesh("_defaultBoxMesh");
    defaultBoxMesh->permanence = true;
    defaultBoxMesh->CreateBox(Vec3::origin, Mat3::identity, Vec3(MeterToUnit(0.5)));

    // Create default sphere mesh
    defaultSphereMesh = AllocMesh("_defaultSphereMesh");
    defaultSphereMesh->permanence = true;
    defaultSphereMesh->CreateSphere(Vec3::origin, Mat3::identity, MeterToUnit(0.5), 32);

    // Create default geosphere mesh
    defaultGeoSphereMesh = AllocMesh("_defaultGeoSphereMesh");
    defaultGeoSphereMesh->permanence = true;
    defaultGeoSphereMesh->CreateGeosphere(Vec3::origin, MeterToUnit(0.5), 3);

    // Create default cylinder mesh
    //defaultConeMesh = AllocMesh("_defaultConeMesh");
    //defaultConeMesh->permanence = true;
    //defaultConeMesh->CreateCone(Vec3::origin, Mat3::identity, MeterToUnit(0.5), MeterToUnit(2), 32);

    // Create default cylinder mesh
    defaultCylinderMesh = AllocMesh("_defaultCylinderMesh");
    defaultCylinderMesh->permanence = true;
    defaultCylinderMesh->CreateCylinder(Vec3::origin, Mat3::identity, MeterToUnit(0.5), MeterToUnit(2), 32);

    // Create default capsule mesh
    defaultCapsuleMesh = AllocMesh("_defaultCapsuleMesh");
    defaultCapsuleMesh->permanence = true;
    defaultCapsuleMesh->CreateCapsule(Vec3::origin, Mat3::identity, MeterToUnit(0.5), MeterToUnit(1), 32);
}

void MeshManager::Shutdown() {
    cmdSystem.RemoveCommand(L"listMeshes");
    cmdSystem.RemoveCommand(L"reloadMesh");

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
        BE_FATALERROR(L"%hs mesh already allocated", hashName);
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
        instantiatedMeshList.Remove(mesh);
        delete mesh;
        return;
    }

    if (mesh->refCount > 1) {
        BE_WARNLOG(L"MeshManager::DestroyMesh: mesh '%hs' has %i reference count\n", mesh->name.c_str(), mesh->refCount);
    }

    meshHashMap.Remove(mesh->hashName);

    delete mesh;
}

void MeshManager::ReleaseMesh(Mesh *mesh, bool immediateDestroy) {
    if (mesh->permanence) {
        return;
    }

    if (mesh->isInstantiated) {
        instantiatedMeshList.Remove(mesh);
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
        BE_WARNLOG(L"Couldn't load mesh '%hs'\n", hashName);
        DestroyMesh(mesh);
        return defaultMesh;
    }

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

        BE_LOG(L"%3d refs %3d surfs, %6d verts, %6d tris, %3d joints : %hs\n", 
            mesh->refCount,
            mesh->surfaces.Count(),
            numVerts, numTris,
            mesh->numJoints,
            mesh->hashName.c_str());

        count++;
    }

    BE_LOG(L"%i total meshes\n", count);
}

void MeshManager::Cmd_ReloadMesh(const CmdArgs &args) {
    if (args.Argc() != 2) {
        BE_LOG(L"reloadMesh <filename>\n");
        return;
    }

    if (!WStr::Icmp(args.Argv(1), L"all")) {
        int count = meshManager.meshHashMap.Count();

        for (int i = 0; i < count; i++) {
            const auto *entry = meshManager.meshHashMap.GetByIndex(i);
            Mesh *mesh = entry->second;
            mesh->Reload();
        }
    } else {
        Mesh *mesh = meshManager.FindMesh(WStr::ToStr(args.Argv(1)));
        if (!mesh) {
            BE_WARNLOG(L"Couldn't find mesh to reload \"%ls\"\n", args.Argv(1));
            return;
        }

        mesh->Reload();
    }
}

BE_NAMESPACE_END
