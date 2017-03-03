#include "Precompiled.h"
#include "Render/Render.h"
#include "BModel.h"
#include "Core/Heap.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

bool Mesh::LoadBMesh(const char *filename) {
    byte *data;
    fileSystem.LoadFile(filename, true, (void **)&data);
    if (!data) {
        return false;
    }
 
    const BMeshHeader *bMeshHeader = (const BMeshHeader *)data;
    byte *ptr = data + sizeof(BMeshHeader);
    
    if (bMeshHeader->ident != BMESH_IDENT) {
        BE_WARNLOG(L"Mesh::LoadBMesh: bad format %hs\n", filename);
        fileSystem.FreeFile(data);
        return false;
    }

    numJoints = bMeshHeader->numJoints;
    if (numJoints > 0) {
        joints = new Joint[numJoints];

        // --- joints ---
        for (int jointIndex = 0; jointIndex < numJoints; jointIndex++) {
            const BJoint *bJoint = (const BJoint *)ptr;

            joints[jointIndex].name = bJoint->name;
            joints[jointIndex].parent = bJoint->parentIndex >= 0 ? &this->joints[bJoint->parentIndex] : nullptr;

            ptr += sizeof(BJoint);
        }
    }

    // --- surfaces ---
    for (int surfaceIndex = 0; surfaceIndex < bMeshHeader->numSurfs; surfaceIndex++) {
        const BMeshSurf *bMeshSurf = (const BMeshSurf *)ptr;
        ptr += sizeof(BMeshSurf);

        MeshSurf *meshSurf = AllocSurface(bMeshSurf->numVerts, bMeshSurf->numIndexes);
        surfaces.Append(meshSurf);
        SubMesh *subMesh = meshSurf->subMesh;

        meshSurf->materialIndex = bMeshSurf->materialIndex;

        // --- vertexes ---
        for (int i = 0; i < bMeshSurf->numVerts; i++) {
            VertexLightingGeneric *v = &subMesh->verts[i];
            
            BMeshVert copy;
            memcpy(&copy, ptr, sizeof(BMeshVert)); // for alignment BUS Error

            v->SetPosition(copy.position);
            v->SetTexCoord(copy.texCoord);
            v->SetNormal(copy.normal);
            v->SetTangent(copy.tangent);
            v->SetBiTangent(copy.bitangent);
            v->SetColor(*reinterpret_cast<const uint32_t *>(copy.color));

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
                    dstPtr->index = *ptr++;
                }
            } else if (bMeshSurf->maxWeights <= 4) {
                vertexWeightSize = sizeof(VertexWeight4);
                subMesh->vertWeights = Mem_Alloc16(vertexWeightSize * bMeshSurf->numVerts);
                subMesh->gpuSkinningVersionIndex = 1;

                VertexWeight4 *dstPtr = (VertexWeight4 *)subMesh->vertWeights;
                for (int i = 0; i < bMeshSurf->numVerts; i++, dstPtr++) {
                    dstPtr->index[0] = *ptr++;
                    dstPtr->index[1] = *ptr++;
                    dstPtr->index[2] = *ptr++;
                    dstPtr->index[3] = *ptr++;

                    dstPtr->weight[0] = *ptr++;
                    dstPtr->weight[1] = *ptr++;
                    dstPtr->weight[2] = *ptr++;
                    dstPtr->weight[3] = *ptr++;
                }
            } else if (bMeshSurf->maxWeights <= 8) {
                vertexWeightSize = sizeof(VertexWeight8);
                subMesh->vertWeights = Mem_Alloc16(vertexWeightSize * bMeshSurf->numVerts);
                subMesh->gpuSkinningVersionIndex = 2;

                VertexWeight8 *dstPtr = (VertexWeight8 *)subMesh->vertWeights;
                for (int i = 0; i < bMeshSurf->numVerts; i++, dstPtr++) {
                    dstPtr->index[0] = *ptr++;
                    dstPtr->index[1] = *ptr++;
                    dstPtr->index[2] = *ptr++;
                    dstPtr->index[3] = *ptr++;
                    dstPtr->index[4] = *ptr++;
                    dstPtr->index[5] = *ptr++;
                    dstPtr->index[6] = *ptr++;
                    dstPtr->index[7] = *ptr++;

                    dstPtr->weight[0] = *ptr++;
                    dstPtr->weight[1] = *ptr++;
                    dstPtr->weight[2] = *ptr++;
                    dstPtr->weight[3] = *ptr++;
                    dstPtr->weight[4] = *ptr++;
                    dstPtr->weight[5] = *ptr++;
                    dstPtr->weight[6] = *ptr++;
                    dstPtr->weight[7] = *ptr++;
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
    }

    fileSystem.FreeFile(data);

    FinishSurfaces(0);

    return true;
}

void Mesh::WriteBMesh(const char *filename) {
    File *fp = fileSystem.OpenFile(filename, File::WriteMode);
    if (!fp) {
        BE_WARNLOG(L"Mesh::WriteBMesh: file open error\n");
        return;
    }

    BMeshHeader bMeshHeader;
    bMeshHeader.ident = BMESH_IDENT;
    bMeshHeader.version = BMESH_VERSION;
    bMeshHeader.numJoints = NumJoints();
    bMeshHeader.numSurfs = NumSurfaces();
    fp->Write(&bMeshHeader, sizeof(bMeshHeader));

    if (bMeshHeader.numJoints > 0) {
        // --- joints ---
        for (int jointIndex = 0; jointIndex < bMeshHeader.numJoints; jointIndex++) {
            const Joint *joint = &joints[jointIndex];

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
        fp->Write(&bMeshSurf, sizeof(bMeshSurf));

        // --- vertexes ---
        for (int i = 0; i < subMesh->numVerts; i++) {
            const VertexLightingGeneric *v  = &subMesh->verts[i];
            const Vec3 position     = v->GetPosition();
            const Vec2 texCoord     = v->GetTexCoord();
            const Vec3 normal       = v->GetNormal();
            const Vec3 tangent      = v->GetTangent();
            const Vec3 biTangent    = v->GetBiTangent();
            
            fp->Write(&position, sizeof(Vec3));
            fp->Write(&texCoord, sizeof(Vec2));
            fp->Write(&normal, sizeof(Vec3));
            fp->Write(&tangent, sizeof(Vec3));
            fp->Write(&biTangent, sizeof(Vec3));
            fp->WriteUInt32(subMesh->verts[i].GetColor());
        }

        // --- vertex weights ---
        if (bMeshSurf.maxWeights > 0) {
            if (bMeshSurf.maxWeights == 1) {
                VertexWeight1 *vw = (VertexWeight1 *)subMesh->vertWeights;
                for (int i = 0; i < bMeshSurf.numVerts; i++, vw++) {
                    fp->WriteUChar(vw->index);
                }
            } else if (bMeshSurf.maxWeights <= 4) {
                VertexWeight4 *vw = (VertexWeight4 *)subMesh->vertWeights;
                for (int i = 0; i < bMeshSurf.numVerts; i++, vw++) {
                    fp->WriteUChar(vw->index[0]);
                    fp->WriteUChar(vw->index[1]);
                    fp->WriteUChar(vw->index[2]);
                    fp->WriteUChar(vw->index[3]);

                    fp->WriteUChar(vw->weight[0]);
                    fp->WriteUChar(vw->weight[1]);
                    fp->WriteUChar(vw->weight[2]);
                    fp->WriteUChar(vw->weight[3]);
                }
            } else if (bMeshSurf.maxWeights <= 8) {
                VertexWeight8 *vw = (VertexWeight8 *)subMesh->vertWeights;
                for (int i = 0; i < bMeshSurf.numVerts; i++, vw++) {
                    fp->WriteUChar(vw->index[0]);
                    fp->WriteUChar(vw->index[1]);
                    fp->WriteUChar(vw->index[2]);
                    fp->WriteUChar(vw->index[3]);
                    fp->WriteUChar(vw->index[4]);
                    fp->WriteUChar(vw->index[5]);
                    fp->WriteUChar(vw->index[6]);
                    fp->WriteUChar(vw->index[7]);

                    fp->WriteUChar(vw->weight[0]);
                    fp->WriteUChar(vw->weight[1]);
                    fp->WriteUChar(vw->weight[2]);
                    fp->WriteUChar(vw->weight[3]);
                    fp->WriteUChar(vw->weight[4]);
                    fp->WriteUChar(vw->weight[5]);
                    fp->WriteUChar(vw->weight[6]);
                    fp->WriteUChar(vw->weight[7]);
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
    }

    fileSystem.CloseFile(fp);
}
    
BE_NAMESPACE_END
