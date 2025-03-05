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

#include "RHI.h"

class Mesh;
class RenderWorld;
class RenderBackEnd;
enum class MeshType : uint8_t;

/*
-------------------------------------------------------------------------------
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
-------------------------------------------------------------------------------
*/
struct Edge {
    int32_t                 v[2];       ///< Vertex indices defining an edge. v[0] is always less than v[1]; the reverse edge is not stored.
    int32_t                 t[2];       ///< Indices of adjacent triangle. t[0] represents the CCW triangle.
};

enum class MeshType : uint8_t {
    Dummy,
    Static,         ///< Mesh data in a static vertex/index buffer.
    Dynamic,        ///< Mesh data in a dynamic vertex/index buffer.
    Skinned         ///< Mesh data used for skinning.
};

class SubMesh {
    friend class Mesh;
    friend class MeshManager;
    friend class RenderWorld;
    friend class RenderBackEnd;

public:
    enum class Flag : uint8_t {
        None                = 0,
        NormalsCalculated   = BIT(0),   ///< Normals have been calculated.
        TangentsCalculated  = BIT(1),   ///< Tangents have been calculated.
        EdgesCalculated     = BIT(2)    ///< Edges have been calculated.
    };

    ~SubMesh() { Purge(); }

    MeshType                GetType() const { return type; }

    uint32_t                NumVerts() const { return numVerts; }
    BE1::VertexGenericLit * Verts() const { return verts; }

    uint32_t                NumIndexes() const { return numIndexes; }
    BE1::VertIndex *        Indexes() const { return indexes; }

    const BE1::AABB &       GetAABB() const { return aabb; }

                            /// Finds the edge index corresponding to the edge defined by vertex indexes v1 and v2.
                            /// If the given vertex indices are in descending order (v1 > v2), a negative value is returned.
                            /// Returns 0 if no valid edge was found.
                            /// NOTE: Edge data must be precomputed before calling this function.
    int                     FindEdge(int32_t v1, int32_t v2) const;

                            /// Check if this sub-mesh forms a closed surface.
                            /// All edges must have two adjacent triangles for a closed mesh.
                            /// NOTE: Edge data must be precomputed before calling this function.
    bool                    IsClosed() const;

                            /// Calculates the volume of the sub-mesh.
    float                   CalculateVolume() const;

                            /// Calculates the centroid (center of mass) of the sub-mesh.
    const BE1::Vec3         CalculateCentroid() const;

                            /// Calculates the inertia tensor of the sub-mesh given its centroid and mass.
    const BE1::Mat3         CalculateInertiaTensor(const BE1::Vec3 &centroid, float mass) const;

    void                    UploadStaticDataToGPU();

private:
    void                    Alloc(MeshType meshType, uint32_t numVerts, uint32_t numIndexes);
    void                    Purge();

                            /// Copies sub-mesh data from another sub-mesh. The destination should already be allocated.
    void                    CopyFrom(const SubMesh *other);

                            /// Computes axis-aligned bounding box.
    void                    ComputeAABB();

                            /// Computes area weighted average of the normals.
    void                    ComputeNormals();

                            /// Computes the dominant triangle for each vertex.
    void                    ComputeDominantTris();

    void                    ComputeTangents(bool includeNormals, bool useUnsmoothedTangents);

    void                    ComputeEdges();

    int32_t                 subMeshId = -1;             ///< ID of the sub-mesh.
    uint32_t                numVerts = 0;               ///< Number of vertices.
    BE1::VertexGenericLit * verts = nullptr;            ///< Pointer to vertices.
    BE1::DominantTri *      dominantTris = nullptr;     ///< Dominant triangles for each vertex.
    uint32_t                numIndexes = 0;             ///< Number of triangle indices.
    BE1::VertIndex *        indexes = nullptr;          ///< Triangle indices in counter-clockwise order.
    uint32_t                numEdges = 0;               ///< Number of edges that is not including shared edges.
    Edge *                  edges = nullptr;            ///< Pointer to edges (shared edges are not stored explicitly).
    int32_t *               edgeIndexes = nullptr;      ///< Mapping from triangle edge indices to edge indices (negative values indicate shared edges).
    BE1::AABB               aabb = BE1::AABB::empty;    ///< Local axis-aligned bounding box (AABB) for this sub-mesh.

    RHI::VertexBuffer *     vertexBuffer = nullptr;     ///< GPU vertex buffer.
    RHI::IndexBuffer *      indexBuffer = nullptr;      ///< GPU index buffer.

    MeshType                type = MeshType::Dummy;
    Flag                    flags = Flag::None;
};

template<>
struct enable_bitmask_operators<SubMesh::Flag> {
    static const bool enable = true;
};
