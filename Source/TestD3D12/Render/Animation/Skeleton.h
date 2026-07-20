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

class SkeletonManager;
class LoadingSystem;

class Joint {
public:
    Joint() = default;
    BE1::Str                        name;
    Joint *                         parent = nullptr;
};

/*
---------------------------------------------------------------------------------------------------

    Skeleton

---------------------------------------------------------------------------------------------------
*/

class Skeleton : public std::enable_shared_from_this<Skeleton> {
    friend class SkeletonManager;
    friend class LoadingSystem;

public:
    enum class State : uint8_t {
        Empty,
        Loading,
        Ready,
        Reloading,
        Failed
    };

    Skeleton() = default;
    Skeleton(const Skeleton &rhs) = delete;
    Skeleton &operator=(const Skeleton &rhs) = delete;
    ~Skeleton() { Purge(); }

    const char *                    GetName() const { return name.c_str(); }
    const char *                    GetHashName() const { return hashName.c_str(); }

    State                           GetState() const { return state.load(); }
    void                            SetState(State newState) { state.store(newState); }
    bool                            IsReady() const { State s = GetState(); return s == State::Ready || s == State::Reloading; }
    bool                            IsLoading() const { State s = GetState(); return s == State::Loading || s == State::Reloading; }

                                    // 로딩 요청 했던 시리얼 번호.
    uint32_t                        GetLoadSerial() const { return loadSerial.load(); }

                                    // 이미 진행 중인 로딩 completion 무효화
    uint32_t                        BumpLoadSerial() { return loadSerial.fetch_add(1) + 1; }

    int                             NumJoints() const { assert(IsReady()); return internalData.numJoints; }
    const Joint *                   GetJoints() const { assert(IsReady()); return internalData.joints; }
    const Joint *                   GetJoint(int index) const { assert(IsReady()); return &internalData.joints[index]; }
    int                             GetJointIndex(const char *name) const;
    const char *                    GetJointName(int jointIndex) const;
    const int *                     GetJointParentIndexes() const { assert(IsReady()); return internalData.jointParents; }
    const BE1::JointPose *          GetBindPoses() const { assert(IsReady()); return internalData.bindPoses; }
    const BE1::Mat3x4 *             GetInvBindPoseMatrices() const { assert(IsReady()); return internalData.invBindPoseMats; }

    bool                            IsDefaultSkeleton() const;

    void                            Reset();

                                    // 리로드 완료 후 리소스를 사용할 수 있으면 true 를 반환한다. 새 데이터 로딩 성공 여부와는 무관하다.
    bool                            Reload();

                                    // 비동기 리로드 요청 등록에 성공하면 true 를 반환하며, 완료 전까지 기존 리소스를 계속 사용할 수 있다.
    bool                            ReloadAsync();

                                    // 최초 로딩 중이면 완료될 때까지 기다리고, 리로딩 중이면 기존 Skeleton 을 사용할 수 있으므로 Ready 로 간주한다.
    bool                            WaitReady();

                                    // 최초 로딩과 리로딩이 모두 완료될 때까지 기다린다.
    bool                            WaitLoadComplete();

    void                            Write(const char *filename);

    bool                            CheckHierarchy(const Skeleton *otherSkeleton) const;

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

        int32_t                     numJoints = 0;              ///< Number of joints
        Joint *                     joints = nullptr;           ///< Joint information array
        int *                       jointParents = nullptr;     ///< Parent indexes for each joint
        BE1::JointPose *            bindPoses = nullptr;        ///< Local joint transforms of bindpose (quaternion based)
        BE1::Mat3x4 *               invBindPoseMats = nullptr;  ///< World inverse transform of bindpose (for use in HW skinning)
    };

    void                            Purge();

    static bool                     Prepare(const char *filename, InternalData &outInternalData);
    void                            ApplyPrepared(InternalData &newInternalData);

    bool                            Load(const char *filename);
    bool                            LoadAsync(const char *filename);

    void                            CreateDefaultSkeleton();

    BE1::Str                        hashName;
    BE1::Str                        name;
    std::atomic<State>              state = State::Empty;
    std::atomic<uint32_t>           loadSerial = 0;
    InternalData                    internalData;
};

BE_INLINE int Skeleton::GetJointIndex(const char *name) const {
    assert(IsReady());

    for (int i = 0; i < internalData.numJoints; i++) {
        if (!internalData.joints[i].name.Cmp(name)) {
            return i;
        }
    }
    return -1;
}

BE_INLINE const char *Skeleton::GetJointName(int jointIndex) const {
    assert(IsReady());

    if (jointIndex >= 0 && jointIndex < internalData.numJoints) {
        return internalData.joints[jointIndex].name.c_str();
    }
    return nullptr;
}
