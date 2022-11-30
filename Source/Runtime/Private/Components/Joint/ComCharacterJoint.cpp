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
    REGISTER_ACCESSOR_PROPERTY("anchor", "Anchor", Vec3, GetLocalAnchor, SetLocalAnchor, Vec3::zero, 
        "Joint position in local space", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_MIXED_ACCESSOR_PROPERTY("angles", "Angles", Angles, GetLocalAngles, SetLocalAngles, Angles::zero, 
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
    localAxis = Mat3::identity;
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

    const ComTransform *transform = GetEntity()->GetTransform();
    startLocalAxis = transform->GetLocalAxis() * localAxis;
}

void ComCharacterJoint::CreateConstraint() {
    const ComTransform *transform = GetEntity()->GetTransform();
    const ComRigidBody *rigidBody = GetEntity()->GetComponent<ComRigidBody>();
    assert(rigidBody);

    Vec3 scaledLocalAnchor = transform->GetScale() * localAnchor;

    PhysConstraintDesc desc;
    desc.type = PhysConstraint::Type::GenericSpring;
    desc.collision = collisionEnabled;
    desc.breakImpulse = breakImpulse;

    desc.bodyB = rigidBody->GetBody();
    desc.anchorInB = scaledLocalAnchor;
    desc.axisInB = localAxis;

    const ComRigidBody *connectedBody = GetConnectedBody();
    if (connectedBody) {
        Vec3 worldAnchor = desc.bodyB->GetOrigin() + desc.bodyB->GetAxis() * scaledLocalAnchor;
        Mat3 worldAxis = desc.bodyB->GetAxis() * localAxis;

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

const Vec3 &ComCharacterJoint::GetLocalAnchor() const {
    return localAnchor;
}

void ComCharacterJoint::SetLocalAnchor(const Vec3 &anchor) {
    this->localAnchor = anchor;

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameB(anchor, localAxis);
    }
}

Angles ComCharacterJoint::GetLocalAngles() const {
    return localAxis.ToAngles();
}

void ComCharacterJoint::SetLocalAngles(const Angles &angles) {
    this->localAxis = angles.ToMat3();
    this->localAxis.FixDegeneracies();

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameB(localAnchor, localAxis);
    }
}

const Vec3 &ComCharacterJoint::GetConnectedAnchor() const {
    return connectedAnchor;
}

void ComCharacterJoint::SetConnectedAnchor(const Vec3 &anchor) {
    this->connectedAnchor = anchor;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameA(anchor, connectedAxis);
    }
}

Angles ComCharacterJoint::GetConnectedAngles() const {
    return connectedAxis.ToAngles();
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
            Vec3 worldAnchor = transform->GetWorldMatrix().TransformPos(localAnchor);
            Mat3 worldAxis = transform->GetAxis() * localAxis;

            float viewScale = camera->CalcViewScale(worldAnchor);

            if (!constraint) {
                startLocalAxis = transform->GetLocalAxis() * localAxis;
            }

            Mat3 constraintAxis;
            const ComRigidBody *connectedBody = GetConnectedBody();
            if (connectedBody) {
                constraintAxis = connectedBody->GetEntity()->GetTransform()->GetAxis() * startLocalAxis;
            } else {
                constraintAxis = startLocalAxis;
            }

            RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

            renderWorld->SetDebugColor(Color4::red, Color4::red * 0.5f);
            renderWorld->DebugArc(worldAnchor, worldAxis[1], -worldAxis[2], MeterToUnit(12) * viewScale, lowerLimit.x, upperLimit.x, true);

            renderWorld->SetDebugColor(Color4::green, Color4::green * 0.5f);
            renderWorld->DebugArc(worldAnchor, worldAxis[2], -worldAxis[0], MeterToUnit(12) * viewScale, lowerLimit.y, upperLimit.y, true);

            renderWorld->SetDebugColor(Color4::blue, Color4::blue * 0.5f);
            renderWorld->DebugArc(worldAnchor, worldAxis[0], -worldAxis[1], MeterToUnit(12) * viewScale, lowerLimit.z, upperLimit.z, true);

            renderWorld->SetDebugColor(Color4::red, Color4::zero);
            renderWorld->DebugLine(worldAnchor, worldAnchor + constraintAxis[1] * MeterToUnit(18) * viewScale);

            renderWorld->SetDebugColor(Color4::green, Color4::zero);
            renderWorld->DebugLine(worldAnchor, worldAnchor + constraintAxis[2] * MeterToUnit(18) * viewScale);

            renderWorld->SetDebugColor(Color4::blue, Color4::zero);
            renderWorld->DebugLine(worldAnchor, worldAnchor + constraintAxis[0] * MeterToUnit(18) * viewScale);
        }
    }
}
#endif

BE_NAMESPACE_END
