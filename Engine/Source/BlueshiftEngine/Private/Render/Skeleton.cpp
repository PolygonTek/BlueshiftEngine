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
#include "RenderInternal.h"
#include "BModel.h"
#include "Core/JointPose.h"
#include "Core/Heap.h"
#include "Simd/Simd.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

bool Skeleton::IsDefaultSkeleton() const {
    return (this == SkeletonManager::defaultSkeleton ? true : false);
}

void Skeleton::Purge() {
    SAFE_DELETE_ARRAY(joints);

    if (bindPoses) {
        Mem_AlignedFree(bindPoses);
        bindPoses = nullptr;
    }

    if (invBindPoseMats) {
        Mem_AlignedFree(invBindPoseMats);	
        invBindPoseMats = nullptr;
    }
}

bool Skeleton::CheckHierarchy(const Skeleton *otherSkeleton) const {
    if (numJoints != otherSkeleton->NumJoints()) {
        return false;
    }
    
    const Joint *otherJoints = otherSkeleton->GetJoints();

    for (int i = 0; i < numJoints; i++) {
        const Joint *joint = &joints[i];
        const Joint *otherJoint = &otherJoints[i];
        
        if (joint->name != otherJoint->name) {
            return false;
        }
        
        int	parentIndex;
        if (joint->parent)  {
            parentIndex = (int)(joint->parent - joints);
        } else {
            parentIndex = -1;
        }
        
        int otherParentIndex;
        if (otherJoint->parent)  {
            otherParentIndex = (int)(otherJoint->parent - otherJoints);
        } else {
            otherParentIndex = -1;
        }
        
        if (parentIndex != otherParentIndex) {
            return false;
        }
    }
    
    return true;
}

void Skeleton::CreateDefaultSkeleton() {
    Purge();

    numJoints = 1;

    joints = new Joint[1];
    joints[0].name = "root";
    joints[0].parent = nullptr;

    bindPoses = (JointPose *)Mem_Alloc16(sizeof(JointPose));
    bindPoses[0].q.SetIdentity();
    bindPoses[0].t.SetFromScalar(0);
    bindPoses[0].s.Set(1.0f, 1.0f, 1.0f);
    
    invBindPoseMats = (Mat3x4 *)Mem_Alloc16(sizeof(Mat3x4));
    invBindPoseMats[0].SetIdentity();
}

bool Skeleton::Load(const char *filename) {
    Purge();

    Str bSkelFilename = filename;
    if (!Str::CheckExtension(filename, ".bskel")) {
        bSkelFilename.SetFileExtension(".bskel");
    }

    BE_LOG(L"Loading skeleton '%hs'...\n", filename);

    byte *data;
    fileSystem.LoadFile(filename, true, (void **)&data);
    if (!data) {
        return false;
    }

    const BSkelHeader *bSkelHeader = (const BSkelHeader *)data;
    byte *ptr = data + sizeof(BSkelHeader);

    if (bSkelHeader->ident != BSKEL_IDENT) {
        BE_WARNLOG(L"Skeleton::Load: bad format %hs\n", filename);
        fileSystem.FreeFile(data);
        return false;
    }

    numJoints = bSkelHeader->numJoints;
    if (numJoints > 0) {
        joints = new Joint[numJoints];
        bindPoses = (JointPose *)Mem_Alloc16(sizeof(JointPose) * numJoints);
        invBindPoseMats = (Mat3x4 *)Mem_Alloc16(sizeof(Mat3x4) * numJoints);

        // --- joints ---
        for (int jointIndex = 0; jointIndex < numJoints; jointIndex++) {
            const BJoint *bJoint = (const BJoint *)ptr;

            joints[jointIndex].name = bJoint->name;
            joints[jointIndex].parent = bJoint->parentIndex >= 0 ? &this->joints[bJoint->parentIndex] : nullptr;

            ptr += sizeof(BJoint);
        }

        // --- bindpose ---
        for (int jointIndex = 0; jointIndex < numJoints; jointIndex++) {
            simdProcessor->Memcpy(&bindPoses[jointIndex].q, ptr, sizeof(bindPoses[jointIndex].q));
            ptr += sizeof(bindPoses[jointIndex].q);

            simdProcessor->Memcpy(&bindPoses[jointIndex].t, ptr, sizeof(bindPoses[jointIndex].t));
            ptr += sizeof(bindPoses[jointIndex].t);

            simdProcessor->Memcpy(&bindPoses[jointIndex].s, ptr, sizeof(bindPoses[jointIndex].s));
            ptr += sizeof(bindPoses[jointIndex].s);
        }

        // --- inverted bindpose ---
        int invBindPoseMatsSize = numJoints * sizeof(invBindPoseMats[0]);
        simdProcessor->Memcpy(invBindPoseMats, ptr, invBindPoseMatsSize);
        ptr += invBindPoseMatsSize;
    }

    fileSystem.FreeFile(data);

    return true;
}

void Skeleton::Write(const char *filename) {
    File *fp = fileSystem.OpenFile(filename, File::WriteMode);
    if (!fp) {
        BE_WARNLOG(L"Skeleton::Write: file open error\n");
        return;
    }

    BSkelHeader bSkelHeader;
    bSkelHeader.ident = BSKEL_IDENT;
    bSkelHeader.version = BSKEL_VERSION;
    bSkelHeader.numJoints = NumJoints();
    bSkelHeader.padding = 0;
    fp->Write(&bSkelHeader, sizeof(bSkelHeader));

    // --- joints ---
    for (int jointIndex = 0; jointIndex < bSkelHeader.numJoints; jointIndex++) {
        const Joint *joint = &joints[jointIndex];

        BJoint bJoint;
        Str::Copynz(bJoint.name, joint->name, sizeof(bJoint.name));
        bJoint.parentIndex = !joint->parent ? -1 : (int)(joint->parent - joints);
        fp->Write(&bJoint, sizeof(bJoint));
    }

    // --- bindpose ---
    for (int jointIndex = 0; jointIndex < numJoints; jointIndex++) {
        fp->Write(&bindPoses[jointIndex].q, sizeof(bindPoses[jointIndex].q));
        fp->Write(&bindPoses[jointIndex].t, sizeof(bindPoses[jointIndex].t));
        fp->Write(&bindPoses[jointIndex].s, sizeof(bindPoses[jointIndex].s));
    }

    // --- inverted bindpose ---
    fp->Write(invBindPoseMats, numJoints * sizeof(invBindPoseMats[0]));

    fileSystem.CloseFile(fp);
}

bool Skeleton::Reload() {
    Str _hashName = hashName;
    bool ret = Load(_hashName);    
    return ret;
}

BE_NAMESPACE_END
