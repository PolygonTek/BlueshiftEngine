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
#include "Components/Transform/ComTransform.h"
#include "Components/Collider/ComCylinderCollider.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Cylinder Collider", ComCylinderCollider, ComCollider)
BEGIN_EVENTS(ComCylinderCollider)
END_EVENTS

void ComCylinderCollider::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("center", "Center", Vec3, GetCenter, SetCenter, Vec3::zero, 
        "The position of the Collider in the object's local space.", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("radius", "Radius", float, GetRadius, SetRadius, MeterToUnit(0.5f),
        "The radius of the Collider's local width.", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("height", "Height", float, GetHeight, SetHeight, MeterToUnit(1.0f),
        "The height of the Collider.", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_PROPERTY("direction", "Direction", Direction::Enum, direction, Direction::Enum::ZAxis,
        "", PropertyInfo::Flag::Editor).SetEnumString("X-Axis;Y-Axis;Z-Axis");
}

ComCylinderCollider::ComCylinderCollider() {
}

ComCylinderCollider::~ComCylinderCollider() {
}

void ComCylinderCollider::Init() {
    ComCollider::Init();

    CreateCollider();
}

void ComCylinderCollider::CreateCollider() {
    if (collider) {
        colliderManager.ReleaseCollider(collider);
    }

    const ComTransform *transform = GetEntity()->GetTransform();

    Vec3 scaledCenter = transform->GetScale() * center;
    float scaledRadius = (transform->GetScale().ToVec2() * radius).MaxComponent();
    float scaledHeight = transform->GetScale().z * height;

    Collider::Axis::Enum axis;
    if (direction == Direction::XAxis) {
        axis = Collider::Axis::X;
    } else if (direction == Direction::YAxis) {
        axis = Collider::Axis::Y;
    } else {
        axis = Collider::Axis::Z;
    }

    collider = colliderManager.AllocUnnamedCollider();
    collider->CreateCylinder(scaledCenter, scaledRadius, scaledHeight, axis);
}

void ComCylinderCollider::SetCenter(const Vec3 &center) {
    this->center = center;

    if (IsInitialized()) {
        CreateCollider();
    }
}

void ComCylinderCollider::SetRadius(float radius) {
    this->radius = radius;

    if (IsInitialized()) {
        CreateCollider();
    }
}

void ComCylinderCollider::SetHeight(float height) {
    this->height = height;

    if (IsInitialized()) {
        CreateCollider();
    }
}

#if WITH_EDITOR
void ComCylinderCollider::DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) {
    if (selectedByParent) {
        const ComTransform *transform = GetEntity()->GetTransform();

        if (transform->GetOrigin().DistanceSqr(camera->GetState().origin) < MeterToUnit(500.0f * 500.0f)) {
            float scaledRadius = (transform->GetScale().ToVec2() * radius).MaxComponent();
            float scaledHeight = transform->GetScale().z * height;

            Vec3 worldCenter = transform->GetWorldMatrix().TransformPos(center);

            Mat3 axisRotation;
            if (direction == Direction::XAxis) {
                axisRotation = Mat3::FromRotationY(Math::HalfPi);
            } else if (direction == Direction::YAxis) {
                axisRotation = Mat3::FromRotationX(-Math::HalfPi);
            } else {
                axisRotation = Mat3::identity;
            }

            RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();
            if (selected) {
                renderWorld->SetDebugColor(Color4(Color4::orange.ToColor3(), 0.1f), Color4::zero);
                renderWorld->DebugCylinderSimple(worldCenter, transform->GetAxis() * axisRotation, scaledHeight, scaledRadius + CmToUnit(0.15f), 1.25f, false);
            }
            renderWorld->SetDebugColor(Color4::orange, Color4::zero);
            renderWorld->DebugCylinderSimple(worldCenter, transform->GetAxis() * axisRotation, scaledHeight, scaledRadius + CmToUnit(0.15f), 1.25f, true);
        }
    }
}
#endif

BE_NAMESPACE_END
