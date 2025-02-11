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

Mesh::Surface *Mesh::AllocSurface(int numVerts, int numIndexes) {
    Surface *surf = new Surface;
    surf->materialIndex = 0;
    surf->subMesh = new SubMesh;
    surf->subMesh->Alloc(type, numVerts, numIndexes);
    return surf;
}

void Mesh::FinishSurfaces(FinishFlag finishFlags) {
    if (BE1::HasFlag(finishFlags, FinishFlag::SortAndMerge)) {
        SortAndMerge();
    }

    if (BE1::HasFlag(finishFlags, FinishFlag::SortAndMerge) || BE1::HasFlag(finishFlags, FinishFlag::ComputeAABB)) {
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

void Mesh::TransformVerts(const BE1::Mat3 &rotation, const BE1::Vec3 &scale, const BE1::Vec3 &translation, bool recomputeAABB) {
    for (Surface *surface : surfaces) {
        SubMesh *subMesh = surface->subMesh;

        for (int vertexIndex = 0; vertexIndex < subMesh->numVerts; vertexIndex++) {
            subMesh->verts[vertexIndex].Transform(rotation, scale, translation);
        }
    }

    if (recomputeAABB) {
        ComputeAABB();
    }
}

void Mesh::ComputeAABB() {
    aabb.Clear();

    for (Surface *surface : surfaces) {
        SubMesh *subMesh = surface->subMesh;
        subMesh->ComputeAABB();

        aabb.AddAABB(subMesh->aabb);
    }

    // add small epsilon
    aabb.ExpandSelf(BE1::MmToUnit(0.1f));
}

void Mesh::ComputeNormals() {
    for (Surface *surface : surfaces) {
        surface->subMesh->ComputeNormals();
    }
}

void Mesh::ComputeTangents(bool includeNormals, bool useUnsmoothedTangents) {
    for (Surface *surface : surfaces) {
        surface->subMesh->ComputeTangents(includeNormals, useUnsmoothedTangents);
    }
}

void Mesh::ComputeEdges() {
    for (Surface *surface : surfaces) {
        surface->subMesh->ComputeEdges();
    }
}

float Mesh::CalculateVolume() const {
    float totalVolume = 0;

    for (const Surface *surface : surfaces) {
        const SubMesh *subMesh = surface->subMesh;

        if (subMesh->IsClosed()) {
            totalVolume += subMesh->CalculateVolume();
        } else {
            // Compute the volume using AABB.
            totalVolume = subMesh->GetAABB().Volume();
        }
    }
    return totalVolume;
}

const BE1::Vec3 Mesh::CalculateCentroid() const {
    BE1::Vec3 centroid;
    CalculateVolumeAndCentroid(centroid);
    return centroid;
}

float Mesh::CalculateVolumeAndCentroid(BE1::Vec3 &outCentroid) const {
    BE1::Vec3 totalVolumeCentroid(0.0f);
    float totalVolume = 0;

    for (Surface *surface : surfaces) {
        const SubMesh *subMesh = surface->subMesh;
        float volume;
        BE1::Vec3 centroid;

        if (subMesh->IsClosed()) {
            volume = subMesh->CalculateVolume();
            centroid = subMesh->CalculateCentroid();
        } else {
            // Computes the volume and centroid using AABB.
            volume = subMesh->GetAABB().Volume();
            centroid = subMesh->GetAABB().Center();
        }

        totalVolumeCentroid += centroid * volume;
        totalVolume += volume;
    }

    if (totalVolume > 0.0f) {
        outCentroid = totalVolumeCentroid / totalVolume;
    } else {
        outCentroid = BE1::Vec3::origin;
    }
    return totalVolume;
}

bool Mesh::Load(const char *filename) {
    Purge();

    BE1::Str binaryMeshFilename = filename;
    if (!BE1::Str::CheckExtension(filename, ".bmesh")) {
        binaryMeshFilename.SetFileExtension(".bmesh");
    }

    BE_LOG("Loading mesh '%s'...\n", binaryMeshFilename.c_str());
    return LoadBinaryMesh(binaryMeshFilename);
}

bool Mesh::Reload() {
    BE1::Str _hashName = hashName;
    return Load(_hashName);
}

void Mesh::Write(const char *filename) {
    WriteBinaryMesh(filename);
}

bool Mesh::LoadBinaryMesh(const char *filename) {
    return false;
}

void Mesh::WriteBinaryMesh(const char *filename) {
}
