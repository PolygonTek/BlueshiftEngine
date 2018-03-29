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
-------------------------------------------------------------------------------

    Skeleton

-------------------------------------------------------------------------------
*/

#include "Core/Str.h"
#include "Containers/HashMap.h"

class SkeletonImporter;

BE_NAMESPACE_BEGIN

class CmdArgs;
class JointPose;
class Mat3x4;

class Joint {
public:
    Joint() { parent = nullptr; }
    Str                     name;
    Joint *                 parent;
};

class Skeleton {
    friend class SkeletonManager;
    friend class ::SkeletonImporter;

public:
    Skeleton();
    ~Skeleton();

    const char *            GetName() const { return name; }
    const char *            GetHashName() const { return hashName; }

    bool                    IsDefaultSkeleton() const;

    int                     NumJoints() const { return numJoints; }
    const Joint *           GetJoints() const { return joints; }
    const Joint *           GetJoint(int index) const { return &joints[index]; }
    int                     GetJointIndex(const char *name) const;
    const char *            GetJointName(int jointIndex) const;

    const JointPose *       GetBindPoses() const { return bindPoses; }

    const Mat3x4 *          GetInvBindPoseMats() const { return invBindPoseMats; }

    bool                    CheckHierarchy(const Skeleton *otherSkeleton) const;

    void                    Purge();

    void                    CreateDefaultSkeleton();

    bool                    Load(const char *filename);
    bool                    Reload();
    void                    Write(const char *filename);
    const Skeleton *        AddRefCount() const { refCount++; return this; }

private:
    Str                     hashName;
    Str                     name;
    mutable int             refCount;
    bool                    permanence;

    int32_t                 numJoints;              ///< Number of joints
    Joint *                 joints;                 ///< Joint information array
    JointPose *             bindPoses;              ///< Local transforms of bindpose (quaternion based)
    Mat3x4 *                invBindPoseMats;        ///< World inverse transform of bindpose (for use in HW skinning)
};

BE_INLINE Skeleton::Skeleton() {
    refCount = 0;
    permanence = false;
    numJoints = 0;
    joints = nullptr;
    bindPoses = nullptr;
    invBindPoseMats = nullptr;
}

BE_INLINE Skeleton::~Skeleton() {
    Purge();
}

BE_INLINE int Skeleton::GetJointIndex(const char *name) const {
    for (int i = 0; i < numJoints; i++) {
        if (!joints[i].name.Cmp(name)) {
            return i;
        }
    }
    return -1;
}

BE_INLINE const char *Skeleton::GetJointName(int jointIndex) const {
    if (jointIndex >= 0 && jointIndex < numJoints) {
        return joints[jointIndex].name.c_str();
    }
    return nullptr;
}

class SkeletonManager {
    friend class Skeleton;

public:
    void                    Init();
    void                    Shutdown();

    Skeleton *              AllocSkeleton(const char *name);
    Skeleton *              FindSkeleton(const char *name) const;
    Skeleton *              GetSkeleton(const char *name);

    void                    ReleaseSkeleton(Skeleton *skeleton, bool immediateDestroy = false);
    void                    DestroySkeleton(Skeleton *skeleton);
    void                    DestroyUnusedSkeletons();

    void                    PrecacheSkeleton(const char *filename);

    void                    RenameSkeleton(Skeleton *skeleton, const Str &newName);

    static Skeleton *       defaultSkeleton;

private:
    static void             Cmd_ListSkeletons(const CmdArgs &args);
    static void             Cmd_ReloadSkeleton(const CmdArgs &args);

    StrIHashMap<Skeleton *> skeletonHashMap;
};

extern SkeletonManager      skeletonManager;

BE_NAMESPACE_END