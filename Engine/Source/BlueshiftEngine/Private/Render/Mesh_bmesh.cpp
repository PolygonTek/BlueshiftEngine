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
#include "BModel.h"
#include "Core/Heap.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

bool Mesh::LoadBinaryMesh(const char *filename) {
    byte *data;
    fileSystem.LoadFile(filename, true, (void **)&data);
    if (!data) {
        return false;
    }
 
    const BMeshHeader *bMeshHeader = (const BMeshHeader *)data;
    byte *ptr = data + sizeof(BMeshHeader);
    
    if (bMeshHeader->ident != BMESH_IDENT) {
        BE_WARNLOG(L"Mesh::LoadBinaryMesh: bad format %hs\n", filename);
        fileSystem.FreeFile(data);
        return false;
    }

    numJoints = bMeshHeader->numJoints;
    if (numJoints > 0) {
        joints = new Joint[numJoints];

        // --- joints ---
        for (int jointIndexes = 0; jointIndexes < numJoints; jointIndexes++) {
            const BJoint *bJoint = (const BJoint *)ptr;

            joints[jointIndexes].name = bJoint->name;
            joints[jointIndexes].parent = bJoint->parentIndex >= 0 ? &this->joints[bJoint->parentIndex] : nullptr;

            ptr += sizeof(BJoint);
        }
    }

    aabb = AABB(bMeshHeader->aabbMin, bMeshHeader->aabbMax);

    // --- surfaces ---
    for (int surfaceIndex = 0; surfaceIndex < bMeshHeader->numSurfs; surfaceIndex++) {
        const BMeshSurf *bMeshSurf = (const BMeshSurf *)ptr;
        ptr += sizeof(BMeshSurf);

        MeshSurf *meshSurf = AllocSurface(bMeshSurf->numVerts, bMeshSurf->numIndexes);
        surfaces.Append(meshSurf);
        SubMesh *subMesh = meshSurf->subMesh;
        subMesh->aabb = AABB(bMeshSurf->aabbMin, bMeshSurf->aabbMax);

        meshSurf->materialIndex = bMeshSurf->materialIndex;

        // --- vertexes ---
        for (int i = 0; i < bMeshSurf->numVerts; i++) {
            VertexGenericLit *v = &subMesh->verts[i];
            
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
                vertexWeightSize = sizeof(VertexWeight1);
                subMesh->vertWeights = Mem_Alloc16(vertexWeightSize * bMeshSurf->numVerts);
                subMesh->gpuSkinningVersionIndex = 0;

                VertexWeight1 *dstPtr = (VertexWeight1 *)subMesh->vertWeights;
                for (int i = 0; i < bMeshSurf->numVerts; i++, dstPtr++) {
                    dstPtr->jointIndex = *ptr++;
                }
            } else if (bMeshSurf->maxWeights <= 4) {
                vertexWeightSize = sizeof(VertexWeight4);
                subMesh->vertWeights = Mem_Alloc16(vertexWeightSize * bMeshSurf->numVerts);
                subMesh->gpuSkinningVersionIndex = 1;

                VertexWeight4 *dstPtr = (VertexWeight4 *)subMesh->vertWeights;
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
                vertexWeightSize = sizeof(VertexWeight8);
                subMesh->vertWeights = Mem_Alloc16(vertexWeightSize * bMeshSurf->numVerts);
                subMesh->gpuSkinningVersionIndex = 2;

                VertexWeight8 *dstPtr = (VertexWeight8 *)subMesh->vertWeights;
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
        ptr += AlignUp(offset, 8) - offset;
    }

    fileSystem.FreeFile(data);

    FinishSurfaces();

    return true;
}

void Mesh::WriteBinaryMesh(const char *filename) {
    File *fp = fileSystem.OpenFile(filename, File::WriteMode);
    if (!fp) {
        BE_WARNLOG(L"Mesh::WriteBinaryMesh: file open error\n");
        return;
    }

    BMeshHeader bMeshHeader;
    bMeshHeader.ident = BMESH_IDENT;
    bMeshHeader.version = BMESH_VERSION;
    bMeshHeader.numJoints = NumJoints();
    bMeshHeader.numSurfs = NumSurfaces();
    bMeshHeader.aabbMin = GetAABB()[0];
    bMeshHeader.aabbMax = GetAABB()[1];
    fp->Write(&bMeshHeader, sizeof(bMeshHeader));

    if (bMeshHeader.numJoints > 0) {
        // --- joints ---
        for (int jointIndexes = 0; jointIndexes < bMeshHeader.numJoints; jointIndexes++) {
            const Joint *joint = &joints[jointIndexes];

            BJoint bJoint;
            Str::Copynz(bJoint.name, joint->name, sizeof(bJoint.name));
            bJoint.parentIndex = !joint->parent ? -1 : (int32_t)(joint->parent - joints);
            fp->Write(&bJoint, sizeof(bJoint));
        }
    }

    // --- surfaces ---
    for (int surfaceIndex = 0; surfaceIndex < bMeshHeader.numSurfs; surfaceIndex++) {
        const MeshSurf *meshSurf = GetSurface(surfaceIndex);
        const SubMesh *subMesh = meshSurf->subMesh;

        BMeshSurf bMeshSurf;
        bMeshSurf.materialIndex     = meshSurf->materialIndex;
        bMeshSurf.numVerts          = subMesh->numVerts;
        bMeshSurf.numIndexes        = subMesh->numIndexes;
        bMeshSurf.indexSize         = subMesh->numIndexes < BIT(16) ? sizeof(uint16_t) : sizeof(uint32_t);
        bMeshSurf.maxWeights        = subMesh->MaxVertexWeights();
        bMeshSurf.aabbMin           = subMesh->GetAABB()[0];
        bMeshSurf.aabbMax           = subMesh->GetAABB()[1];
        fp->Write(&bMeshSurf, sizeof(bMeshSurf));

        // --- vertexes ---
        for (int i = 0; i < subMesh->numVerts; i++) {
            const VertexGenericLit *v = &subMesh->verts[i];

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
                VertexWeight1 *vw = (VertexWeight1 *)subMesh->vertWeights;
                for (int i = 0; i < bMeshSurf.numVerts; i++, vw++) {
                    fp->WriteUChar(vw->jointIndex);
                }
            } else if (bMeshSurf.maxWeights <= 4) {
                VertexWeight4 *vw = (VertexWeight4 *)subMesh->vertWeights;
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
                VertexWeight8 *vw = (VertexWeight8 *)subMesh->vertWeights;
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
        int dummyBytes = AlignUp(offset, 8) - offset;
        fp->Write(dummy, dummyBytes);
    }

    fileSystem.CloseFile(fp);
}
    
BE_NAMESPACE_END
