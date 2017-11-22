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
#include "AnimController/AnimState.h"
#include "Animator/Animator.h"
#include "Core/JointPose.h"
#include "Simd/Simd.h"
#include "Game/Entity.h"
#include "Components/ComScript.h"

BE_NAMESPACE_BEGIN

AnimStateBlender::AnimStateBlender() {
    Clear();
}

void AnimStateBlender::Clear() {
    animState           = nullptr;
    isAtomic            = false;

    startTime           = 0;
    timeOffset          = 0;
    invDuration         = 0;
    exitTime            = 0;

    blendStartTime      = 0;
    blendDuration       = 0;
    blendStartWeight    = 0.0f;
    blendEndWeight      = 0.0f;
}

AnimStateBlender &AnimStateBlender::operator=(const AnimStateBlender &other) {
    animState           = other.animState;
    isAtomic            = other.isAtomic;

    startTime           = other.startTime;
    timeOffset          = other.timeOffset;
    invDuration         = other.invDuration;
    exitTime            = other.exitTime;

    blendStartTime      = other.blendStartTime;
    blendDuration       = other.blendDuration;
    blendStartWeight    = other.blendStartWeight;
    blendEndWeight      = other.blendEndWeight;

    animator            = other.animator;

    return *this;
}

float AnimStateBlender::GetBlendWeight(int currentTime) const {
    float w;

    int timeDelta = currentTime - blendStartTime;
    if (timeDelta <= 0) {
        w = blendStartWeight;
    } else if (timeDelta >= blendDuration) {
        w = blendEndWeight;
    } else {
        float frac = Clamp((float)timeDelta / (float)blendDuration, 0.0f, 1.0f);
        w = Lerp(blendStartWeight, blendEndWeight, frac);
    }

    return w;
}

void AnimStateBlender::SetBlendWeight(int currentTime, float newValue, int blendDuration) {
    // set current blended weight to blend start weight
    this->blendStartWeight = GetBlendWeight(currentTime);
    this->blendEndWeight = newValue;
    this->blendStartTime = currentTime - 1;
    this->blendDuration = blendDuration;
}

void AnimStateBlender::SetDuration(int currentTime, int newDuration) {
    float newInvDuration = 1.0f / newDuration;

    if (invDuration == newInvDuration) {
        return;
    }

    // Actual elapsed time from start time
    int timeDelta = currentTime - startTime;
    // Get the normalized animation time
    float time = NormalizedTime(currentTime);
    
    timeOffset = time - timeDelta * newInvDuration;
    
    invDuration = newInvDuration;
}

bool AnimStateBlender::IsInBlending(int currentTime) const {
    if (!animState) {
        return false;
    }

    if (blendStartWeight == blendEndWeight) {
        return false;
    }

    if (currentTime > blendStartTime + blendDuration) {
        return false;
    }

    return true;
}

void AnimStateBlender::BlendIn(const AnimState *animState, int currentTime, float startOffset, int blendDuration, bool isAtomic) {
    Clear();

    this->animState         = animState;
    this->isAtomic          = isAtomic;
    this->startTime         = currentTime;
    this->timeOffset        = startOffset;
    this->blendStartTime    = currentTime - 1;
    this->blendDuration     = blendDuration;
    this->blendStartWeight  = 0.0f;
    this->blendEndWeight    = 1.0f;
 }

void AnimStateBlender::BlendOut(int currentTime, int blendDuration) {
    if (blendDuration == 0) {
        Clear();
    } else {
        // blendEndWeight 를 0 으로 설정하고, blendDuration 동안 서서히 value 를 줄여간다
        SetBlendWeight(currentTime, 0.0f, blendDuration);
    }
}

void AnimStateBlender::CallEvents(Entity *entity, int fromTime, int toTime) {
    ComponentPtrArray scriptComponents = entity->GetComponents(&ComScript::metaObject);
    if (scriptComponents.Count() == 0) {
        return;
    }

    float t1 = NormalizedTime(fromTime);
    float t2 = NormalizedTime(toTime);

    // FIXME: time wrap for only loop state
    t1 -= Math::Floor(t1);
    t2 -= Math::Floor(t2);

    for (int eventIndex = 0; eventIndex < animState->events.Count(); eventIndex++) {
        const auto *event = &animState->events[eventIndex];

        if (t1 <= t2) {
            if (event->time < t1 || event->time > t2) {
                continue;
            }
        } else {
            if (event->time < t1 && event->time > t2) {
                continue;
            }
        }

        for (int scriptIndex = 0; scriptIndex < scriptComponents.Count(); scriptIndex++) {
            ComScript *scriptComponent = scriptComponents[scriptIndex]->Cast<ComScript>();
            scriptComponent->CallFunc(event->string.c_str());
            //BE_LOG(L"%hs %f %i %i %f %f\n", event->string.c_str(), event->time, fromTime, toTime, t1, t2);
        }
    }
}

bool AnimStateBlender::BlendFrame(int currentTime, int numJoints, JointPose *blendedFrame, float &blendedWeight) const {
    if (!animState) {
        return false;
    }

    // Get the current weight of this animation state
    float currentWeight = GetBlendWeight(currentTime);
    if (currentWeight == 0.0f) {
        return false;
    }

    JointPose *jointFrame;
    if (blendedWeight == 0.0f) {
        // we don't need a temporary buffer, so just store it directly in the blendedFrame
        jointFrame = blendedFrame;
    } else {
        // allocate a temporary buffer to copy the joints from
        jointFrame = (JointPose *)_alloca16(numJoints * sizeof(jointFrame[0]));
    }

    float time = NormalizedTime(currentTime);

    animState->GetFrame(animator, time, numJoints, jointFrame);

    if (blendedWeight == 0.0f) {
        blendedWeight = currentWeight;
    } else {
        blendedWeight += currentWeight;
        float fraction = currentWeight / blendedWeight;

        const Array<int> &maskJoints = animState->animLayer->GetMaskJoints();
        simdProcessor->BlendJoints(blendedFrame, jointFrame, fraction, maskJoints.Ptr(), maskJoints.Count());
    }

    return true;
}

bool AnimStateBlender::BlendTranslation(int currentTime, Vec3 &blendedTranslation, float &blendedWeight) const {
    if (!animState) {
        return false;
    }

    float currentWeight = GetBlendWeight(currentTime);
    if (currentWeight == 0.0f) {
        return false;
    }

    float time = NormalizedTime(currentTime);

    Vec3 translation = Vec3::FromScalar(0);

    animState->GetTranslation(animator, time, translation);
    
    if (blendedWeight == 0.0f) {
        blendedWeight = currentWeight;
        blendedTranslation = translation;
    } else {
        blendedWeight += currentWeight;
        float fraction = currentWeight / blendedWeight;
        blendedTranslation = Lerp(blendedTranslation, translation, fraction);
    }

    return true;
}

bool AnimStateBlender::BlendTranslationDelta(int fromTime, int toTime, Vec3 &blendedDelta, float &blendedWeight) const {
    if (!animState) {
        return false;
    }

    float currentWeight = GetBlendWeight(toTime);
    if (currentWeight == 0.0f) {
        return false;
    }

    float time1 = NormalizedTime(fromTime);
    float time2 = NormalizedTime(toTime);

    Vec3 t1 = Vec3::zero;
    Vec3 t2 = Vec3::zero;

    animState->GetTranslation(animator, time1, t1);
    animState->GetTranslation(animator, time2, t2);
    
    Vec3 delta = t2 - t1;

    if (blendedWeight == 0.0f) {
        blendedWeight = currentWeight;
        blendedDelta = delta;
    } else {
        blendedWeight += currentWeight;
        float fraction = currentWeight / blendedWeight;
        blendedDelta = Lerp(blendedDelta, delta, fraction);
    }

    return true;
}

bool AnimStateBlender::BlendRotationDelta(int fromTime, int toTime, Quat &blendedDelta, float &blendedWeight) const {
    if (!animState) {
        return false;
    }

    float currentWeight = GetBlendWeight(toTime);
    if (currentWeight == 0.0f) {
        return false;
    }

    float time1 = NormalizedTime(fromTime);
    float time2 = NormalizedTime(toTime);

    Quat q1, q2;
    q1.SetIdentity();
    q2.SetIdentity();

    animState->GetRotation(animator, time1, q1);
    animState->GetRotation(animator, time2, q2);
    
    Quat q3 = q2 * q1.Inverse();

    if (blendedWeight == 0.0f) {
        blendedWeight = currentWeight;
        blendedDelta = q3;
    } else {
        blendedWeight += currentWeight;
        float fraction = currentWeight / blendedWeight;
        blendedDelta.SetFromSlerp(blendedDelta, q3, fraction);
    }

    return true;
}

bool AnimStateBlender::AddAABB(int currentTime, AABB &aabb, bool ignoreRootTranslation) const {
    if (!animState) {
        return false;
    }

    float currentWeight = GetBlendWeight(currentTime);
    if (currentWeight == 0.0f) {
        return false;
    }

    float time = NormalizedTime(currentTime);

    AABB a;
    a.Clear();

    animState->GetAABB(animator, time, a);

    aabb += a;

    return true;
}

BE_NAMESPACE_END
