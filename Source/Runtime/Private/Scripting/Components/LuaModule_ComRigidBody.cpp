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
#include "Scripting/LuaVM.h"
#include "Game/Entity.h"
#include "Components/ComCharacterController.h"
#include "Components/ComRigidBody.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterRigidBodyComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComRigidBody = module["ComRigidBody"];

    _ComRigidBody.SetClass<ComRigidBody>(module["Component"]);
    _ComRigidBody.AddClassMembers<ComRigidBody>(
        "mass", &ComRigidBody::GetMass,
        "set_mass", &ComRigidBody::SetMass,
        "gravity", &ComRigidBody::GetGravity,
        "set_gravity", &ComRigidBody::SetGravity,
        "linear_damping", &ComRigidBody::GetLinearDamping,
        "set_linear_damping", &ComRigidBody::SetLinearDamping,
        "angular_damping", &ComRigidBody::GetAngularDamping,
        "set_angular_damping", &ComRigidBody::SetAngularDamping,
        "linear_velocity", &ComRigidBody::GetLinearVelocity,
        "set_linear_velocity", &ComRigidBody::SetLinearVelocity,
        "angular_velocity", &ComRigidBody::GetAngularVelocity,
        "set_angular_velocity", &ComRigidBody::SetAngularVelocity,
        "linear_factor", &ComRigidBody::GetLinearFactor,
        "set_linear_factor", &ComRigidBody::SetLinearFactor,
        "angular_factor", &ComRigidBody::GetAngularFactor,
        "set_angular_factor", &ComRigidBody::SetAngularFactor,
        "total_force", &ComRigidBody::GetTotalForce,
        "total_torque", &ComRigidBody::GetTotalTorque, 
        "is_kinematic", &ComRigidBody::IsKinematic,
        "set_kinematic", &ComRigidBody::SetKinematic,
        "is_ccd", &ComRigidBody::IsCCD,
        "set_ccd", &ComRigidBody::SetCCD,
        "clear_forces", &ComRigidBody::ClearForces,
        "clear_velocities", &ComRigidBody::ClearVelocities,
        "apply_central_force", &ComRigidBody::ApplyCentralForce,
        "apply_force", &ComRigidBody::ApplyForce,
        "apply_torque", &ComRigidBody::ApplyTorque,
        "apply_central_impulse", &ComRigidBody::ApplyCentralImpulse,
        "apply_impulse", &ComRigidBody::ApplyImpulse,
        "apply_angular_impulse", &ComRigidBody::ApplyAngularImpulse);

    _ComRigidBody["meta_object"] = ComRigidBody::metaObject;

    LuaCpp::Selector _Collision = module["Collision"];

    _Collision.SetClass<Collision>();
    _Collision.AddClassMembers<Collision>(
        "entity", &Collision::entity,
        "rigid_body", &Collision::body,
        "character_controller", &Collision::characterController,
        "point", &Collision::point,
        "normal", &Collision::normal,
        "distance", &Collision::distance,
        "impulse", &Collision::impulse);
}

BE_NAMESPACE_END
