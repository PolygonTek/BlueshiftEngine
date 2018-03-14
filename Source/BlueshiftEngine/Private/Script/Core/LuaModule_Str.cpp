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
#include "Script/LuaVM.h"
#include "Core/Str.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterStr(LuaCpp::Module &module) {
    LuaCpp::Selector _Str = module["Str"];

    _Str.SetClass<Str>();
    _Str.AddClassCtor<Str>();
    _Str.AddClassMembers<Str>(
        "c_str", &Str::c_str,
        "is_empty", &Str::IsEmpty,
        "length", static_cast<int(Str::*)() const>(&Str::Length),
        "clear", &Str::Clear,
        "to_lower", static_cast<void(Str::*)()>(&Str::ToLower),
        "to_upper", static_cast<void(Str::*)()>(&Str::ToUpper),
        "cmp", static_cast<int(Str::*)(const char *) const>(&Str::Cmp),
        "cmpn", static_cast<int(Str::*)(const char *, int) const>(&Str::Cmpn),
        "icmp", static_cast<int(Str::*)(const char *) const>(&Str::Icmp), 
        "icmpn", static_cast<int(Str::*)(const char *, int) const>(&Str::Icmpn));

    _Str.AddClassMembers<Str>(
        "__tostring", static_cast<const char *(Str::*)(void) const>(&Str::c_str));
}

BE_NAMESPACE_END
