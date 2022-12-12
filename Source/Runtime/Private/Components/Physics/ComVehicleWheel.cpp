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
#include "Components/Physics/ComRigidBody.h"
#include "Components/Physics/ComVehicleWheel.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Vehicle Wheel", ComVehicleWheel, Component)
BEGIN_EVENTS(ComVehicleWheel)
END_EVENTS

void ComVehicleWheel::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("center", "Center", Vec3, GetLocalOrigin, SetLocalOrigin, Vec3::origin,
        "Wheel position in local space", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_MIXED_ACCESSOR_PROPERTY("angles", "Angles", Angles, GetLocalAngles, SetLocalAngles, Angles::zero,
        "Wheel angles in local space", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("radius", "Radius", float, GetRadius, SetRadius, MeterToUnit(0.5f),
        "Wheel radius", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("susDistance", "Suspension/Distance", float, GetSuspensionDistance, SetSuspensionDistance, MeterToUnit(0.0f),
        "", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("susMaxDistance", "Suspension/Max Distance", float, GetSuspensionMaxDistance, SetSuspensionMaxDistance, MeterToUnit(0.3f),
        "", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("susMaxForce", "Suspension/Max Force", float, GetSuspensionMaxForce, SetSuspensionMaxForce, MeterToUnit(100.0f),
        "", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("susStiffness", "Suspension/Stiffness", float, GetSuspensionStiffness, SetSuspensionStiffness, 60.f,
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("susDampingRelaxation", "Suspension/Damping Relaxation", float, GetSuspensionDampingRelaxation, SetSuspensionDampingRelaxation, 2.3f,
        "The damping coefficient for when the suspension is expanding", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("susDampingCompression", "Suspension/Damping Compression", float, GetSuspensionDampingCompression, SetSuspensionDampingCompression, 4.4f,
        "The damping coefficient for when the suspension is compressed", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("rollingFriction", "Rolling/Friction", float, GetRollingFriction, SetRollingFriction, 2.0f,
        "The coefficient of friction between the tyre and the ground", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("rollingInfluence", "Rolling/Influence", float, GetRollingInfluence, SetRollingInfluence, 0.1f,
        "Rolling torque applied from the wheels that cause the vehicle to roll over", PropertyInfo::Flag::Editor);
}

ComVehicleWheel::ComVehicleWheel() {
    updatable = true;
}

ComVehicleWheel::~ComVehicleWheel() {
}

bool ComVehicleWheel::IsConflictComponent(const MetaObject &componentClass) const {
    if (componentClass.IsTypeOf(ComRigidBody::metaObject)) {
        return true;
    }

    return false;
}

void ComVehicleWheel::Init() {
    Component::Init();

    // Mark as initialized
    SetInitialized(true);
}

void ComVehicleWheel::Update() {
    if (!vehicle) {
        return;
    }

    vehicle->UpdateWheelTransform(vehicleWheelIndex);

    ALIGN_AS32 const Mat3x4 wheelMatrix = vehicle->GetWheelTransform(vehicleWheelIndex);

    ComTransform *transform = GetEntity()->GetTransform();

    transform->SetPhysicsUpdating(true);
    transform->SetOriginAxis(wheelMatrix.ToTranslationVec3(), wheelMatrix.ToMat3() * localAxis.Transpose());
    transform->SetPhysicsUpdating(false);
}

void ComVehicleWheel::ClearCache() {
    vehicle = nullptr;
    vehicleWheelIndex = -1;
}

void ComVehicleWheel::SetLocalOrigin(const Vec3 &origin) {
    this->localOrigin = origin;
}

void ComVehicleWheel::SetLocalAngles(const Angles &angles) {
    this->localAxis = angles.ToMat3();
    this->localAxis.FixDegeneracies();
}

void ComVehicleWheel::SetRadius(float radius) {
    this->radius = radius;
}

void ComVehicleWheel::SetSuspensionDistance(float susDist) {
    this->suspensionRestLength = susDist;
}

void ComVehicleWheel::SetSuspensionMaxDistance(float susMaxDist) {
    this->suspensionMaxDistance = susMaxDist;
}

void ComVehicleWheel::SetSuspensionMaxForce(float susMaxForce) {
    this->suspensionMaxForce = susMaxForce;
}

void ComVehicleWheel::SetSuspensionStiffness(float susStiffness) {
    this->suspensionStiffness = susStiffness;
}

void ComVehicleWheel::SetSuspensionDampingRelaxation(float susDampingRelaxation) {
    this->suspensionDampingRelaxation = susDampingRelaxation;
}

void ComVehicleWheel::SetSuspensionDampingCompression(float susDampingCompression) {
    this->suspensionDampingCompression = susDampingCompression;
}

void ComVehicleWheel::SetRollingFriction(float rollingFriction) {
    this->rollingFriction = rollingFriction;
}

void ComVehicleWheel::SetRollingInfluence(float rollingInfluence) {
    this->rollingInfluence = rollingInfluence;
}

float ComVehicleWheel::GetSteeringAngle() const {
    if (vehicle) {
        return RAD2DEG(vehicle->GetSteeringAngle(vehicleWheelIndex));
    }
    return 0;
}

void ComVehicleWheel::SetSteeringAngle(float angle) {
    if (vehicle) {
        vehicle->SetSteeringAngle(vehicleWheelIndex, DEG2RAD(angle));
    }
}

float ComVehicleWheel::GetTorque() const {
    if (vehicle) {
        return vehicle->GetTorque(vehicleWheelIndex);
    }
    return 0;
}

void ComVehicleWheel::SetTorque(float torque) {
    if (vehicle) {
        vehicle->SetTorque(vehicleWheelIndex, torque);
    }
}

float ComVehicleWheel::GetBrakingTorque() const {
    if (vehicle) {
        return vehicle->GetBrakingTorque(vehicleWheelIndex);
    }
    return 0;
}

void ComVehicleWheel::SetBrakingTorque(float torque) {
    if (vehicle) {
        vehicle->SetBrakingTorque(vehicleWheelIndex, torque);
    }
}

float ComVehicleWheel::GetSkidInfo() const {
    if (vehicle) {
        return vehicle->GetSkidInfo(vehicleWheelIndex);
    }
    return 1.0f;
}

float ComVehicleWheel::GetSuspensionRelativeVelocity() const {
    if (vehicle) {
        return vehicle->GetSuspensionRelativeVelocity(vehicleWheelIndex);
    }
    return 1.0f;
}

#if WITH_EDITOR
void ComVehicleWheel::DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    const ComTransform *transform = GetEntity()->GetTransform();

    if (transform->GetOrigin().DistanceSqr(camera->GetState().origin) < MeterToUnit(100.0f * 100.0f)) {
        Vec3 worldOrigin = transform->GetMatrix().TransformPos(localOrigin);
        Mat3 worldAxis = transform->GetAxis() * localAxis;

        // Draw wheel circle
        renderWorld->SetDebugColor(Color4::lime, Color4::zero);
        renderWorld->DebugCircle(worldOrigin, worldAxis[1], radius, 1, true, true);

        if (selectedByParent) {
            // Draw forward direction
            renderWorld->SetDebugColor(Color4::red, Color4::zero);
            renderWorld->DebugLine(worldOrigin, worldOrigin + worldAxis[0] * radius);

            // Draw axle axis
            renderWorld->SetDebugColor(Color4::lime, Color4::zero);
            renderWorld->DebugLine(worldOrigin, worldOrigin + worldAxis[1] * radius);

            // Draw suspension direction
            renderWorld->SetDebugColor(Color4::blue, Color4::zero);
            renderWorld->DebugLine(worldOrigin, worldOrigin + worldAxis[2] * radius);
        }
    }
}
#endif

BE_NAMESPACE_END
