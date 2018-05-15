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
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Hinge Joint", ComHingeJoint, ComJoint)
BEGIN_EVENTS(ComHingeJoint)
END_EVENTS

void ComHingeJoint::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("anchor", "Anchor", Vec3, GetLocalAnchor, SetLocalAnchor, Vec3::zero, 
        "Joint position in local space", PropertyInfo::SystemUnits | PropertyInfo::EditorFlag);
    REGISTER_MIXED_ACCESSOR_PROPERTY("angles", "Angles", Angles, GetLocalAngles, SetLocalAngles, Vec3::zero, 
        "Joint angles in local space", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("useLimits", "Limits/Use Limits", bool, GetEnableLimitAngles, SetEnableLimitAngles, false, 
        "Activate joint limits", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("minAngle", "Limits/Minimum Angle", float, GetMinimumAngle, SetMinimumAngle, 0.f, 
        "Minimum value of joint angle", PropertyInfo::EditorFlag).SetRange(-180, 0, 1);
    REGISTER_ACCESSOR_PROPERTY("maxAngle", "Limits/Maximum Angle", float, GetMaximumAngle, SetMaximumAngle, 0.f, 
        "Maximum value of joint angle", PropertyInfo::EditorFlag).SetRange(0, 180, 1);
    REGISTER_ACCESSOR_PROPERTY("motorTargetVelocity", "Motor/Target Velocity", float, GetMotorTargetVelocity, SetMotorTargetVelocity, 0.f, 
        "Target angular velocity (degree/s) of motor", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("maxMotorImpulse", "Motor/Maximum Impulse", float, GetMaxMotorImpulse, SetMaxMotorImpulse, 0.f, 
        "Maximum motor impulse", PropertyInfo::EditorFlag);
}

ComHingeJoint::ComHingeJoint() {
}

ComHingeJoint::~ComHingeJoint() {
}

void ComHingeJoint::Init() {
    ComJoint::Init();

    // Mark as initialized
    SetInitialized(true);
}

void ComHingeJoint::CreateConstraint() {
    const ComTransform *transform = GetEntity()->GetTransform();
    const ComRigidBody *rigidBody = GetEntity()->GetComponent<ComRigidBody>();
    assert(rigidBody);

    PhysConstraintDesc desc;
    desc.type = PhysConstraint::Hinge;
    desc.collision = collisionEnabled;
    desc.breakImpulse = breakImpulse;

    desc.bodyA = rigidBody->GetBody();
    desc.axisInA = localAxis;
    desc.anchorInA = transform->GetScale() * localAnchor;

    if (connectedBody) {
        Mat3 worldAxis = desc.bodyA->GetAxis() * localAxis;
        Vec3 worldAnchor = desc.bodyA->GetOrigin() + desc.bodyA->GetAxis() * desc.anchorInA;

        desc.bodyB = connectedBody->GetBody();
        desc.axisInB = connectedBody->GetBody()->GetAxis().TransposedMul(worldAxis);
        desc.anchorInB = connectedBody->GetBody()->GetAxis().TransposedMulVec(worldAnchor - connectedBody->GetBody()->GetOrigin());

        connectedAxis = desc.axisInB;
        connectedAnchor = desc.anchorInB;
    } else {
        desc.bodyB = nullptr;

        connectedAxis = Mat3::identity;
        connectedAnchor = Vec3::origin;
    }

    // Create a constraint with the given description
    PhysHingeConstraint *hingeConstraint = (PhysHingeConstraint *)physicsSystem.CreateConstraint(desc);

    // Apply limit angles
    hingeConstraint->SetAngularLimits(DEG2RAD(minAngle), DEG2RAD(maxAngle));
    hingeConstraint->EnableAngularLimits(enableLimitAngles);

    // Apply motor
    if (motorTargetVelocity != 0.0f) {
        hingeConstraint->SetMotor(DEG2RAD(motorTargetVelocity), maxMotorImpulse);
        hingeConstraint->EnableMotor(true);
    }

    constraint = hingeConstraint;
}

const Vec3 &ComHingeJoint::GetLocalAnchor() const {
    return localAnchor;
}

void ComHingeJoint::SetLocalAnchor(const Vec3 &anchor) {
    this->localAnchor = anchor;

    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetFrameA(anchor, localAxis);
    }
}

Angles ComHingeJoint::GetLocalAngles() const {
    return localAxis.ToAngles();
}

void ComHingeJoint::SetLocalAngles(const Angles &angles) {
    this->localAxis = angles.ToMat3();
    this->localAxis.FixDegeneracies();

    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetFrameA(localAnchor, localAxis);
    }
}

const Vec3 &ComHingeJoint::GetConnectedAnchor() const {
    return connectedAnchor;
}

void ComHingeJoint::SetConnectedAnchor(const Vec3 &anchor) {
    this->connectedAnchor = anchor;
    if (constraint) {
        ((PhysP2PConstraint *)constraint)->SetAnchorB(anchor);
    }
}

Angles ComHingeJoint::GetConnectedAngles() const {
    return connectedAxis.ToAngles();
}

void ComHingeJoint::SetConnectedAngles(const Angles &angles) {
    this->connectedAxis = angles.ToMat3();
    this->connectedAxis.FixDegeneracies();

    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetFrameB(connectedAnchor, connectedAxis);
    }
}

void ComHingeJoint::SetEnableLimitAngles(bool enable) {
    this->enableLimitAngles = enable;
    if (constraint) {
        ((PhysHingeConstraint *)constraint)->EnableAngularLimits(enableLimitAngles);
    }
}

void ComHingeJoint::SetMinimumAngle(float minAngle) {
    this->minAngle = minAngle;
    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetAngularLimits(DEG2RAD(minAngle), DEG2RAD(maxAngle));
    }
}

void ComHingeJoint::SetMaximumAngle(float maxAngle) {
    this->maxAngle = maxAngle;
    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetAngularLimits(DEG2RAD(minAngle), DEG2RAD(maxAngle));
    }
}

void ComHingeJoint::SetMotorTargetVelocity(float motorTargetVelocity) {
    this->motorTargetVelocity = motorTargetVelocity;
    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetMotor(DEG2RAD(motorTargetVelocity), maxMotorImpulse);
        ((PhysHingeConstraint *)constraint)->EnableMotor(motorTargetVelocity != 0.0f ? true : false);
    }
}

void ComHingeJoint::SetMaxMotorImpulse(float maxMotorImpulse) {
    this->maxMotorImpulse = maxMotorImpulse;
    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetMotor(DEG2RAD(motorTargetVelocity), maxMotorImpulse);
        ((PhysHingeConstraint *)constraint)->EnableMotor(motorTargetVelocity != 0.0f ? true : false);
    }
}

void ComHingeJoint::DrawGizmos(const RenderView::State &viewState, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    const ComTransform *transform = GetEntity()->GetTransform();

    if (transform->GetOrigin().DistanceSqr(viewState.origin) < MeterToUnit(500.0f * 500.0f)) {
        Vec3 worldOrigin = transform->GetMatrix() * localAnchor;
        Mat3 worldAxis = transform->GetAxis() * localAxis;

        Mat3 constraintAxis = Mat3::identity;
        if (connectedBody) {
            constraintAxis = connectedBody->GetEntity()->GetTransform()->GetAxis();
        }

        if (enableLimitAngles) {
            renderWorld->SetDebugColor(Color4::yellow, Color4::yellow * 0.5f);
            renderWorld->DebugArc(worldOrigin, constraintAxis[0], constraintAxis[1], CentiToUnit(2.5f), minAngle, maxAngle, true);

            renderWorld->SetDebugColor(Color4::red, Color4::zero);
            renderWorld->DebugLine(worldOrigin, worldOrigin + worldAxis[0] * CentiToUnit(2.5f), 1);
        }

        renderWorld->SetDebugColor(Color4::red, Color4::red);
        renderWorld->DebugArrow(worldOrigin - worldAxis[2] * CentiToUnit(5), worldOrigin + worldAxis[2] * CentiToUnit(5.0f), CentiToUnit(3.0f), CentiToUnit(0.75f));
    }
}

BE_NAMESPACE_END
