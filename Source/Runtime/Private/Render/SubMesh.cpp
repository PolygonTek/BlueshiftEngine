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

#include "Precompiled.h"
#include "Render/Render.h"
#include "RenderInternal.h"
#include "SIMD/SIMD.h"
#include "Core/Heap.h"
#include "NvTriStrip.h"

BE_NAMESPACE_BEGIN

int SubMesh::Allocated() const {
    int size = 0;

    if (verts) {
        size += sizeof(verts[0]) * numVerts;
    }
    if (indexes) {
        size += sizeof(indexes[0]) * numIndexes;
    }
    if (dominantTris) {
        size += sizeof(dominantTris[0]) * numVerts;
    }
    if (edges) {
        size += sizeof(edges[0]) * numEdges;
    }
    if (vertexCache) {
        size += sizeof(BufferCache);
    }
    if (indexCache) {
        size += sizeof(BufferCache);
    }

    return size;
}

void SubMesh::AllocSubMesh(int numVerts, int numIndexes) {
    static int subMeshCounter = 0;

    this->alloced                   = true;
    this->normalsCalculated         = false;
    this->tangentsCalculated        = false;
    this->edgesCalculated           = false;

    this->type                      = Mesh::Type::Reference;
    this->refSubMesh                = nullptr;
    this->subMeshIndex              = subMeshCounter++;

    this->numVerts                  = numVerts;
    this->verts                     = (VertexGenericLit *)Mem_Alloc16(sizeof(VertexGenericLit) * numVerts);
    this->numMirroredVerts          = 0;
    this->mirroredVerts             = nullptr;

    this->numIndexes                = numIndexes;
    this->indexes                   = (TriIndex *)Mem_Alloc16(sizeof(TriIndex) * numIndexes);

    this->dominantTris              = nullptr;
    this->numEdges                  = 0;
    this->edges                     = nullptr;
    this->edgeIndexes               = nullptr;

    this->numJointWeights           = 0;
    this->jointWeights              = nullptr;
    this->jointWeightVerts          = nullptr;

    this->vertWeights               = nullptr;
    this->useGpuSkinning            = false;
    this->gpuSkinningVersionIndex   = 0;

    this->vertexCache               = (BufferCache *)Mem_ClearedAlloc(sizeof(BufferCache));
    this->indexCache                = (BufferCache *)Mem_ClearedAlloc(sizeof(BufferCache));
}

void SubMesh::AllocInstantiatedSubMesh(const SubMesh *ref, int meshType) {
    assert(ref->type == Mesh::Type::Reference);

    this->alloced                   = true;
    this->normalsCalculated         = ref->normalsCalculated;
    this->tangentsCalculated        = ref->tangentsCalculated;
    this->edgesCalculated           = ref->edgesCalculated;

    this->type                      = meshType;
    this->refSubMesh                = static_cast<const SubMesh *>(ref);
    this->subMeshIndex              = refSubMesh->subMeshIndex;

    this->numVerts                  = ref->numVerts;
    this->numMirroredVerts          = ref->numMirroredVerts;
    this->mirroredVerts             = ref->mirroredVerts;

    this->numIndexes                = ref->numIndexes;
    this->indexes                   = ref->indexes;

    this->dominantTris              = ref->dominantTris;
    this->numEdges                  = ref->numEdges;
    this->edges                     = ref->edges;
    this->edgeIndexes               = ref->edgeIndexes;

    this->numJointWeights           = ref->numJointWeights;
    this->jointWeights              = ref->jointWeights;
    this->jointWeightVerts          = ref->jointWeightVerts;

    this->vertWeights               = ref->vertWeights;
    this->useGpuSkinning            = (ref->vertWeights && meshType == Mesh::Type::Skinned) ? true : false;
    this->gpuSkinningVersionIndex   = ref->gpuSkinningVersionIndex;

    this->aabb                      = ref->aabb;

    if (this->type == Mesh::Type::Static || this->useGpuSkinning) {
        this->verts                 = ref->verts;

        this->vertexCache           = ref->vertexCache;
        this->indexCache            = ref->indexCache;
    } else {
        this->verts                 = (VertexGenericLit *)Mem_Alloc16(sizeof(VertexGenericLit) * ref->numVerts);

        this->vertexCache           = (BufferCache *)Mem_ClearedAlloc(sizeof(BufferCache));
        this->indexCache            = nullptr;

        simdProcessor->Memcpy(this->verts, ref->verts, sizeof(VertexGenericLit) * ref->numVerts);
    }
}

void SubMesh::FreeSubMesh() {
    if (!alloced) {
        return;
    }

    alloced = false;

    if (type == Mesh::Type::Reference) {
        if (vertexCache->buffer != RHI::NullBuffer) {
            rhi.DestroyBuffer(vertexCache->buffer);
            vertexCache->buffer = RHI::NullBuffer;
        }

        if (indexCache->buffer != RHI::NullBuffer) {
            rhi.DestroyBuffer(indexCache->buffer);
            indexCache->buffer = RHI::NullBuffer;
        }

        Mem_AlignedFree(verts);
        Mem_AlignedFree(mirroredVerts);
        Mem_AlignedFree(indexes);
        Mem_AlignedFree(dominantTris);
        Mem_AlignedFree(edges);
        Mem_AlignedFree(edgeIndexes);
        Mem_AlignedFree(jointWeights);
        Mem_AlignedFree(jointWeightVerts);
        Mem_AlignedFree(vertWeights);

        Mem_Free(vertexCache);
        Mem_Free(indexCache);
        return;
    }

    if (type == Mesh::Type::Dynamic || (type == Mesh::Type::Skinned && !useGpuSkinning)) {
        Mem_AlignedFree(verts);
        Mem_Free(vertexCache);
    }
}

void SubMesh::CopyFrom(const SubMesh *other) {
    if (!alloced) {
        BE_ERRLOG("SubMesh::CopyFrom: Should be allocated first");
        return;
    }

    if (numVerts != other->numVerts || numIndexes != other->numIndexes) {
        BE_ERRLOG("SubMesh::CopyFrom: different verts and indices count with source sub mesh");
        return;
    }

    tangentsCalculated = other->tangentsCalculated;
    normalsCalculated = other->normalsCalculated;
    edgesCalculated = false;

    simdProcessor->Memcpy(verts, other->verts, sizeof(verts[0]) * other->numVerts);
    simdProcessor->Memcpy(indexes, other->indexes, sizeof(indexes[0]) * other->numIndexes);
}

void SubMesh::CacheStaticDataToGpu() {
    // Fill in static vertex buffer.
    if (!bufferCacheManager.IsCached(vertexCache)) {
        int sizeVerts = sizeof(VertexGenericLit) * numVerts;

        // Write vertex weights after vertex data in the vertex buffer.
        if (vertWeights) {//surfSpace->def->state.joints && useGpuSkinning) {
            int sizeVertsAligned = ((sizeVerts + 15) >> 4) << 4;
            int sizeWeights = VertexWeightSize() * numVerts;
            int size = sizeVertsAligned + sizeWeights;
            
            bufferCacheManager.AllocStaticVertex(size, nullptr, vertexCache);

            rhi.BindBuffer(RHI::BufferType::Vertex, vertexCache->buffer);
            byte *ptr = (byte *)rhi.MapBuffer(vertexCache->buffer, RHI::BufferLockMode::WriteOnly);

            rhi.WriteBuffer(ptr, (const byte *)verts, sizeVerts);
            rhi.WriteBuffer(ptr + sizeVertsAligned, (const byte *)vertWeights, sizeWeights);

            if (!rhi.UnmapBuffer(vertexCache->buffer)) {
                BE_WARNLOG("Error unmapping buffer\n");
            }
        } else {
            bufferCacheManager.AllocStaticVertex(sizeVerts, verts, vertexCache);
        }
    }

    // Fill in static index buffer.
    if (!bufferCacheManager.IsCached(indexCache)) {
        bufferCacheManager.AllocStaticIndex(numIndexes * sizeof(TriIndex), indexes, indexCache);
    }
}

void SubMesh::CacheDynamicDataToGpu(const Mat3x4 *joints, const Material *material) {
    if (bufferCacheManager.IsCached(vertexCache)) {
        return;
    }

    if (joints) {
        simdProcessor->TransformVerts(verts, numVerts, joints, jointWeightVerts, reinterpret_cast<int *>(jointWeights), numJointWeights);
    }

    // NOTE: No need normals and tangents for VertexGeneric
#if 0
    bool unsmoothedTangents = (material->GetFlags() & Material::Flag::UnsmoothTangents) ? true : false;

    ComputeTangents(true, unsmoothedTangents);

    FixMirroredVerts();
#endif

    // Fill in dynamic vertex buffer
    bufferCacheManager.AllocVertex(numVerts, sizeof(VertexGenericLit), verts, vertexCache);

    int filledVertexCount = vertexCache->offset / sizeof(VertexGenericLit);

    // Fill in dynamic index buffer
    bufferCacheManager.AllocIndex(numIndexes, sizeof(TriIndex), nullptr, indexCache);

    TriIndex *dst_idxptr = (TriIndex *)bufferCacheManager.MapIndexBuffer(indexCache);
    TriIndex *src_idxptr = indexes;

    for (int i = 0; i < numIndexes; i += 3, dst_idxptr += 3, src_idxptr += 3) {
        dst_idxptr[0] = filledVertexCount + src_idxptr[0];
        dst_idxptr[1] = filledVertexCount + src_idxptr[1];
        dst_idxptr[2] = filledVertexCount + src_idxptr[2];
    }

    bufferCacheManager.UnmapIndexBuffer(indexCache);
}

void SubMesh::SplitMirroredVerts() {
    Vec3        tangents[2];
    float       handedness;

    static const TriIndex invalidIndex = std::numeric_limits<TriIndex>::max();
    float *vertHandednesses = (float *)_alloca16(sizeof(float) * numVerts);
    TriIndex *mirroredVertsIndexMap = (TriIndex *)_alloca16(sizeof(TriIndex) * numVerts);
    TriIndex *mirroredVertsIndexes = (TriIndex *)_alloca16(sizeof(TriIndex) * numVerts);

    VertexGenericLit *dupVerts = (VertexGenericLit *)Mem_Alloc16(sizeof(VertexGenericLit) * numVerts);

    for (int i = 0; i < numVerts; i++) {
        vertHandednesses[i] = 0;
        mirroredVertsIndexMap[i] = invalidIndex;
    }

    numMirroredVerts = 0;

    for (int i = 0; i < numIndexes; i += 3) {
        int i0 = indexes[i];
        int i1 = indexes[i + 1];
        int i2 = indexes[i + 2];

        assert(i0 >= 0 && i0 < numVerts);
        assert(i1 >= 0 && i1 < numVerts);
        assert(i2 >= 0 && i2 < numVerts);

        R_TangentsFromTriangle(verts[i0].xyz, verts[i1].xyz, verts[i2].xyz, 
            verts[i0].GetTexCoord(), verts[i1].GetTexCoord(), verts[i2].GetTexCoord(), 
            tangents[0], tangents[1], &handedness);

        for (int k = 0; k < 3; k++) {
            TriIndex idx = indexes[i + k];

            if (vertHandednesses[idx] == 0) {
                vertHandednesses[idx] = handedness;
            } else if (vertHandednesses[idx] != handedness && mirroredVertsIndexMap[idx] == invalidIndex) { // handedness 가 다르고 mirrored 버텍스가 없다면, 새 버텍스를 만든다
                // mirrored vertex index 심기
                indexes[i + k] = numVerts + numMirroredVerts; 

                dupVerts[numMirroredVerts] = verts[idx];

                mirroredVertsIndexes[numMirroredVerts] = idx;
                mirroredVertsIndexMap[idx] = numMirroredVerts;

                numMirroredVerts++;
            }
        }
    }

    if (numMirroredVerts > 0) {
        VertexGenericLit *newVerts = (VertexGenericLit *)Mem_Alloc16(sizeof(VertexGenericLit) * (numVerts + numMirroredVerts));

        simdProcessor->Memcpy(newVerts, verts, sizeof(VertexGenericLit) * numVerts);
        simdProcessor->Memcpy(newVerts + numVerts, dupVerts, sizeof(VertexGenericLit) * numMirroredVerts);

        TriIndex *newMirroredVerts = (TriIndex *)Mem_Alloc16(sizeof(TriIndex) * numMirroredVerts);
        simdProcessor->Memcpy(newMirroredVerts, mirroredVertsIndexes, sizeof(TriIndex) * numMirroredVerts);

        if (numJointWeights > 0) {
            int numAppendedJointWeights = 0;

            int k = 0;
            for (int i = 0; i < numVerts; i++) {
                if (mirroredVertsIndexMap[i] != invalidIndex) {
                    numAppendedJointWeights++;
                }

                while (jointWeights[k].nextVertOffset == 0) {
                    k++;
                    if (mirroredVertsIndexMap[i] != invalidIndex) {
                        numAppendedJointWeights++;
                    }
                }

                k++;
            }

            JointWeight *newJointWeights = (JointWeight *)Mem_Alloc16(sizeof(JointWeight) * (numJointWeights + numAppendedJointWeights));
            simdProcessor->Memcpy(newJointWeights, jointWeights, sizeof(JointWeight) * numJointWeights);

            Vec4 *newJointWeightVerts = (Vec4 *)Mem_Alloc16(sizeof(Vec4) * (numJointWeights + numAppendedJointWeights));
            simdProcessor->Memcpy(newJointWeightVerts, jointWeightVerts, sizeof(Vec4) * numJointWeights);

            int newJointWeightIndex = numJointWeights;
            int jointWeightIndex;

            for (int i = 0; i < numMirroredVerts; i++) {
                jointWeightIndex = 0;

                for (int k = 0; k < numVerts; k++) {
                    if (k == mirroredVertsIndexes[i]) {
                        break;
                    }

                    while (jointWeights[jointWeightIndex].nextVertOffset == 0) {
                        jointWeightIndex++;
                    }

                    jointWeightIndex++;
                }

                while (jointWeights[jointWeightIndex].nextVertOffset == 0) {
                    newJointWeights[newJointWeightIndex] = jointWeights[jointWeightIndex];
                    newJointWeightVerts[newJointWeightIndex] = jointWeightVerts[jointWeightIndex];
                    newJointWeightIndex++;
                    jointWeightIndex++;
                }

                newJointWeights[newJointWeightIndex] = jointWeights[jointWeightIndex];
                newJointWeightVerts[newJointWeightIndex] = jointWeightVerts[jointWeightIndex];
                newJointWeightIndex++;


            }

            Mem_AlignedFree(jointWeights);
            Mem_AlignedFree(jointWeightVerts);

            jointWeights        = newJointWeights;
            jointWeightVerts    = newJointWeightVerts;
            numJointWeights     = numJointWeights + numAppendedJointWeights;
        }

        if (vertWeights) {
            int vertexWeightSize = VertexWeightSize();
            void *newVertWeights = (void *)Mem_Alloc16(vertexWeightSize * (numVerts + numMirroredVerts));
            simdProcessor->Memcpy(newVertWeights, vertWeights, vertexWeightSize * numVerts);

            for (int i = 0; i < numMirroredVerts; i++) {
                memcpy((byte *)newVertWeights + vertexWeightSize * (numVerts + i), (byte *)vertWeights + vertexWeightSize * mirroredVertsIndexes[i], vertexWeightSize);
            }

            Mem_AlignedFree(vertWeights);

            vertWeights = newVertWeights;
        }

        Mem_AlignedFree(verts);

        verts           = newVerts;
        numVerts        = numVerts + numMirroredVerts;
        mirroredVerts   = newMirroredVerts;
    }

    Mem_AlignedFree(dupVerts);
}

void SubMesh::FixMirroredVerts() {
    VertexGenericLit *v0 = &verts[numVerts - numMirroredVerts];

    for (int i = 0; i < numMirroredVerts; i++, v0++) {
        VertexGenericLit *v1 = &verts[mirroredVerts[i]];

        // NOTE: should we normalize this ?
        //v0->normal += v1->normal;
        //v1->normal = v0->normal;
    }
}

void SubMesh::ComputeAABB() {
    aabb.Clear();
    for (int i = 0; i < numVerts; i++) {
        aabb.AddPoint(verts[i].xyz);
    }
}

// Compute area weighted average of the normals
void SubMesh::ComputeNormals() {
    if (normalsCalculated) {
        return;
    }

    Vec3 *tempNormals = (Vec3 *)Mem_Alloc16(numVerts * sizeof(Vec3));

    for (int i = 0; i < numVerts; i++) {
        tempNormals[i] = Vec3::zero;
    }

    for (int i = 0; i < numIndexes; i += 3) {
        int i0 = indexes[i];
        int i1 = indexes[i + 1];
        int i2 = indexes[i + 2];

        const VertexGenericLit &v0 = verts[i0];
        const VertexGenericLit &v1 = verts[i1];
        const VertexGenericLit &v2 = verts[i2];

        const Vec3 side0 = v1.xyz - v0.xyz;
        const Vec3 side1 = v2.xyz - v0.xyz;

        Vec3 faceNormal = side0.Cross(side1);

        tempNormals[i0] += faceNormal;
        tempNormals[i1] += faceNormal;
        tempNormals[i2] += faceNormal;
    }

    for (int i = 0; i < numVerts; i++) {
        tempNormals[i].Normalize();
        verts[i].SetNormal(tempNormals[i]);
    }

    Mem_AlignedFree(tempNormals);

    normalsCalculated = true;
}

static void R_DeriveTangentsWithoutNormals(VertexGenericLit *verts, const int numVerts, const TriIndex *indexes, const int numIndexes) {
    int numTris = numIndexes / 3;

    Vec3 *triangleTangents = (Vec3 *)Mem_Alloc16(numTris * sizeof(Vec3));
    Vec3 *triangleBitangents = (Vec3 *)Mem_Alloc16(numTris * sizeof(Vec3));

    // Calculate tangent vectors for each face in isolation
    for (int i = 0; i < numIndexes; i += 3) {
        int triIndex = i / 3;

        int v0 = indexes[i + 0];
        int v1 = indexes[i + 1];
        int v2 = indexes[i + 2];

        const VertexGenericLit &a = verts[v0];
        const VertexGenericLit &b = verts[v1];
        const VertexGenericLit &c = verts[v2];

        const Vec2 aST = a.GetTexCoord();
        const Vec2 bST = b.GetTexCoord();
        const Vec2 cST = c.GetTexCoord();

        Vec3 baXYZ;
        baXYZ[0] = b.xyz[0] - a.xyz[0];
        baXYZ[1] = b.xyz[1] - a.xyz[1];
        baXYZ[2] = b.xyz[2] - a.xyz[2];

        Vec3 caXYZ;
        caXYZ[0] = c.xyz[0] - a.xyz[0];
        caXYZ[1] = c.xyz[1] - a.xyz[1];
        caXYZ[2] = c.xyz[2] - a.xyz[2];

        Vec2 baST;
        baST[0] = bST[0] - aST[0];
        baST[1] = bST[1] - aST[1];

        Vec2 caST;
        caST[0] = cST[0] - aST[0];
        caST[1] = cST[1] - aST[1];

        // area sign bit
        const float area = baST[0] * caST[1] - baST[1] * caST[0]; // determinant
        if (fabs(area) < 1e-20f) {
            triangleTangents[triIndex] = Vec3::zero;
            triangleBitangents[triIndex] = Vec3::zero;
            continue;
        }

        float inva = (area < 0.0f) ? -1.0f : 1.0f;  // was = 1.0f / area;

        Vec3 tangent;
        tangent[0] = inva * (baXYZ[0] * caST[1] - caXYZ[0] * baST[1]);
        tangent[1] = inva * (baXYZ[1] * caST[1] - caXYZ[1] * baST[1]);
        tangent[2] = inva * (baXYZ[2] * caST[1] - caXYZ[2] * baST[1]);
        tangent.Normalize();

        triangleTangents[triIndex] = tangent;

        Vec3 bitangent;
        bitangent[0] = inva * (caXYZ[0] * baST[0] - baXYZ[0] * caST[0]);
        bitangent[1] = inva * (caXYZ[1] * baST[0] - baXYZ[1] * caST[0]);
        bitangent[2] = inva * (caXYZ[2] * baST[0] - baXYZ[2] * caST[0]);
        bitangent.Normalize();

        triangleBitangents[triIndex] = bitangent;
    }

    Vec3 *vertexTangents = (Vec3 *)Mem_Alloc16(numVerts * sizeof(Vec3));
    Vec3 *vertexBitangents = (Vec3 *)Mem_Alloc16(numVerts * sizeof(Vec3));

    // Clear the tangents
    for (int i = 0; i < numVerts; i++) {
        vertexTangents[i] = Vec3::zero;
        vertexBitangents[i] = Vec3::zero;
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
        Vec3 normal = verts[i].GetNormal();
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

static void R_DeriveNormalsAndTangents(VertexGenericLit *verts, const int numVerts, const TriIndex *indexes, const int numIndexes) {
    Vec3 *vertexNormals = (Vec3 *)Mem_Alloc16(numVerts * sizeof(Vec3));
    Vec3 *vertexTangents = (Vec3 *)Mem_Alloc16(numVerts * sizeof(Vec3));
    Vec3 *vertexBitangents = (Vec3 *)Mem_Alloc16(numVerts * sizeof(Vec3));

    memset(vertexNormals, 0, numVerts * sizeof(Vec3));
    memset(vertexTangents, 0, numVerts * sizeof(Vec3));
    memset(vertexBitangents, 0, numVerts * sizeof(Vec3));

    for (int i = 0; i < numIndexes; i += 3) {
        int v0 = indexes[i + 0];
        int v1 = indexes[i + 1];
        int v2 = indexes[i + 2];

        const VertexGenericLit &a = verts[v0];
        const VertexGenericLit &b = verts[v1];
        const VertexGenericLit &c = verts[v2];

        const Vec2 aST = a.GetTexCoord();
        const Vec2 bST = b.GetTexCoord();
        const Vec2 cST = c.GetTexCoord();

        Vec3 baXYZ;
        baXYZ[0] = b.xyz[0] - a.xyz[0];
        baXYZ[1] = b.xyz[1] - a.xyz[1];
        baXYZ[2] = b.xyz[2] - a.xyz[2];

        Vec3 caXYZ;
        caXYZ[0] = c.xyz[0] - a.xyz[0];
        caXYZ[1] = c.xyz[1] - a.xyz[1];
        caXYZ[2] = c.xyz[2] - a.xyz[2];

        Vec2 baST;
        baST[0] = bST[0] - aST[0];
        baST[1] = bST[1] - aST[1];

        Vec2 caST;
        caST[0] = cST[0] - aST[0];
        caST[1] = cST[1] - aST[1];

        // normal
        Vec3 normal;
        normal[0] = caXYZ[2] * baXYZ[1] - caXYZ[1] * baXYZ[2];
        normal[1] = caXYZ[0] * baXYZ[2] - caXYZ[2] * baXYZ[0];
        normal[2] = caXYZ[1] * baXYZ[0] - caXYZ[0] * baXYZ[1];

        const float f0 = Math::InvSqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);

        normal.x *= f0;
        normal.y *= f0;
        normal.z *= f0;

        // area sign bit
        float area = baST[0] * caST[1] - baST[1] * caST[0]; // determinant
        uint32_t signBit = (*(uint32_t *)&area) & (1 << 31);

        // tangent
        Vec3 tangent;
        tangent[0] = baXYZ[0] * caST[1] - caXYZ[0] * baST[1];
        tangent[1] = baXYZ[1] * caST[1] - caXYZ[1] * baST[1];
        tangent[2] = baXYZ[2] * caST[1] - caXYZ[2] * baST[1];

        const float f1 = Math::InvSqrt(tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z);
        *(uint32_t *)&f1 ^= signBit;

        tangent.x *= f1;
        tangent.y *= f1;
        tangent.z *= f1;

        // bitangent
        Vec3 bitangent;
        bitangent[0] = caXYZ[0] * baST[0] - baXYZ[0] * caST[0];
        bitangent[1] = caXYZ[1] * baST[0] - baXYZ[1] * caST[0];
        bitangent[2] = caXYZ[2] * baST[0] - baXYZ[2] * caST[0];

        const float f2 = Math::InvSqrt(bitangent.x * bitangent.x + bitangent.y * bitangent.y + bitangent.z * bitangent.z);
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
        const float normalScale = Math::InvSqrt(vertexNormals[i].LengthSqr());
        vertexNormals[i].x *= normalScale;
        vertexNormals[i].y *= normalScale;
        vertexNormals[i].z *= normalScale;

        vertexTangents[i].ProjectOnPlane(vertexNormals[i]);
        vertexBitangents[i].ProjectOnPlane(vertexNormals[i]);

        const float tangentScale = Math::InvSqrt(vertexTangents[i].LengthSqr());
        vertexTangents[i].x *= tangentScale;
        vertexTangents[i].y *= tangentScale;
        vertexTangents[i].z *= tangentScale;

        const float bitangentScale = Math::InvSqrt(vertexBitangents[i].LengthSqr());
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

static void R_DeriveUnsmoothedNormalsAndTangents(VertexGenericLit *verts, const DominantTri *dominantTris, const int numVerts) {
    for (int i = 0; i < numVerts; i++) {
        float d0, d1, d2, d3, d4;
        float d5, d6, d7, d8, d9;
        float s0, s1, s2;
        float n0, n1, n2;
        float t0, t1, t2;
        float t3, t4, t5;

        const DominantTri &dt = dominantTris[i];

        VertexGenericLit *a = verts + i;
        VertexGenericLit *b = verts + dt.v2;
        VertexGenericLit *c = verts + dt.v3;

        const Vec2 aST = a->GetTexCoord();
        const Vec2 bST = b->GetTexCoord();
        const Vec2 cST = c->GetTexCoord();

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
        dominantTris = (DominantTri *)Mem_Alloc16(sizeof(DominantTri) * numVerts);
    }

    // Compute the dominant triangles for each vertices
    // The dominant triangle should have most larger surface area among the adjacent triangles of a vertex
    for (int i = 0; i < numVerts; i++) {
        float dominantTriArea = -1.0f;
        TriIndex dominantTriVertex2 = -1;
        TriIndex dominantTriVertex3 = -1;

        for (int j = 0; j < numIndexes; j += 3) {
            if (indexes[j] == i || indexes[j+1] == i || indexes[j+2] == i) {
                Vec3 p0 = verts[indexes[j+1]].xyz - verts[indexes[j]].xyz;
                Vec3 p1 = verts[indexes[j+2]].xyz - verts[indexes[j]].xyz;

                // Compute the surface area of a triangle
                //float area = p0.Cross(p1).Length() * 0.5f; // original version
                float area = p0.Cross(p1).LengthSqr(); // low cost version

                if (area > dominantTriArea) {
                    dominantTriArea = area;

                    if (indexes[j] == i) {
                        dominantTriVertex2 = indexes[j + 1];
                        dominantTriVertex3 = indexes[j + 2];
                    } else if (indexes[j+1] == i) {
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

        VertexGenericLit *a = &verts[i];
        VertexGenericLit *b = &verts[dominantTriVertex2];
        VertexGenericLit *c = &verts[dominantTriVertex3];

        const Vec2 aST = a->GetTexCoord();
        const Vec2 bST = b->GetTexCoord();
        const Vec2 cST = c->GetTexCoord();

        Vec3 side0 = b->xyz - a->xyz;
        Vec3 side1 = c->xyz - a->xyz;

        float ds1 = bST[0] - aST[0];
        float dt1 = bST[1] - aST[1];

        float ds2 = cST[0] - aST[0];
        float dt2 = cST[1] - aST[1];

        // We just need determinant sign because tangents vectors should be normalized
        float det = ds1 * dt2 - ds2 * dt1;
        uint32_t signBit = (*(uint32_t *)&det) & (1 << 31);

        Vec3 t0;
        t0.x = dt2 * side0.x - dt1 * side1.x;
        t0.y = dt2 * side0.y - dt1 * side1.y;
        t0.z = dt2 * side0.z - dt1 * side1.z;

        const float f0 = Math::InvSqrt(t0.LengthSqr());
        *(uint32_t *)&f0 ^= signBit;
        dominantTris[i].normalizationScale[0] = f0;

        Vec3 t1;
        t1.x = ds1 * side1.x - ds2 * side0.x;
        t1.y = ds1 * side1.y - ds2 * side0.y;
        t1.z = ds1 * side1.z - ds2 * side0.z;

        const float f1 = Math::InvSqrt(t1.LengthSqr());
        *(uint32_t *)&f1 ^= signBit;
        dominantTris[i].normalizationScale[1] = f1;

        Vec3 n;
        n.x = side0.y * side1.z - side0.z * side1.y;
        n.y = side0.z * side1.x - side0.x * side1.z;
        n.z = side0.x * side1.y - side0.y * side1.x;

        const float f2 = Math::InvSqrt(n.LengthSqr());
        dominantTris[i].normalizationScale[2] = f2;
    }
}

void SubMesh::ComputeTangents(bool includeNormals, bool useUnsmoothedTangents) {
    if (tangentsCalculated) {
        return;
    }

    if (useUnsmoothedTangents) {
        ComputeDominantTris();
        R_DeriveUnsmoothedNormalsAndTangents(verts, dominantTris, numVerts);
        normalsCalculated = true;
    } else if (!includeNormals) {
        R_DeriveTangentsWithoutNormals(verts, numVerts, (const TriIndex *)indexes, numIndexes);
    } else {
        R_DeriveNormalsAndTangents(verts, numVerts, (const TriIndex *)indexes, numIndexes);
        normalsCalculated = true;
    }

    tangentsCalculated = true;
}

// Assumes each edge is shared by at most two triangles.
void SubMesh::ComputeEdges() {
    if (numVerts == 0 || numIndexes == 0) {
        assert(0);
        return;
    }

    // Temporary edge buffer to compute real 'edges'.
    // Maximum edge count is same as index count. but we need one more space for 0'th edge for dummy.
    Edge *tempEdges = (Edge *)Mem_Alloc16((numIndexes + 1) * sizeof(tempEdges[0]));

    Edge triEdges[3];
    // 0'th edge is not possible to have negative index, so we'll ignore it.
    memset(&triEdges[0], 0, sizeof(triEdges[0]));
    tempEdges[0] = triEdges[0];

    // edge's vertex index v0 to the edge table.
    int32_t *vertexEdges = (int32_t *)Mem_Alloc16(numVerts * sizeof(vertexEdges[0]));
    memset(vertexEdges, -1, numVerts * sizeof(int32_t));
    // vertices might have many edges.
    int32_t *edgeChain = (int32_t *)Mem_Alloc16((numIndexes + 1) * sizeof(edgeChain[0]));

    // edge indexes.
    edgeIndexes = (int32_t *)Mem_Alloc16(numIndexes * sizeof(edgeIndexes[0]));

    int numTempEdges = 1;
    int numDisjunctiveEdges = 0;

    for (int i = 0; i < numIndexes; i += 3) {
        // Vertex indexes for a current triangle
        const TriIndex *triIndexes = indexes + i;

        const int32_t i0 = triIndexes[0];
        const int32_t i1 = triIndexes[1];
        const int32_t i2 = triIndexes[2];

        // Ordering to small index comes first.
        int32_t s = INT32_SIGNBITSET(i1 - i0);
        triEdges[0].v[0] = triIndexes[s];
        triEdges[0].v[1] = triIndexes[s^1];
        s = INT32_SIGNBITSET(i2 - i1) + 1;
        triEdges[1].v[0] = triIndexes[s];
        triEdges[1].v[1] = triIndexes[s^3];
        s = INT32_SIGNBITSET(i2 - i0) << 1;
        triEdges[2].v[0] = triIndexes[s];
        triEdges[2].v[1] = triIndexes[s^2];

        for (int j = 0; j < 3; j++) {
            Edge &edge = triEdges[j];

            const int32_t v0 = edge.v[0]; // edge vertex index 0
            const int32_t v1 = edge.v[1]; // edge vertex index 1

            // edge vertex winding 이 triangle winding (CCW) 과 같다면 0
            // edge vertex winding 이 triangle winding (CCW) 과 다르다면 1
            const unsigned int order = (v0 == triIndexes[j] ? 0 : 1);

            // Find the shared edge.
            int edgeNum;
            for (edgeNum = vertexEdges[v0]; edgeNum >= 0; edgeNum = edgeChain[edgeNum]) {
                if (tempEdges[edgeNum].v[1] == v1) {
                    break;
                }
            }

            // Add new edge if no shared edge is found or two edges are already shared.
            if (edgeNum < 0 || tempEdges[edgeNum].t[order] != -1) {
                if (edgeNum >= 0) {
                    numDisjunctiveEdges++;
                }

                // Add an edge to the temporary edge buffer.
                edge.t[0] = edge.t[1] = -1;
                edgeNum = numTempEdges;
                tempEdges[numTempEdges++] = edge;

                // Update edge chain for later use.
                edgeChain[edgeNum] = vertexEdges[v0];
                vertexEdges[v0] = edgeNum;
            }

            // Update a triangle index of an edge.
            //assert(tempEdges[edgeNum].t[order] == -1);
            tempEdges[edgeNum].t[order] = i / 3;

            // Update an edge index.
            edgeIndexes[i + j] = order ? -edgeNum : edgeNum;
        }
    }

    // Warning output for two or more shared edges.
    if (numDisjunctiveEdges > 0) {
        BE_WARNLOG("%i disjunctive edges found\n", numDisjunctiveEdges);
    }

    numEdges = numTempEdges;
    edges = (Edge *)Mem_Alloc16(sizeof(Edge) * numEdges);
    simdProcessor->Memcpy(edges, tempEdges, sizeof(Edge) * numEdges);

    // Cleans up temporary memory
    Mem_AlignedFree(vertexEdges);
    Mem_AlignedFree(edgeChain);
    Mem_AlignedFree(tempEdges);

    edgesCalculated = true;
}

int SubMesh::FindEdge(int32_t v1, int32_t v2) const {
    if (!edgesCalculated) {
        return false;
    }

    int32_t firstVert, secondVert;
    if (v1 < v2) {
        firstVert = v1;
        secondVert = v2;
    } else {
        firstVert = v2;
        secondVert = v1;
    }

    for (int i = 1; i < numEdges; i++) {
        if (edges[i].v[0] == firstVert &&
            edges[i].v[1] == secondVert) {
            // Returns negative edge index if and if only first vertex index is larger than second one.
            return v1 < v2 ? i : -i;
        }
    }

    return 0;
}

bool SubMesh::IsClosed() const {
    if (!edgesCalculated) {
        return false;
    }

    // All the edges should have two adjacent triangles if the mesh is closed
    for (int i = 0; i < numEdges; i++) {
        if (edges[i].t[0] < 0 || edges[i].t[1] < 0) {
            return false;
        }
    }
    return true;
}

bool SubMesh::IsIntersectLine(const Vec3 &start, const Vec3 &end, bool ignoreBackFace) const {
    if (!edgesCalculated) {
        return false;
    }

    Ray ray;
    ray.origin = start;
    ray.dir = end - start;
    ray.dir.Normalize();

    return IntersectRay(ray, ignoreBackFace);
}

bool SubMesh::IntersectRay(const Ray &ray, bool ignoreBackFace, float *hitDist) const {
    if (!edgesCalculated) {
        return false;
    }

    byte *sidedness = (byte *)_alloca(numEdges * sizeof(byte));
    float dist = Math::Infinity;

    Pluecker rayPl, pl;
    rayPl.SetFromRay(ray.origin, ray.dir);

    // ray sidedness for edges
    for (int i = 0; i < numEdges; i++) {
        pl.SetFromLine(verts[edges[i].v[1]].xyz, verts[edges[i].v[0]].xyz);
        float d = pl.PermutedInnerProduct(rayPl);
        sidedness[i] = IEEE_FLT_SIGNBITSET(d);
    }

    Plane plane;

    // test triangles
    for (int i = 0; i < numIndexes; i += 3) {
        const int32_t i0 = edgeIndexes[i + 0];
        const int32_t i1 = edgeIndexes[i + 1];
        const int32_t i2 = edgeIndexes[i + 2];

        const int32_t s0 = sidedness[Math::Abs(i0)] ^ INT32_SIGNBITSET(i0);
        const int32_t s1 = sidedness[Math::Abs(i1)] ^ INT32_SIGNBITSET(i1);
        const int32_t s2 = sidedness[Math::Abs(i2)] ^ INT32_SIGNBITSET(i2);

        if ((s0 & s1 & s2) || (!ignoreBackFace && !(s0 | s1 | s2))) {
            plane.SetFromPoints(verts[indexes[i + 0]].xyz, verts[indexes[i + 1]].xyz, verts[indexes[i + 2]].xyz);

            float d;
            if (plane.IntersectRay(ray, false, &d)) {
                if (!hitDist) {
                    return true;
                }

                if (d < dist) {
                    dist = d;
                }
            }
        }
    }

    if (dist == Math::Infinity) {
        return false;
    }

    *hitDist = dist;
    return true;
}

float SubMesh::ComputeVolume() const {
    float intg = 0;

    for (int i = 0; i < numIndexes; i += 3) {
        const Vec3 &a = verts[indexes[i]].xyz;
        const Vec3 &b = verts[indexes[i + 1]].xyz;
        const Vec3 &c = verts[indexes[i + 2]].xyz;

        const Vec3 side0 = b - a;
        const Vec3 side1 = c - a;

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

const Vec3 SubMesh::ComputeCentroid() const {
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

        simd4f cr = cross_ps(side0, side1);

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

    Vec3 centroid;
    centroid.x = intg[1] * invVolume;
    centroid.y = intg[2] * invVolume;
    centroid.z = intg[3] * invVolume;

    return centroid;
#else
    float intg[4] = { 0, 0, 0, 0 };
    const float multipliers[2] = { 1.0f/6.0f, 1.0f/24.0f };
    float f1x, f1y, f1z, f2x, f2y, f2z;

    for (int i = 0; i < numIndexes; i += 3) {
        const Vec3 &a = verts[indexes[i]].xyz;
        const Vec3 &b = verts[indexes[i + 1]].xyz;
        const Vec3 &c = verts[indexes[i + 2]].xyz;

        const Vec3 side0 = b - a;
        const Vec3 side1 = c - a;

        const Vec3 cr = side0.Cross(side1);

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

    Vec3 centroid;
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

const Mat3 SubMesh::ComputeInertiaTensor(const Vec3 &centroid, float mass) const {
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

        simd4f cr = cross_ps(side0, side1);

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

    Mat3 inertia;
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
        const Vec3 &a = verts[indexes[i]].xyz;
        const Vec3 &b = verts[indexes[i + 1]].xyz;
        const Vec3 &c = verts[indexes[i + 2]].xyz;

        const Vec3 side0 = b - a;
        const Vec3 side1 = c - a;

        const Vec3 cr = side0.Cross(side1);

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

    Mat3 inertia;
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

void SubMesh::OptimizeIndexedTriangles() {
#if 0
    // NOTE: consider if size of the index is greater than 16 bit number or not
    PrimitiveGroup *srcPrimGroup = new PrimitiveGroup;
    srcPrimGroup->type = PrimType::PT_LIST;
    srcPrimGroup->numIndices = numIndexes;
    srcPrimGroup->indices = new unsigned short [numIndexes];

    for (int i = 0; i < numIndexes; i++) {
        srcPrimGroup->indices[i] = indexes[i];
    }

    PrimitiveGroup *dstPrimGroup;

    // remap indices
    RemapIndices(srcPrimGroup, 1, numVerts, &dstPrimGroup);

    for (int i = 0; i < numIndexes; i++) {
        indexes[i] = dstPrimGroup->indices[i];
    }

    // remap vertices
    VertexGenericLit *remappedVerts = (VertexGenericLit *)Mem_Alloc16(sizeof(verts[0]) * numVerts);

    for (int i = 0; i < numIndexes; i++) {
        remappedVerts[dstPrimGroup->indices[i]] = verts[srcPrimGroup->indices[i]];
    }

    Mem_AlignedFree(verts);
    verts = remappedVerts;
    
    delete srcPrimGroup;
    delete [] dstPrimGroup;
#endif
}

BE_NAMESPACE_END
