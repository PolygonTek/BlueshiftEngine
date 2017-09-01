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
#include "Core/JointPose.h"
#include "Simd/Simd.h"
#include "Core/Heap.h"

BE_NAMESPACE_BEGIN

bool Mesh::IsDefaultMesh() const {
    return (this == MeshManager::defaultMesh ? true : false);
}

void Mesh::Purge() {
    for (int i = 0; i < surfaces.Count(); i++) {
        FreeSurface(surfaces[i]);
    }
    surfaces.Clear();

    if (isInstantiated) {
        if (skinningJointCache) {
            if (skinningJointCache->skinningJoints) {
                Mem_AlignedFree(skinningJointCache->skinningJoints);
            }
            delete skinningJointCache;
            skinningJointCache = nullptr;
        }

        if (originalMesh) {
            originalMesh->instantiatedMeshes.Remove(this);
            originalMesh = nullptr;
        }
    } else {
        SAFE_DELETE_ARRAY(joints);
    }
}

MeshSurf *Mesh::AllocSurface(int numVerts, int numIndexes) const {
    MeshSurf *surf = new MeshSurf;
    surf->materialIndex = 0;
    surf->subMesh       = new SubMesh;
    surf->drawSurf      = nullptr;
    surf->viewCount     = 0;
    
    surf->subMesh->AllocSubMesh(numVerts, numIndexes);

    return surf;
}

void Mesh::FreeSurface(MeshSurf *surf) const {
    surf->subMesh->FreeSubMesh();

    delete surf->subMesh;

    SAFE_DELETE(surf);
}

MeshSurf *Mesh::AllocInstantiatedSurface(const MeshSurf *refSurf, int meshType) const {
    MeshSurf *surf = new MeshSurf;
    surf->materialIndex = refSurf->materialIndex;
    surf->subMesh       = new SubMesh;
    surf->drawSurf      = nullptr;
    surf->viewCount     = 0;

    surf->subMesh->AllocInstantiatedSubMesh(refSurf->subMesh, meshType);

    return surf;
}

Mesh *Mesh::InstantiateMesh(int meshType) {
    Mesh *mesh = meshManager.AllocInstantiatedMesh(this);

    instantiatedMeshes.Append(mesh);

    mesh->Instantiate(meshType);

    return mesh;
}

void Mesh::Instantiate(int meshType) {
    if (meshType == SkinnedMesh && numJoints > 0) {
        isSkinnedMesh = true;
    } else {
        isStaticMesh = true;
        meshType = StaticMesh; // override to static mesh
    }

    // Free previously allocated skinning joint cache
    if (skinningJointCache) {
        if (skinningJointCache->skinningJoints) {
            Mem_AlignedFree(skinningJointCache->skinningJoints);
        }
        delete skinningJointCache;
        skinningJointCache = nullptr;
    }

    if (isSkinnedMesh) {
        useGpuSkinning = CapableGPUJointSkinning((Mesh::SkinningMethod)renderGlobal.skinningMethod, numJoints);

        if (useGpuSkinning) {
            skinningJointCache = new SkinningJointCache;
            skinningJointCache->viewFrameCount = -1;

            // NOTE: VTF skinning 일 때만 모션블러 함
            if (renderGlobal.skinningMethod == VtfSkinning) {
                skinningJointCache->numJoints = numJoints;
                if (r_motionBlur.GetInteger() == 2) {
                    skinningJointCache->numJoints *= 2;
                }
                skinningJointCache->skinningJoints = (Mat3x4 *)Mem_Alloc16(sizeof(Mat3x4) * skinningJointCache->numJoints);

                skinningJointCache->jointIndexOffsetCurr = 0;
                skinningJointCache->jointIndexOffsetPrev = 0;
            } else {
                skinningJointCache->numJoints = numJoints;
                skinningJointCache->skinningJoints = (Mat3x4 *)Mem_Alloc16(sizeof(Mat3x4) * skinningJointCache->numJoints);
            }
        }
    }

    // Free previously allocated surfaces
    if (surfaces.Count() > 0) {
        for (int i = 0; i < surfaces.Count(); i++) {
            FreeSurface(surfaces[i]);
        }
        surfaces.Clear();
    }

    for (int i = 0; i < originalMesh->surfaces.Count(); i++) {
        MeshSurf *surf = AllocInstantiatedSurface(originalMesh->surfaces[i], meshType);
        surfaces.Append(surf);
    }
}

void Mesh::Reinstantiate() {
    int meshType;
    if (isSkinnedMesh) {
        meshType = SkinnedMesh;
    } else {
        meshType = StaticMesh;
    }

    Instantiate(meshType);
}

void Mesh::FinishSurfaces(int flags) {
    if (flags & SortAndMergeFlag) {
        SortAndMerge();
        ComputeAABB();
    }

    if (flags & ComputeAABBFlag) {
        ComputeAABB();
    }

    //SplitMirroredVerts();

    if (flags & OptimizeIndicesFlag) {
        OptimizeIndexedTriangles();
    }

    if ((flags & ComputeNormalsFlag) && !(flags & ComputeTangentsFlag)) {
        ComputeNormals();
    }

    /*for (int i = 0; i < surfaces.Count(); i++) {
        SubMesh *subMesh = surfaces[i]->subMesh;
        subMesh->FixMirroredVerts();
    }*/

    if (flags & ComputeTangentsFlag) {
        ComputeTangents(flags & ComputeNormalsFlag ? true : false, flags & UseUnsmoothedTangentsFlag ? true : false);
    }

    // TODO: consider to remove this
    ComputeEdges();
}

void Mesh::TransformVerts(const Mat3 &rotation, const Vec3 &translation) {
    for (int i = 0; i < surfaces.Count(); i++) {
        SubMesh *subMesh = surfaces[i]->subMesh;

        for (int j = 0; j < subMesh->numVerts; j++) {
            subMesh->verts[j].Transform(rotation, translation);
        }
    }

    ComputeAABB();
}

void Mesh::OptimizeIndexedTriangles() {
    for (int i = 0; i < surfaces.Count(); i++) {
        SubMesh *subMesh = surfaces[i]->subMesh;
        subMesh->OptimizeIndexedTriangles();
    }
}

void Mesh::Voxelize() {
}

bool Mesh::LineIntersection(const Vec3 &start, const Vec3 &end, bool backFaceCull) const {
    for (int i = 0; i < surfaces.Count(); i++) {
        MeshSurf *surf = surfaces[i];
        if (surf->subMesh->LineIntersection(start, end, backFaceCull)) {
            return true;
        }
    }

    return false;
}

bool Mesh::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &scale) const {
    float smin = Math::Infinity;

    for (int i = 0; i < surfaces.Count(); i++) {
        MeshSurf *surf = surfaces[i];
        if (surf->subMesh->RayIntersection(start, dir, scale, backFaceCull)) {
            if (scale > 0.0f && scale < smin) {
                smin = scale;
            }
        }
    }

    if (smin < Math::Infinity) {
        scale = smin;
        return true;
    }

    return false;
}

void Mesh::SplitMirroredVerts() {
    for (int i = 0; i < surfaces.Count(); i++) {
        surfaces[i]->subMesh->SplitMirroredVerts();
    }
}

void Mesh::ComputeAABB() {
    aabb.Clear();

    for (int i = 0; i < surfaces.Count(); i++) {
        SubMesh *subMesh = surfaces[i]->subMesh;
        subMesh->ComputeAABB();

        aabb.AddAABB(subMesh->aabb);
    }

    // add small epsilon
    aabb.Expand(CentiToUnit(0.01));
}

void Mesh::ComputeNormals() {
    for (int i = 0; i < surfaces.Count(); i++) {
        surfaces[i]->subMesh->ComputeNormals();
    }
}

void Mesh::ComputeTangents(bool includeNormals, bool useUnsmoothedTangents) {
    for (int i = 0; i < surfaces.Count(); i++) {
        MeshSurf *surf = surfaces[i];

        surf->subMesh->ComputeTangents(includeNormals, useUnsmoothedTangents);
    }
}

void Mesh::ComputeEdges() {
    for (int i = 0; i < surfaces.Count(); i++) {
        surfaces[i]->subMesh->ComputeEdges();
    }
}

bool Mesh::IsCompatibleSkeleton(const Skeleton *skeleton) const {
    if (numJoints != skeleton->NumJoints()) {
        return false;
    }
    
    const Joint *otherJoints = skeleton->GetJoints();

    for (int i = 0; i < numJoints; i++) {
        const Joint *joint = &joints[i];
        const Joint *otherJoint = &otherJoints[i];
        
        if (joint->name != otherJoint->name) {
            return false;
        }
        
        int	parentIndex;
        if (joint->parent)  {
            parentIndex = (int)(joint->parent - joints);
        } else {
            parentIndex = -1;
        }
        
        int otherParentIndex;
        if (otherJoint->parent)  {
            otherParentIndex = (int)(otherJoint->parent - otherJoints);
        } else {
            otherParentIndex = -1;
        }
        
        if (parentIndex != otherParentIndex) {
            return false;
        }
    }
    
    return true;
}

bool Mesh::CapableGPUJointSkinning(SkinningMethod skinningMethod, int numJoints) const {
    assert(numJoints > 0 && numJoints < 256);

    if (skinningMethod == VtfSkinning) {
        return true;
    } else if (skinningMethod == VertexShaderSkinning) {
        if (numJoints <= 74) {
            if (rhi.HWLimit().maxVertexUniformComponents >= 256) {
                return true;
            }
        } else if (numJoints <= 256) {
            if (rhi.HWLimit().maxVertexUniformComponents >= 2048) {
                return true;
            }
        }
    }

    return false;
}

void Mesh::UpdateSkinningJointCache(const Skeleton *skeleton, const Mat3x4 *jointMats) {
    if (!useGpuSkinning || !skinningJointCache) {
        return;
    }

    if (skinningJointCache->viewFrameCount == renderSystem.GetCurrentRenderContext()->frameCount) {
        return;
    }

    skinningJointCache->viewFrameCount = renderSystem.GetCurrentRenderContext()->frameCount;

    if (r_usePostProcessing.GetBool() && (r_motionBlur.GetInteger() & 2)) {
        if (skinningJointCache->viewFrameCount == renderSystem.GetCurrentRenderContext()->frameCount) {
            skinningJointCache->jointIndexOffsetPrev = skinningJointCache->jointIndexOffsetCurr;
            skinningJointCache->jointIndexOffsetCurr = skinningJointCache->jointIndexOffsetCurr == 0 ? numJoints : 0;
        }
    } else {
        skinningJointCache->jointIndexOffsetPrev = 0;
        skinningJointCache->jointIndexOffsetCurr = 0;
    }

    simdProcessor->MultiplyJoints(skinningJointCache->skinningJoints + skinningJointCache->jointIndexOffsetCurr, jointMats, skeleton->GetInvBindPoseMats(), numJoints);

    if (renderGlobal.skinningMethod == VtfSkinning) {
        bufferCacheManager.AllocTexel(skinningJointCache->numJoints * sizeof(Mat3x4), skinningJointCache->skinningJoints, &skinningJointCache->bufferCache);
    }

    renderSystem.GetCurrentRenderContext()->renderCounter.numSkinningEntities++;
}

float Mesh::ComputeVolume() const {
    float   totalVolume = 0;

    for (int i = 0; i < NumSurfaces(); i++) {
        const MeshSurf *surf = GetSurface(i);
        const SubMesh *subMesh = surf->subMesh;

        // NOTE: should be a solid polytope to calculate volume exactly
        if (subMesh->IsClosed()) {
            totalVolume += subMesh->ComputeVolume();
        } else {
            // compute volume using AABB
            totalVolume = subMesh->GetAABB().Volume();
        }
    }

    return totalVolume;
}

const Vec3 Mesh::ComputeCentroid() const {
    float   totalVolume = 0;
    Vec3    totalVolumeCentroid(0.0f);
    float   volume;
    Vec3    centroid;

    for (int i = 0; i < NumSurfaces(); i++) {
        const MeshSurf *surf = GetSurface(i);
        const SubMesh *subMesh = surf->subMesh;

        // NOTE: should be a solid polytope to calculate volume & centroid exactly
        if (subMesh->IsClosed()) {
            volume = subMesh->ComputeVolume();
            centroid = subMesh->ComputeCentroid();
        } else {
            // compute volume & centroid using AABB
            volume = subMesh->GetAABB().Volume();
            centroid = subMesh->GetAABB().Center();
        }

        totalVolumeCentroid += centroid * volume;
        totalVolume += volume;
    }

    if (totalVolume > 0.0f) {
        return totalVolumeCentroid / totalVolume;
    }

    return Vec3::origin;
}

bool Mesh::Load(const char *filename) {
    Purge();

    Str bMeshFilename = filename;
    if (!Str::CheckExtension(filename, ".bmesh")) {
        bMeshFilename.SetFileExtension(".bmesh");
    }

    BE_LOG(L"Loading mesh '%hs'...\n", bMeshFilename.c_str());
    
    bool ret = LoadBinaryMesh(bMeshFilename);
    if (!ret) {
        return false;
    }

    return true;
}

void Mesh::Write(const char *filename) {
    WriteBinaryMesh(filename);
}

bool Mesh::Reload() {
    const Mesh *mesh = this;
    if (originalMesh) {
        mesh = originalMesh;
    }

    Str _hashName = hashName;
    bool ret = Load(_hashName);

    for (int i = 0; i < mesh->instantiatedMeshes.Count(); i++) {
        instantiatedMeshes[i]->Reinstantiate();
    }
    
    return ret;
}

BE_NAMESPACE_END
