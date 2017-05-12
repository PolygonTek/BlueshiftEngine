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
        BE_WARNLOG(L"Anim::LoadBinaryAnim: bad format %hs\n", filename);
        fileSystem.FreeFile(data);
        return false;
    }

    numFrames = bAnimHeader->numFrames;
    numJoints = bAnimHeader->numJoints;
    numAnimatedComponents = bAnimHeader->numAnimatedComponents;
    animLength = bAnimHeader->animLength;
    maxCycleCount = bAnimHeader->maxCycleCount;
    rootRotation = (bAnimHeader->flags & BAnimFlag::RootRotation) ? true : false;
    rootTranslationXY = (bAnimHeader->flags & BAnimFlag::RootTranslationXY) ? true : false;
    rootTranslationZ = (bAnimHeader->flags & BAnimFlag::RootTranslationZ) ? true : false;

    // --- frameToTimeMap & timeToFrameMap ---
    int frameToTimeMapCount = *(const int *)ptr;
    ptr += sizeof(frameToTimeMapCount);

    frameToTimeMap.SetGranularity(1);
    frameToTimeMap.SetCount(frameToTimeMapCount);
    simdProcessor->Memcpy(frameToTimeMap.Ptr(), ptr, (int)frameToTimeMap.MemoryUsed());
    ptr += frameToTimeMap.MemoryUsed();

    int timeToFrameMapCount = *(const int *)ptr;
    ptr += sizeof(timeToFrameMapCount);

    timeToFrameMap.SetGranularity(1);
    timeToFrameMap.SetCount(timeToFrameMapCount);
    simdProcessor->Memcpy(timeToFrameMap.Ptr(), ptr, (int)timeToFrameMap.MemoryUsed());
    ptr += timeToFrameMap.MemoryUsed();

    // --- joint info ---
    jointInfo.SetGranularity(1);
    jointInfo.SetCount(numJoints);

    for (int jointIndex = 0; jointIndex < numJoints; jointIndex++) {
        const BAnimJoint *bAnimJoint = (const BAnimJoint *)ptr;
        ptr += sizeof(BAnimJoint);

        JointInfo *jointInfo = &this->jointInfo[jointIndex];

        jointInfo->nameIndex = animManager.JointIndexByName(bAnimJoint->name);
        jointInfo->parentNum = bAnimJoint->parentIndex;
        jointInfo->animBits = bAnimJoint->animBits;
        jointInfo->firstComponent = bAnimJoint->firstComponent;
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
    frameComponents.SetGranularity(1);
    frameComponents.SetCount(numAnimatedComponents * numFrames);
    memcpy(frameComponents.Ptr(), ptr, frameComponents.MemoryUsed());
    ptr += frameComponents.MemoryUsed();

    // --- total delta ---
    memcpy(&totalDelta, ptr, sizeof(totalDelta));
    ptr += sizeof(totalDelta);

    fileSystem.FreeFile(data);

    return true;
}

void Anim::WriteBinaryAnim(const char *filename) {
    File *fp = fileSystem.OpenFile(filename, File::WriteMode);
    if (!fp) {
        BE_WARNLOG(L"Anim::WriteBinaryAnim: file open error\n");
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
    bAnimHeader.numAnimatedComponents = numAnimatedComponents;
    bAnimHeader.animLength = animLength;
    bAnimHeader.maxCycleCount = maxCycleCount;
    fp->Write(&bAnimHeader, sizeof(bAnimHeader));
    
    // --- frameToTimeMap & timeToFrameMap ---
    int frameToTimeMapCount = frameToTimeMap.Count();
    fp->Write(&frameToTimeMapCount, sizeof(frameToTimeMapCount)); 
    fp->Write(frameToTimeMap.Ptr(), frameToTimeMap.MemoryUsed());

    int timeToFrameMapCount = timeToFrameMap.Count();
    fp->Write(&timeToFrameMapCount, sizeof(timeToFrameMapCount));
    fp->Write(timeToFrameMap.Ptr(), timeToFrameMap.MemoryUsed());

    // --- joint info ---
    for (int jointIndex = 0; jointIndex < numJoints; jointIndex++) {
        const JointInfo *jointInfo = &this->jointInfo[jointIndex];

        BAnimJoint bAnimJoint;
        Str::Copynz(bAnimJoint.name, animManager.JointNameByIndex(jointInfo->nameIndex), sizeof(bAnimJoint.name));
        bAnimJoint.parentIndex = jointInfo->parentNum;
        bAnimJoint.animBits = jointInfo->animBits;
        bAnimJoint.firstComponent = jointInfo->firstComponent;
        fp->Write(&bAnimJoint, sizeof(bAnimJoint));
    }

    // --- base frames ---
    for (int jointIndex = 0; jointIndex < numJoints; jointIndex++) {
        fp->Write(&baseFrame[jointIndex].q, sizeof(baseFrame[jointIndex].q));
        fp->Write(&baseFrame[jointIndex].t, sizeof(baseFrame[jointIndex].t));
        fp->Write(&baseFrame[jointIndex].s, sizeof(baseFrame[jointIndex].s));
    }

    // --- frames ---
    fp->Write(frameComponents.Ptr(), frameComponents.MemoryUsed());
    
    // --- total delta ---
    fp->Write(&totalDelta, sizeof(totalDelta));

    fileSystem.CloseFile(fp);    
}

BE_NAMESPACE_END
