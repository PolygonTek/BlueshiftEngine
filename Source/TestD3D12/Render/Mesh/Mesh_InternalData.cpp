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
#include "meshoptimizer.h"
#include <cmath>
#include "Render/Animation/Skeleton.h"

static BE1::AABB mergeAABB;

#ifdef PACKED_VERTEX_TANGENT_FRAME
// Direct decoding requires the disk layout to match every runtime vertex field.
static_assert(sizeof(BMeshVert) == sizeof(BE1::VertexGenericLit), "Mismatched bmesh vertex size");
static_assert(offsetof(BMeshVert, position) == offsetof(BE1::VertexGenericLit, xyz), "Mismatched bmesh position offset");
static_assert(offsetof(BMeshVert, texCoord) == offsetof(BE1::VertexGenericLit, st), "Mismatched bmesh UV offset");
static_assert(offsetof(BMeshVert, color) == offsetof(BE1::VertexGenericLit, color), "Mismatched bmesh color offset");
static_assert(offsetof(BMeshVert, tangentFrame) == offsetof(BE1::VertexGenericLit, tangentFrame), "Mismatched bmesh tangent offset");
#endif

static bool ReadBMeshVertices(BE1::VertexGenericLit *verts, int numVerts, const byte *data, size_t dataSize, bool compressed) {
    if (numVerts == 0) {
        return true;
    }
#ifdef PACKED_VERTEX_TANGENT_FRAME
    if (compressed) {
        return meshopt_decodeVertexBuffer(verts, numVerts, sizeof(BMeshVert), data, dataSize) == 0;
    }
    memcpy(verts, data, dataSize);
#else
    BE1::Array<BMeshVert> decodedVerts;
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
        BE1::Vec3 normal, tangent;
        float bitangentSign;
        BE1::PackedTangentFrame::Unpack(bMeshVert.tangentFrame, normal, tangent, bitangentSign);
        verts[i].SetTangentFrame(normal, tangent, bitangentSign);
    }
#endif
    return true;
}

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
    hasLODs = false;
    aabb = BE1::AABB::empty;
}

void Mesh::InternalData::Swap(InternalData &rhs) {
    surfaces.Swap(rhs.surfaces);
    BE1::Swap(numJoints, rhs.numJoints);
    BE1::Swap(hasLODs, rhs.hasLODs);
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
    const size_t fileSize = BE1::fileSystem.LoadFile(binaryMeshFilename, true, (void **)&data);
    if (!data) {
        return false;
    }

    if (fileSize < sizeof(BMeshHeader)) {
        BE_WARNLOG("Mesh::InternalData::LoadBinaryMesh: truncated header %s\n", binaryMeshFilename.c_str());
        BE1::fileSystem.FreeFile(data);
        return false;
    }

    const BMeshHeader *bMeshHeader = (const BMeshHeader *)data;
    byte *ptr = data + sizeof(BMeshHeader);

    if (bMeshHeader->ident != BMESH_IDENT) {
        BE_WARNLOG("Mesh::InternalData::LoadBinaryMesh: bad format %s\n", binaryMeshFilename.c_str());
        BE1::fileSystem.FreeFile(data);
        return false;
    }

    if (bMeshHeader->version < 1 || bMeshHeader->version > BMESH_VERSION) {
        BE_WARNLOG("Mesh::InternalData::LoadBinaryMesh: unsupported version %i in %s\n", bMeshHeader->version, binaryMeshFilename.c_str());
        BE1::fileSystem.FreeFile(data);
        return false;
    }

    const byte *end = data + fileSize;

    auto fail = [&](const char *reason) {
        BE_WARNLOG("Mesh::InternalData::LoadBinaryMesh: %s in %s\n", reason, binaryMeshFilename.c_str());
        BE1::fileSystem.FreeFile(data);
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
        joints = new Joint[numJoints];

        // --- joints ---
        for (int jointIndexes = 0; jointIndexes < numJoints; jointIndexes++) {
            const BJoint *bJoint = (const BJoint *)ptr;

            if (!memchr(bJoint->name, 0, sizeof(bJoint->name)) ||
                bJoint->parentIndex < -1 || bJoint->parentIndex >= numJoints) {
                return fail("invalid joint data");
            }

            joints[jointIndexes].name = bJoint->name;
            joints[jointIndexes].parent = bJoint->parentIndex >= 0 ? &joints[bJoint->parentIndex] : nullptr;

            ptr += sizeof(BJoint);
        }
    }

    aabb = BE1::AABB(bMeshHeader->boundsMin, bMeshHeader->boundsMax);

    // --- surfaces ---
    for (int surfaceIndex = 0; surfaceIndex < bMeshHeader->numSurfs; surfaceIndex++) {
        if (size_t(end - ptr) < surfaceHeaderSize) {
            return fail("truncated surface header");
        }
        const BMeshSurf *bMeshSurf = reinterpret_cast<const BMeshSurf *>(ptr);
        ptr += sizeof(BMeshSurf);

        const size_t vertexSize = bMeshHeader->version == 1 ? sizeof(BMeshVertV1) : sizeof(BMeshVert);
        if (bMeshSurf->numVerts > INT_MAX / BE1::Max(vertexSize, sizeof(BE1::VertexGenericLit)) ||
            bMeshSurf->numIndexes > INT_MAX / sizeof(BE1::VertIndex) ||
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

            if (lodHeader.numLods > SubMesh::MaxLODLevel || (lodHeader.numLods > 0 && (lodHeader.numLods >= bMeshSurf->numIndexes / 3 || lodHeader.numLods > size_t(end - ptr) / sizeof(BMeshLod)))) {
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
        subMesh->boundingBox = BE1::AABB(bMeshSurf->boundsMin, bMeshSurf->boundsMax);
        const BE1::Vec3 sphereCenter = (bMeshSurf->boundsMin + bMeshSurf->boundsMax) * 0.5f;
        subMesh->boundingSphere = BE1::Sphere(sphereCenter, (bMeshSurf->boundsMax - sphereCenter).Length());

        meshSurf->materialIndex = bMeshSurf->materialIndex;

        // --- vertexes ---
        if (bMeshHeader->version == 1) {
            const BMeshVertV1 *bMeshVerts = (const BMeshVertV1 *)ptr;
            for (int i = 0; i < bMeshSurf->numVerts; i++) {
                BE1::VertexGenericLit *v = &subMesh->verts[i];
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
        if (compression.flags & BMeshSurfCompression::IndexBuffer) {
            if (meshopt_decodeIndexBuffer(subMesh->indexes, bMeshSurf->numIndexes, sizeof(BE1::VertIndex), ptr, storedIndexSize) != 0) {
                return fail("invalid compressed index data");
            }
        } else if (bMeshSurf->indexSize == sizeof(BE1::VertIndex)) {
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
                if (meshopt_decodeIndexBuffer(lod.indexes.Ptr(), bMeshLod.numIndexes, sizeof(BE1::VertIndex), ptr, bMeshLod.indexDataSize) != 0) {
                    return fail("invalid compressed LOD index data");
                }
            } else if (bMeshSurf->indexSize == sizeof(BE1::VertIndex)) {
                memcpy(lod.indexes.Ptr(), ptr, rawSize);
            } else {
                for (uint32_t i = 0; i < bMeshLod.numIndexes; i++) {
                    uint16_t index;
                    memcpy(&index, ptr + size_t(i) * sizeof(index), sizeof(index));
                    lod.indexes[i] = index;
                }
            }

            for (BE1::VertIndex index : lod.indexes) {
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
    bMeshHeader.boundsMin = aabb[0];
    bMeshHeader.boundsMax = aabb[1];
    fp->Write(&bMeshHeader, sizeof(bMeshHeader));

    if (bMeshHeader.numJoints > 0) {
        // --- joints ---
        for (int jointIndexes = 0; jointIndexes < bMeshHeader.numJoints; jointIndexes++) {
            const Joint *joint = &joints[jointIndexes];

            BJoint bJoint = {};
            BE1::Str::Copynz(bJoint.name, joint->name, sizeof(bJoint.name));
            bJoint.parentIndex = !joint->parent ? -1 : (int32_t)(joint->parent - joints);
            fp->Write(&bJoint, sizeof(bJoint));
        }
    }

    std::unique_ptr<byte[]> encodedData;
    size_t encodedCapacity = 0;

    // --- surfaces ---
    for (int surfaceIndex = 0; surfaceIndex < bMeshHeader.numSurfs; surfaceIndex++) {
        const Surface *meshSurf = surfaces[surfaceIndex];
        const SubMesh *subMesh = meshSurf->subMesh;

        BMeshSurf bMeshSurf;
        bMeshSurf.materialIndex = meshSurf->materialIndex;
        bMeshSurf.numVerts = subMesh->numVerts;
        bMeshSurf.numIndexes = subMesh->numIndexes;
        bMeshSurf.indexSize = subMesh->numVerts <= BIT(16) ? sizeof(uint16_t) : sizeof(uint32_t);
        bMeshSurf.maxWeights = subMesh->MaxVertexWeights();
        bMeshSurf.boundsMin = subMesh->GetBoundingBox()[0];
        bMeshSurf.boundsMax = subMesh->GetBoundingBox()[1];

        BMeshSurfCompression compression = {};
        const size_t vertexDataSize = size_t(subMesh->numVerts) * sizeof(BMeshVert);
        const size_t indexDataSize = size_t(subMesh->numIndexes) * bMeshSurf.indexSize;

        if (vertexDataSize > UINT32_MAX || indexDataSize > UINT32_MAX || subMesh->numIndexes % 3 != 0) {
            BE_WARNLOG("Mesh::InternalData::WriteBinaryMesh: invalid surface size\n");
            BE1::fileSystem.CloseFile(fp);
            return;
        }

        if (subMesh->NumLODs() > SubMesh::MaxLODLevel + 1) {
            BE_WARNLOG("Mesh::InternalData::WriteBinaryMesh: too many LOD levels\n");
            BE1::fileSystem.CloseFile(fp);
            return;
        }

        int previousIndexCount = subMesh->NumIndexes();
        float previousError = 0.0f;

        for (int level = 1; level < subMesh->NumLODs(); level++) {
            const int indexCount = subMesh->NumIndexes(level);
            const float error = subMesh->GetLODError(level);

            if (indexCount <= 0 || indexCount >= previousIndexCount || indexCount % 3 != 0 || !std::isfinite(error) || error < previousError) {
                BE_WARNLOG("Mesh::WriteBinaryMesh: invalid LOD layout\n");
                BE1::fileSystem.CloseFile(fp);
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
        BE1::Array<BMeshVert> packedVerts;
        packedVerts.SetCount(subMesh->numVerts);
        for (int i = 0; i < subMesh->numVerts; i++) {
            const BE1::VertexGenericLit *v = &subMesh->verts[i];
            BMeshVert &bMeshVert = packedVerts[i];
            bMeshVert.position = v->GetPosition();
            bMeshVert.texCoord[0] = v->st[0];
            bMeshVert.texCoord[1] = v->st[1];
            bMeshVert.color = v->GetColor();
            bMeshVert.tangentFrame = BE1::PackedTangentFrame::Pack(v->GetNormal(), v->GetTangent(), v->GetBiTangentSign());
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
        if (compression.flags & BMeshSurfCompression::IndexBuffer) {
            fp->Write(encodedData.get() + vertexBound, compression.indexDataSize);
        } else if (bMeshSurf.indexSize == sizeof(BE1::VertIndex)) {
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
            const BE1::VertIndex *lodIndexes = subMesh->Indexes(level);

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
            } else if (bMeshSurf.indexSize == sizeof(BE1::VertIndex)) {
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

    if (BE1::HasFlag(finishFlags, FinishFlag::OptimizeIndices)) {
        OptimizeIndexedTriangles();
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

    // 프레임마다 LOD 유무를 확인하기 위해 surface 들을 순회하지 않도록 저장한다.
    hasLODs = false;
    for (const Surface *surface : surfaces) {
        if (surface && surface->subMesh && surface->subMesh->NumLODs() > 1) {
            hasLODs = true;
            break;
        }
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

void Mesh::InternalData::OptimizeIndexedTriangles() {
    for (int surfaceIndex = 0; surfaceIndex < surfaces.Count(); surfaceIndex++) {
        SubMesh *subMesh = surfaces[surfaceIndex]->subMesh;
        subMesh->OptimizeIndexedTriangles();
    }
}

bool Mesh::InternalData::IsCompatibleSkeleton(const Skeleton *skeleton) const {
    if (!skeleton || !skeleton->IsReady()) {
        return false;
    }

    if (numJoints != skeleton->NumJoints()) {
        return false;
    }

    const Joint *otherJoints = skeleton->GetJoints();

    for (int jointIndex = 0; jointIndex < numJoints; ++jointIndex) {
        const Joint *joint = &joints[jointIndex];
        const Joint *otherJoint = &otherJoints[jointIndex];

        if (joint->name != otherJoint->name) {
            return false;
        }

        const int parentIndex = joint->parent ? static_cast<int>(joint->parent - joints) : -1;
        const int otherParentIndex = otherJoint->parent ? static_cast<int>(otherJoint->parent - otherJoints) : -1;
        if (parentIndex != otherParentIndex) {
            return false;
        }
    }

    return true;
}

void Mesh::InternalData::ComputeAABB() {
    aabb.Clear();

    for (Surface *surface : surfaces) {
        SubMesh *subMesh = surface->subMesh;
        subMesh->ComputeBounds();

        aabb.AddAABB(subMesh->boundingBox);
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
    const BE1::AABB &aAABB = (a)->subMesh->GetBoundingBox();
    const BE1::AABB &bAABB = (b)->subMesh->GetBoundingBox();

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
                mergeAABB = surfaces[m0]->subMesh->boundingBox;
                surfaces.SortSubSection(m0, m2, ListSortCompareMergedAABBArea<MeshSurfPtr>);
            }

            // 합칠 기준 AABB 를 확장
            BE1::AABB aabb1 = surfaces[m0]->subMesh->boundingBox;
            aabb1.ExpandSelf(BE1::MeterToUnit(10.0f));

            while (m1 + 1 < surfaces.Count() && 
                surfaces[m0]->materialIndex == surfaces[m1 + 1]->materialIndex && 
                numNewIndexes < 32768) {
                BE1::AABB aabb2 = surfaces[m1 + 1]->subMesh->boundingBox;

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
