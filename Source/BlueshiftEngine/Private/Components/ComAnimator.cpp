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
#include "AnimController/AnimController.h"
#include "Components/ComAnimator.h"
#include "Game/GameWorld.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Animator", ComAnimator, ComMeshRenderer)
BEGIN_EVENTS(ComAnimator)
END_EVENTS

void ComAnimator::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("animController", "Anim Controller", Guid, GetAnimControllerGuid, SetAnimControllerGuid, GuidMapper::defaultAnimControllerGuid, "", PropertyInfo::EditorFlag)
        .SetMetaObject(&AnimControllerAsset::metaObject);
}

ComAnimator::ComAnimator() {
    animControllerAsset = nullptr;
}

ComAnimator::~ComAnimator() {
    Purge(false);
}

bool ComAnimator::IsConflictComponent(const MetaObject &componentClass) const {
    if (componentClass.IsTypeOf(ComAnimator::metaObject)) {
        return true;
    }
    return false;
}

void ComAnimator::Purge(bool chainPurge) {
    animator.ClearAnimController();

    if (chainPurge) {
        ComMeshRenderer::Purge();
    }
}

void ComAnimator::Init() {
    ComMeshRenderer::Init();

    animator.ComputeAnimAABBs(referenceMesh);

    const Skeleton *skeleton = animator.GetAnimController()->GetSkeleton();
    bool isCompatibleSkeleton = referenceMesh->IsCompatibleSkeleton(skeleton) ? true : false;

    // Set SceneEntity parameters
    sceneEntity.mesh = referenceMesh->InstantiateMesh(isCompatibleSkeleton ? Mesh::SkinnedMesh : Mesh::StaticMesh);
    sceneEntity.skeleton = isCompatibleSkeleton ? skeleton : nullptr;
    sceneEntity.numJoints = isCompatibleSkeleton ? skeleton->NumJoints() : 0;

    // Mark as initialized
    SetInitialized(true);

    UpdateVisuals();
}

void ComAnimator::ResetAnimState() {
    animator.ResetState(GetGameWorld()->GetTime());
}

const char *ComAnimator::GetCurrentAnimState(int layerNum) const {
    const AnimState *animState = animator.CurrentAnimState(layerNum);
    if (animState) {
        return animState->GetName().c_str();
    }
    return "";
}

void ComAnimator::TransitAnimState(int layerNum, const char *stateName, int blendOffset, int blendDuration, bool isAtomic) {
    animator.TransitState(layerNum, stateName, GetGameWorld()->GetTime(), blendOffset, blendDuration, isAtomic);
}

void ComAnimator::Update() {
    if (!IsActiveInHierarchy()) { 
        return;
    }

    if (!animator.GetAnimController()->GetSkeleton()) {
        return;
    }

    /*if (!animator.FrameHasChanged(GetGameWorld()->GetTime())) {
        return;
    }

    animator.ClearForceUpdate();*/

    animator.UpdateFrame(GetEntity(), GetGameWorld()->GetPrevTime(), GetGameWorld()->GetTime());

    UpdateVisuals();
}

void ComAnimator::UpdateVisuals() {
    if (!IsInitialized() || !IsActiveInHierarchy()) {
        return;
    }

    int currentTime = GetGameWorld()->GetTime();

    UpdateAnimation(currentTime);
}

void ComAnimator::ChangeAnimController(const Guid &animControllerGuid) {
    // Disconnect with previously connected animation controller asset
    if (animControllerAsset) {
        animControllerAsset->Disconnect(&Asset::SIG_Reloaded, this);
        animControllerAsset = nullptr;
    }

    // Set new animation controller
    const Str animControllerPath = resourceGuidMapper.Get(animControllerGuid);
    animator.SetAnimController(animControllerPath);

    // Reset animator state
    animator.ResetState(GetGameWorld() ? GetGameWorld()->GetTime() : 0);

    // Need to connect animation controller asset to be reloaded in Editor
    animControllerAsset = (AnimControllerAsset *)AnimControllerAsset::FindInstance(animControllerGuid);
    if (animControllerAsset) {
        animControllerAsset->Connect(&Asset::SIG_Reloaded, this, (SignalCallback)&ComAnimator::AnimControllerReloaded, SignalObject::Queued);
    }
}

void ComAnimator::UpdateAnimation(int currentTime) {
    animator.ComputeFrame(currentTime);

    Mat3x4 *jointMats = animator.GetFrame();

    // Modify jointMats for IK here !

    sceneEntity.joints = jointMats;

    // Get AABB from animator
    animator.ComputeAABB(currentTime);
    animator.GetAABB(sceneEntity.aabb);

    ComRenderable::UpdateVisuals();
}

void ComAnimator::MeshUpdated() {
    if (!IsInitialized()) {
        return;
    }

    const Skeleton *skeleton = animator.GetAnimController()->GetSkeleton();
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

    // temp code
    renderWorld->RemoveEntity(sceneEntityHandle);
    sceneEntityHandle = -1;
    // temp code
    UpdateVisuals();
}

void ComAnimator::AnimControllerReloaded() {
    SetAnimControllerGuid(GetProperty("animController").As<Guid>());
}

Guid ComAnimator::GetAnimControllerGuid() const {
    if (animator.GetAnimController()) {
        const Str animControllerPath = animator.GetAnimController()->GetHashName();
        return resourceGuidMapper.Get(animControllerPath);
    }
    return Guid();
}

void ComAnimator::SetAnimControllerGuid(const Guid &guid) {
    ChangeAnimController(guid);

    MeshUpdated();
}

BE_NAMESPACE_END
