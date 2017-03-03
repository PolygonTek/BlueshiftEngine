#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComHingeJoint.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterHingeJointComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComHingeJoint = module["ComHingeJoint"];

    _ComHingeJoint.SetClass<ComHingeJoint>(module["ComJoint"]);
    _ComHingeJoint.AddClassMembers<ComHingeJoint>(
        "anchor", &ComHingeJoint::GetAnchor,
        "set_anchor", &ComHingeJoint::SetAnchor,
        "angles", &ComHingeJoint::GetAngles,
        "set_angles", &ComHingeJoint::SetAngles,
        "motor_speed", &ComHingeJoint::GetMotorSpeed,
        "set_motor_speed", &ComHingeJoint::SetMotorSpeed,
        "max_motor_impulse", &ComHingeJoint::GetMaxMotorImpulse,
        "set_max_motor_impulse", &ComHingeJoint::SetMaxMotorImpulse);

    _ComHingeJoint["meta_object"] = ComHingeJoint::metaObject;
}

BE_NAMESPACE_END