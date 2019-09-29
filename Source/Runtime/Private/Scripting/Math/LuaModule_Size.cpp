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

void LuaVM::RegisterSize(LuaCpp::Module &module) {
    LuaCpp::Selector _Size = module["Size"];

    _Size.SetClass<Size>();
    _Size.AddClassCtor<Size, int, int>();
    _Size.AddClassMembers<Size>(
        "w", &Size::w,
        "h", &Size::h,
        "__tostring", static_cast<const char*(Size::*)(void)const>(&Size::ToString),
        "element", static_cast<int &(Size::*)(int)>(&Size::operator[]), // index start from zero
        "assign", static_cast<Size&(Size::*)(const Size&)>(&Size::operator=),
        "set", &Size::Set,
        "is_empty", &Size::IsEmpty,
        "to_string", static_cast<const char*(Size::*)()const>(&Size::ToString));
}

BE_NAMESPACE_END
