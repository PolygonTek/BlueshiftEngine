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
#include "Game/Entity.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Sphere Collider", ComSphereCollider, ComCollider)
BEGIN_EVENTS(ComSphereCollider)
END_EVENTS
BEGIN_PROPERTIES(ComSphereCollider)
    PROPERTY_VEC3("center", "Center", "", "0 0 0", PropertySpec::ReadWrite),
    PROPERTY_FLOAT("radius", "Radius", "", "1", PropertySpec::ReadWrite),
END_PROPERTIES

void ComSphereCollider::RegisterProperties() {
    //REGISTER_PROPERTY("Center", Vec3, center, "0 0 0", PropertySpec::ReadWrite);
    //REGISTER_PROPERTY("Radius", float, radius, "1", PropertySpec::ReadWrite);
}

ComSphereCollider::ComSphereCollider() {
    Connect(&Properties::SIG_PropertyChanged, this, (SignalCallback)&ComSphereCollider::PropertyChanged);
}

ComSphereCollider::~ComSphereCollider() {
}

void ComSphereCollider::Init() {
    ComCollider::Init();

    center = props->Get("center").As<Vec3>();
    radius = props->Get("radius").As<float>();

    const ComTransform *transform = GetEntity()->GetTransform();

    const Vec3 scaledCenter = transform->GetScale() * center;
    const float scaledRadius = (transform->GetScale() * radius).MaxComponent();

    collider = colliderManager.AllocUnnamedCollider();
    collider->CreateSphere(scaledCenter, scaledRadius);
}

void ComSphereCollider::Enable(bool enable) {
    if (enable) {
        if (!IsEnabled()) {
            //UpdateVisuals();
            Component::Enable(true);
        }
    } else {
        if (IsEnabled()) {
            //renderWorld->RemoveEntity(renderEntityHandle);
            //renderEntityHandle = -1;
            Component::Enable(false);
        }
    }
}

bool ComSphereCollider::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    return false;
}

void ComSphereCollider::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    if (selected) {
        const ComTransform *transform = GetEntity()->GetTransform();

        float scaledRadius = (transform->GetScale() * radius).MaxComponent();

        renderWorld->SetDebugColor(Color4::orange, Color4::zero);
        renderWorld->DebugSphereSimple(transform->GetWorldMatrix() * center, transform->GetAxis(), scaledRadius + CentiToUnit(0.25f), 1.25f, true);
    }
}

void ComSphereCollider::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
        return;
    }

    if (!Str::Cmp(propName, "center") || !Str::Cmp(propName, "radius")) {
        center = props->Get("center").As<Vec3>();
        radius = props->Get("radius").As<float>();
        return;
    }

    ComCollider::PropertyChanged(classname, propName);
}

BE_NAMESPACE_END
