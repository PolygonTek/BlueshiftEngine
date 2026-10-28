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
#include "Render/Mesh/MeshInstance.h"
#include "Render/RenderFrame.h"
#include "Render/RenderInternal.h"
#include "Shaders/ShaderInterop.h"

SkinningJointBufferBuilder::SkinningJointBufferBuilder() :
    taskGroupId(BE1::Engine::taskManager->CreateGroupId()) {
}

void SkinningJointBufferBuilder::UpdateJointCaches(RenderFrame *frame, const FrameSkinnedMesh **objects, uint32_t numObjects, uint32_t numSkinningJoints) {
    PROFILER_CPU_SCOPED_EVENT("SkinningJointBufferBuilder::UpdateJointCaches");

    if (numObjects == 0 || numSkinningJoints == 0) {
        return;
    }

#ifdef USE_RENDER_TASK
    static constexpr uint32_t MaxSkinningJointsPerTask = 8192;

    uint32_t numTasks = (numSkinningJoints + MaxSkinningJointsPerTask - 1) / MaxSkinningJointsPerTask;
    numTasks = BE1::Min(frame->NumRenderTaskThreads(), numTasks);

    if (numTasks > 1) {
        UpdateJointCachesWithTask(objects, numObjects, numSkinningJoints, numTasks);
    } else {
        UpdateJointCachesWithoutTask(objects, numObjects);
    }
#else
    UpdateJointCachesWithoutTask(objects, numObjects);
#endif
}

void SkinningJointBufferBuilder::UpdateJointCachesWithoutTask(const FrameSkinnedMesh **objects, uint32_t numObjects) {
    PROFILER_CPU_SCOPED_EVENT("SkinningJointBufferBuilder::UpdateJointCachesWithoutTask");

    for (uint32_t i = 0; i < numObjects; ++i) {
        const FrameSkinnedMesh *frameSkinnedMesh = objects[i];
        frameSkinnedMesh->meshInstance->UpdateSkinningJointCache(frameSkinnedMesh->skeleton);
    }
}

void SkinningJointBufferBuilder::UpdateJointCachesTask(void *data) {
    PROFILER_CPU_SCOPED_EVENT("SkinningJointBufferBuilder::UpdateJointCachesTask");

    const UpdateTaskDesc *taskDesc = static_cast<const UpdateTaskDesc *>(data);
    for (uint32_t i = 0; i < taskDesc->numObjects; ++i) {
        const FrameSkinnedMesh *frameSkinnedMesh = taskDesc->objects[i];
        frameSkinnedMesh->meshInstance->UpdateSkinningJointCache(frameSkinnedMesh->skeleton);
    }
}

void SkinningJointBufferBuilder::UpdateJointCachesWithTask(const FrameSkinnedMesh **objects, uint32_t numObjects, uint32_t numSkinningJoints, uint32_t numTasks) {
    PROFILER_CPU_SCOPED_EVENT("SkinningJointBufferBuilder::UpdateJointCachesWithTask");

    const uint32_t targetJointsPerTask = (numSkinningJoints + numTasks - 1) / numTasks;
    uint32_t firstIndex = 0;

    updateTaskDescs.Reserve(numTasks);
    updateTaskDescs.SetCount(0, false);

    while (firstIndex < numObjects) {
        uint32_t endIndex = firstIndex;
        uint32_t numTaskJoints = 0;
        const bool lastTask = updateTaskDescs.Count() + 1 == static_cast<int>(numTasks);
        do {
            numTaskJoints += objects[endIndex++]->meshInstance->NumJointMats();
        } while (endIndex < numObjects && (lastTask || numTaskJoints < targetJointsPerTask));

        UpdateTaskDesc &taskDesc = updateTaskDescs.Alloc();
        taskDesc.objects = objects + firstIndex;
        taskDesc.numObjects = endIndex - firstIndex;
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

static int CountSkinningUploadJoints(const RenderFrame::ChunkRange<FrameSkinnedMesh> &frameSkinnedMeshes, bool needVelocityOutput, uint64_t currentFrameSerial) {
    uint32_t numTotalUploadJoints = 0;

    for (const FrameSkinnedMesh &frameSkinnedMesh : frameSkinnedMeshes) {
        if (!frameSkinnedMesh.space->IsRenderable()) {
            continue;
        }

        const SkinningJointCache *skinningJointCache = frameSkinnedMesh.meshInstance->GetSkinningJointCache();
        if (!skinningJointCache) {
            continue;
        }

        // 캐시가 최신 상태가 아니라면 업로드를 하지 않는다.
        if (!skinningJointCache->IsUpToDate(frameSkinnedMesh.skeleton, frameSkinnedMesh.meshInstance->GetJointMatsSerial())) {
            continue;
        }

        const uint32_t numJoints = static_cast<uint32_t>(skinningJointCache->NumJoints());
        const bool needPreviousPose = needVelocityOutput && BE1::HasFlag(frameSkinnedMesh.space->flags, FrameObject::Flag::CameraVisible);
        const uint32_t numPoses = needPreviousPose && skinningJointCache->HasPreviousPose(currentFrameSerial) ? 2u : 1u;
        const uint32_t numUploadJoints = numJoints * numPoses;

        if (numUploadJoints > (UINT32_MAX / sizeof(ShaderSkinningJoint) - numTotalUploadJoints)) {
            return -1;
        }

        numTotalUploadJoints += numUploadJoints;
    }
    return static_cast<int>(numTotalUploadJoints);
}

void SkinningJointBufferBuilder::Build(RenderFrame *frame, FrameCamera *frameCamera) {
    PROFILER_CPU_SCOPED_EVENT("SkinningJointBufferBuilder::Build");

    assert(BE1::Engine::IsInMainThread());

    if (frame->NumFrameSkinnedMeshes() == frameCamera->firstFrameSkinnedMeshIndex) {
        return;
    }

    updateObjects.SetCount(0, false);

    uint32_t numTotalJoints = 0;

    const auto frameSkinnedMeshes = frame->GetFrameSkinnedMeshes(frameCamera->firstFrameSkinnedMeshIndex);
    const uint64_t currentFrameSerial = frame->GetFrameSerial();

    // 카메라/CSM/local shadow 가 공유하는 FrameSkinnedMesh 를 순회하여 광원 수와 무관하게 한 번만 준비한다.
    for (const FrameSkinnedMesh &frameSkinnedMesh : frameSkinnedMeshes) {
        if (!frameSkinnedMesh.space->IsRenderable()) {
            continue;
        }

        if (!frameSkinnedMesh.skeleton->IsReady() || !frameSkinnedMesh.meshInstance->EnsureCurrent()) {
            continue;
        }

        SkinningJointCache *skinningJointCache = frameSkinnedMesh.meshInstance->GetSkinningJointCache();
        if (!skinningJointCache) {
            continue;
        }

        // 캐시의 joint pose 나 skeleton 이 변경되었다면 업데이트를 예약한다.
        // 캐시 업데이트가 예약된 상태이거나 이미 업데이트된 상태라면 건너뛴다.
        if (!skinningJointCache->PrepareUpdate(frameSkinnedMesh.skeleton, frameSkinnedMesh.meshInstance->GetJointMatsSerial(), currentFrameSerial)) {
            continue;
        }

        updateObjects.Alloc() = &frameSkinnedMesh;

        numTotalJoints += skinningJointCache->NumJoints();
    }

    // 앞선 카메라나 프레임에서 이미 갱신된 캐시도 현재 카메라의 jointBuffer 에 업로드해야 한다.
    UpdateJointCaches(frame, updateObjects.Ptr(), updateObjects.Count(), numTotalJoints);

    // jointBuffer 에 업로드할 전체 joint pose 개수를 계산한다. 카메라 velocity 에 필요한 이전 pose 만 포함한다.
    const int numTotalUploadJoints = CountSkinningUploadJoints(frameSkinnedMeshes, frameCamera->needVelocityOutput, currentFrameSerial);
    if (numTotalUploadJoints < 0) {
        BE_WARNLOG("SkinningJointBufferBuilder::Build: too many skinning joints\n");
        return;
    }

    if (numTotalUploadJoints == 0) {
        return;
    }

    // 전체 joint pose 를 한 번에 업로드하기 위해서 업로드 버퍼를 할당한다.
    RHI::Buffer *jointBuffer = frame->GetThreadData(0)->AllocStructuredBuffer(sizeof(ShaderSkinningJoint), static_cast<uint32_t>(numTotalUploadJoints));
    if (!jointBuffer) {
        return;
    }

    // 업로드 버퍼에는 메인 스레드에서 연속으로 복사한다.
    BE1::Mat3x4 *dst = static_cast<BE1::Mat3x4 *>(jointBuffer->mappedPtr);
    uint32_t jointOffset = 0;
    for (FrameSkinnedMesh &frameSkinnedMesh : frameSkinnedMeshes) {
        if (!frameSkinnedMesh.space->IsRenderable()) {
            continue;
        }

        const SkinningJointCache *skinningJointCache = frameSkinnedMesh.meshInstance->GetSkinningJointCache();
        if (!skinningJointCache) {
            continue;
        }

        if (!skinningJointCache->IsUpToDate(frameSkinnedMesh.skeleton, frameSkinnedMesh.meshInstance->GetJointMatsSerial())) {
            continue;
        }

        const uint32_t numJoints = static_cast<uint32_t>(skinningJointCache->NumJoints());
        const uint32_t byteSize = numJoints * sizeof(ShaderSkinningJoint);

        BE1::simdProcessor->Memcpy(dst + jointOffset, skinningJointCache->GetSkinningJoints(), byteSize);
        frameSkinnedMesh.skinningJointOffset = jointOffset;
        jointOffset += numJoints;

        frameSkinnedMesh.prevSkinningJointOffset = frameSkinnedMesh.skinningJointOffset;

        const bool needPreviousPose = frameCamera->needVelocityOutput && BE1::HasFlag(frameSkinnedMesh.space->flags, FrameObject::Flag::CameraVisible);
        if (needPreviousPose && skinningJointCache->HasPreviousPose(currentFrameSerial)) {
            BE1::simdProcessor->Memcpy(dst + jointOffset, skinningJointCache->GetPrevSkinningJoints(), byteSize);
            frameSkinnedMesh.prevSkinningJointOffset = jointOffset;
            jointOffset += numJoints;
        }
    }

    assert(jointOffset == static_cast<uint32_t>(numTotalUploadJoints));
    frameCamera->skinningJointBuffer = jointBuffer;
}
