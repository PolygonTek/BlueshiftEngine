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

#include <cstdint>

namespace BE1 { class Mat3x4; }

class Skeleton;

/*
---------------------------------------------------------------------------------------------------

    Skinning Joint Cache

    GPU 스키닝에 사용할 로컬 행렬을 계산해 보관하고, 이전 joint pose 이력까지 관리하는 CPU 캐시

---------------------------------------------------------------------------------------------------
*/

class SkinningJointCache {
public:
    explicit SkinningJointCache(int numJoints);
    SkinningJointCache(const SkinningJointCache &) = delete;
    SkinningJointCache &operator=(const SkinningJointCache &) = delete;
    ~SkinningJointCache();

    int                     NumJoints() const { return numJoints; }
    const BE1::Mat3x4 *     GetSkinningJoints() const;
    const BE1::Mat3x4 *     GetPrevSkinningJoints() const;

                            // 할당된 메모리를 유지한 채 현재 캐시와 이전 포즈 이력을 무효화한다.
    void                    Invalidate();

                            // 현재 프레임에서 이전 joint pose 이력을 사용할 수 있는지 여부를 리턴.
    bool                    HasPreviousPose(uint64_t currentFrameSerial) const { return historyFrameSerial == currentFrameSerial; }

                            // 캐시가 입력 포즈와 스켈레톤 기준으로 최신 상태인지 확인한다.
    bool                    IsUpToDate(const Skeleton *skeleton, uint64_t jointMatsSerial) const;

                            // 필요한 캐시 갱신을 예약한다. true 를 반환한 경우에만 Update 를 호출해야 한다.
    bool                    PrepareUpdate(const Skeleton *skeleton, uint64_t jointMatsSerial, uint64_t currentFrameSerial);

                            // 캐시를 갱신한다.
    bool                    Update(const Skeleton *skeleton, const BE1::Mat3x4 *jointMats);

private:
    enum class State : uint8_t {
        Invalid,            // 캐시가 유효하지 않은 상태
        Pending,            // 캐시 갱신이 예약된 상태
        Ready               // 캐시가 갱신된 상태
    };

    int                     numJoints = 0;
    BE1::Mat3x4 *           skinningJoints = nullptr;                   // 2 * numJoints 개의 joint pose 를 저장할 수 있는 메모리. current joint pose 와 previous joint pose 를 번갈아 사용한다.
    uint32_t                currentPoseIndex = 0;                       // 0 또는 1, skinningJoints 의 어느 절반을 현재 joint pose 로 사용할지 결정한다.

    uint64_t                jointMatsSerial = UINT64_MAX;
    uint64_t                historyFrameSerial = UINT64_MAX;            // previous pose 가 유효한 프레임 시리얼.
    uint32_t                skeletonDataSerial = 0;
    State                   state = State::Invalid;
};

BE_INLINE SkinningJointCache::SkinningJointCache(int numJoints) {
    assert(numJoints > 0);
    this->numJoints = numJoints;
    this->skinningJoints = static_cast<BE1::Mat3x4 *>(Mem_Alloc16(sizeof(BE1::Mat3x4) * numJoints * 2));
}

BE_INLINE SkinningJointCache::~SkinningJointCache() {
    Mem_AlignedFree(skinningJoints);
}

BE_INLINE const BE1::Mat3x4 *SkinningJointCache::GetSkinningJoints() const {
    assert(state == State::Ready);
    return skinningJoints + currentPoseIndex * numJoints;
}

BE_INLINE const BE1::Mat3x4 *SkinningJointCache::GetPrevSkinningJoints() const {
    assert(state == State::Ready);
    const uint32_t previousPoseIndex = historyFrameSerial != UINT64_MAX ? currentPoseIndex ^ 1u : currentPoseIndex;
    return skinningJoints + previousPoseIndex * numJoints;
}
