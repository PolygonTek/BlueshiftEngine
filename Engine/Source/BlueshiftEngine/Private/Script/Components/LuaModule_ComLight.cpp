#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComLight.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterLightComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComLight = module["ComLight"];

    _ComLight.SetClass<ComLight>(module["Component"]);

    _ComLight["meta_object"] = ComLight::metaObject;
}

BE_NAMESPACE_END