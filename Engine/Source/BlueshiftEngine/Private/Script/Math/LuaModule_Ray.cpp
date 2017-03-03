#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterRay(LuaCpp::Module &module) {
    LuaCpp::Selector _Ray = module["Ray"];

    _Ray.SetClass<Ray>();
    _Ray.AddClassCtor<Ray, const Vec3 &, const Vec3 &>();
    _Ray.AddClassMembers<Ray>(
        "set", &Ray::Set,
        "origin", &Ray::GetOrigin,
        "direction", &Ray::GetDirection,
        "distance_point", &Ray::GetDistancePoint);
}

BE_NAMESPACE_END