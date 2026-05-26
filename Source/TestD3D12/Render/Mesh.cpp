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
#include "MeshInstance.h"
#include "LoadingSystem.h"

void Mesh::Purge() {
    internalData.Purge();
}

void Mesh::Reset() {
    BumpLoadSerial(); // 진행 중인 로딩 completion 무효화
    state = State::Empty;

    Purge();
}

Mesh::Surface *Mesh::AllocSurface(int numVerts, int numIndexes) {
    Surface *surf = new Surface;
    surf->materialIndex = 0;
    surf->subMesh = new SubMesh;
    surf->subMesh->Alloc(numVerts, numIndexes);
    return surf;
}

void Mesh::ApplyPrepared(InternalData &newInternalData) {
    assert(state == State::Empty || state == State::Loading || state == State::Ready);

    Purge();

    internalData.Swap(newInternalData);
    BumpDataSerial();
}

bool Mesh::Prepare(const char *filename, InternalData &outInternalData) {
    outInternalData.Purge();

    if (!outInternalData.LoadBinaryMesh(filename)) {
        return false;
    }
    outInternalData.FinishSurfaces(Mesh::FinishFlag::ComputeEdges);
    return true;
}

bool Mesh::Load(const char *filename) {
    InternalData newInternalData;
    if (!Mesh::Prepare(filename, newInternalData)) {
        return false;
    }

    BumpLoadSerial(); // 진행 중인 async completion 무효화
    state = State::Empty;

    ApplyPrepared(newInternalData);

    state = State::Ready;
    return true;
}

bool Mesh::LoadAsync(const char *filename) {
    state = State::Loading;
    uint32_t newSerial = BumpLoadSerial();

    if (!loadingSystem.EnqueueMeshLoad(shared_from_this(), filename, false, newSerial)) {
        state = State::Failed;
        return false;
    }
    return true;
}

bool Mesh::Reload() {
    if (state != State::Ready) {
        BE_WARNLOG("Mesh::Reload: mesh '%s' is not ready\n", GetHashName());
        return false;
    }
    return Load(hashName);
}

bool Mesh::ReloadAsync() {
    if (state != State::Ready) {
        BE_WARNLOG("Mesh::ReloadAsync: mesh '%s' is not ready\n", GetHashName());
        return false;
    }

    uint32_t newSerial = BumpLoadSerial();

    if (!loadingSystem.EnqueueMeshLoad(shared_from_this(), hashName, true, newSerial)) {
        state = State::Failed;
        Purge();
        return false;
    }
    return true;
}

void Mesh::Write(const char *filename) {
    assert(state == State::Ready);

    internalData.WriteBinaryMesh(filename);
}

std::shared_ptr<MeshInstance> Mesh::GetStaticInstance() {
    if (std::shared_ptr<MeshInstance> meshInstance = staticMeshInstance.lock()) {
        return meshInstance;
    }

    std::shared_ptr<MeshInstance> meshInstance = std::make_shared<MeshInstance>();
    if (!meshInstance->InitStatic(shared_from_this())) {
        return nullptr;
    }

    staticMeshInstance = meshInstance;
    return meshInstance;
}

std::shared_ptr<MeshInstance> Mesh::InstantiateSkinned() {
    std::shared_ptr<MeshInstance> meshInstance = std::make_shared<MeshInstance>();
    if (!meshInstance->InitSkinned(shared_from_this())) {
        return nullptr;
    }

    return meshInstance;
}

float Mesh::CalculateVolume() const {
    float totalVolume = 0;

    for (const Mesh::Surface *surface : internalData.surfaces) {
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

    for (Mesh::Surface *surface : internalData.surfaces) {
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
