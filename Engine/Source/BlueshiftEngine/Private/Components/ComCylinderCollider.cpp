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
#include "Components/ComCylinderCollider.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Cylinder Collider", ComCylinderCollider, ComCollider)
BEGIN_EVENTS(ComCylinderCollider)
END_EVENTS

void ComCylinderCollider::RegisterProperties() {
    REGISTER_PROPERTY("center", "Center", Vec3, center, Vec3::zero, "", PropertyInfo::EditorFlag);
    REGISTER_PROPERTY("radius", "Radius", float, radius, 1.f, "", PropertyInfo::EditorFlag);
    REGISTER_PROPERTY("height", "Height", float, height, 1.f, "", PropertyInfo::EditorFlag);
}

ComCylinderCollider::ComCylinderCollider() {
}

ComCylinderCollider::~ComCylinderCollider() {
}

void ComCylinderCollider::Init() {
    ComCollider::Init();

    // Create collider based on transformed cylinder
    const ComTransform *transform = GetEntity()->GetTransform();
    Vec3 scaledCenter = transform->GetScale() * center;
    float scaledRadius = (transform->GetScale().ToVec2() * radius).MaxComponent();
    float scaledHeight = transform->GetScale().z * height;

    collider = colliderManager.AllocUnnamedCollider();
    collider->CreateCylinder(scaledCenter, scaledRadius, scaledHeight);

    // Mark as initialized
    SetInitialized(true);
}

bool ComCylinderCollider::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    return false;
}

void ComCylinderCollider::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    if (selected) {
        const ComTransform *transform = GetEntity()->GetTransform();

        if (transform->GetOrigin().DistanceSqr(sceneView.origin) < 20000.0f * 20000.0f) {
            float scaledRadius = (transform->GetScale().ToVec2() * radius).MaxComponent();
            float scaledHeight = transform->GetScale().z * height;

            Vec3 worldCenter = transform->GetTransform() * center;

            renderWorld->SetDebugColor(Color4::orange, Color4::zero);
            renderWorld->DebugCylinderSimple(worldCenter, transform->GetAxis(), scaledHeight, scaledRadius + CentiToUnit(0.25f), 1.25f, true);
        }
    }
}

BE_NAMESPACE_END
