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
#include "Components/ComTransform.h"
#include "Components/ComMeshRenderer.h"
#include "Game/Entity.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

ABSTRACT_DECLARATION("Mesh Renderer", ComMeshRenderer, ComRenderable)
BEGIN_EVENTS(ComMeshRenderer)
END_EVENTS

void ComMeshRenderer::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("mesh", "Mesh", Guid, GetMeshGuid, SetMeshGuid, GuidMapper::defaultMeshGuid, "", PropertyInfo::EditorFlag)
        .SetMetaObject(&MeshAsset::metaObject);
    REGISTER_MIXED_ACCESSOR_ARRAY_PROPERTY("materials", "Materials", Guid, GetMaterialGuid, SetMaterialGuid, GetMaterialCount, SetMaterialCount, GuidMapper::defaultMaterialGuid, "List of materials to use when rendering.", PropertyInfo::EditorFlag)
        .SetMetaObject(&MaterialAsset::metaObject);
    REGISTER_ACCESSOR_PROPERTY("useLightProve", "Use Light Probe", bool, IsUseLightProbe, SetUseLightProbe, true, "", PropertyInfo::EditorFlag);
    //REGISTER_ACCESSOR_PROPERTY("useReflectionProbe", "Use Reflection Probe", bool, IsUseReflectionProbe, SetUseReflectionProbe, false, PropertyInfo::EditorFlag),
    REGISTER_ACCESSOR_PROPERTY("castShadows", "Cast Shadows", bool, IsCastShadows, SetCastShadows, true, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("receiveShadows", "Receive Shadows", bool, IsReceiveShadows, SetReceiveShadows, true, "", PropertyInfo::EditorFlag);
}

ComMeshRenderer::ComMeshRenderer() {
    meshAsset = nullptr;
    referenceMesh = nullptr;
}

ComMeshRenderer::~ComMeshRenderer() {
    Purge(false);
}

void ComMeshRenderer::Purge(bool chainPurge) {
    if (sceneEntity.mesh) {
        meshManager.ReleaseMesh(sceneEntity.mesh);
        sceneEntity.mesh = nullptr;
    }

    if (referenceMesh) {
        meshManager.ReleaseMesh(referenceMesh);
        referenceMesh = nullptr;
    }

    if (chainPurge) {
        ComRenderable::Purge();
    }
}

void ComMeshRenderer::Init() {
    ComRenderable::Init();

    sceneEntity.aabb = referenceMesh->GetAABB();

    // Mark as initialized
    SetInitialized(true);
}

void ComMeshRenderer::ChangeMesh(const Guid &meshGuid) {
    // Disconnect with previously connected mesh asset
    if (meshAsset) {
        meshAsset->Disconnect(&Asset::SIG_Reloaded, this);
        meshAsset = nullptr;
    }

    // Release the previously used instantiated mesh
    if (sceneEntity.mesh) {
        meshManager.ReleaseMesh(sceneEntity.mesh);
        sceneEntity.mesh = nullptr;
    }

    // Release the previously used reference mesh
    if (referenceMesh) {
        meshManager.ReleaseMesh(referenceMesh);
        referenceMesh = nullptr;
    }

    // Release previously used materials
    for (int i = 0; i < sceneEntity.materials.Count(); i++) {
        materialManager.ReleaseMaterial(sceneEntity.materials[i]);
    }

    // Get the new reference mesh
    const Str meshPath = resourceGuidMapper.Get(meshGuid);
    referenceMesh = meshManager.GetMesh(meshPath);

    // Get number of materials
    Array<int> materialIndexArray;
    for (int i = 0; i < referenceMesh->NumSurfaces(); i++) {
        materialIndexArray.AddUnique(referenceMesh->GetSurface(i)->materialIndex);
    }
    int numMaterials = materialIndexArray.Count();

    // Get previously used number of materials
    int oldCount = sceneEntity.materials.Count();

    // Resize material slots
    sceneEntity.materials.SetCount(numMaterials);

    for (int i = oldCount; i < sceneEntity.materials.Count(); i++) {
        sceneEntity.materials[i] = materialManager.GetMaterial("_defaultMaterial");
    }

    // Need mesh asset to be reloaded in editor
    meshAsset = (MeshAsset *)MeshAsset::FindInstance(meshGuid);
    if (meshAsset) {
        meshAsset->Connect(&Asset::SIG_Reloaded, this, (SignalCallback)&ComMeshRenderer::MeshReloaded, SignalObject::Queued);
    }
}

void ComMeshRenderer::MeshReloaded() {
    ChangeMesh(GetProperty("mesh").As<Guid>());

    MeshUpdated();
}

Guid ComMeshRenderer::GetMeshGuid() const {
    if (referenceMesh) {
        const Str meshPath = referenceMesh->GetHashName();
        return resourceGuidMapper.Get(meshPath);
    }
    return Guid();
}

void ComMeshRenderer::SetMeshGuid(const Guid &guid) {
    ChangeMesh(guid);

    MeshUpdated();
}

int ComMeshRenderer::GetMaterialCount() const {
    return sceneEntity.materials.Count();
}

void ComMeshRenderer::SetMaterialCount(int count) {
    int oldCount = sceneEntity.materials.Count();

    sceneEntity.materials.SetCount(count);

    if (count > oldCount) {
        for (int index = oldCount; index < count; index++) {
            sceneEntity.materials[index] = materialManager.GetMaterial("_defaultMaterial");
        }
    }
}

Guid ComMeshRenderer::GetMaterialGuid(int index) const {
    if (index >= 0 && index < sceneEntity.materials.Count()) {
        const Str materialPath = sceneEntity.materials[index]->GetHashName();
        return resourceGuidMapper.Get(materialPath);
    }
    return Guid();
}

void ComMeshRenderer::SetMaterialGuid(int index, const Guid &materialGuid) {
    if (index >= 0 && index < sceneEntity.materials.Count()) {
        // Release the previously used material
        if (sceneEntity.materials[index]) {
            materialManager.ReleaseMaterial(sceneEntity.materials[index]);
        }

        // Get the new material
        const Str materialPath = resourceGuidMapper.Get(materialGuid);
        sceneEntity.materials[index] = materialManager.GetMaterial(materialPath);
    }

    UpdateVisuals();
}

Material *ComMeshRenderer::GetMaterial(int index) const {
    Guid materialGuid = GetMaterialGuid(index);
    if (materialGuid.IsZero()) {
        return nullptr;
    }
    
    const Str materialPath = resourceGuidMapper.Get(materialGuid);
    return materialManager.GetMaterial(materialPath); // FIXME: release ?
}

void ComMeshRenderer::SetMaterial(int index, const Material *material) {
    const Guid materialGuid = resourceGuidMapper.Get(material->GetHashName());

    SetMaterialGuid(index, materialGuid);
}

bool ComMeshRenderer::IsUseLightProbe() const {
    return sceneEntity.useLightProbe;
}

void ComMeshRenderer::SetUseLightProbe(bool useLightProbe) {
    sceneEntity.useLightProbe = useLightProbe;

    UpdateVisuals();
}

bool ComMeshRenderer::IsCastShadows() const {
    return sceneEntity.castShadows;
}

void ComMeshRenderer::SetCastShadows(bool castShadows) {
    sceneEntity.castShadows = castShadows;
    
    UpdateVisuals();
}

bool ComMeshRenderer::IsReceiveShadows() const {
    return sceneEntity.receiveShadows;
}

void ComMeshRenderer::SetReceiveShadows(bool receiveShadows) {
    sceneEntity.receiveShadows = receiveShadows;

    UpdateVisuals();
}

bool ComMeshRenderer::GetClosestVertex(const SceneView *view, const Point &mousePixelLocation, Vec3 &closestVertex, float &closestDistance) const {
    const float initialClosestDistance = closestDistance;

    const ComTransform *transform = GetEntity()->GetTransform();

    for (int surfaceIndex = 0; surfaceIndex < sceneEntity.mesh->NumSurfaces(); surfaceIndex++) {
        const SubMesh *subMesh = sceneEntity.mesh->GetSurface(surfaceIndex)->subMesh;
        const VertexGenericLit *v = subMesh->Verts();

        for (int vertexIndex = 0; vertexIndex < subMesh->NumVerts(); vertexIndex++, v++) {
            Vec3 localPosition = v->GetPosition();
            Vec3 localNormal = v->GetNormal();

            Vec3 worldPosition = transform->GetTransform() * localPosition;
            Vec3 worldNormal = transform->GetTransform().TransformNormal(localNormal);

            bool isBackface;
            // Ignore backface vertices 
            if (!view->parms.orthogonal && !worldNormal.IsZero() && worldNormal.Dot(view->parms.origin - worldPosition) < 0) {
                isBackface = true;
            } else {
                isBackface = false;
            }

            if (!isBackface) {
                Point pixelLocation;

                if (view->WorldToPixel(worldPosition, pixelLocation)) {
                    float dist = pixelLocation.DistanceSqr(mousePixelLocation);

                    if (dist < closestDistance) {
                        closestDistance = dist;
                        closestVertex = worldPosition;
                    }
                }
            }
        }
    }

    return initialClosestDistance != closestDistance;
}

BE_NAMESPACE_END
