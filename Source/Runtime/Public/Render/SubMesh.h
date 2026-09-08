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

/*
-------------------------------------------------------------------------------

    SubMesh

-------------------------------------------------------------------------------
*/

#include "Core/Vertex.h"

class MeshImporter;

BE_NAMESPACE_BEGIN

struct BufferCache;

class Material;

/// Edge
/// 
/// Represents a line segment of a triangle.
/// The vertex index represents the line segment from small to large.
/// Edges in this direction use positive indexes, and shared opposite edges use negative indexes.
struct Edge {
/*
      ------ v0
            / \
      t1   /   \
   \      e     \
    \    /   t0  \
     \  /        
      v1 ------
*/
    int32_t                 v[2];               ///< vertex indexes to define an edge. v[0] is always less than v[1]. opposite direction edge is not stored in the sub mesh.
    int32_t                 t[2];               ///< adjacent triangle indexes. t[0] is a CCW triangle.
};

/// Joint weight
struct JointWeight {
    int32_t                 jointMatOffset;     ///< offset of matrix array referenced by weight index.
    int32_t                 nextVertOffset;     ///< 0 means the same vertex.
};

class SubMesh {
    friend class Mesh;
    friend class MeshManager;
    friend class Collider;
    friend class RenderWorld;
    friend class Batch;
    friend class ::MeshImporter;
    
public:
    SubMesh();
    ~SubMesh();

                            /// Returns total size of allocated memory
    int                     Allocated() const;

    int                     GetType() const { return type; }

    bool                    IsShared(const SubMesh *other) const { return refSubMesh == other->refSubMesh; }

                            /// Copy sub mesh data. Should be allocated first.
    void                    CopyFrom(const SubMesh *other);

    int                     NumVerts() const { return numVerts; }
    int                     NumOriginalVerts() const { return numVerts - numMirroredVerts; }
    VertexGenericLit *      Verts() const { return verts; }

    int                     NumIndexes(int lodLevel = 0) const;
    VertIndex *             Indexes() const { return indexes; }
    const VertIndex *       Indexes(int lodLevel) const;

                            /// LOD 0 는 원본이며, 모든 LOD 는 같은 vertex 와 skinning weights 를 공유한다.
    int                     NumLODs() const { return 1 + (refSubMesh ? refSubMesh->lods : lods).Count(); }

                            /// 원본 대비 LOD 의 단순화 오차 추정값을 로컬 공간 단위로 반환한다.
                            /// 노멀, UV, 색상 등의 속성 오차도 포함하며 LOD 0 은 0 을 반환한다.
    float                   GetLODError(int level) const;

                            /// 메시의 LOD 를 생성한다. LOD 0 는 보존한다.
                            /// maxLevels 는 최대 개수를 의미하며, 0 은 제한 없음을 의미한다.
    void                    GenerateLODs(int maxLevels);

    int                     VertexWeightSize() const;
    int                     MaxVertexWeights() const;
    void *                  VertexWeights() const { return vertWeights; }

                            /// Finds an edge index by two vertex indexes.
                            /// If v1 is larger than v2, negative number will be returned.
                            /// To use this function, edge information must be pre-calculated.
    int                     FindEdge(int32_t v1, int32_t v2) const;

                            /// Test if this sub mesh is closed surface form.
                            /// To use this function, edge information must be pre-calculated.
    bool                    IsClosed() const;

                            /// Tests if this sub mesh intersect with the given line segment.
                            /// To use this function, edge information must be pre-calculated.
    bool                    IsIntersectLine(const Vec3 &p1, const Vec3 &p2, bool ignoreBackFace) const;

                            /// Ray intersection.
                            /// To use this function, edge information must be pre-calculated.
    bool                    IntersectRay(const Ray &ray, bool ignoreBackFace, float *hitDist = nullptr, Vec3 *hitNormal = nullptr) const;

    const AABB &            GetAABB() const { return aabb; }

                            /// Compute mass properties (useful only for closed mesh)
    float                   CalculateVolume() const;
    const Vec3              CalculateCentroid() const;
    const Mat3              CalculateInertiaTensor(const Vec3 &centroid, float mass) const;

                            /// Optimizes triangle and vertex order; requires an unshared reference mesh before GPU upload.
                            /// Invalidates edge data; recompute edges before using edge-dependent operations.
    void                    OptimizeIndexedTriangles();

    bool                    IsGpuSkinningEnabled() const { return gpuSkinningEnabled; }

                            /// vertices 와 지정한 LOD indices 를 캐시하고 해당 인덱스 캐시를 반환한다.
    const BufferCache *     CacheStaticDataToGpu(int lodLevel = 0);
    void                    CacheDynamicDataToGpu(const Mat3x4 *joints, const Material *material);

private:
    struct LOD {
        Array<VertIndex>    indexes;
        float               error = 0.0f;   ///< 로컬 공간 단위의 속성 오차를 포함한 단순화 오차.
    };

    void                    AllocSubMesh(int numVerts, int numIndexes);
    void                    AllocInstantiatedSubMesh(const SubMesh *refMesh, int meshType);
    void                    FreeSubMesh();
    void                    FreeLODIndexCaches();

    void                    SplitMirroredVerts();
    void                    FixMirroredVerts();

    void                    ComputeAABB();
    void                    ComputeNormals();
    void                    ComputeDominantTris();
    void                    ComputeTangents(bool includeNormals, bool useUnsmoothedTangents);
    void                    ComputeEdges();

    int                     type;
    bool                    alloced;
    const SubMesh *         refSubMesh;
    int                     subMeshIndex;

    bool                    tangentsCalculated;         // is tangents calculated ?
    bool                    normalsCalculated;          // is normals calculated ?
    bool                    edgesCalculated;            // is edge calculated ?

    int                     numVerts;                   // mirrored vertices 스플릿팅 후에,
    VertexGenericLit *      verts;                      // verts 배열 뒷 부분에 스플릿팅된 vertex 들이 추가된다.
    int                     numMirroredVerts;           // numVerts 에 합산되어 있다 (numOriginalVerts = numVerts - numMirroredVerts)
    VertIndex *             mirroredVerts;              // 추가된 mirrored vertex 들의 original vertex index 배열
    DominantTri *           dominantTris;               // dominant triangles for each vertices

    int                     numIndexes;                 // number of triangle indexes
    VertIndex *             indexes;                    // counter clock-wise order triangle indexes
    Array<LOD>              lods{ 1 };                  // LOD 0 를 제외한 LOD 정보
                            
    int                     numEdges;                   // number of edges that is not including shared edges
    Edge *                  edges;                      // shared edges are not stored explicitly
    int32_t *               edgeIndexes;                // triangle edge indexes to edge indexes including negative index number

    int                     numJointWeights;            // verts 와 별개로 weight 배열을 따로 관리 (for CPU skinning)
    JointWeight *           jointWeights;               // weight information array
    Vec4 *                  jointWeightVerts;           // local vertex of weight.

    void *                  vertWeights;                // vertex weights for GPU skinning (numVerts * sizeof(VertexWeightX))
    bool                    gpuSkinningEnabled;
    int                     gpuSkinningVersionIndex;    // 0: VertexWeight1, 1: VertexWeight4, 2: VertexWeight8

    AABB                    aabb;                       // AABB in local space of mesh

    BufferCache *           vertexCache;
    BufferCache *           indexCache;
    mutable BufferCache *   lodIndexCaches = nullptr;   // 참조 메시가 소유하며, 해당 LOD 렌더링 시 지연 할당한다.
};

BE_INLINE SubMesh::SubMesh() {
    alloced = false;
    refSubMesh = nullptr;
}

BE_INLINE SubMesh::~SubMesh() {
    FreeSubMesh();
}

BE_INLINE int SubMesh::NumIndexes(int lodLevel) const {
    assert(lodLevel >= 0 && lodLevel < NumLODs());
    return lodLevel == 0 ? numIndexes : (refSubMesh ? refSubMesh->lods : lods)[lodLevel - 1].indexes.Count();
}

BE_INLINE const VertIndex *SubMesh::Indexes(int lodLevel) const {
    assert(lodLevel >= 0 && lodLevel < NumLODs());
    return lodLevel == 0 ? indexes : (refSubMesh ? refSubMesh->lods : lods)[lodLevel - 1].indexes.Ptr();
}

BE_INLINE float SubMesh::GetLODError(int level) const {
    assert(level >= 0 && level < NumLODs());
    return level == 0 ? 0.0f : (refSubMesh ? refSubMesh->lods : lods)[level - 1].error;
}

BE_INLINE int SubMesh::VertexWeightSize() const {
    if (!vertWeights) {
        return 0;
    }
    return gpuSkinningVersionIndex == 2 ? sizeof(VertexWeight8) : (gpuSkinningVersionIndex == 1 ? sizeof(VertexWeight4) : sizeof(VertexWeight1)); 
}

BE_INLINE int SubMesh::MaxVertexWeights() const {
    int size = VertexWeightSize();
    if (size == 0) {
        return 0;
    }
    return size == sizeof(VertexWeight8) ? 8 : (size == sizeof(VertexWeight4) ? 4 : 1);
}

BE_NAMESPACE_END
