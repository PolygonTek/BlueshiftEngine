#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComSocketJoint.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterSocketJointComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComSocketJoint = module["ComSocketJoint"];

    _ComSocketJoint.SetClass<ComSocketJoint>(module["ComJoint"]);
    _ComSocketJoint.AddClassMembers<ComSocketJoint>(
        "anchor", &ComSocketJoint::GetAnchor,
        "set_anchor", &ComSocketJoint::SetAnchor);

    _ComSocketJoint["meta_object"] = ComSocketJoint::metaObject;
}

BE_NAMESPACE_END