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
#include "Components/Joint/ComSliderJoint.h"
#include "Components/ComRigidBody.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Slider Joint", ComSliderJoint, ComJoint)
BEGIN_EVENTS(ComSliderJoint)
END_EVENTS

void ComSliderJoint::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("anchor", "Anchor", Vec3, GetAnchor, SetAnchor, Vec3::zero, 
        "Joint position in local space", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_MIXED_ACCESSOR_PROPERTY("angles", "Angles", Angles, GetAngles, SetAngles, Angles::zero,
        "Joint angles in local space", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("useLinearLimits", "Linear Limits/Use Limits", bool, GetEnableLimitDistances, SetEnableLimitDistances, false, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("minDist", "Linear Limits/Minimum Distance", float, GetMinimumDistance, SetMinimumDistance, 0.f, 
        "", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("maxDist", "Linear Limits/Maximum Distance", float, GetMaximumDistance, SetMaximumDistance, 0.f, 
        "", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("useAngularLimits", "Angular Limits/Use Limits", bool, GetEnableLimitAngles, SetEnableLimitAngles, true, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("minAngle", "Angular Limits/Minimum Angle", float, GetMinimumAngle, SetMinimumAngle, 0.f, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("maxAngle", "Angular Limits/Maximum Angle", float, GetMaximumAngle, SetMaximumAngle, 0.f, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("linearMotorTargetVelocity", "Linear Motor/Target Velocity", float, GetLinearMotorTargetVelocity, SetLinearMotorTargetVelocity, 0.f, 
        "Target linear velocity (m/s) of motor", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("maxLinearMotorImpulse", "Linear Motor/Maximum Impulse", float, GetMaxLinearMotorImpulse, SetMaxLinearMotorImpulse, 0.f, 
        "Maximum linear motor impulse", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("angularMotorTargetVelocity", "Angular Motor/Target Velocity", float, GetAngularMotorTargetVelocity, SetAngularMotorTargetVelocity, 0.f, 
        "Target angular velocity (degree/s) of motor", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("maxAngularMotorImpulse", "Angular Motor/Maximum Impulse", float, GetMaxAngularMotorImpulse, SetMaxAngularMotorImpulse, 0.f, 
        "Maximum angular motor impulse", PropertyInfo::Flag::Editor);
}

ComSliderJoint::ComSliderJoint() {
}

ComSliderJoint::~ComSliderJoint() {
}

void ComSliderJoint::Init() {
    ComJoint::Init();

    // Mark as initialized.
    SetInitialized(true);
}

void ComSliderJoint::Awake() {
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

void ComSliderJoint::CreateConstraint() {
    const ComTransform *transform = GetEntity()->GetTransform();
    const ComRigidBody *rigidBody = GetEntity()->GetComponent<ComRigidBody>();
    assert(rigidBody);

    Vec3 scaledAnchor = transform->GetScale() * anchor;

    PhysConstraintDesc desc;
    desc.type = PhysConstraint::Type::Slider;
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
    PhysSliderConstraint *sliderConstraint = (PhysSliderConstraint *)physicsSystem.CreateConstraint(desc);
    constraint = sliderConstraint;

    // Apply limit distances.
    sliderConstraint->SetLinearLimits(minDist, maxDist);
    sliderConstraint->EnableLinearLimits(enableLimitDistances);

    // Apply limit angles.
    sliderConstraint->SetAngularLimits(DEG2RAD(minAngle), DEG2RAD(maxAngle));
    sliderConstraint->EnableAngularLimits(enableLimitAngles);

    // Apply linear motor.
    if (linearMotorTargetVelocity != 0.0f) {
        sliderConstraint->SetLinearMotor(linearMotorTargetVelocity, maxLinearMotorImpulse / GetGameWorld()->GetPhysicsWorld()->GetFrameTimeDelta());
        sliderConstraint->EnableLinearMotor(true);
    }

    // Apply angular motor.
    if (angularMotorTargetVelocity != 0.0f) {
        sliderConstraint->SetAngularMotor(DEG2RAD(angularMotorTargetVelocity), maxAngularMotorImpulse / GetGameWorld()->GetPhysicsWorld()->GetFrameTimeDelta());
        sliderConstraint->EnableAngularMotor(true);
    }
}

void ComSliderJoint::SetAnchor(const Vec3 &anchor) {
    this->anchor = anchor;
    if (constraint) {
        ((PhysSliderConstraint *)constraint)->SetFrameB(anchor, axis);
    }
}

void ComSliderJoint::SetAngles(const Angles &angles) {
    this->axis = angles.ToMat3();
    this->axis.FixDegeneracies();

    if (constraint) {
        ((PhysSliderConstraint *)constraint)->SetFrameB(anchor, axis);
    }
}

void ComSliderJoint::SetConnectedAnchor(const Vec3 &anchor) {
    this->connectedAnchor = anchor;

    if (constraint) {
        ((PhysSliderConstraint *)constraint)->SetFrameA(anchor, connectedAxis);
    }
}

void ComSliderJoint::SetConnectedAxis(const Mat3 &axis) {
    this->connectedAxis = axis;

    if (constraint) {
        ((PhysSliderConstraint *)constraint)->SetFrameA(connectedAnchor, connectedAxis);
    }
}

void ComSliderJoint::SetEnableLimitDistances(bool enable) {
    this->enableLimitDistances = enable;

    if (constraint) {
        ((PhysSliderConstraint *)constraint)->EnableLinearLimits(enableLimitDistances);
    }
}

void ComSliderJoint::SetMinimumDistance(float minDist) {
    this->minDist = minDist;

    if (constraint) {
        ((PhysSliderConstraint *)constraint)->SetLinearLimits(minDist, maxDist);
    }
}

void ComSliderJoint::SetMaximumDistance(float maxDist) {
    this->maxDist = maxDist;

    if (constraint) {
        ((PhysSliderConstraint *)constraint)->SetLinearLimits(minDist, maxDist);
    }
}

void ComSliderJoint::SetEnableLimitAngles(bool enable) {
    this->enableLimitAngles = enable;

    if (constraint) {
        ((PhysSliderConstraint *)constraint)->EnableAngularLimits(enableLimitAngles);
    }
}

void ComSliderJoint::SetMinimumAngle(float angle) {
    this->minAngle = angle;

    if (constraint) {
        ((PhysSliderConstraint *)constraint)->SetAngularLimits(DEG2RAD(minAngle), DEG2RAD(maxAngle));
    }
}

void ComSliderJoint::SetMaximumAngle(float angle) {
    this->maxAngle = angle;

    if (constraint) {
        ((PhysSliderConstraint *)constraint)->SetAngularLimits(DEG2RAD(minAngle), DEG2RAD(maxAngle));
    }
}

void ComSliderJoint::SetLinearMotorTargetVelocity(float linearMotorTargetVelocity) {
    this->linearMotorTargetVelocity = linearMotorTargetVelocity;

    if (constraint) {
        ((PhysSliderConstraint *)constraint)->SetLinearMotor(DEG2RAD(linearMotorTargetVelocity), maxLinearMotorImpulse / GetGameWorld()->GetPhysicsWorld()->GetFrameTimeDelta());
        ((PhysSliderConstraint *)constraint)->EnableLinearMotor(linearMotorTargetVelocity != 0.0f ? true : false);
    }
}

void ComSliderJoint::SetMaxLinearMotorImpulse(float maxLinearMotorImpulse) {
    this->maxLinearMotorImpulse = maxLinearMotorImpulse;

    if (constraint) {
        ((PhysSliderConstraint *)constraint)->SetLinearMotor(DEG2RAD(linearMotorTargetVelocity), maxLinearMotorImpulse / GetGameWorld()->GetPhysicsWorld()->GetFrameTimeDelta());
        ((PhysSliderConstraint *)constraint)->EnableLinearMotor(linearMotorTargetVelocity != 0.0f ? true : false);
    }
}

void ComSliderJoint::SetAngularMotorTargetVelocity(float angularMotorTargetVelocity) {
    this->angularMotorTargetVelocity = angularMotorTargetVelocity;

    if (constraint) {
        ((PhysSliderConstraint *)constraint)->SetAngularMotor(DEG2RAD(angularMotorTargetVelocity), maxAngularMotorImpulse / GetGameWorld()->GetPhysicsWorld()->GetFrameTimeDelta());
        ((PhysSliderConstraint *)constraint)->EnableAngularMotor(angularMotorTargetVelocity != 0.0f ? true : false);
    }
}

void ComSliderJoint::SetMaxAngularMotorImpulse(float maxAngularMotorImpulse) {
    this->maxAngularMotorImpulse = maxAngularMotorImpulse;

    if (constraint) {
        ((PhysSliderConstraint *)constraint)->SetAngularMotor(DEG2RAD(angularMotorTargetVelocity), maxAngularMotorImpulse / GetGameWorld()->GetPhysicsWorld()->GetFrameTimeDelta());
        ((PhysSliderConstraint *)constraint)->EnableAngularMotor(angularMotorTargetVelocity != 0.0f ? true : false);
    }
}

#if WITH_EDITOR
void ComSliderJoint::DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) {
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

                renderWorld->SetDebugColor(Color4::red * 0.5f, Color4::red * 0.5f);
                renderWorld->DebugArc(worldAnchor, worldAxis[1], -worldAxis[2], MeterToUnit(12) * viewScale, minAngle, maxAngle, true);

                renderWorld->SetDebugColor(Color4::red, Color4::zero);
                renderWorld->DebugLine(worldAnchor, worldAnchor + constraintWorldAxis[1] * MeterToUnit(20) * viewScale);
            }

            renderWorld->SetDebugColor(Color4::red, Color4::red);
            renderWorld->DebugArrow(worldAnchor, worldAnchor + worldAxis[0] * MeterToUnit(10) * viewScale, MeterToUnit(6) * viewScale, MeterToUnit(1) * viewScale);
            renderWorld->DebugArrow(worldAnchor, worldAnchor - worldAxis[0] * MeterToUnit(10) * viewScale, MeterToUnit(6) * viewScale, MeterToUnit(1) * viewScale);
        }
    }
}
#endif

BE_NAMESPACE_END
