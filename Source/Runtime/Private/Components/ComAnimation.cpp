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
#include "Components/Transform/ComTransform.h"
#include "Components/Transform/JointHierarchy.h"
#include "Components/Renderable/ComSkinnedMeshRenderer.h"
#include "Components/ComAnimation.h"
#include "Game/GameWorld.h"
#include "Asset/Asset.h"
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"
#include "Core/JointPose.h"
#include "SIMD/SIMD.h"

BE_NAMESPACE_BEGIN

const SignalDef ComAnimation::SIG_AnimUpdated("ComAnimation::SIG_AnimUpdated", "ai");

OBJECT_DECLARATION("Animation", ComAnimation, Component)
BEGIN_EVENTS(ComAnimation)
END_EVENTS

void ComAnimation::RegisterProperties() {
    REGISTER_PROPERTY("rootBoneTransform", "Root Bone Transform", Guid, rootBoneTransformGuid, Guid::zero,
        "", PropertyInfo::Flag::Editor).SetMetaObject(&ComTransform::metaObject);
    REGISTER_MIXED_ACCESSOR_PROPERTY("skeleton", "Skeleton", Guid, GetSkeletonGuid, SetSkeletonGuid, Guid::zero,
        "", PropertyInfo::Flag::Empty).SetMetaObject(&SkeletonResource::metaObject);
    REGISTER_MIXED_ACCESSOR_ARRAY_PROPERTY("anims", "Animations", Guid, GetAnimGuid, SetAnimGuid, GetAnimCount, SetAnimCount, Guid::zero,
        "", PropertyInfo::Flag::Editor).SetMetaObject(&AnimResource::metaObject);
    REGISTER_PROPERTY("animIndex", "Animation Index", int, currentAnimIndex, 0,
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("timeOffset", "Time Offset", float, GetTimeOffset, SetTimeOffset, 0.f,
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("timeScale", "Time Scale", float, GetTimeScale, SetTimeScale, 1.f,
        "", PropertyInfo::Flag::Editor);
}

ComAnimation::ComAnimation() {
    updatable = true;
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

void ComAnimation::LateInit() {
    rootBoneTransform = (ComTransform *)ComTransform::FindInstance(rootBoneTransformGuid);
    if (rootBoneTransform && !rootBoneTransform->GetJointHierarchy()) {
        rootBoneTransform->ConstructJointHierarchy();
    }
}

void ComAnimation::Update() {
    int currentTime = (GetGameWorld()->GetTime() - playStartTime) * GetTimeScale() + Math::FtoiFast(SEC2MILLI(GetTimeOffset()));

    UpdateAnim(currentTime);
}

void ComAnimation::UpdateAnim(int animTime) {
    if (currentAnimIndex < 0 || currentAnimIndex >= anims.Count()) {
        return;
    }

    if (!rootBoneTransform) {
        return;
    }

    const Anim *currentAnim = anims[currentAnimIndex];
    if (currentAnim) {
        JointHierarchy *jointHierarchy = rootBoneTransform->GetJointHierarchy();

        if (jointHierarchy) {
            Anim::FrameInterpolation frameInterpolation;
            currentAnim->TimeToFrameInterpolation(animTime, frameInterpolation);

            JointPose *jointFrame = (JointPose *)_alloca16(jointHierarchy->NumJoints() * sizeof(jointFrame[0]));
            currentAnim->GetInterpolatedFrame(frameInterpolation, jointIndexes.Count(), jointIndexes.Ptr(), jointFrame);

            // Convert the joint quaternions to joint matrices.
            simdProcessor->ConvertJointPosesToJointMats(jointHierarchy->GetLocalJointMatrices(), jointFrame, jointHierarchy->NumJoints());

            rootBoneTransform->UpdateJointHierarchy(skeleton->GetJointParentIndexes());
        }
    }

    EmitSignal(&ComAnimation::SIG_AnimUpdated, this, animTime);
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
#if WITH_EDITOR
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

    for (int i = 0; i < skeleton->NumJoints(); i++) {
        jointIndexes[i] = i;
    }

    if (rootBoneTransform) {
        JointHierarchy *jointHierarchy = rootBoneTransform->GetJointHierarchy();

        // Set up joints to T-pose
        const JointPose *bindPoses = skeleton->GetBindPoses();
        simdProcessor->ConvertJointPosesToJointMats(jointHierarchy->GetLocalJointMatrices(), bindPoses, skeleton->NumJoints());

        rootBoneTransform->UpdateJointHierarchy(skeleton->GetJointParentIndexes());
    }

#if WITH_EDITOR
    // Need to connect skeleton asset to be reloaded in Editor
    skeletonAsset = (Asset *)Asset::FindInstance(skeletonGuid);
    if (skeletonAsset) {
        skeletonAsset->Connect(&Asset::SIG_Reloaded, this, (SignalCallback)&ComAnimation::SkeletonReloaded, SignalObject::ConnectionType::Queued);
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

#if WITH_EDITOR
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

#if WITH_EDITOR
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
#if WITH_EDITOR
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
        if (rootBoneTransform) {
            JointHierarchy *jointHierarchy = rootBoneTransform->GetJointHierarchy();
            if (jointHierarchy && jointHierarchy->NumJoints() != anims[index]->NumJoints()) {
                animManager.ReleaseAnim(anims[index]);
                anims[index] = nullptr;
                return;
            }

            // FIXME: Slow!!
            //anim->ComputeFrameAABBs(skeleton, referenceMesh, frameAABBs);
        }
    }

#if WITH_EDITOR
    // Need to connect anim asset to be reloaded in Editor
    animAssets[index] = (Asset *)Asset::FindInstance(animGuid);
    if (animAssets[index]) {
        animAssets[index]->Connect(&Asset::SIG_Reloaded, this, (SignalCallback)&ComAnimation::AnimReloaded, SignalObject::ConnectionType::Queued);
    }
#endif
}

float ComAnimation::GetTimeOffset() const {
    return timeOffset;
}

void ComAnimation::SetTimeOffset(float inTimeOffset) {
    timeOffset = inTimeOffset;
}

float ComAnimation::GetTimeScale() const {
    return timeScale;
}

void ComAnimation::SetTimeScale(float inTimeScale) {
    timeScale = inTimeScale;
}

Mat3x4 *ComAnimation::GetJointMatrices() const {
    if (rootBoneTransform) {
        JointHierarchy *jointHierarchy = rootBoneTransform->GetJointHierarchy();
        if (jointHierarchy) {
            return jointHierarchy->GetWorldJointMatrices();
        }
    }
    return nullptr;
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

    return MILLI2SEC(currentAnim->Length());
}

BE_NAMESPACE_END
