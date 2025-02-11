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

#include "Precompiled.h"
#include "SubMesh.h"

void SubMesh::Alloc(MeshType newType, uint32_t numNewVerts, uint32_t numNewIndexes) {
    static int32_t subMeshIdCounter = 0;

    assert(newType != MeshType::Dummy);
    type = newType;
    subMeshId = subMeshIdCounter++;

    numVerts = numNewVerts;
    verts = (BE1::VertexGenericLit *)Mem_Alloc16(sizeof(BE1::VertexGenericLit) * numNewVerts);

    numIndexes = numNewIndexes;
    indexes = (BE1::VertIndex *)Mem_Alloc16(sizeof(BE1::VertIndex) * numNewIndexes);
}

void SubMesh::Purge() {
    if (type == MeshType::Dummy) {
        return;
    }

    if (vertexBuffer) {
        RHI::renderer->DestroyVertexBuffer(vertexBuffer);
        vertexBuffer = nullptr;
    }
    if (indexBuffer) {
        RHI::renderer->DestroyIndexBuffer(indexBuffer);
        indexBuffer = nullptr;
    }

    if (verts) {
        Mem_AlignedFree(verts);
        verts = nullptr;
    }
    if (indexes) {
        Mem_AlignedFree(indexes);
        indexes = nullptr;
    }
    if (dominantTris) {
        Mem_AlignedFree(dominantTris);
        dominantTris = nullptr;
    }
    if (edges) {
        Mem_AlignedFree(edges);
        edges = nullptr;
    }
    if (edgeIndexes) {
        Mem_AlignedFree(edgeIndexes);
        edgeIndexes = nullptr;
    }
}

void SubMesh::CopyFrom(const SubMesh *other) {
    if (!verts || !indexes) {
        BE_ERRLOG("SubMesh::CopyFrom: Should be allocated first");
        return;
    }

    if (numVerts != other->numVerts || numIndexes != other->numIndexes) {
        BE_ERRLOG("SubMesh::CopyFrom: different verts and indices count with source sub-mesh");
        return;
    }

    flags = other->flags;
    flags &= ~Flag::EdgesCalculated;

    BE1::simdProcessor->Memcpy(verts, other->verts, sizeof(verts[0]) * other->numVerts);
    BE1::simdProcessor->Memcpy(indexes, other->indexes, sizeof(indexes[0]) * other->numIndexes);
}

void SubMesh::ComputeAABB() {
    aabb.Clear();
    for (int i = 0; i < numVerts; i++) {
        aabb.AddPoint(verts[i].xyz);
    }
}

void SubMesh::ComputeNormals() {
    if (BE1::HasFlag(flags, Flag::NormalsCalculated)) {
        return;
    }

    BE1::Vec3 *tempNormals = (BE1::Vec3 *)Mem_Alloc16(numVerts * sizeof(BE1::Vec3));

    for (int i = 0; i < numVerts; i++) {
        tempNormals[i] = BE1::Vec3::zero;
    }

    for (int i = 0; i < numIndexes; i += 3) {
        int i0 = indexes[i];
        int i1 = indexes[i + 1];
        int i2 = indexes[i + 2];

        const BE1::VertexGenericLit &v0 = verts[i0];
        const BE1::VertexGenericLit &v1 = verts[i1];
        const BE1::VertexGenericLit &v2 = verts[i2];

        const BE1::Vec3 side0 = v1.xyz - v0.xyz;
        const BE1::Vec3 side1 = v2.xyz - v0.xyz;

        BE1::Vec3 faceNormal = side0.Cross(side1);

        tempNormals[i0] += faceNormal;
        tempNormals[i1] += faceNormal;
        tempNormals[i2] += faceNormal;
    }

    for (int i = 0; i < numVerts; i++) {
        tempNormals[i].Normalize();
        verts[i].SetNormal(tempNormals[i]);
    }

    Mem_AlignedFree(tempNormals);

    flags |= Flag::NormalsCalculated;
}

static void R_DeriveTangentsWithoutNormals(BE1::VertexGenericLit *verts, const int numVerts, const BE1::VertIndex *indexes, const int numIndexes) {
    int numTris = numIndexes / 3;

    BE1::Vec3 *triangleTangents = (BE1::Vec3 *)Mem_Alloc16(numTris * sizeof(BE1::Vec3));
    BE1::Vec3 *triangleBitangents = (BE1::Vec3 *)Mem_Alloc16(numTris * sizeof(BE1::Vec3));

    // Calculate tangent vectors for each face in isolation
    for (int i = 0; i < numIndexes; i += 3) {
        int triIndex = i / 3;

        int v0 = indexes[i + 0];
        int v1 = indexes[i + 1];
        int v2 = indexes[i + 2];

        const BE1::VertexGenericLit &a = verts[v0];
        const BE1::VertexGenericLit &b = verts[v1];
        const BE1::VertexGenericLit &c = verts[v2];

        const BE1::Vec2 aST = a.GetTexCoord();
        const BE1::Vec2 bST = b.GetTexCoord();
        const BE1::Vec2 cST = c.GetTexCoord();

        BE1::Vec3 baXYZ;
        baXYZ[0] = b.xyz[0] - a.xyz[0];
        baXYZ[1] = b.xyz[1] - a.xyz[1];
        baXYZ[2] = b.xyz[2] - a.xyz[2];

        BE1::Vec3 caXYZ;
        caXYZ[0] = c.xyz[0] - a.xyz[0];
        caXYZ[1] = c.xyz[1] - a.xyz[1];
        caXYZ[2] = c.xyz[2] - a.xyz[2];

        BE1::Vec2 baST;
        baST[0] = bST[0] - aST[0];
        baST[1] = bST[1] - aST[1];

        BE1::Vec2 caST;
        caST[0] = cST[0] - aST[0];
        caST[1] = cST[1] - aST[1];

        // area sign bit
        const float area = baST[0] * caST[1] - baST[1] * caST[0]; // determinant
        if (fabs(area) < 1e-20f) {
            triangleTangents[triIndex] = BE1::Vec3::zero;
            triangleBitangents[triIndex] = BE1::Vec3::zero;
            continue;
        }

        float inva = (area < 0.0f) ? -1.0f : 1.0f;  // was = 1.0f / area;

        BE1::Vec3 tangent;
        tangent[0] = inva * (baXYZ[0] * caST[1] - caXYZ[0] * baST[1]);
        tangent[1] = inva * (baXYZ[1] * caST[1] - caXYZ[1] * baST[1]);
        tangent[2] = inva * (baXYZ[2] * caST[1] - caXYZ[2] * baST[1]);
        tangent.Normalize();

        triangleTangents[triIndex] = tangent;

        BE1::Vec3 bitangent;
        bitangent[0] = inva * (caXYZ[0] * baST[0] - baXYZ[0] * caST[0]);
        bitangent[1] = inva * (caXYZ[1] * baST[0] - baXYZ[1] * caST[0]);
        bitangent[2] = inva * (caXYZ[2] * baST[0] - baXYZ[2] * caST[0]);
        bitangent.Normalize();

        triangleBitangents[triIndex] = bitangent;
    }

    BE1::Vec3 *vertexTangents = (BE1::Vec3 *)Mem_Alloc16(numVerts * sizeof(BE1::Vec3));
    BE1::Vec3 *vertexBitangents = (BE1::Vec3 *)Mem_Alloc16(numVerts * sizeof(BE1::Vec3));

    // Clear the tangents
    for (int i = 0; i < numVerts; i++) {
        vertexTangents[i] = BE1::Vec3::zero;
        vertexBitangents[i] = BE1::Vec3::zero;
    }

    // Sum up the neighbors
    for (int i = 0; i < numIndexes; i += 3) {
        int triIndex = i / 3;

        // For each vertex on this face
        for (int j = 0; j < 3; j++) {
            int index = indexes[i + j];

            vertexTangents[index] += triangleTangents[triIndex];
            vertexBitangents[index] += triangleBitangents[triIndex];
        }
    }

    // Project the summed vectors onto the normal plane and normalize.
    // The tangent vectors will not necessarily be orthogonal to each
    // other, but they will be orthogonal to the surface normal.
    for (int i = 0; i < numVerts; i++) {
        BE1::Vec3 normal = verts[i].GetNormal();
        normal.Normalize();

        vertexTangents[i].ProjectOnPlane(normal);
        vertexTangents[i].Normalize();

        vertexBitangents[i].ProjectOnPlane(normal);
        vertexBitangents[i].Normalize();
    }

    for (int i = 0; i < numVerts; i++) {
        verts[i].SetTangent(vertexTangents[i]);
        verts[i].SetBiTangent(vertexBitangents[i]);
    }

    Mem_AlignedFree(vertexTangents);
    Mem_AlignedFree(vertexBitangents);
    Mem_AlignedFree(triangleTangents);
    Mem_AlignedFree(triangleBitangents);
}

static void R_DeriveNormalsAndTangents(BE1::VertexGenericLit *verts, const int numVerts, const BE1::VertIndex *indexes, const int numIndexes) {
    BE1::Vec3 *vertexNormals = (BE1::Vec3 *)Mem_Alloc16(numVerts * sizeof(BE1::Vec3));
    BE1::Vec3 *vertexTangents = (BE1::Vec3 *)Mem_Alloc16(numVerts * sizeof(BE1::Vec3));
    BE1::Vec3 *vertexBitangents = (BE1::Vec3 *)Mem_Alloc16(numVerts * sizeof(BE1::Vec3));

    memset(vertexNormals, 0, numVerts * sizeof(BE1::Vec3));
    memset(vertexTangents, 0, numVerts * sizeof(BE1::Vec3));
    memset(vertexBitangents, 0, numVerts * sizeof(BE1::Vec3));

    for (int i = 0; i < numIndexes; i += 3) {
        int v0 = indexes[i + 0];
        int v1 = indexes[i + 1];
        int v2 = indexes[i + 2];

        const BE1::VertexGenericLit &a = verts[v0];
        const BE1::VertexGenericLit &b = verts[v1];
        const BE1::VertexGenericLit &c = verts[v2];

        const BE1::Vec2 aST = a.GetTexCoord();
        const BE1::Vec2 bST = b.GetTexCoord();
        const BE1::Vec2 cST = c.GetTexCoord();

        BE1::Vec3 baXYZ;
        baXYZ[0] = b.xyz[0] - a.xyz[0];
        baXYZ[1] = b.xyz[1] - a.xyz[1];
        baXYZ[2] = b.xyz[2] - a.xyz[2];

        BE1::Vec3 caXYZ;
        caXYZ[0] = c.xyz[0] - a.xyz[0];
        caXYZ[1] = c.xyz[1] - a.xyz[1];
        caXYZ[2] = c.xyz[2] - a.xyz[2];

        BE1::Vec2 baST;
        baST[0] = bST[0] - aST[0];
        baST[1] = bST[1] - aST[1];

        BE1::Vec2 caST;
        caST[0] = cST[0] - aST[0];
        caST[1] = cST[1] - aST[1];

        // normal
        BE1::Vec3 normal;
        normal[0] = caXYZ[2] * baXYZ[1] - caXYZ[1] * baXYZ[2];
        normal[1] = caXYZ[0] * baXYZ[2] - caXYZ[2] * baXYZ[0];
        normal[2] = caXYZ[1] * baXYZ[0] - caXYZ[0] * baXYZ[1];

        const float f0 = BE1::Math::InvSqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);

        normal.x *= f0;
        normal.y *= f0;
        normal.z *= f0;

        // area sign bit
        float area = baST[0] * caST[1] - baST[1] * caST[0]; // determinant
        uint32_t signBit = (*(uint32_t *)&area) & (1 << 31);

        // tangent
        BE1::Vec3 tangent;
        tangent[0] = baXYZ[0] * caST[1] - caXYZ[0] * baST[1];
        tangent[1] = baXYZ[1] * caST[1] - caXYZ[1] * baST[1];
        tangent[2] = baXYZ[2] * caST[1] - caXYZ[2] * baST[1];

        const float f1 = BE1::Math::InvSqrt(tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z);
        *(uint32_t *)&f1 ^= signBit;

        tangent.x *= f1;
        tangent.y *= f1;
        tangent.z *= f1;

        // bitangent
        BE1::Vec3 bitangent;
        bitangent[0] = caXYZ[0] * baST[0] - baXYZ[0] * caST[0];
        bitangent[1] = caXYZ[1] * baST[0] - baXYZ[1] * caST[0];
        bitangent[2] = caXYZ[2] * baST[0] - baXYZ[2] * caST[0];

        const float f2 = BE1::Math::InvSqrt(bitangent.x * bitangent.x + bitangent.y * bitangent.y + bitangent.z * bitangent.z);
        *(uint32_t *)&f2 ^= signBit;

        bitangent.x *= f2;
        bitangent.y *= f2;
        bitangent.z *= f2;

        vertexNormals[v0] += normal;
        vertexTangents[v0] += tangent;
        vertexBitangents[v0] += bitangent;

        vertexNormals[v1] += normal;
        vertexTangents[v1] += tangent;
        vertexBitangents[v1] += bitangent;

        vertexNormals[v2] += normal;
        vertexTangents[v2] += tangent;
        vertexBitangents[v2] += bitangent;
    }

    // Project the summed vectors onto the normal plane and normalize.
    // The tangent vectors will not necessarily be orthogonal to each
    // other, but they will be orthogonal to the surface normal.
    for (int i = 0; i < numVerts; i++) {
        const float normalScale = BE1::Math::InvSqrt(vertexNormals[i].LengthSqr());
        vertexNormals[i].x *= normalScale;
        vertexNormals[i].y *= normalScale;
        vertexNormals[i].z *= normalScale;

        vertexTangents[i].ProjectOnPlane(vertexNormals[i]);
        vertexBitangents[i].ProjectOnPlane(vertexNormals[i]);

        const float tangentScale = BE1::Math::InvSqrt(vertexTangents[i].LengthSqr());
        vertexTangents[i].x *= tangentScale;
        vertexTangents[i].y *= tangentScale;
        vertexTangents[i].z *= tangentScale;

        const float bitangentScale = BE1::Math::InvSqrt(vertexBitangents[i].LengthSqr());
        vertexBitangents[i].x *= bitangentScale;
        vertexBitangents[i].y *= bitangentScale;
        vertexBitangents[i].z *= bitangentScale;
    }

    // compress the normals and tangents
    for (int i = 0; i < numVerts; i++) {
        verts[i].SetNormal(vertexNormals[i]);
        verts[i].SetTangent(vertexTangents[i]);
        verts[i].SetBiTangent(vertexBitangents[i]);
    }

    Mem_AlignedFree(vertexNormals);
    Mem_AlignedFree(vertexTangents);
    Mem_AlignedFree(vertexBitangents);
}

// Derives the normal and orthogonal tangent vectors for the triangle vertices.
// For each vertex the normal and tangent vectors are derived from a single dominant triangle.
//#define DERIVE_UNSMOOTHED_BITANGENT

static void R_DeriveUnsmoothedNormalsAndTangents(BE1::VertexGenericLit *verts, const BE1::DominantTri *dominantTris, const int numVerts) {
    for (int i = 0; i < numVerts; i++) {
        float d0, d1, d2, d3, d4;
        float d5, d6, d7, d8, d9;
        float s0, s1, s2;
        float n0, n1, n2;
        float t0, t1, t2;
        float t3, t4, t5;

        const BE1::DominantTri &dt = dominantTris[i];

        BE1::VertexGenericLit *a = verts + i;
        BE1::VertexGenericLit *b = verts + dt.v2;
        BE1::VertexGenericLit *c = verts + dt.v3;

        const BE1::Vec2 aST = a->GetTexCoord();
        const BE1::Vec2 bST = b->GetTexCoord();
        const BE1::Vec2 cST = c->GetTexCoord();

        d0 = b->xyz[0] - a->xyz[0];
        d1 = b->xyz[1] - a->xyz[1];
        d2 = b->xyz[2] - a->xyz[2];
        d3 = bST[0] - aST[0];
        d4 = bST[1] - aST[1];

        d5 = c->xyz[0] - a->xyz[0];
        d6 = c->xyz[1] - a->xyz[1];
        d7 = c->xyz[2] - a->xyz[2];
        d8 = cST[0] - aST[0];
        d9 = cST[1] - aST[1];

        s0 = dt.normalizationScale[0];
        s1 = dt.normalizationScale[1];
        s2 = dt.normalizationScale[2];

        n0 = s2 * (d7 * d1 - d6 * d2);
        n1 = s2 * (d5 * d2 - d7 * d0);
        n2 = s2 * (d6 * d0 - d5 * d1);

        t0 = s0 * (d0 * d9 - d4 * d5);
        t1 = s0 * (d1 * d9 - d4 * d6);
        t2 = s0 * (d2 * d9 - d4 * d7);

#ifndef DERIVE_UNSMOOTHED_BITANGENT
        t3 = s1 * (d3 * d5 - d0 * d8);
        t4 = s1 * (d3 * d6 - d1 * d8);
        t5 = s1 * (d3 * d7 - d2 * d8);
#else
        t3 = s1 * (n2 * t1 - n1 * t2);
        t4 = s1 * (n0 * t2 - n2 * t0);
        t5 = s1 * (n1 * t0 - n0 * t1);
#endif

        a->SetNormal(n0, n1, n2);
        a->SetTangent(t0, t1, t2);
        a->SetBiTangent(t3, t4, t5);
    }
}

void SubMesh::ComputeDominantTris() {
    if (!dominantTris) {
        dominantTris = (BE1::DominantTri *)Mem_Alloc16(sizeof(BE1::DominantTri) * numVerts);
    }

    // Compute the dominant triangles for each vertices
    // The dominant triangle should have most larger surface area among the adjacent triangles of a vertex
    for (int i = 0; i < numVerts; i++) {
        float dominantTriArea = -1.0f;
        BE1::VertIndex dominantTriVertex2 = -1;
        BE1::VertIndex dominantTriVertex3 = -1;

        for (int j = 0; j < numIndexes; j += 3) {
            if (indexes[j] == i || indexes[j + 1] == i || indexes[j + 2] == i) {
                BE1::Vec3 p0 = verts[indexes[j + 1]].xyz - verts[indexes[j]].xyz;
                BE1::Vec3 p1 = verts[indexes[j + 2]].xyz - verts[indexes[j]].xyz;

                // Compute the surface area of a triangle
                //float area = p0.Cross(p1).Length() * 0.5f; // original version
                float area = p0.Cross(p1).LengthSqr(); // low cost version

                if (area > dominantTriArea) {
                    dominantTriArea = area;

                    if (indexes[j] == i) {
                        dominantTriVertex2 = indexes[j + 1];
                        dominantTriVertex3 = indexes[j + 2];
                    } else if (indexes[j + 1] == i) {
                        dominantTriVertex2 = indexes[j + 2];
                        dominantTriVertex3 = indexes[j];
                    } else {
                        dominantTriVertex2 = indexes[j];
                        dominantTriVertex3 = indexes[j + 1];
                    }
                }
            }
        }

        // Now dominantTri is [i, dominantTriVertex2, dominantTriVertex3]
        if (dominantTriVertex2 == -1 || dominantTriVertex3 == -1) {
            BE_FATALERROR("SubMesh::ComputeDominantTris: dominant triangle is not exist");
        }

        dominantTris[i].v2 = dominantTriVertex2;
        dominantTris[i].v3 = dominantTriVertex3;

        BE1::VertexGenericLit *a = &verts[i];
        BE1::VertexGenericLit *b = &verts[dominantTriVertex2];
        BE1::VertexGenericLit *c = &verts[dominantTriVertex3];

        const BE1::Vec2 aST = a->GetTexCoord();
        const BE1::Vec2 bST = b->GetTexCoord();
        const BE1::Vec2 cST = c->GetTexCoord();

        BE1::Vec3 side0 = b->xyz - a->xyz;
        BE1::Vec3 side1 = c->xyz - a->xyz;

        float ds1 = bST[0] - aST[0];
        float dt1 = bST[1] - aST[1];

        float ds2 = cST[0] - aST[0];
        float dt2 = cST[1] - aST[1];

        // We just need determinant sign because tangents vectors should be normalized
        float det = ds1 * dt2 - ds2 * dt1;
        uint32_t signBit = (*(uint32_t *)&det) & (1 << 31);

        BE1::Vec3 t0;
        t0.x = dt2 * side0.x - dt1 * side1.x;
        t0.y = dt2 * side0.y - dt1 * side1.y;
        t0.z = dt2 * side0.z - dt1 * side1.z;

        const float f0 = BE1::Math::InvSqrt(t0.LengthSqr());
        *(uint32_t *)&f0 ^= signBit;
        dominantTris[i].normalizationScale[0] = f0;

        BE1::Vec3 t1;
        t1.x = ds1 * side1.x - ds2 * side0.x;
        t1.y = ds1 * side1.y - ds2 * side0.y;
        t1.z = ds1 * side1.z - ds2 * side0.z;

        const float f1 = BE1::Math::InvSqrt(t1.LengthSqr());
        *(uint32_t *)&f1 ^= signBit;
        dominantTris[i].normalizationScale[1] = f1;

        BE1::Vec3 n;
        n.x = side0.y * side1.z - side0.z * side1.y;
        n.y = side0.z * side1.x - side0.x * side1.z;
        n.z = side0.x * side1.y - side0.y * side1.x;

        const float f2 = BE1::Math::InvSqrt(n.LengthSqr());
        dominantTris[i].normalizationScale[2] = f2;
    }
}

void SubMesh::ComputeTangents(bool includeNormals, bool useUnsmoothedTangents) {
    if (BE1::HasFlag(flags, Flag::TangentsCalculated)) {
        return;
    }

    if (useUnsmoothedTangents) {
        ComputeDominantTris();
        R_DeriveUnsmoothedNormalsAndTangents(verts, dominantTris, numVerts);
        flags |= Flag::NormalsCalculated;
    } else if (!includeNormals) {
        R_DeriveTangentsWithoutNormals(verts, numVerts, (const BE1::VertIndex *)indexes, numIndexes);
    } else {
        R_DeriveNormalsAndTangents(verts, numVerts, (const BE1::VertIndex *)indexes, numIndexes);
        flags |= Flag::NormalsCalculated;
    }

    flags |= Flag::TangentsCalculated;
}

// Assumes each edge is shared by at most two triangles.
void SubMesh::ComputeEdges() {
    if (BE1::HasFlag(flags, Flag::EdgesCalculated)) {
        return;
    }

    if (numVerts == 0 || numIndexes == 0) {
        assert(0);
        return;
    }

    // Temporary edge buffer to compute real 'edges'.
    // Maximum edge count is same as index count (if all triangles are separated).
    // But we need one more space for 0'th edge for dummy (not used).
    Edge *tempEdges = (Edge *)Mem_Alloc16((numIndexes + 1) * sizeof(Edge));
    memset(&tempEdges[0], 0, sizeof(Edge));

    // Edge's vertex index v0 to the edge index table.
    int32_t *indexToEdgeIndex = (int32_t *)Mem_Alloc16(numVerts * sizeof(indexToEdgeIndex[0]));
    memset(indexToEdgeIndex, -1, numVerts * sizeof(int32_t));

    // Vertices might have many edges.
    int32_t *edgeChain = (int32_t *)Mem_Alloc16((numIndexes + 1) * sizeof(edgeChain[0]));

    // Edge indexes.
    edgeIndexes = (int32_t *)Mem_Alloc16(numIndexes * sizeof(edgeIndexes[0]));

    Edge triEdges[3];
    int numTempEdges = 1;
    int numDisjunctiveEdges = 0;

    for (int i = 0; i < numIndexes; i += 3) {
        // Vertex indexes for a current triangle.
        const BE1::VertIndex *indexesForTri = indexes + i;
        // Current triangle index.
        const int32_t triIndex = i / 3;

        const int32_t i0 = indexesForTri[0];
        const int32_t i1 = indexesForTri[1];
        const int32_t i2 = indexesForTri[2];

        // Ordering to small index comes first.
        int32_t s = INT32_SIGNBITSET(i1 - i0);
        triEdges[0].v[0] = indexesForTri[s];
        triEdges[0].v[1] = indexesForTri[s ^ 1];
        s = INT32_SIGNBITSET(i2 - i1) + 1;
        triEdges[1].v[0] = indexesForTri[s];
        triEdges[1].v[1] = indexesForTri[s ^ 3];
        s = INT32_SIGNBITSET(i2 - i0) << 1;
        triEdges[2].v[0] = indexesForTri[s];
        triEdges[2].v[1] = indexesForTri[s ^ 2];

        for (int j = 0; j < 3; j++) {
            Edge &edge = triEdges[j];

            const int32_t v0 = edge.v[0]; // current edge's smaller vertex index
            const int32_t v1 = edge.v[1]; // current edge's bigger vertex index

            const unsigned int order = (v0 == indexesForTri[j] ? 0 : 1);

            // Find the shared edge.
            int currentEdgeIndex = indexToEdgeIndex[v0];
            while (currentEdgeIndex >= 0) {
                if (tempEdges[currentEdgeIndex].v[1] == v1) {
                    break;
                }
                currentEdgeIndex = edgeChain[currentEdgeIndex];
            }

            // Add new edge if no shared edge is found or two edges are already shared.
            if (currentEdgeIndex < 0 || tempEdges[currentEdgeIndex].t[order] != -1) {
                if (currentEdgeIndex >= 0) {
                    numDisjunctiveEdges++;
                }

                // Add an edge to the temporary edge buffer.
                edge.t[0] = edge.t[1] = -1;
                currentEdgeIndex = numTempEdges;
                tempEdges[numTempEdges++] = edge;

                // Update edge chain for later use.
                edgeChain[currentEdgeIndex] = indexToEdgeIndex[v0];
                indexToEdgeIndex[v0] = currentEdgeIndex;
            }

            // Update a triangle index of an edge.
            //assert(tempEdges[currentEdgeIndex].t[order] == -1);
            tempEdges[currentEdgeIndex].t[order] = triIndex;

            // Update an edge index.
            edgeIndexes[i + j] = (order == 0 ? currentEdgeIndex : -currentEdgeIndex);
        }
    }

    // Warning output for two or more shared edges.
    if (numDisjunctiveEdges > 0) {
        BE_WARNLOG("%i disjunctive edges found\n", numDisjunctiveEdges);
    }

    numEdges = numTempEdges;
    edges = (Edge *)Mem_Alloc16(sizeof(Edge) * numEdges);
    BE1::simdProcessor->Memcpy(edges, tempEdges, sizeof(Edge) * numEdges);

    // Cleans up temporary memory
    Mem_AlignedFree(indexToEdgeIndex);
    Mem_AlignedFree(edgeChain);
    Mem_AlignedFree(tempEdges);

    flags |= Flag::EdgesCalculated;
}

int SubMesh::FindEdge(int32_t v1, int32_t v2) const {
    assert(v1 != v2);

    if (!BE1::HasFlag(flags, Flag::EdgesCalculated)) {
        return false;
    }

    bool ascendingOrder = v1 < v2;
    int32_t minVertexIndex, maxVertexIndex;
    if (ascendingOrder) {
        minVertexIndex = v1;
        maxVertexIndex = v2;
    } else {
        minVertexIndex = v2;
        maxVertexIndex = v1;
    }

    for (int edgeIndex = 1; edgeIndex < numEdges; edgeIndex++) {
        const Edge *edge = &edges[edgeIndex];

        if (edge->v[0] == minVertexIndex &&
            edge->v[1] == maxVertexIndex) {
            // Returns negative edge index if and if only first vertex index is larger than second one.
            return ascendingOrder ? edgeIndex : -edgeIndex;
        }
    }
    return 0;
}

bool SubMesh::IsClosed() const {
    if (!BE1::HasFlag(flags, Flag::EdgesCalculated)) {
        return false;
    }

    // All the edges should have two adjacent triangles if the mesh is closed
    for (int edgeIndex = 1; edgeIndex < numEdges; edgeIndex++) {
        const Edge *edge = &edges[edgeIndex];

        if (edge->t[0] < 0 ||
            edge->t[1] < 0) {
            return false;
        }
    }
    return true;
}

float SubMesh::CalculateVolume() const {
    assert(IsClosed());

    float intg = 0;

    for (int i = 0; i < numIndexes; i += 3) {
        const BE1::Vec3 &a = verts[indexes[i]].xyz;
        const BE1::Vec3 &b = verts[indexes[i + 1]].xyz;
        const BE1::Vec3 &c = verts[indexes[i + 2]].xyz;

        const BE1::Vec3 side0 = b - a;
        const BE1::Vec3 side1 = c - a;

        const float crx = side0.y * side1.z - side0.z * side1.y;

        intg += crx * (a.x + b.x + c.x);
    }

    return intg / 6.0f;
}

static void Moment01SubExpressions(float w0, float w1, float w2, float &f1, float &f2) {
    const float temp0 = w0 + w1;
    const float temp1 = w0 * w0;
    const float temp2 = temp1 + w1 * temp0;
    f1 = temp0 + w2;
    f2 = temp2 + w2 * f1;
}

const BE1::Vec3 SubMesh::CalculateCentroid() const {
    assert(IsClosed());

#if defined(ENABLE_SIMD4_INTRIN)
    const simd4f multipliers = { 1.0f / 6.0f, 1.0f / 24.0f, 1.0f / 24.0f, 1.0f / 24.0f };
    const simd4b mask = { false, true, true, true };

    simd4f mintg = setzero_ps();

    for (int i = 0; i < numIndexes; i += 3) {
        simd4f a = loadu_ps(verts[indexes[i]].xyz);
        simd4f b = loadu_ps(verts[indexes[i + 1]].xyz);
        simd4f c = loadu_ps(verts[indexes[i + 2]].xyz);

        simd4f side0 = b - a;
        simd4f side1 = c - a;

        simd4f cr = BE1::cross_ps(side0, side1);

        simd4f temp0 = a + b;
        simd4f temp1 = a * a;
        simd4f temp2 = madd_ps(b, temp0, temp1);

        simd4f f1 = c + temp0;
        simd4f f2 = madd_ps(c, f1, temp2);

        temp0 = shuffle_ps<0, 0, 1, 2>(cr);
        temp1 = select_ps(f1, shuffle_ps<0, 0, 1, 2>(f2), mask);

        mintg = madd_ps(temp0, temp1, mintg);
    }

    mintg = mintg * multipliers;

    ALIGN_AS16 float intg[4];
    store_ps(mintg, intg);

    const float invVolume = 1.0f / intg[0];

    BE1::Vec3 centroid;
    centroid.x = intg[1] * invVolume;
    centroid.y = intg[2] * invVolume;
    centroid.z = intg[3] * invVolume;

    return centroid;
#else
    float intg[4] = { 0, 0, 0, 0 };
    const float multipliers[2] = { 1.0f / 6.0f, 1.0f / 24.0f };
    float f1x, f1y, f1z, f2x, f2y, f2z;

    for (int i = 0; i < numIndexes; i += 3) {
        const BE1::Vec3 &a = verts[indexes[i]].xyz;
        const BE1::Vec3 &b = verts[indexes[i + 1]].xyz;
        const BE1::Vec3 &c = verts[indexes[i + 2]].xyz;

        const BE1::Vec3 side0 = b - a;
        const BE1::Vec3 side1 = c - a;

        const BE1::Vec3 cr = side0.Cross(side1);

        Moment01SubExpressions(a.x, b.x, c.x, f1x, f2x);
        Moment01SubExpressions(a.y, b.y, c.y, f1y, f2y);
        Moment01SubExpressions(a.z, b.z, c.z, f1z, f2z);

        intg[0] += cr.x * f1x;
        intg[1] += cr.x * f2x;
        intg[2] += cr.y * f2y;
        intg[3] += cr.z * f2z;
    }

    intg[0] *= multipliers[0];
    intg[1] *= multipliers[1];
    intg[2] *= multipliers[1];
    intg[3] *= multipliers[1];

    const float invVolume = 1.0f / intg[0];

    BE1::Vec3 centroid;
    centroid.x = intg[1] * invVolume;
    centroid.y = intg[2] * invVolume;
    centroid.z = intg[3] * invVolume;

    return centroid;
#endif
}

static void Moment012SubExpressions(float w0, float w1, float w2, float &f1, float &f2, float &f3, float &g0, float &g1, float &g2) {
    float temp0 = w0 + w1;
    float temp1 = w0 * w0;
    float temp2 = temp1 + w1 * temp0;
    f1 = temp0 + w2;
    f2 = temp2 + w2 * f1;
    f3 = w0 * temp1 + w1 * temp2 + w2 * f2;
    g0 = f2 + w0 * (f1 + w0);
    g1 = f2 + w1 * (f1 + w1);
    g2 = f2 + w2 * (f1 + w2);
}

const BE1::Mat3 SubMesh::CalculateInertiaTensor(const BE1::Vec3 &centroid, float mass) const {
    assert(IsClosed());

#if defined(ENABLE_SIMD4_INTRIN)
    const simd4f multipliers1 = { 1.0f / 60.0f, 1.0f / 60.0f, 1.0f / 60.0f, 0.0f };
    const simd4f multipliers2 = { 1.0f / 120.0f, 1.0f / 120.0f, 1.0f / 120.0f, 0.0f };

    simd4f mintg1 = setzero_ps();
    simd4f mintg2 = setzero_ps();

    for (int i = 0; i < numIndexes; i += 3) {
        simd4f a = loadu_ps(verts[indexes[i]].xyz);
        simd4f b = loadu_ps(verts[indexes[i + 1]].xyz);
        simd4f c = loadu_ps(verts[indexes[i + 2]].xyz);

        simd4f side0 = b - a;
        simd4f side1 = c - a;

        simd4f cr = BE1::cross_ps(side0, side1);

        simd4f temp0 = a + b;
        simd4f temp1 = a * a;
        simd4f temp2 = madd_ps(b, temp0, temp1);

        simd4f f1 = c + temp0;
        simd4f f2 = madd_ps(c, f1, temp2);
        simd4f f3 = a * temp1 + b * temp2 + c * f2;
        simd4f g0 = madd_ps(f1 + a, a, f2);
        simd4f g1 = madd_ps(f1 + b, b, f2);
        simd4f g2 = madd_ps(f1 + c, c, f2);

        temp0 = shuffle_ps<1, 2, 0, 3>(a) * g0;
        temp1 = shuffle_ps<1, 2, 0, 3>(b) * g1;
        temp2 = shuffle_ps<1, 2, 0, 3>(c) * g2;

        mintg1 = madd_ps(cr, f3, mintg1);
        mintg2 = cr * (temp0 + temp1 + temp2);
    }

    mintg1 = mintg1 * multipliers1;
    mintg2 = mintg2 * multipliers2;

    ALIGN_AS16 float intg1[4];
    ALIGN_AS16 float intg2[4];
    store_ps(mintg1, intg1);
    store_ps(mintg2, intg2);

    BE1::Mat3 inertia;
    inertia[0][0] = intg1[1] + intg1[2] - mass * (centroid.y * centroid.y + centroid.z * centroid.z);
    inertia[1][1] = intg1[0] + intg1[2] - mass * (centroid.z * centroid.z + centroid.x * centroid.x);
    inertia[2][2] = intg1[0] + intg1[1] - mass * (centroid.x * centroid.x + centroid.y * centroid.y);
    inertia[0][1] = -(intg2[0] - mass * centroid.x * centroid.y);
    inertia[1][2] = -(intg2[1] - mass * centroid.y * centroid.z);
    inertia[0][2] = -(intg2[2] - mass * centroid.z * centroid.x);
    inertia[1][0] = inertia[0][1];
    inertia[2][0] = inertia[0][2];
    inertia[2][1] = inertia[1][2];

    return inertia;
#else
    const float multipliers[2] = { 1.0f / 60.0f, 1.0f / 120.0f };
    float intg[10] = { 0, };
    float f1x, f1y, f1z, f2x, f2y, f2z, f3x, f3y, f3z;
    float g0x, g0y, g0z, g1x, g1y, g1z, g2x, g2y, g2z;

    for (int i = 0; i < numIndexes; i += 3) {
        const BE1::Vec3 &a = verts[indexes[i]].xyz;
        const BE1::Vec3 &b = verts[indexes[i + 1]].xyz;
        const BE1::Vec3 &c = verts[indexes[i + 2]].xyz;

        const BE1::Vec3 side0 = b - a;
        const BE1::Vec3 side1 = c - a;

        const BE1::Vec3 cr = side0.Cross(side1);

        Moment012SubExpressions(a.x, b.x, c.x, f1x, f2x, f3x, g0x, g1x, g2x);
        Moment012SubExpressions(a.y, b.y, c.y, f1y, f2y, f3y, g0y, g1y, g2y);
        Moment012SubExpressions(a.z, b.z, c.z, f1z, f2z, f3z, g0z, g1z, g2z);

        intg[4] += cr.x * f3x;
        intg[5] += cr.y * f3y;
        intg[6] += cr.z * f3z;
        intg[7] += cr.x * (a.y * g0x + b.y * g1x + c.y * g2x);
        intg[8] += cr.y * (a.z * g0y + b.z * g1y + c.z * g2y);
        intg[9] += cr.z * (a.x * g0z + b.x * g1z + c.x * g2z);
    }

    intg[4] *= multipliers[0];
    intg[5] *= multipliers[0];
    intg[6] *= multipliers[0];
    intg[7] *= multipliers[1];
    intg[8] *= multipliers[1];
    intg[9] *= multipliers[1];

    BE1::Mat3 inertia;
    inertia[0][0] = intg[5] + intg[6] - mass * (centroid.y * centroid.y + centroid.z * centroid.z);
    inertia[1][1] = intg[4] + intg[6] - mass * (centroid.z * centroid.z + centroid.x * centroid.x);
    inertia[2][2] = intg[4] + intg[5] - mass * (centroid.x * centroid.x + centroid.y * centroid.y);
    inertia[0][1] = -(intg[7] - mass * centroid.x * centroid.y);
    inertia[1][2] = -(intg[8] - mass * centroid.y * centroid.z);
    inertia[0][2] = -(intg[9] - mass * centroid.z * centroid.x);
    inertia[1][0] = inertia[0][1];
    inertia[2][0] = inertia[0][2];
    inertia[2][1] = inertia[1][2];

    return inertia;
#endif
}

void SubMesh::UploadStaticDataToGPU() {
    if (!vertexBuffer) {
        vertexBuffer = RHI::renderer->CreateVertexBuffer(RHI::BufferUsage::Default, sizeof(verts[0]), numVerts, verts);
    }
    if (!indexBuffer) {
        indexBuffer = RHI::renderer->CreateIndexBuffer(RHI::BufferUsage::Default, sizeof(indexes[0]), numIndexes, indexes);
    }
}
