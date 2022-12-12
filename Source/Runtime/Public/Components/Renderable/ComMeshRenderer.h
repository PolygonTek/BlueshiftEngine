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

#include "ComRenderable.h"

BE_NAMESPACE_BEGIN

class Asset;
class Material;
class MeshCombiner;

class ComMeshRenderer : public ComRenderable {
    friend class MeshCombiner;

public:
    ABSTRACT_PROTOTYPE(ComMeshRenderer);

    ComMeshRenderer();
    virtual ~ComMeshRenderer() = 0;

    virtual void            Purge(bool chainPurge = true) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

    void                    ReinstantiateMesh();

    const Mesh *            GetReferenceMesh() const { return referenceMesh; }

    bool                    GetClosestVertex(const RenderCamera *view, const Point &mousePixelLocation, Vec3 &closestVertex, float &closestDistance) const;

    Guid                    GetMeshGuid() const;
    void                    SetMeshGuid(const Guid &guid);

    Mesh *                  GetMesh() const;
    void                    SetMesh(const Mesh *mesh);

    int                     GetMaterialCount() const;
    void                    SetMaterialCount(int count);
    Guid                    GetMaterialGuid(int index) const;
    void                    SetMaterialGuid(int index, const Guid &materialGuid);

    Material *              GetMaterial(int index) const;
    void                    SetMaterial(int index, const Material *material);

    bool                    IsUseEnvProbeLighting() const;
    void                    SetUseEnvProbeLighting(bool useEnvProbeLighting);

    bool                    IsCastShadows() const;
    void                    SetCastShadows(bool castShadows);

    bool                    IsReceiveShadows() const;
    void                    SetReceiveShadows(bool receiveShadows);

protected:
    void                    ChangeMesh(const Guid &meshGuid);

    virtual void            InstantiateMesh() = 0;

    virtual void            MeshUpdated() = 0;

    void                    MeshReloaded();

    Asset *                 meshAsset = nullptr;
    Mesh *                  referenceMesh = nullptr;    // shared mesh to retain shared information for instantiated mesh
};

BE_NAMESPACE_END
