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
#include "Asset/GuidMapper.h"
#include "Asset/AssetImporter.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Skeleton", SkeletonAsset, Asset)
BEGIN_EVENTS(SkeletonAsset)
END_EVENTS

void SkeletonAsset::RegisterProperties() {
}

SkeletonAsset::SkeletonAsset() {
    skeleton = nullptr;
}

SkeletonAsset::~SkeletonAsset() {
    if (!skeleton) {
        const Str skeletonPath = resourceGuidMapper.Get(GetGuid());
        skeleton = skeletonManager.FindSkeleton(skeletonPath);
    }

    if (skeleton) {
        skeletonManager.ReleaseSkeleton(skeleton, true);
    }
}

Skeleton *SkeletonAsset::GetSkeleton() {
    if (skeleton) {
        return skeleton;
    }
    const Str skeletonPath = resourceGuidMapper.Get(GetGuid());
    skeleton = skeletonManager.GetSkeleton(skeletonPath);
    return skeleton;
}

void SkeletonAsset::Rename(const Str &newName) {
    Skeleton *existingSkeleton = skeletonManager.FindSkeleton(GetResourceFilename());
    if (existingSkeleton) {
        skeletonManager.RenameSkeleton(existingSkeleton, newName);
    }

    Asset::Rename(newName);
}

void SkeletonAsset::Reload() {
    Skeleton *existingSkeleton = skeletonManager.FindSkeleton(GetResourceFilename());
    if (existingSkeleton) {
        existingSkeleton->Reload();
        EmitSignal(&SIG_Reloaded);
    }
}

BE_NAMESPACE_END
