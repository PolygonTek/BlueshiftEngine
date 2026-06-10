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

#pragma once

#include "Core/Str.h"
#include "Containers/Array.h"
#include "Containers/StrArray.h"
#include "Containers/HashIndex.h"
#include "Containers/HashMap.h"
#include "Core/JointPose.h"

class CmdArgs;
class Mesh;
class Skeleton;
class LoadingSystem;

/*
---------------------------------------------------------------------------------------------------

    Anim

---------------------------------------------------------------------------------------------------
*/

class Anim : public std::enable_shared_from_this<Anim> {
    friend class AnimManager;
    friend class LoadingSystem;

public:
    struct ComponentBit {
        enum Enum {
            Tx                      = BIT(0),
            Ty                      = BIT(1),
            Tz                      = BIT(2),
            Qx                      = BIT(3),
            Qy                      = BIT(4),
            Qz                      = BIT(5),
            Sx                      = BIT(6),
            Sy                      = BIT(7),
            Sz                      = BIT(8),
            Txyz                    = Tx | Ty | Tz,
            Qxyz                    = Qx | Qy | Qz,
            Sxyz                    = Sx | Sy | Sz,
            AllBits                 = Tx | Ty | Tz | Qx | Qy | Qz | Sx | Sy | Sz
        };
    };

    enum class State : uint8_t {
        Empty,
        Loading,
        Ready,
        Reloading,
        Failed
    };

    struct JointInfo {
        int32_t                     nameIndex;          ///< Joint name index managed by AnimManager class.
        int32_t                     parentIndex;        ///< Parent joint index. -1 means has no parent.
        int32_t                     componentBits;      ///< Animation component bits for this joint.
        int32_t                     componentOffset;    ///< Offset of the component buffer for this joint.
    };

    struct FrameInterpolation {
        int32_t                     frame1;             ///< Frame number 1 for interpolation.
        int32_t                     frame2;             ///< Frame number 2 for interpolation.
        int32_t                     cycleCount;         ///< How many times the animation has wrapped to the beginning (0 for clamped anims)
        float                       backlerp;           ///< A fractional value that maps [0, 1] to [frame1, frame2]
        float                       frontlerp;          ///< A fractional value that maps [1, 0] to [frame1, frame2]
    };

    Anim() = default;
    ~Anim() { Purge(); }

    const char *                    GetName() const { return name; }
    const char *                    GetHashName() const { return hashName; }

    State                           GetState() const { return state.load(); }
    void                            SetState(State newState) { state.store(newState); }
    bool                            IsReady() const { State s = GetState(); return s == State::Ready || s == State::Reloading; }
    bool                            IsLoading() const { State s = GetState(); return s == State::Loading || s == State::Reloading; }

    uint32_t                        GetLoadSerial() const { return loadSerial.load(); }
    uint32_t                        BumpLoadSerial() { return loadSerial.fetch_add(1) + 1; }

    bool                            IsDefaultAnim() const { return internalData.isDefaultAnim; }
    bool                            IsAdditiveAnim() const { return internalData.isAdditiveAnim; }

                                    /// Returns number of frames.
    int                             NumFrames() const { assert(IsReady()); return internalData.numFrames; }

                                    /// Returns number of joints.
    int                             NumJoints() const { assert(IsReady()); return internalData.numJoints; }

                                    /// Returns joint info of animation.
    const JointInfo &               GetJointInfo(int index) const { assert(IsReady()); return internalData.joints[index]; }

                                    /// Returns animation length by milliseconds.
    uint32_t                        Length() const { assert(IsReady()); return internalData.length; }

                                    /// Returns maximum cycle count. 0 means infinite cycle loop.
    int                             MaxCycleCount() const { assert(IsReady()); return internalData.maxCycleCount; }

                                    /// Returns movement delta of root joint.
    const BE1::Vec3 &               TotalMovementDelta() const { assert(IsReady()); return internalData.totalDelta; }

                                    /// Returns total size of allocated memory.
    size_t                          Allocated() const;
                                    /// Returns total size of allocated memory including size of this type.
    size_t                          Size() const { return sizeof(*this) + Allocated(); };

    void                            Reset();

                                    /// Creates additive anim from other anim.
    SharedPtr<Anim>                 CreateAdditiveAnim(const Anim *refAnim, int numJointIndexes, const int *jointIndexes);

                                    /// Creates additive anim from bind-pose.
    SharedPtr<Anim>                 CreateAdditiveAnim(const Skeleton *skeleton, int numJointIndexes, const int *jointIndexes);

                                    // TODO: IMPLEMENT THIS
    SharedPtr<Anim>                 CreateMirroredAnim(const int *jointMirrorTable);

    bool                            Load(const char *filename);
    bool                            LoadAsync(const char *filename);

    bool                            Reload();
    bool                            ReloadAsync();

    bool                            WaitReady();
    bool                            WaitLoadComplete();

    void                            Write(const char *filename);
    
                                    /// Check if the hierarchy is the same with the given skeleton (must have same joint names)
    bool                            CheckHierarchy(const Skeleton *skeleton) const;

                                    /// Compute mesh AABB for each frames.
    void                            ComputeFrameAABBs(const Skeleton *skeleton, const Mesh *mesh, BE1::Array<BE1::AABB> &frameAABBs) const;

                                    /// Converts time in milliseconds to the FrameInterpolation.
    void                            TimeToFrameInterpolation(int time, FrameInterpolation &frameInterpolation) const;

                                    /// Compute translation of the root joint with the given time.
    void                            GetTranslation(BE1::Vec3 &outTranslation, int time, bool isCyclicTranslation = true) const;

                                    /// Compute rotation of the root joint with the given time.
    void                            GetRotation(BE1::Quat &outRotation, int time) const;

                                    /// Compute scaling of the root joint with the given time.
    void                            GetScaling(BE1::Vec3 &outScaling, int time) const;

                                    /// Compute AABB with the given time and each frame AABBs.
    void                            GetAABB(BE1::AABB &outAabb, const BE1::Array<BE1::AABB> &frameAABBs, int time) const;

                                    /// Compute single frame without interpolation.
    void                            GetSingleFrame(int frameNum, int numJointIndexes, const int *jointIndexes, BE1::JointPose *frame) const;

                                    /// Compute interpolated frame.
    void                            GetInterpolatedFrame(FrameInterpolation &frameInterpolation, int numJointIndexes, const int *jointIndexes, BE1::JointPose *frame) const;

private:
    struct InternalData {
        InternalData() = default;
        InternalData(const InternalData &rhs) = delete;
        InternalData &operator=(const InternalData &rhs) = delete;
        InternalData(InternalData &&rhs) noexcept { Swap(rhs); }
        InternalData &operator=(InternalData &&rhs) noexcept;
        ~InternalData() { Purge(); }

        void                        Purge();
        void                        Swap(InternalData &rhs);

        size_t                      Allocated() const;

        void                        CopyFrom(const InternalData &rhs);
        void                        ResolveJointNames();

        bool                        LoadBinaryAnim(const char *filename);
        void                        WriteBinaryAnim(const char *filename) const;

        bool                        hasRootRotation = false;
        bool                        hasRootTranslationXY = false;
        bool                        hasRootTranslationZ = false;
        bool                        isDefaultAnim = false;
        bool                        isAdditiveAnim = false;

        int                         numJoints = 0;                  ///< Number of joints.
        int                         numFrames = 0;                  ///< Number of frames.
        int                         numComponentsPerFrame = 0;      ///< Number of animated components of all joints.
        uint32_t                    length = 0;                     ///< Animation length in milliseconds.
        int                         maxCycleCount = 1;              ///< 0 means infinite cycle loop.

        BE1::Array<JointInfo>       joints;                         ///< Joints informations.
        BE1::StrArray               jointNames;                     ///< Joint names pending AnimManager index resolve.
        BE1::Array<BE1::JointPose>  baseFrame;                      ///< Local transform of all joints in the 0'th frame.
        BE1::Array<float>           components;                     ///< Components for each animated joints of all frames.
        BE1::Array<int>             frameTimes;                     ///< Times for each frames.
        BE1::Vec3                   totalDelta = BE1::Vec3::zero;   ///< Root translation offset in total animation evaluation.
    };

    void                            Purge();

    static bool                     Prepare(const char *filename, InternalData &outInternalData);
    void                            ApplyPrepared(InternalData &newInternalData);

    Anim &                          Copy(const Anim &other);

    void                            CreateDefaultAnim(const Skeleton *skeleton);
    SharedPtr<Anim>                 CreateAdditiveAnim(const char *name, const BE1::JointPose *firstFrame, int numJointIndexes, const int *jointIndexes);

    void                            ComputeTotalDelta();

    void                            ComputeRemovableFrames(const BE1::JointPose *frameJoints, const int *jointIndexes, BE1::JointPose *lerpedJoints, float epsilonT, float epsilonQ, float epsilonS, int frameNum1, int frameNum2, BE1::Array<int> &removableFrameNums);
    void                            RemoveFrames(const BE1::Array<int> &removableFrameNums);
    void                            OptimizeFrames(float epsilonT = BE1::MmToUnit(0.1f), float epsilonQ = 0.0015f, float epsilonS = 0.0001f);

    BE1::Str                        hashName;
    BE1::Str                        name;
    std::atomic<State>              state = State::Empty;
    std::atomic<uint32_t>           loadSerial = 0;
    InternalData                    internalData;
};

/*
---------------------------------------------------------------------------------------------------

    Anim manager

---------------------------------------------------------------------------------------------------
*/

class AnimManager {
public:
    void                            Init();
    void                            Shutdown();

    void                            UpdateFrame();

    SharedPtr<Anim>                 AllocAnim(const char *hashName);
    SharedPtr<Anim>                 FindAnim(const char *hashName) const;
    SharedPtr<Anim>                 GetAnim(const char *hashName, bool async = true);
    SharedPtr<Anim>                 GetDefaultAnim(const char *hashName, const Skeleton *skeleton);

    int                             JointIndexByName(const char *jointName);
    const char *                    JointNameByIndex(int index) const;

private:
    void                            CleanExpiredAnims();

    BE1::StrIHashMap<WeakPtr<Anim>> animHashMap;
    int                             expiredAnimHashMapCursor = 0;

    BE1::StrArray                   jointNames;
    BE1::HashIndex                  jointNameHash;
};

extern AnimManager                  animManager;
