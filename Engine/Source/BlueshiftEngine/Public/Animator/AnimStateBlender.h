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

#pragma once

/*
---------------------------------------------------------------------------------

    Anim State Blender

    context for playing animation with anim state

---------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class Vec3;
class Quat;
class AABB;
class Animator;
class AnimState;
class JointPose;
class Entity;

class AnimStateBlender {
    friend class Animator;

public:
    AnimStateBlender();

    AnimStateBlender &      operator=(const AnimStateBlender &other);

    void                    Clear();

    bool                    IsAtomic() const { return isAtomic; }

                            /// Convert the current time to the normalized playback time
    float                   NormalizedTime(int currentTime) const;

                            /// Gets blend weight on current time
    float                   GetBlendWeight(int currentTime) const;

                            /// Sets the duration from the current time
    void                    SetDuration(int currentTime, int duration);

                            /// Tests if it is in blending now
    bool                    IsInBlending(int currentTime) const;
                            /// Fades in new animation state 
    void                    BlendIn(const AnimState *animState, int currentTime, float startOffset, int blendDuration, bool isAtomic);
                            /// Fades out current animation state 
    void                    BlendOut(int currentTime, int blendDuration);

    void                    CallEvents(Entity *entity, int fromTime, int toTime);

                            // blendedFrame 에 current time 의 frame 을 blend 한다.
    bool                    BlendFrame(int currentTime, int numJoints, JointPose *blendedFrame, float &blendedWeight) const;
                            // blendedTranslation 에 current time 의 translation 을 blend 한다.
    bool                    BlendTranslation(int currentTime, Vec3 &blendedTranslation, float &blendedWeight) const;
                            // blendedTranslationDelta 에 current time 의 translation delta 를 blend 한다.
    bool                    BlendTranslationDelta(int fromTime, int toTime, Vec3 &blendedTranslationDelta, float &blendedWeight) const;
                            // blendedRotationDelta 에 current time 의 rotation delta 를 blend 한다.
    bool                    BlendRotationDelta(int fromTime, int toTime, Quat &blendedRotationDelta, float &blendedWeight) const;

    bool                    AddAABB(int currentTime, AABB &aabb, bool ignoreRootTranslation) const;

private:
                            // current time 의 value 를 기준으로 newValue 로 blendDuration 동안 블렌딩하는 것으로 설정
    void                    SetBlendWeight(int currentTime, float newWeight, int blendDuration);

    const AnimState *       animState;
    bool                    isAtomic;

    int                     startTime;          ///< Time to start blend in 
    float                   timeOffset;         ///< Time offset to compensate playback rate change in real time
    float                   invDuration;        ///< 
    float                   exitTime;

    int                     blendStartTime;     ///< Time to start blend out
    int                     blendDuration;      ///< Blend duration
    float                   blendStartWeight;   ///< Blend start value (0 ~ 1)
    float                   blendEndWeight;     ///< Blend end value (0 ~ 1)

    const Animator *        animator; 
};

BE_INLINE float AnimStateBlender::NormalizedTime(int currentTime) const {
    int timeDelta = currentTime - startTime;
    return timeDelta * invDuration + timeOffset;
}

BE_NAMESPACE_END
