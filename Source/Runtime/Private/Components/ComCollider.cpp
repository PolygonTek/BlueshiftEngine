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
#include "Components/ComTransform.h"
#include "Components/ComCollider.h"

BE_NAMESPACE_BEGIN

ABSTRACT_DECLARATION("Collider", ComCollider, Component)
BEGIN_EVENTS(ComCollider)
END_EVENTS

void ComCollider::RegisterProperties() {
}

ComCollider::ComCollider() {
    collider = nullptr;
}

ComCollider::~ComCollider() {
    Purge(false);
}

void ComCollider::Purge(bool chainPurge) {
    if (collider) {
        colliderManager.ReleaseCollider(collider);
        collider = nullptr;
    }

    if (chainPurge) {
        Component::Purge();
    }
}

void ComCollider::Init() {
    Component::Init();

    // Mark as initialized
    SetInitialized(true);
}

const AABB ComCollider::GetAABB() {
    if (collider) {
        return collider->GetAABB();
    }
    return AABB::zero;
}

bool ComCollider::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    return false;
}

BE_NAMESPACE_END
