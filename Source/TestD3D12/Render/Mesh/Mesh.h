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

#include "SubMesh.h"

class Joint;
class Skeleton;
class MeshInstance;
class MeshManager;
class LoadingSystem;

/*
---------------------------------------------------------------------------------------------------

    Mesh

    3D 메시 데이터를 나타낸다.
    Mesh 는 하나 이상의 Surface 로 구성된다.
    각 Surface 는 지오메트리를 저장하는 SubMesh 와 Material 인덱스를 가진다.

---------------------------------------------------------------------------------------------------
*/

class Mesh : public std::enable_shared_from_this<Mesh> {
    friend class MeshManager;
    friend class LoadingSystem;

public:
    enum class FinishFlag : uint8_t {
        None                        = 0,
        ComputeAABB                 = BIT(0),
        ComputeEdges                = BIT(1),
        ComputeNormals              = BIT(2),
        ComputeTangents             = BIT(3),
        UseUnsmoothedTangents       = BIT(4),
        SortAndMerge                = BIT(5),
        OptimizeIndices             = BIT(6)
    };

    friend constexpr std::true_type EnableBitmaskEnum(FinishFlag) noexcept { return {}; }

    enum class State : uint8_t {
        Empty,
        Loading,
        Ready,
        Reloading,
        Failed
    };

    enum class InstanceType : uint8_t {
        Static,
        Skinned
    };

    class Surface {
    public:
        Surface() = default;
        ~Surface() { SAFE_DELETE(subMesh); }

        SubMesh *                   subMesh = nullptr;
        int32_t                     materialIndex = -1;
    };

    Mesh() {}
    Mesh(const Mesh &rhs) = delete;
    Mesh &operator=(const Mesh &rhs) = delete;
    ~Mesh() { Purge(); }

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

                                    // 현재 적용된 메시 데이터의 시리얼 번호.
    uint32_t                        GetDataSerial() const { return dataSerial.load(); }

                                    // 실제 메시 데이터가 교체되었음을 표시한다.
    uint32_t                        BumpDataSerial() { return dataSerial.fetch_add(1) + 1; }

    int                             NumSurfaces() const { assert(IsReady()); return internalData.surfaces.Count(); }
    Surface *                       GetSurface(int index) const { assert(IsReady()); return internalData.surfaces[index]; }

                                    /// LOD 0 외에 추가 LOD 가 있는지 반환한다.
    bool                            HasLODs() const { return IsReady() && internalData.hasLODs; }

    int                             NumJoints() const { assert(IsReady()); return internalData.numJoints; }
    const Joint *                   GetJoints() const { assert(IsReady()); return internalData.joints; }

    bool                            IsCompatibleSkeleton(const Skeleton *skeleton) const { assert(IsReady()); return internalData.IsCompatibleSkeleton(skeleton); }

                                    /// Returns local axis-aligned bounding box (AABB).
    const BE1::AABB &               GetAABB() const { assert(IsReady()); return internalData.aabb; }

    static Surface *                AllocSurface(int numVerts, int numIndexes);

    void                            Reset();

                                    // 리로드 완료 후 리소스를 사용할 수 있으면 true 를 반환한다. 새 데이터 로딩 성공 여부와는 무관하다.
    bool                            Reload();

                                    // 비동기 리로드 요청 등록에 성공하면 true 를 반환하며, 완료 전까지 기존 리소스를 계속 사용할 수 있다.
    bool                            ReloadAsync();

                                    // 최초 로딩 중이면 완료될 때까지 기다리고, 리로딩 중이면 기존 Mesh 를 사용할 수 있으므로 Ready 로 간주한다.
    bool                            WaitReady();

                                    // 최초 로딩과 리로딩이 모두 완료될 때까지 기다린다.
    bool                            WaitLoadComplete();

    void                            Write(const char *filename);

    SharedPtr<MeshInstance>         GetStaticInstance();
    SharedPtr<MeshInstance>         InstantiateSkinned();

                                    /// Returns the volume of the solid mesh.
                                    /// NOTE: For an exact calculation the mesh must form a closed polytope. Otherwise, an approximation based on the AABB is used.
    float                           CalculateVolume() const;

                                    /// Returns centroid of solid mesh.
                                    /// NOTE: For an exact calculation the mesh must form a closed polytope. Otherwise, an approximation based on the AABB is used.
    const BE1::Vec3                 CalculateCentroid() const;

                                    /// Returns volume and centroid of solid mesh.
                                    /// NOTE: For an exact calculation the mesh must form a closed polytope. Otherwise, an approximation based on the AABB is used.
    float                           CalculateVolumeAndCentroid(BE1::Vec3 &outCentroid) const;

    static bool                     TrySliceMesh(const Mesh &srcMesh, const BE1::Plane &slicePlane, bool generateCap, float capTextureScale, bool generateAboveMesh, SharedPtr<Mesh> &outBelowMesh, SharedPtr<Mesh> &outAboveMesh);

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

        bool                        LoadBinaryMesh(const char *filename);
        void                        WriteBinaryMesh(const char *filename);

                                    /// Finalizes all surfaces by performing post-processing operations.
        void                        FinishSurfaces(FinishFlag finishFlags = FinishFlag::None);

                                    /// Applies a transformation to all vertices and optionally recomputes the AABB.
        void                        TransformVerts(const BE1::Mat3 &rotation, const BE1::Vec3 &scale, const BE1::Vec3 &translation, bool recomputeAABB = true);

        void                        RecomputeTangents();

        void                        GetJointAABBs(const BE1::Mat3x4 *invBindPoseMats, BE1::Array<BE1::AABB> &jointAabbs) const;

                                    /// Optimizes surface triangle and vertex order before instantiation or GPU upload.
                                    /// Invalidates edge data; request ComputeEdges in FinishSurfaces when needed.
        void                        OptimizeIndexedTriangles();

        bool                        IsCompatibleSkeleton(const Skeleton *skeleton) const;

        void                        SortAndMerge();
        void                        ComputeAABB();
        void                        ComputeNormals();
        void                        ComputeTangents(bool includeNormals, bool useUnsmoothedTangents);
        void                        ComputeEdges();

        BE1::Array<Surface *>       surfaces = BE1::Array<Surface *>(16);
        int32_t                     numJoints = 0;
        bool                        hasLODs = false;
        Joint *                     joints = nullptr;
        BE1::AABB                   aabb = BE1::AABB::empty;
    };

    void                            Purge();

    static bool                     Prepare(const char *filename, InternalData &outInternalData);
    void                            ApplyPrepared(InternalData &newInternalData);

    bool                            Load(const char *filename);
    bool                            LoadAsync(const char *filename);

    void                            CreateMissingMesh();
    void                            CreatePlane(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float size, int numSegments);
    void                            CreateBox(const BE1::Vec3 &origin, const BE1::Mat3 &axis, const BE1::Vec3 &extents);
    void                            CreateSphere(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float radius, int numSegments);
    void                            CreateGeosphere(const BE1::Vec3 &origin, float radius, int numSubdivisions);
    void                            CreateCylinder(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float radius, float height, int numSegments);
    void                            CreateCapsule(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float radius, float height, int numSegments);

    BE1::Str                        hashName;
    BE1::Str                        name;
    std::atomic<State>              state = State::Empty;
    std::atomic<uint32_t>           loadSerial = 0;
    std::atomic<uint32_t>           dataSerial = 0;
    InternalData                    internalData;
    WeakPtr<MeshInstance>           staticMeshInstance;
};
