#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Render/Skeleton.h"
#include "Asset/Asset.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterSkeletonAsset(LuaCpp::Module &module) {
    LuaCpp::Selector _Skeleton = module["Skeleton"];

    _Skeleton.SetClass<Skeleton>();

    LuaCpp::Selector _SkeletonAsset = module["SkeletonAsset"];

    _SkeletonAsset.SetClass<SkeletonAsset>(module["Asset"]);
    _SkeletonAsset.AddClassMembers<SkeletonAsset>(
        "skeleton", &SkeletonAsset::GetSkeleton);
}

BE_NAMESPACE_END