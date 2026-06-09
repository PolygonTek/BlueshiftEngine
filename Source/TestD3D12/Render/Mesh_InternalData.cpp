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
#include "Mesh.h"
#include "BModel.h"
#include "Skeleton.h"

static BE1::AABB mergeAABB;

Mesh::InternalData &Mesh::InternalData::operator=(InternalData &&rhs) noexcept {
    if (this != &rhs) {
        Purge();
        Swap(rhs);
    }
    return *this;
}

void Mesh::InternalData::Purge() {
    surfaces.DeleteContents(true);

    SAFE_DELETE_ARRAY(joints);

    numJoints = 0;
    aabb = BE1::AABB::empty;
}

void Mesh::InternalData::Swap(InternalData &rhs) {
    surfaces.Swap(rhs.surfaces);
    BE1::Swap(numJoints, rhs.numJoints);
    BE1::Swap(joints, rhs.joints);
    BE1::Swap(aabb, rhs.aabb);
}

bool Mesh::InternalData::LoadBinaryMesh(const char *filename) {
    BE1::Str binaryMeshFilename = filename;
    if (!BE1::Str::CheckExtension(filename, ".bmesh")) {
        binaryMeshFilename.SetFileExtension(".bmesh");
    }

    BE_LOG("Loading mesh '%s'...\n", binaryMeshFilename.c_str());

    byte *data;
    BE1::fileSystem.LoadFile(binaryMeshFilename, true, (void **)&data);
    if (!data) {
        return false;
    }

    const BMeshHeader *bMeshHeader = (const BMeshHeader *)data;
    byte *ptr = data + sizeof(BMeshHeader);

    if (bMeshHeader->ident != BMESH_IDENT) {
        BE_WARNLOG("Mesh::InternalData::LoadBinaryMesh: bad format %s\n", binaryMeshFilename.c_str());
        BE1::fileSystem.FreeFile(data);
        return false;
    }

    numJoints = bMeshHeader->numJoints;
    if (numJoints > 0) {
        joints = new Joint[numJoints];

        // --- joints ---
        for (int jointIndexes = 0; jointIndexes < numJoints; jointIndexes++) {
            const BJoint *bJoint = (const BJoint *)ptr;

            joints[jointIndexes].name = bJoint->name;
            joints[jointIndexes].parent = bJoint->parentIndex >= 0 ? &joints[bJoint->parentIndex] : nullptr;

            ptr += sizeof(BJoint);
        }
    }

    aabb = BE1::AABB(bMeshHeader->aabbMin, bMeshHeader->aabbMax);

    // --- surfaces ---
    for (int surfaceIndex = 0; surfaceIndex < bMeshHeader->numSurfs; surfaceIndex++) {
        const BMeshSurf *bMeshSurf = (const BMeshSurf *)ptr;
        ptr += sizeof(BMeshSurf);

        Surface *meshSurf = Mesh::AllocSurface(bMeshSurf->numVerts, bMeshSurf->numIndexes);
        surfaces.Append(meshSurf);
        SubMesh *subMesh = meshSurf->subMesh;
        subMesh->aabb = BE1::AABB(bMeshSurf->aabbMin, bMeshSurf->aabbMax);

        meshSurf->materialIndex = bMeshSurf->materialIndex;

        // --- vertexes ---
        for (int i = 0; i < bMeshSurf->numVerts; i++) {
            BE1::VertexGenericLit *v = &subMesh->verts[i];

            const BMeshVert *bMeshVert = (const BMeshVert *)ptr;

            v->SetPosition(bMeshVert->position);
            v->SetTexCoord(bMeshVert->texCoord);
            v->SetNormal(bMeshVert->normal);
            v->SetTangent(bMeshVert->tangent);
            v->SetBiTangent(bMeshVert->bitangent);
            v->SetColor(bMeshVert->color);

            ptr += sizeof(BMeshVert);
        }

        // --- vertex weights ---
        if (bMeshSurf->maxWeights > 0) {
            int vertexWeightSize = 0;

            if (bMeshSurf->maxWeights == 1) {
                vertexWeightSize = sizeof(BE1::VertexWeight1);
                subMesh->vertWeights = Mem_Alloc16(vertexWeightSize * bMeshSurf->numVerts);
                subMesh->gpuSkinningVersionIndex = 0;

                BE1::VertexWeight1 *dstPtr = (BE1::VertexWeight1 *)subMesh->vertWeights;
                for (int i = 0; i < bMeshSurf->numVerts; i++, dstPtr++) {
                    dstPtr->jointIndex = *ptr++;
                }
            } else if (bMeshSurf->maxWeights <= 4) {
                vertexWeightSize = sizeof(BE1::VertexWeight4);
                subMesh->vertWeights = Mem_Alloc16(vertexWeightSize * bMeshSurf->numVerts);
                subMesh->gpuSkinningVersionIndex = 1;

                BE1::VertexWeight4 *dstPtr = (BE1::VertexWeight4 *)subMesh->vertWeights;
                for (int i = 0; i < bMeshSurf->numVerts; i++, dstPtr++) {
                    dstPtr->jointIndexes[0] = *ptr++;
                    dstPtr->jointIndexes[1] = *ptr++;
                    dstPtr->jointIndexes[2] = *ptr++;
                    dstPtr->jointIndexes[3] = *ptr++;

                    dstPtr->jointWeights[0] = *ptr++;
                    dstPtr->jointWeights[1] = *ptr++;
                    dstPtr->jointWeights[2] = *ptr++;
                    dstPtr->jointWeights[3] = *ptr++;
                }
            } else if (bMeshSurf->maxWeights <= 8) {
                vertexWeightSize = sizeof(BE1::VertexWeight8);
                subMesh->vertWeights = Mem_Alloc16(vertexWeightSize * bMeshSurf->numVerts);
                subMesh->gpuSkinningVersionIndex = 2;

                BE1::VertexWeight8 *dstPtr = (BE1::VertexWeight8 *)subMesh->vertWeights;
                for (int i = 0; i < bMeshSurf->numVerts; i++, dstPtr++) {
                    dstPtr->jointIndexes[0] = *ptr++;
                    dstPtr->jointIndexes[1] = *ptr++;
                    dstPtr->jointIndexes[2] = *ptr++;
                    dstPtr->jointIndexes[3] = *ptr++;
                    dstPtr->jointIndexes[4] = *ptr++;
                    dstPtr->jointIndexes[5] = *ptr++;
                    dstPtr->jointIndexes[6] = *ptr++;
                    dstPtr->jointIndexes[7] = *ptr++;

                    dstPtr->jointWeights[0] = *ptr++;
                    dstPtr->jointWeights[1] = *ptr++;
                    dstPtr->jointWeights[2] = *ptr++;
                    dstPtr->jointWeights[3] = *ptr++;
                    dstPtr->jointWeights[4] = *ptr++;
                    dstPtr->jointWeights[5] = *ptr++;
                    dstPtr->jointWeights[6] = *ptr++;
                    dstPtr->jointWeights[7] = *ptr++;
                }
            } else {
                assert(0);
            }
        }

        // --- indexes ---
        if (bMeshSurf->indexSize == 4) {
            for (int i = 0; i < bMeshSurf->numIndexes; i++) {
                subMesh->indexes[i] = *(uint32_t *)ptr;
                ptr += sizeof(uint32_t);
            }
        } else if (bMeshSurf->indexSize == 2) {
            for (int i = 0; i < bMeshSurf->numIndexes; i++) {
                subMesh->indexes[i] = *(uint16_t *)ptr;
                ptr += sizeof(uint16_t);
            }
        }

        // guarantee 8 bytes aligned read
        long offset = (intptr_t)ptr;
        ptr += BE1::AlignUp(offset, 8) - offset;
    }

    BE1::fileSystem.FreeFile(data);
    return true;
}

void Mesh::InternalData::WriteBinaryMesh(const char *filename) {
    BE1::File *fp = BE1::fileSystem.OpenFile(filename, BE1::File::Mode::Write);
    if (!fp) {
        BE_WARNLOG("Mesh::InternalData::WriteBinaryMesh: file open error\n");
        return;
    }

    BMeshHeader bMeshHeader;
    bMeshHeader.ident = BMESH_IDENT;
    bMeshHeader.version = BMESH_VERSION;
    bMeshHeader.numJoints = numJoints;
    bMeshHeader.numSurfs = surfaces.Count();
    bMeshHeader.aabbMin = aabb[0];
    bMeshHeader.aabbMax = aabb[1];
    fp->Write(&bMeshHeader, sizeof(bMeshHeader));

    if (bMeshHeader.numJoints > 0) {
        // --- joints ---
        for (int jointIndexes = 0; jointIndexes < bMeshHeader.numJoints; jointIndexes++) {
            const Joint *joint = &joints[jointIndexes];

            BJoint bJoint;
            BE1::Str::Copynz(bJoint.name, joint->name, sizeof(bJoint.name));
            bJoint.parentIndex = !joint->parent ? -1 : (int32_t)(joint->parent - joints);
            fp->Write(&bJoint, sizeof(bJoint));
        }
    }

    // --- surfaces ---
    for (int surfaceIndex = 0; surfaceIndex < bMeshHeader.numSurfs; surfaceIndex++) {
        const Surface *meshSurf = surfaces[surfaceIndex];
        const SubMesh *subMesh = meshSurf->subMesh;

        BMeshSurf bMeshSurf;
        bMeshSurf.materialIndex = meshSurf->materialIndex;
        bMeshSurf.numVerts = subMesh->numVerts;
        bMeshSurf.numIndexes = subMesh->numIndexes;
        bMeshSurf.indexSize = subMesh->numIndexes < BIT(16) ? sizeof(uint16_t) : sizeof(uint32_t);
        bMeshSurf.maxWeights = subMesh->MaxVertexWeights();
        bMeshSurf.aabbMin = subMesh->GetAABB()[0];
        bMeshSurf.aabbMax = subMesh->GetAABB()[1];
        fp->Write(&bMeshSurf, sizeof(bMeshSurf));

        // --- vertexes ---
        for (int i = 0; i < subMesh->numVerts; i++) {
            const BE1::VertexGenericLit *v = &subMesh->verts[i];

            BMeshVert bMeshVert;
            bMeshVert.position = v->GetPosition();
            bMeshVert.texCoord = v->GetTexCoord();
            bMeshVert.normal = v->GetNormal();
            bMeshVert.tangent = v->GetTangent();
            bMeshVert.bitangent = v->GetBiTangent();
            bMeshVert.color = v->GetColor();
            fp->Write(&bMeshVert, sizeof(bMeshVert));
        }

        // --- vertex weights ---
        if (bMeshSurf.maxWeights > 0) {
            if (bMeshSurf.maxWeights == 1) {
                BE1::VertexWeight1 *vw = (BE1::VertexWeight1 *)subMesh->vertWeights;
                for (int i = 0; i < bMeshSurf.numVerts; i++, vw++) {
                    fp->WriteUChar(vw->jointIndex);
                }
            } else if (bMeshSurf.maxWeights <= 4) {
                BE1::VertexWeight4 *vw = (BE1::VertexWeight4 *)subMesh->vertWeights;
                for (int i = 0; i < bMeshSurf.numVerts; i++, vw++) {
                    fp->WriteUChar(vw->jointIndexes[0]);
                    fp->WriteUChar(vw->jointIndexes[1]);
                    fp->WriteUChar(vw->jointIndexes[2]);
                    fp->WriteUChar(vw->jointIndexes[3]);

                    fp->WriteUChar(vw->jointWeights[0]);
                    fp->WriteUChar(vw->jointWeights[1]);
                    fp->WriteUChar(vw->jointWeights[2]);
                    fp->WriteUChar(vw->jointWeights[3]);
                }
            } else if (bMeshSurf.maxWeights <= 8) {
                BE1::VertexWeight8 *vw = (BE1::VertexWeight8 *)subMesh->vertWeights;
                for (int i = 0; i < bMeshSurf.numVerts; i++, vw++) {
                    fp->WriteUChar(vw->jointIndexes[0]);
                    fp->WriteUChar(vw->jointIndexes[1]);
                    fp->WriteUChar(vw->jointIndexes[2]);
                    fp->WriteUChar(vw->jointIndexes[3]);
                    fp->WriteUChar(vw->jointIndexes[4]);
                    fp->WriteUChar(vw->jointIndexes[5]);
                    fp->WriteUChar(vw->jointIndexes[6]);
                    fp->WriteUChar(vw->jointIndexes[7]);

                    fp->WriteUChar(vw->jointWeights[0]);
                    fp->WriteUChar(vw->jointWeights[1]);
                    fp->WriteUChar(vw->jointWeights[2]);
                    fp->WriteUChar(vw->jointWeights[3]);
                    fp->WriteUChar(vw->jointWeights[4]);
                    fp->WriteUChar(vw->jointWeights[5]);
                    fp->WriteUChar(vw->jointWeights[6]);
                    fp->WriteUChar(vw->jointWeights[7]);
                }
            }
        }

        // --- indexes ---
        if (subMesh->numIndexes < BIT(16)) {
            for (int i = 0; i < subMesh->numIndexes; i++) {
                fp->WriteUInt16(subMesh->indexes[i]);
            }
        } else {
            for (int i = 0; i < subMesh->numIndexes; i++) {
                fp->WriteUInt32(subMesh->indexes[i]);
            }
        }

        // guarantee 8 bytes aligned write
        byte dummy[8] = { 0, };
        int offset = fp->Tell();
        int dummyBytes = BE1::AlignUp(offset, 8) - offset;
        fp->Write(dummy, dummyBytes);
    }

    BE1::fileSystem.CloseFile(fp);
}

void Mesh::InternalData::FinishSurfaces(FinishFlag finishFlags) {
    if (BE1::HasFlag(finishFlags, FinishFlag::SortAndMerge)) {
        SortAndMerge();
    }

    if (BE1::HasAnyFlag(finishFlags, FinishFlag::SortAndMerge | FinishFlag::ComputeAABB)) {
        ComputeAABB();
    }

    if (BE1::HasFlag(finishFlags, FinishFlag::ComputeNormals) && !BE1::HasFlag(finishFlags, FinishFlag::ComputeTangents)) {
        ComputeNormals();
    }

    if (BE1::HasFlag(finishFlags, FinishFlag::ComputeTangents)) {
        ComputeTangents(BE1::HasFlag(finishFlags, FinishFlag::ComputeNormals), BE1::HasFlag(finishFlags, FinishFlag::UseUnsmoothedTangents));
    }

    if (BE1::HasFlag(finishFlags, FinishFlag::ComputeEdges)) {
        ComputeEdges();
    }
}

void Mesh::InternalData::TransformVerts(const BE1::Mat3 &rotation, const BE1::Vec3 &scale, const BE1::Vec3 &translation, bool recomputeAABB) {
    for (Mesh::Surface *surface : surfaces) {
        SubMesh *subMesh = surface->subMesh;

        for (int vertexIndex = 0; vertexIndex < subMesh->numVerts; vertexIndex++) {
            subMesh->verts[vertexIndex].Transform(rotation, scale, translation);
        }
    }

    if (recomputeAABB) {
        ComputeAABB();
    }
}

void Mesh::InternalData::RecomputeTangents() {
    for (Mesh::Surface *surface : surfaces) {
        surface->subMesh->flags |= SubMesh::Flag::TangentsCalculated;
        surface->subMesh->ComputeTangents(false, false);
    }
}

void Mesh::InternalData::GetJointAABBs(const BE1::Mat3x4 *invBindPoseMats, BE1::Array<BE1::AABB> &jointAabbs) const {
    for (BE1::AABB &jointAabb : jointAabbs) {
        jointAabb.Clear();
    }

    if (jointAabbs.Count() != numJoints) {
        BE_WARNLOG("Mesh::InternalData::GetJointAABBs: different joint count\n");
        return;
    }

    for (Mesh::Surface *surface : surfaces) {
        const SubMesh *subMesh = surface->subMesh;
        const int numVerts = subMesh->NumVerts();

        switch (subMesh->MaxVertexWeights()) {
        case 8:
        {
            const BE1::VertexWeight8 *vertWeights = (const BE1::VertexWeight8 *)subMesh->VertexWeights();

            for (int vertexIndex = 0; vertexIndex < numVerts; vertexIndex++) {
                const BE1::VertexWeight8 *vw = &vertWeights[vertexIndex];
                const BE1::Vec3 &position = subMesh->Verts()[vertexIndex].xyz;

                for (int weightIndex = 0; weightIndex < 8; weightIndex++) {
                    int jointIndex = vw->jointIndexes[weightIndex];

                    jointAabbs[jointIndex].AddPoint(invBindPoseMats[jointIndex].TransformPos(position));
                }
            }
        }
        break;
        case 4:
        {
            const BE1::VertexWeight4 *vertWeights = (const BE1::VertexWeight4 *)subMesh->VertexWeights();

            for (int vertexIndex = 0; vertexIndex < numVerts; vertexIndex++) {
                const BE1::VertexWeight4 *vw = &vertWeights[vertexIndex];
                const BE1::Vec3 &position = subMesh->Verts()[vertexIndex].xyz;

                for (int weightIndex = 0; weightIndex < 4; weightIndex++) {
                    int jointIndex = vw->jointIndexes[weightIndex];

                    jointAabbs[jointIndex].AddPoint(invBindPoseMats[jointIndex].TransformPos(position));
                }
            }
        }
        break;
        case 1:
        {
            const BE1::VertexWeight1 *vertWeights = (const BE1::VertexWeight1 *)subMesh->VertexWeights();

            for (int vertexIndex = 0; vertexIndex < numVerts; vertexIndex++) {
                const BE1::VertexWeight1 *vw = &vertWeights[vertexIndex];
                const BE1::Vec3 &position = subMesh->Verts()[vertexIndex].xyz;

                int jointIndex = vw->jointIndex;

                jointAabbs[jointIndex].AddPoint(invBindPoseMats[jointIndex].TransformPos(position));
            }
        }
        break;
        }
    }
}

void Mesh::InternalData::ComputeAABB() {
    aabb.Clear();

    for (Surface *surface : surfaces) {
        SubMesh *subMesh = surface->subMesh;
        subMesh->ComputeAABB();

        aabb.AddAABB(subMesh->aabb);
    }

    // add small epsilon
    aabb.ExpandSelf(BE1::MmToUnit(0.1f));
}

void Mesh::InternalData::ComputeNormals() {
    for (Mesh::Surface *surface : surfaces) {
        surface->subMesh->ComputeNormals();
    }
}

void Mesh::InternalData::ComputeTangents(bool includeNormals, bool useUnsmoothedTangents) {
    for (Mesh::Surface *surface : surfaces) {
        surface->subMesh->ComputeTangents(includeNormals, useUnsmoothedTangents);
    }
}

void Mesh::InternalData::ComputeEdges() {
    for (Mesh::Surface *surface : surfaces) {
        surface->subMesh->ComputeEdges();
    }
}

using MeshSurfPtr = Mesh::Surface *;
template <typename MeshSurfPtr>
static bool ListSortCompareMeshMaterials(const MeshSurfPtr &a, const MeshSurfPtr &b) {
    int index_a = (a)->materialIndex;
    int index_b = (b)->materialIndex;

    return index_a < index_b;
}

using MeshSurfPtr = Mesh::Surface *;
template <typename MeshSurfPtr>
static bool ListSortCompareMergedAABBArea(const MeshSurfPtr &a, const MeshSurfPtr &b) {
    const BE1::AABB &aAABB = (a)->subMesh->GetAABB();
    const BE1::AABB &bAABB = (b)->subMesh->GetAABB();

    float aArea = (mergeAABB + aAABB).Area();
    float bArea = (mergeAABB + bAABB).Area();

    if (aArea == bArea) {
        float aDist = aAABB.DistanceSqr(mergeAABB.Center());
        float bDist = bAABB.DistanceSqr(mergeAABB.Center());

        return aDist < bDist;
    }

    return aArea < bArea;
}

void Mesh::InternalData::SortAndMerge() {
    ComputeAABB();

    if (numJoints > 1) {
        return;
    }

    if (surfaces.Count() <= 1) {
        return;
    }

    surfaces.Sort(ListSortCompareMeshMaterials<MeshSurfPtr>);

    int m0 = 0;
    int m1 = 0;

    int numNewSurfs = 0;
    int numNewVerts;
    int numNewIndexes;

    while (m1 < surfaces.Count()) {
        if (m1 + 1 < surfaces.Count()) {
            numNewVerts = 0;
            numNewIndexes = 0;

            int m2 = m0;
            while (m2 + 1 < surfaces.Count() && 
                surfaces[m0]->materialIndex == surfaces[m2 + 1]->materialIndex) {
                m2++;
            }

            if (m2 > m0) {
                // m0 의 AABB 와 합쳤을 때 area 가 작은 순으로 sorting
                mergeAABB = surfaces[m0]->subMesh->aabb;
                surfaces.SortSubSection(m0, m2, ListSortCompareMergedAABBArea<MeshSurfPtr>);
            }

            // 합칠 기준 AABB 를 확장
            BE1::AABB aabb1 = surfaces[m0]->subMesh->aabb;
            aabb1.ExpandSelf(BE1::MeterToUnit(10.0f));

            while (m1 + 1 < surfaces.Count() && 
                surfaces[m0]->materialIndex == surfaces[m1 + 1]->materialIndex && 
                numNewIndexes < 32768) {
                BE1::AABB aabb2 = surfaces[m1 + 1]->subMesh->aabb;

                if (!aabb1.IsIntersectAABB(aabb2)) {
                    break;
                }

                // 합쳐질 AABB 의 면적이 80% 이하일 경우 중단
                BE1::AABB aabb3 = aabb1.Intersect(aabb2);
                if (aabb3.Area() / aabb2.Area() <= 0.8f && numNewIndexes > 36) {
                    break;
                }

                numNewVerts += surfaces[m1 + 1]->subMesh->numVerts;
                numNewIndexes += surfaces[m1 + 1]->subMesh->numIndexes;

                m1++;
            }
        }

        if (m1 - m0 > 0) {
            numNewVerts += surfaces[m0]->subMesh->numVerts;
            numNewIndexes += surfaces[m0]->subMesh->numIndexes;

            Mesh::Surface *newSurf = AllocSurface(numNewVerts, numNewIndexes);
            newSurf->materialIndex = surfaces[m0]->materialIndex;

            BE1::VertexGenericLit *vp = newSurf->subMesh->verts;
            BE1::VertIndex *ip = newSurf->subMesh->indexes;

            int filledVertexCount = 0;

            for (int i = m0; i <= m1; ++i) {
                Mesh::Surface *surf = surfaces[i];

                BE1::simdProcessor->Memcpy(vp, surf->subMesh->verts, sizeof(BE1::VertexGenericLit) * surf->subMesh->numVerts);
                vp += surf->subMesh->numVerts;

                for (int j = 0; j < surf->subMesh->numIndexes; j++) {
                    ip[j] = surf->subMesh->indexes[j] + filledVertexCount;
                }
                ip += surf->subMesh->numIndexes;

                filledVertexCount += surf->subMesh->numVerts;

                delete surf;
                surfaces[i] = nullptr;
            }

            surfaces[numNewSurfs] = newSurf;
            numNewSurfs++;

            m0 = m1;
        } else {
            surfaces[numNewSurfs] = surfaces[m0];
            numNewSurfs++;
        }

        m0++;
        m1++;
    }

    surfaces.Resize(numNewSurfs);
}
