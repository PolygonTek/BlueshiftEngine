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
    REGISTER_ACCESSOR_PROPERTY("anchor", "Anchor", Vec3, GetAnchor, SetAnchor, Vec3::zero, "", PropertyInfo::EditorFlag);
    REGISTER_MIXED_ACCESSOR_PROPERTY("angles", "Angles", Angles, GetAngles, SetAngles, Vec3::zero, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("useLimits", "Use Limits", bool, GetEnableLimitAngles, SetEnableLimitAngles, false, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("minimumAngle", "Minimum Angle", float, GetMinimumAngle, SetMinimumAngle, 0.f, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("maximumAngle", "Maximum Angle", float, GetMaximumAngle, SetMaximumAngle, 0.f, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("motorTargetVelocity", "Motor Target Velocity", float, GetMotorTargetVelocity, SetMotorTargetVelocity, 0.f, 
        "Target angular velocity (degree/s) of motor", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("maxMotorImpulse", "Maximum Motor Impulse", float, GetMaxMotorImpulse, SetMaxMotorImpulse, 0.f, 
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

void ComHingeJoint::Start() {
    ComJoint::Start();

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
        Vec3 worldAnchor = desc.bodyA->GetOrigin() + desc.bodyA->GetAxis() * desc.anchorInA;

        desc.bodyB = connectedBody->GetBody();
        desc.axisInB = localAxis;
        desc.anchorInB = connectedBody->GetBody()->GetAxis().TransposedMulVec(worldAnchor - connectedBody->GetBody()->GetOrigin());
    } else {
        desc.bodyB = nullptr;
    }

    constraint = physicsSystem.CreateConstraint(&desc);

    PhysHingeConstraint *hingeConstraint = static_cast<PhysHingeConstraint *>(constraint);

    // Apply limit angles
    if (enableLimitAngles) {
        hingeConstraint->SetLimitAngles(DEG2RAD(minimumAngle), DEG2RAD(maximumAngle));
        hingeConstraint->EnableLimitAngles(enableLimitAngles);
    }

    // Apply motor
    if (motorTargetVelocity != 0.0f) {
        hingeConstraint->SetMotor(DEG2RAD(motorTargetVelocity), maxMotorImpulse);
        hingeConstraint->EnableMotor(true);
    }

    if (IsActiveInHierarchy()) {
        hingeConstraint->AddToWorld(GetGameWorld()->GetPhysicsWorld());
    }
}

void ComHingeJoint::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    const ComTransform *transform = GetEntity()->GetTransform();

    if (transform->GetOrigin().DistanceSqr(sceneView.origin) < 20000.0f * 20000.0f) {
        Vec3 worldOrigin = transform->GetTransform() * localAnchor;
        Mat3 worldAxis = transform->GetAxis() * localAxis;

        renderWorld->SetDebugColor(Color4::red, Color4::zero);
        renderWorld->DebugLine(worldOrigin - worldAxis[0] * CentiToUnit(2), worldOrigin + worldAxis[0] * CentiToUnit(2), 1);
        renderWorld->DebugLine(worldOrigin - worldAxis[1] * CentiToUnit(2), worldOrigin + worldAxis[1] * CentiToUnit(2), 1);
        renderWorld->DebugLine(worldOrigin - worldAxis[2] * CentiToUnit(10), worldOrigin + worldAxis[2] * CentiToUnit(10), 1);
    }
}

const Vec3 &ComHingeJoint::GetAnchor() const {
    return localAnchor;
}

void ComHingeJoint::SetAnchor(const Vec3 &anchor) {
    this->localAnchor = anchor;
    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetFrameA(anchor, localAxis);
    }
}

Angles ComHingeJoint::GetAngles() const {
    return localAxis.ToAngles();
}

void ComHingeJoint::SetAngles(const Angles &angles) {
    this->localAxis = angles.ToMat3();
    this->localAxis.FixDegeneracies();

    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetFrameA(localAnchor, localAxis);
    }
}

bool ComHingeJoint::GetEnableLimitAngles() const {
    return enableLimitAngles;
}

void ComHingeJoint::SetEnableLimitAngles(bool enable) {
    this->enableLimitAngles = enable;
    if (constraint) {
        ((PhysHingeConstraint *)constraint)->EnableLimitAngles(enableLimitAngles);
    }
}

float ComHingeJoint::GetMinimumAngle() const {
    return minimumAngle;
}

void ComHingeJoint::SetMinimumAngle(float minimumAngle) {
    this->minimumAngle = minimumAngle;
    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetLimitAngles(DEG2RAD(minimumAngle), DEG2RAD(maximumAngle));
    }
}

float ComHingeJoint::GetMaximumAngle() const {
    return maximumAngle;
}

void ComHingeJoint::SetMaximumAngle(float maximumAngle) {
    this->maximumAngle = maximumAngle;
    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetLimitAngles(DEG2RAD(minimumAngle), DEG2RAD(maximumAngle));
    }
}

float ComHingeJoint::GetMotorTargetVelocity() const {
    return motorTargetVelocity;
}

void ComHingeJoint::SetMotorTargetVelocity(float motorTargetVelocity) {
    this->motorTargetVelocity = motorTargetVelocity;
    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetMotor(DEG2RAD(motorTargetVelocity), maxMotorImpulse);
        ((PhysHingeConstraint *)constraint)->EnableMotor(motorTargetVelocity != 0.0f ? true : false);
    }
}

float ComHingeJoint::GetMaxMotorImpulse() const {
    return maxMotorImpulse;
}

void ComHingeJoint::SetMaxMotorImpulse(float maxMotorImpulse) {
    this->maxMotorImpulse = maxMotorImpulse;
    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetMotor(DEG2RAD(motorTargetVelocity), maxMotorImpulse);
        ((PhysHingeConstraint *)constraint)->EnableMotor(motorTargetVelocity != 0.0f ? true : false);
    }
}

BE_NAMESPACE_END
