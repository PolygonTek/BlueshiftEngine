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
#include "Containers/Array.h"
#include "Containers/HashIndex.h"
#include "Render/RenderFrame.h"

namespace BE1 { class Mat3x4; }

class SkinningPose;
struct FrameCamera;

/*
---------------------------------------------------------------------------------------------------

    Skinning Joint Buffer Builder

    카메라별 스키닝 캐시 갱신과 joint buffer 업로드를 담당한다.
    작업 그룹과 임시 배열을 소유하며, pose 캐시는 메시 파트들이 공유하는 SkinningPose 에서 관리한다.

---------------------------------------------------------------------------------------------------
*/

class SkinningJointBufferBuilder {
public:
    SkinningJointBufferBuilder();
    SkinningJointBufferBuilder(const SkinningJointBufferBuilder &) = delete;
    SkinningJointBufferBuilder &operator=(const SkinningJointBufferBuilder &) = delete;

                                    // 메인 스레드에서 호출한다. 모든 캐시 갱신 작업을 완료한 뒤 업로드하고 반환한다.
    void                            Build(RenderFrame *frame, FrameCamera *frameCamera);

private:
    struct SkinningPoseEntry {
        SkinningPose *              pose = nullptr;
        const BE1::Mat3x4 *         uploadJoints = nullptr;
        uint32_t                    numUploadJoints = 0;
        uint32_t                    skinningJointOffset = UINT32_MAX;
        uint32_t                    prevSkinningJointOffset = UINT32_MAX;
        bool                        needPreviousPose = false;
    };

    struct UpdateTaskDesc {
        SkinningPose *const *       poses = nullptr;
        uint32_t                    numPoses = 0;
    };

    void                            UpdateJointCaches(RenderFrame *frame, SkinningPose *const *poses, uint32_t numPoses, uint32_t numSkinningJoints);
    static void                     UpdateJointCachesWithoutTask(SkinningPose *const *poses, uint32_t numPoses);
    void                            UpdateJointCachesWithTask(SkinningPose *const *poses, uint32_t numPoses, uint32_t numSkinningJoints, uint32_t numTasks);
    static void                     UpdateJointCachesTask(void *data);

                                    // 갱신 완료된 고유 포즈의 업로드 구간과 offset 을 정한다. 버퍼 크기 초과 시 -1 을 반환한다.
    int                             PrepareSkinningUploads(uint64_t currentFrameSerial);

                                    // 갱신된 캐시를 업로드하고 현재 카메라의 메시별 offset 과 joint buffer 를 설정한다.
    void                            UploadJointBuffer(RenderFrame *frame, FrameCamera *frameCamera, const RenderFrame::ChunkRange<FrameSkinnedMesh> &frameSkinnedMeshes);

    int32_t                         taskGroupId;
    BE1::Array<UpdateTaskDesc>      updateTaskDescs;

                                    // 현재 Build 의 고유 포즈와 메시별 포즈 인덱스. 저장 공간은 카메라/프레임 사이에 재사용한다.
                                    // meshPoseIndexes 는 현재 카메라의 FrameSkinnedMesh 순서와 대응하며, 제외된 메시에는 -1 을 저장한다.
    BE1::Array<SkinningPoseEntry>   skinningPoses = BE1::Array<SkinningPoseEntry>(256);
    BE1::HashIndex                  skinningPosesHash { 4096, 4096 };
    BE1::Array<int32_t>             meshPoseIndexes = BE1::Array<int32_t>(256);
    BE1::Array<SkinningPose *>      updatePoses = BE1::Array<SkinningPose *>(256);
};
