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
#include "Game/Entity.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Cylinder Collider", ComCylinderCollider, ComCollider)
BEGIN_EVENTS(ComCylinderCollider)
END_EVENTS
BEGIN_PROPERTIES(ComCylinderCollider)
    PROPERTY_VEC3("center", "Center", "", "0 0 0", PropertySpec::ReadWrite),
    PROPERTY_FLOAT("radius", "Radius", "", "1", PropertySpec::ReadWrite),
    PROPERTY_FLOAT("height", "Height", "", "1", PropertySpec::ReadWrite),
END_PROPERTIES

void ComCylinderCollider::RegisterProperties() {
#ifdef NEW_PROPERTY_SYSTEM
    REGISTER_PROPERTY("Center", Vec3, center, Vec3::zero, "", PropertySpec::ReadWrite);
    REGISTER_PROPERTY("Radius", float, radius, 1.f, "", PropertySpec::ReadWrite);
    REGISTER_PROPERTY("Height", float, height, 1.f, "", PropertySpec::ReadWrite);
#endif
}

ComCylinderCollider::ComCylinderCollider() {
#ifndef NEW_PROPERTY_SYSTEM
    Connect(&Properties::SIG_PropertyChanged, this, (SignalCallback)&ComCylinderCollider::PropertyChanged);
#endif
}

ComCylinderCollider::~ComCylinderCollider() {
}

void ComCylinderCollider::Init() {
    ComCollider::Init();

    center = props->Get("center").As<Vec3>();
    radius = props->Get("radius").As<float>();
    height = props->Get("height").As<float>();

    ComTransform *transform = GetEntity()->GetTransform();
    
    Vec3 scaledCenter = transform->GetScale() * center;
    float scaledRadius = (transform->GetScale().ToVec2() * radius).MaxComponent();
    float scaledHeight = transform->GetScale().z * height;

    collider = colliderManager.AllocUnnamedCollider();
    collider->CreateCylinder(scaledCenter, scaledRadius, scaledHeight);
}

void ComCylinderCollider::SetEnable(bool enable) {
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

bool ComCylinderCollider::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    return false;
}

void ComCylinderCollider::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    if (selected) {
        const ComTransform *transform = GetEntity()->GetTransform();

        float scaledRadius = (transform->GetScale().ToVec2() * radius).MaxComponent();
        float scaledHeight = transform->GetScale().z * height;

        Vec3 worldCenter = transform->GetWorldMatrix() * center;

        renderWorld->SetDebugColor(Color4::orange, Color4::zero);
        renderWorld->DebugCylinderSimple(worldCenter, transform->GetAxis(), scaledHeight, scaledRadius + CentiToUnit(0.25f), 1.25f, true);
    }
}

void ComCylinderCollider::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
        return;
    }

    if (!Str::Cmp(propName, "center") || !Str::Cmp(propName, "radius") || !Str::Cmp(propName, "height")) {
        center = props->Get("center").As<Vec3>();
        radius = props->Get("radius").As<float>();
        height = props->Get("height").As<float>();
        return;
    }

    ComCollider::PropertyChanged(classname, propName);
}

BE_NAMESPACE_END
