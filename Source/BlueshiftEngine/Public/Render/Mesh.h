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
struct SkinningJointCache;
class DrawSurf;
class SubMesh;

class MeshSurf {
public:
    SubMesh *               subMesh;
    DrawSurf *              drawSurf;
    int32_t                 materialIndex;
    int32_t                 viewCount;
};

class Mesh {
    friend class MeshManager;
    friend class RBSurf;
    friend class ::MeshImporter;

public:    
    enum Type {
        ReferenceMesh,      // 직접적으로 사용하지 않고, 다른 type 의 mesh 가 참조하는 용도로 사용한다.
        StaticMesh,         // vertex 데이터는 static buffer 에 들어간다.
        DynamicMesh,        // vertex 데이터를 CPU 에서 deform 하는 용도의 mesh (will be deprecated)
        SkinnedMesh         // skinning 용 mesh
    };

    enum SkinningMethod {
        CpuSkinning,
        VertexShaderSkinning,
        VtfSkinning
    };

    enum VertexTextureUpdate {
        DirectCopyUpdate,   // obsolete !
        PboUpdate,
        TboUpdate
    };    

    enum FinishFlag {
        ComputeAABBFlag     = BIT(0),
        ComputeNormalsFlag  = BIT(1),
        ComputeTangentsFlag = BIT(2),
        UseUnsmoothedTangentsFlag = BIT(3),
        SortAndMergeFlag    = BIT(4),
        OptimizeIndicesFlag = BIT(5)
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
    int                     GetJointIndex(const char *name) const;
    const char *            GetJointName(int jointIndex) const;

                            // Create instantiated mesh
    Mesh *                  InstantiateMesh(int meshType);

                            // Reinstantiate itself
    void                    Reinstantiate();

    MeshSurf *              AllocSurface(int numVerts, int numIndexes) const;
    void                    FinishSurfaces(int finishFlags = 0);

    void                    TransformVerts(const Mat3 &rotation, const Vec3 &translation);

    void                    OptimizeIndexedTriangles();

    void                    Voxelize();

    void                    UpdateSkinningJointCache(const Skeleton *skeleton, const Mat3x4 *joints);

    bool                    LineIntersection(const Vec3 &start, const Vec3 &end, bool backFaceCull) const;
    bool                    RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &scale) const;

    float                   ComputeVolume() const;
    const Vec3              ComputeCentroid() const;

    void                    Purge();

    void                    CreateDefaultBox();
    void                    CreatePlane(const Vec3 &origin, const Mat3 &axis, float size, int numSegments);
    void                    CreateBox(const Vec3 &origin, const Mat3 &axis, const Vec3 &extents);
    void                    CreateSphere(const Vec3 &origin, const Mat3 &axis, float radius, int numSegments);
    void                    CreateGeosphere(const Vec3 &origin, float radius, int numTess);
    void                    CreateCylinder(const Vec3 &origin, const Mat3 &axis, float radius, float height, int numSegments);
    void                    CreateCapsule(const Vec3 &origin, const Mat3 &axis, float radius, float height, int numSegments);

    bool                    Load(const char *filename);
    bool                    Reload();
    void                    Write(const char *filename);
    const Mesh *            AddRefCount() const { refCount++; return this; }

private:
    void                    FreeSurface(MeshSurf *surf) const;
    MeshSurf *              AllocInstantiatedSurface(const MeshSurf *refSurf, int meshType) const;

    void                    Instantiate(int meshType);

    void                    SortAndMerge();
    void                    SplitMirroredVerts();

    void                    ComputeAABB();
    void                    ComputeNormals();
    void                    ComputeTangents(bool includeNormals, bool useUnsmoothedTangents);
    void                    ComputeEdges();

    bool                    CapableGPUJointSkinning(SkinningMethod skinningMethod, int numJoints) const;

    bool                    LoadBinaryMesh(const char *filename);
    void                    WriteBinaryMesh(const char *filename);

    Str                     hashName;
    Str                     name;
    mutable int             refCount;
    bool                    permanence;

    Mesh *                  originalMesh;
    Array<Mesh *>           instantiatedMeshes;

    bool                    isInstantiated;
    bool                    isStaticMesh;
    bool                    isSkinnedMesh;
    AABB                    aabb;
    Array<MeshSurf *>       surfaces;

    bool                    useGpuSkinning;
    SkinningJointCache *    skinningJointCache;     // joint cache for HW skinning

    int32_t                 numJoints;
    Joint *                 joints;                 // joint information array
};

BE_INLINE Mesh::Mesh() {
    refCount                = 0;
    permanence              = false;
    isInstantiated          = false;
    originalMesh            = nullptr;
    isStaticMesh            = false;
    isSkinnedMesh           = false;
    useGpuSkinning          = false;
    skinningJointCache      = nullptr;
    numJoints               = 0;
    joints                  = nullptr;
    aabb.Clear();
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

private:
    static void             Cmd_ListMeshes(const CmdArgs &args);
    static void             Cmd_ReloadMesh(const CmdArgs &args);

    StrIHashMap<Mesh *>     meshHashMap;

    Array<Mesh *>           instantiatedMeshList;
};

extern MeshManager          meshManager;

BE_NAMESPACE_END
