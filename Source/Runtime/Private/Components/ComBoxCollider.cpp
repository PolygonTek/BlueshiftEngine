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
#include "Physics/Collider.h"
#include "Render/Render.h"
#include "Components/ComTransform.h"
#include "Components/ComBoxCollider.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Box Collider", ComBoxCollider, ComCollider)
BEGIN_EVENTS(ComBoxCollider)
END_EVENTS

void ComBoxCollider::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("center", "Center", Vec3, GetCenter, SetCenter, Vec3::zero,
        "The position of the Collider in the object's local space.", PropertyInfo::SystemUnits | PropertyInfo::EditorFlag);
    REGISTER_MIXED_ACCESSOR_PROPERTY("extents", "Extents", Vec3, GetExtents, SetExtents, Vec3(MeterToUnit(0.5f)),
        "The size of the Collider in the X, Y, Z directions.", PropertyInfo::SystemUnits | PropertyInfo::EditorFlag);
}

ComBoxCollider::ComBoxCollider() {
}

ComBoxCollider::~ComBoxCollider() {
}

void ComBoxCollider::CreateCollider() {
    if (collider) {
        colliderManager.ReleaseCollider(collider);
    }

    const ComTransform *transform = GetEntity()->GetTransform();

    Vec3 scaledCenter = transform->GetScale() * center;
    Vec3 scaledExtents = transform->GetScale() * extents;

    collider = colliderManager.AllocUnnamedCollider();
    collider->CreateBox(scaledCenter, scaledExtents);
}

void ComBoxCollider::SetCenter(const Vec3 &center) {
    this->center = center;

    if (IsInitialized()) {
        CreateCollider();
    }
}

void ComBoxCollider::SetExtents(const Vec3 &extents) {
    this->extents = extents;

    if (IsInitialized()) {
        CreateCollider();
    }
}

bool ComBoxCollider::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    return false;
}

void ComBoxCollider::DrawGizmos(const RenderView::State &viewState, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    if (selected) {
        ComTransform *transform = GetEntity()->GetTransform();

        if (transform->GetOrigin().DistanceSqr(viewState.origin) < MeterToUnit(500.0f * 500.0f)) {
            Vec3 scaledExtents = transform->GetScale() * extents;

            OBB obb(transform->GetMatrix() * center, scaledExtents + CentiToUnit(0.15f), transform->GetAxis());

            renderWorld->SetDebugColor(Color4::orange, Color4::zero);
            renderWorld->DebugOBB(obb, 1.25f);
        }
    }
}

BE_NAMESPACE_END
