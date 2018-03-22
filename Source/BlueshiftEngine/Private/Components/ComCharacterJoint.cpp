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
#include "Components/ComCharacterJoint.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Character Joint", ComCharacterJoint, ComJoint)
BEGIN_EVENTS(ComCharacterJoint)
END_EVENTS

void ComCharacterJoint::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("anchor", "Anchor", Vec3, GetAnchor, SetAnchor, Vec3::zero, "Joint position in local space", PropertyInfo::EditorFlag);
    REGISTER_MIXED_ACCESSOR_PROPERTY("angles", "Angles", Angles, GetAngles, SetAngles, Vec3::zero, "Joint angles in local space", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("swing1LowerLimit", "X Swing Lower Limit", float, GetSwing1LowerLimit, SetSwing1LowerLimit, -45.f, "", PropertyInfo::EditorFlag)
        .SetRange(-180, 0, 1);
    REGISTER_ACCESSOR_PROPERTY("swing1UpperLimit", "X Swing Upper Limit", float, GetSwing1UpperLimit, SetSwing1UpperLimit, 45.f, "", PropertyInfo::EditorFlag)
        .SetRange(0, 180, 1);
    REGISTER_ACCESSOR_PROPERTY("swing1Stiffness", "X Swing Stiffness", float, GetSwing1Stiffness, SetSwing1Stiffness, 0.f, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("swing1Damping", "X Swing Damping", float, GetSwing1Damping, SetSwing1Damping, 0.2f, "", PropertyInfo::EditorFlag)
        .SetRange(0, 1, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("swing2LowerLimit", "Y Swing Lower Limit", float, GetSwing2LowerLimit, SetSwing2LowerLimit, -45.f, "", PropertyInfo::EditorFlag)
        .SetRange(-180, 0, 1);
    REGISTER_ACCESSOR_PROPERTY("swing2UpperLimit", "Y Swing Upper Limit", float, GetSwing2UpperLimit, SetSwing2UpperLimit, 45.f, "", PropertyInfo::EditorFlag)
        .SetRange(0, 180, 1);
    REGISTER_ACCESSOR_PROPERTY("swing2Stiffness", "Y Swing Stiffness", float, GetSwing2Stiffness, SetSwing2Stiffness, 0.f, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("swing2Damping", "Y Swing Damping", float, GetSwing2Damping, SetSwing2Damping, 0.2f, "", PropertyInfo::EditorFlag)
        .SetRange(0, 1, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("twistLowerLimit", "Twist Lower Limit", float, GetTwistLowerLimit, SetTwistLowerLimit, -45.f, "", PropertyInfo::EditorFlag)
        .SetRange(-180, 0, 1);
    REGISTER_ACCESSOR_PROPERTY("twistUpperLimit", "Twist Upper Limit", float, GetTwistUpperLimit, SetTwistUpperLimit, 45.f, "", PropertyInfo::EditorFlag)
        .SetRange(0, 180, 1);
    REGISTER_ACCESSOR_PROPERTY("twistStiffness", "Twist Stiffness", float, GetTwistStiffness, SetTwistStiffness, 0.f, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("twistDamping", "Twist Damping", float, GetTwistDamping, SetTwistDamping, 0.2f, "", PropertyInfo::EditorFlag)
        .SetRange(0, 1, 0.01f);
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

void ComCharacterJoint::Start() {
    ComJoint::Start();

    const ComTransform *transform = GetEntity()->GetTransform();
    const ComRigidBody *rigidBody = GetEntity()->GetComponent<ComRigidBody>();
    assert(rigidBody);

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
    } else {
        desc.bodyB = nullptr;
    }

    // Create a constraint by description
    constraint = physicsSystem.CreateConstraint(&desc);

    PhysGenericSpringConstraint *genericSpringConstraint = static_cast<PhysGenericSpringConstraint *>(constraint);

    genericSpringConstraint->SetAngularStiffness(stiffness);
    genericSpringConstraint->SetAngularDamping(damping);

    // Apply limit angles
    genericSpringConstraint->SetAngularLowerLimit(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
    genericSpringConstraint->SetAngularUpperLimit(Vec3(DEG2RAD(upperLimit.x), DEG2RAD(upperLimit.y), DEG2RAD(upperLimit.z)));
    genericSpringConstraint->EnableAngularLimits(true, true, true);

    if (IsActiveInHierarchy()) {
        constraint->AddToWorld(GetGameWorld()->GetPhysicsWorld());
    }
}

const Vec3 &ComCharacterJoint::GetAnchor() const {
    return localAnchor;
}

void ComCharacterJoint::SetAnchor(const Vec3 &anchor) {
    this->localAnchor = anchor;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameA(anchor, localAxis);
    }
}

Angles ComCharacterJoint::GetAngles() const {
    return localAxis.ToAngles();
}

void ComCharacterJoint::SetAngles(const Angles &angles) {
    this->localAxis = angles.ToMat3();
    this->localAxis.FixDegeneracies();

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameA(localAnchor, localAxis);
    }
}

void ComCharacterJoint::SetSwing1LowerLimit(float limit) {
    this->lowerLimit.x = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularLowerLimit(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
    }
}

void ComCharacterJoint::SetSwing1UpperLimit(float limit) {
    this->upperLimit.x = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularUpperLimit(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
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
        ((PhysGenericSpringConstraint *)constraint)->SetAngularLowerLimit(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
    }
}

void ComCharacterJoint::SetSwing2UpperLimit(float limit) {
    this->upperLimit.y = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularUpperLimit(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
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
        ((PhysGenericSpringConstraint *)constraint)->SetAngularLowerLimit(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
    }
}

void ComCharacterJoint::SetTwistUpperLimit(float limit) {
    this->upperLimit.z = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularUpperLimit(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
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

void ComCharacterJoint::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    const ComTransform *transform = GetEntity()->GetTransform();

    if (transform->GetOrigin().DistanceSqr(sceneView.origin) < 20000.0f * 20000.0f) {
        Vec3 worldOrigin = transform->GetTransform() * localAnchor;
        Mat3 worldAxis = transform->GetAxis() * localAxis;

        Mat3 constraintAxis = Mat3::identity;
        if (connectedBody) {
            constraintAxis = connectedBody->GetEntity()->GetTransform()->GetAxis();
        }

        renderWorld->SetDebugColor(Color4::yellow, Color4::yellow * 0.5f);
        renderWorld->DebugArc(worldOrigin, -constraintAxis[2], -constraintAxis[1], CentiToUnit(2.5), lowerLimit.x, upperLimit.x, true);
        renderWorld->DebugArc(worldOrigin, -constraintAxis[2], +constraintAxis[0], CentiToUnit(2.5), lowerLimit.y, upperLimit.y, true);
        renderWorld->DebugArc(worldOrigin, +constraintAxis[0], -constraintAxis[1], CentiToUnit(2.5), lowerLimit.z, upperLimit.z, true);

        renderWorld->SetDebugColor(Color4::red, Color4::zero);
        renderWorld->DebugLine(worldOrigin, worldOrigin + worldAxis[0] * CentiToUnit(3), 1);
        renderWorld->DebugLine(worldOrigin, worldOrigin - worldAxis[2] * CentiToUnit(3), 1);
    }
}

BE_NAMESPACE_END
