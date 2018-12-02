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

    Anim
    
-------------------------------------------------------------------------------
*/

#include "Core/Str.h"
#include "Containers/Array.h"
#include "Containers/StrArray.h"
#include "Containers/HashIndex.h"
#include "Containers/HashMap.h"
#include "Core/JointPose.h"

class AnimImporter;

BE_NAMESPACE_BEGIN

class CmdArgs;
class Mesh;
class Skeleton;

class Anim {
    friend class AnimManager;
    friend class ::AnimImporter;

public:
    enum AnimBit {
        Tx                  = BIT(0),
        Ty                  = BIT(1),
        Tz                  = BIT(2),
        Qx                  = BIT(3),
        Qy                  = BIT(4),
        Qz                  = BIT(5),
        Sx                  = BIT(6),
        Sy                  = BIT(7),
        Sz                  = BIT(8),
        AllBits             = Tx | Ty | Tz | Qx | Qy | Qz | Sx | Sy | Sz
    };

    struct JointInfo {
        int32_t             nameIndex;
        int32_t             parentIndex;
        int32_t             animBits;
        int32_t             firstComponent;
    };

    struct FrameInterpolation {
        int32_t             frame1;
        int32_t             frame2;
        int32_t             cycleCount;     // how many times the animation has wrapped to the begining (0 for clamped anims)
        float               frontlerp;
        float               backlerp;
    };

    Anim();
    ~Anim();

    const char *            GetName() const { return name; }
    const char *            GetHashName() const { return hashName; }

    bool                    IsDefaultAnim() const { return isDefaultAnim; }
    bool                    IsAdditiveAnim() const { return isAdditiveAnim; }

                            /// Returns number of frames
    int                     NumFrames() const { return numFrames; }

                            /// Returns number of joints
    int                     NumJoints() const { return numJoints; }

                            /// Returns joint anim info
    const JointInfo &       GetJointInfo(int index) const { return jointInfo[index]; }

                            /// Returns animation length by milliseconds
    uint32_t                Length() const { return animLength; }

                            /// Returns maximum cycle count. 0 means infinite cycle loop
    int                     MaxCycleCount() const { return maxCycleCount; }

                            /// Returns movement delta of root joint
    const Vec3 &            TotalMovementDelta() const { return totalDelta; }

                            /// Returns total size of allocated memory
    size_t                  Allocated() const;
                            /// Returns total size of allocated memory including size of this type
    size_t                  Size() const { return sizeof(*this) + Allocated(); };

    void                    Purge();

                            /// Creates additive anim from other anim
    Anim *                  CreateAdditiveAnim(const Anim *refAnim, int numJointIndexes, const int *jointIndexes);

                            /// Creates additive anim from bind-pose
    Anim *                  CreateAdditiveAnim(const Skeleton *skeleton, int numJointIndexes, const int *jointIndexes);

                            // IMPLEMENT THIS
    Anim *                  CreateMirroredAnim(const int *jointMirrorTable);

    bool                    Load(const char *filename);
    bool                    Reload();
    void                    Write(const char *filename);
    const Anim *            AddRefCount() const { refCount++; return this; }
    
                            /// Check if the hierarchy is the same with the given skeleton (must have same joint names)
    bool                    CheckHierarchy(const Skeleton *skeleton) const;

                            /// Compute mesh AABB for each frames.
    void                    ComputeFrameAABBs(const Skeleton *skeleton, const Mesh *mesh, Array<AABB> &frameAABBs) const;

                            /// Converts time in milliseconds to the FrameInterpolation
    void                    TimeToFrameInterpolation(int time, FrameInterpolation &frameInterpolation) const;

    void                    GetTranslation(Vec3 &outTranslation, int time, bool cyclicTranslation = true) const;

    void                    GetRotation(Quat &outRotation, int time) const;

    void                    GetScaling(Vec3 &outScaling, int time) const;

    void                    GetAABB(AABB &outAabb, const Array<AABB> &frameAABBs, int time) const;

                            // frameNum 의 JointPose 를 얻는다. (NOTE: animation 의 frame 은 JointPose 배열을 말함)
    void                    GetSingleFrame(int frameNum, int numJointIndexes, const int *jointIndexes, JointPose *joints) const;

                            // frameInterpolation 의 JointPose 를 얻는다.
    void                    GetInterpolatedFrame(FrameInterpolation &frameInterpolation, int numJointIndexes, const int *jointIndexes, JointPose *joints) const;

private:
    Anim &                  Copy(const Anim &other);
    void                    CreateDefaultAnim(const Skeleton *skeleton);
    Anim *                  CreateAdditiveAnim(const char *name, const JointPose *firstFrame, int numJointIndexes, const int *jointIndexes);

    bool                    LoadBinaryAnim(const char *filename);
    void                    WriteBinaryAnim(const char *filename);

    void                    ComputeTotalDelta();

    void                    ComputeTimeFrames();

    void                    LerpFrame(int frameNum1, int frameNum2, float backlerp, JointPose *joints);
    void                    RemoveFrames(int numRemoveFrames, const int *removeFrameNums);
    void                    OptimizeFrames(float epsilonT = CentiToUnit(0.01f), float epsilonQ = 0.0015f, float epsilonS = 0.0001f);

    Str                     hashName;
    Str                     name;
    mutable int             refCount;

    int                     numJoints;              ///< Number of joints
    int                     numFrames;              ///< Number of frames
    int                     numAnimatedComponents;  ///< Number of animation components [0, 9]
    uint32_t                animLength;             ///< Animation length in milliseconds
    int                     maxCycleCount;          ///< 0 means infinite cycle loop

    bool                    rootRotation;
    bool                    rootTranslationXY;
    bool                    rootTranslationZ;
    bool                    isDefaultAnim;
    bool                    isAdditiveAnim;

    Array<JointInfo>        jointInfo;
    Array<JointPose>        baseFrame;              ///< Local transform for the first frame
    Array<float>            frameComponents;
    Array<int>              frameToTimeMap;         ///< Times for each frame
    Array<int>              timeToFrameMap;         ///< Frames for each 100 milliseconds
    Vec3                    totalDelta;             ///< Root translation offset in the total animation evaluation
};

BE_INLINE Anim::Anim() {
    refCount                = 0;
    numJoints               = 0;
    numFrames               = 0;
    numAnimatedComponents   = 0;
    animLength              = 0;
    totalDelta.SetFromScalar(0);
}

BE_INLINE Anim::~Anim() {
    Purge();
}

/*
-------------------------------------------------------------------------------

    Anim manager

-------------------------------------------------------------------------------
*/

class AnimManager {
public:
    void                    Init();
    void                    Shutdown();

    Anim *                  AllocAnim(const char *name);
    Anim *                  FindAnim(const char *name) const;
    Anim *                  GetAnim(const char *name);
    Anim *                  GetDefaultAnim(const char *name, const Skeleton *skeleton);

    void                    ReleaseAnim(Anim *anim, bool immediateDestroy = false);
    void                    DestroyAnim(Anim *anim);
    void                    DestroyUnusedAnims();

    void                    PrecacheAnim(const char *name);

    void                    RenameAnim(Anim *anim, const Str &newName);

    void                    ReloadAnims();

    int                     JointIndexByName(const char *name);
    const char *            JointNameByIndex(int index) const;

    static void             Cmd_ListAnims(const CmdArgs &args);

private:
    StrIHashMap<Anim *>     animHashMap;

    StrArray                jointNames;
    HashIndex               jointNameHash;
};

extern AnimManager          animManager;

BE_NAMESPACE_END
