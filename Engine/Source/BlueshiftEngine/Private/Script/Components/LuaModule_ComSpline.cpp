#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComSpline.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterSplineComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComSpline = module["ComSpline"];

    _ComSpline.SetClass<ComSpline>(module["Component"]);
    _ComSpline.AddClassMembers<ComSpline>(
        "length", &ComSpline::Length,
        "current_origin", &ComSpline::GetCurrentOrigin,
        "current_axis", &ComSpline::GetCurrentAxis);

    _ComSpline["meta_object"] = ComSpline::metaObject;
}

BE_NAMESPACE_END