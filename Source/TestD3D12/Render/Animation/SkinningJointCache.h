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

                            // 이전 joint pose 를 사용할 수 있는지 여부를 확인한다.
    bool                    HasPreviousPose(uint64_t currentFrameSerial) const { return historyValidFrameSerial == currentFrameSerial; }

                            // 계산이 완료된 캐시의 joint pose 버전과 skeleton 데이터 버전 및 joint 수가 입력과 일치하면 true 를 반환한다.
    bool                    IsUpToDate(const Skeleton *skeleton, uint64_t jointMatsSerial) const;

                            // 메인 스레드에서 joint pose 와 skeleton 버전을 확인하여 필요한 캐시 갱신을 예약한다.
                            // 업데이트할 버퍼와 이전 포즈의 유효 프레임을 설정하고 Pending 상태로 전환한다.
                            // 같은 캐시의 중복 갱신을 막기 위해 true 를 반환한 경우에만 Update 를 실행한다.
    bool                    PrepareUpdate(const Skeleton *skeleton, uint64_t jointMatsSerial, uint64_t currentFrameSerial);

                            // 예약된 인스턴스별 작업은 CPU 메모리에만 기록한다. GPU 업로드는 작업 완료 후 별도로 수행한다.
    bool                    Update(const Skeleton *skeleton, const BE1::Mat3x4 *jointMats);

private:
    enum class State : uint8_t {
        Invalid,
        Pending,
        Ready
    };

    int                     numJoints = 0;
    BE1::Mat3x4 *           skinningJoints = nullptr;
    uint32_t                currentPoseIndex = 0;

    uint64_t                jointMatsSerial = UINT64_MAX;
    uint64_t                historyValidFrameSerial = UINT64_MAX; // 이전 포즈를 사용할 수 있는 프레임. UINT64_MAX 이면 이력이 없다는 의미.
    uint32_t                skeletonDataSerial = 0;
    State                   state = State::Invalid;
};
