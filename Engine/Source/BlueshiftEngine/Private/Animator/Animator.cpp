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
#include "Core/Heap.h"
#include "Render/Skeleton.h"
#include "Render/Mesh.h"
#include "AnimController/AnimController.h"
#include "Animator/Animator.h"
#include "Simd/Simd.h"
#include "Core/JointPose.h"
#include "Game/Entity.h"

BE_NAMESPACE_BEGIN

Animator::Animator() {
    animController = nullptr;
    numJoints = 0;
    jointMats = nullptr;
    ignoreRootTranslation = false;

    for (int i = 0; i < MaxLayers; i++) {
        for (int j = 0; j < MaxBlendersPerLayer; j++) {
            layerAnimStateBlenders[i][j].animator = this;
        }
    }
}

Animator::~Animator() {
    FreeData();
}

void Animator::FreeData() {
    if (animController) {
        animControllerManager.ReleaseAnimController(animController);
    }

    for (int i = 0; i < MaxLayers; i++) {
        for (int j = 0; j < MaxBlendersPerLayer; j++) {
            layerAnimStateBlenders[i][j].Clear();
        }
    }

    if (jointMats) {
        Mem_AlignedFree(jointMats);
        jointMats = nullptr;
    }

    numJoints = 0;
    animController = nullptr;
}

size_t Animator::Allocated() const {
    return numJoints * sizeof(jointMats[0]);
}

size_t Animator::Size() const {
    return sizeof(*this) + Allocated();
}

int Animator::GetParameterIndex(const char *parmName) const {
    if (!animController) {
        return -1;
    }
    return animController->FindParameterIndex(parmName);
}

const float Animator::GetParameterValue(int parmIndex) const {
    if (parmIndex < 0 || parmIndex >= parameters.Count()) {
        return 0;
    }
    return parameters[parmIndex];
}

void Animator::SetParameterValue(int parmIndex, const float value) {
    if (parmIndex < 0 || parmIndex >= parameters.Count()) {
        return;
    }
    parameters[parmIndex] = value;
}

bool Animator::SetParameterValue(const char *parmName, const float value) {
    int parmIndex = GetParameterIndex(parmName);
    if (parmIndex < 0) {
        return false;
    }
    SetParameterValue(parmIndex, value);
    return true;
}

void Animator::UpdateFrame(Entity *entity, int previousTime, int currentTime) {
    for (int layerIndex = 0; layerIndex < MaxLayers; layerIndex++) {
        const AnimLayer *animLayer = animController->GetAnimLayerByIndex(layerIndex);
        if (!animLayer) {
            break;
        }        

        AnimStateBlender *stateBlenders = layerAnimStateBlenders[layerIndex];

        for (int blenderIndex = 0; blenderIndex < MaxBlendersPerLayer; blenderIndex++) {
            AnimStateBlender *stateBlender = &stateBlenders[blenderIndex];
            if (stateBlender->animState && stateBlender->GetBlendWeight(currentTime) > 0) {
                // Sets blended duration for each blending states
                const int duration = stateBlenders[blenderIndex].animState->GetDuration(this);
                stateBlenders[blenderIndex].SetDuration(currentTime, duration);

                // Calls events for each blending states
                stateBlenders[blenderIndex].CallEvents(entity, previousTime, currentTime);
            }
        }

        float animTime = stateBlenders[0].NormalizedTime(currentTime);
        //BE_LOG(L"%hs: %.3f\n", stateBlenders[0].animState->GetName().c_str(), animTime);

        // Gets the transitions which have src state currently blended in
        Array<const AnimLayer::AnimTransition *> transitionArray;
        animLayer->ListTransitionsFrom(stateBlenders[0].animState->GetName(), transitionArray);

        for (int transitionIndex = 0; transitionIndex < transitionArray.Count(); transitionIndex++) {
            const AnimLayer::AnimTransition *transition = transitionArray[transitionIndex];

            // Atomic transition cannot be interrupted by other transition until it finish
            if (stateBlenders[0].IsAtomic()) {
                if (stateBlenders[0].IsInBlending(currentTime)) {
                    continue;
                }
            }

            // Check all of the conditions to transit to
            int conditionIndex = 0;
            for (; conditionIndex < transition->conditions.Count(); conditionIndex++) {
                const AnimLayer::Condition &condition = transition->conditions[conditionIndex];

                bool result = false;
                float value = GetParameterValue(condition.parameterIndex);
                switch (condition.compareFunc) {
                case AnimLayer::Condition::CompareFunc::GreaterThan:    result = value >  condition.value; break;
                case AnimLayer::Condition::CompareFunc::GreaterEqual:   result = value >= condition.value; break;
                case AnimLayer::Condition::CompareFunc::LessThan:       result = value <  condition.value; break;
                case AnimLayer::Condition::CompareFunc::LessEqual:      result = value <= condition.value; break;
                case AnimLayer::Condition::CompareFunc::Equal:          result = value == condition.value; break;
                }

                if (!result) {
                    break;
                }
            }

            // Skip transition unless all of the conditions are satisfied
            if (conditionIndex < transition->conditions.Count()) {
                continue;
            }

            // Defer transition if it has exit time
            if (transition->hasExitTime) {
                if (stateBlenders[0].exitTime == 0) {
                    stateBlenders[0].exitTime = Math::Ceil(animTime - transition->exitTime) + transition->exitTime;
                }

                if (animTime < stateBlenders[0].exitTime) {
                    continue;
                }
            }

            int blendDuration;
            if (transition->fixedDuration) {
                blendDuration = SEC2MS(transition->duration);
            } else {
                blendDuration = transition->duration * stateBlenders[0].animState->GetDuration(this);
            }

            TransitState(layerIndex, transition->dstState->GetName(), currentTime, transition->startTime, blendDuration, transition->isAtomic);
            break;
        }
    }
}

void Animator::ClearAnimController() {
    FreeData();
}

void Animator::SetAnimController(const Str &name) {
    ClearAnimController();

    animController = animControllerManager.GetAnimController(name);
    if (!animController) {
        return;
    }

    const Skeleton *skeleton = animController->GetSkeleton();
    if (!skeleton) {
        animController = nullptr;
        return;
    }

    // Initialize jointMats from bindposes
    animController->BuildBindPoseMats(&numJoints, &jointMats);

    // Clear all animation state blenders for each layers 
    for (int i = 0; i < MaxLayers; i++) {
        for (int j = 0; j < MaxBlendersPerLayer; j++) {
            layerAnimStateBlenders[i][j].Clear();
        }
    }

    // Set all the parameter values to 0
    parameters.SetCount(animController->NumParameters());
    for (int i = 0; i < parameters.Count(); i++) {
        parameters[i] = 0;
    }
}

const char *Animator::GetJointName(int jointIndex) const {
    if (!animController || !animController->GetSkeleton()) {
        return "";
    }
    
    return animController->GetSkeleton()->GetJointName(jointIndex);
}

int Animator::GetJointIndex(const char *name) const {
    if (!animController || !animController->GetSkeleton()) {
        return -1;
    }

    return animController->GetSkeleton()->GetJointIndex(name);
}

int Animator::NumAnimLayers() const { 
    if (!animController) {
        return 0;
    }

    return animController->NumAnimLayers(); 
}

const AnimLayer *Animator::GetAnimLayer(int index) const {
    if (!animController) {
        return nullptr;
    }

    return animController->GetAnimLayerByIndex(index);
}

void Animator::ComputeAnimAABBs(const Mesh *mesh) {
    // TODO: 
#if 0
    animAABBs.SetGranularity(1);
    animAABBs.SetCount(animController->NumAnimClips());

    if (animController->NumAnimClips() > 0) {
        for (int animClipIndex = 0; animClipIndex < animController->NumAnimClips(); animClipIndex++) {
            const AnimClip *animClip = animController->GetAnimClip(animClipIndex);

            animClip->GetAnim()->ComputeFrameAABBs(animController->GetSkeleton(), mesh, animAABBs[animClipIndex].frameAABBs);
        }
    }
#endif
    // bindpose AABB
    frameAABB = mesh->GetAABB();

    meshAABB = mesh->GetAABB();
}

void Animator::ResetState(int currentTime) {
    for (int i = 0; i < MaxLayers; i++) {
        const AnimLayer *animLayer = animController->GetAnimLayerByIndex(i);
        if (!animLayer) {
            break;
        }

        // Clear all animation state blenders for each layers 
        for (int j = 0; j < MaxBlendersPerLayer; j++) {
            layerAnimStateBlenders[i][j].Clear();
        }

        // Set to the default animation state for each layers
        const AnimState *state = animLayer->GetDefaultState();
        if (state) {
            layerAnimStateBlenders[i][0].BlendIn(state, currentTime, 0, 0, false);
        }
    }
}

const AnimState *Animator::CurrentAnimState(int layerNum) const {
    if (layerNum < 0 || layerNum >= MaxLayers) {
        BE_WARNLOG(L"Animator::CurrentAnim: layerNum out of range\n");
        return nullptr;
    }

    // 0'th element of the animation state array represents current animation state
    return layerAnimStateBlenders[layerNum][0].animState;
}

void Animator::TransitState(int layerNum, const char *stateName, int currentTime, float startOffset, int blendDuration, bool isAtomic) {
    if (layerNum < 0 || layerNum >= MaxLayers) {
        BE_WARNLOG(L"Animator::TransitState: layerNum out of range\n");
        return;
    }

    const AnimLayer *animLayer = animController->GetAnimLayerByIndex(layerNum);
    if (!animLayer) {
        BE_WARNLOG(L"Animator::TransitState: invalid layerNum %i\n", layerNum);
        return;
    }

    const AnimState *state = animLayer->FindState(stateName);
    if (!state) {
        BE_WARNLOG(L"Animator::TransitState: invalid state name '%hs'\n", stateName);
        return;
    }

    AnimStateBlender *stateBlenders = layerAnimStateBlenders[layerNum];

    // If current state animation is still blending in
    if (currentTime > stateBlenders[0].startTime && stateBlenders[0].GetBlendWeight(currentTime) > 0.0f) {
        PushStateBlenders(layerNum, currentTime, blendDuration);
    }

    stateBlenders[0].BlendIn(state, currentTime, startOffset, blendDuration, isAtomic);
    stateBlenders[0].SetDuration(currentTime, stateBlenders[0].animState->GetDuration(this));
    stateBlenders[0].exitTime = 0;
}

void Animator::PushStateBlenders(int layerNum, int currentTime, int blendDuration) {
    AnimStateBlender *stateBlenders = layerAnimStateBlenders[layerNum];

    // 새로운 animation state 를 0 번째 element 에 넣기 위해 이전 state blenders 들을 뒤쪽 element 로 이동시킨다.
    for (int i = MaxBlendersPerLayer - 1; i > 0; i--) {
        stateBlenders[i] = stateBlenders[i - 1];
    }

    // ready to blend in new animation state
    stateBlenders[0].Clear();
    // blend out for previous animation state
    stateBlenders[1].BlendOut(currentTime, blendDuration);
}

void Animator::ComputeFrame(int currentTime) {
    const JointPose *bindPoses = animController->GetBindPoses();
    if (!bindPoses) {
        BE_WARNLOG(L"Animator::ComputeFrame: no bindPoses on '%hs'\n", animController->GetHashName());
        return;
    }

    // Temporary buffer for the joint poses of base layer
    JointPose *jointFrame1 = (JointPose *)_alloca16(numJoints * sizeof(jointFrame1[0]));
    // Copy bindposes for all joints
    // Masked joints will be calculated against a layer so unmasked joints still have bindposes
    simdProcessor->Memcpy(jointFrame1, bindPoses, numJoints * sizeof(jointFrame1[0]));

    bool hasAnim = false;

    // Blending animation state only for base layer 
    float blendedWeight = 0.0f;
    AnimStateBlender *stateBlender = layerAnimStateBlenders[0];
    for (int i = 0; i < MaxBlendersPerLayer; i++, stateBlender++) {
        if (stateBlender->animState) {
            if (stateBlender->BlendFrame(currentTime, numJoints, jointFrame1, blendedWeight)) {
                hasAnim = true;
                if (blendedWeight >= 1.0f) {
                    break;
                }
            }
        }
    }

    // Temporary buffer for the joint poses of the other layers
    JointPose *jointFrame2 = (JointPose *)_alloca16(numJoints * sizeof(jointFrame2[0]));

    // Blending animation state for other layers
    for (int i = 1; i < MaxLayers; i++) {
        const AnimLayer *animLayer = animController->GetAnimLayerByIndex(i);
        if (!animLayer) {
            break;
        }

        blendedWeight = 0.0f;        
        stateBlender = layerAnimStateBlenders[i];
        for (int j = 0; j < MaxBlendersPerLayer; j++, stateBlender++) {
            if (stateBlender->animState) {
                if (stateBlender->BlendFrame(currentTime, numJoints, jointFrame2, blendedWeight)) {
                    hasAnim = true;
                    if (blendedWeight >= 1.0f) {
                        break;
                    }
                }
            }
        }

        // layer 의 blended weight 가 있다면 layer 끼리 블렌딩한다
        if (blendedWeight > 0) {
            // other layers have the mask joints
            const Array<int> &maskJoints = animLayer->GetMaskJoints();
            float layerBlendWeight = blendedWeight * animLayer->GetWeight(); // NOTE: anim layer weight -- is it really necessary ?

            if (animLayer->GetBlending() == AnimLayer::Blending::Override) {
                simdProcessor->BlendJoints(jointFrame1, jointFrame2, layerBlendWeight, maskJoints.Ptr(), maskJoints.Count());
            } else if (animLayer->GetBlending() == AnimLayer::Blending::Additive) {
                simdProcessor->AdditiveBlendJoints(jointFrame1, jointFrame2, layerBlendWeight, maskJoints.Ptr(), maskJoints.Count());
            } else {
                BE_WARNLOG(L"Animator::ComputeFrame: invalid layer blending method\n");
            }
        }
    }

    if (!hasAnim) {
        return;
    }

    // Convert the joint quaternions to rotation matrices
    simdProcessor->ConvertJointPosesToJointMats(jointMats, jointFrame1, numJoints);

    // Add in the animController offset
    jointMats[0].SetTranslation(jointMats[0].ToTranslationVec3() + animController->GetRootOffset());

    // Transform the rest of the hierarchy
    simdProcessor->TransformJoints(jointMats, animController->GetJointParents(), 1, numJoints - 1);
}

void Animator::GetTranslation(int currentTime, Vec3 &translation) const {
    if (!animController || !animController->GetSkeleton()) {
        translation.SetFromScalar(0);
        return;
    }

    translation.SetFromScalar(0);
    float blendedWeight = 0.0f;
    const AnimStateBlender *stateBlender = layerAnimStateBlenders[0];
    for (int i = 0; i < MaxBlendersPerLayer; i++, stateBlender++) {
        stateBlender->BlendTranslation(currentTime, translation, blendedWeight);
    }

    Vec3 translation2;

    for (int i = 1; i < MaxLayers; i++) {
        const AnimLayer *animLayer = animController->GetAnimLayerByIndex(i);
        if (!animLayer) {
            break;
        }

        if (animLayer->GetMaskJoints()[0] == 0) {
            blendedWeight = 0.0f;
            stateBlender = layerAnimStateBlenders[i];
            for (int j = 0; j < MaxBlendersPerLayer; j++, stateBlender++) {
                stateBlender->BlendTranslation(currentTime, translation2, blendedWeight);
            }
        }

        float layerBlendWeight = blendedWeight * animLayer->GetWeight();
        translation += layerBlendWeight * (translation2 - translation);
    }

    translation += animController->GetRootOffset();
}

void Animator::GetTranslationDelta(int fromTime, int toTime, Vec3 &delta) const {
    if (!animController || !animController->GetSkeleton() || (fromTime == toTime)) {
        delta.SetFromScalar(0);
        return;
    }

    delta.SetFromScalar(0);
    float blendedWeight = 0.0f;

    const AnimStateBlender *stateBlender = layerAnimStateBlenders[0];
    for (int i = 0; i < MaxBlendersPerLayer; i++, stateBlender++) {
        stateBlender->BlendTranslationDelta(fromTime, toTime, delta, blendedWeight);
    }

    Vec3 delta2;

    for (int i = 1; i < MaxLayers; i++) {
        const AnimLayer *animLayer = animController->GetAnimLayerByIndex(i);
        if (!animLayer) {
            break;
        }

        if (animLayer->GetMaskJoints()[0] == 0) {
            delta2.SetFromScalar(0);
            blendedWeight = 0.0f;
            stateBlender = layerAnimStateBlenders[i];
            for (int j = 0; j < MaxBlendersPerLayer; j++, stateBlender++) {
                stateBlender->BlendTranslationDelta(fromTime, toTime, delta2, blendedWeight);
            }
        }

        float layerBlendWeight = blendedWeight * animLayer->GetWeight();
        delta += layerBlendWeight * (delta2 - delta);
    }
}

bool Animator::GetRotationDelta(int fromTime, int toTime, Mat3 &delta) const {
    if (!animController || !animController->GetSkeleton() || (fromTime == toTime)) {
        delta.SetIdentity();
        return false;
    }

    Quat q1, q2;
    q1.SetIdentity();
    float blendedWeight = 0.0f;

    const AnimStateBlender *stateBlender = layerAnimStateBlenders[0];
    for (int i = 0; i < MaxBlendersPerLayer; i++, stateBlender++) {
        stateBlender->BlendRotationDelta(fromTime, toTime, q1, blendedWeight);
    }

    for (int i = 1; i < MaxLayers; i++) {
        const AnimLayer *animLayer = animController->GetAnimLayerByIndex(i);
        if (!animLayer) {
            break;
        }

        if (animLayer->GetMaskJoints()[0] == 0) {
            q2.SetIdentity();
            blendedWeight = 0.0f;
            stateBlender = layerAnimStateBlenders[i];
            for (int j = 0; j < MaxBlendersPerLayer; j++, stateBlender++) {
                stateBlender->BlendRotationDelta(fromTime, toTime, q2, blendedWeight);
            }
        }

        float layerBlendWeight = blendedWeight * animLayer->GetWeight();
        q1.SetFromSlerp(q1, q2, layerBlendWeight);
    }

    if (blendedWeight > 0.0f) {
        delta = q1.ToMat3();
        return true;
    } else {
        delta.SetIdentity();
        return false;
    }
}

void Animator::ComputeAABB(int currentTime) {
    if (!animController || !animController->GetSkeleton()) {
        return;
    }

    AABB aabb;
    aabb.Clear();
    int count = 0;
    const AnimStateBlender *stateBlender = layerAnimStateBlenders[0];
    for (int i = 0; i < MaxLayers; i++) {
        const AnimLayer *animLayer = animController->GetAnimLayerByIndex(i);
        if (!animLayer) {
            break;
        }

        for (int j = 0; j < MaxBlendersPerLayer; j++, stateBlender++) {
            if (stateBlender->AddAABB(currentTime, aabb, ignoreRootTranslation)) {
                count++;
            }
        }
    }

    if (count) {
        aabb.TranslateSelf(animController->GetRootOffset());

        frameAABB = aabb;
    }
}

void Animator::GetAABB(AABB &aabb) const {
    aabb = frameAABB;
}

BE_NAMESPACE_END
