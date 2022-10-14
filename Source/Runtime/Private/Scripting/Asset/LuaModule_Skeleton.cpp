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
#include "Render/Skeleton.h"
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterSkeleton(LuaCpp::Module &module) {
    LuaCpp::Selector _Skeleton = module["Skeleton"];

    _Skeleton.SetClass<Skeleton>();
    _Skeleton.AddClassMembers<Skeleton>(
        "name", &Skeleton::GetName,
        "num_joints", &Skeleton::NumJoints);

    _Skeleton["new"].SetFunc([]() {
        Guid newGuid = Guid::CreateGuid();
        Str skeletonName = Str("Skeleton-") + newGuid.ToString();
        resourceGuidMapper.Set(newGuid, skeletonName);
        return skeletonManager.AllocSkeleton(skeletonName);
    });
    _Skeleton["release"].SetFunc([](Skeleton *skeleton) {
        skeletonManager.ReleaseSkeleton(skeleton);
    });
}

BE_NAMESPACE_END
