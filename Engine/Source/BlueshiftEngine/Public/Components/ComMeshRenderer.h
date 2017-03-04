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

class MeshAsset;

class ComMeshRenderer : public ComRenderable {
public:
    ABSTRACT_PROTOTYPE(ComMeshRenderer);

    ComMeshRenderer();
    virtual ~ComMeshRenderer() = 0;

    virtual void            Purge(bool chainPurge = true) override;

    virtual void            Init() override;

    const Mesh *            GetReferenceMesh() const { return referenceMesh; }

    bool                    GetClosestVertex(const SceneView *view, const Point &mousePixelLocation, Vec3 &closestVertex, float &closestDistance) const;

protected:
    void                    ChangeMesh(const Guid &meshGuid);
    void                    ChangeMaterial(const char *materialName);

    virtual void            MeshUpdated() = 0;

    Guid                    GetMesh() const;
    void                    SetMesh(const Guid &guid);

    bool                    IsUseLightProbe() const;
    void                    SetUseLightProbe(bool useLightProbe);

    bool                    IsCastShadows() const;
    void                    SetCastShadows(bool castShadows);

    bool                    IsReceiveShadows() const;
    void                    SetReceiveShadows(bool receiveShadows);

    void                    MeshReloaded();
    void                    PropertyChanged(const char *classname, const char *propName);

    MeshAsset *             meshAsset;
    Mesh *                  referenceMesh;
};

BE_NAMESPACE_END
