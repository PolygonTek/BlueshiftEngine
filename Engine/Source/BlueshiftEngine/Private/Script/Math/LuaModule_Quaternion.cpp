#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterQuaternion(LuaCpp::Module &module) {
    LuaCpp::Selector _Quat = module["Quat"];

    _Quat.SetClass<Quat>();
    _Quat.AddClassCtor<Quat, float, float, float, float>();
    _Quat.AddClassMembers<Quat>(
        "x", &Quat::x,
        "y", &Quat::y,
        "z", &Quat::z,
        "w", &Quat::w,
        "element", static_cast<float &(Quat::*)(int)>(&Quat::operator[]), // index start from zero
        "set", &Quat::Set,
        "set_identity", &Quat::SetIdentity,
        "to_string", static_cast<const char*(Quat::*)()const>(&Quat::ToString),
        "calc_w", &Quat::CalcW,
        "transform", static_cast<Vec3(Quat::*)(const Vec3&)const>(&Quat::operator*),
        "length", &Quat::Length,
        "normalize", &Quat::Normalize,
        "inverse", &Quat::Inverse,
        "set_from_two_vectors", &Quat::SetFromTwoVectors,
        "set_from_slerp", &Quat::SetFromSlerp,
        "set_from_slerp_fast", &Quat::SetFromSlerpFast
    );
    _Quat.AddClassMembers<Quat>(
        "__tostring", static_cast<const char*(Quat::*)(void)const>(&Quat::ToString),
        "__unm", static_cast<Quat(Quat::*)(void)const>(&Quat::operator-),
        "__add", static_cast<Quat(Quat::*)(const Quat&)const>(&Quat::operator+),
        "__sub", static_cast<Quat(Quat::*)(const Quat&)const>(&Quat::operator-),
        "__mul", static_cast<Quat(Quat::*)(const Quat&)const>(&Quat::operator*)
    );
}

BE_NAMESPACE_END