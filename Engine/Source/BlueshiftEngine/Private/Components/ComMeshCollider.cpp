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
#include "Physics/Collider.h"
#include "Components/ComTransform.h"
#include "Components/ComMeshCollider.h"
#include "Game/Entity.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Mesh Collider", ComMeshCollider, ComCollider)
BEGIN_EVENTS(ComMeshCollider)
END_EVENTS
BEGIN_PROPERTIES(ComMeshCollider)
    PROPERTY_BOOL("convex", "Convex", "", true, PropertyInfo::ReadWrite),
    PROPERTY_OBJECT("mesh", "Mesh", "collision mesh", GuidMapper::defaultMeshGuid, MeshAsset::metaObject, PropertyInfo::ReadWrite),
END_PROPERTIES

void ComMeshCollider::RegisterProperties() {
#ifdef NEW_PROPERTY_SYSTEM
    REGISTER_PROPERTY("Convex", bool, convex, true, "", PropertyInfo::ReadWrite);
    REGISTER_MIXED_ACCESSOR_PROPERTY("Mesh", ObjectRef, GetMeshRef, SetMeshRef, ObjectRef(MeshAsset::metaObject, GuidMapper::defaultMeshGuid), "", PropertyInfo::ReadWrite);
#endif
}

ComMeshCollider::ComMeshCollider() {
#ifndef NEW_PROPERTY_SYSTEM
    Connect(&Properties::SIG_PropertyChanged, this, (SignalCallback)&ComMeshCollider::PropertyChanged);
#endif
}

ComMeshCollider::~ComMeshCollider() {
}

void ComMeshCollider::Init() {
    ComCollider::Init();

#ifndef NEW_PROPERTY_SYSTEM
    convex = props->Get("convex").As<bool>();
    meshGuid = props->Get("mesh").As<Guid>();
#endif

    if (!meshGuid.IsZero()) {
        const Str meshPath = resourceGuidMapper.Get(meshGuid);
        collider = colliderManager.GetCollider(meshPath, GetEntity()->GetTransform()->GetScale(), convex);
    }

    // Mark as initialized
    SetInitialized(true);
}

void ComMeshCollider::SetEnable(bool enable) {
    if (enable) {
        if (!IsEnabled()) {
            //UpdateVisuals();
            Component::SetEnable(true);
        }
    } else {
        if (IsEnabled()) {
            //renderWorld->RemoveEntity(renderEntityHandle);
            //renderEntityHandle = -1;
            Component::SetEnable(false);
        }
    }
}

bool ComMeshCollider::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    return false;
}

void ComMeshCollider::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    //collider;
}

void ComMeshCollider::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitialized()) {
        return;
    }

    if (!Str::Cmp(propName, "mesh")) {
        SetMesh(props->Get("mesh").As<Guid>());
        return;
    }

    if (!Str::Cmp(propName, "convex")) {
        convex = props->Get("convex").As<bool>();
        return;
    }

    ComCollider::PropertyChanged(classname, propName);
}

Guid ComMeshCollider::GetMesh() const {
    return meshGuid;
}

void ComMeshCollider::SetMesh(const Guid &meshGuid) {
    this->meshGuid = meshGuid;

    if (!meshGuid.IsZero()) {
        const Str meshPath = resourceGuidMapper.Get(meshGuid);
        collider = colliderManager.GetCollider(meshPath, GetEntity()->GetTransform()->GetScale(), convex);
    }
}

ObjectRef ComMeshCollider::GetMeshRef() const {
    return ObjectRef(MeshAsset::metaObject, meshGuid);
}

void ComMeshCollider::SetMeshRef(const ObjectRef &meshRef) {
    meshGuid = meshRef.objectGuid;

    if (!meshGuid.IsZero()) {
        const Str meshPath = resourceGuidMapper.Get(meshGuid);
        collider = colliderManager.GetCollider(meshPath, GetEntity()->GetTransform()->GetScale(), convex);
    }
}

BE_NAMESPACE_END
