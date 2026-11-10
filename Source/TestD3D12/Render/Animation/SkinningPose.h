// Copyright(c) 2017 POLYGONTEK
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "Containers/Array.h"
#include "Math/Matrix3x4.h"
#include "SkinningJointCache.h"

class Skeleton;

/*
---------------------------------------------------------------------------------------------------

    Skinning Pose

---------------------------------------------------------------------------------------------------
*/

class SkinningPose {
public:
    explicit SkinningPose(SharedPtr<Skeleton> skeleton);
    SkinningPose(const SkinningPose &) = delete;
    SkinningPose &operator=(const SkinningPose &) = delete;

    const Skeleton *                GetSkeleton() const { return skeleton.get(); }

    int                             NumJointMats() const { return jointMats.Count(); }
    const BE1::Mat3x4 *             GetJointMats() const { return jointMats.Ptr(); }
    uint64_t                        GetJointMatsSerial() const { return jointMatsSerial; }
    bool                            SetJointMats(const BE1::Mat3x4 *matrices, int numMatrices);

    bool                            SyncWithSkeleton();

    const SkinningJointCache *      GetSkinningJointCache() const;
    bool                            PrepareSkinningJointCache(uint64_t currentFrameSerial);
    bool                            UpdateSkinningJointCache();

private:
    SharedPtr<Skeleton>             skeleton;                                   ///< Skeleton reference
    BE1::Array<BE1::Mat3x4>         jointMats = BE1::Array<BE1::Mat3x4>(64);    ///< 최종 world-space joint matrices
    uint64_t                        jointMatsSerial = 0;                        ///< jointMats 변경 시 증가하는 시리얼. 0 은 유효한 행렬이 없음을 의미한다.
    uint32_t                        skeletonDataSerial = UINT32_MAX;            ///< skeleton data 가 변경될 때마다 증가하는 시리얼
    UniquePtr<SkinningJointCache>   skinningJointCache;
};

BE_INLINE SkinningPose::SkinningPose(SharedPtr<Skeleton> skeleton) : skeleton(std::move(skeleton)) {
    SyncWithSkeleton();
}
