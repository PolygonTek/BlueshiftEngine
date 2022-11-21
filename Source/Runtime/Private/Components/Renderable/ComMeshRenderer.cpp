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
#include "Components/Transform/ComTransform.h"
#include "Components/Renderable/ComMeshRenderer.h"
#include "Game/Entity.h"
#include "Asset/Asset.h"
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

ABSTRACT_DECLARATION("Mesh Renderer", ComMeshRenderer, ComRenderable)
BEGIN_EVENTS(ComMeshRenderer)
END_EVENTS

void ComMeshRenderer::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("mesh", "Mesh", Guid, GetMeshGuid, SetMeshGuid, GuidMapper::defaultMeshGuid, 
        "", PropertyInfo::Flag::Editor).SetMetaObject(&MeshResource::metaObject);
    REGISTER_MIXED_ACCESSOR_ARRAY_PROPERTY("materials", "Materials", Guid, GetMaterialGuid, SetMaterialGuid, GetMaterialCount, SetMaterialCount, GuidMapper::defaultMaterialGuid, 
        "List of materials to use when rendering.", PropertyInfo::Flag::Editor).SetMetaObject(&MaterialResource::metaObject);
    REGISTER_ACCESSOR_PROPERTY("useEnvironmentProveLighting", "Use Env Probe Lighting", bool, IsUseEnvProbeLighting, SetUseEnvProbeLighting, true, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("castShadows", "Cast Shadows", bool, IsCastShadows, SetCastShadows, true, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("receiveShadows", "Receive Shadows", bool, IsReceiveShadows, SetReceiveShadows, true, 
        "", PropertyInfo::Flag::Editor);
}

ComMeshRenderer::ComMeshRenderer() {
    meshAsset = nullptr;
    referenceMesh = nullptr;
}

ComMeshRenderer::~ComMeshRenderer() {
    Purge(false);
}

void ComMeshRenderer::Purge(bool chainPurge) {
    // Release instantiated mesh
    if (renderObjectDef.mesh) {
        meshManager.ReleaseMesh(renderObjectDef.mesh);
        renderObjectDef.mesh = nullptr;
    }

    // Release reference mesh
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

    if (referenceMesh) {
        renderObjectDef.aabb = referenceMesh->GetAABB();
    }

    // Mark as initialized
    SetInitialized(true);
}

void ComMeshRenderer::ChangeMesh(const Guid &meshGuid) {
#if WITH_EDITOR
    // Disconnect with previously connected mesh asset
    if (meshAsset) {
        meshAsset->Disconnect(&Asset::SIG_Reloaded, this);
        meshAsset = nullptr;
    }
#endif

    // Release the previously used instantiated mesh
    if (renderObjectDef.mesh) {
        meshManager.ReleaseMesh(renderObjectDef.mesh);
        renderObjectDef.mesh = nullptr;
    }

    // Release the previously used reference mesh
    if (referenceMesh) {
        meshManager.ReleaseMesh(referenceMesh);
        referenceMesh = nullptr;
    }

    // Release previously used materials
    for (int i = 0; i < renderObjectDef.materials.Count(); i++) {
        materialManager.ReleaseMaterial(renderObjectDef.materials[i]);
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
    int oldCount = renderObjectDef.materials.Count();

    // Resize material slots
    renderObjectDef.materials.SetCount(numMaterials);

    // Set default materials to material slots
    for (int i = oldCount; i < renderObjectDef.materials.Count(); i++) {
        renderObjectDef.materials[i] = materialManager.GetMaterial("_defaultMaterial");
    }

#if WITH_EDITOR
    // Need mesh asset to be reloaded in editor
    meshAsset = (Asset *)Asset::FindInstance(meshGuid);
    if (meshAsset) {
        meshAsset->Connect(&Asset::SIG_Reloaded, this, (SignalCallback)&ComMeshRenderer::MeshReloaded, SignalObject::ConnectionType::Queued);
    }
#endif
}

void ComMeshRenderer::MeshReloaded() {
    ChangeMesh(GetMeshGuid());

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

Mesh *ComMeshRenderer::GetMesh() const {
    Guid meshGuid = GetMeshGuid();
    if (meshGuid.IsZero()) {
        return nullptr;
    }

    const Str meshHashName = resourceGuidMapper.Get(meshGuid);
    return meshManager.GetMesh(meshHashName);
}

void ComMeshRenderer::SetMesh(const Mesh *mesh) {
    const Guid meshGuid = resourceGuidMapper.Get(mesh->GetHashName());

    SetMeshGuid(meshGuid);
}

int ComMeshRenderer::GetMaterialCount() const {
    return renderObjectDef.materials.Count();
}

void ComMeshRenderer::SetMaterialCount(int count) {
    int oldCount = renderObjectDef.materials.Count();

    renderObjectDef.materials.SetCount(count);

    // Set default materials to appended material slots
    if (count > oldCount) {
        for (int index = oldCount; index < count; index++) {
            renderObjectDef.materials[index] = materialManager.GetMaterial("_defaultMaterial");
        }
    }
}

Guid ComMeshRenderer::GetMaterialGuid(int index) const {
    if (index >= 0 && index < renderObjectDef.materials.Count()) {
        const Str materialPath = renderObjectDef.materials[index]->GetHashName();
        return resourceGuidMapper.Get(materialPath);
    }
    return Guid();
}

void ComMeshRenderer::SetMaterialGuid(int index, const Guid &materialGuid) {
    if (index >= 0 && index < renderObjectDef.materials.Count()) {
        // Release the previously used material
        if (renderObjectDef.materials[index]) {
            materialManager.ReleaseMaterial(renderObjectDef.materials[index]);
        }

        // Get the new material
        const Str materialPath = resourceGuidMapper.Get(materialGuid);
        renderObjectDef.materials[index] = materialManager.GetMaterial(materialPath);
    }

    UpdateVisuals();
}

Material *ComMeshRenderer::GetMaterial(int index) const {
    Guid materialGuid = GetMaterialGuid(index);
    if (materialGuid.IsZero()) {
        return nullptr;
    }
    
    const Str materialHashName = resourceGuidMapper.Get(materialGuid);
    return materialManager.GetMaterial(materialHashName); // FIXME: release ?
}

void ComMeshRenderer::SetMaterial(int index, const Material *material) {
    const Guid materialGuid = resourceGuidMapper.Get(material->GetHashName());

    SetMaterialGuid(index, materialGuid);
}

bool ComMeshRenderer::IsUseEnvProbeLighting() const {
    return !!(renderObjectDef.flags & RenderObject::Flag::EnvProbeLit);
}

void ComMeshRenderer::SetUseEnvProbeLighting(bool useEnvProbe) {
    if (useEnvProbe) {
        renderObjectDef.flags |= RenderObject::Flag::EnvProbeLit;
    } else {
        renderObjectDef.flags &= ~RenderObject::Flag::EnvProbeLit;
    }

    UpdateVisuals();
}

bool ComMeshRenderer::IsCastShadows() const {
    return !!(renderObjectDef.flags & RenderObject::Flag::CastShadows);
}

void ComMeshRenderer::SetCastShadows(bool castShadows) {
    if (castShadows) {
        renderObjectDef.flags |= RenderObject::Flag::CastShadows;
    } else {
        renderObjectDef.flags &= ~RenderObject::Flag::CastShadows;
    }
    
    UpdateVisuals();
}

bool ComMeshRenderer::IsReceiveShadows() const {
    return !!(renderObjectDef.flags & RenderObject::Flag::ReceiveShadows);
}

void ComMeshRenderer::SetReceiveShadows(bool receiveShadows) {
    if (receiveShadows) {
        renderObjectDef.flags |= RenderObject::Flag::ReceiveShadows;
    } else {
        renderObjectDef.flags &= ~RenderObject::Flag::ReceiveShadows;
    }

    UpdateVisuals();
}

bool ComMeshRenderer::GetClosestVertex(const RenderCamera *camera, const Point &mousePixelLocation, Vec3 &closestVertex, float &closestDistance) const {
    const float initialClosestDistance = closestDistance;

    const ComTransform *transform = GetEntity()->GetTransform();

    for (int surfaceIndex = 0; surfaceIndex < referenceMesh->NumSurfaces(); surfaceIndex++) {
        const SubMesh *subMesh = referenceMesh->GetSurface(surfaceIndex)->subMesh;
        const VertexGenericLit *v = subMesh->Verts();

        for (int vertexIndex = 0; vertexIndex < subMesh->NumVerts(); vertexIndex++, v++) {
            Vec3 localPosition = v->GetPosition();
            Vec3 localNormal = v->GetNormal();

            Vec3 worldPosition = transform->GetWorldMatrix().TransformPos(localPosition);
            Vec3 worldNormal = transform->GetWorldMatrix().TransformDir(localNormal);

            bool isBackface;
            // Ignore backface vertices 
            if (!camera->GetState().orthogonal && !worldNormal.IsZero() && worldNormal.Dot(camera->GetState().origin - worldPosition) < 0) {
                isBackface = true;
            } else {
                isBackface = false;
            }

            if (!isBackface) {
                PointF pixelLocation;

                if (camera->TransformWorldToPixel(worldPosition, pixelLocation)) {
                    float dist = pixelLocation.ToPoint().DistanceSqr(mousePixelLocation);

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
