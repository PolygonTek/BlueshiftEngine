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
#include "Components/ComConeCollider.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Cone Collider", ComConeCollider, ComCollider)
BEGIN_EVENTS(ComConeCollider)
END_EVENTS

void ComConeCollider::RegisterProperties() {
    REGISTER_PROPERTY("center", "Center", Vec3, center, Vec3::zero, "", PropertyInfo::EditorFlag);
    REGISTER_PROPERTY("radius", "Radius", float, radius, 1.f, "", PropertyInfo::EditorFlag);
    REGISTER_PROPERTY("height", "Height", float, height, 1.f, "", PropertyInfo::EditorFlag);
}

ComConeCollider::ComConeCollider() {
}

ComConeCollider::~ComConeCollider() {
}

void ComConeCollider::Init() {
    ComCollider::Init();

    // Create collider based on transformed cone
    const ComTransform *transform = GetEntity()->GetTransform();
    Vec3 scaledCenter = transform->GetScale() * center;
    float scaledRadius = (transform->GetScale().ToVec2() * radius).MaxComponent();
    float scaledHeight = transform->GetScale().z * height;

    collider = colliderManager.AllocUnnamedCollider();
    collider->CreateCone(scaledCenter, scaledRadius, scaledHeight);

    // Mark as initialized
    SetInitialized(true);
}

bool ComConeCollider::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    return false;
}

void ComConeCollider::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    if (selected) {
        ComTransform *transform = GetEntity()->GetTransform();

        if (transform->GetOrigin().DistanceSqr(sceneView.origin) < 20000.0f * 20000.0f) {
            float scaledRadius = (transform->GetScale().ToVec2() * radius).MaxComponent();
            float scaledHeight = transform->GetScale().z * height;

            Vec3 worldOrigin = transform->GetTransform() * center - transform->GetAxis()[2] * scaledHeight * 0.5f;

            renderWorld->SetDebugColor(Color4::orange, Color4::zero);
            renderWorld->DebugCone(worldOrigin, transform->GetAxis(), scaledHeight, 0, scaledRadius, false, 1.25f);
        }
    }
}

BE_NAMESPACE_END
