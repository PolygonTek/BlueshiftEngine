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

class MeshManager;

struct BatchSubMesh {
    SubMesh *                   subMesh = nullptr;
    ALIGN_AS16 BE1::Mat3x4      localTransform;
};

// A class for managing 3D mesh data.
// A mesh consists of one or more surfaces, each containing its own sub-mesh and material information.
class Mesh {
    friend class MeshManager;

public:
    enum class Flag : uint8_t {
        None                    = 0,
        Permanent               = BIT(0),
    };

    enum class FinishFlag : uint8_t {
        None                    = 0,
        ComputeAABB             = BIT(0),
        ComputeEdges            = BIT(1),
        ComputeNormals          = BIT(2),
        ComputeTangents         = BIT(3),
        UseUnsmoothedTangents   = BIT(4),
        SortAndMerge            = BIT(5)
    };

    class Surface {
    public:
        Surface() = default;
        ~Surface() { SAFE_DELETE(subMesh); }

        SubMesh *               subMesh = nullptr;
        int32_t                 materialIndex = -1;
    };

    Mesh() : surfaces(16) {}
    ~Mesh() { Purge(); }

    const char *                GetName() const { return name; }
    const char *                GetHashName() const { return hashName; }

    Mesh *                      AddRefCount() { refCount++; return this; }
    const Mesh *                AddRefCount() const { refCount++; return this; }
    int                         GetRefCount() const { return refCount; }

    bool                        IsPermanentMesh() const { return BE1::HasFlag(flags, Flag::Permanent); }
    bool                        IsStaticMesh() const { return type == MeshType::Static; }
    bool                        IsSkinnedMesh() const { return type == MeshType::Skinned; }

                                /// Returns local axis-aligned bounding box (AABB).
    const BE1::AABB &           GetAABB() const { return aabb; }

                                /// Returns the volume of the solid mesh.
                                /// NOTE: For an exact calculation the mesh must form a closed polytope. Otherwise, an approximation based on the AABB is used.
    float                       CalculateVolume() const;
                                /// Returns centroid of solid mesh.
                                /// NOTE: For an exact calculation the mesh must form a closed polytope. Otherwise, an approximation based on the AABB is used.
    const BE1::Vec3             CalculateCentroid() const;
                                /// Returns volume and centroid of solid mesh.
                                /// NOTE: For an exact calculation the mesh must form a closed polytope. Otherwise, an approximation based on the AABB is used.
    float                       CalculateVolumeAndCentroid(BE1::Vec3 &outCentroid) const;

    int                         NumSurfaces() const { return surfaces.Count(); }
    Surface *                   GetSurface(int index) const { return surfaces[index]; }

    Surface *                   AllocSurface(int numVerts, int numIndexes);

                                /// Finalizes all surfaces by performing post-processing operations.
    void                        FinishSurfaces(FinishFlag finishFlags = FinishFlag::None);

                                /// Applies a transformation to all vertices and optionally recomputes the AABB.
    void                        TransformVerts(const BE1::Mat3 &rotation, const BE1::Vec3 &scale, const BE1::Vec3 &translation, bool recomputeAABB = true);

    void                        Purge() { surfaces.DeleteContents(true); }

    bool                        Load(const char *filename);
    bool                        Reload();

    void                        Write(const char *filename);

private:
    void                        CreateDefaultBox();
    void                        CreatePlane(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float size, int numSegments);
    void                        CreateBox(const BE1::Vec3 &origin, const BE1::Mat3 &axis, const BE1::Vec3 &extents);
    void                        CreateSphere(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float radius, int numSegments);
    void                        CreateCylinder(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float radius, float height, int numSegments);
    void                        CreateCapsule(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float radius, float height, int numSegments);
        
    void                        SortAndMerge();
    void                        ComputeAABB();
    void                        ComputeNormals();
    void                        ComputeTangents(bool includeNormals, bool useUnsmoothedTangents);
    void                        ComputeEdges();

    bool                        LoadBinaryMesh(const char *filename);
    void                        WriteBinaryMesh(const char *filename);

    BE1::Str                    hashName;
    BE1::Str                    name;
    mutable int32_t             refCount = 0;
    BE1::AABB                   aabb = BE1::AABB::empty;
    BE1::Array<Surface *>       surfaces;
    MeshType                    type = MeshType::Dummy;
    Flag                        flags = Flag::None;
};

template<>
struct enable_bitmask_operators<Mesh::Flag> {
    static const bool enable = true;
};

template<>
struct enable_bitmask_operators<Mesh::FinishFlag> {
    static const bool enable = true;
};

class MeshManager {
public:
    void                        Init();
    void                        Shutdown();

    Mesh *                      AllocMesh(const char *name);
    Mesh *                      FindMesh(const char *name) const;
    Mesh *                      GetMesh(const char *name);
    void                        ReleaseMesh(Mesh *mesh, bool immediateDestroy = false);
    void                        DestroyMesh(Mesh *mesh);

    void                        DestroyUnusedMeshes();

    Mesh *                      CreateCombinedMesh(const char *name, const BE1::Array<BatchSubMesh> &batchSubMeshes);

    void                        RenameMesh(Mesh *mesh, const BE1::Str &newName);

    Mesh *                      defaultMesh = nullptr;
    Mesh *                      defaultQuadMesh = nullptr;
    Mesh *                      defaultPlaneMesh = nullptr;
    Mesh *                      defaultBoxMesh = nullptr;
    Mesh *                      defaultSphereMesh = nullptr;
    Mesh *                      defaultCylinderMesh = nullptr;
    Mesh *                      defaultCapsuleMesh = nullptr;

private:
    void                        CreateEngineMeshes();

    BE1::StrIHashMap<Mesh *>    meshHashMap;
};

extern MeshManager              meshManager;
