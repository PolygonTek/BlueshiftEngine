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
