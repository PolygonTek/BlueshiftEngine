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
#include "Game/Entity.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Box Collider", ComBoxCollider, ComCollider)
BEGIN_EVENTS(ComBoxCollider)
END_EVENTS
BEGIN_PROPERTIES(ComBoxCollider)
    PROPERTY_VEC3("center", "Center", "", "0 0 0", PropertySpec::ReadWrite),
    PROPERTY_VEC3("extents", "Extents", "", "1 1 1", PropertySpec::ReadWrite),
END_PROPERTIES

void ComBoxCollider::RegisterProperties() {
#ifdef NEW_PROPERTY_SYSTEM
    REGISTER_PROPERTY("Center", Vec3, center, "0 0 0", "", PropertySpec::ReadWrite);
    REGISTER_PROPERTY("Extents", Vec3, extents, "1 1 1", "", PropertySpec::ReadWrite);
#endif
}

ComBoxCollider::ComBoxCollider() {
    Connect(&Properties::SIG_PropertyChanged, this, (SignalCallback)&ComBoxCollider::PropertyChanged);
}

ComBoxCollider::~ComBoxCollider() {
}

void ComBoxCollider::Init() {
    ComCollider::Init();

    center = props->Get("center").As<Vec3>();
    extents = props->Get("extents").As<Vec3>();

    ComTransform *transform = GetEntity()->GetTransform();

    Vec3 scaledCenter = transform->GetScale() * center;
    Vec3 scaledExtents = transform->GetScale() * extents;

    collider = colliderManager.AllocUnnamedCollider();
    collider->CreateBox(scaledCenter, scaledExtents);
}

void ComBoxCollider::Enable(bool enable) {
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

bool ComBoxCollider::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    return false;
}

void ComBoxCollider::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    if (selected) {
        ComTransform *transform = GetEntity()->GetTransform();

        Vec3 scaledExtents = transform->GetScale() * extents;
        OBB obb(transform->GetWorldMatrix() * center, scaledExtents + Vec3(CentiToUnit(0.25f)), transform->GetAxis());

        renderWorld->SetDebugColor(Color4::orange, Color4::zero);
        renderWorld->DebugOBB(obb, 1.25f);
    }
}

void ComBoxCollider::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
        return;
    }

    if (!Str::Cmp(propName, "center")) { 
        center = props->Get("center").As<Vec3>();
        return;
    }

    if (!Str::Cmp(propName, "extents")) {
        extents = props->Get("extents").As<Vec3>();
        return;
    }

    ComCollider::PropertyChanged(classname, propName);
}

BE_NAMESPACE_END
