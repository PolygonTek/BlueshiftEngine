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
#include "Render/Mesh.h"
#include "Asset/Asset.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterMeshAsset(LuaCpp::Module &module) {
    LuaCpp::Selector _Mesh = module["Mesh"];

    _Mesh.SetClass<Mesh>();

    LuaCpp::Selector _MeshAsset = module["MeshAsset"];

    _MeshAsset.SetClass<MeshAsset>(module["Asset"]);
    _MeshAsset.AddClassMembers<MeshAsset>(
        "mesh", &MeshAsset::GetMesh);
}

BE_NAMESPACE_END
