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
#include "MeshInstance.h"

void MeshInstance::Reset() {
    type = Type::Static;
    sourceMesh.reset();
    sourceDataSerial = InvalidDataSerial;
    skinningMatrices.Clear();
    skinningJointCache.reset();
}

bool MeshInstance::InitStatic(const std::shared_ptr<Mesh> &newSourceMesh) {
    Reset();

    if (!newSourceMesh) {
        return false;
    }

    type = Type::Static;
    sourceMesh = newSourceMesh;

    return IsSourceReady() ? RefreshFromSource() : true;
}

bool MeshInstance::InitSkinned(const std::shared_ptr<Mesh> &newSourceMesh) {
    Reset();

    if (!newSourceMesh) {
        return false;
    }

    type = Type::Skinned;
    sourceMesh = newSourceMesh;

    return IsSourceReady() ? RefreshFromSource() : true;
}

bool MeshInstance::IsSourceReady() const {
    if (!sourceMesh || sourceMesh->GetState() != Mesh::State::Ready) {
        return false;
    }

    return true;
}

bool MeshInstance::IsSourceDirty() const {
    return sourceMesh &&
        sourceMesh->GetState() == Mesh::State::Ready &&
        sourceDataSerial != sourceMesh->GetDataSerial();
}

bool MeshInstance::EnsureCurrent() {
    if (!IsSourceReady()) {
        return false;
    }

    if (!IsSourceDirty()) {
        return true;
    }

    return RefreshFromSource();
}

bool MeshInstance::RefreshFromSource() {
    if (!IsSourceReady()) {
        return false;
    }

    if (type == Type::Skinned) {
        if (!HasSkinningData()) {
            return false;
        }

        ResizeSkinningMatrices(sourceMesh->NumJoints());
    } else {
        skinningMatrices.Clear();
        skinningJointCache.reset();
    }

    sourceDataSerial = sourceMesh->GetDataSerial();
    return true;
}

int MeshInstance::NumSurfaces() const {
    if (!sourceMesh || sourceMesh->GetState() != Mesh::State::Ready) {
        return 0;
    }

    return sourceMesh->NumSurfaces();
}

const Mesh::Surface *MeshInstance::GetSurface(int surfaceIndex) const {
    if (!sourceMesh || sourceMesh->GetState() != Mesh::State::Ready) {
        return nullptr;
    }
    if (surfaceIndex < 0 || surfaceIndex >= sourceMesh->NumSurfaces()) {
        return nullptr;
    }

    return sourceMesh->GetSurface(surfaceIndex);
}

SubMesh *MeshInstance::GetSubMesh(int surfaceIndex) const {
    const Mesh::Surface *surface = GetSurface(surfaceIndex);
    return surface ? surface->subMesh : nullptr;
}

int32_t MeshInstance::GetMaterialIndex(int surfaceIndex) const {
    const Mesh::Surface *surface = GetSurface(surfaceIndex);
    return surface ? surface->materialIndex : -1;
}

const BE1::AABB &MeshInstance::GetAABB() const {
    if (!sourceMesh || sourceMesh->GetState() != Mesh::State::Ready) {
        return BE1::AABB::empty;
    }

    return sourceMesh->GetAABB();
}

bool MeshInstance::HasSkinningData() const {
    if (!sourceMesh || sourceMesh->NumJoints() <= 0) {
        return false;
    }

    const int numSurfaces = sourceMesh->NumSurfaces();
    for (int surfaceIndex = 0; surfaceIndex < numSurfaces; ++surfaceIndex) {
        const Mesh::Surface *surface = sourceMesh->GetSurface(surfaceIndex);
        if (surface && surface->subMesh && surface->subMesh->MaxVertexWeights() > 0) {
            return true;
        }
    }

    return false;
}

void MeshInstance::ResizeSkinningMatrices(int numMatrices) {
    int oldCount = skinningMatrices.Count();
    if (oldCount == numMatrices) {
        if (!skinningJointCache) {
            skinningJointCache = numMatrices > 0 ? std::make_unique<SkinningJointCache>(numMatrices) : nullptr;
        }
        return;
    }

    skinningMatrices.SetCount(numMatrices, false);

    for (int i = oldCount; i < numMatrices; ++i) {
        skinningMatrices[i].SetIdentity();
    }

    skinningJointCache = numMatrices > 0 ? std::make_unique<SkinningJointCache>(numMatrices) : nullptr;
}

bool MeshInstance::SetSkinningMatrices(const BE1::Mat3x4 *matrices, int numMatrices) {
    if (type != Type::Skinned || !matrices || numMatrices != skinningMatrices.Count()) {
        return false;
    }

    BE1::simdProcessor->Memcpy(skinningMatrices.Ptr(), matrices, sizeof(BE1::Mat3x4) * numMatrices);
    return true;
}

bool MeshInstance::UpdateSkinningJointCache(RHI::FrameThreadData *frameThreadData, const Skeleton *skeleton) {
    if (type != Type::Skinned || !skinningJointCache) {
        return false;
    }

    return skinningJointCache->Update(frameThreadData, skeleton, skinningMatrices.Ptr());
}
