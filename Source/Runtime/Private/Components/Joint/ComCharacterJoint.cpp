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
#include "Components/Joint/ComCharacterJoint.h"
#include "Components/Physics/ComRigidBody.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Character Joint", ComCharacterJoint, ComJoint)
BEGIN_EVENTS(ComCharacterJoint)
END_EVENTS

void ComCharacterJoint::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("anchor", "Anchor", Vec3, GetAnchor, SetAnchor, Vec3::zero, 
        "Joint position in local space", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_PROPERTY("baseAngles", "Base Angles", Angles, baseAngles, Angles::zero,
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("swing1Axis", "Swing 1/Axis", Vec3, GetSwing1Axis, SetSwing1Axis, Vec3::unitX,
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("swing1Limit", "Swing 1/Limit", float, GetSwing1Limit, SetSwing1Limit, 0, 
        "", PropertyInfo::Flag::Editor).SetRange(0, 179, 1);
    REGISTER_ACCESSOR_PROPERTY("swing1Stiffness", "Swing 1/Stiffness", float, GetSwing1Stiffness, SetSwing1Stiffness, 0.f, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("swing1Damping", "Swing 1/Damping", float, GetSwing1Damping, SetSwing1Damping, 0.2f, 
        "", PropertyInfo::Flag::Editor).SetRange(0, 1, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("swing2Axis", "Swing 2/Axis", Vec3, GetSwing2Axis, SetSwing2Axis, Vec3::unitY,
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("swing2Limit", "Swing 2/Limit", float, GetSwing2Limit, SetSwing2Limit, 0, 
        "", PropertyInfo::Flag::Editor).SetRange(0, 179, 1);
    REGISTER_ACCESSOR_PROPERTY("swing2Stiffness", "Swing 2/Stiffness", float, GetSwing2Stiffness, SetSwing2Stiffness, 0.f, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("swing2Damping", "Swing 2/Damping", float, GetSwing2Damping, SetSwing2Damping, 0.2f, 
        "", PropertyInfo::Flag::Editor).SetRange(0, 1, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("twistLimit", "Twist/Limit", float, GetTwistLimit, SetTwistLimit, 0, 
        "", PropertyInfo::Flag::Editor).SetRange(0, 89, 1);
    REGISTER_ACCESSOR_PROPERTY("twistStiffness", "Twist/Stiffness", float, GetTwistStiffness, SetTwistStiffness, 0.f, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("twistDamping", "Twist/Damping", float, GetTwistDamping, SetTwistDamping, 0.2f, 
        "", PropertyInfo::Flag::Editor).SetRange(0, 1, 0.01f);
}

ComCharacterJoint::ComCharacterJoint() {
}

ComCharacterJoint::~ComCharacterJoint() {
}

void ComCharacterJoint::Init() {
    ComJoint::Init();

    // Mark as initialized
    SetInitialized(true);
}

void ComCharacterJoint::Awake() {
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

void ComCharacterJoint::CreateConstraint() {
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

    genericSpringConstraint->SetAngularStiffness(stiffness);
    // FIXME: Bullet physics's angular damping is not working !!
    genericSpringConstraint->SetAngularDamping(damping);

    // Apply angle limits.
    ApplyAngleLimits();
}

void ComCharacterJoint::ApplyAngleLimits() {
    PhysGenericSpringConstraint *genericSpringConstraint = (PhysGenericSpringConstraint *)constraint;
    if (genericSpringConstraint) {
        Vec3 lowerLimits = *(Vec3 *)baseAngles.Ptr() - angleLimits;
        Vec3 upperLimits = *(Vec3 *)baseAngles.Ptr() + angleLimits;

        genericSpringConstraint->SetAngularLowerLimits(Vec3(DEG2RAD(lowerLimits.x), DEG2RAD(lowerLimits.y), DEG2RAD(lowerLimits.z)));
        genericSpringConstraint->SetAngularUpperLimits(Vec3(DEG2RAD(upperLimits.x), DEG2RAD(upperLimits.y), DEG2RAD(upperLimits.z)));

        genericSpringConstraint->EnableAngularLimits(true, true, true);
    }
}

void ComCharacterJoint::SetAnchor(const Vec3 &anchor) {
    this->anchor = anchor;

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameB(anchor, axis);
    }
}

void ComCharacterJoint::SetAxis(const Mat3 &axis) {
    this->axis = axis;

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameB(anchor, axis);
    }
}

void ComCharacterJoint::SetConnectedAnchor(const Vec3 &anchor) {
    this->connectedAnchor = anchor;

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameA(anchor, connectedAxis);
    }
}

void ComCharacterJoint::SetConnectedAxis(const Mat3 &axis) {
    this->connectedAxis = axis;

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameA(connectedAnchor, connectedAxis);
    }
}

void ComCharacterJoint::SetSwing1Axis(const Vec3 &swing1Axis) {
    this->swing1Axis = swing1Axis;

    axis[0] = swing1Axis;
    axis.OrthoNormalizeSelf();
}

void ComCharacterJoint::SetSwing1Limit(float limit) {
    this->angleLimits.x = limit;

    if (constraint) {
        ApplyAngleLimits();
    }
}

void ComCharacterJoint::SetSwing1Stiffness(float stiffness) {
    this->stiffness.x = stiffness;

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularStiffness(this->stiffness);
    }
}

void ComCharacterJoint::SetSwing1Damping(float damping) {
    this->damping.x = damping;

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularDamping(this->damping);
    }
}

void ComCharacterJoint::SetSwing2Axis(const Vec3 &swing2Axis) {
    this->swing2Axis = swing2Axis;

    axis[1] = swing2Axis;
    axis.OrthoNormalizeSelf();
}

void ComCharacterJoint::SetSwing2Limit(float limit) {
    this->angleLimits.y = limit;

    if (constraint) {
        ApplyAngleLimits();
    }
}

void ComCharacterJoint::SetSwing2Stiffness(float stiffness) {
    this->stiffness.y = stiffness;

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularStiffness(this->stiffness);
    }
}

void ComCharacterJoint::SetSwing2Damping(float damping) {
    this->damping.y = damping;

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularDamping(this->damping);
    }
}

void ComCharacterJoint::SetTwistLimit(float limit) {
    this->angleLimits.z = limit;

    if (constraint) {
        ApplyAngleLimits();
    }
}

void ComCharacterJoint::SetTwistStiffness(float stiffness) {
    this->stiffness.z = stiffness;

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularStiffness(this->stiffness);
    }
}

void ComCharacterJoint::SetTwistDamping(float damping) {
    this->damping.z = damping;

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularDamping(this->damping);
    }
}

#if WITH_EDITOR
void ComCharacterJoint::DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) {
    if (selectedByParent) {
        const ComTransform *transform = GetEntity()->GetTransform();

        if (transform->GetOrigin().DistanceSqr(camera->GetState().origin) < MeterToUnit(100.0f * 100.0f)) {
            Vec3 worldAnchor = transform->GetMatrix().TransformPos(anchor);
            Mat3 worldAxis = transform->GetAxis() * axis;

            float viewScale = camera->CalcClampedViewScale(worldAnchor);

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

            RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

            if (angleLimits.x > 0 || angleLimits.y > 0 || angleLimits.z > 0) {
                Vec3 lowerLimits = *(Vec3 *)baseAngles.Ptr() - angleLimits;
                Vec3 upperLimits = *(Vec3 *)baseAngles.Ptr() + angleLimits;

                if (angleLimits.x > 0) {
                    // Draw rotation limit range in X-axis
                    renderWorld->SetDebugColor(Color4::zero, Color4::red * 0.5f);
                    renderWorld->DebugArc(worldAnchor, worldAxis[1], -worldAxis[2], MeterToUnit(12) * viewScale, lowerLimits.x, upperLimits.x, true);

                    renderWorld->SetDebugColor(Color4::red, Color4::zero);
                    renderWorld->DebugLine(worldAnchor, worldAnchor + constraintWorldAxis[1] * MeterToUnit(20) * viewScale);
                }

                if (angleLimits.y > 0) {
                    // Draw rotation limit range in Y-axis
                    renderWorld->SetDebugColor(Color4::zero, Color4::lime * 0.5f);
                    renderWorld->DebugArc(worldAnchor, worldAxis[2], -worldAxis[0], MeterToUnit(12) * viewScale, lowerLimits.y, upperLimits.y, true);

                    renderWorld->SetDebugColor(Color4::lime, Color4::zero);
                    renderWorld->DebugLine(worldAnchor, worldAnchor + constraintWorldAxis[2] * MeterToUnit(20) * viewScale);
                }

                if (angleLimits.z > 0) {
                    // Draw rotation limit range in Z-axis
                    renderWorld->SetDebugColor(Color4::zero, Color4::blue * 0.5f);
                    renderWorld->DebugArc(worldAnchor, worldAxis[0], -worldAxis[1], MeterToUnit(12) * viewScale, lowerLimits.z, upperLimits.z, true);

                    renderWorld->SetDebugColor(Color4::blue, Color4::zero);
                    renderWorld->DebugLine(worldAnchor, worldAnchor + constraintWorldAxis[0] * MeterToUnit(20) * viewScale);
                }
            }
        }
    }
}
#endif

BE_NAMESPACE_END
