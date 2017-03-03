#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterVec2(LuaCpp::Module &module) {
    LuaCpp::Selector _Vec2 = module["Vec2"];

    _Vec2.SetClass<Vec2>();
    _Vec2.AddClassCtor<Vec2, float, float>();
    _Vec2.AddClassMembers<Vec2>(
        "x", &Vec2::x,
        "y", &Vec2::y,
        "at", &Vec2::At, // index start from zero        
        "add", &Vec2::Add,
        "add_self", &Vec2::AddSelf,
        "sub", &Vec2::Sub,
        "sub_self", &Vec2::SubSelf,
        "mul", &Vec2::Mul,
        "mul_self", &Vec2::MulSelf,
        "mul_comp", &Vec2::MulComp,
        "mul_comp_self", &Vec2::MulCompSelf,
        "div", &Vec2::Div,
        "div_self", &Vec2::DivSelf,
        "div_comp", &Vec2::DivComp,
        "div_comp_self", &Vec2::DivCompSelf,
        "equals", static_cast<bool(Vec2::*)(const Vec2&, const float)const>(&Vec2::Equals),
        "set", &Vec2::Set,
        "set_from_scalar", &Vec2::SetFromScalar,
        "length", &Vec2::Length,
        "length_squared", &Vec2::LengthSqr,
        "distance", &Vec2::Distance,
        "distance_squared", &Vec2::DistanceSqr,
        "normalize", &Vec2::Normalize,
        "normalize_fast", &Vec2::NormalizeFast,
        "scale_to_length", &Vec2::ScaleToLength,
        "clamp", &Vec2::Clamp,
        "dot", &Vec2::Dot,
        "cross", &Vec2::Cross,
        "set_from_lerp", &Vec2::SetFromLerp,
        "to_angle", &Vec2::ToAngle,
        "to_string", static_cast<const char*(Vec2::*)(void)const>(&Vec2::ToString)
    );
    _Vec2.AddClassMembers<Vec2>(
        "__tostring", static_cast<const char *(Vec2::*)(void)const>(&Vec2::ToString),
        "__unm", static_cast<Vec2(Vec2::*)() const>(&Vec2::operator-),
        "__add", &Vec2::Add,
        "__sub", &Vec2::Sub,
        "__eq", static_cast<bool(Vec2::*)(const Vec2&)const>(&Vec2::operator==)
    );

    _Vec2["origin"] = Vec2::origin;
    _Vec2["zero"] = Vec2::zero;
    _Vec2["one"] = Vec2::one;
    _Vec2["unit_x"] = Vec2::unitX;
    _Vec2["unit_y"] = Vec2::unitY;
}

BE_NAMESPACE_END