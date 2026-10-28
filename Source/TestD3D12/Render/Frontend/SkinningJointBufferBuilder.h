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

class RenderFrame;
struct FrameCamera;
struct FrameSkinnedMesh;

// 카메라별 스키닝 캐시 갱신과 joint buffer 업로드를 담당한다.
// 작업 그룹과 임시 배열을 소유하며, pose 캐시는 MeshInstance 에서 계속 관리한다.
class SkinningJointBufferBuilder {
public:
    SkinningJointBufferBuilder();
    SkinningJointBufferBuilder(const SkinningJointBufferBuilder &) = delete;
    SkinningJointBufferBuilder &operator=(const SkinningJointBufferBuilder &) = delete;

                                    // 메인 스레드에서 호출한다. 모든 캐시 갱신 작업을 완료한 뒤 업로드하고 반환한다.
    void                            Build(RenderFrame *frame, FrameCamera *frameCamera);

private:
    struct UpdateTaskDesc {
        const FrameSkinnedMesh **   objects = nullptr;
        uint32_t                    numObjects = 0;
    };

    void                            UpdateJointCaches(RenderFrame *frame, const FrameSkinnedMesh **objects, uint32_t numObjects, uint32_t numSkinningJoints);
    static void                     UpdateJointCachesWithoutTask(const FrameSkinnedMesh **objects, uint32_t numObjects);
    void                            UpdateJointCachesWithTask(const FrameSkinnedMesh **objects, uint32_t numObjects, uint32_t numSkinningJoints, uint32_t numTasks);
    static void                     UpdateJointCachesTask(void *data);

    int32_t                         taskGroupId;
    BE1::Array<UpdateTaskDesc>      updateTaskDescs;
    BE1::Array<const FrameSkinnedMesh *> updateObjects = BE1::Array<const FrameSkinnedMesh *>(256); // Reused across cameras/frames.
};
