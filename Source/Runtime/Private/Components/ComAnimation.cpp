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
#include "Render/Render.h"
#include "Components/ComAnimation.h"
#include "Components/ComSkinnedMeshRenderer.h"
#include "Game/GameWorld.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Core/JointPose.h"
#include "Simd/Simd.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Animation", ComAnimation, Component)
BEGIN_EVENTS(ComAnimation)
END_EVENTS

void ComAnimation::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("skeleton", "Skeleton", Guid, GetSkeletonGuid, SetSkeletonGuid, Guid::zero,
        "", PropertyInfo::EditorFlag).SetMetaObject(&SkeletonAsset::metaObject);
    REGISTER_MIXED_ACCESSOR_ARRAY_PROPERTY("anims", "Animations", Guid, GetAnimGuid, SetAnimGuid, GetAnimCount, SetAnimCount, Guid::zero,
        "", PropertyInfo::EditorFlag).SetMetaObject(&AnimAsset::metaObject);
    REGISTER_PROPERTY("animIndex", "Animation Index", int, currentAnimIndex, 0,
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("timeOffset", "Time Offset", float, GetTimeOffset, SetTimeOffset, 0.f,
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("timeScale", "Time Scale", float, GetTimeScale, SetTimeScale, 1.f,
        "", PropertyInfo::EditorFlag);
}

ComAnimation::ComAnimation() {
    skeleton = nullptr;

    skeletonAsset = nullptr;

    jointMats = nullptr;

    currentAnimIndex = 0;

    timeOffset = 0;
    timeScale = 1.0f;
}

ComAnimation::~ComAnimation() {
    Purge(false);
}

bool ComAnimation::IsConflictComponent(const MetaObject &componentClass) const {
    if (componentClass.IsTypeOf(ComAnimation::metaObject)) {
        return true;
    }
    return false;
}

void ComAnimation::Purge(bool chainPurge) {
    if (jointMats) {
        Mem_AlignedFree(jointMats);
        jointMats = nullptr;
    }

    if (skeleton) {
        skeletonManager.ReleaseSkeleton(skeleton);
        skeleton = nullptr;
    }

    for (int i = 0; i < anims.Count(); i++) {
        if (anims[i]) {
            animManager.ReleaseAnim(anims[i]);
            anims[i] = nullptr;
        }
    }
    anims.Clear();

    if (chainPurge) {
        Component::Purge();
    }
}

void ComAnimation::Init() {
    Component::Init();

    playStartTime = GetGameWorld()->GetTime();

    // Mark as initialized
    SetInitialized(true);
}

void ComAnimation::Update() {
    if (!IsInitialized() || !IsActiveInHierarchy()) {
        return;
    }

    int currentTime = (GetGameWorld()->GetTime() - playStartTime) * GetTimeScale() + SEC2MS(GetTimeOffset());

    UpdateAnim(currentTime);
}

void ComAnimation::UpdateAnim(int animTime) {
    if (currentAnimIndex < 0 || currentAnimIndex >= anims.Count()) {
        return;
    }

    const Anim *currentAnim = anims[currentAnimIndex];
    if (!currentAnim) {
        return;
    }
    
    Anim::FrameInterpolation frameInterpolation;
    currentAnim->TimeToFrameInterpolation(animTime, frameInterpolation);

    JointPose *jointFrame = (JointPose *)_alloca16(skeleton->NumJoints() * sizeof(jointFrame[0]));
    currentAnim->GetInterpolatedFrame(frameInterpolation, jointIndexes.Count(), jointIndexes.Ptr(), jointFrame);

    simdProcessor->ConvertJointPosesToJointMats(jointMats, jointFrame, skeleton->NumJoints());

    // Convert joint matrices from local space to world space
    simdProcessor->TransformJoints(jointMats, jointParents.Ptr(), 1, skeleton->NumJoints() - 1);
}

Guid ComAnimation::GetSkeletonGuid() const {
    if (!skeleton) {
        return Guid();
    }

    const Str skeletonPath = skeleton->GetHashName();
    return resourceGuidMapper.Get(skeletonPath);
}

void ComAnimation::SetSkeletonGuid(const Guid &guid) {
    ChangeSkeleton(guid);

    EmitSignal(&ComSkinnedMeshRenderer::SIG_SkeletonUpdated, this);
}

void ComAnimation::SkeletonReloaded() {
    SetSkeletonGuid(GetProperty("skeleton").As<Guid>());
}

void ComAnimation::ChangeSkeleton(const Guid &skeletonGuid) {
#if 1
    // Disconnect with previously connected skeleton asset
    if (skeletonAsset) {
        skeletonAsset->Disconnect(&Asset::SIG_Reloaded, this);
        skeletonAsset = nullptr;
    }
#endif

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

    if (jointMats) {
        Mem_AlignedFree(jointMats);
    }
    jointMats = (Mat3x4 *)Mem_Alloc16(skeleton->NumJoints() * sizeof(jointMats[0]));

    // Set up joints to T-pose
    const JointPose *bindPoses = skeleton->GetBindPoses();
    simdProcessor->ConvertJointPosesToJointMats(jointMats, bindPoses, skeleton->NumJoints());

    // Convert joint matrices from local space to world space
    simdProcessor->TransformJoints(jointMats, jointParents.Ptr(), 1, skeleton->NumJoints() - 1);

    //anim->ComputeFrameAABBs(skeleton, referenceMesh, frameAABBs);

#if 1
    // Need to connect skeleton asset to be reloaded in Editor
    skeletonAsset = (SkeletonAsset *)SkeletonAsset::FindInstance(skeletonGuid);
    if (skeletonAsset) {
        skeletonAsset->Connect(&Asset::SIG_Reloaded, this, (SignalCallback)&ComAnimation::SkeletonReloaded, SignalObject::Queued);
    }
#endif
}

Guid ComAnimation::GetAnimGuid(int index) const {
    if (index < 0 || index >= anims.Count()) {
        return Guid();
    }

    const Anim *anim = anims[index];
    if (!anim) {
        return Guid();
    }

    const Str animPath = anim->GetHashName();
    return resourceGuidMapper.Get(animPath);
}

void ComAnimation::SetAnimGuid(int index, const Guid &guid) {
    ChangeAnim(index, guid);
}

int ComAnimation::GetAnimCount() const {
    return anims.Count();
}

void ComAnimation::SetAnimCount(int count) {
    int oldCount = anims.Count();

    if (count < oldCount) {
        for (int index = count; index < oldCount; index++) {
            if (anims[index]) {
                animManager.ReleaseAnim(anims[index]);
                anims[index] = nullptr;
            }

#if 1
            if (animAssets[index]) {
                animAssets[index]->Disconnect(&Asset::SIG_Reloaded, this);
                animAssets[index] = nullptr;
            }
#endif
        }
    }

    anims.SetCount(count);
    animAssets.SetCount(count);

    if (count > oldCount) {
        for (int index = oldCount; index < count; index++) {
            anims[index] = nullptr;

#if 1
            animAssets[index] = nullptr;
#endif
        }
    }
}

void ComAnimation::AnimReloaded() {
    for (int animIndex = 0; animIndex < anims.Count(); animIndex++) {
        SetAnimGuid(animIndex, GetArrayProperty("anims", animIndex).As<Guid>());
    }
}

void ComAnimation::ChangeAnim(int index, const Guid &animGuid) {
#if 1
    // Disconnect with previously connected anim asset
    if (animAssets[index]) {
        animAssets[index]->Disconnect(&Asset::SIG_Reloaded, this);
        animAssets[index] = nullptr;
    }
#endif

    // Release the previously used anim
    if (anims[index]) {
        animManager.ReleaseAnim(anims[index]);
        anims[index] = nullptr;
    }

    const Str animPath = resourceGuidMapper.Get(animGuid);
    if (!animPath.IsEmpty()) {
        anims[index] = animManager.GetAnim(animPath);
    }

    if (anims[index]) {
        if (skeleton) {
            if (skeleton->NumJoints() != anims[index]->NumJoints()) {
                // Set up joints to T-pose
                const JointPose *bindPoses = skeleton->GetBindPoses();
                simdProcessor->ConvertJointPosesToJointMats(jointMats, bindPoses, skeleton->NumJoints());

                // Convert joint matrices from local space to world space
                simdProcessor->TransformJoints(jointMats, jointParents.Ptr(), 1, skeleton->NumJoints() - 1);

                animManager.ReleaseAnim(anims[index]);
                anims[index] = nullptr;
                return;
            }

            // FIXME: Slow!!
            //anim->ComputeFrameAABBs(skeleton, referenceMesh, frameAABBs);
        }
    }

#if 1
    // Need to connect anim asset to be reloaded in Editor
    animAssets[index] = (AnimAsset *)AnimAsset::FindInstance(animGuid);
    if (animAssets[index]) {
        animAssets[index]->Connect(&Asset::SIG_Reloaded, this, (SignalCallback)&ComAnimation::AnimReloaded, SignalObject::Queued);
    }
#endif
}

float ComAnimation::GetTimeOffset() const {
    return timeOffset;
}

void ComAnimation::SetTimeOffset(float timeOffset) {
    this->timeOffset = timeOffset;
}

float ComAnimation::GetTimeScale() const {
    return timeScale;
}

void ComAnimation::SetTimeScale(float timeScale) {
    this->timeScale = timeScale;
}

const Anim *ComAnimation::GetCurrentAnim() const {
    if (currentAnimIndex < 0 || currentAnimIndex >= anims.Count()) {
        return nullptr;
    }

    const Anim *currentAnim = anims[currentAnimIndex];
    if (!currentAnim) {
        return nullptr;
    }

    return currentAnim;
}

float ComAnimation::GetCurrentAnimSeconds() const {
    if (currentAnimIndex < 0 || currentAnimIndex >= anims.Count()) {
        return 0;
    }

    const Anim *currentAnim = anims[currentAnimIndex];
    if (!currentAnim) {
        return 0;
    }

    return MS2SEC(currentAnim->Length());
}

BE_NAMESPACE_END
