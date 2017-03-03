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
BEGIN_PROPERTIES(SkeletonAsset)
END_PROPERTIES

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

void SkeletonAsset::Reload() {
    Skeleton *existingSkeleton = skeletonManager.FindSkeleton(GetResourceFilename());
    if (existingSkeleton) {
        existingSkeleton->Reload();
        EmitSignal(&SIG_Reloaded);
    }
}

BE_NAMESPACE_END