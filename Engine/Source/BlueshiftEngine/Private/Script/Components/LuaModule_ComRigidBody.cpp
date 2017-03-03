#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Game/Entity.h"
#include "Components/ComCharacterController.h"
#include "Components/ComRigidBody.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterRigidBodyComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComRigidBody = module["ComRigidBody"];

    _ComRigidBody.SetClass<ComRigidBody>(module["Component"]);
    _ComRigidBody.AddClassMembers<ComRigidBody>(
        "origin", &ComRigidBody::GetOrigin,
        "set_origin", &ComRigidBody::SetOrigin,
        "axis", &ComRigidBody::GetAxis,
        "set_axis", &ComRigidBody::SetAxis,
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
        "controller", &Collision::controller,
        "point", &Collision::point,
        "normal", &Collision::normal,
        "distance", &Collision::distance,
        "impulse", &Collision::impulse);
}

BE_NAMESPACE_END
