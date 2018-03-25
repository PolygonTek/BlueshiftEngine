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
#include "Components/ComSocketJoint.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Socket Joint", ComSocketJoint, ComJoint)
BEGIN_EVENTS(ComSocketJoint)
END_EVENTS

void ComSocketJoint::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("anchor", "Anchor", Vec3, GetLocalAnchor, SetLocalAnchor, Vec3::zero, "Joint position in local space", PropertyInfo::EditorFlag);
}

ComSocketJoint::ComSocketJoint() {
}

ComSocketJoint::~ComSocketJoint() {
}

void ComSocketJoint::Init() {
    ComJoint::Init();

    // Mark as initialized
    SetInitialized(true);
}

void ComSocketJoint::Start() {
    ComJoint::Start();

    const ComTransform *transform = GetEntity()->GetTransform();
    const ComRigidBody *rigidBody = GetEntity()->GetComponent<ComRigidBody>();
    assert(rigidBody);

    // Fill up a constraint description 
    PhysConstraintDesc desc;
    desc.type = PhysConstraint::Point2Point;
    desc.collision = collisionEnabled;
    desc.breakImpulse = breakImpulse;
 
    desc.bodyA = rigidBody->GetBody();
    desc.anchorInA = transform->GetScale() * localAnchor;

    if (connectedBody) {
        Vec3 worldAnchor = desc.bodyA->GetOrigin() + desc.bodyA->GetAxis() * desc.anchorInA;

        desc.bodyB = connectedBody->GetBody();
        desc.anchorInB = connectedBody->GetBody()->GetAxis().TransposedMulVec(worldAnchor - connectedBody->GetBody()->GetOrigin());

        connectedAnchor = desc.anchorInB;
    } else {
        desc.bodyB = nullptr;

        connectedAnchor = Vec3::origin;
    }

    // Create a constraint with the given description
    PhysP2PConstraint *p2pConstraint = (PhysP2PConstraint *)physicsSystem.CreateConstraint(&desc);

    constraint = p2pConstraint;

    if (IsActiveInHierarchy()) {
        p2pConstraint->AddToWorld(GetGameWorld()->GetPhysicsWorld());
    }
}

const Vec3 &ComSocketJoint::GetLocalAnchor() const {
    return localAnchor;
}

void ComSocketJoint::SetLocalAnchor(const Vec3 &anchor) {
    this->localAnchor = anchor;
    if (constraint) {
        ((PhysP2PConstraint *)constraint)->SetAnchorA(anchor);
    }
}

const Vec3 &ComSocketJoint::GetConnectedAnchor() const {
    return connectedAnchor;
}

void ComSocketJoint::SetConnectedAnchor(const Vec3 &anchor) {
    this->connectedAnchor = anchor;
    if (constraint) {
        ((PhysP2PConstraint *)constraint)->SetAnchorB(anchor);
    }
}

void ComSocketJoint::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    const ComTransform *transform = GetEntity()->GetTransform();
    Vec3 worldOrigin = transform->GetTransform() * localAnchor;
    
    renderWorld->SetDebugColor(Color4::red, Color4::zero);
    renderWorld->DebugLine(worldOrigin - Mat3::identity[0] * CentiToUnit(1), worldOrigin + Mat3::identity[0] * CentiToUnit(1), 1);
    renderWorld->DebugLine(worldOrigin - Mat3::identity[1] * CentiToUnit(1), worldOrigin + Mat3::identity[1] * CentiToUnit(1), 1);
    renderWorld->DebugLine(worldOrigin - Mat3::identity[2] * CentiToUnit(1), worldOrigin + Mat3::identity[2] * CentiToUnit(1), 1);
}

BE_NAMESPACE_END
