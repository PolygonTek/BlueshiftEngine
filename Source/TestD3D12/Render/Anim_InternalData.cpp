// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Render/Anim.h"
#include "Render/AnimManager.h"
#include "BModel.h"

Anim::InternalData &Anim::InternalData::operator=(InternalData &&rhs) noexcept {
    if (this != &rhs) {
        Purge();
        Swap(rhs);
    }
    return *this;
}

void Anim::InternalData::Purge() {
    numFrames = 0;
    numJoints = 0;
    numComponentsPerFrame = 0;
    length = 0;
    maxCycleCount = 1;

    hasRootRotation = false;
    hasRootTranslationXY = false;
    hasRootTranslationZ = false;
    isDefaultAnim = false;
    isAdditiveAnim = false;

    totalDelta.SetFromScalar(0);

    joints.Clear();
    jointNames.Clear();
    baseFrame.Clear();
    components.Clear();
    frameTimes.Clear();
}

void Anim::InternalData::Swap(InternalData &rhs) {
    BE1::Swap(hasRootRotation, rhs.hasRootRotation);
    BE1::Swap(hasRootTranslationXY, rhs.hasRootTranslationXY);
    BE1::Swap(hasRootTranslationZ, rhs.hasRootTranslationZ);
    BE1::Swap(isDefaultAnim, rhs.isDefaultAnim);
    BE1::Swap(isAdditiveAnim, rhs.isAdditiveAnim);
    BE1::Swap(numJoints, rhs.numJoints);
    BE1::Swap(numFrames, rhs.numFrames);
    BE1::Swap(numComponentsPerFrame, rhs.numComponentsPerFrame);
    BE1::Swap(length, rhs.length);
    BE1::Swap(maxCycleCount, rhs.maxCycleCount);
    joints.Swap(rhs.joints);
    jointNames.Swap(rhs.jointNames);
    baseFrame.Swap(rhs.baseFrame);
    components.Swap(rhs.components);
    frameTimes.Swap(rhs.frameTimes);
    BE1::Swap(totalDelta, rhs.totalDelta);
}

size_t Anim::InternalData::Allocated() const {
    size_t size = joints.Allocated() + jointNames.Size() + baseFrame.Allocated() + components.Allocated() + frameTimes.Allocated();
    return size;
}

void Anim::InternalData::CopyFrom(const InternalData &rhs) {
    hasRootRotation = rhs.hasRootRotation;
    hasRootTranslationXY = rhs.hasRootTranslationXY;
    hasRootTranslationZ = rhs.hasRootTranslationZ;
    isDefaultAnim = rhs.isDefaultAnim;
    isAdditiveAnim = rhs.isAdditiveAnim;
    numJoints = rhs.numJoints;
    numFrames = rhs.numFrames;
    numComponentsPerFrame = rhs.numComponentsPerFrame;
    length = rhs.length;
    maxCycleCount = rhs.maxCycleCount;
    joints = rhs.joints;
    jointNames = rhs.jointNames;
    baseFrame = rhs.baseFrame;
    components = rhs.components;
    frameTimes = rhs.frameTimes;
    totalDelta = rhs.totalDelta;
}

void Anim::InternalData::ResolveJointNames() {
    if (jointNames.Count() != numJoints) {
        return;
    }

    for (int jointIndex = 0; jointIndex < numJoints; jointIndex++) {
        joints[jointIndex].nameIndex = animManager.JointIndexByName(jointNames[jointIndex]);
    }

    jointNames.Clear();
}

bool Anim::InternalData::LoadBinaryAnim(const char *filename) {
    BE1::Str bAnimFilename = filename;
    if (!BE1::Str::CheckExtension(filename, ".banim")) {
        bAnimFilename.SetFileExtension(".banim");
    }

    BE_LOG("Loading anim '%s'...\n", bAnimFilename.c_str());

    byte *data;
    BE1::fileSystem.LoadFile(bAnimFilename, true, (void **)&data);
    if (!data) {
        return false;
    }
     
    const BAnimHeader *bAnimHeader = (const BAnimHeader *)data;
    byte *ptr = data + sizeof(BAnimHeader);
    
    if (bAnimHeader->ident != BANIM_IDENT) {
        BE_WARNLOG("Anim::InternalData::LoadBinaryAnim: bad format %s\n", bAnimFilename.c_str());
        BE1::fileSystem.FreeFile(data);
        return false;
    }

    numFrames = bAnimHeader->numFrames;
    numJoints = bAnimHeader->numJoints;
    numComponentsPerFrame = bAnimHeader->numComponentsPerFrame;
    length = bAnimHeader->length;
    maxCycleCount = bAnimHeader->maxCycleCount;
    hasRootRotation = (bAnimHeader->flags & BAnimFlag::RootRotation) ? true : false;
    hasRootTranslationXY = (bAnimHeader->flags & BAnimFlag::RootTranslationXY) ? true : false;
    hasRootTranslationZ = (bAnimHeader->flags & BAnimFlag::RootTranslationZ) ? true : false;

    // --- frame times ---
    int frameTimesCount = *(const int *)ptr;
    ptr += sizeof(frameTimesCount);

    frameTimes.SetGranularity(1);
    frameTimes.SetCount(frameTimesCount);
    BE1::simdProcessor->Memcpy(frameTimes.Ptr(), ptr, (int)frameTimes.MemoryUsed());
    ptr += frameTimes.MemoryUsed();

    if (bAnimHeader->version == 1) {
        int timeToFramesCount = *(const int *)ptr;
        ptr += sizeof(timeToFramesCount);
        ptr += sizeof(int) * timeToFramesCount;
    }

    // --- joint info ---
    joints.SetGranularity(1);
    joints.SetCount(numJoints);
    jointNames.SetGranularity(1);
    jointNames.SetCount(numJoints);

    for (int jointIndex = 0; jointIndex < numJoints; jointIndex++) {
        const BAnimJoint *bAnimJoint = (const BAnimJoint *)ptr;
        ptr += sizeof(BAnimJoint);

        JointInfo *jointInfo = &this->joints[jointIndex];

        jointNames[jointIndex] = bAnimJoint->name;
        jointInfo->nameIndex = -1;
        jointInfo->parentIndex = bAnimJoint->parentIndex;
        jointInfo->componentBits = bAnimJoint->componentBits;
        jointInfo->componentOffset = bAnimJoint->componentOffset;
    }

    // --- base frame ---
    baseFrame.SetGranularity(1);
    baseFrame.SetCount(numJoints);

     for (int jointIndex = 0; jointIndex < numJoints; jointIndex++) {
        memcpy(&baseFrame[jointIndex].q, ptr, sizeof(baseFrame[jointIndex].q));
        ptr += sizeof(baseFrame[jointIndex].q);

        memcpy(&baseFrame[jointIndex].t, ptr, sizeof(baseFrame[jointIndex].t));
        ptr += sizeof(baseFrame[jointIndex].t);

        memcpy(&baseFrame[jointIndex].s, ptr, sizeof(baseFrame[jointIndex].s));
        ptr += sizeof(baseFrame[jointIndex].s);
    }

    // --- frames ---
    components.SetGranularity(1);
    components.SetCount(numComponentsPerFrame * numFrames);
    memcpy(components.Ptr(), ptr, components.MemoryUsed());
    ptr += components.MemoryUsed();

    // --- total delta ---
    memcpy(&totalDelta, ptr, sizeof(totalDelta));
    ptr += sizeof(totalDelta);

    BE1::fileSystem.FreeFile(data);

    return true;
}

void Anim::InternalData::WriteBinaryAnim(const char *filename) const {
    BE1::File *fp = BE1::fileSystem.OpenFile(filename, BE1::File::Mode::Write);
    if (!fp) {
        BE_WARNLOG("Anim::InternalData::WriteBinaryAnim: file open error\n");
        return;
    }

    int flags = 0;
    flags |= hasRootTranslationXY ? BAnimFlag::RootTranslationXY : 0;
    flags |= hasRootTranslationZ ? BAnimFlag::RootTranslationZ : 0;
    flags |= hasRootRotation ? BAnimFlag::RootRotation : 0;

    BAnimHeader bAnimHeader;
    bAnimHeader.ident = BANIM_IDENT;
    bAnimHeader.version = BANIM_VERSION;
    bAnimHeader.flags = flags;
    bAnimHeader.numJoints = numJoints;
    bAnimHeader.numFrames = numFrames;
    bAnimHeader.numComponentsPerFrame = numComponentsPerFrame;
    bAnimHeader.length = length;
    bAnimHeader.maxCycleCount = maxCycleCount;
    fp->Write(&bAnimHeader, sizeof(bAnimHeader));
    
    // --- frame times ---
    int frameTimesCount = frameTimes.Count();
    fp->Write(&frameTimesCount, sizeof(frameTimesCount));
    fp->Write(frameTimes.Ptr(), frameTimes.MemoryUsed());

    // --- joint info ---
    for (int jointIndex = 0; jointIndex < numJoints; jointIndex++) {
        const JointInfo *jointInfo = &this->joints[jointIndex];

        BAnimJoint bAnimJoint;
        BE1::Str::Copynz(bAnimJoint.name, animManager.JointNameByIndex(jointInfo->nameIndex), sizeof(bAnimJoint.name));
        bAnimJoint.parentIndex = jointInfo->parentIndex;
        bAnimJoint.componentBits = jointInfo->componentBits;
        bAnimJoint.componentOffset = jointInfo->componentOffset;
        fp->Write(&bAnimJoint, sizeof(bAnimJoint));
    }

    // --- base frames ---
    for (int jointIndex = 0; jointIndex < numJoints; jointIndex++) {
        fp->Write(&baseFrame[jointIndex].q, sizeof(baseFrame[jointIndex].q));
        fp->Write(&baseFrame[jointIndex].t, sizeof(baseFrame[jointIndex].t));
        fp->Write(&baseFrame[jointIndex].s, sizeof(baseFrame[jointIndex].s));
    }

    // --- frames ---
    fp->Write(components.Ptr(), components.MemoryUsed());
    
    // --- total delta ---
    fp->Write(&totalDelta, sizeof(totalDelta));

    BE1::fileSystem.CloseFile(fp);
}
