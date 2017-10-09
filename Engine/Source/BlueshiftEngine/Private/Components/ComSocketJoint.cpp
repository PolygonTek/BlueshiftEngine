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
#include "Game/Entity.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Socket Joint", ComSocketJoint, ComJoint)
BEGIN_EVENTS(ComSocketJoint)
END_EVENTS
BEGIN_PROPERTIES(ComSocketJoint)
    PROPERTY_VEC3("anchor", "Anchor", "", "0 0 0", PropertySpec::ReadWrite),
END_PROPERTIES

void ComSocketJoint::RegisterProperties() {
#ifdef NEW_PROPERTY_SYSTEM
    REGISTER_ACCESSOR_PROPERTY("Anchor", Vec3, GetAnchor, SetAnchor, Vec3::zero, "", PropertySpec::ReadWrite);
#endif
}

ComSocketJoint::ComSocketJoint() {
#ifndef NEW_PROPERTY_SYSTEM
    Connect(&Properties::SIG_PropertyChanged, this, (SignalCallback)&ComSocketJoint::PropertyChanged);
#endif
}

ComSocketJoint::~ComSocketJoint() {
}

void ComSocketJoint::Init() {
    ComJoint::Init();

    anchor = props->Get("anchor").As<Vec3>();
}

void ComSocketJoint::Start() {
    ComJoint::Start();

    const ComTransform *transform = GetEntity()->GetTransform();
    const ComRigidBody *rigidBody = GetEntity()->GetComponent<ComRigidBody>();
    assert(rigidBody);

    PhysConstraintDesc desc;
    desc.type           = PhysConstraint::Point2Point;
    desc.bodyA          = rigidBody->GetBody();
    desc.anchorInA      = transform->GetScale() * anchor;

    if (connectedBody) {
        Vec3 worldAnchor = desc.bodyA->GetOrigin() + desc.bodyA->GetAxis() * desc.anchorInA;

        desc.bodyB      = connectedBody->GetBody();  
        desc.anchorInB  = connectedBody->GetBody()->GetAxis().TransposedMulVec(worldAnchor - connectedBody->GetBody()->GetOrigin());
    } else {
        desc.bodyB      = nullptr;
    }

    desc.collision      = collisionEnabled;
    desc.breakImpulse   = breakImpulse;
    constraint = physicsSystem.CreateConstraint(&desc);

    PhysP2PConstraint *p2pConstraint = static_cast<PhysP2PConstraint *>(constraint);

    if (IsEnabled()) {
        p2pConstraint->AddToWorld(GetGameWorld()->GetPhysicsWorld());
    }
}

void ComSocketJoint::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    const ComTransform *transform = GetEntity()->GetTransform();
    Vec3 worldOrigin = transform->GetWorldMatrix() * anchor;
    
    renderWorld->SetDebugColor(Color4::red, Color4::zero);
    renderWorld->DebugLine(worldOrigin - Mat3::identity[0] * CentiToUnit(5), worldOrigin + Mat3::identity[0] * CentiToUnit(5), 1);
    renderWorld->DebugLine(worldOrigin - Mat3::identity[1] * CentiToUnit(5), worldOrigin + Mat3::identity[1] * CentiToUnit(5), 1);
    renderWorld->DebugLine(worldOrigin - Mat3::identity[2] * CentiToUnit(5), worldOrigin + Mat3::identity[2] * CentiToUnit(5), 1);
}

void ComSocketJoint::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
        return;
    }

    if (!Str::Cmp(propName, "anchor")) {
        SetAnchor(props->Get("anchor").As<Vec3>());
        return;
    }

    ComJoint::PropertyChanged(classname, propName);
}

const Vec3 &ComSocketJoint::GetAnchor() const {
    return anchor;
}

void ComSocketJoint::SetAnchor(const Vec3 &anchor) {
    this->anchor = anchor;
    if (constraint) {
        ((PhysP2PConstraint *)constraint)->SetAnchorA(anchor);
    }
}

BE_NAMESPACE_END
