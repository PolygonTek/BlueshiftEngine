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
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Mesh Collider", ComMeshCollider, ComCollider)
BEGIN_EVENTS(ComMeshCollider)
END_EVENTS

void ComMeshCollider::RegisterProperties() {
    REGISTER_PROPERTY("convex", "Convex", bool, convex, false, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_MIXED_ACCESSOR_PROPERTY("mesh", "Mesh", Guid, GetMeshGuid, SetMeshGuid, GuidMapper::defaultMeshGuid, 
        "", PropertyInfo::Flag::Editor).SetMetaObject(&MeshResource::metaObject);
}

ComMeshCollider::ComMeshCollider() {
    meshGuid = Guid::zero;
}

ComMeshCollider::~ComMeshCollider() {
}

void ComMeshCollider::CreateCollider() {
    if (collider) {
        colliderManager.ReleaseCollider(collider);
    }

    const Str meshPath = resourceGuidMapper.Get(meshGuid);
    collider = colliderManager.GetCollider(meshPath, GetEntity()->GetTransform()->GetScale(), convex);
}

void ComMeshCollider::SetMeshGuid(const Guid &meshGuid) {
    this->meshGuid = meshGuid;

    if (IsInitialized()) {
        CreateCollider();
    }
}

#if WITH_EDITOR
void ComMeshCollider::DrawGizmos(const RenderCamera *camera, bool selected) {
    //collider;
}
#endif

BE_NAMESPACE_END
