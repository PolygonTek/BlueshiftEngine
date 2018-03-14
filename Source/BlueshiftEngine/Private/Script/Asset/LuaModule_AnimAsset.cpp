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
#include "Render/Anim.h"
#include "Asset/Asset.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterAnimAsset(LuaCpp::Module &module) {
    LuaCpp::Selector _Anim = module["Anim"];

    _Anim.SetClass<Anim>();

    LuaCpp::Selector _AnimAsset = module["AnimAsset"];

    _AnimAsset.SetClass<AnimAsset>(module["Asset"]);
    _AnimAsset.AddClassMembers<AnimAsset>(
        "anim", &AnimAsset::GetAnim);
}

BE_NAMESPACE_END
