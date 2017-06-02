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
    PROPERTY_BOOL("occluder", "Occluder", "", "false", PropertySpec::ReadWrite),
END_PROPERTIES

void ComStaticMeshRenderer::RegisterProperties() {
    //REGISTER_ACCESSOR_PROPERTY("Occluder", bool, IsOccluder, SetOccluder, "false", PropertySpec::ReadWrite);
}

ComStaticMeshRenderer::ComStaticMeshRenderer() {
    Connect(&Properties::SIG_PropertyChanged, this, (SignalCallback)&ComStaticMeshRenderer::PropertyChanged);
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

    // Set SceneEntity parameters
    sceneEntity.mesh = referenceMesh->InstantiateMesh(Mesh::StaticMesh);
    sceneEntity.occluder = props->Get("occluder").As<bool>();

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

void ComStaticMeshRenderer::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
        return;
    }

    if (!Str::Cmp(propName, "occluder")) {
        sceneEntity.occluder = props->Get("occluder").As<bool>();
        UpdateVisuals();
        return;
    }

    ComMeshRenderer::PropertyChanged(classname, propName);
}

BE_NAMESPACE_END
