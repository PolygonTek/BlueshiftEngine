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

void LuaVM::RegisterSizeF(LuaCpp::Module &module) {
    LuaCpp::Selector _SizeF = module["SizeF"];

    _SizeF.SetClass<SizeF>();
    _SizeF.AddClassCtor<SizeF, float, float>();
    _SizeF.AddClassMembers<SizeF>(
        "w", &SizeF::w,
        "h", &SizeF::h,
        "__tostring", static_cast<const char*(SizeF::*)(void)const>(&SizeF::ToString),
        "element", static_cast<float &(SizeF::*)(int)>(&SizeF::operator[]), // index start from zero
        "assign", static_cast<SizeF&(SizeF::*)(const SizeF&)>(&SizeF::operator=),
        "set", &SizeF::Set,
        "is_empty", &SizeF::IsEmpty,
        "to_string", static_cast<const char*(SizeF::*)()const>(&SizeF::ToString));
}

BE_NAMESPACE_END
