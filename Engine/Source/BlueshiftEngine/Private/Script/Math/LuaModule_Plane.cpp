#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterPlane(LuaCpp::Module &module) {
    LuaCpp::Selector _Plane = module["Plane"];

    _Plane.SetClass<Plane>();
    _Plane.AddClassCtor<Plane, const Vec3 &, float>();
    _Plane.AddClassMembers<Plane>(
        "element", static_cast<float&(Plane::*)(int)>(&Plane::operator[]), // index start from zero
        "set_normal", &Plane::SetNormal,
        "set_dist", &Plane::SetDist,
        "normal", static_cast<Vec3&(Plane::*)(void)>(&Plane::Normal),
        "dist", &Plane::Dist,
        "set_from_points", &Plane::SetFromPoints,
        "set_from_vecs", &Plane::SetFromVecs,
        "fit_through_point", &Plane::FitThroughPoint,
        "to_string", static_cast<const char*(Plane::*)()const>(&Plane::ToString),
        "flip", &Plane::Flip,
        "normalize", &Plane::Normalize,
        "fix_degenerate_normal", &Plane::FixDegenerateNormal,
        "translate", &Plane::Translate,
        "translate_self", &Plane::TranslateSelf,
        "rotate", &Plane::Rotate,
        "rotate_self", &Plane::RotateSelf,
        "distance", &Plane::Distance,
        "side", &Plane::GetSide,
        "line_intersection", &Plane::LineIntersection,
        "ray_intersection", &Plane::RayIntersection
    );
    _Plane.AddClassMembers<Plane>(
        "__tostring", static_cast<const char*(Plane::*)(void)const>(&Plane::ToString)
    );
}

BE_NAMESPACE_END