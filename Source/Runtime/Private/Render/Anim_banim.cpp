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
#include "Core/JointPose.h"
#include "Render/Render.h"
#include "BModel.h"
#include "File/FileSystem.h"
#include "Simd/Simd.h"

BE_NAMESPACE_BEGIN

bool Anim::LoadBinaryAnim(const char *filename) {
    byte *data;
    fileSystem.LoadFile(filename, true, (void **)&data);
    if (!data) {
        return false;
    }
     
    const BAnimHeader *bAnimHeader = (const BAnimHeader *)data;
    byte *ptr = data + sizeof(BAnimHeader);
    
    if (bAnimHeader->ident != BANIM_IDENT) {
        BE_WARNLOG("Anim::LoadBinaryAnim: bad format %s\n", filename);
        fileSystem.FreeFile(data);
        return false;
    }

    numFrames = bAnimHeader->numFrames;
    numJoints = bAnimHeader->numJoints;
    numComponentsPerFrame = bAnimHeader->numComponentsPerFrame;
    length = bAnimHeader->length;
    maxCycleCount = bAnimHeader->maxCycleCount;
    rootRotation = (bAnimHeader->flags & BAnimFlag::RootRotation) ? true : false;
    rootTranslationXY = (bAnimHeader->flags & BAnimFlag::RootTranslationXY) ? true : false;
    rootTranslationZ = (bAnimHeader->flags & BAnimFlag::RootTranslationZ) ? true : false;

    // --- frame times ---
    int frameTimesCount = *(const int *)ptr;
    ptr += sizeof(frameTimesCount);

    frameTimes.SetGranularity(1);
    frameTimes.SetCount(frameTimesCount);
    simdProcessor->Memcpy(frameTimes.Ptr(), ptr, (int)frameTimes.MemoryUsed());
    ptr += frameTimes.MemoryUsed();

    if (bAnimHeader->version == 1) {
        int timeToFramesCount = *(const int *)ptr;
        ptr += sizeof(timeToFramesCount);
        ptr += sizeof(int) * timeToFramesCount;
    }

    // --- joint info ---
    joints.SetGranularity(1);
    joints.SetCount(numJoints);

    for (int jointIndex = 0; jointIndex < numJoints; jointIndex++) {
        const BAnimJoint *bAnimJoint = (const BAnimJoint *)ptr;
        ptr += sizeof(BAnimJoint);

        JointInfo *jointInfo = &this->joints[jointIndex];

        jointInfo->nameIndex = animManager.JointIndexByName(bAnimJoint->name);
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

    fileSystem.FreeFile(data);

    return true;
}

void Anim::WriteBinaryAnim(const char *filename) {
    File *fp = fileSystem.OpenFile(filename, File::Mode::Write);
    if (!fp) {
        BE_WARNLOG("Anim::WriteBinaryAnim: file open error\n");
        return;
    }

    int flags = 0;
    flags |= rootTranslationXY ? BAnimFlag::RootTranslationXY : 0;
    flags |= rootTranslationZ ? BAnimFlag::RootTranslationZ : 0;
    flags |= rootRotation ? BAnimFlag::RootRotation : 0;

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
        Str::Copynz(bAnimJoint.name, animManager.JointNameByIndex(jointInfo->nameIndex), sizeof(bAnimJoint.name));
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

    fileSystem.CloseFile(fp);
}

BE_NAMESPACE_END
