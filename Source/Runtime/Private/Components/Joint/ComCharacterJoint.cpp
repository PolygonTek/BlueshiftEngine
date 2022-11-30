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
#include "Components/ComRigidBody.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Character Joint", ComCharacterJoint, ComJoint)
BEGIN_EVENTS(ComCharacterJoint)
END_EVENTS

void ComCharacterJoint::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("anchor", "Anchor", Vec3, GetAnchor, SetAnchor, Vec3::zero, 
        "Joint position in local space", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_MIXED_ACCESSOR_PROPERTY("angles", "Angles", Angles, GetAngles, SetAngles, Angles::zero, 
        "Joint angles in local space", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("swing1LowerLimit", "X Swing/Lower Limit", float, GetSwing1LowerLimit, SetSwing1LowerLimit, -45.f, 
        "", PropertyInfo::Flag::Editor).SetRange(-177, 0, 1);
    REGISTER_ACCESSOR_PROPERTY("swing1UpperLimit", "X Swing/Upper Limit", float, GetSwing1UpperLimit, SetSwing1UpperLimit, 45.f, 
        "", PropertyInfo::Flag::Editor).SetRange(0, 177, 1);
    REGISTER_ACCESSOR_PROPERTY("swing1Stiffness", "X Swing/Stiffness", float, GetSwing1Stiffness, SetSwing1Stiffness, 0.f, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("swing1Damping", "X Swing/Damping", float, GetSwing1Damping, SetSwing1Damping, 0.2f, 
        "", PropertyInfo::Flag::Editor).SetRange(0, 1, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("swing2LowerLimit", "Y Swing/Lower Limit", float, GetSwing2LowerLimit, SetSwing2LowerLimit, -45.f, 
        "", PropertyInfo::Flag::Editor).SetRange(-177, 0, 1);
    REGISTER_ACCESSOR_PROPERTY("swing2UpperLimit", "Y Swing/Upper Limit", float, GetSwing2UpperLimit, SetSwing2UpperLimit, 45.f, 
        "", PropertyInfo::Flag::Editor).SetRange(0, 177, 1);
    REGISTER_ACCESSOR_PROPERTY("swing2Stiffness", "Y Swing/Stiffness", float, GetSwing2Stiffness, SetSwing2Stiffness, 0.f, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("swing2Damping", "Y Swing/Damping", float, GetSwing2Damping, SetSwing2Damping, 0.2f, 
        "", PropertyInfo::Flag::Editor).SetRange(0, 1, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("twistLowerLimit", "Twist/Lower Limit", float, GetTwistLowerLimit, SetTwistLowerLimit, -45.f, 
        "", PropertyInfo::Flag::Editor).SetRange(-45, 0, 1);
    REGISTER_ACCESSOR_PROPERTY("twistUpperLimit", "Twist/Upper Limit", float, GetTwistUpperLimit, SetTwistUpperLimit, 45.f, 
        "", PropertyInfo::Flag::Editor).SetRange(0, 45, 1);
    REGISTER_ACCESSOR_PROPERTY("twistStiffness", "Twist/Stiffness", float, GetTwistStiffness, SetTwistStiffness, 0.f, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("twistDamping", "Twist/Damping", float, GetTwistDamping, SetTwistDamping, 0.2f, 
        "", PropertyInfo::Flag::Editor).SetRange(0, 1, 0.01f);
}

ComCharacterJoint::ComCharacterJoint() {
    axis = Mat3::identity;
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

    genericSpringConstraint->SetAngularStiffness(stiffness);
    genericSpringConstraint->SetAngularDamping(damping);

    // Apply limit angles.
    genericSpringConstraint->SetAngularLowerLimits(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
    genericSpringConstraint->SetAngularUpperLimits(Vec3(DEG2RAD(upperLimit.x), DEG2RAD(upperLimit.y), DEG2RAD(upperLimit.z)));
    genericSpringConstraint->EnableAngularLimits(true, true, true);

    constraint = genericSpringConstraint;
}

void ComCharacterJoint::SetAnchor(const Vec3 &anchor) {
    this->anchor = anchor;

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameB(anchor, axis);
    }
}

void ComCharacterJoint::SetAngles(const Angles &angles) {
    this->axis = angles.ToMat3();
    this->axis.FixDegeneracies();

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

void ComCharacterJoint::SetConnectedAngles(const Angles &angles) {
    this->connectedAxis = angles.ToMat3();
    this->connectedAxis.FixDegeneracies();

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameA(connectedAnchor, connectedAxis);
    }
}

void ComCharacterJoint::SetSwing1LowerLimit(float limit) {
    this->lowerLimit.x = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularLowerLimits(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
    }
}

void ComCharacterJoint::SetSwing1UpperLimit(float limit) {
    this->upperLimit.x = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularUpperLimits(Vec3(DEG2RAD(upperLimit.x), DEG2RAD(upperLimit.y), DEG2RAD(upperLimit.z)));
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

void ComCharacterJoint::SetSwing2LowerLimit(float limit) {
    this->lowerLimit.y = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularLowerLimits(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
    }
}

void ComCharacterJoint::SetSwing2UpperLimit(float limit) {
    this->upperLimit.y = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularUpperLimits(Vec3(DEG2RAD(upperLimit.x), DEG2RAD(upperLimit.y), DEG2RAD(upperLimit.z)));
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

void ComCharacterJoint::SetTwistLowerLimit(float limit) {
    this->lowerLimit.z = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularLowerLimits(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
    }
}

void ComCharacterJoint::SetTwistUpperLimit(float limit) {
    this->upperLimit.z = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularUpperLimits(Vec3(DEG2RAD(upperLimit.x), DEG2RAD(upperLimit.y), DEG2RAD(upperLimit.z)));
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

            if (upperLimit.x > lowerLimit.x) {
                // Draw rotation limit range in X-axis
                renderWorld->SetDebugColor(Color4::red * 0.5f, Color4::red * 0.5f);
                renderWorld->DebugArc(worldAnchor, worldAxis[1], -worldAxis[2], MeterToUnit(12) * viewScale, lowerLimit.x, upperLimit.x, true);

                renderWorld->SetDebugColor(Color4::red, Color4::zero);
                renderWorld->DebugLine(worldAnchor, worldAnchor + constraintWorldAxis[1] * MeterToUnit(20) * viewScale);
            }

            if (upperLimit.y > lowerLimit.y) {
                // Draw rotation limit range in Y-axis
                renderWorld->SetDebugColor(Color4::lime * 0.5f, Color4::lime * 0.5f);
                renderWorld->DebugArc(worldAnchor, worldAxis[2], -worldAxis[0], MeterToUnit(12) * viewScale, lowerLimit.y, upperLimit.y, true);

                renderWorld->SetDebugColor(Color4::lime, Color4::zero);
                renderWorld->DebugLine(worldAnchor, worldAnchor + constraintWorldAxis[2] * MeterToUnit(20) * viewScale);
            }

            if (upperLimit.z > lowerLimit.z) {
                // Draw rotation limit range in Z-axis
                renderWorld->SetDebugColor(Color4::blue * 0.5f, Color4::blue * 0.5f);
                renderWorld->DebugArc(worldAnchor, worldAxis[0], -worldAxis[1], MeterToUnit(12) * viewScale, lowerLimit.z, upperLimit.z, true);

                renderWorld->SetDebugColor(Color4::blue, Color4::zero);
                renderWorld->DebugLine(worldAnchor, worldAnchor + constraintWorldAxis[0] * MeterToUnit(20) * viewScale);
            }
        }
    }
}
#endif

BE_NAMESPACE_END
