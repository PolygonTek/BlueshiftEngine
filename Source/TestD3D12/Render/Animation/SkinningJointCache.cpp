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

void SkinningJointCache::Invalidate() {
    jointMatsSerial = UINT64_MAX;
    historyFrameSerial = UINT64_MAX;
    skeletonDataSerial = 0;
    state = State::Invalid;
}

bool SkinningJointCache::IsUpToDate(const Skeleton *skeleton, uint64_t jointMatsSerial) const {
    return state == State::Ready && this->jointMatsSerial == jointMatsSerial && skeletonDataSerial == skeleton->GetDataSerial() && skeleton->NumJoints() == numJoints;
}

bool SkinningJointCache::PrepareUpdate(const Skeleton *skeleton, uint64_t jointMatsSerial, uint64_t currentFrameSerial) {
    if (skeleton->NumJoints() != numJoints) {
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

        // previous joint pose 가 갱신된 상태인지 검사
        if (this->jointMatsSerial + 1 == jointMatsSerial) {
            historyFrameSerial = currentFrameSerial;
        } else {
            historyFrameSerial = UINT64_MAX;
        }
    } else {
        historyFrameSerial = UINT64_MAX;
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
        historyFrameSerial = UINT64_MAX;
        return false;
    }

    BE1::Mat3x4 *currentPosePtr = skinningJoints + currentPoseIndex * numJoints;
    BE1::simdProcessor->MultiplyJoints(currentPosePtr, jointMats, skeleton->GetInvBindPoseMatrices(), numJoints);
    state = State::Ready;
    return true;
}
