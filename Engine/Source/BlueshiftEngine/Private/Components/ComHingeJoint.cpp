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
#include "Components/ComTransform.h"
#include "Components/ComRigidBody.h"
#include "Components/ComHingeJoint.h"
#include "Game/Entity.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Hinge Joint", ComHingeJoint, ComJoint)
BEGIN_EVENTS(ComHingeJoint)
END_EVENTS
BEGIN_PROPERTIES(ComHingeJoint)
    PROPERTY_VEC3("anchor", "Anchor", "", "0 0 0", PropertyInfo::ReadWrite),
    PROPERTY_ANGLES("angles", "Angles", "", "0 0 0", PropertyInfo::ReadWrite),
    PROPERTY_FLOAT("motorSpeed", "Motor Speed", "", "0", PropertyInfo::ReadWrite),
    PROPERTY_FLOAT("maxMotorImpulse", "Max Motor Impulse", "", "0", PropertyInfo::ReadWrite),
END_PROPERTIES

void ComHingeJoint::RegisterProperties() {
#ifdef NEW_PROPERTY_SYSTEM
    REGISTER_ACCESSOR_PROPERTY("Anchor", Vec3, GetAnchor, SetAnchor, Vec3::zero, "", PropertyInfo::ReadWrite);
    REGISTER_MIXED_ACCESSOR_PROPERTY("Angles", Angles, GetAngles, SetAngles, Vec3::zero, "", PropertyInfo::ReadWrite);
    REGISTER_ACCESSOR_PROPERTY("Motor Speed", float, GetMotorSpeed, SetMotorSpeed, 0.f, "", PropertyInfo::ReadWrite);
    REGISTER_ACCESSOR_PROPERTY("Max Motor Impulse", float, GetMaxMotorImpulse, SetMaxMotorImpulse, 0.f, "", PropertyInfo::ReadWrite);
#endif
}

ComHingeJoint::ComHingeJoint() {
#ifndef NEW_PROPERTY_SYSTEM
    Connect(&Properties::SIG_PropertyChanged, this, (SignalCallback)&ComHingeJoint::PropertyChanged);
#endif
}

ComHingeJoint::~ComHingeJoint() {
}

void ComHingeJoint::Init() {
    ComJoint::Init();

#ifndef NEW_PROPERTY_SYSTEM
    anchor = props->Get("anchor").As<Vec3>();
    axis = props->Get("angles").As<Angles>().ToMat3();
    axis.FixDegeneracies();
    motorSpeed = props->Get("motorSpeed").As<float>();
    maxMotorImpulse = props->Get("maxMotorImpulse").As<float>();
#endif

    // Mark as initialized
    SetInitialized(true);
}

void ComHingeJoint::Start() {
    ComJoint::Start();

    const ComTransform *transform = GetEntity()->GetTransform();
    const ComRigidBody *rigidBody = GetEntity()->GetComponent<ComRigidBody>();
    assert(rigidBody);

    PhysConstraintDesc desc;
    desc.type           = PhysConstraint::Hinge;
    desc.bodyA          = rigidBody->GetBody();
    desc.axisInA        = axis;
    desc.anchorInA      = transform->GetScale() * anchor;

    if (connectedBody) {
        Vec3 worldAnchor = desc.bodyA->GetOrigin() + desc.bodyA->GetAxis() * desc.anchorInA;

        desc.bodyB      = connectedBody->GetBody();
        desc.axisInB    = axis;
        desc.anchorInB  = connectedBody->GetBody()->GetAxis().TransposedMulVec(worldAnchor - connectedBody->GetBody()->GetOrigin());
    } else {
        desc.bodyB      = nullptr;
    }
    
    desc.collision      = collisionEnabled;
    desc.breakImpulse   = breakImpulse;
    constraint = physicsSystem.CreateConstraint(&desc);

    PhysHingeConstraint *hingeConstraint = static_cast<PhysHingeConstraint *>(constraint);
    hingeConstraint->EnableMotor(motorSpeed != 0.0f ? true : false);
    hingeConstraint->SetMotor(motorSpeed, maxMotorImpulse);

    if (IsEnabled()) {
        hingeConstraint->AddToWorld(GetGameWorld()->GetPhysicsWorld());
    }
}

void ComHingeJoint::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    const ComTransform *transform = GetEntity()->GetTransform();
    Vec3 worldOrigin = transform->GetWorldMatrix() * anchor;
    Mat3 worldAxis = transform->GetAxis() * axis;
    
    renderWorld->SetDebugColor(Color4::red, Color4::zero);
    renderWorld->DebugLine(worldOrigin - worldAxis[0] * CentiToUnit(5), worldOrigin + worldAxis[0] * CentiToUnit(5), 1);
    renderWorld->DebugLine(worldOrigin - worldAxis[1] * CentiToUnit(5), worldOrigin + worldAxis[1] * CentiToUnit(5), 1);
    renderWorld->DebugLine(worldOrigin - worldAxis[2] * CentiToUnit(10), worldOrigin + worldAxis[2] * CentiToUnit(10), 1);
}

void ComHingeJoint::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitialized()) {
        return;
    }

    if (!Str::Cmp(propName, "anchor")) {
        SetAnchor(props->Get("anchor").As<Vec3>());
        return;
    }
    
    if (!Str::Cmp(propName, "angles")) {
        SetAngles(props->Get("angles").As<Angles>());
        return;
    }

    if (!Str::Cmp(propName, "motorSpeed")) {
        SetMotorSpeed(props->Get("motorSpeed").As<float>());
        return;
    }

    if (!Str::Cmp(propName, "maxMotorImpulse")) {
        SetMaxMotorImpulse(props->Get("maxMotorImpulse").As<float>());
        return;
    }
    
    ComJoint::PropertyChanged(classname, propName);
}

const Vec3 &ComHingeJoint::GetAnchor() const {
    return anchor;
}

void ComHingeJoint::SetAnchor(const Vec3 &anchor) {
    this->anchor = anchor;
    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetFrameA(anchor, axis);
    }
}

Angles ComHingeJoint::GetAngles() const {
    return axis.ToAngles();
}

void ComHingeJoint::SetAngles(const Angles &angles) {
    this->axis = angles.ToMat3();
    this->axis.FixDegeneracies();

    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetFrameA(anchor, axis);
    }
}

float ComHingeJoint::GetMotorSpeed() const {
    return motorSpeed;
}

void ComHingeJoint::SetMotorSpeed(float motorSpeed) {
    this->motorSpeed = motorSpeed;
    if (constraint) {
        ((PhysHingeConstraint *)constraint)->EnableMotor(motorSpeed != 0.0f ? true : false);
        ((PhysHingeConstraint *)constraint)->SetMotor(motorSpeed, maxMotorImpulse);
    }
}

float ComHingeJoint::GetMaxMotorImpulse() const {
    return maxMotorImpulse;
}

void ComHingeJoint::SetMaxMotorImpulse(float maxMotorImpulse) {
    this->maxMotorImpulse = maxMotorImpulse;
    if (constraint) {
        ((PhysHingeConstraint *)constraint)->EnableMotor(motorSpeed != 0.0f ? true : false);
        ((PhysHingeConstraint *)constraint)->SetMotor(motorSpeed, maxMotorImpulse);
    }
}

BE_NAMESPACE_END
