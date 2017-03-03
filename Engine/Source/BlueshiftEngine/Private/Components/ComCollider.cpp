#include "Precompiled.h"
#include "Physics/Collider.h"
#include "Components/ComTransform.h"
#include "Components/ComCollider.h"

BE_NAMESPACE_BEGIN

ABSTRACT_DECLARATION("Collider", ComCollider, Component)
BEGIN_EVENTS(ComCollider)
END_EVENTS
BEGIN_PROPERTIES(ComCollider)
    PROPERTY_STRING("material", "Physics Material", "", "", PropertySpec::ReadWrite),
END_PROPERTIES

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
    Purge();

    Component::Init();
}

void ComCollider::Enable(bool enable) {
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

const AABB ComCollider::GetAABB() {
    if (collider) {
        return collider->GetAABB();
    }
    return AABB::zero;
}

bool ComCollider::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    return false;
}

void ComCollider::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
        return;
    }
    
    if (!Str::Cmp(propName, "material")) {
        material = props->Get("material").As<Str>();
        return;
    } 

    Component::PropertyChanged(classname, propName);
}

BE_NAMESPACE_END
