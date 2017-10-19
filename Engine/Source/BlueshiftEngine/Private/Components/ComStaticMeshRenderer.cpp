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

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Static Mesh Renderer", ComStaticMeshRenderer, ComMeshRenderer)
BEGIN_EVENTS(ComStaticMeshRenderer)
END_EVENTS
BEGIN_PROPERTIES(ComStaticMeshRenderer)
    PROPERTY_BOOL("occluder", "Occluder", "", "false", PropertyInfo::ReadWrite),
END_PROPERTIES

void ComStaticMeshRenderer::RegisterProperties() {
#ifdef NEW_PROPERTY_SYSTEM
    REGISTER_ACCESSOR_PROPERTY("Occluder", bool, IsOccluder, SetOccluder, false, "", PropertyInfo::ReadWrite);
#endif
}

ComStaticMeshRenderer::ComStaticMeshRenderer() {
#ifndef NEW_PROPERTY_SYSTEM
    Connect(&Properties::SIG_PropertyChanged, this, (SignalCallback)&ComStaticMeshRenderer::PropertyChanged);
#endif
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

#ifndef NEW_PROPERTY_SYSTEM
    sceneEntity.occluder = props->Get("occluder").As<bool>();
#endif

    sceneEntity.mesh = referenceMesh->InstantiateMesh(Mesh::StaticMesh);

    // Mark as initialized
    SetInitialized(true);

    UpdateVisuals();
}

void ComStaticMeshRenderer::Update() {
}

void ComStaticMeshRenderer::MeshUpdated() {
    sceneEntity.mesh = referenceMesh->InstantiateMesh(Mesh::StaticMesh);
    sceneEntity.aabb = referenceMesh->GetAABB();
    // temp code
    renderWorld->RemoveEntity(sceneEntityHandle);
    sceneEntityHandle = -1;
    // temp code

    UpdateVisuals();
}

bool ComStaticMeshRenderer::IsOccluder() const {
    return sceneEntity.occluder;
}

void ComStaticMeshRenderer::SetOccluder(bool occluder) {
    sceneEntity.occluder = occluder;
    UpdateVisuals();
}

void ComStaticMeshRenderer::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitialized()) {
        return;
    }

    if (!Str::Cmp(propName, "occluder")) {
        SetOccluder(props->Get("occluder").As<bool>());
        return;
    }

    ComMeshRenderer::PropertyChanged(classname, propName);
}

BE_NAMESPACE_END
