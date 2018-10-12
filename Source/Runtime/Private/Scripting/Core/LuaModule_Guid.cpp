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
#include "Core/Guid.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterGuid(LuaCpp::Module &module) {
    LuaCpp::Selector _Guid = module["Guid"];

    _Guid.SetClass<Guid>();
    _Guid.AddClassCtor<Guid>();
    _Guid.AddClassMembers<Guid>(
        "is_zero", &Guid::IsZero,
        "set", &Guid::Set,
        "set_from_string", &Guid::SetFromString);

    _Guid["zero"] = Guid::zero;
    
    _Guid["create_guid"].SetFunc(Guid::CreateGuid);
    _Guid["equal"].SetFunc(static_cast<bool(*)(const Guid &lhs, const Guid &rhs)>(&operator==));
    _Guid["notequal"].SetFunc(static_cast<bool(*)(const Guid &lhs, const Guid &rhs)>(&operator!=));
}

BE_NAMESPACE_END
