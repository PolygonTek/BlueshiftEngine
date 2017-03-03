#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComJoint.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterJointComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComJoint = module["ComJoint"];

    _ComJoint.SetClass<ComJoint>(module["Component"]);
    _ComJoint.AddClassMembers<ComJoint>(
        "connected_body", &ComJoint::GetConnectedBody,
        "set_connected_body", &ComJoint::SetConnectedBody,
        "is_collision_enabled", &ComJoint::IsCollisionEnabled,
        "set_collision_enabled", &ComJoint::SetCollisionEnabled,
        "break_impulse", &ComJoint::GetBreakImpulse,
        "set_break_impulse", &ComJoint::SetBreakImpulse);

    _ComJoint["meta_object"] = ComJoint::metaObject;
}

BE_NAMESPACE_END