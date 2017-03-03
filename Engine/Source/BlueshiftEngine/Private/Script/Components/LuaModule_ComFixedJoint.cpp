#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComFixedJoint.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterFixedJointComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComFixedJoint = module["ComFixedJoint"];

    _ComFixedJoint.SetClass<ComFixedJoint>(module["ComJoint"]);

    _ComFixedJoint["meta_object"] = ComFixedJoint::metaObject;
}

BE_NAMESPACE_END