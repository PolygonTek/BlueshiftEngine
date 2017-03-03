#include "Precompiled.h"
#include "Components/ComTransform.h"
#include "Components/ComRigidBody.h"
#include "Components/ComJoint.h"

BE_NAMESPACE_BEGIN

ABSTRACT_DECLARATION("Joint", ComJoint, Component)
BEGIN_EVENTS(ComJoint)
END_EVENTS
BEGIN_PROPERTIES(ComJoint)
    PROPERTY_OBJECT("connectedBody", "Connected Body", "", Guid::zero.ToString(), ComRigidBody::metaObject, PropertySpec::ReadWrite),
    PROPERTY_BOOL("collisionEnabled", "Collision Enabled", "", "true", PropertySpec::ReadWrite),
    PROPERTY_FLOAT("breakImpulse", "Break Impulse", "", "1e30f", PropertySpec::ReadWrite),
END_PROPERTIES

void ComJoint::RegisterProperties() {
    //REGISTER_ACCESSOR_PROPERTY("Connected Body", ComRigidBody, GetConnectedBody, SetConnectedBody, Guid::zero.ToString(), PropertySpec::ReadWrite);
    //REGISTER_ACCESSOR_PROPERTY("Collision Enabled", bool, IsCollisionEnabled, SetCollisionEnabled, PropertySpec::ReadWrite);
    //REGISTER_ACCESSOR_PROPERTY("Break Impulse", float, GetBreakImpulse, SetBreakImpulse, PropertySpec::ReadWrite);
}

ComJoint::ComJoint() {
    constraint = nullptr;
    connectedBody = nullptr;
}

ComJoint::~ComJoint() {
    Purge(false);
}

void ComJoint::Purge(bool chainPurge) {
    if (constraint) {
        physicsSystem.DestroyConstraint(constraint);
        constraint = nullptr;
    }

    if (chainPurge) {
        Component::Purge();
    }
}

void ComJoint::Init() {
    Purge();

    Component::Init();

    collisionEnabled = props->Get("collisionEnabled").As<bool>();
    // FIXME: atof("infinity") parsing 안됨
    breakImpulse = props->Get("breakImpulse").As<float>();
}

void ComJoint::Start() {
    connectedBody = nullptr;
    const Guid guid = props->Get("connectedBody").As<Guid>();
    if (!guid.IsZero()) {
        connectedBody = Object::FindInstance(guid)->Cast<ComRigidBody>();
    }
}

void ComJoint::Enable(bool enable) {
    if (enable) {
        if (!IsEnabled()) {
            if (constraint) {
                constraint->SetEnabled(true);
            }
            Component::Enable(true);
        }
    } else {
        if (IsEnabled()) {
            if (constraint) {
                constraint->SetEnabled(false);
            }
            Component::Enable(false);
        }
    }
}

void ComJoint::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
        return;
    }

    if (!Str::Cmp(propName, "connectedBody")) {        
        SetConnectedBody(props->Get("connectedBody").As<Guid>());
        return;
    }

    if (!Str::Cmp(propName, "collisionEnabled")) {
        SetCollisionEnabled(props->Get("collisionEnabled").As<bool>());
        return;
    }

    if (!Str::Cmp(propName, "breakImpulse")) {
        SetBreakImpulse(props->Get("breakImpulse").As<float>());
        return;
    }

    Component::PropertyChanged(classname, propName);
}

const Guid ComJoint::GetConnectedBody() const {
    if (connectedBody) {
        return connectedBody->GetGuid();
    }
    return Guid();
}

void ComJoint::SetConnectedBody(const Guid &guid) {
    if (!guid.IsZero()) {
        connectedBody = Object::FindInstance(guid)->Cast<ComRigidBody>();
    } else {
        connectedBody = nullptr;
    }
}

const bool ComJoint::IsCollisionEnabled() const {
    return collisionEnabled;
}

void ComJoint::SetCollisionEnabled(const bool enabled) {
    collisionEnabled = enabled;
    if (constraint) {
        constraint->EnableCollision(collisionEnabled);
    }
}

const float ComJoint::GetBreakImpulse() const {
    return breakImpulse;
}

void ComJoint::SetBreakImpulse(const float breakImpulse) {
    this->breakImpulse = breakImpulse;
    if (constraint) {
        constraint->SetBreakImpulse(breakImpulse);
    }
}

BE_NAMESPACE_END
