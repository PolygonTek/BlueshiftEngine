// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Scripting/LuaVM.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterPoint(LuaCpp::Module &module) {
    LuaCpp::Selector _Point = module["Point"];

    _Point.SetClass<Point>();
    _Point.AddClassCtor<Point, int, int>();
    _Point.AddClassMembers<Point>(
        "x", &Point::x,
        "y", &Point::y,
        "__tostring", static_cast<const char*(Point::*)(void)const>(&Point::ToString),
        "__unm", static_cast<Point(Point::*)(void)const>(&Point::operator-),
        "__add", static_cast<Point(Point::*)(const Point&)const>(&Point::operator+),
        "__sub", static_cast<Point(Point::*)(const Point&)const>(&Point::operator-),
        "element", static_cast<int &(Point::*)(int)>(&Point::operator[]), // index start from zero
        "assign", static_cast<Point&(Point::*)(const Point&)>(&Point::operator=),
        "set", &Point::Set,
        "add_self", static_cast<Point&(Point::*)(const Point&)>(&Point::operator+=),
        "sub_self", static_cast<Point&(Point::*)(const Point&)>(&Point::operator-=),
        "distance", &Point::Distance,
        "distance_squared", &Point::DistanceSqr,
        "to_string", static_cast<const char*(Point::*)()const>(&Point::ToString));

    _Point["zero"] = Point::zero;
}

BE_NAMESPACE_END
