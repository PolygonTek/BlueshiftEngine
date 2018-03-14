// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Platform/PlatformTime.h"
#include "Render/Render.h"
#include "Components/ComTransform.h"
#include "Components/ComStaticMeshRenderer.h"
#include "Components/ComSkinnedMeshRenderer.h"
#include "Game/GameWorld.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Core/JointPose.h"
#include "Simd/Simd.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Skinned Mesh Renderer", ComSkinnedMeshRenderer, ComMeshRenderer)
BEGIN_EVENTS(ComSkinnedMeshRenderer)
END_EVENTS

void ComSkinnedMeshRenderer::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("skeleton", "Skeleton", Guid, GetSkeletonGuid, SetSkeletonGuid, Guid::zero, "", PropertyInfo::EditorFlag)
        .SetMetaObject(&SkeletonAsset::metaObject);
    REGISTER_MIXED_ACCESSOR_PROPERTY("anim", "Animation", Guid, GetAnimGuid, SetAnimGuid, Guid::zero, "", PropertyInfo::EditorFlag)
        .SetMetaObject(&AnimAsset::metaObject);
}

ComSkinnedMeshRenderer::ComSkinnedMeshRenderer() {
    jointMats = nullptr;

    skeletonAsset = nullptr;
    skeleton = nullptr;

    animAsset = nullptr;
    anim = nullptr;
    animGuid = Guid::zero;
}

ComSkinnedMeshRenderer::~ComSkinnedMeshRenderer() {
    Purge(false);
}

bool ComSkinnedMeshRenderer::IsConflictComponent(const MetaObject &componentClass) const {
    if (componentClass.IsTypeOf(ComStaticMeshRenderer::metaObject)) {
        return true;
    }
    return false;
}

void ComSkinnedMeshRenderer::Purge(bool chainPurge) {
    if (jointMats) {
        Mem_AlignedFree(jointMats);
        jointMats = nullptr;
    }

    if (skeleton) {
        skeletonManager.ReleaseSkeleton(skeleton);
        skeleton = nullptr;
    }

    if (anim) {
        animManager.ReleaseAnim(anim);
        anim = nullptr;
    }

    if (chainPurge) {
        ComMeshRenderer::Purge();
    }
}

void ComSkinnedMeshRenderer::Init() {
    ComMeshRenderer::Init();

    if (!anim) {
        ChangeAnim(animGuid);
    }

    bool isCompatibleSkeleton = referenceMesh->IsCompatibleSkeleton(skeleton) ? true : false;

    // Set SceneEntity parameters
    sceneEntity.mesh = referenceMesh->InstantiateMesh(isCompatibleSkeleton ? Mesh::SkinnedMesh : Mesh::StaticMesh);
    sceneEntity.skeleton = isCompatibleSkeleton ? skeleton : nullptr;
    sceneEntity.numJoints = isCompatibleSkeleton ? skeleton->NumJoints() : 0;

    // Mark as initialized
    SetInitialized(true);

    UpdateVisuals();
}

void ComSkinnedMeshRenderer::ChangeSkeleton(const Guid &skeletonGuid) {
    // Disconnect with previously connected skeleton asset
    if (skeletonAsset) {
        skeletonAsset->Disconnect(&Asset::SIG_Reloaded, this);
        skeletonAsset = nullptr;
    }

    if (skeleton) {
        skeletonManager.ReleaseSkeleton(skeleton);
        skeleton = nullptr;
    }

    const Str skeletonPath = resourceGuidMapper.Get(skeletonGuid);
    skeleton = skeletonManager.GetSkeleton(skeletonPath);

    jointIndexes.SetGranularity(1);
    jointIndexes.SetCount(skeleton->NumJoints());

    jointParents.SetGranularity(1);
    jointParents.SetCount(skeleton->NumJoints());

    const Joint *skeletonJoints = skeleton->GetJoints();
    const Joint *skeletonJoint = skeletonJoints;

    for (int i = 0; i < skeleton->NumJoints(); i++, skeletonJoint++) {
        jointIndexes[i] = i;

        if (skeletonJoint->parent) {
            jointParents[i] = static_cast<int>(skeletonJoint->parent - skeletonJoints);
        } else {
            jointParents[i] = -1;
        }
    }

    // Set up initial pose for skeleton
    jointMats = (Mat3x4 *)Mem_Alloc16(skeleton->NumJoints() * sizeof(jointMats[0]));
    sceneEntity.joints = jointMats;

    const JointPose *bindPoses = skeleton->GetBindPoses();
    simdProcessor->ConvertJointPosesToJointMats(jointMats, bindPoses, skeleton->NumJoints());
    simdProcessor->TransformJoints(jointMats, jointParents.Ptr(), 1, skeleton->NumJoints() - 1);

    if (anim) {
        anim->ComputeFrameAABBs(skeleton, referenceMesh, frameAABBs);
    }

    // Need to connect skeleton asset to be reloaded in Editor
    skeletonAsset = (SkeletonAsset *)SkeletonAsset::FindInstance(skeletonGuid);
    if (skeletonAsset) {
        skeletonAsset->Connect(&Asset::SIG_Reloaded, this, (SignalCallback)&ComSkinnedMeshRenderer::SkeletonReloaded, SignalObject::Queued);
    }
}

void ComSkinnedMeshRenderer::ChangeAnim(const Guid &animGuid) {
    if (!referenceMesh) {
        this->animGuid = animGuid;
        return;
    }

    // Disconnect with previously connected anim asset
    if (animAsset) {
        animAsset->Disconnect(&Asset::SIG_Reloaded, this);
        animAsset = nullptr;
    }

    // Release the previously used anim
    if (anim) {
        animManager.ReleaseAnim(anim);
        anim = nullptr;
    }

    const Str animPath = resourceGuidMapper.Get(animGuid);
    anim = animManager.GetAnim(animPath);
     
    if (anim) {
        if (skeleton) {
            if (skeleton->NumJoints() != anim->NumJoints()) {
                const JointPose *bindPoses = skeleton->GetBindPoses();
                simdProcessor->ConvertJointPosesToJointMats(jointMats, bindPoses, skeleton->NumJoints());
                simdProcessor->TransformJoints(jointMats, jointParents.Ptr(), 1, skeleton->NumJoints() - 1);

                animManager.ReleaseAnim(anim);
                anim = nullptr;
                return;
            }
        }

        anim->ComputeFrameAABBs(skeleton, referenceMesh, frameAABBs);
    }

    playStartTime = GetGameWorld() ? GetGameWorld()->GetTime() : 0;
   
    // Need to connect anim asset to be reloaded in Editor
    animAsset = (AnimAsset *)AnimAsset::FindInstance(animGuid);
    if (animAsset) {
        animAsset->Connect(&Asset::SIG_Reloaded, this, (SignalCallback)&ComSkinnedMeshRenderer::AnimReloaded, SignalObject::Queued);
    }
}

void ComSkinnedMeshRenderer::Update() {
    UpdateVisuals();
}

void ComSkinnedMeshRenderer::UpdateVisuals() {
    if (!IsInitialized() || !IsActiveInHierarchy()) {
        return;
    }

    int currentTime = GetGameWorld()->GetTime();

    UpdateAnimation(currentTime);
}

void ComSkinnedMeshRenderer::UpdateAnimation(int currentTime) {
    if (anim) {
        int time = currentTime - playStartTime;
        Anim::FrameInterpolation frameInterpolation;
        anim->TimeToFrameInterpolation(time, frameInterpolation);

        JointPose *jointFrame = (JointPose *)_alloca16(skeleton->NumJoints() * sizeof(jointFrame[0]));
        anim->GetInterpolatedFrame(frameInterpolation, jointIndexes.Count(), jointIndexes.Ptr(), jointFrame);

        simdProcessor->ConvertJointPosesToJointMats(jointMats, jointFrame, skeleton->NumJoints());

        simdProcessor->TransformJoints(jointMats, jointParents.Ptr(), 1, skeleton->NumJoints() - 1);

        anim->GetAABB(sceneEntity.aabb, frameAABBs, time);
    } else {
        sceneEntity.aabb = referenceMesh->GetAABB();
    }

    ComRenderable::UpdateVisuals();
}

void ComSkinnedMeshRenderer::MeshUpdated() {
    if (!IsInitialized()) {
        return;
    }

    bool isCompatibleSkeleton = referenceMesh->IsCompatibleSkeleton(skeleton) ? true : false;

    if (isCompatibleSkeleton) {
        if (anim) {
            anim->ComputeFrameAABBs(skeleton, referenceMesh, frameAABBs);
        }

        sceneEntity.mesh = referenceMesh->InstantiateMesh(Mesh::SkinnedMesh);
        sceneEntity.skeleton = skeleton;
        sceneEntity.numJoints = skeleton->NumJoints();
    } else {
        sceneEntity.mesh = referenceMesh->InstantiateMesh(Mesh::StaticMesh);
        sceneEntity.skeleton = nullptr;
        sceneEntity.numJoints = 0;
    }

    // temp code
    renderWorld->RemoveEntity(sceneEntityHandle);
    sceneEntityHandle = -1;
    // temp code
    UpdateVisuals();
}

void ComSkinnedMeshRenderer::SkeletonReloaded() {
    ChangeSkeleton(GetProperty("skeleton").As<Guid>());

    MeshUpdated();
}

void ComSkinnedMeshRenderer::AnimReloaded() {
    ChangeAnim(GetProperty("anim").As<Guid>());

    MeshUpdated();
}

Guid ComSkinnedMeshRenderer::GetSkeletonGuid() const {
    if (skeleton) {
        const Str skeletonPath = skeleton->GetHashName();
        return resourceGuidMapper.Get(skeletonPath);
    }
    return Guid();
}

void ComSkinnedMeshRenderer::SetSkeletonGuid(const Guid &guid) {
    ChangeSkeleton(guid);

    MeshUpdated();
}

Guid ComSkinnedMeshRenderer::GetAnimGuid() const {
    if (anim) {
        const Str animPath = anim->GetHashName();
        return resourceGuidMapper.Get(animPath);
    }
    return Guid();
}

void ComSkinnedMeshRenderer::SetAnimGuid(const Guid &guid) {
    ChangeAnim(guid);

    MeshUpdated();
}

BE_NAMESPACE_END
