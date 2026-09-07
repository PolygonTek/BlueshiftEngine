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
    this->skinningJoints = static_cast<BE1::Mat3x4 *>(Mem_Alloc16(sizeof(BE1::Mat3x4) * numJoints * 2));
}

SkinningJointCache::~SkinningJointCache() {
    Mem_AlignedFree(skinningJoints);
}

const BE1::Mat3x4 *SkinningJointCache::GetSkinningJoints() const {
    assert(state == State::Ready);
    return skinningJoints + currentPoseIndex * numJoints;
}

const BE1::Mat3x4 *SkinningJointCache::GetPrevSkinningJoints() const {
    assert(state == State::Ready);
    const uint32_t previousPoseIndex = historyValidFrameSerial != UINT64_MAX ? currentPoseIndex ^ 1u : currentPoseIndex;
    return skinningJoints + previousPoseIndex * numJoints;
}

void SkinningJointCache::Invalidate() {
    jointMatsSerial = UINT64_MAX;
    historyValidFrameSerial = UINT64_MAX;
    skeletonDataSerial = 0;
    state = State::Invalid;
}

bool SkinningJointCache::IsUpToDate(const Skeleton *skeleton, uint64_t jointMatsSerial) const {
    return state == State::Ready && this->jointMatsSerial == jointMatsSerial && skeleton->NumJoints() == numJoints && skeletonDataSerial == skeleton->GetDataSerial();
}

bool SkinningJointCache::PrepareUpdate(const Skeleton *skeleton, uint64_t jointMatsSerial, uint64_t currentFrameSerial) {
    if (skeleton->NumJoints() != numJoints) {
        // MeshInstance 기준으로 생성한 캐시와 skeleton 의 joint 수가 다르면 갱신할 수 없다.
        return false;
    }

    if (state == State::Pending) {
        // 이미 갱신이 예약된 상태이므로 갱신 예약을 하지 않는다.
        return false;
    }

    // 마지막 캐시 갱신 이후 skeleton 데이터가 변경되었는지 확인한다.
    const uint32_t newSkeletonDataSerial = skeleton->GetDataSerial();
    if (state == State::Ready && skeletonDataSerial == newSkeletonDataSerial) {
        if (this->jointMatsSerial == jointMatsSerial) {
            // 이미 갱신된 joint pose 이므로 갱신 예약을 하지 않는다.
            return false;
        }
        if (this->jointMatsSerial + 1 == jointMatsSerial) {
            // 이전 joint pose 가 갱신된 상태이므로 이전 joint pose 를 사용 가능하다.
            historyValidFrameSerial = currentFrameSerial;
        } else {
            historyValidFrameSerial = UINT64_MAX;
        }
    } else {
        historyValidFrameSerial = UINT64_MAX;
    }

    this->currentPoseIndex ^= 1u;
    this->jointMatsSerial = jointMatsSerial;
    this->skeletonDataSerial = newSkeletonDataSerial;
    this->state = State::Pending;
    return true;
}

bool SkinningJointCache::Update(const Skeleton *skeleton, const BE1::Mat3x4 *jointMats) {
    assert(state == State::Pending);
    assert(jointMats);

    if (state != State::Pending) {
        state = State::Invalid;
        historyValidFrameSerial = UINT64_MAX;
        return false;
    }

    BE1::Mat3x4 *currentPosePtr = skinningJoints + currentPoseIndex * numJoints;
    BE1::simdProcessor->MultiplyJoints(currentPosePtr, jointMats, skeleton->GetInvBindPoseMatrices(), numJoints);
    state = State::Ready;
    return true;
}
