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
#include "SkinningJointBufferBuilder.h"
#include "Render/Animation/Skeleton.h"
#include "Render/Animation/SkinningJointCache.h"
#include "Render/Animation/SkinningPose.h"
#include "Render/Mesh/MeshInstance.h"
#include "Render/RenderFrame.h"
#include "Render/RenderInternal.h"
#include "Shaders/ShaderInterop.h"

SkinningJointBufferBuilder::SkinningJointBufferBuilder() :
    taskGroupId(BE1::Engine::taskManager->CreateGroupId()) {
}

void SkinningJointBufferBuilder::UpdateJointCaches(RenderFrame *frame, SkinningPose *const *poses, uint32_t numPoses, uint32_t numSkinningJoints) {
    PROFILER_CPU_SCOPED_EVENT("SkinningJointBufferBuilder::UpdateJointCaches");

    if (numPoses == 0 || numSkinningJoints == 0) {
        return;
    }

#ifdef USE_RENDER_TASK
    static constexpr uint32_t MaxSkinningJointsPerTask = 8192;

    uint32_t numTasks = (numSkinningJoints + MaxSkinningJointsPerTask - 1) / MaxSkinningJointsPerTask;
    numTasks = BE1::Min(frame->NumRenderTaskThreads(), numTasks);

    if (numTasks > 1) {
        UpdateJointCachesWithTask(poses, numPoses, numSkinningJoints, numTasks);
    } else {
        UpdateJointCachesWithoutTask(poses, numPoses);
    }
#else
    UpdateJointCachesWithoutTask(poses, numPoses);
#endif
}

void SkinningJointBufferBuilder::UpdateJointCachesWithoutTask(SkinningPose *const *poses, uint32_t numPoses) {
    PROFILER_CPU_SCOPED_EVENT("SkinningJointBufferBuilder::UpdateJointCachesWithoutTask");

    for (uint32_t i = 0; i < numPoses; ++i) {
        poses[i]->UpdateSkinningJointCache();
    }
}

void SkinningJointBufferBuilder::UpdateJointCachesTask(void *data) {
    PROFILER_CPU_SCOPED_EVENT("SkinningJointBufferBuilder::UpdateJointCachesTask");

    const UpdateTaskDesc *taskDesc = static_cast<const UpdateTaskDesc *>(data);
    for (uint32_t i = 0; i < taskDesc->numPoses; ++i) {
        taskDesc->poses[i]->UpdateSkinningJointCache();
    }
}

void SkinningJointBufferBuilder::UpdateJointCachesWithTask(SkinningPose *const *poses, uint32_t numPoses, uint32_t numSkinningJoints, uint32_t numTasks) {
    PROFILER_CPU_SCOPED_EVENT("SkinningJointBufferBuilder::UpdateJointCachesWithTask");

    const uint32_t targetJointsPerTask = (numSkinningJoints + numTasks - 1) / numTasks;
    uint32_t firstIndex = 0;

    updateTaskDescs.Reserve(numTasks);
    updateTaskDescs.SetCount(0, false);

    while (firstIndex < numPoses) {
        uint32_t endIndex = firstIndex;
        uint32_t numTaskJoints = 0;
        const bool lastTask = updateTaskDescs.Count() + 1 == static_cast<int>(numTasks);
        do {
            numTaskJoints += poses[endIndex++]->NumJointMats();
        } while (endIndex < numPoses && (lastTask || numTaskJoints < targetJointsPerTask));

        UpdateTaskDesc &taskDesc = updateTaskDescs.Alloc();
        taskDesc.poses = poses + firstIndex;
        taskDesc.numPoses = endIndex - firstIndex;
        firstIndex = endIndex;
    }

    for (int taskIndex = 0; taskIndex < updateTaskDescs.Count(); ++taskIndex) {
        UpdateTaskDesc &taskDesc = updateTaskDescs[taskIndex];
        if (!BE1::Engine::taskManager->AddTask(SkinningJointBufferBuilder::UpdateJointCachesTask, &taskDesc, taskGroupId, false)) {
            UpdateJointCachesTask(&taskDesc);
        }
    }

    BE1::Engine::taskManager->WaitFinish(taskGroupId, true);
}

int SkinningJointBufferBuilder::PrepareSkinningUploads(uint64_t currentFrameSerial) {
    uint32_t numTotalUploadJoints = 0;

    for (SkinningPoseEntry &entry : skinningPoses) {
        const SkinningJointCache *skinningJointCache = entry.pose->GetSkinningJointCache();
        if (!skinningJointCache) {
            continue;
        }

        if (!skinningJointCache->IsUpToDate(entry.pose->GetSkeleton(), entry.pose->GetJointMatsSerial())) {
            continue;
        }

        const uint32_t numJoints = static_cast<uint32_t>(skinningJointCache->NumJoints());
        const bool needPreviousPose = entry.needPreviousPose;
        const uint32_t numPoses = needPreviousPose && skinningJointCache->HasPreviousPose(currentFrameSerial) ? 2u : 1u;
        const uint64_t numUploadJoints = static_cast<uint64_t>(numJoints) * numPoses;

        if (numUploadJoints > (UINT32_MAX / sizeof(ShaderSkinningJoint) - numTotalUploadJoints)) {
            return -1;
        }

        entry.uploadJoints = skinningJointCache->GetSkinningJoints();
        entry.numUploadJoints = static_cast<uint32_t>(numUploadJoints);
        entry.skinningJointOffset = numTotalUploadJoints;
        entry.prevSkinningJointOffset = numTotalUploadJoints;

        if (numPoses == 2) {
            // 캐시의 current/previous pose 는 연속되어 있으므로 메모리 순서대로 한 번에 복사하고 offset 으로 구분한다.
            const BE1::Mat3x4 *prevSkinningJoints = skinningJointCache->GetPrevSkinningJoints();
            if (prevSkinningJoints < entry.uploadJoints) {
                entry.uploadJoints = prevSkinningJoints;
                entry.skinningJointOffset += numJoints;
            } else {
                entry.prevSkinningJointOffset += numJoints;
            }
        }

        numTotalUploadJoints += entry.numUploadJoints;
    }
    return static_cast<int>(numTotalUploadJoints);
}

void SkinningJointBufferBuilder::Build(RenderFrame *frame, FrameCamera *frameCamera) {
    PROFILER_CPU_SCOPED_EVENT("SkinningJointBufferBuilder::Build");

    // 현재 카메라에서 새로 등록된 FrameSkinnedMesh 가 없으면 처리를 생략한다.
    if (frame->NumFrameSkinnedMeshes() == frameCamera->firstFrameSkinnedMeshIndex) {
        return;
    }

    skinningPoses.SetCount(0, false);
    skinningPosesHash.Clear();
    meshPoseIndexes.SetCount(frame->NumFrameSkinnedMeshes() - frameCamera->firstFrameSkinnedMeshIndex, false);
    updatePoses.SetCount(0, false);

    const uint64_t currentFrameSerial = frame->GetFrameSerial();
    uint32_t numTotalUpdateJoints = 0;
    int meshIndex = 0;

    // skinned mesh 별로 고유 포즈를 찾아서 캐시 갱신을 예약한다. 이미 갱신된 캐시는 건너뛴다.
    const auto frameSkinnedMeshes = frame->GetFrameSkinnedMeshes(frameCamera->firstFrameSkinnedMeshIndex);
    for (const FrameSkinnedMesh &frameSkinnedMesh : frameSkinnedMeshes) {
        int32_t &poseIndex = meshPoseIndexes[meshIndex++];
        poseIndex = -1;

        if (!frameSkinnedMesh.space->NeedsRendering()) {
            continue;
        }

        if (!frameSkinnedMesh.skeleton->IsReady() || !frameSkinnedMesh.meshInstance->SyncWithSource()) {
            continue;
        }

        SkinningPose *pose = frameSkinnedMesh.meshInstance->GetSkinningPose();
        if (!pose || pose->GetSkeleton() != frameSkinnedMesh.skeleton || !pose->GetSkinningJointCache()) {
            continue;
        }

        const bool needPreviousPose = frameCamera->needVelocityOutput && frameSkinnedMesh.space->IsVisible();

        const int hashKey = skinningPosesHash.GenerateHash(pose);
        for (poseIndex = skinningPosesHash.First(hashKey); poseIndex != -1; poseIndex = skinningPosesHash.Next(poseIndex)) {
            if (skinningPoses[poseIndex].pose == pose) {
                break;
            }
        }

        // 이미 계산된 포즈라면 needPreviousPose 만 갱신하고 스킵한다.
        if (poseIndex != -1) {
            skinningPoses[poseIndex].needPreviousPose |= needPreviousPose;
            continue;
        }

        poseIndex = skinningPoses.Count();
        SkinningPoseEntry &entry = skinningPoses.Alloc();
        entry = {};
        entry.pose = pose;
        entry.needPreviousPose = needPreviousPose;
        skinningPosesHash.Add(hashKey, poseIndex);

        // 캐시의 joint pose 나 skeleton 이 변경되었다면 고유 포즈당 한 번만 업데이트를 예약한다.
        // 캐시 업데이트가 예약된 상태이거나 이미 업데이트된 상태라면 건너뛴다.
        if (pose->PrepareSkinningJointCache(currentFrameSerial)) {
            updatePoses.Alloc() = pose;
            numTotalUpdateJoints += pose->NumJointMats();
        }
    }

    // 고유 포즈별로 예약된 캐시 갱신을 수행한다.
    UpdateJointCaches(frame, updatePoses.Ptr(), updatePoses.Count(), numTotalUpdateJoints);

    // 갱신된 캐시를 joint buffer 에 한꺼번에 업로드 한다.
    // 앞선 카메라나 프레임에서 이미 갱신된 캐시도 현재 카메라의 jointBuffer 에 업로드한다.
    UploadJointBuffer(frame, frameCamera, frameSkinnedMeshes);
}

void SkinningJointBufferBuilder::UploadJointBuffer(RenderFrame *frame, FrameCamera *frameCamera, const RenderFrame::ChunkRange<FrameSkinnedMesh> &frameSkinnedMeshes) {
    const int numTotalUploadJoints = PrepareSkinningUploads(frame->GetFrameSerial());
    if (numTotalUploadJoints < 0) {
        BE_WARNLOG("SkinningJointBufferBuilder::UploadJointBuffer: too many skinning joints\n");
        return;
    }

    if (numTotalUploadJoints == 0) {
        return;
    }

    RHI::Buffer *jointBuffer = frame->GetThreadData(0)->AllocStructuredBuffer(sizeof(ShaderSkinningJoint), static_cast<uint32_t>(numTotalUploadJoints));
    if (!jointBuffer) {
        return;
    }

    // 업로드 버퍼에 고유 포즈당 한 번씩 연속으로 복사한다.
    BE1::Mat3x4 *dst = static_cast<BE1::Mat3x4 *>(jointBuffer->mappedPtr);
    uint32_t jointOffset = 0;
    for (const SkinningPoseEntry &entry : skinningPoses) {
        if (entry.numUploadJoints == 0) {
            continue;
        }

        BE1::simdProcessor->Memcpy(dst + jointOffset, entry.uploadJoints, entry.numUploadJoints * sizeof(ShaderSkinningJoint));
        jointOffset += entry.numUploadJoints;
    }

    assert(jointOffset == static_cast<uint32_t>(numTotalUploadJoints));

    // 각 skinned mesh 별로 고유 포즈를 찾아서 joint buffer offset 을 설정한다.
    int meshIndex = 0;
    for (FrameSkinnedMesh &frameSkinnedMesh : frameSkinnedMeshes) {
        const int32_t poseIndex = meshPoseIndexes[meshIndex++];
        if (poseIndex < 0) {
            continue;
        }

        const SkinningPoseEntry &entry = skinningPoses[poseIndex];
        frameSkinnedMesh.skinningJointOffset = entry.skinningJointOffset;
        frameSkinnedMesh.prevSkinningJointOffset = entry.prevSkinningJointOffset;
    }

    frameCamera->skinningJointBuffer = jointBuffer;
}
