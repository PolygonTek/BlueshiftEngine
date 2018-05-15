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
#include "Components/ComWheelJoint.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Wheel Joint", ComWheelJoint, ComJoint)
BEGIN_EVENTS(ComWheelJoint)
END_EVENTS

void ComWheelJoint::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("anchor", "Anchor", Vec3, GetLocalAnchor, SetLocalAnchor, Vec3::zero, 
        "Joint position in local space", PropertyInfo::SystemUnits | PropertyInfo::EditorFlag);
    REGISTER_MIXED_ACCESSOR_PROPERTY("angles", "Angles", Angles, GetLocalAngles, SetLocalAngles, Vec3::zero,
        "Joint angles in local space", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("useSusLimits", "Suspension/Use Limits", bool, GetEnableSuspensionLimit, SetEnableSuspensionLimit, false, 
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("minSusDist", "Suspension/Min Distance", float, GetMinimumSuspensionDistance, SetMinimumSuspensionDistance, 0.f, 
        "", PropertyInfo::SystemUnits | PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("maxSusDist", "Suspension/Max Distance", float, GetMaximumSuspensionDistance, SetMaximumSuspensionDistance, 0.f, 
        "", PropertyInfo::SystemUnits | PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("susStiffness", "Suspension/Stiffness", float, GetSuspensionStiffness, SetSuspensionStiffness, 30.f, 
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("susDamping", "Suspension/Damping", float, GetSuspensionDamping, SetSuspensionDamping, 0.2f,
        "", PropertyInfo::EditorFlag);// .SetRange(0, 1, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("useSteeringLimits", "Steering/Use Limits", bool, GetEnableSteeringLimit, SetEnableSteeringLimit, false, 
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("minSteeringAngle", "Steering/Min Angle", float, GetMinimumSteeringAngle, SetMinimumSteeringAngle, 0.f, 
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("maxSteeringAngle", "Steering/Max Angle", float, GetMaximumSteeringAngle, SetMaximumSteeringAngle, 0.f, 
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("motorTargetVelocity", "Motor/Target Velocity", float, GetMotorTargetVelocity, SetMotorTargetVelocity, 0.f,
        "Target angular velocity (degree/s) of motor", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("maxMotorImpulse", "Motor/Maximum Impulse", float, GetMaxMotorImpulse, SetMaxMotorImpulse, 0.f,
        "Maximum motor impulse", PropertyInfo::EditorFlag);
}

ComWheelJoint::ComWheelJoint() {
}

ComWheelJoint::~ComWheelJoint() {
}

void ComWheelJoint::Init() {
    ComJoint::Init();

    // Mark as initialized
    SetInitialized(true);
}

void ComWheelJoint::CreateConstraint() {
    const ComTransform *transform = GetEntity()->GetTransform();
    const ComRigidBody *rigidBody = GetEntity()->GetComponent<ComRigidBody>();

    // Fill up a constraint description 
    PhysConstraintDesc desc;
    desc.type = PhysConstraint::GenericSpring;
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
    PhysGenericSpringConstraint *genericSpringConstraint = (PhysGenericSpringConstraint *)physicsSystem.CreateConstraint(desc);

    // Apply limit suspension distances
    genericSpringConstraint->SetLinearLowerLimit(Vec3(0, 0, minSusDist));
    genericSpringConstraint->SetLinearUpperLimit(Vec3(0, 0, maxSusDist));
    genericSpringConstraint->EnableLinearLimits(true, true, enableSusLimit);

    // Apply limit steering angles
    genericSpringConstraint->SetAngularLowerLimit(Vec3(0, 0, DEG2RAD(minSteeringAngle)));
    genericSpringConstraint->SetAngularUpperLimit(Vec3(0, 0, DEG2RAD(maxSteeringAngle)));
    genericSpringConstraint->EnableAngularLimits(false, true, enableSteeringLimit);

    // Apply suspension stiffness & damping
    genericSpringConstraint->SetLinearStiffness(Vec3(0, 0, susStiffness));
    genericSpringConstraint->SetLinearDamping(Vec3(0, 0, susDamping));

    // Apply motor
    if (motorTargetVelocity != 0.0f) {
        genericSpringConstraint->SetMotor(Vec3(DEG2RAD(motorTargetVelocity), 0, 0), Vec3(maxMotorImpulse, 0, 0));
        genericSpringConstraint->EnableMotor(true, false, false);
    }

    constraint = genericSpringConstraint;
}

const Vec3 &ComWheelJoint::GetLocalAnchor() const {
    return localAnchor;
}

void ComWheelJoint::SetLocalAnchor(const Vec3 &anchor) {
    this->localAnchor = anchor;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameA(anchor, localAxis);
    }
}

Angles ComWheelJoint::GetLocalAngles() const {
    return localAxis.ToAngles();
}

void ComWheelJoint::SetLocalAngles(const Angles &angles) {
    this->localAxis = angles.ToMat3();
    this->localAxis.FixDegeneracies();

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameA(localAnchor, localAxis);
    }
}

const Vec3 &ComWheelJoint::GetConnectedAnchor() const {
    return connectedAnchor;
}

void ComWheelJoint::SetConnectedAnchor(const Vec3 &anchor) {
    this->connectedAnchor = anchor;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameB(anchor, connectedAxis);
    }
}

Angles ComWheelJoint::GetConnectedAngles() const {
    return connectedAxis.ToAngles();
}

void ComWheelJoint::SetConnectedAngles(const Angles &angles) {
    this->connectedAxis = angles.ToMat3();
    this->connectedAxis.FixDegeneracies();

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameB(connectedAnchor, connectedAxis);
    }
}

void ComWheelJoint::SetEnableSuspensionLimit(bool enable) {
    this->enableSusLimit = enable;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->EnableLinearLimits(true, true, enableSusLimit);
    }
}

void ComWheelJoint::SetMinimumSuspensionDistance(float minSusDist) {
    this->minSusDist = minSusDist;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetLinearLowerLimit(Vec3(0, 0, minSusDist));
    }
}

void ComWheelJoint::SetMaximumSuspensionDistance(float maxSusDist) {
    this->maxSusDist = maxSusDist;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetLinearUpperLimit(Vec3(0, 0, maxSusDist));
    }
}

void ComWheelJoint::SetSuspensionStiffness(float susStiffness) {
    this->susStiffness = susStiffness;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetLinearStiffness(Vec3(0, 0, susStiffness));
    }
}

void ComWheelJoint::SetSuspensionDamping(float susDamping) {
    this->susDamping = susDamping;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetLinearDamping(Vec3(0, 0, susDamping));
    }
}

void ComWheelJoint::SetEnableSteeringLimit(bool enable) {
    this->enableSteeringLimit = enable;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->EnableAngularLimits(false, true, enableSteeringLimit);
    }
}

void ComWheelJoint::SetMinimumSteeringAngle(float angle) {
    this->minSteeringAngle = angle;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularLowerLimit(Vec3(0, 0, DEG2RAD(minSteeringAngle)));
    }
}

void ComWheelJoint::SetMaximumSteeringAngle(float angle) {
    this->maxSteeringAngle = angle;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularUpperLimit(Vec3(0, 0, DEG2RAD(maxSteeringAngle)));
    }
}

void ComWheelJoint::SetMotorTargetVelocity(float motorTargetVelocity) {
    this->motorTargetVelocity = motorTargetVelocity;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetMotor(Vec3(DEG2RAD(motorTargetVelocity), 0, 0), Vec3(maxMotorImpulse, 0, 0));
        ((PhysGenericSpringConstraint *)constraint)->EnableMotor(motorTargetVelocity != 0.0f ? true : false, false, false);
    }
}

void ComWheelJoint::SetMaxMotorImpulse(float maxMotorImpulse) {
    this->maxMotorImpulse = maxMotorImpulse;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetMotor(Vec3(DEG2RAD(motorTargetVelocity), 0, 0), Vec3(maxMotorImpulse, 0, 0));
        ((PhysGenericSpringConstraint *)constraint)->EnableMotor(motorTargetVelocity != 0.0f ? true : false, false, false);
    }
}

void ComWheelJoint::DrawGizmos(const RenderView::State &viewState, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    const ComTransform *transform = GetEntity()->GetTransform();

    if (transform->GetOrigin().DistanceSqr(viewState.origin) < MeterToUnit(500.0f * 500.0f)) {
        Vec3 worldOrigin = transform->GetMatrix() * localAnchor;
        Mat3 worldAxis = transform->GetAxis() * localAxis;

        // Draw wheel circle
        renderWorld->SetDebugColor(Color4::red, Color4::zero);
        renderWorld->DebugCircle(worldOrigin, worldAxis[0], CentiToUnit(5.0f));
        renderWorld->DebugCircle(worldOrigin, worldAxis[0], CentiToUnit(15.0f));

        // Draw axle axis
        renderWorld->SetDebugColor(Color4::red, Color4::zero);
        renderWorld->DebugLine(worldOrigin - worldAxis[0] * CentiToUnit(5.0f), worldOrigin + worldAxis[0] * CentiToUnit(5.0f), 1);

        // Draw forward direction
        renderWorld->SetDebugColor(Color4::green, Color4::zero);
        renderWorld->DebugLine(worldOrigin, worldOrigin - worldAxis[1] * CentiToUnit(5.0f), 1);

        // Draw suspension direction
        renderWorld->SetDebugColor(Color4::blue, Color4::zero);
        renderWorld->DebugLine(worldOrigin, worldOrigin + worldAxis[2] * CentiToUnit(5.0f), 1);
    }
}

BE_NAMESPACE_END
