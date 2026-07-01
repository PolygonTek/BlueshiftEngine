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

#pragma once

#include "Containers/HashMap.h"
#include "Mesh.h"

struct BatchSubMesh {
    SubMesh *                       subMesh = nullptr;
    ALIGN_AS16 BE1::Mat3x4          localTransform;
};

/*
---------------------------------------------------------------------------------------------------

    Mesh Manager

---------------------------------------------------------------------------------------------------
*/

class MeshManager {
    friend class LoadingSystem;

public:
    void                            Init();
    void                            Shutdown();

    void                            UpdateFrame();

    SharedPtr<Mesh>                 AllocMesh(const char *name);
    SharedPtr<Mesh>                 AllocTransientMesh(const char *name);
    SharedPtr<Mesh>                 FindMesh(const char *name) const;
    SharedPtr<Mesh>                 GetMesh(const char *name, bool async = true);

    SharedPtr<Mesh>                 CreateCombinedMesh(const char *name, const BE1::Array<BatchSubMesh> &batchSubMeshes);

    void                            RenameMesh(Mesh *mesh, const BE1::Str &newName);

    SharedPtr<Mesh>                 defaultMesh;
    SharedPtr<Mesh>                 defaultQuadMesh;
    SharedPtr<Mesh>                 defaultPlaneMesh;
    SharedPtr<Mesh>                 defaultBoxMesh;
    SharedPtr<Mesh>                 defaultSphereMesh;
    SharedPtr<Mesh>                 defaultGeoSphereMesh;
    SharedPtr<Mesh>                 defaultCylinderMesh;
    SharedPtr<Mesh>                 defaultCapsuleMesh;

private:
    void                            CleanExpiredMeshes();
    void                            CreateEngineMeshes();
    void                            DestroyEngineMeshes();

    BE1::StrIHashMap<WeakPtr<Mesh>> meshHashMap;
    int                             expiredMeshHashMapCursor = 0;
};

extern MeshManager                  meshManager;
