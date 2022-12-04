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
#include "Components/Joint/ComHingeJoint.h"
#include "Components/Physics/ComRigidBody.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Hinge Joint", ComHingeJoint, ComJoint)
BEGIN_EVENTS(ComHingeJoint)
END_EVENTS

void ComHingeJoint::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("anchor", "Anchor", Vec3, GetAnchor, SetAnchor, Vec3::zero, 
        "Joint position in local space", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_MIXED_ACCESSOR_PROPERTY("angles", "Angles", Angles, GetAngles, SetAngles, Angles::zero, 
        "Joint angles in local space", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("useLimits", "Limits/Use Limits", bool, GetEnableLimitAngles, SetEnableLimitAngles, false, 
        "Activate joint limits", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("minAngle", "Limits/Minimum Angle", float, GetMinimumAngle, SetMinimumAngle, 0.f, 
        "Minimum value of joint angle", PropertyInfo::Flag::Editor).SetRange(-180, 0, 1);
    REGISTER_ACCESSOR_PROPERTY("maxAngle", "Limits/Maximum Angle", float, GetMaximumAngle, SetMaximumAngle, 0.f, 
        "Maximum value of joint angle", PropertyInfo::Flag::Editor).SetRange(0, 180, 1);
    REGISTER_ACCESSOR_PROPERTY("motorTargetVelocity", "Motor/Target Velocity", float, GetMotorTargetVelocity, SetMotorTargetVelocity, 0.f, 
        "Target angular velocity (degree/s) of motor", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("maxMotorImpulse", "Motor/Maximum Impulse", float, GetMaxMotorImpulse, SetMaxMotorImpulse, 0.f, 
        "Maximum motor impulse", PropertyInfo::Flag::Editor);
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

void ComHingeJoint::Awake() {
    ComJoint::Awake();

#if WITH_EDITOR
    const ComTransform *transform = GetEntity()->GetTransform();
    startLocalAxisInConnectedBody = transform->GetAxis() * axis;

    const ComRigidBody *connectedBody = GetConnectedBody();
    if (connectedBody) {
        startLocalAxisInConnectedBody = connectedBody->GetEntity()->GetTransform()->GetAxis().TransposedMul(startLocalAxisInConnectedBody);
    }
#endif
}

void ComHingeJoint::CreateConstraint() {
    const ComTransform *transform = GetEntity()->GetTransform();
    const ComRigidBody *rigidBody = GetEntity()->GetComponent<ComRigidBody>();
    assert(rigidBody);

    Vec3 scaledAnchor = transform->GetScale() * anchor;

    PhysConstraintDesc desc;
    desc.type = PhysConstraint::Type::Hinge;
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
    PhysHingeConstraint *hingeConstraint = (PhysHingeConstraint *)physicsSystem.CreateConstraint(desc);
    constraint = hingeConstraint;

    // Apply limit angles.
    hingeConstraint->SetAngularLimits(DEG2RAD(minAngle), DEG2RAD(maxAngle));
    hingeConstraint->EnableAngularLimits(enableLimitAngles);

    // Apply motor.
    if (motorTargetVelocity != 0.0f) {
        hingeConstraint->SetMotor(DEG2RAD(motorTargetVelocity), maxMotorImpulse);
        hingeConstraint->EnableMotor(true);
    }
}

void ComHingeJoint::SetAnchor(const Vec3 &anchor) {
    this->anchor = anchor;

    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetFrameB(anchor, axis);
    }
}

void ComHingeJoint::SetAngles(const Angles &angles) {
    this->axis = angles.ToMat3();
    this->axis.FixDegeneracies();

    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetFrameB(anchor, axis);
    }
}

void ComHingeJoint::SetConnectedAnchor(const Vec3 &anchor) {
    this->connectedAnchor = anchor;
    if (constraint) {
        ((PhysP2PConstraint *)constraint)->SetAnchorA(anchor);
    }
}

void ComHingeJoint::SetConnectedAxis(const Mat3 &axis) {
    this->connectedAxis = axis;

    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetFrameA(connectedAnchor, connectedAxis);
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

#if WITH_EDITOR
void ComHingeJoint::DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) {
    if (selectedByParent) {
        const ComTransform *transform = GetEntity()->GetTransform();

        if (transform->GetOrigin().DistanceSqr(camera->GetState().origin) < MeterToUnit(100.0f * 100.0f)) {
            Vec3 worldAnchor = transform->GetMatrix().TransformPos(anchor);
            Mat3 worldAxis = transform->GetAxis() * axis;

            float viewScale = camera->CalcClampedViewScale(worldAnchor);

            RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

            if (enableLimitAngles) {
                Mat3 constraintWorldAxis;
                if (constraint) {
                    const ComRigidBody *connectedBody = GetConnectedBody();
                    if (connectedBody) {
                        constraintWorldAxis = connectedBody->GetEntity()->GetTransform()->GetAxis() * startLocalAxisInConnectedBody;
                    } else {
                        constraintWorldAxis = startLocalAxisInConnectedBody;
                    }
                } else {
                    constraintWorldAxis = transform->GetAxis() * axis;
                }

                renderWorld->SetDebugColor(Color4::blue * 0.5f, Color4::blue * 0.5f);
                renderWorld->DebugArc(worldAnchor, worldAxis[0], -worldAxis[1], MeterToUnit(12) * viewScale, minAngle, maxAngle, true);

                renderWorld->SetDebugColor(Color4::blue, Color4::zero);
                renderWorld->DebugLine(worldAnchor, worldAnchor + constraintWorldAxis[0] * MeterToUnit(20) * viewScale);
            }

            renderWorld->SetDebugColor(Color4::red, Color4::red);
            renderWorld->DebugArrow(worldAnchor, worldAnchor + worldAxis[2] * MeterToUnit(10) * viewScale, MeterToUnit(6) * viewScale, MeterToUnit(1.5) * viewScale);
        }
    }
}
#endif

BE_NAMESPACE_END
