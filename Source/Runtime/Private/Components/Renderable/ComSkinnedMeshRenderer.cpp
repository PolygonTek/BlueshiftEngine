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
#include "Components/Renderable/ComStaticMeshRenderer.h"
#include "Components/Renderable/ComSkinnedMeshRenderer.h"
#include "Components/Physics/ComSoftBody.h"
#include "Components/ComAnimation.h"
#include "Components/ComAnimator.h"
#include "AnimController/AnimController.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

const SignalDef ComSkinnedMeshRenderer::SIG_SkeletonUpdated("ComSkinnedMeshRenderer::SkeletonUpdated", "a");

OBJECT_DECLARATION("Skinned Mesh Renderer", ComSkinnedMeshRenderer, ComMeshRenderer)
BEGIN_EVENTS(ComSkinnedMeshRenderer)
END_EVENTS

void ComSkinnedMeshRenderer::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("root", "Root", Guid, GetRootGuid, SetRootGuid, Guid::zero,
        "Root Entity", PropertyInfo::Flag::Editor).SetMetaObject(&Entity::metaObject);
}

ComSkinnedMeshRenderer::ComSkinnedMeshRenderer() {
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
    if (chainPurge) {
        ComMeshRenderer::Purge();
    }
}

void ComSkinnedMeshRenderer::Init() {
    ComMeshRenderer::Init();

    // Mark as initialized
    SetInitialized(true);
}

void ComSkinnedMeshRenderer::LateInit() {
    UpdateSkeleton();

    UpdateVisuals();
}

void ComSkinnedMeshRenderer::InstantiateMesh() {
    const ComSoftBody *softBody = entity->GetComponent<ComSoftBody>();
    Mesh::Type::Enum meshType = softBody ? Mesh::Type::Dynamic : Mesh::Type::Skinned;

    renderObjectDef.mesh = referenceMesh->InstantiateMesh(meshType);
}

void ComSkinnedMeshRenderer::UpdateSkeleton() {
    const Skeleton *skeleton = nullptr;
    Mat3x4 *joints = nullptr;

    // Root object should have ComAnimation or ComAnimator component
    Object *rootObject = Entity::FindInstance(rootGuid);
    if (rootObject) {
        Entity *rootEntity = rootObject->Cast<Entity>();

        if (rootEntity->HasComponent(&ComAnimation::metaObject)) {
            ComAnimation *animationComponent = rootEntity->GetComponent<ComAnimation>();

            skeleton = animationComponent->GetSkeleton();
            joints = animationComponent->GetJointMatrices();
        } else if (rootEntity->HasComponent(&ComAnimator::metaObject)) {
            ComAnimator *animatorComponent = rootEntity->GetComponent<ComAnimator>();

            skeleton = animatorComponent->GetAnimator().GetAnimController()->GetSkeleton();
            joints = animatorComponent->GetJointMatrices();
        }
    }

    if (renderObjectDef.mesh) {
        meshManager.ReleaseMesh(renderObjectDef.mesh);
    }

    if (referenceMesh) {
        InstantiateMesh();

        bool isCompatibleSkeleton = skeleton && referenceMesh->IsCompatibleSkeleton(skeleton) ? true : false;
        if (isCompatibleSkeleton) {
            renderObjectDef.skeleton = skeleton;
            renderObjectDef.numJoints = skeleton->NumJoints();
            renderObjectDef.joints = joints;

            jointAabbs.SetCount(renderObjectDef.numJoints);
            renderObjectDef.mesh->GetJointAABBs(renderObjectDef.skeleton->GetInvBindPoseMatrices(), jointAabbs);
        } else {
            renderObjectDef.skeleton = nullptr;
            renderObjectDef.numJoints = 0;
            renderObjectDef.joints = nullptr;
        }
    }

    if (renderObjectDef.skeleton) {
        Entity *rootEntity = rootObject->Cast<Entity>();

        frameAabbs.Clear();

        if (rootEntity->HasComponent(&ComAnimation::metaObject)) {
            ComAnimation *animationComponent = rootEntity->GetComponent<ComAnimation>();
            const Anim *currentAnim = animationComponent->GetCurrentAnim();

            if (currentAnim) {
                currentAnim->ComputeFrameAABBs(skeleton, renderObjectDef.mesh, frameAabbs);
            }
        } else if (rootEntity->HasComponent(&ComAnimator::metaObject)) {
            ComAnimator *animatorComponent = rootEntity->GetComponent<ComAnimator>();
            Animator &animator = animatorComponent->GetAnimator();
            // FIXME
            //animator.ComputeAnimAABBs(renderObjectDef.mesh);
        }
    }

    UpdateVisuals();
}

void ComSkinnedMeshRenderer::UpdateVisuals() {
    if (!IsInitialized() || !IsActiveInHierarchy()) {
        return;
    }

    if (referenceMesh) {
        if (isAabbCalculatedByAnimation) {
            // AABB is calculated by current animation.
        } else if (jointAabbs.Count() > 0 && renderObjectDef.joints) {
            // AABB is calculated by joint movements.
            renderObjectDef.aabb.Clear();

            for (int i = 0; i < jointAabbs.Count(); i++) {
                if (!jointAabbs[i].IsCleared()) {
                    AABB aabb;
                    aabb.SetFromTransformedAABBFast(jointAabbs[i], renderObjectDef.joints[i]);

                    renderObjectDef.aabb += aabb;
                }
            }
        } else {
            // AABB is static.
            renderObjectDef.aabb = referenceMesh->GetAABB();
        }
    }

    ComRenderable::UpdateVisuals();
}

void ComSkinnedMeshRenderer::UpdateVisualsByAnimation(const ComAnimation *animationComponent, int time) {
    const Anim *currentAnim = animationComponent->GetCurrentAnim();
    if (currentAnim) {
        currentAnim->GetAABB(renderObjectDef.aabb, frameAabbs, time);

        isAabbCalculatedByAnimation = true;
    } else {
        isAabbCalculatedByAnimation = false;
    }

    UpdateVisuals();
}

void ComSkinnedMeshRenderer::UpdateVisualsByAnimator(const ComAnimator *animatorComponent, int time) {
    const Animator &animator = animatorComponent->GetAnimator();
    /*if (animator.GetAABB(time, renderObjectDef.aabb)) {
        isAabbCalculatedByAnimation = true;
    } else {
        isAabbCalculatedByAnimation = false;
    }*/

    UpdateVisuals();
}

Guid ComSkinnedMeshRenderer::GetRootGuid() const {
    return rootGuid;
}

void ComSkinnedMeshRenderer::SetRootGuid(const Guid &guid) {
    rootGuid = guid;

    Object *rootObject = Entity::FindInstance(rootGuid);
    if (rootObject) {
        Entity *rootEntity = rootObject->Cast<Entity>();

        if (rootEntity->HasComponent(&ComAnimation::metaObject)) {
            ComAnimation *animationComponent = rootEntity->GetComponent<ComAnimation>();
            animationComponent->Connect(&SIG_SkeletonUpdated, this, (SignalCallback)&ComSkinnedMeshRenderer::UpdateSkeleton, SignalObject::ConnectionType::Unique);
            animationComponent->Connect(&ComAnimation::SIG_AnimUpdated, this, (SignalCallback)&ComSkinnedMeshRenderer::UpdateVisualsByAnimation, SignalObject::ConnectionType::Unique);
        } else if (rootEntity->HasComponent(&ComAnimator::metaObject)) {
            ComAnimator *animatorComponent = rootEntity->GetComponent<ComAnimator>();
            animatorComponent->Connect(&SIG_SkeletonUpdated, this, (SignalCallback)&ComSkinnedMeshRenderer::UpdateSkeleton, SignalObject::ConnectionType::Unique);
            animatorComponent->Connect(&ComAnimator::SIG_AnimUpdated, this, (SignalCallback)&ComSkinnedMeshRenderer::UpdateVisualsByAnimator, SignalObject::ConnectionType::Unique);
        }
    }
}

void ComSkinnedMeshRenderer::MeshUpdated() {
    if (!IsInitialized()) {
        return;
    }

    if (renderObjectHandle != -1) {
        renderWorld->RemoveRenderObject(renderObjectHandle);
        renderObjectHandle = -1;
    }

    UpdateSkeleton();

    ComSoftBody *softBody = entity->GetComponent<ComSoftBody>();
    if (softBody) {
        softBody->ResetPoints();
    }

    UpdateVisuals();
}

BE_NAMESPACE_END
