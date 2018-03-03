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
#include "Components/ComSpringJoint.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Spring Joint", ComSpringJoint, ComJoint)
BEGIN_EVENTS(ComSpringJoint)
END_EVENTS

void ComSpringJoint::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("anchor", "Anchor", Vec3, GetAnchor, SetAnchor, Vec3::zero, "", PropertyInfo::EditorFlag);
    REGISTER_MIXED_ACCESSOR_PROPERTY("angles", "Angles", Angles, GetAngles, SetAngles, Vec3::zero, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("lowerLimit", "Lower Limit", float, GetLowerLimit, SetLowerLimit, 0.f, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("upperLimit", "Upper Limit", float, GetUpperLimit, SetUpperLimit, 0.f, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("stiffness", "Stiffness", float, GetStiffness, SetStiffness, 2.f, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("damping", "Damping", float, GetDamping, SetDamping, 0.2f, "", PropertyInfo::EditorFlag)
        .SetRange(0, 1, 0.01f);
}

ComSpringJoint::ComSpringJoint() {
}

ComSpringJoint::~ComSpringJoint() {
}

void ComSpringJoint::Init() {
    ComJoint::Init();

    // Mark as initialized
    SetInitialized(true);
}

void ComSpringJoint::Start() {
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
    genericSpringConstraint->SetLinearLowerLimit(Vec3(0, 0, lowerLimit));
    genericSpringConstraint->SetLinearUpperLimit(Vec3(0, 0, upperLimit));
    genericSpringConstraint->SetLinearStiffness(Vec3(0, 0, stiffness));
    genericSpringConstraint->SetLinearDamping(Vec3(0, 0, damping));

    if (IsActiveInHierarchy()) {
        genericSpringConstraint->AddToWorld(GetGameWorld()->GetPhysicsWorld());
    }
}

void ComSpringJoint::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    const ComTransform *transform = GetEntity()->GetTransform();

    if (transform->GetOrigin().DistanceSqr(sceneView.origin) < 20000.0f * 20000.0f) {
        Vec3 worldOrigin = transform->GetTransform() * anchor;

        renderWorld->SetDebugColor(Color4::red, Color4::zero);
        renderWorld->DebugLine(worldOrigin - Mat3::identity[0] * CentiToUnit(5), worldOrigin + Mat3::identity[0] * CentiToUnit(5), 1);
        renderWorld->DebugLine(worldOrigin - Mat3::identity[1] * CentiToUnit(5), worldOrigin + Mat3::identity[1] * CentiToUnit(5), 1);
        renderWorld->DebugLine(worldOrigin - Mat3::identity[2] * CentiToUnit(5), worldOrigin + Mat3::identity[2] * CentiToUnit(5), 1);
    }
}

const Vec3 &ComSpringJoint::GetAnchor() const {
    return anchor;
}

void ComSpringJoint::SetAnchor(const Vec3 &anchor) {
    this->anchor = anchor;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameA(anchor, axis);
    }
}

Angles ComSpringJoint::GetAngles() const {
    return axis.ToAngles();
}

void ComSpringJoint::SetAngles(const Angles &angles) {
    this->axis = angles.ToMat3();
    this->axis.FixDegeneracies();

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameA(anchor, axis);
    }
}

float ComSpringJoint::GetLowerLimit() const {
    return lowerLimit;
}

void ComSpringJoint::SetLowerLimit(float limit) {
    this->lowerLimit = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetLinearLowerLimit(Vec3(0, 0, lowerLimit));
    }
}

float ComSpringJoint::GetUpperLimit() const {
    return upperLimit;
}

void ComSpringJoint::SetUpperLimit(float limit) {
    this->upperLimit = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetLinearUpperLimit(Vec3(0, 0, upperLimit));
    }
}

float ComSpringJoint::GetStiffness() const {
    return stiffness;
}

void ComSpringJoint::SetStiffness(float stiffness) {
    this->stiffness = stiffness;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetLinearStiffness(Vec3(0, 0, stiffness));
    }
}

float ComSpringJoint::GetDamping() const {
    return damping;
}

void ComSpringJoint::SetDamping(float damping) {
    this->damping = damping;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetLinearDamping(Vec3(0, 0, damping));
    }
}

BE_NAMESPACE_END
