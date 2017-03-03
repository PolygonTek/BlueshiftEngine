#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterPoint(LuaCpp::Module &module) {
    LuaCpp::Selector _Point = module["Point"];

    _Point.SetClass<Point>();
    _Point.AddClassCtor<Point, float, float>();
    _Point.AddClassMembers<Point>(
        "x", &Point::x,
        "y", &Point::y,
        "__tostring", static_cast<const char*(Point::*)(void)const>(&Point::ToString),
        "__unm", static_cast<Point(Point::*)(void)const>(&Point::operator-),
        "__add", static_cast<Point(Point::*)(const Point&)const>(&Point::operator+),
        "__sub", static_cast<Point(Point::*)(const Point&)const>(&Point::operator-),
        "element", static_cast<int &(Point::*)(int)>(&Point::operator[]), // index start from zero
        "set", &Point::Set,
        "add_self", static_cast<Point&(Point::*)(const Point&)>(&Point::operator+=),
        "sub_self", static_cast<Point&(Point::*)(const Point&)>(&Point::operator-=),
        "distance", &Point::Distance,
        "distance_squared", &Point::DistanceSqr,
        "to_string", static_cast<const char*(Point::*)()const>(&Point::ToString));
}

BE_NAMESPACE_END