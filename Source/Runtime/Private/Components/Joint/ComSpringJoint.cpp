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
    REGISTER_ACCESSOR_PROPERTY("anchor", "Anchor", Vec3, GetAnchor, SetAnchor, Vec3::zero, 
        "Joint position in local space", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_MIXED_ACCESSOR_PROPERTY("angles", "Angles", Angles, GetAngles, SetAngles, Angles::zero,
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
    axis = Mat3::identity;
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

    // Apply limit distances.
    genericSpringConstraint->SetLinearLowerLimits(Vec3(0, 0, minDist));
    genericSpringConstraint->SetLinearUpperLimits(Vec3(0, 0, maxDist));
    genericSpringConstraint->EnableLinearLimits(true, true, enableLimitDistances);

    // Apply spring stiffness & damping.
    genericSpringConstraint->SetLinearStiffness(Vec3(0, 0, stiffness));
    genericSpringConstraint->SetLinearDamping(Vec3(0, 0, damping));

    constraint = genericSpringConstraint;
}

const Vec3 &ComSpringJoint::GetAnchor() const {
    return anchor;
}

void ComSpringJoint::SetAnchor(const Vec3 &anchor) {
    this->anchor = anchor;

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameB(anchor, axis);
    }
}

Angles ComSpringJoint::GetAngles() const {
    return axis.ToAngles();
}

void ComSpringJoint::SetAngles(const Angles &angles) {
    this->axis = angles.ToMat3();
    this->axis.FixDegeneracies();

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameB(anchor, axis);
    }
}

const Vec3 &ComSpringJoint::GetConnectedAnchor() const {
    return connectedAnchor;
}

void ComSpringJoint::SetConnectedAnchor(const Vec3 &anchor) {
    this->connectedAnchor = anchor;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameA(anchor, connectedAxis);
    }
}

Angles ComSpringJoint::GetConnectedAngles() const {
    return connectedAxis.ToAngles();
}

void ComSpringJoint::SetConnectedAngles(const Angles &angles) {
    this->connectedAxis = angles.ToMat3();
    this->connectedAxis.FixDegeneracies();

    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameA(connectedAnchor, connectedAxis);
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
        ((PhysGenericSpringConstraint *)constraint)->SetLinearLowerLimits(Vec3(0, 0, minDist));
    }
}

void ComSpringJoint::SetMaximumDistance(float maxDist) {
    this->maxDist = maxDist;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetLinearUpperLimits(Vec3(0, 0, maxDist));
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
    if (selectedByParent) {
        const ComTransform *transform = GetEntity()->GetTransform();

        if (transform->GetOrigin().DistanceSqr(camera->GetState().origin) < MeterToUnit(100.0f * 100.0f)) {
            Vec3 worldAnchor = transform->GetMatrix().TransformPos(anchor);
            Mat3 worldAxis = transform->GetAxis() * axis;

            float viewScale = camera->CalcClampedViewScale(worldAnchor);

            RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

            renderWorld->SetDebugColor(Color4::red, Color4::zero);
            renderWorld->DebugLine(worldAnchor - worldAxis[0] * MeterToUnit(5) * viewScale, worldAnchor + worldAxis[0] * MeterToUnit(5) * viewScale);
            renderWorld->DebugLine(worldAnchor - worldAxis[1] * MeterToUnit(5) * viewScale, worldAnchor + worldAxis[1] * MeterToUnit(5) * viewScale);

            renderWorld->DebugCircle(worldAnchor - worldAxis[2] * MeterToUnit(3) * viewScale, worldAxis[2], MeterToUnit(5) * viewScale);
            renderWorld->DebugCircle(worldAnchor, worldAxis[2], MeterToUnit(5) * viewScale);
            renderWorld->DebugCircle(worldAnchor + worldAxis[2] * MeterToUnit(3) * viewScale, worldAxis[2], MeterToUnit(5) * viewScale);
        }
    }
}
#endif

BE_NAMESPACE_END
