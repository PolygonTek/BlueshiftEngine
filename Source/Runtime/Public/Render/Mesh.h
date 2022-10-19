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

    Mesh

-------------------------------------------------------------------------------
*/

#include "Math/Math.h"
#include "Containers/Array.h"
#include "Containers/HashMap.h"

class MeshImporter;

BE_NAMESPACE_BEGIN

class CmdArgs;
class Skeleton;
class Joint;
class Mat3x4;
class SkinningJointCache;
class DrawSurf;
class SubMesh;
class Ray;

class MeshSurf {
public:
    SubMesh *               subMesh;
    DrawSurf *              drawSurf;
    int32_t                 materialIndex;
    int32_t                 viewCount;
};

struct BatchSubMesh {
    SubMesh *               subMesh;
    ALIGN_AS16 Mat3x4       localTransform;
};

class Mesh {
    friend class MeshManager;
    friend class RenderWorld;
    friend class Batch;
    friend class ::MeshImporter;

public:
    struct Type {
        enum Enum {
            Reference,      // referenced by other type, not used directly.
            Static,         // vertex data in the static buffer.
            Dynamic,        // vertex data in the CPU memory (used for GUI, text, particle, ...).
            Skinned         // meshes for skinning.
        };
    };

    struct InstancingMethod {
        enum Enum {
            NoInstancing,
            UniformBuffer,
            InstancedArrays
        };
    };

    struct FinishFlag {
        enum Enum {
            ComputeAABB             = BIT(0),
            ComputeNormals          = BIT(1),
            ComputeTangents         = BIT(2),
            UseUnsmoothedTangents   = BIT(3),
            SortAndMerge            = BIT(4),
            OptimizeIndices         = BIT(5)
        };
    };

    Mesh();
    ~Mesh();

    const char *            GetName() const { return name; }
    const char *            GetHashName() const { return hashName; }

    const AABB &            GetAABB() const { return aabb; }

    bool                    IsDefaultMesh() const;
    bool                    IsStaticMesh() const { return isStaticMesh; }
    bool                    IsSkinnedMesh() const { return isSkinnedMesh; }

    bool                    IsCompatibleSkeleton(const Skeleton *skeleton) const;

    int                     NumSurfaces() const { return surfaces.Count(); }
    MeshSurf *              GetSurface(int index) const { assert(index >= 0 && index < surfaces.Count()); return surfaces[index]; }

    int                     NumJoints() const { return numJoints; }
    const Joint *           GetJoints() const { return joints; }

                            // Create instantiated mesh.
    Mesh *                  InstantiateMesh(Type::Enum meshType);

                            // Reinstantiate itself.
    void                    Reinstantiate();

    MeshSurf *              AllocSurface(int numVerts, int numIndexes) const;
    void                    FinishSurfaces(int finishFlags = 0);

    void                    TransformVerts(const Mat3 &rotation, const Vec3 &scale, const Vec3 &translation);

    void                    OptimizeIndexedTriangles();

    void                    Voxelize();

    void                    UpdateSkinningJointCache(const Skeleton *skeleton, const Mat3x4 *joints);

    bool                    IsIntersectLine(const Vec3 &p1, const Vec3 &p2, bool backFaceCull) const;

                            /// Intersects a ray with this mesh.
                            /// Returns false if there is no intersection.
    bool                    IntersectRay(const Ray &ray, bool ignoreBackFace, float *hitDist) const;
    float                   IntersectRay(const Ray &ray, bool ignoreBackFace) const;

                            /// Returns volume of solid mesh.
                            /// Should be a closed polytope to calculate exactly. If not AABB approximation will be used.
    float                   ComputeVolume() const;
                            /// Returns centroid of solid mesh.
                            /// Should be a closed polytope to calculate exactly. If not AABB approximation will be used.
    const Vec3              ComputeCentroid() const;

    float                   ComputeVolumeAndCentroid(Vec3 &outCentroid) const;

    void                    Purge();

    void                    CreateDefaultBox();
    void                    CreatePlane(const Vec3 &origin, const Mat3 &axis, float size, int numSegments);
    void                    CreateBox(const Vec3 &origin, const Mat3 &axis, const Vec3 &extents);
    void                    CreateSphere(const Vec3 &origin, const Mat3 &axis, float radius, int numSegments);
    void                    CreateGeosphere(const Vec3 &origin, float radius, int numSubdivisions);
    void                    CreateCylinder(const Vec3 &origin, const Mat3 &axis, float radius, float height, int numSegments);
    void                    CreateCapsule(const Vec3 &origin, const Mat3 &axis, float radius, float height, int numSegments);
    void                    CreateRoundedBox(const Vec3 &origin, const Mat3 &axis, const Vec3 extents, float radius, int numSubdivisions);

    static bool             TrySliceMesh(const Mesh &srcMesh, const Plane &slicePlane, bool generateCap, float capTextureScale, bool generateAboveMesh, Mesh *outBelowMesh, Mesh *outAboveMesh);

    bool                    Load(const char *filename);
    bool                    Reload();

    void                    Write(const char *filename);

    const Mesh *            AddRefCount() const { refCount++; return this; }
    int                     GetRefCount() const { return refCount; }

private:
    void                    FreeSurface(MeshSurf *surf) const;
    MeshSurf *              AllocInstantiatedSurface(const MeshSurf *refSurf, int meshType) const;

    void                    Instantiate(Type::Enum meshType);

    void                    SortAndMerge();
    void                    SplitMirroredVerts();

    void                    ComputeAABB();
    void                    ComputeNormals();
    void                    ComputeTangents(bool includeNormals, bool useUnsmoothedTangents);
    void                    ComputeEdges();

    bool                    LoadBinaryMesh(const char *filename);
    void                    WriteBinaryMesh(const char *filename);

    Str                     hashName;
    Str                     name;
    mutable int             refCount = 0;
    bool                    permanence = false;

    Mesh *                  originalMesh = nullptr;
    Array<Mesh *>           instantiatedMeshes;

    bool                    isInstantiated = false;
    bool                    isStaticMesh = false;
    bool                    isSkinnedMesh = false;
    AABB                    aabb = AABB::empty;
    Array<MeshSurf *>       surfaces;

    bool                    useGpuSkinning = false;
    SkinningJointCache *    skinningJointCache = nullptr;   // joint cache for HW skinning

    int32_t                 numJoints = 0;
    Joint *                 joints = nullptr;               // joint information array
};

BE_INLINE Mesh::Mesh() {
    surfaces.SetGranularity(16);
}

BE_INLINE Mesh::~Mesh() {
    Purge();
}

class MeshManager {
    friend class Mesh;

public:
    void                    Init();
    void                    Shutdown();
    
    Mesh *                  AllocMesh(const char *name);
    Mesh *                  AllocInstantiatedMesh(Mesh *refMesh);
    Mesh *                  FindMesh(const char *name) const;
    Mesh *                  GetMesh(const char *name);

    Mesh *                  CreateCombinedMesh(const char *name, const Array<BatchSubMesh> &batchSubMeshes);

    void                    ReleaseMesh(Mesh *mesh, bool immediateDestroy = false);
    void                    DestroyMesh(Mesh *mesh);
    void                    DestroyUnusedMeshes();

    void                    PrecacheMesh(const char *filename);

    void                    RenameMesh(Mesh *mesh, const Str &newName);

    void                    EndLevelLoad();

    void                    ReinstantiateSkinnedMeshes();

    static Mesh *           defaultMesh;
    static Mesh *           defaultQuadMesh;
    static Mesh *           defaultPlaneMesh;
    static Mesh *           defaultBoxMesh;
    static Mesh *           defaultSphereMesh;
    static Mesh *           defaultGeoSphereMesh;
    static Mesh *           defaultCylinderMesh;
    static Mesh *           defaultCapsuleMesh;
    static Mesh *           defaultRoundedBoxMesh;

private:
    static void             Cmd_ListMeshes(const CmdArgs &args);
    static void             Cmd_ReloadMesh(const CmdArgs &args);

    void                    CreateEngineMeshes();

    StrIHashMap<Mesh *>     meshHashMap;

    Array<Mesh *>           instantiatedMeshList;
};

extern MeshManager          meshManager;

BE_NAMESPACE_END
