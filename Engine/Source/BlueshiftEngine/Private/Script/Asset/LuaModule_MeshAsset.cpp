#include "Precompiled.h"
#include "Script/LuaVM.h"
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