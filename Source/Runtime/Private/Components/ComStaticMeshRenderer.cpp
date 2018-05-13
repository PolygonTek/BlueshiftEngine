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
#include "Components/ComSkinnedMeshRenderer.h"
#include "Components/ComStaticMeshRenderer.h"
#include "StaticBatching/StaticBatch.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Static Mesh Renderer", ComStaticMeshRenderer, ComMeshRenderer)
BEGIN_EVENTS(ComStaticMeshRenderer)
END_EVENTS

void ComStaticMeshRenderer::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("occluder", "Occluder", bool, IsOccluder, SetOccluder, false, 
        "", PropertyInfo::EditorFlag);
}

ComStaticMeshRenderer::ComStaticMeshRenderer() {
}

ComStaticMeshRenderer::~ComStaticMeshRenderer() {
    Purge(false);
}

bool ComStaticMeshRenderer::IsConflictComponent(const MetaObject &componentClass) const {
    if (componentClass.IsTypeOf(ComSkinnedMeshRenderer::metaObject)) {
        return true;
    }
    return false;
}

void ComStaticMeshRenderer::Purge(bool chainPurge) {
    if (chainPurge) {
        ComMeshRenderer::Purge();
    }
}

void ComStaticMeshRenderer::Init() {
    ComMeshRenderer::Init();

    renderObjectDef.mesh = referenceMesh->InstantiateMesh(Mesh::StaticMesh);

    // Mark as initialized
    SetInitialized(true);

    UpdateVisuals();
}

void ComStaticMeshRenderer::Update() {
}

void ComStaticMeshRenderer::MeshUpdated() {
    if (!IsInitialized()) {
        return;
    }

    renderWorld->RemoveRenderObject(renderObjectHandle);
    renderObjectHandle = -1;

    renderObjectDef.mesh = referenceMesh->InstantiateMesh(Mesh::StaticMesh);
    renderObjectDef.localAABB = referenceMesh->GetAABB();

    UpdateVisuals();
}

bool ComStaticMeshRenderer::IsOccluder() const {
    return !!(renderObjectDef.flags & RenderObject::OccluderFlag);
}

void ComStaticMeshRenderer::SetOccluder(bool occluder) {
    if (occluder) {
        renderObjectDef.flags |= RenderObject::OccluderFlag;
    } else {
        renderObjectDef.flags &= ~RenderObject::OccluderFlag;
    }
    UpdateVisuals();
}

BE_NAMESPACE_END
