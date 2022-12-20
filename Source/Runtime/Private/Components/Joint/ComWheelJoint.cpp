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
#include "Components/Transform/ComTransform.h"
#include "Components/Joint/ComWheelJoint.h"
#include "Components/Physics/ComRigidBody.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Wheel Joint", ComWheelJoint, ComJoint)
BEGIN_EVENTS(ComWheelJoint)
END_EVENTS

void ComWheelJoint::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("anchor", "Anchor", Vec3, GetAnchor, SetAnchor, Vec3::zero, 
        "Joint position in local space", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_MIXED_ACCESSOR_PROPERTY("angles", "Angles", Angles, GetAngles, SetAngles, Angles::zero,
        "Joint angles in local space", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("useSusLimits", "Suspension/Use Limits", bool, GetEnableSuspensionLimit, SetEnableSuspensionLimit, false, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("minSusDist", "Suspension/Min Distance", float, GetMinimumSuspensionDistance, SetMinimumSuspensionDistance, 0.f, 
        "", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("maxSusDist", "Suspension/Max Distance", float, GetMaximumSuspensionDistance, SetMaximumSuspensionDistance, 0.f, 
        "", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("susStiffness", "Suspension/Stiffness", float, GetSuspensionStiffness, SetSuspensionStiffness, 30.f, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("susDamping", "Suspension/Damping", float, GetSuspensionDamping, SetSuspensionDamping, 0.2f,
        "", PropertyInfo::Flag::Editor);// .SetRange(0, 1, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("useSteeringLimits", "Steering/Use Limits", bool, GetEnableSteeringLimit, SetEnableSteeringLimit, false,
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("minSteeringAngle", "Steering/Min Angle", float, GetMinimumSteeringAngle, SetMinimumSteeringAngle, 0.f, 
        "", PropertyInfo::Flag::Editor).SetRange(-90, 0, 1);
    REGISTER_ACCESSOR_PROPERTY("maxSteeringAngle", "Steering/Max Angle", float, GetMaximumSteeringAngle, SetMaximumSteeringAngle, 0.f, 
        "", PropertyInfo::Flag::Editor).SetRange(0, 90, 1);
    REGISTER_ACCESSOR_PROPERTY("motorTargetVelocity", "Motor/Target Velocity", float, GetMotorTargetVelocity, SetMotorTargetVelocity, 0.f,
        "Target angular velocity (degree/s) of motor", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("maxMotorImpulse", "Motor/Maximum Impulse", float, GetMaxMotorImpulse, SetMaxMotorImpulse, 0.f,
        "Maximum motor impulse", PropertyInfo::Flag::Editor);
}

ComWheelJoint::ComWheelJoint() {
}

ComWheelJoint::~ComWheelJoint() {
}

void ComWheelJoint::Init() {
    ComJoint::Init();

    // Mark as initialized.
    SetInitialized(true);
}

void ComWheelJoint::CreateConstraint() {
    const ComTransform *transform = GetEntity()->GetTransform();
    const ComRigidBody *rigidBody = GetEntity()->GetComponent<ComRigidBody>();
    assert(rigidBody);

    Vec3 scaledAnchor = transform->GetScale() * anchor;

    PhysConstraintDesc desc;
    desc.type = PhysConstraint::Type::GenericSpring;
    desc.collision = collisionEnabled;
    desc.breakImpulse = breakImpulse;

    desc.bodyB = rigidBody->GetBody();
    desc.anchorInB = scaledAnchor;
    desc.axisInB = axis;

    const ComRigidBody *connectedBody = GetConnectedBody();
    if (connectedBody) {
        Vec3 worldAnchor = desc.bodyB->GetOrigin() + desc.bodyB->GetAxis() * scaledAnchor;
        Mat3 worldAxis = desc.bodyB->GetAxis() * axis;

        Mat3 connectedBodyWorldAxis = connectedBody->GetBody()->GetAxis();

        desc.bodyA = connectedBody->GetBody();
        desc.anchorInA = connectedBodyWorldAxis.TransposedMulVec(worldAnchor - connectedBody->GetBody()->GetOrigin());
        desc.axisInA = connectedBodyWorldAxis.TransposedMul(worldAxis);

        connectedAnchor = desc.anchorInB;
        connectedAxis = desc.axisInB;
    } else {
        desc.bodyA = nullptr;

        connectedAnchor = Vec3::origin;
        connectedAxis = Mat3::identity;
    }

    // Create a constraint with the given description.
    PhysGenericSpringConstraint *genericSpringConstraint = (PhysGenericSpringConstraint *)physicsSystem.CreateConstraint(desc);
    constraint = genericSpringConstraint;

    // Apply limit suspension distances.
    genericSpringConstraint->SetLinearLowerLimits(Vec3(0, 0, minSusDist));
    genericSpringConstraint->SetLinearUpperLimits(Vec3(0, 0, maxSusDist));
    genericSpringConstraint->EnableLinearLimits(true, true, enableSusLimit);

    // Apply limit steering angles.
    genericSpringConstraint->SetAngularLowerLimits(Vec3(0, 0, DEG2RAD(minSteeringAngle)));
    genericSpringConstraint->SetAngularUpperLimits(Vec3(0, 0, DEG2RAD(maxSteeringAngle)));
    genericSpringConstraint->EnableAngularLimits(false, true, enableSteeringLimit);

    // Apply suspension stiffness & damping.
    genericSpringConstraint->SetLinearStiffness(Vec3(0, 0, susStiffness));
    genericSpringConstraint->SetLinearDamping(Vec3(0, 0, susDamping));

    // Apply motor.
    if (motorTargetVelocity != 0.0f) {
        genericSpringConstraint->SetMotor(Vec3(DEG2RAD(motorTargetVelocity), 0, 0), Vec3(maxMotorImpulse, 0, 0));
        genericSpringConstraint->EnableMotor(true, false, false);
    }
}

void ComWheelJoint::SetAnchor(const Vec3 &anchor) {
    this->anchor = anchor;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameB(anchor, axis);
    }
}

void ComWheelJoint::SetAngles(const Angles &angles) {
    this->axis = angles.ToMat3();
    this->axis.FixDegeneracies();

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameB(anchor, axis);
    }
}

void ComWheelJoint::SetConnectedAnchor(const Vec3 &anchor) {
    this->connectedAnchor = anchor;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameA(anchor, connectedAxis);
    }
}

void ComWheelJoint::SetConnectedAxis(const Mat3 &axis) {
    this->connectedAxis = axis;
    this->connectedAxis.FixDegeneracies();

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameA(connectedAnchor, connectedAxis);
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
        ((PhysGenericSpringConstraint *)constraint)->SetLinearLowerLimits(Vec3(0, 0, minSusDist));
    }
}

void ComWheelJoint::SetMaximumSuspensionDistance(float maxSusDist) {
    this->maxSusDist = maxSusDist;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetLinearUpperLimits(Vec3(0, 0, maxSusDist));
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
        ((PhysGenericSpringConstraint *)constraint)->SetAngularLowerLimits(Vec3(0, 0, DEG2RAD(minSteeringAngle)));
    }
}

void ComWheelJoint::SetMaximumSteeringAngle(float angle) {
    this->maxSteeringAngle = angle;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularUpperLimits(Vec3(0, 0, DEG2RAD(maxSteeringAngle)));
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

#if WITH_EDITOR
void ComWheelJoint::DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) {
    if (selectedByParent) {
        const ComTransform *transform = GetEntity()->GetTransform();

        if (transform->GetOrigin().DistanceSqr(camera->GetState().origin) < MeterToUnit(100.0f * 100.0f)) {
            Vec3 worldAnchor = transform->GetMatrix().TransformPos(anchor);
            Mat3 worldAxis = transform->GetAxis() * axis;

            float viewScale = camera->CalcClampedViewScale(worldAnchor);

            RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

            // Draw wheel circle
            renderWorld->SetDebugColor(Color4::red, Color4::zero);
            renderWorld->DebugCircle(worldAnchor, worldAxis[0], MeterToUnit(5.0f) * viewScale);
            renderWorld->DebugCircle(worldAnchor, worldAxis[0], MeterToUnit(15.0f) * viewScale);

            // Draw axle axis
            renderWorld->SetDebugColor(Color4::red, Color4::zero);
            renderWorld->DebugLine(
                worldAnchor - worldAxis[0] * MeterToUnit(10.0f) * viewScale,
                worldAnchor + worldAxis[0] * MeterToUnit(10.0f) * viewScale);

            // Draw forward direction
            renderWorld->SetDebugColor(Color4::lime, Color4::zero);
            renderWorld->DebugLine(worldAnchor, worldAnchor - worldAxis[1] * MeterToUnit(10.0f) * viewScale);

            // Draw suspension direction
            renderWorld->SetDebugColor(Color4::blue, Color4::zero);
            renderWorld->DebugLine(worldAnchor, worldAnchor + worldAxis[2] * MeterToUnit(10.0f) * viewScale);
        }
    }
}
#endif

BE_NAMESPACE_END
