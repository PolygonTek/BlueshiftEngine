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
    REGISTER_ACCESSOR_PROPERTY("anchor", "Anchor", Vec3, GetAnchor, SetAnchor, Vec3::zero, "", PropertyInfo::EditorFlag);
    REGISTER_MIXED_ACCESSOR_PROPERTY("angles", "Angles", Angles, GetAngles, SetAngles, Vec3::zero, "", PropertyInfo::EditorFlag);
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
    desc.type           = PhysConstraint::GenericSpring;
    desc.bodyA          = rigidBody->GetBody();
    desc.axisInA        = axis;
    desc.anchorInA      = transform->GetScale() * anchor;

    if (connectedBody) {
        Vec3 worldAnchor = desc.bodyA->GetOrigin() + desc.bodyA->GetAxis() * desc.anchorInA;

        desc.bodyB      = connectedBody->GetBody();
        desc.axisInB    = axis;
        desc.anchorInB  = connectedBody->GetBody()->GetAxis().TransposedMulVec(worldAnchor - connectedBody->GetBody()->GetOrigin());
    } else {
        desc.bodyB      = nullptr;
    }

    desc.collision      = collisionEnabled;
    desc.breakImpulse   = breakImpulse;

    // Create a constraint by description
    constraint = physicsSystem.CreateConstraint(&desc);

    PhysGenericSpringConstraint *genericSpringConstraint = static_cast<PhysGenericSpringConstraint *>(constraint);
    genericSpringConstraint->SetAngularLowerLimit(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
    genericSpringConstraint->SetAngularUpperLimit(Vec3(DEG2RAD(upperLimit.x), DEG2RAD(upperLimit.y), DEG2RAD(upperLimit.z)));
    genericSpringConstraint->SetAngularStiffness(stiffness);
    genericSpringConstraint->SetAngularDamping(damping);

    if (IsActiveInHierarchy()) {
        genericSpringConstraint->AddToWorld(GetGameWorld()->GetPhysicsWorld());
    }
}

void ComCharacterJoint::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    const ComTransform *transform = GetEntity()->GetTransform();

    if (transform->GetOrigin().DistanceSqr(sceneView.origin) < 20000.0f * 20000.0f) {
        Vec3 worldOrigin = transform->GetTransform() * anchor;
        Mat3 worldAxis = transform->GetAxis() * axis;

        renderWorld->SetDebugColor(Color4::red, Color4::zero);
        renderWorld->DebugLine(worldOrigin - worldAxis[0] * CentiToUnit(5), worldOrigin + worldAxis[0] * CentiToUnit(5), 1);
        renderWorld->DebugLine(worldOrigin - worldAxis[1] * CentiToUnit(5), worldOrigin + worldAxis[1] * CentiToUnit(5), 1);
        renderWorld->DebugLine(worldOrigin - worldAxis[2] * CentiToUnit(5), worldOrigin + worldAxis[2] * CentiToUnit(5), 1);
    }
}

const Vec3 &ComCharacterJoint::GetAnchor() const {
    return anchor;
}

void ComCharacterJoint::SetAnchor(const Vec3 &anchor) {
    this->anchor = anchor;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameA(anchor, axis);
    }
}

Angles ComCharacterJoint::GetAngles() const {
    return axis.ToAngles();
}

void ComCharacterJoint::SetAngles(const Angles &angles) {
    this->axis = angles.ToMat3();
    this->axis.FixDegeneracies();

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameA(anchor, axis);
    }
}

float ComCharacterJoint::GetSwing1LowerLimit() const {
    return lowerLimit.x;
}

void ComCharacterJoint::SetSwing1LowerLimit(float limit) {
    this->lowerLimit.x = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularLowerLimit(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
    }
}

float ComCharacterJoint::GetSwing1UpperLimit() const {
    return upperLimit.x;
}

void ComCharacterJoint::SetSwing1UpperLimit(float limit) {
    this->upperLimit.x = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularUpperLimit(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
    }
}

float ComCharacterJoint::GetSwing1Stiffness() const {
    return stiffness.x;
}

void ComCharacterJoint::SetSwing1Stiffness(float stiffness) {
    this->stiffness.x = stiffness;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularStiffness(this->stiffness);
    }
}

float ComCharacterJoint::GetSwing1Damping() const {
    return damping.x;
}

void ComCharacterJoint::SetSwing1Damping(float damping) {
    this->damping.x = damping;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularDamping(this->damping);
    }
}

float ComCharacterJoint::GetSwing2LowerLimit() const {
    return lowerLimit.y;
}

void ComCharacterJoint::SetSwing2LowerLimit(float limit) {
    this->lowerLimit.y = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularLowerLimit(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
    }
}

float ComCharacterJoint::GetSwing2UpperLimit() const {
    return upperLimit.y;
}

void ComCharacterJoint::SetSwing2UpperLimit(float limit) {
    this->upperLimit.y = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularUpperLimit(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
    }
}

float ComCharacterJoint::GetSwing2Stiffness() const {
    return stiffness.y;
}

void ComCharacterJoint::SetSwing2Stiffness(float stiffness) {
    this->stiffness.y = stiffness;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularStiffness(this->stiffness);
    }
}

float ComCharacterJoint::GetSwing2Damping() const {
    return damping.y;
}

void ComCharacterJoint::SetSwing2Damping(float damping) {
    this->damping.y = damping;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularDamping(this->damping);
    }
}

float ComCharacterJoint::GetTwistLowerLimit() const {
    return lowerLimit.z;
}

void ComCharacterJoint::SetTwistLowerLimit(float limit) {
    this->lowerLimit.z = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularLowerLimit(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
    }
}

float ComCharacterJoint::GetTwistUpperLimit() const {
    return upperLimit.z;
}

void ComCharacterJoint::SetTwistUpperLimit(float limit) {
    this->upperLimit.z = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularUpperLimit(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
    }
}

float ComCharacterJoint::GetTwistStiffness() const {
    return stiffness.z;
}

void ComCharacterJoint::SetTwistStiffness(float stiffness) {
    this->stiffness.z = stiffness;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularStiffness(this->stiffness);
    }
}

float ComCharacterJoint::GetTwistDamping() const {
    return damping.z;
}

void ComCharacterJoint::SetTwistDamping(float damping) {
    this->damping.z = damping;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularDamping(this->damping);
    }
}

BE_NAMESPACE_END
