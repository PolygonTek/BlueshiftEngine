#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterCylinder(LuaCpp::Module &module) {
    LuaCpp::Selector _Cylinder = module["Cylinder"];

    _Cylinder.SetClass<Cylinder>();
    _Cylinder.AddClassMembers<Cylinder>(
        "__eq", static_cast<bool(Cylinder::*)(const Cylinder&)const>(&Cylinder::operator==),
        "set_p1", &Cylinder::SetP1,
        "set_p2", &Cylinder::SetP2,
        "set_radius", &Cylinder::SetRadius,
        "p1", &Cylinder::P1,
        "p2", &Cylinder::P2,
        "radius", &Cylinder::Radius,
        "equals", static_cast<bool(Cylinder::*)(const Cylinder&, const float)const>(&Cylinder::Equals),
        "line_intersection", &Cylinder::LineIntersection,
        "ray_intersection", &Cylinder::RayIntersection,
        "to_sphere", &Cylinder::ToSphere,
        "to_obb", &Cylinder::ToOBB);
}

BE_NAMESPACE_END