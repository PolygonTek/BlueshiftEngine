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
#include "SIMD/SIMD.h"
#include "Core/Heap.h"

BE_NAMESPACE_BEGIN

bool Mesh::IsDefaultMesh() const {
    return (this == MeshManager::defaultMesh ? true : false);
}

void Mesh::Purge() {
    for (int surfaceIndex = 0; surfaceIndex < surfaces.Count(); surfaceIndex++) {
        FreeSurface(surfaces[surfaceIndex]);
    }
    surfaces.Clear();

    if (IsInstantiatedMesh()) {
        SAFE_DELETE(skinningJointCache);

        if (originalMesh) {
            originalMesh->instantiatedMeshes.RemoveFast(this);
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

Mesh *Mesh::InstantiateMesh(Type::Enum meshType) {
    Mesh *mesh = meshManager.AllocInstantiatedMesh(this);

    instantiatedMeshes.Append(mesh);

    mesh->Instantiate(meshType);

    return mesh;
}

void Mesh::Instantiate(Type::Enum meshType) {
    if (meshType == Type::Skinned && numJoints > 0) {
        flags |= Flag::IsSkinnedMesh;
    } else if (meshType == Type::Dynamic) {
        flags |= Flag::IsDynamicMesh;
    } else {
        flags |= Flag::IsStaticMesh;
    }

    // Free previously allocated skinning joint cache
    SAFE_DELETE(skinningJointCache);

    if (flags & Flag::IsSkinnedMesh) {
        gpuSkinningEnabled = SkinningJointCache::CapableGPUJointSkinning((SkinningJointCache::SkinningMethod::Enum)renderGlobal.skinningMethod, numJoints);

        if (gpuSkinningEnabled) {
            skinningJointCache = new SkinningJointCache(numJoints);
        }
    }

    // Free previously allocated surfaces
    if (surfaces.Count() > 0) {
        for (int i = 0; i < surfaces.Count(); i++) {
            FreeSurface(surfaces[i]);
        }
        surfaces.Clear();
    }

    for (int surfaceIndex = 0; surfaceIndex < originalMesh->surfaces.Count(); surfaceIndex++) {
        MeshSurf *surf = AllocInstantiatedSurface(originalMesh->surfaces[surfaceIndex], meshType);
        surfaces.Append(surf);
    }
}

void Mesh::Reinstantiate() {
    Type::Enum meshType;
    if (flags & Flag::IsSkinnedMesh) {
        meshType = Type::Skinned;
    } else {
        meshType = Type::Static;
    }

    Instantiate(meshType);
}

void Mesh::FinishSurfaces(int flags) {
    if (flags & FinishFlag::SortAndMerge) {
        SortAndMerge();
        ComputeAABB();
    }

    if (flags & FinishFlag::ComputeAABB) {
        ComputeAABB();
    }

    //SplitMirroredVerts();

    if (flags & FinishFlag::OptimizeIndices) {
        OptimizeIndexedTriangles();
    }

    if ((flags & FinishFlag::ComputeNormals) && !(flags & FinishFlag::ComputeTangents)) {
        ComputeNormals();
    }

    /*for (int i = 0; i < surfaces.Count(); i++) {
        SubMesh *subMesh = surfaces[i]->subMesh;
        subMesh->FixMirroredVerts();
    }*/

    if (flags & FinishFlag::ComputeTangents) {
        ComputeTangents((flags & FinishFlag::ComputeNormals) ? true : false, (flags & FinishFlag::UseUnsmoothedTangents) ? true : false);
    }

    // TODO: consider to remove this
    if (flags & FinishFlag::ComputeEdges) {
        ComputeEdges();
    }
}

void Mesh::TransformVerts(const Mat3 &rotation, const Vec3 &scale, const Vec3 &translation) {
    for (int surfaceIndex = 0; surfaceIndex < surfaces.Count(); surfaceIndex++) {
        SubMesh *subMesh = surfaces[surfaceIndex]->subMesh;

        for (int vertexIndex = 0; vertexIndex < subMesh->numVerts; vertexIndex++) {
            subMesh->verts[vertexIndex].Transform(rotation, scale, translation);
        }
    }

    ComputeAABB();
}

void Mesh::GetJointAABBs(const Mat3x4 *invBindPoseMats, Array<AABB> &jointAabbs) const {
    for (int aabbIndex = 0; aabbIndex < jointAabbs.Count(); aabbIndex++) {
        jointAabbs[aabbIndex].Clear();
    }

    if (jointAabbs.Count() != numJoints) {
        BE_WARNLOG("Mesh::GetJointAABBs: different joint count\n");
        return;
    }

    for (int surfaceIndex = 0; surfaceIndex < surfaces.Count(); surfaceIndex++) {
        SubMesh *subMesh = surfaces[surfaceIndex]->subMesh;

        switch (subMesh->MaxVertexWeights()) {
        case 8: {
            const VertexWeight8 *vertWeights = (const VertexWeight8 *)subMesh->VertexWeights();

            for (int vertexIndex = 0; vertexIndex < subMesh->NumVerts(); vertexIndex++) {
                const VertexWeight8 *vw = &vertWeights[vertexIndex];
                const Vec3 &position = subMesh->Verts()[vertexIndex].xyz;

                for (int weightIndex = 0; weightIndex < 8; weightIndex++) {
                    int jointIndex = vw->jointIndexes[weightIndex];

                    jointAabbs[jointIndex].AddPoint(invBindPoseMats[jointIndex].TransformPos(position));
                }
            }
            break; }
        case 4: {
            const VertexWeight4 *vertWeights = (const VertexWeight4 *)subMesh->VertexWeights();

            for (int vertexIndex = 0; vertexIndex < subMesh->NumVerts(); vertexIndex++) {
                const VertexWeight4 *vw = &vertWeights[vertexIndex];
                const Vec3 &position = subMesh->Verts()[vertexIndex].xyz;

                for (int weightIndex = 0; weightIndex < 4; weightIndex++) {
                    int jointIndex = vw->jointIndexes[weightIndex];

                    jointAabbs[jointIndex].AddPoint(invBindPoseMats[jointIndex].TransformPos(position));
                }
            }
            break; }
        case 1: {
            const VertexWeight1 *vertWeights = (const VertexWeight1 *)subMesh->VertexWeights();

            for (int vertexIndex = 0; vertexIndex < subMesh->NumVerts(); vertexIndex++) {
                const VertexWeight1 *vw = &vertWeights[vertexIndex];
                const Vec3 &position = subMesh->Verts()[vertexIndex].xyz;

                int jointIndex = vw->jointIndex;

                jointAabbs[jointIndex].AddPoint(invBindPoseMats[jointIndex].TransformPos(position));
            }
            break; }
        }
    }
}

void Mesh::OptimizeIndexedTriangles() {
    for (int surfaceIndex = 0; surfaceIndex < surfaces.Count(); surfaceIndex++) {
        SubMesh *subMesh = surfaces[surfaceIndex]->subMesh;
        subMesh->OptimizeIndexedTriangles();
    }
}

void Mesh::Voxelize() {
}

bool Mesh::IsIntersectLine(const Vec3 &start, const Vec3 &end, bool backFaceCull) const {
    for (int surfaceIndex = 0; surfaceIndex < surfaces.Count(); surfaceIndex++) {
        MeshSurf *surf = surfaces[surfaceIndex];
        if (surf->subMesh->IsIntersectLine(start, end, backFaceCull)) {
            return true;
        }
    }

    return false;
}

bool Mesh::IntersectRay(const Ray &ray, bool ignoreBackFace, float *hitDist) const {
    float minDist = Math::Infinity;

    for (int surfaceIndex = 0; surfaceIndex < surfaces.Count(); surfaceIndex++) {
        MeshSurf *surf = surfaces[surfaceIndex];

        float dist;
        if (surf->subMesh->IntersectRay(ray, ignoreBackFace, &dist)) {
            if (!hitDist) {
                return true;
            }

            if (dist > 0.0f && dist < minDist) {
                minDist = dist;
            }
        }
    }

    if (minDist == Math::Infinity) {
        return false;
    }

    *hitDist = minDist;
    return true;
}

float Mesh::IntersectRay(const Ray &ray, bool ignoreBackFace) const {
    float hitDist;

    if (IntersectRay(ray, ignoreBackFace, &hitDist)) {
        return hitDist;
    }
    return FLT_MAX;
}

void Mesh::SplitMirroredVerts() {
    for (int surfaceIndex = 0; surfaceIndex < surfaces.Count(); surfaceIndex++) {
        surfaces[surfaceIndex]->subMesh->SplitMirroredVerts();
    }
}

void Mesh::ComputeAABB() {
    aabb.Clear();

    for (int surfaceIndex = 0; surfaceIndex < surfaces.Count(); surfaceIndex++) {
        SubMesh *subMesh = surfaces[surfaceIndex]->subMesh;
        subMesh->ComputeAABB();

        aabb.AddAABB(subMesh->aabb);
    }

    // add small epsilon
    aabb.ExpandSelf(MmToUnit(0.1f));
}

void Mesh::ComputeNormals() {
    for (int surfaceIndex = 0; surfaceIndex < surfaces.Count(); surfaceIndex++) {
        surfaces[surfaceIndex]->subMesh->ComputeNormals();
    }
}

void Mesh::ComputeTangents(bool includeNormals, bool useUnsmoothedTangents) {
    for (int surfaceIndex = 0; surfaceIndex < surfaces.Count(); surfaceIndex++) {
        MeshSurf *surf = surfaces[surfaceIndex];

        surf->subMesh->ComputeTangents(includeNormals, useUnsmoothedTangents);
    }
}

void Mesh::ComputeEdges() {
    for (int surfaceIndex = 0; surfaceIndex < surfaces.Count(); surfaceIndex++) {
        surfaces[surfaceIndex]->subMesh->ComputeEdges();
    }
}

bool Mesh::IsCompatibleSkeleton(const Skeleton *skeleton) const {
    if (numJoints != skeleton->NumJoints()) {
        return false;
    }
    
    const Joint *otherJoints = skeleton->GetJoints();

    for (int jointIndex = 0; jointIndex < numJoints; jointIndex++) {
        const Joint *joint = &joints[jointIndex];
        const Joint *otherJoint = &otherJoints[jointIndex];
        
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

void Mesh::UpdateSkinningJointCache(const Skeleton *skeleton, const Mat3x4 *jointMats) {
    if (!gpuSkinningEnabled || !skinningJointCache) {
        return;
    }

    skinningJointCache->Update(skeleton, jointMats);

    renderSystem.GetCurrentRenderContext()->GetRenderCounter().numSkinningEntities++;
}

float Mesh::ComputeVolume() const {
    float totalVolume = 0;

    for (int i = 0; i < NumSurfaces(); i++) {
        const MeshSurf *surf = GetSurface(i);
        const SubMesh *subMesh = surf->subMesh;

        if (subMesh->IsClosed()) {
            totalVolume += subMesh->ComputeVolume();
        } else {
            // Compute volume using AABB
            totalVolume = subMesh->GetAABB().Volume();
        }
    }

    return totalVolume;
}

const Vec3 Mesh::ComputeCentroid() const {
    Vec3 centroid;
    ComputeVolumeAndCentroid(centroid);
    return centroid;
}

float Mesh::ComputeVolumeAndCentroid(Vec3 &outCentroid) const {
    float   totalVolume = 0;
    Vec3    totalVolumeCentroid(0.0f);
    float   volume;
    Vec3    centroid;

    for (int i = 0; i < NumSurfaces(); i++) {
        const MeshSurf *surf = GetSurface(i);
        const SubMesh *subMesh = surf->subMesh;

        if (subMesh->IsClosed()) {
            volume = subMesh->ComputeVolume();
            centroid = subMesh->ComputeCentroid();
        } else {
            // Compute volume and centroid using AABB
            volume = subMesh->GetAABB().Volume();
            centroid = subMesh->GetAABB().Center();
        }

        totalVolumeCentroid += centroid * volume;
        totalVolume += volume;
    }

    if (totalVolume > 0.0f) {
        outCentroid = totalVolumeCentroid / totalVolume;
    } else {
        outCentroid = Vec3::origin;
    }
    return totalVolume;
}

void Mesh::RecomputeTangents() {
    for (int surfaceIndex = 0; surfaceIndex < surfaces.Count(); surfaceIndex++) {
        MeshSurf *surf = surfaces[surfaceIndex];

        surf->subMesh->tangentsCalculated = false;
        surf->subMesh->ComputeTangents(false, false);
    }
}

bool Mesh::Load(const char *filename) {
    Purge();

    Str bMeshFilename = filename;
    if (!Str::CheckExtension(filename, ".bmesh")) {
        bMeshFilename.SetFileExtension(".bmesh");
    }

    BE_LOG("Loading mesh '%s'...\n", bMeshFilename.c_str());
    
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
