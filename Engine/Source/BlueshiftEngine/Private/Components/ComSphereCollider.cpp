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
#include "Components/ComSphereCollider.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Sphere Collider", ComSphereCollider, ComCollider)
BEGIN_EVENTS(ComSphereCollider)
END_EVENTS

void ComSphereCollider::RegisterProperties() {
    REGISTER_PROPERTY("center", "Center", Vec3, center, Vec3::zero, "", PropertyInfo::EditorFlag);
    REGISTER_PROPERTY("radius", "Radius", float, radius, 1.0f, "", PropertyInfo::EditorFlag);
}

ComSphereCollider::ComSphereCollider() {
}

ComSphereCollider::~ComSphereCollider() {
}

void ComSphereCollider::Init() {
    ComCollider::Init();

    // Create collider based on transformed sphere
    const ComTransform *transform = GetEntity()->GetTransform();
    const Vec3 scaledCenter = transform->GetScale() * center;
    const float scaledRadius = (transform->GetScale() * radius).MaxComponent();

    collider = colliderManager.AllocUnnamedCollider();
    collider->CreateSphere(scaledCenter, scaledRadius);

    // Mark as initialized
    SetInitialized(true);
}

bool ComSphereCollider::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    return false;
}

void ComSphereCollider::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    if (selected) {
        const ComTransform *transform = GetEntity()->GetTransform();

        if (transform->GetOrigin().DistanceSqr(sceneView.origin) < 20000.0f * 20000.0f) {
            float scaledRadius = (transform->GetScale() * radius).MaxComponent();

            renderWorld->SetDebugColor(Color4::orange, Color4::zero);
            renderWorld->DebugSphereSimple(transform->GetTransform() * center, transform->GetAxis(), scaledRadius + CentiToUnit(0.25f), 1.25f, true);
        }
    }
}

BE_NAMESPACE_END
