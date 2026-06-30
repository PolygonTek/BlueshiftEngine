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
class MeshInstance;
class MeshManager;
class LoadingSystem;

/*
---------------------------------------------------------------------------------------------------

    Mesh

    A class for managing 3D mesh data.
    A mesh consists of one or more surfaces, each containing its own sub-mesh and material information.

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
        SortAndMerge                = BIT(5)
    };

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

    Mesh() = default;
    Mesh(const Mesh &rhs) = delete;
    Mesh &operator=(const Mesh &rhs) = delete;
    ~Mesh() { Purge(); }

    const char *                    GetName() const { return name; }
    const char *                    GetHashName() const { return hashName; }

    State                           GetState() const { return state.load(); }
    void                            SetState(State newState) { state.store(newState); }
    bool                            IsReady() const { State s = GetState(); return s == State::Ready || s == State::Reloading; }
    bool                            IsLoading() const { State s = GetState(); return s == State::Loading || s == State::Reloading; }

    uint32_t                        GetLoadSerial() const { return loadSerial.load(); }
    uint32_t                        BumpLoadSerial() { return loadSerial.fetch_add(1) + 1; }

    uint32_t                        GetDataSerial() const { return dataSerial.load(); }

    int                             NumSurfaces() const { assert(IsReady()); return internalData.surfaces.Count(); }
    Surface *                       GetSurface(int index) const { assert(IsReady()); return internalData.surfaces[index]; }

    int                             NumJoints() const { assert(IsReady()); return internalData.numJoints; }
    const Joint *                   GetJoints() const { assert(IsReady()); return internalData.joints; }

                                    /// Returns local axis-aligned bounding box (AABB).
    const BE1::AABB &               GetAABB() const { assert(IsReady()); return internalData.aabb; }

    static Surface *                AllocSurface(int numVerts, int numIndexes);

    void                            Reset();

    bool                            Load(const char *filename);
    bool                            LoadAsync(const char *filename);

    bool                            Reload();
    bool                            ReloadAsync();

    bool                            WaitReady();
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

        void                        SortAndMerge();
        void                        ComputeAABB();
        void                        ComputeNormals();
        void                        ComputeTangents(bool includeNormals, bool useUnsmoothedTangents);
        void                        ComputeEdges();

        BE1::Array<Surface *>       surfaces = BE1::Array<Surface *>(16);
        int32_t                     numJoints = 0;
        Joint *                     joints = nullptr;
        BE1::AABB                   aabb = BE1::AABB::empty;
    };

    void                            Purge();

    static bool                     Prepare(const char *filename, InternalData &outInternalData);
    void                            ApplyPrepared(InternalData &newInternalData);

    uint32_t                        BumpDataSerial() { return dataSerial.fetch_add(1) + 1; }

    void                            CreateDefaultBox();
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

template<>
struct enable_bitmask_operators<Mesh::FinishFlag> {
    static const bool enable = true;
};
