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

#include "Precompiled.h"
#include "SkinningPose.h"
#include "Skeleton.h"

bool SkinningPose::SetJointMats(const BE1::Mat3x4 *matrices, int numMatrices) {
    assert(BE1::Engine::IsInMainThread());
    assert(matrices);

    if (!SyncWithSkeleton() || numMatrices != jointMats.Count()) {
        return false;
    }

    BE1::simdProcessor->Memcpy(jointMats.Ptr(), matrices, sizeof(BE1::Mat3x4) * numMatrices);
    jointMatsSerial++;
    return true;
}

bool SkinningPose::SyncWithSkeleton() {
    assert(BE1::Engine::IsInMainThread());

    if (!skeleton || !skeleton->IsReady()) {
        jointMatsSerial = 0;

        if (skinningJointCache) {
            skinningJointCache->Invalidate();
        }
        return false;
    }

    // skeleton 이 변경되었다면 캐시를 재설정하고, joint 개수까지 변경되었다면 joint 행렬 배열과 캐시를 새로 만든다.
    const uint32_t newSkeletonDataSerial = skeleton->GetDataSerial();
    if (skeletonDataSerial != newSkeletonDataSerial) {
        jointMatsSerial = 0;

        const int numJoints = skeleton->NumJoints();
        if (jointMats.Count() != numJoints) {
            jointMats.SetCount(numJoints, false);
            skinningJointCache = numJoints > 0 ? MakeUnique<SkinningJointCache>(numJoints) : nullptr;
        } else if (skinningJointCache) {
            skinningJointCache->Invalidate();
        }
        skeletonDataSerial = newSkeletonDataSerial;
    }

    return skinningJointCache != nullptr;
}

const SkinningJointCache *SkinningPose::GetSkinningJointCache() const {
    if (jointMatsSerial == 0 || !skeleton->IsReady() || skeletonDataSerial != skeleton->GetDataSerial()) {
        return nullptr;
    }
    return skinningJointCache.get();
}

bool SkinningPose::PrepareSkinningJointCache(uint64_t currentFrameSerial) {
    assert(BE1::Engine::IsInMainThread());

    if (!GetSkinningJointCache()) {
        return false;
    }
    return skinningJointCache->PrepareUpdate(skeleton.get(), jointMatsSerial, currentFrameSerial);
}

bool SkinningPose::UpdateSkinningJointCache() {
    if (!GetSkinningJointCache()) {
        return false;
    }
    return skinningJointCache->Update(skeleton.get(), jointMats.Ptr());
}
