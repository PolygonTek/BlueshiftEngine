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
#include "Skeleton.h"
#include "SkeletonManager.h"
#include "Render/Mesh/BModel.h"
#include "LoadingSystem.h"

Skeleton::InternalData &Skeleton::InternalData::operator=(InternalData &&rhs) noexcept {
    if (this != &rhs) {
        Purge();
        Swap(rhs);
    }
    return *this;
}

void Skeleton::InternalData::Purge() {
    SAFE_DELETE_ARRAY(joints);
    numJoints = 0;

    if (jointParents) {
        Mem_AlignedFree(jointParents);
        jointParents = nullptr;
    }
    if (bindPoses) {
        Mem_AlignedFree(bindPoses);
        bindPoses = nullptr;
    }
    if (invBindPoseMats) {
        Mem_AlignedFree(invBindPoseMats);
        invBindPoseMats = nullptr;
    }
}

void Skeleton::InternalData::Swap(InternalData &rhs) {
    BE1::Swap(numJoints, rhs.numJoints);
    BE1::Swap(joints, rhs.joints);
    BE1::Swap(jointParents, rhs.jointParents);
    BE1::Swap(bindPoses, rhs.bindPoses);
    BE1::Swap(invBindPoseMats, rhs.invBindPoseMats);
}

bool Skeleton::IsDefaultSkeleton() const {
    return (this == skeletonManager.defaultSkeleton.get());
}

void Skeleton::Purge() {
    internalData.Purge();
}

void Skeleton::Reset() {
    BumpLoadSerial();
    state = State::Empty;

    Purge();
}

void Skeleton::CreateDefaultSkeleton() {
    Purge();

    internalData.numJoints = 1;

    internalData.joints = new Joint[1];
    internalData.joints[0].name = "root";
    internalData.joints[0].parent = nullptr;

    internalData.bindPoses = (BE1::JointPose *)Mem_Alloc16(sizeof(BE1::JointPose));
    internalData.bindPoses[0].q.SetIdentity();
    internalData.bindPoses[0].t.SetFromScalar(0);
    internalData.bindPoses[0].s.Set(1.0f, 1.0f, 1.0f);
    
    internalData.invBindPoseMats = (BE1::Mat3x4 *)Mem_Alloc16(sizeof(BE1::Mat3x4));
    internalData.invBindPoseMats[0].SetIdentity();

    internalData.jointParents = (int *)Mem_Alloc16(sizeof(int));
    internalData.jointParents[0] = -1;

    state = State::Ready;
}

bool Skeleton::Prepare(const char *filename, InternalData &outInternalData) {
    outInternalData.Purge();

    BE1::Str bSkelFilename = filename;
    if (!BE1::Str::CheckExtension(filename, ".bskel")) {
        bSkelFilename.SetFileExtension(".bskel");
    }

    BE_LOG("Loading skeleton '%s'...\n", bSkelFilename.c_str());

    byte *data;
    BE1::fileSystem.LoadFile(bSkelFilename, true, (void **)&data);
    if (!data) {
        return false;
    }

    const BSkelHeader *bSkelHeader = (const BSkelHeader *)data;
    byte *ptr = data + sizeof(BSkelHeader);

    if (bSkelHeader->ident != BSKEL_IDENT) {
        BE_WARNLOG("Skeleton::Load: bad format %s\n", bSkelFilename.c_str());
        BE1::fileSystem.FreeFile(data);
        return false;
    }

    outInternalData.numJoints = bSkelHeader->numJoints;
    if (outInternalData.numJoints > 0) {
        outInternalData.joints = new Joint[outInternalData.numJoints];
        outInternalData.bindPoses = (BE1::JointPose *)Mem_Alloc16(sizeof(BE1::JointPose) * outInternalData.numJoints);
        outInternalData.invBindPoseMats = (BE1::Mat3x4 *)Mem_Alloc16(sizeof(BE1::Mat3x4) * outInternalData.numJoints);
        outInternalData.jointParents = (int *)Mem_Alloc16(sizeof(int) * outInternalData.numJoints);

        // --- joints ---
        for (int jointIndex = 0; jointIndex < outInternalData.numJoints; jointIndex++) {
            const BJoint *bJoint = (const BJoint *)ptr;

            outInternalData.joints[jointIndex].name = bJoint->name;
            outInternalData.joints[jointIndex].parent = bJoint->parentIndex >= 0 ? &outInternalData.joints[bJoint->parentIndex] : nullptr;

            ptr += sizeof(BJoint);
        }

        for (int jointIndex = 0; jointIndex < outInternalData.numJoints; jointIndex++) {
            const Joint *joint = &outInternalData.joints[jointIndex];

            if (joint->parent) {
                outInternalData.jointParents[jointIndex] = static_cast<int>(joint->parent - outInternalData.joints);
            } else {
                outInternalData.jointParents[jointIndex] = -1;
            }
        }

        // --- bindpose ---
        for (int jointIndex = 0; jointIndex < outInternalData.numJoints; jointIndex++) {
            BE1::simdProcessor->Memcpy(&outInternalData.bindPoses[jointIndex].q, ptr, sizeof(outInternalData.bindPoses[jointIndex].q));
            ptr += sizeof(outInternalData.bindPoses[jointIndex].q);

            BE1::simdProcessor->Memcpy(&outInternalData.bindPoses[jointIndex].t, ptr, sizeof(outInternalData.bindPoses[jointIndex].t));
            ptr += sizeof(outInternalData.bindPoses[jointIndex].t);

            BE1::simdProcessor->Memcpy(&outInternalData.bindPoses[jointIndex].s, ptr, sizeof(outInternalData.bindPoses[jointIndex].s));
            ptr += sizeof(outInternalData.bindPoses[jointIndex].s);
        }

        // --- inverted bindpose ---
        int invBindPoseMatsSize = outInternalData.numJoints * sizeof(outInternalData.invBindPoseMats[0]);
        BE1::simdProcessor->Memcpy(outInternalData.invBindPoseMats, ptr, invBindPoseMatsSize);
        ptr += invBindPoseMatsSize;
    }

    BE1::fileSystem.FreeFile(data);
    return true;
}

void Skeleton::ApplyPrepared(InternalData &newInternalData) {
    assert(state == State::Empty || state == State::Loading || state == State::Ready || state == State::Reloading);

    Purge();

    internalData.Swap(newInternalData);
}

bool Skeleton::Load(const char *filename) {
    InternalData newInternalData;
    if (!Skeleton::Prepare(filename, newInternalData)) {
        return false;
    }

    BumpLoadSerial();
    state = State::Empty;

    ApplyPrepared(newInternalData);

    state = State::Ready;
    return true;
}

bool Skeleton::LoadAsync(const char *filename) {
    state = State::Loading;
    uint32_t newSerial = BumpLoadSerial();

    if (!loadingSystem.EnqueueSkeletonLoad(shared_from_this(), filename, false, newSerial)) {
        state = State::Failed;
        return false;
    }
    return true;
}

bool Skeleton::Reload() {
    if (state != State::Ready) {
        BE_WARNLOG("Skeleton::Reload: skeleton '%s' is not ready\n", GetHashName());
        return false;
    }

    if (!ReloadAsync()) {
        return false;
    }

    return WaitLoadComplete();
}

bool Skeleton::ReloadAsync() {
    if (!IsReady()) {
        BE_WARNLOG("Skeleton::ReloadAsync: skeleton '%s' is not ready\n", GetHashName());
        return false;
    }

    uint32_t newSerial = BumpLoadSerial();
    state = State::Reloading;

    if (!loadingSystem.EnqueueSkeletonLoad(shared_from_this(), hashName, true, newSerial)) {
        state = State::Ready;
        return false;
    }
    return true;
}

bool Skeleton::WaitReady() {
    assert(BE1::Engine::IsInMainThread());

    State currentState = GetState();
    if (currentState == State::Loading) {
        loadingSystem.BoostSkeletonLoad(this, GetLoadSerial(), LoadingSystem::LoadPriority::Critical);
    }

    while (currentState == State::Loading) {
        loadingSystem.ProcessLoadCompletions();
        currentState = GetState();

        if (currentState == State::Loading) {
            BE1::PlatformThread::Yield();
        }
    }

    return currentState == State::Ready || currentState == State::Reloading;
}

bool Skeleton::WaitLoadComplete() {
    assert(BE1::Engine::IsInMainThread());

    State currentState = GetState();
    if (currentState == State::Loading || currentState == State::Reloading) {
        loadingSystem.BoostSkeletonLoad(this, GetLoadSerial(), LoadingSystem::LoadPriority::Critical);
    }

    while (currentState == State::Loading || currentState == State::Reloading) {
        loadingSystem.ProcessLoadCompletions();
        loadingSystem.ProcessPendingReloads();
        currentState = GetState();

        if (currentState == State::Loading || currentState == State::Reloading) {
            BE1::PlatformThread::Yield();
        }
    }

    return currentState == State::Ready;
}

void Skeleton::Write(const char *filename) {
    assert(IsReady());

    BE1::File *fp = BE1::fileSystem.OpenFile(filename, BE1::File::Mode::Write);
    if (!fp) {
        BE_WARNLOG("Skeleton::Write: file open error\n");
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
        const Joint *joint = &internalData.joints[jointIndex];

        BJoint bJoint;
        BE1::Str::Copynz(bJoint.name, joint->name, sizeof(bJoint.name));
        bJoint.parentIndex = !joint->parent ? -1 : (int)(joint->parent - internalData.joints);
        fp->Write(&bJoint, sizeof(bJoint));
    }

    // --- bindposes ---
    for (int jointIndex = 0; jointIndex < internalData.numJoints; jointIndex++) {
        fp->Write(&internalData.bindPoses[jointIndex].q, sizeof(internalData.bindPoses[jointIndex].q));
        fp->Write(&internalData.bindPoses[jointIndex].t, sizeof(internalData.bindPoses[jointIndex].t));
        fp->Write(&internalData.bindPoses[jointIndex].s, sizeof(internalData.bindPoses[jointIndex].s));
    }

    // --- inverted bindposes ---
    fp->Write(internalData.invBindPoseMats, internalData.numJoints * sizeof(internalData.invBindPoseMats[0]));

    BE1::fileSystem.CloseFile(fp);
}

bool Skeleton::CheckHierarchy(const Skeleton *otherSkeleton) const {
    assert(IsReady());
    assert(otherSkeleton && otherSkeleton->IsReady());

    if (internalData.numJoints != otherSkeleton->NumJoints()) {
        return false;
    }

    const Joint *otherJoints = otherSkeleton->GetJoints();

    for (int i = 0; i < internalData.numJoints; i++) {
        const Joint *joint = &internalData.joints[i];
        const Joint *otherJoint = &otherJoints[i];

        if (joint->name != otherJoint->name) {
            return false;
        }

        int	parentIndex;
        if (joint->parent) {
            parentIndex = (int)(joint->parent - internalData.joints);
        } else {
            parentIndex = -1;
        }

        int otherParentIndex;
        if (otherJoint->parent) {
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
