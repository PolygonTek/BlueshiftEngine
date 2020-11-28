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
#include "Render/Skeleton.h"
#include "Asset/Asset.h"
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Skeleton", SkeletonResource, Resource)
BEGIN_EVENTS(SkeletonResource)
END_EVENTS

void SkeletonResource::RegisterProperties() {
}

SkeletonResource::SkeletonResource() {
    skeleton = nullptr;
}

SkeletonResource::~SkeletonResource() {
    if (skeleton) {
        skeletonManager.ReleaseSkeleton(skeleton);
    }
}

Skeleton *SkeletonResource::GetSkeleton() {
    if (skeleton) {
        return skeleton;
    }
    const Str skeletonPath = resourceGuidMapper.Get(asset->GetGuid());
    skeleton = skeletonManager.GetSkeleton(skeletonPath);
    return skeleton;
}

void SkeletonResource::Rename(const Str &newName) {
    const Str skeletonPath = resourceGuidMapper.Get(asset->GetGuid());
    Skeleton *existingSkeleton = skeletonManager.FindSkeleton(skeletonPath);
    if (existingSkeleton) {
        skeletonManager.RenameSkeleton(existingSkeleton, newName);
    }
}

bool SkeletonResource::Reload() {
    const Str skeletonPath = resourceGuidMapper.Get(asset->GetGuid());
    Skeleton *existingSkeleton = skeletonManager.FindSkeleton(skeletonPath);
    if (existingSkeleton) {
        existingSkeleton->Reload();
        return true;
    }
    return false;
}

bool SkeletonResource::Save() {
    return false;
}

BE_NAMESPACE_END
