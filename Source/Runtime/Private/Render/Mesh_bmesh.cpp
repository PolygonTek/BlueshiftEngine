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
#include "Render/Render.h"
#include "BModel.h"
#include "meshoptimizer.h"
#include <cmath>
#include "Core/Heap.h"
#include "IO/FileSystem.h"

BE_NAMESPACE_BEGIN

#ifdef PACKED_VERTEX_TANGENT_FRAME
// Direct decoding requires the disk layout to match every runtime vertex field.
static_assert(sizeof(BMeshVert) == sizeof(VertexGenericLit), "Mismatched bmesh vertex size");
static_assert(offsetof(BMeshVert, position) == offsetof(VertexGenericLit, xyz), "Mismatched bmesh position offset");
static_assert(offsetof(BMeshVert, texCoord) == offsetof(VertexGenericLit, st), "Mismatched bmesh UV offset");
static_assert(offsetof(BMeshVert, color) == offsetof(VertexGenericLit, color), "Mismatched bmesh color offset");
static_assert(offsetof(BMeshVert, tangentFrame) == offsetof(VertexGenericLit, tangentFrame), "Mismatched bmesh tangent offset");
#endif

static bool ReadBMeshVertices(VertexGenericLit *verts, int numVerts, const byte *data, size_t dataSize, bool compressed) {
    if (numVerts == 0) {
        return true;
    }
#ifdef PACKED_VERTEX_TANGENT_FRAME
    if (compressed) {
        return meshopt_decodeVertexBuffer(verts, numVerts, sizeof(BMeshVert), data, dataSize) == 0;
    }
    memcpy(verts, data, dataSize);
#else
    Array<BMeshVert> decodedVerts;
    if (compressed) {
        decodedVerts.SetCount(numVerts);
        if (meshopt_decodeVertexBuffer(decodedVerts.Ptr(), numVerts, sizeof(BMeshVert), data, dataSize) != 0) {
            return false;
        }
        data = reinterpret_cast<const byte *>(decodedVerts.Ptr());
    }
    const BMeshVert *bMeshVerts = reinterpret_cast<const BMeshVert *>(data);
    for (int i = 0; i < numVerts; i++) {
        const BMeshVert &bMeshVert = bMeshVerts[i];
        verts[i].SetPosition(bMeshVert.position);
        verts[i].st[0] = bMeshVert.texCoord[0];
        verts[i].st[1] = bMeshVert.texCoord[1];
        verts[i].SetColor(bMeshVert.color);
        Vec3 normal, tangent;
        float bitangentSign;
        PackedTangentFrame::Unpack(bMeshVert.tangentFrame, normal, tangent, bitangentSign);
        verts[i].SetTangentFrame(normal, tangent, bitangentSign);
    }
#endif
    return true;
}

bool Mesh::LoadBinaryMesh(const char *filename) {
    byte *data;
    const size_t fileSize = fileSystem.LoadFile(filename, true, (void **)&data);
    if (!data) {
        return false;
    }
 
    if (fileSize < sizeof(BMeshHeader)) {
        BE_WARNLOG("Mesh::LoadBinaryMesh: truncated header %s\n", filename);
        fileSystem.FreeFile(data);
        return false;
    }

    const BMeshHeader *bMeshHeader = (const BMeshHeader *)data;
    byte *ptr = data + sizeof(BMeshHeader);
    
    if (bMeshHeader->ident != BMESH_IDENT) {
        BE_WARNLOG("Mesh::LoadBinaryMesh: bad format %s\n", filename);
        fileSystem.FreeFile(data);
        return false;
    }

    if (bMeshHeader->version < 1 || bMeshHeader->version > BMESH_VERSION) {
        BE_WARNLOG("Mesh::LoadBinaryMesh: unsupported version %i in %s\n", bMeshHeader->version, filename);
        fileSystem.FreeFile(data);
        return false;
    }

    const byte *end = data + fileSize;

    auto fail = [&](const char *reason) {
        BE_WARNLOG("Mesh::LoadBinaryMesh: %s in %s\n", reason, filename);
        fileSystem.FreeFile(data);
        Purge();
        return false;
    };

    if (bMeshHeader->numJoints > INT_MAX / sizeof(Joint) ||
        bMeshHeader->numJoints > size_t(end - ptr) / sizeof(BJoint)) {
        return fail("invalid joint count");
    }
    const size_t jointDataSize = size_t(bMeshHeader->numJoints) * sizeof(BJoint);
    const size_t surfaceHeaderSize = sizeof(BMeshSurf) + (bMeshHeader->version >= 3 ? sizeof(BMeshSurfCompression) : 0) + (bMeshHeader->version >= 4 ? sizeof(BMeshSurfLods) : 0);
    if (bMeshHeader->numSurfs > INT_MAX ||
        bMeshHeader->numSurfs > (size_t(end - ptr) - jointDataSize) / surfaceHeaderSize) {
        return fail("invalid surface count");
    }

    numJoints = bMeshHeader->numJoints;
    if (numJoints > 0) {
        joints = BE_NEW Joint[numJoints];

        // --- joints ---
        for (int jointIndexes = 0; jointIndexes < numJoints; jointIndexes++) {
            const BJoint *bJoint = (const BJoint *)ptr;

            if (!memchr(bJoint->name, 0, sizeof(bJoint->name)) ||
                bJoint->parentIndex < -1 || bJoint->parentIndex >= numJoints) {
                return fail("invalid joint data");
            }

            joints[jointIndexes].name = bJoint->name;
            joints[jointIndexes].parent = bJoint->parentIndex >= 0 ? &this->joints[bJoint->parentIndex] : nullptr;

            ptr += sizeof(BJoint);
        }
    }

    aabb = AABB(bMeshHeader->aabbMin, bMeshHeader->aabbMax);

    // --- surfaces ---
    for (int surfaceIndex = 0; surfaceIndex < bMeshHeader->numSurfs; surfaceIndex++) {
        if (size_t(end - ptr) < surfaceHeaderSize) {
            return fail("truncated surface header");
        }
        const BMeshSurf *bMeshSurf = reinterpret_cast<const BMeshSurf *>(ptr);
        ptr += sizeof(BMeshSurf);

        const size_t vertexSize = bMeshHeader->version == 1 ? sizeof(BMeshVertV1) : sizeof(BMeshVert);
        if (bMeshSurf->numVerts > INT_MAX / Max(vertexSize, sizeof(VertexGenericLit)) ||
            bMeshSurf->numIndexes > INT_MAX / sizeof(VertIndex) ||
            bMeshSurf->numIndexes % 3 != 0 || (bMeshSurf->numIndexes > 0 && bMeshSurf->numVerts == 0) ||
            (bMeshSurf->indexSize != 2 && bMeshSurf->indexSize != 4) || bMeshSurf->maxWeights > 8) {
            return fail("invalid surface layout");
        }

        const size_t vertexDataSize = size_t(bMeshSurf->numVerts) * vertexSize;
        const size_t indexDataSize = size_t(bMeshSurf->numIndexes) * bMeshSurf->indexSize;
        const size_t weightSize = bMeshSurf->maxWeights == 0 ? 0 : (bMeshSurf->maxWeights == 1 ? 1 : (bMeshSurf->maxWeights <= 4 ? 8 : 16));
        const size_t weightDataSize = size_t(bMeshSurf->numVerts) * weightSize;
        BMeshSurfCompression compression = {};
        size_t storedVertexSize = vertexDataSize;
        size_t storedIndexSize = indexDataSize;

        if (bMeshHeader->version >= 3) {
            memcpy(&compression, ptr, sizeof(compression));
            ptr += sizeof(compression);
            storedVertexSize = compression.vertexDataSize;
            storedIndexSize = compression.indexDataSize;
            if (compression.flags & ~(BMeshSurfCompression::VertexBuffer | BMeshSurfCompression::IndexBuffer)) {
                return fail("unsupported compression flags");
            }

            if (compression.flags & BMeshSurfCompression::VertexBuffer) {
                if (bMeshSurf->numVerts == 0 || storedVertexSize == 0) {
                    return fail("invalid compressed vertex size");
                }
            } else if (storedVertexSize != vertexDataSize) {
                return fail("invalid raw vertex size");
            }

            if (compression.flags & BMeshSurfCompression::IndexBuffer) {
                if (bMeshSurf->numIndexes == 0 || storedIndexSize == 0) {
                    return fail("invalid compressed index size");
                }
            } else if (storedIndexSize != indexDataSize) {
                return fail("invalid raw index size");
            }
        }

        BMeshSurfLods lodHeader = {};
        if (bMeshHeader->version >= 4) {
            memcpy(&lodHeader, ptr, sizeof(lodHeader));
            ptr += sizeof(lodHeader);

            if (lodHeader.numLods > 0 && (lodHeader.numLods >= bMeshSurf->numIndexes / 3 ||
                lodHeader.numLods > size_t(end - ptr) / sizeof(BMeshLod))) {
                return fail("invalid LOD count");
            }
        }

        const size_t remaining = size_t(end - ptr);
        if (storedVertexSize > remaining || weightDataSize > remaining - storedVertexSize ||
            storedIndexSize > remaining - storedVertexSize - weightDataSize) {
            return fail("truncated surface data");
        }

        Surface *meshSurf = Mesh::AllocSurface(bMeshSurf->numVerts, bMeshSurf->numIndexes);
        surfaces.Append(meshSurf);
        SubMesh *subMesh = meshSurf->subMesh;
        subMesh->aabb = AABB(bMeshSurf->aabbMin, bMeshSurf->aabbMax);

        meshSurf->materialIndex = bMeshSurf->materialIndex;

        // --- vertexes ---
        if (bMeshHeader->version == 1) {
            const BMeshVertV1 *bMeshVerts = (const BMeshVertV1 *)ptr;
            for (int i = 0; i < bMeshSurf->numVerts; i++) {
                VertexGenericLit *v = &subMesh->verts[i];
                const BMeshVertV1 &bMeshVert = bMeshVerts[i];
                v->SetPosition(bMeshVert.position);
                v->SetTexCoord(bMeshVert.texCoord);
                v->SetTangentFrame(bMeshVert.normal, bMeshVert.tangent, bMeshVert.bitangent);
                v->SetColor(bMeshVert.color);
            }
            ptr += sizeof(BMeshVertV1) * bMeshSurf->numVerts;
        } else {
            if (!ReadBMeshVertices(subMesh->verts, bMeshSurf->numVerts, ptr, storedVertexSize,
                (compression.flags & BMeshSurfCompression::VertexBuffer) != 0)) {
                return fail("invalid compressed vertex data");
            }
            ptr += storedVertexSize;
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
        if (compression.flags & BMeshSurfCompression::IndexBuffer) {
            if (meshopt_decodeIndexBuffer(subMesh->indexes, bMeshSurf->numIndexes, sizeof(VertIndex), ptr, storedIndexSize) != 0) {
                return fail("invalid compressed index data");
            }
        } else if (bMeshSurf->indexSize == sizeof(VertIndex)) {
            if (storedIndexSize > 0) {
                memcpy(subMesh->indexes, ptr, storedIndexSize);
            }
        } else {
            for (int i = 0; i < bMeshSurf->numIndexes; i++) {
                uint16_t index;
                memcpy(&index, ptr + size_t(i) * sizeof(index), sizeof(index));
                subMesh->indexes[i] = index;
            }
        }
        ptr += storedIndexSize;

        // A successful codec decode does not guarantee that indices reference existing vertices.
        for (int i = 0; i < bMeshSurf->numIndexes; i++) {
            if (subMesh->indexes[i] >= bMeshSurf->numVerts) {
                return fail("vertex index out of range");
            }
        }

        // LOD 정점은 따로 할당하지 않고 인덱스만 읽는다.
        uint32_t previousIndexCount = bMeshSurf->numIndexes;
        float previousError = 0.0f;
        for (uint32_t level = 0; level < lodHeader.numLods; level++) {
            if (size_t(end - ptr) < sizeof(BMeshLod)) {
                return fail("truncated LOD header");
            }

            BMeshLod bMeshLod;
            memcpy(&bMeshLod, ptr, sizeof(bMeshLod));
            ptr += sizeof(bMeshLod);

            if (bMeshLod.numIndexes == 0 || bMeshLod.numIndexes >= previousIndexCount || bMeshLod.numIndexes % 3 != 0 ||
                (bMeshLod.flags & ~BMeshLod::Compressed) || !std::isfinite(bMeshLod.error) || bMeshLod.error < previousError) {
                return fail("invalid LOD layout");
            }

            const size_t rawSize = size_t(bMeshLod.numIndexes) * bMeshSurf->indexSize;
            if (bMeshLod.indexDataSize > size_t(end - ptr) ||
                ((bMeshLod.flags & BMeshLod::Compressed) ? bMeshLod.indexDataSize == 0 : bMeshLod.indexDataSize != rawSize)) {
                return fail("invalid LOD index size");
            }

            SubMesh::LOD &lod = subMesh->lods.Alloc();
            lod.indexes.SetCount(static_cast<int>(bMeshLod.numIndexes));
            lod.error = bMeshLod.error;

            if (bMeshLod.flags & BMeshLod::Compressed) {
                if (meshopt_decodeIndexBuffer(lod.indexes.Ptr(), bMeshLod.numIndexes, sizeof(VertIndex), ptr, bMeshLod.indexDataSize) != 0) {
                    return fail("invalid compressed LOD index data");
                }
            } else if (bMeshSurf->indexSize == sizeof(VertIndex)) {
                memcpy(lod.indexes.Ptr(), ptr, rawSize);
            } else {
                for (uint32_t i = 0; i < bMeshLod.numIndexes; i++) {
                    uint16_t index;
                    memcpy(&index, ptr + size_t(i) * sizeof(index), sizeof(index));
                    lod.indexes[i] = index;
                }
            }

            for (VertIndex index : lod.indexes) {
                if (index >= bMeshSurf->numVerts) {
                    return fail("LOD vertex index out of range");
                }
            }

            ptr += bMeshLod.indexDataSize;
            previousIndexCount = bMeshLod.numIndexes;
            previousError = bMeshLod.error;
        }

        // guarantee 8 bytes aligned read
        const size_t padding = (8 - (size_t(ptr - data) & 7)) & 7;
        if (padding > size_t(end - ptr)) {
            return fail("truncated surface padding");
        }
        ptr += padding;
    }

    fileSystem.FreeFile(data);

    FinishSurfaces(BE1::Mesh::FinishFlag::ComputeEdges);

    return true;
}

void Mesh::WriteBinaryMesh(const char *filename) {
    File *fp = fileSystem.OpenFile(filename, File::Mode::Write);
    if (!fp) {
        BE_WARNLOG("Mesh::WriteBinaryMesh: file open error\n");
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

            BJoint bJoint = {};
            Str::Copynz(bJoint.name, joint->name, sizeof(bJoint.name));
            bJoint.parentIndex = !joint->parent ? -1 : (int32_t)(joint->parent - joints);
            fp->Write(&bJoint, sizeof(bJoint));
        }
    }

    std::unique_ptr<byte[]> encodedData;
    size_t encodedCapacity = 0;

    // --- surfaces ---
    for (int surfaceIndex = 0; surfaceIndex < bMeshHeader.numSurfs; surfaceIndex++) {
        const Surface *meshSurf = GetSurface(surfaceIndex);
        const SubMesh *subMesh = meshSurf->subMesh;

        BMeshSurf bMeshSurf;
        bMeshSurf.materialIndex     = meshSurf->materialIndex;
        bMeshSurf.numVerts          = subMesh->numVerts;
        bMeshSurf.numIndexes        = subMesh->numIndexes;
        bMeshSurf.indexSize         = subMesh->numVerts <= BIT(16) ? sizeof(uint16_t) : sizeof(uint32_t);
        bMeshSurf.maxWeights        = subMesh->MaxVertexWeights();
        bMeshSurf.aabbMin           = subMesh->GetAABB()[0];
        bMeshSurf.aabbMax           = subMesh->GetAABB()[1];

        BMeshSurfCompression compression = {};
        const size_t vertexDataSize = size_t(subMesh->numVerts) * sizeof(BMeshVert);
        const size_t indexDataSize = size_t(subMesh->numIndexes) * bMeshSurf.indexSize;

        if (vertexDataSize > UINT32_MAX || indexDataSize > UINT32_MAX || subMesh->numIndexes % 3 != 0) {
            BE_WARNLOG("Mesh::WriteBinaryMesh: invalid surface size\n");
            fileSystem.CloseFile(fp);
            return;
        }

        int previousIndexCount = subMesh->NumIndexes();
        float previousError = 0.0f;

        for (int level = 1; level < subMesh->NumLODs(); level++) {
            const int indexCount = subMesh->NumIndexes(level);
            const float error = subMesh->GetLODError(level);

            if (indexCount <= 0 || indexCount >= previousIndexCount || indexCount % 3 != 0 || !std::isfinite(error) || error < previousError) {
                BE_WARNLOG("Mesh::WriteBinaryMesh: invalid LOD layout\n");
                fileSystem.CloseFile(fp);
                return;
            }

            previousIndexCount = indexCount;
            previousError = error;
        }

        compression.vertexDataSize = static_cast<uint32_t>(vertexDataSize);
        compression.indexDataSize = static_cast<uint32_t>(indexDataSize);

        // Reuse uninitialized scratch storage across surfaces; the encoder writes every stored byte.
        const size_t vertexBound = subMesh->numVerts > 0 ? meshopt_encodeVertexBufferBound(subMesh->numVerts, sizeof(BMeshVert)) : 0;
        const size_t indexBound = subMesh->numIndexes > 0 ? meshopt_encodeIndexBufferBound(subMesh->numIndexes, subMesh->numVerts) : 0;
        const size_t requiredCapacity = vertexBound + indexBound;

        if (requiredCapacity > encodedCapacity) {
            encodedData.reset(new byte[requiredCapacity]);
            encodedCapacity = requiredCapacity;
        }

#ifdef PACKED_VERTEX_TANGENT_FRAME
        const void *vertexData = subMesh->verts;
#else
        Array<BMeshVert> packedVerts;
        packedVerts.SetCount(subMesh->numVerts);
        for (int i = 0; i < subMesh->numVerts; i++) {
            const VertexGenericLit *v = &subMesh->verts[i];
            BMeshVert &bMeshVert = packedVerts[i];
            bMeshVert.position = v->GetPosition();
            bMeshVert.texCoord[0] = v->st[0];
            bMeshVert.texCoord[1] = v->st[1];
            bMeshVert.color = v->GetColor();
            bMeshVert.tangentFrame = PackedTangentFrame::Pack(v->GetNormal(), v->GetTangent(), v->GetBiTangentSign());
        }
        const void *vertexData = packedVerts.Ptr();
#endif
        if (vertexBound > 0) {
            // Pin the codec format per call; global encoder settings are unsafe during parallel imports.
            const size_t encodedSize = meshopt_encodeVertexBufferLevel(encodedData.get(), vertexBound,
                vertexData, subMesh->numVerts, sizeof(BMeshVert), 2, 1);
            if (encodedSize > 0 && encodedSize < vertexDataSize) {
                compression.flags |= BMeshSurfCompression::VertexBuffer;
                compression.vertexDataSize = static_cast<uint32_t>(encodedSize);
                vertexData = encodedData.get();
            }
        }
        if (indexBound > 0) {
            // Triangle order and winding are preserved, but the codec may rotate triangle corners.
            const size_t encodedSize = meshopt_encodeIndexBuffer(encodedData.get() + vertexBound, indexBound,
                subMesh->indexes, subMesh->numIndexes);
            if (encodedSize > 0 && encodedSize < indexDataSize) {
                compression.flags |= BMeshSurfCompression::IndexBuffer;
                compression.indexDataSize = static_cast<uint32_t>(encodedSize);
            }
        }
        fp->Write(&bMeshSurf, sizeof(bMeshSurf));
        fp->Write(&compression, sizeof(compression));
        const BMeshSurfLods lodHeader = {static_cast<uint32_t>(subMesh->NumLODs() - 1)};
        fp->Write(&lodHeader, sizeof(lodHeader));

        // --- vertexes ---
        if (compression.vertexDataSize > 0) {
            fp->Write(vertexData, compression.vertexDataSize);
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
        if (compression.flags & BMeshSurfCompression::IndexBuffer) {
            fp->Write(encodedData.get() + vertexBound, compression.indexDataSize);
        } else if (bMeshSurf.indexSize == sizeof(VertIndex)) {
            if (indexDataSize > 0) {
                fp->Write(subMesh->indexes, indexDataSize);
            }
        } else {
            for (int i = 0; i < subMesh->numIndexes; i++) {
                fp->WriteUInt16(subMesh->indexes[i]);
            }
        }

        // 정점/LOD 0 블록을 쓴 뒤 기존 압축 임시 버퍼를 LOD 인덱스에 재사용한다.
        for (int level = 1; level < subMesh->NumLODs(); level++) {
            const VertIndex *lodIndexes = subMesh->Indexes(level);

            BMeshLod bMeshLod = {};
            bMeshLod.numIndexes = subMesh->NumIndexes(level);
            bMeshLod.indexDataSize = bMeshLod.numIndexes * bMeshSurf.indexSize;
            bMeshLod.error = subMesh->GetLODError(level);

            const size_t encodedSize = meshopt_encodeIndexBuffer(encodedData.get(), encodedCapacity, lodIndexes, bMeshLod.numIndexes);
            if (encodedSize > 0 && encodedSize < bMeshLod.indexDataSize) {
                bMeshLod.flags = BMeshLod::Compressed;
                bMeshLod.indexDataSize = static_cast<uint32_t>(encodedSize);
            }

            fp->Write(&bMeshLod, sizeof(bMeshLod));

            if (bMeshLod.flags & BMeshLod::Compressed) {
                fp->Write(encodedData.get(), bMeshLod.indexDataSize);
            } else if (bMeshSurf.indexSize == sizeof(VertIndex)) {
                fp->Write(lodIndexes, bMeshLod.indexDataSize);
            } else {
                for (uint32_t i = 0; i < bMeshLod.numIndexes; i++) {
                    fp->WriteUInt16(lodIndexes[i]);
                }
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
