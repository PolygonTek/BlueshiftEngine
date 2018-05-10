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
#include "Game/Prefab.h"
#include "Asset/Asset.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterPrefabAsset(LuaCpp::Module &module) {
    LuaCpp::Selector _Prefab = module["Prefab"];

    _Prefab.SetClass<Prefab>();
    _Prefab.AddClassMembers<Prefab>(
        "name", &Prefab::GetName,
        "root_entity", &Prefab::GetRootEntity);

    LuaCpp::Selector _PrefabAsset = module["PrefabAsset"];

    _PrefabAsset.SetClass<PrefabAsset>(module["Asset"]);
    _PrefabAsset.AddClassMembers<PrefabAsset>(
        "prefab", &PrefabAsset::GetPrefab);
}

BE_NAMESPACE_END
