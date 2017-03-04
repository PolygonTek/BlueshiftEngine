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
#include "AnimController/AnimLayer.h"
#include "AnimController/AnimController.h"
#include "Animator/Animator.h"

BE_NAMESPACE_BEGIN

AnimState::AnimState(AnimLayer *animLayer) {
    this->animLayer = animLayer;
    this->nodeNum = INVALID_ANIM_NODE;
    this->position = Vec2::zero;
}

AnimState::AnimState(AnimLayer *animLayer, const AnimState *animState) {
    this->animLayer = animLayer;
    this->name = animState->name;
    this->nodeNum = animState->nodeNum;
    this->position = animState->position;
}

bool AnimState::IsAnimClip() const {
    return IS_ANIM_LEAF(nodeNum);
}

bool AnimState::IsAnimBlendTree() const {
    return IS_ANIM_NODE(nodeNum);
}

AnimClip *AnimState::GetAnimClip() const {
    if (!IS_ANIM_LEAF(nodeNum)) {
        return nullptr;
    }

    return animLayer->GetNodeAnimClip(nodeNum);
}

AnimBlendTree *AnimState::GetAnimBlendTree() const {
    if (!IS_ANIM_NODE(nodeNum)) {
        return nullptr;
    }

    return animLayer->GetNodeAnimBlendTree(nodeNum);
}

int32_t AnimState::SetAnimClip(AnimClip *animClip) {
    if (nodeNum != INVALID_ANIM_NODE) {
        animLayer->RemoveNode(nodeNum);
        nodeNum = INVALID_ANIM_NODE;
    }

    if (animClip) {
        nodeNum = animLayer->CreateLeaf(Vec3::zero, animClip);
    }

    return nodeNum;
}

int32_t AnimState::SetBlendTree(AnimBlendTree *animBlendTree) {
    if (nodeNum != INVALID_ANIM_NODE) {
        animLayer->RemoveNode(nodeNum);
        nodeNum = INVALID_ANIM_NODE;
    }

    if (animBlendTree) {
        nodeNum = animLayer->CreateNode(Vec3::zero, animBlendTree);
    }

    return nodeNum;
}

void AnimState::AddTimeEvent(float time, const char *string) {
    int index = 0;
    for (; index < events.Count(); index++) {
        if (time < events[index].time) {
            break;
        }
    }

    AnimTimeEvent event;
    event.time = time;
    event.string = string;

    events.Insert(event, index);
}

int AnimState::GetDuration(const Animator *animator) const {
    if (IS_ANIM_NODE(nodeNum)) {
        const AnimBlendTree *blendTree = animLayer->GetNodeAnimBlendTree(nodeNum);
        if (blendTree) {
            return blendTree->GetDuration(animator);
        }
    }

    const AnimClip *animClip = animLayer->GetNodeAnimClip(nodeNum);
    return animClip->Length();
}

void AnimState::GetFrame(const Animator *animator, float normalizedTime, int numJoints, JointPose *outJointPose) const {
    const Array<int> &maskJoints = animLayer->GetMaskJoints();

    if (IS_ANIM_NODE(nodeNum)) {
        const AnimBlendTree *blendTree = animLayer->GetNodeAnimBlendTree(nodeNum);
        if (blendTree) {
            blendTree->GetFrame(animator, normalizedTime, maskJoints.Count(), maskJoints.Ptr(), numJoints, outJointPose);
        }
    } else {
        Anim::FrameInterpolation frameInterpolation;
        const AnimClip *animClip = animLayer->GetNodeAnimClip(nodeNum);
        if (animClip) {
            animClip->TimeToFrameInterpolation(normalizedTime * animClip->Length(), frameInterpolation);
            animClip->GetInterpolatedFrame(frameInterpolation, maskJoints.Count(), maskJoints.Ptr(), outJointPose);
        }
    }
}

void AnimState::GetTranslation(const Animator *animator, float normalizedTime, Vec3 &outTranslation) const {
    if (IS_ANIM_NODE(nodeNum)) {
        const AnimBlendTree *blendTree = animLayer->GetNodeAnimBlendTree(nodeNum);
        if (blendTree) {
            blendTree->GetTranslation(animator, normalizedTime, outTranslation);
        }
    } else {
        const AnimClip *animClip = animLayer->GetNodeAnimClip(nodeNum);
        if (animClip) {
            animClip->GetTranslation(normalizedTime * animClip->Length(), outTranslation);
        }
    }
}

void AnimState::GetRotation(const Animator *animator, float normalizedTime, Quat &outRotation) const {
    if (IS_ANIM_NODE(nodeNum)) {
        const AnimBlendTree *blendTree = animLayer->GetNodeAnimBlendTree(nodeNum);
        if (blendTree) {
            blendTree->GetRotation(animator, normalizedTime, outRotation);
        }
    } else {
        const AnimClip *animClip = animLayer->GetNodeAnimClip(nodeNum);
        if (animClip) {
            animClip->GetRotation(normalizedTime * animClip->Length(), outRotation);
        }
    }
}

void AnimState::GetAABB(const Animator *animator, float normalizedTime, AABB &outAabb) const {
    if (IS_ANIM_NODE(nodeNum)) {
        const AnimBlendTree *blendTree = animLayer->GetNodeAnimBlendTree(nodeNum);
        if (blendTree) {
            blendTree->GetAABB(animator, normalizedTime, outAabb);
        }
    } else {
        const AnimClip *animClip = animLayer->GetNodeAnimClip(nodeNum);
        if (animClip) {
            //int index = animator->GetAnimController()->FindAnimClipIndex(animClip);
            //animClip->GetAABB(normalizedTime * animClip->Length(), animator->GetAnimAABB(index)->frameAABBs, outAabb);
            animator->GetMeshAABB(outAabb);
        }
    }
}

BE_NAMESPACE_END
