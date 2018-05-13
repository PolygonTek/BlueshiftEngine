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
    REGISTER_MIXED_ACCESSOR_PROPERTY("mesh", "Mesh", Guid, GetMeshGuid, SetMeshGuid, GuidMapper::defaultMeshGuid, 
        "", PropertyInfo::EditorFlag).SetMetaObject(&MeshAsset::metaObject);
    REGISTER_MIXED_ACCESSOR_ARRAY_PROPERTY("materials", "Materials", Guid, GetMaterialGuid, SetMaterialGuid, GetMaterialCount, SetMaterialCount, GuidMapper::defaultMaterialGuid, 
        "List of materials to use when rendering.", PropertyInfo::EditorFlag).SetMetaObject(&MaterialAsset::metaObject);
    REGISTER_ACCESSOR_PROPERTY("useLightProve", "Use Light Probe", bool, IsUseLightProbe, SetUseLightProbe, true, 
        "", PropertyInfo::EditorFlag);
    //REGISTER_ACCESSOR_PROPERTY("useReflectionProbe", "Use Reflection Probe", bool, IsUseReflectionProbe, SetUseReflectionProbe, false,
        //"", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("castShadows", "Cast Shadows", bool, IsCastShadows, SetCastShadows, true, 
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("receiveShadows", "Receive Shadows", bool, IsReceiveShadows, SetReceiveShadows, true, 
        "", PropertyInfo::EditorFlag);
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

    renderObjectDef.localAABB = referenceMesh->GetAABB();

    // Mark as initialized
    SetInitialized(true);
}

void ComMeshRenderer::ChangeMesh(const Guid &meshGuid) {
#if 1
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

#if 1
    // Need mesh asset to be reloaded in editor
    meshAsset = (MeshAsset *)MeshAsset::FindInstance(meshGuid);
    if (meshAsset) {
        meshAsset->Connect(&Asset::SIG_Reloaded, this, (SignalCallback)&ComMeshRenderer::MeshReloaded, SignalObject::Queued);
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
    
    const Str materialPath = resourceGuidMapper.Get(materialGuid);
    return materialManager.GetMaterial(materialPath); // FIXME: release ?
}

void ComMeshRenderer::SetMaterial(int index, const Material *material) {
    const Guid materialGuid = resourceGuidMapper.Get(material->GetHashName());

    SetMaterialGuid(index, materialGuid);
}

bool ComMeshRenderer::IsUseLightProbe() const {
    return !!(renderObjectDef.flags & RenderObject::UseLightProbeFlag);
}

void ComMeshRenderer::SetUseLightProbe(bool useLightProbe) {
    if (useLightProbe) {
        renderObjectDef.flags |= RenderObject::UseLightProbeFlag;
    } else {
        renderObjectDef.flags &= ~RenderObject::UseLightProbeFlag;
    }

    UpdateVisuals();
}

bool ComMeshRenderer::IsCastShadows() const {
    return !!(renderObjectDef.flags & RenderObject::CastShadowsFlag);
}

void ComMeshRenderer::SetCastShadows(bool castShadows) {
    if (castShadows) {
        renderObjectDef.flags |= RenderObject::CastShadowsFlag;
    } else {
        renderObjectDef.flags &= ~RenderObject::CastShadowsFlag;
    }
    
    UpdateVisuals();
}

bool ComMeshRenderer::IsReceiveShadows() const {
    return !!(renderObjectDef.flags & RenderObject::ReceiveShadowsFlag);
}

void ComMeshRenderer::SetReceiveShadows(bool receiveShadows) {
    if (receiveShadows) {
        renderObjectDef.flags |= RenderObject::ReceiveShadowsFlag;
    } else {
        renderObjectDef.flags &= ~RenderObject::ReceiveShadowsFlag;
    }

    UpdateVisuals();
}

bool ComMeshRenderer::GetClosestVertex(const RenderView *view, const Point &mousePixelLocation, Vec3 &closestVertex, float &closestDistance) const {
    const float initialClosestDistance = closestDistance;

    const ComTransform *transform = GetEntity()->GetTransform();

    for (int surfaceIndex = 0; surfaceIndex < referenceMesh->NumSurfaces(); surfaceIndex++) {
        const SubMesh *subMesh = referenceMesh->GetSurface(surfaceIndex)->subMesh;
        const VertexGenericLit *v = subMesh->Verts();

        for (int vertexIndex = 0; vertexIndex < subMesh->NumVerts(); vertexIndex++, v++) {
            Vec3 localPosition = v->GetPosition();
            Vec3 localNormal = v->GetNormal();

            Vec3 worldPosition = transform->GetMatrix() * localPosition;
            Vec3 worldNormal = transform->GetMatrix().TransformNormal(localNormal);

            bool isBackface;
            // Ignore backface vertices 
            if (!view->state.orthogonal && !worldNormal.IsZero() && worldNormal.Dot(view->state.origin - worldPosition) < 0) {
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
