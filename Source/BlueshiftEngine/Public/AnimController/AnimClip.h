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

    Anim Clip

    실제 AnimController 에서 사용되는, logical animation 단위
    
    TODO: startTime, endTime 으로 Anim 에서 잘라서 사용할 수 있게 만들자

---------------------------------------------------------------------------------
*/

#include "Render/Anim.h"

BE_NAMESPACE_BEGIN

class AnimController;

class AnimClip {
    friend class AnimController;

public:
    AnimClip();
    AnimClip(const AnimClip *animClip);
    ~AnimClip();

    void                Purge();

    const Anim *        GetAnim() const { return anim; }
    void                SetAnim(const Anim *anim);

    int                 Length() const { return anim->Length(); }

    int                 MaxCycleCount() const { return anim->MaxCycleCount(); }

    const Vec3 &        TotalMovementDelta() const { return anim->TotalMovementDelta(); }

    const Vec3 &        GetAverageVelocity() const { return averageVelocity; }

                        /// Converts animation time to frame interpolation
    void                TimeToFrameInterpolation(int time, Anim::FrameInterpolation &outFrameBlend) const;

                        /// Gets the single frame joints 
    void                GetSingleFrame(int frameNum, int numJointIndexes, const int *jointIndexes, JointPose *joints) const;

                        /// Gets the frame interpolated joints
    void                GetInterpolatedFrame(Anim::FrameInterpolation &frame, int numJointIndexes, const int *jointIndexes, JointPose *joints) const;

                        /// Gets the translation with the given animation time
    void                GetTranslation(int time, Vec3 &outTranslation) const;

                        /// Gets the rotation with the given animation time
    void                GetRotation(int time, Quat &outRotation) const;

                        /// Gets the AABB from the AABB list with the given animation time
    void                GetAABB(int time, const Array<AABB> &frameAABBs, AABB &outAABB) const;

    bool                Load(const char *filename);
    bool                Reload();

    void                Write(const char *filename);

    const AnimClip *    AddRefCount() const { refCount++; return this; }
    
private:
    Anim *              anim;
    mutable int         refCount;
    //int               startTime;
    //int               endTime;
    Vec3                averageVelocity;
};

BE_NAMESPACE_END
