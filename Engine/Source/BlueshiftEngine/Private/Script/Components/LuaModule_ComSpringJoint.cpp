#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComSpringJoint.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterSpringJointComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComSpringJoint = module["ComSpringJoint"];

    _ComSpringJoint.SetClass<ComSpringJoint>(module["ComJoint"]);
    _ComSpringJoint.AddClassMembers<ComSpringJoint>(
        "anchor", &ComSpringJoint::GetAnchor,
        "set_anchor", &ComSpringJoint::SetAnchor,
        "angles", &ComSpringJoint::GetAngles,
        "set_angles", &ComSpringJoint::SetAngles,
        "lower_limit", &ComSpringJoint::GetLowerLimit,
        "set_lower_limit", &ComSpringJoint::SetLowerLimit,
        "upper_limit", &ComSpringJoint::GetUpperLimit,
        "set_upper_limit", &ComSpringJoint::SetUpperLimit,
        "stiffness", &ComSpringJoint::GetStiffness,
        "set_stiffness", &ComSpringJoint::SetStiffness,
        "damping", &ComSpringJoint::GetDamping,
        "set_damping", &ComSpringJoint::SetDamping);

    _ComSpringJoint["meta_object"] = ComSpringJoint::metaObject;
}

BE_NAMESPACE_END