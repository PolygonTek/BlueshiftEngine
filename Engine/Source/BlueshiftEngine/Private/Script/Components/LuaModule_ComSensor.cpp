#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComSensor.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterSensorComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComSensor = module["ComSensor"];

    _ComSensor.SetClass<ComSensor>(module["Component"]);

    _ComSensor["meta_object"] = ComSensor::metaObject;
}

BE_NAMESPACE_END