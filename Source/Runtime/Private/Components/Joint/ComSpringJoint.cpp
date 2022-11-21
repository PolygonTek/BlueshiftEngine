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
#include "Components/Joint/ComSpringJoint.h"
#include "Components/ComRigidBody.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Spring Joint", ComSpringJoint, ComJoint)
BEGIN_EVENTS(ComSpringJoint)
END_EVENTS

void ComSpringJoint::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("anchor", "Anchor", Vec3, GetLocalAnchor, SetLocalAnchor, Vec3::zero, 
        "Joint position in local space", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_MIXED_ACCESSOR_PROPERTY("angles", "Angles", Angles, GetLocalAngles, SetLocalAngles, Angles::zero,
        "Joint angles in local space", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("useLimits", "Limits/Use Limits", bool, GetEnableLimitDistances, SetEnableLimitDistances, false, 
        "Activate joint limits", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("minDist", "Limits/Minimum Distance", float, GetMinimumDistance, SetMinimumDistance, 0.f, 
        "", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("maxDist", "Limits/Maximum Distance", float, GetMaximumDistance, SetMaximumDistance, 0.f, 
        "", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("stiffness", "Spring/Stiffness", float, GetStiffness, SetStiffness, 30.f, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("damping", "Spring/Damping", float, GetDamping, SetDamping, 0.2f, 
        "", PropertyInfo::Flag::Editor).SetRange(0, 1, 0.01f);
}

ComSpringJoint::ComSpringJoint() {
    localAxis = Mat3::identity;
}

ComSpringJoint::~ComSpringJoint() {
}

void ComSpringJoint::Init() {
    ComJoint::Init();

    // Mark as initialized.
    SetInitialized(true);
}

void ComSpringJoint::CreateConstraint() {
    const ComTransform *transform = GetEntity()->GetTransform();
    const ComRigidBody *rigidBody = GetEntity()->GetComponent<ComRigidBody>();
    assert(rigidBody);

    // Fill up a constraint description.
    PhysConstraintDesc desc;
    desc.type = PhysConstraint::Type::GenericSpring;
    desc.collision = collisionEnabled;
    desc.breakImpulse = breakImpulse;

    desc.bodyA = rigidBody->GetBody();
    desc.axisInA = localAxis;
    desc.anchorInA = transform->GetScale() * localAnchor;

    const ComRigidBody *connectedBody = GetConnectedBody();
    if (connectedBody) {
        Mat3 worldAxis = desc.bodyA->GetAxis() * localAxis;
        Vec3 worldAnchor = desc.bodyA->GetOrigin() + desc.bodyA->GetAxis() * desc.anchorInA;

        desc.bodyB = connectedBody->GetBody();
        desc.axisInB = connectedBody->GetBody()->GetAxis().TransposedMul(worldAxis);
        desc.anchorInB = connectedBody->GetBody()->GetAxis().TransposedMulVec(worldAnchor - connectedBody->GetBody()->GetOrigin());

        connectedAxis = desc.axisInB;
        connectedAnchor = desc.anchorInB;
    } else {
        desc.bodyB = nullptr;

        connectedAxis = Mat3::identity;
        connectedAnchor = Vec3::origin;
    }

    // Create a constraint with the given description.
    PhysGenericSpringConstraint *genericSpringConstraint = (PhysGenericSpringConstraint *)physicsSystem.CreateConstraint(desc);

    // Apply limit distances.
    genericSpringConstraint->SetLinearLowerLimit(Vec3(0, 0, minDist));
    genericSpringConstraint->SetLinearUpperLimit(Vec3(0, 0, maxDist));
    genericSpringConstraint->EnableLinearLimits(true, true, enableLimitDistances);

    // Apply spring stiffness & damping.
    genericSpringConstraint->SetLinearStiffness(Vec3(0, 0, stiffness));
    genericSpringConstraint->SetLinearDamping(Vec3(0, 0, damping));

    constraint = genericSpringConstraint;
}

const Vec3 &ComSpringJoint::GetLocalAnchor() const {
    return localAnchor;
}

void ComSpringJoint::SetLocalAnchor(const Vec3 &anchor) {
    this->localAnchor = anchor;

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameA(anchor, localAxis);
    }
}

Angles ComSpringJoint::GetLocalAngles() const {
    return localAxis.ToAngles();
}

void ComSpringJoint::SetLocalAngles(const Angles &angles) {
    this->localAxis = angles.ToMat3();
    this->localAxis.FixDegeneracies();

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameA(localAnchor, localAxis);
    }
}

const Vec3 &ComSpringJoint::GetConnectedAnchor() const {
    return connectedAnchor;
}

void ComSpringJoint::SetConnectedAnchor(const Vec3 &anchor) {
    this->connectedAnchor = anchor;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameB(anchor, connectedAxis);
    }
}

Angles ComSpringJoint::GetConnectedAngles() const {
    return connectedAxis.ToAngles();
}

void ComSpringJoint::SetConnectedAngles(const Angles &angles) {
    this->connectedAxis = angles.ToMat3();
    this->connectedAxis.FixDegeneracies();

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameB(connectedAnchor, connectedAxis);
    }
}

void ComSpringJoint::SetEnableLimitDistances(bool enable) {
    this->enableLimitDistances = enable;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->EnableLinearLimits(true, true, enableLimitDistances);
    }
}

void ComSpringJoint::SetMinimumDistance(float minDist) {
    this->minDist = minDist;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetLinearLowerLimit(Vec3(0, 0, minDist));
    }
}

void ComSpringJoint::SetMaximumDistance(float maxDist) {
    this->maxDist = maxDist;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetLinearUpperLimit(Vec3(0, 0, maxDist));
    }
}

void ComSpringJoint::SetStiffness(float stiffness) {
    this->stiffness = stiffness;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetLinearStiffness(Vec3(0, 0, stiffness));
    }
}

void ComSpringJoint::SetDamping(float damping) {
    this->damping = damping;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetLinearDamping(Vec3(0, 0, damping));
    }
}

#if WITH_EDITOR
void ComSpringJoint::DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    const ComTransform *transform = GetEntity()->GetTransform();

    if (transform->GetOrigin().DistanceSqr(camera->GetState().origin) < MeterToUnit(100.0f * 100.0f)) {
        Vec3 worldOrigin = transform->GetWorldMatrix().TransformPos(localAnchor);
        Mat3 worldAxis = transform->GetAxis() * localAxis;

        float viewScale = camera->CalcViewScale(worldOrigin);

        renderWorld->SetDebugColor(Color4::red, Color4::zero);
        renderWorld->DebugLine(worldOrigin - worldAxis[0] * MeterToUnit(5) * viewScale, worldOrigin + worldAxis[0] * MeterToUnit(5) * viewScale);
        renderWorld->DebugLine(worldOrigin - worldAxis[1] * MeterToUnit(5) * viewScale, worldOrigin + worldAxis[1] * MeterToUnit(5) * viewScale);

        renderWorld->DebugCircle(worldOrigin - worldAxis[2] * MeterToUnit(3) * viewScale, worldAxis[2], MeterToUnit(5) * viewScale);
        renderWorld->DebugCircle(worldOrigin, worldAxis[2], MeterToUnit(5) * viewScale);
        renderWorld->DebugCircle(worldOrigin + worldAxis[2] * MeterToUnit(3) * viewScale, worldAxis[2], MeterToUnit(5) * viewScale);
    }
}
#endif

BE_NAMESPACE_END
