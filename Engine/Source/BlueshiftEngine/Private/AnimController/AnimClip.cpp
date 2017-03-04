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
#include "AnimController/AnimController.h"
#include "Animator/Animator.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

AnimClip::AnimClip() {
    this->anim = nullptr;
}

AnimClip::AnimClip(const AnimClip *other) {
    SetAnim(other->anim);
}

AnimClip::~AnimClip() {
    Purge();
}

void AnimClip::Purge() {
    if (anim) {
        animManager.ReleaseAnim(anim);
        anim = nullptr;
    }
}

void AnimClip::SetAnim(const Anim *anim) {
    Purge();

    this->anim = const_cast<Anim *>(anim);//const_cast<Anim *>(anim->AddRefCount());

    // TODO: 구간 (startTime, endTime) 을 정해서 구할 것
    if (this->anim->Length() > 0) {
        this->averageVelocity = anim->TotalMovementDelta() / MS2SEC(anim->Length());
    } else {
        this->averageVelocity.SetFromScalar(0);
    }
}

void AnimClip::TimeToFrameInterpolation(int time, Anim::FrameInterpolation &outFrameInterpolation) const {
    anim->TimeToFrameInterpolation(time, outFrameInterpolation);
}

void AnimClip::GetSingleFrame(int frameNum, int numJointIndexes, const int *jointIndexes, JointPose *joints) const {
    anim->GetSingleFrame(frameNum, numJointIndexes, jointIndexes, joints);
}

void AnimClip::GetInterpolatedFrame(Anim::FrameInterpolation &frameInterpolation, int numJointIndexes, const int *jointIndexes, JointPose *joints) const {
    anim->GetInterpolatedFrame(frameInterpolation, numJointIndexes, jointIndexes, joints);
}

void AnimClip::GetTranslation(int time, Vec3 &outTranslation) const {
    anim->GetTranslation(outTranslation, time);
}

void AnimClip::GetRotation(int time, Quat &outRotation) const {
    anim->GetRotation(outRotation, time);
}

void AnimClip::GetAABB(int time, const Array<AABB> &frameAABBs, AABB &outAabb) const {
    anim->GetAABB(outAabb, frameAABBs, time);
}

bool AnimClip::Load(const char *filename) {
    File *fp = fileSystem.OpenFile(filename, File::ReadMode);
    if (!fp) {
        BE_WARNLOG(L"AnimClip::Load: file open error\n");
        return false;
    }

    fileSystem.CloseFile(fp);

    return true;
}

bool AnimClip::Reload() {
    Str filename = anim->GetHashName();
    filename += ".clip";
    return Load(filename);
}

void AnimClip::Write(const char *filename) {
    File *fp = fileSystem.OpenFile(filename, File::WriteMode);
    if (!fp) {
        BE_WARNLOG(L"AnimClip::Write: file open error\n");
        return;
    }

    fileSystem.CloseFile(fp);
}

BE_NAMESPACE_END
