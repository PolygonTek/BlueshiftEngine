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

    SubMesh

-------------------------------------------------------------------------------
*/

#include "Core/Vertex.h"

class MeshImporter;

BE_NAMESPACE_BEGIN

/// Edge
/// 
/// Represents a line segment of a triangle.
/// The vertex index represents the line segment from small to large.
/// Edges in this direction use positive indexes, and shared opposite edges use negative indexes.
struct Edge {
/*
      ------ v1
            / \
      t0   /   \
   \      e     \
    \    /   t1  \
     \  /        
      v0 ------
*/
    union {
        struct {
            int32_t         firstVertexIndex;
            int32_t         secondVertexIndex;
            int32_t         firstTriangleIndex;
            int32_t         secondTriangleIndex;
        };
        int32_t             v[2];               ///< vertex indexes to define an edge. v[0] is always less than v[1]. opposite direction edge is not stored in the sub mesh.
        int32_t             t[2];               ///< adjacent triangle indexes. t[0] is a CCW triangle.
    };
};

/// Joint weight
struct JointWeight {
    int32_t                 jointMatOffset;     ///< offset of matrix array referenced by weight index.
    int32_t                 nextVertOffset;     ///< 0 means the same vertex.
};

struct BufferCache;

class Material;

class SubMesh {
    friend class Mesh;
    friend class MeshManager;
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

    int                     NumVerts() const { return numVerts; }
    int                     NumOriginalVerts() const { return numVerts - numMirroredVerts; }

    VertexGenericLit *      Verts() const { return verts; }

    int                     NumIndexes() const { return numIndexes; }
    TriIndex *              Indexes() const { return indexes; }

    int                     VertexWeightSize() const;
    int                     MaxVertexWeights() const;
    void *                  VertexWeights() const { return vertWeights; }

                            /// Finds a edge index by two vertex indexes.
                            /// if v1 is larger than v2, negative number will be returned.
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
    bool                    IntersectRay(const Ray &ray, bool ignoreBackFace, float *hitDist = nullptr) const;

    const AABB &            GetAABB() const { return aabb; }

                            // Compute mass properties (useful only for closed mesh)
    float                   ComputeVolume() const;
    const Vec3              ComputeCentroid() const;
    const Mat3              ComputeInertiaTensor(const Vec3 &centroid, float mass) const;

    void                    OptimizeIndexedTriangles();

    bool                    IsGpuSkinning() const { return useGpuSkinning; }

    void                    CacheStaticDataToGpu();
    void                    CacheDynamicDataToGpu(const Mat3x4 *joints, const Material *material);

private:
    void                    AllocSubMesh(int numVerts, int numIndexes);
    void                    AllocInstantiatedSubMesh(const SubMesh *refMesh, int meshType);
    void                    FreeSubMesh();

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
    TriIndex *              mirroredVerts;              // 추가된 mirrored vertex 들의 original vertex index 배열
    DominantTri *           dominantTris;               // dominant triangles for each vertices

    int                     numIndexes;                 // number of triangle indexes
    TriIndex *              indexes;                    // counter clock-wise order triangle indexes
                            
    int                     numEdges;                   // number of edges that is not including shared edges
    Edge *                  edges;                      // shared edges are not stored explicitly
    int32_t *               edgeIndexes;                // triangle edge indexes to edge indexes including negative index number

    int                     numJointWeights;            // verts 와 별개로 weight 배열을 따로 관리 (for CPU skinning)
    JointWeight *           jointWeights;               // weight information array
    Vec4 *                  jointWeightVerts;           // local vertex of weight.

    void *                  vertWeights;                // vertex weights for GPU skinning (numVerts * sizeof(VertexWeightX))
    bool                    useGpuSkinning;
    int                     gpuSkinningVersionIndex;    // 0: VertexWeight1, 1: VertexWeight4, 2: VertexWeight8

    AABB                    aabb;                       // AABB in local submesh space

    BufferCache *           vertexCache;
    BufferCache *           indexCache;
};

BE_INLINE SubMesh::SubMesh() {
    alloced = false;
}

BE_INLINE SubMesh::~SubMesh() {
    FreeSubMesh();
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
