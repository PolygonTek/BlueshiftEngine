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
#include "SkinningJointCache.h"
#include "Skeleton.h"

SkinningJointCache::SkinningJointCache(int numJoints) {
    assert(numJoints > 0);

    this->numJoints = numJoints;
    this->skinningJoints = (BE1::Mat3x4 *)Mem_Alloc16(sizeof(BE1::Mat3x4) * numJoints);
}

SkinningJointCache::~SkinningJointCache() {
    Purge();
}

void SkinningJointCache::Purge() {
    if (skinningJoints) {
        Mem_AlignedFree(skinningJoints);
        skinningJoints = nullptr;
    }

    jointBuffer = nullptr;
    numJoints = 0;
}

bool SkinningJointCache::Update(RHI::FrameThreadData *frameThreadData, const Skeleton *skeleton, const BE1::Mat3x4 *jointMats) {
    if (!frameThreadData || !skeleton || !jointMats || skeleton->NumJoints() != numJoints) {
        jointBuffer = nullptr;
        return false;
    }

    BE1::simdProcessor->MultiplyJoints(skinningJoints, jointMats, skeleton->GetInvBindPoseMatrices(), numJoints);

    jointBuffer = frameThreadData->AllocStructuredBuffer(sizeof(BE1::Mat3x4), numJoints, skinningJoints);
    return jointBuffer != nullptr;
}
