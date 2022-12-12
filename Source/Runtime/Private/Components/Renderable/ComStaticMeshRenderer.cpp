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
#include "Components/Renderable/ComSkinnedMeshRenderer.h"
#include "Components/Renderable/ComStaticMeshRenderer.h"
#include "Components/Physics/ComSoftBody.h"
#include "StaticBatching/StaticBatch.h"
#include "Game/Entity.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Static Mesh Renderer", ComStaticMeshRenderer, ComMeshRenderer)
BEGIN_EVENTS(ComStaticMeshRenderer)
END_EVENTS

void ComStaticMeshRenderer::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("occluder", "Occluder", bool, IsOccluder, SetOccluder, false, 
        "", PropertyInfo::Flag::Editor);
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

    if (referenceMesh) {
        InstantiateMesh();
    }

    // Mark as initialized
    SetInitialized(true);

    UpdateVisuals();
}

void ComStaticMeshRenderer::MeshUpdated() {
    if (!IsInitialized()) {
        return;
    }

    renderWorld->RemoveRenderObject(renderObjectHandle);
    renderObjectHandle = -1;

    InstantiateMesh();
    renderObjectDef.aabb = referenceMesh->GetAABB();

    ComSoftBody *softBody = entity->GetComponent<ComSoftBody>();
    if (softBody) {
        softBody->ResetPoints();
    }

    UpdateVisuals();
}

void ComStaticMeshRenderer::InstantiateMesh() {
    const ComSoftBody *softBody = entity->GetComponent<ComSoftBody>();
    Mesh::Type::Enum meshType = softBody ? Mesh::Type::Dynamic : Mesh::Type::Static;

    renderObjectDef.mesh = referenceMesh->InstantiateMesh(meshType);
}

bool ComStaticMeshRenderer::IsOccluder() const {
    return !!(renderObjectDef.flags & RenderObject::Flag::Occluder);
}

void ComStaticMeshRenderer::SetOccluder(bool occluder) {
    if (occluder) {
        renderObjectDef.flags |= RenderObject::Flag::Occluder;
    } else {
        renderObjectDef.flags &= ~RenderObject::Flag::Occluder;
    }
    UpdateVisuals();
}

BE_NAMESPACE_END
