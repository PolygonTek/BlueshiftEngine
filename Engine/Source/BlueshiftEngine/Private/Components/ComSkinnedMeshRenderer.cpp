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
#include "AnimController/AnimController.h"
#include "Components/ComTransform.h"
#include "Components/ComStaticMeshRenderer.h"
#include "Components/ComSkinnedMeshRenderer.h"
#include "Game/Entity.h"
#include "Game/GameWorld.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Core/JointPose.h"
#include "Simd/Simd.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Skinned Mesh Renderer", ComSkinnedMeshRenderer, ComMeshRenderer)
BEGIN_EVENTS(ComSkinnedMeshRenderer)
END_EVENTS
BEGIN_PROPERTIES(ComSkinnedMeshRenderer)
    PROPERTY_ENUM("animationType", "Animation Type", "", "Animation Controller;Single Animation", "0", PropertySpec::ReadWrite),
    PROPERTY_OBJECT("animController", "Anim Controller", "", GuidMapper::defaultAnimControllerGuid.ToString(), AnimControllerAsset::metaObject, PropertySpec::ReadWrite),
    PROPERTY_OBJECT("skeleton", "Skeleton", "", "0", SkeletonAsset::metaObject, PropertySpec::ReadWrite),
    PROPERTY_OBJECT("anim", "Animation", "", "0", AnimAsset::metaObject, PropertySpec::ReadWrite),
END_PROPERTIES

void ComSkinnedMeshRenderer::RegisterProperties() {
#ifdef NEW_PROPERTY_SYSTEM
    REGISTER_ENUM_ACCESSOR_PROPERTY("Animation Type", "Animation Controller;Single Animation", GetAnimationType, SetAnimationType, 0, "", PropertySpec::ReadWrite);
    REGISTER_MIXED_ACCESSOR_PROPERTY("Anim Controller", ObjectRef, GetAnimControllerRef, SetAnimControllerRef, ObjectRef(AnimControllerAsset::metaObject, GuidMapper::defaultAnimControllerGuid), "", PropertySpec::ReadWrite);
    REGISTER_MIXED_ACCESSOR_PROPERTY("Skeleton", ObjectRef, GetSkeletonRef, SetSkeletonRef, ObjectRef(SkeletonAsset::metaObject, Guid::zero), "", PropertySpec::ReadWrite);
    REGISTER_MIXED_ACCESSOR_PROPERTY("Animation", ObjectRef, GetAnimRef, SetAnimRef, ObjectRef(AnimAsset::metaObject, Guid::zero), "", PropertySpec::ReadWrite);
#endif
}

ComSkinnedMeshRenderer::ComSkinnedMeshRenderer() {
    animControllerAsset = nullptr;

    skeletonAsset = nullptr;
    skeleton = nullptr;

    animAsset = nullptr;
    anim = nullptr;

    jointMats = nullptr;

#ifndef NEW_PROPERTY_SYSTEM
    Connect(&Properties::SIG_PropertyChanged, this, (SignalCallback)&ComSkinnedMeshRenderer::PropertyChanged);
#endif
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

    animator.ClearAnimController();

    if (chainPurge) {
        ComMeshRenderer::Purge();
    }
}

void ComSkinnedMeshRenderer::Init() {
    ComMeshRenderer::Init();

    animationType = (AnimationType)props->Get("animationType").As<int>();

    ChangeAnimationType();

    if (animationType == AnimationControllerType) {
        ChangeAnimController();
        
        animator.ComputeAnimAABBs(referenceMesh);

        const BE1::Skeleton *skeleton = animator.GetAnimController()->GetSkeleton();
        bool isCompatibleSkeleton = referenceMesh->IsCompatibleSkeleton(skeleton) ? true : false;
        
        // Set SceneEntity parameters
        sceneEntity.mesh = referenceMesh->InstantiateMesh(isCompatibleSkeleton ? Mesh::SkinnedMesh : Mesh::StaticMesh);
        sceneEntity.skeleton = isCompatibleSkeleton ? skeleton : nullptr;
        sceneEntity.numJoints = isCompatibleSkeleton ? skeleton->NumJoints() : 0;
    } else {
        ChangeSkeleton();
        
        ChangeAnim();
        
        bool isCompatibleSkeleton = referenceMesh->IsCompatibleSkeleton(skeleton) ? true : false;
        
        sceneEntity.mesh = referenceMesh->InstantiateMesh(isCompatibleSkeleton ? Mesh::SkinnedMesh : Mesh::StaticMesh);
        sceneEntity.skeleton = isCompatibleSkeleton ? skeleton : nullptr;
        sceneEntity.numJoints = isCompatibleSkeleton ? skeleton->NumJoints() : 0;
        
        playStartTime = GetGameWorld()->GetTime();
    }

    // Mark as initialized
    SetInitialized(true);

    UpdateVisuals();
}

void ComSkinnedMeshRenderer::ResetAnimState() {
    animator.ResetState(GetGameWorld()->GetTime());
}

const char *ComSkinnedMeshRenderer::GetCurrentAnimState(int layerNum) const {
    const AnimState *animState = animator.CurrentAnimState(layerNum);
    if (animState) {
        return animState->GetName().c_str();
    }
    return "";
}

void ComSkinnedMeshRenderer::TransitAnimState(int layerNum, const char *stateName, int blendOffset, int blendDuration, bool isAtomic) {
    animator.TransitState(layerNum, stateName, GetGameWorld()->GetTime(), blendOffset, blendDuration, isAtomic);
}

void ComSkinnedMeshRenderer::ChangeAnimationType() {
    animationType = props->Get("animationType").As<AnimationType>();

    if (animationType == AnimationControllerType) {
        props->SetFlags("animController", props->GetFlags("animController") & ~Property::Hidden);
        props->SetFlags("skeleton", props->GetFlags("skeleton") | Property::Hidden);
        props->SetFlags("anim", props->GetFlags("anim") | Property::Hidden);
    } else {
        props->SetFlags("animController", props->GetFlags("animController") | Property::Hidden);
        props->SetFlags("skeleton", props->GetFlags("skeleton") & ~Property::Hidden);
        props->SetFlags("anim", props->GetFlags("anim") & ~Property::Hidden);
    }
}

void ComSkinnedMeshRenderer::ChangeAnimController() {
    // Disconnect with previously connected animation controller asset
    if (animControllerAsset) {
        animControllerAsset->Disconnect(&Asset::SIG_Reloaded, this);
        animControllerAsset = nullptr;
    }

    // Set new animation controller
    const Guid animControllerGuid = props->Get("animController").As<Guid>();
    const Str animControllerPath = resourceGuidMapper.Get(animControllerGuid);
    animator.SetAnimController(animControllerPath);
    
    // Reset animator state
    animator.ResetState(GetGameWorld()->GetTime());

    // Need to connect animation controller asset to be reloaded in Editor
    animControllerAsset = (AnimControllerAsset *)AnimControllerAsset::FindInstance(animControllerGuid);
    if (animControllerAsset) {
        animControllerAsset->Connect(&Asset::SIG_Reloaded, this, (SignalCallback)&ComSkinnedMeshRenderer::AnimControllerReloaded, SignalObject::Queued);
    }
}

void ComSkinnedMeshRenderer::ChangeSkeleton() {
    // Disconnect with previously connected skeleton asset
    if (skeletonAsset) {
        skeletonAsset->Disconnect(&Asset::SIG_Reloaded, this);
        skeletonAsset = nullptr;
    }

    if (skeleton) {
        skeletonManager.ReleaseSkeleton(skeleton);
        skeleton = nullptr;
    }

    const Guid skeletonGuid = props->Get("skeleton").As<Guid>();
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

void ComSkinnedMeshRenderer::ChangeAnim() {
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

    const Guid animGuid = props->Get("anim").As<Guid>();
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

    playStartTime = GetGameWorld()->GetTime();
   
    // Need to connect anim asset to be reloaded in Editor
    animAsset = (AnimAsset *)AnimAsset::FindInstance(animGuid);
    if (animAsset) {
        animAsset->Connect(&Asset::SIG_Reloaded, this, (SignalCallback)&ComSkinnedMeshRenderer::AnimReloaded, SignalObject::Queued);
    }
}

void ComSkinnedMeshRenderer::Update() {
    if (animationType == AnimationControllerType) {
        if (!animator.GetAnimController()->GetSkeleton()) {
            return;
        }

        /*if (!animator.FrameHasChanged(GetGameWorld()->GetTime())) {
            return;
        }

        animator.ClearForceUpdate();*/        

        animator.UpdateFrame(GetEntity(), GetGameWorld()->GetPrevTime(), GetGameWorld()->GetTime());
    }

    UpdateVisuals();
}

void ComSkinnedMeshRenderer::UpdateVisuals() {
    if (!IsInitialized() || !IsEnabled()) {
        return;
    }

    int currentTime = GetGameWorld()->GetTime();

    UpdateAnimation(currentTime);
}

void ComSkinnedMeshRenderer::UpdateAnimation(int currentTime) {
    if (animationType == AnimationControllerType) {
        animator.ComputeFrame(currentTime);

        BE1::Mat3x4 *jointMats = animator.GetFrame();

        // Modify jointMats for IK here !

        sceneEntity.joints = jointMats;

        // Get AABB from animator
        animator.ComputeAABB(currentTime);
        animator.GetAABB(sceneEntity.aabb);
    } else {
        if (anim) {
            int time = currentTime - playStartTime;
            BE1::Anim::FrameInterpolation frameInterpolation;
            anim->TimeToFrameInterpolation(time, frameInterpolation);

            JointPose *jointFrame = (JointPose *)_alloca16(skeleton->NumJoints() * sizeof(jointFrame[0]));
            anim->GetInterpolatedFrame(frameInterpolation, jointIndexes.Count(), jointIndexes.Ptr(), jointFrame);

            simdProcessor->ConvertJointPosesToJointMats(jointMats, jointFrame, skeleton->NumJoints());

            simdProcessor->TransformJoints(jointMats, jointParents.Ptr(), 1, skeleton->NumJoints() - 1);

            anim->GetAABB(sceneEntity.aabb, frameAABBs, time);
        } else {
            sceneEntity.aabb = referenceMesh->GetAABB();
        }
    }

    ComRenderable::UpdateVisuals();
}

void ComSkinnedMeshRenderer::MeshUpdated() {
    if (animationType == AnimationControllerType) {
        const BE1::Skeleton *skeleton = animator.GetAnimController()->GetSkeleton();
        bool isCompatibleSkeleton = referenceMesh->IsCompatibleSkeleton(skeleton) ? true : false;

        if (isCompatibleSkeleton) {
            animator.ComputeAnimAABBs(referenceMesh);

            sceneEntity.mesh = referenceMesh->InstantiateMesh(Mesh::SkinnedMesh);
            sceneEntity.skeleton = skeleton;
            sceneEntity.numJoints = skeleton->NumJoints();
        } else {
            sceneEntity.mesh = referenceMesh->InstantiateMesh(Mesh::StaticMesh);
            sceneEntity.skeleton = nullptr;
            sceneEntity.numJoints = 0;
        }
    } else {
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
    }

    // temp code
    renderWorld->RemoveEntity(sceneEntityHandle);
    sceneEntityHandle = -1;
    // temp code
    UpdateVisuals();
}

void ComSkinnedMeshRenderer::AnimControllerReloaded() {
    ChangeAnimController();
   
    MeshUpdated();
}

void ComSkinnedMeshRenderer::SkeletonReloaded() {
    ChangeSkeleton();

    MeshUpdated();
}

void ComSkinnedMeshRenderer::AnimReloaded() {
    ChangeAnim();

    MeshUpdated();
}

void ComSkinnedMeshRenderer::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitialized()) {
        return;
    }

    if (!Str::Cmp(propName, "animationType")) {
        ChangeAnimationType();
        Purge();
        Init();
        return;
    }

    if (!Str::Cmp(propName, "skeleton")) {
        SkeletonReloaded();
        return;
    }

    if (!Str::Cmp(propName, "anim")) {
        AnimReloaded();
        return;
    }

    if (!Str::Cmp(propName, "animController")) {
        AnimControllerReloaded();
        return;
    }

    ComMeshRenderer::PropertyChanged(classname, propName);
}

BE_NAMESPACE_END
