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

#include "RHI/RHI.h"

class Mesh;
class RenderWorld;
class RenderBackend;
enum class MeshType : uint8_t;

/*
---------------------------------------------------------------------------------------------------

    Edge

    Represents an edge of a triangle, defined by two vertices.
    The vertices are stored in ascending order (from lower to higher index).
    An edge in this orientation is assigned a positive index,
    while the corresponding shared edge in the opposite orientation uses negative index.

      ---------- v0
     \          / \
      \   t1   /   \
       \      e     \
        \    /   t0  \
         \  /         \
          v1 ---------

---------------------------------------------------------------------------------------------------
*/

struct Edge {
    int32_t                 v[2];       ///< Vertex indices defining an edge. v[0] is always less than v[1]; the reverse edge is not stored.
    int32_t                 t[2];       ///< Indices of adjacent triangle. t[0] represents the CCW triangle.
};

class SubMesh {
    friend class Mesh;
    friend class MeshManager;
    friend class RenderWorld;
    friend class RenderBackend;

public:
    static constexpr int    MaxLODLevel = 127;               ///< LOD 0 을 포함하여 최대 128 단계를 지원한다.

    enum class Flag : uint8_t {
        None                = 0,
        NormalsCalculated   = BIT(0),   ///< Normals have been calculated.
        TangentsCalculated  = BIT(1),   ///< Tangents have been calculated.
        EdgesCalculated     = BIT(2)    ///< Edges have been calculated.
    };

    friend constexpr std::true_type EnableBitmaskEnum(Flag) noexcept { return {}; }

    ~SubMesh() { Purge(); }

    int32_t                 GetID() const { return subMeshId; }

    uint32_t                NumVerts() const { return numVerts; }
    BE1::VertexGenericLit * Verts() const { return verts; }

    uint32_t                NumIndexes(int lodLevel = 0) const;
    BE1::VertIndex *        Indexes() const { return indexes; }
    const BE1::VertIndex *  Indexes(int lodLevel) const;

                            /// LOD 0 는 원본이며, 모든 LOD 는 같은 vertex 와 skinning weights 를 공유한다.
    int                     NumLODs() const { return 1 + lods.Count(); }

                            /// 원본 대비 LOD 의 단순화 오차 추정값을 로컬 공간 단위로 반환한다.
                            /// 노멀, UV, 색상 등의 속성 오차도 포함하며 LOD 0 은 0 을 반환한다.
    float                   GetLODError(int level) const;

                            /// 메시의 LOD 를 생성한다. LOD 0 는 보존한다.
                            /// maxLevels 는 추가 LOD 의 최대 개수이며, 0 은 MaxLODLevel 까지 생성을 허용한다.
                            /// 양수도 MaxLODLevel 을 넘지 않도록 제한한다.
    void                    GenerateLODs(int maxLevels);

    int                     VertexWeightSize() const;
    int                     MaxVertexWeights() const;
    void *                  VertexWeights() const { return vertWeights; }

    const BE1::AABB &       GetBoundingBox() const { return boundingBox; }
    const BE1::Sphere &     GetBoundingSphere() const { return boundingSphere; }

                            /// Finds the edge index corresponding to the edge defined by vertex indexes v1 and v2.
                            /// If the given vertex indices are in descending order (v1 > v2), a negative value is returned.
                            /// Returns 0 if no valid edge was found.
                            /// NOTE: Edge data must be precomputed before calling this function.
    int                     FindEdge(int32_t v1, int32_t v2) const;

                            /// Check if this sub-mesh forms a closed surface.
                            /// All edges must have two adjacent triangles for a closed mesh.
                            /// NOTE: Edge data must be precomputed before calling this function.
    bool                    IsClosed() const;

                            /// Tests if this sub mesh intersect with the given line segment.
                            /// To use this function, edge information must be pre-calculated.
    bool                    IsIntersectLine(const BE1::Vec3 &p1, const BE1::Vec3 &p2, bool ignoreBackFace) const;

                            /// Ray intersection.
                            /// To use this function, edge information must be pre-calculated.
    bool                    IntersectRay(const BE1::Ray &ray, bool ignoreBackFace, float *hitDist = nullptr, BE1::Vec3 *hitNormal = nullptr) const;

                            /// Calculates the volume of the sub-mesh.
    float                   CalculateVolume() const;

                            /// Calculates the centroid (center of mass) of the sub-mesh.
    const BE1::Vec3         CalculateCentroid() const;

                            /// Calculates the inertia tensor of the sub-mesh given its centroid and mass.
    const BE1::Mat3         CalculateInertiaTensor(const BE1::Vec3 &centroid, float mass) const;

                            /// Optimizes triangle and vertex order before instantiation or GPU upload.
                            /// Invalidates edge data; recompute edges before using edge-dependent operations.
    void                    OptimizeIndexedTriangles();

                            /// 공유 vertex/weight buffer 와 요청한 LOD 의 index buffer 를 준비하고 해당 index buffer 를 반환한다.
                            /// 이미 생성된 버퍼는 재사용하며, 생성에 실패하면 nullptr 을 반환한다.
    RHI::IndexBuffer *      UploadStaticDataToGPU(int lodLevel = 0) const;

private:
    struct LOD {
        BE1::Array<BE1::VertIndex> indexes;
        float               error = 0.0f;   ///< 로컬 공간 단위의 속성 오차를 포함한 단순화 오차.
    };

    void                    Alloc(uint32_t numVerts, uint32_t numIndexes);
    void                    Purge();

                            /// Copies CPU sub-mesh data. The destination should be allocated but not uploaded to the GPU.
    void                    CopyFrom(const SubMesh *other);

                            /// Computes local-space bounds (AABB + bounding sphere).
    void                    ComputeBounds();

                            /// Computes area weighted average of the normals.
    void                    ComputeNormals();

                            /// Computes the dominant triangle for each vertex.
    void                    ComputeDominantTris();

    void                    ComputeTangents(bool includeNormals, bool useUnsmoothedTangents);

    void                    ComputeEdges();

    int32_t                 subMeshId = -1;                     ///< ID of the sub-mesh.
    uint32_t                numVerts = 0;                       ///< Number of vertices.
    BE1::VertexGenericLit * verts = nullptr;                    ///< Pointer to vertices.
    BE1::DominantTri *      dominantTris = nullptr;             ///< Dominant triangles for each vertex.
    uint32_t                numIndexes = 0;                     ///< Number of triangle indices.
    BE1::VertIndex *        indexes = nullptr;                  ///< Triangle indices in counter-clockwise order.
    BE1::Array<LOD>         lods{ 1 };                          ///< LOD 0 를 제외한 LOD 정보.

    uint32_t                numEdges = 0;                       ///< Number of edges that is not including shared edges.
    Edge *                  edges = nullptr;                    ///< Pointer to edges (shared edges are not stored explicitly).
    int32_t *               edgeIndexes = nullptr;              ///< Mapping from triangle edge indices to edge indices (negative values indicate shared edges).

    BE1::AABB               boundingBox = BE1::AABB::empty;     ///< Local axis-aligned bounding box (bounding box) for this sub-mesh.
    BE1::Sphere             boundingSphere = BE1::Sphere(BE1::Vec3::zero, 0.0f); ///< Local bounding sphere for this sub-mesh.

    void *                  vertWeights = nullptr;              ///< vertex weights for GPU skinning (numVerts * sizeof(VertexWeightX))
    int                     gpuSkinningVersionIndex;            ///< 0: VertexWeight1, 1: VertexWeight4, 2: VertexWeight8

    mutable RHI::VertexBuffer *vertexBuffer = nullptr;          ///< RHI vertex buffer.
    mutable RHI::VertexBuffer *vertexWeightBuffer = nullptr;    ///< RHI vertex weight buffer for GPU skinning.
    mutable RHI::IndexBuffer *indexBuffer = nullptr;            ///< RHI index buffer.
    mutable RHI::IndexBuffer **lodIndexBuffers = nullptr;       ///< LOD 0 를 제외한 index buffer. 추가 LOD 를 처음 업로드할 때 할당한다.

    Flag                    flags = Flag::None;
};

BE_INLINE uint32_t SubMesh::NumIndexes(int lodLevel) const {
    assert(lodLevel >= 0 && lodLevel < NumLODs());
    return lodLevel == 0 ? numIndexes : lods[lodLevel - 1].indexes.Count();
}

BE_INLINE const BE1::VertIndex *SubMesh::Indexes(int lodLevel) const {
    assert(lodLevel >= 0 && lodLevel < NumLODs());
    return lodLevel == 0 ? indexes : lods[lodLevel - 1].indexes.Ptr();
}

BE_INLINE float SubMesh::GetLODError(int level) const {
    assert(level >= 0 && level < NumLODs());
    return level == 0 ? 0.0f : lods[level - 1].error;
}

BE_INLINE int SubMesh::VertexWeightSize() const {
    if (!vertWeights) {
        return 0;
    }
    return gpuSkinningVersionIndex == 2 ? sizeof(BE1::VertexWeight8) : (gpuSkinningVersionIndex == 1 ? sizeof(BE1::VertexWeight4) : sizeof(BE1::VertexWeight1));
}

BE_INLINE int SubMesh::MaxVertexWeights() const {
    int size = VertexWeightSize();
    if (size == 0) {
        return 0;
    }
    return size == sizeof(BE1::VertexWeight8) ? 8 : (size == sizeof(BE1::VertexWeight4) ? 4 : 1);
}
