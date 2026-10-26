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

    jointMats.Clear();
    jointMatsSerial = 0;
    skinningJointCache.reset();
}

bool MeshInstance::InitStatic(SharedPtr<Mesh> newSourceMesh) {
    Reset();

    if (!newSourceMesh) {
        return false;
    }

    type = Type::Static;
    sourceMesh = std::move(newSourceMesh);

    return IsSourceReady() ? RefreshFromSource() : true;
}

bool MeshInstance::InitSkinned(SharedPtr<Mesh> newSourceMesh) {
    Reset();

    if (!newSourceMesh) {
        return false;
    }

    type = Type::Skinned;
    sourceMesh = std::move(newSourceMesh);

    return IsSourceReady() ? RefreshFromSource() : true;
}

bool MeshInstance::IsSourceReady() const {
    if (!sourceMesh || sourceMesh->GetState() != Mesh::State::Ready) {
        return false;
    }
    return true;
}

bool MeshInstance::NeedsRefreshFromSource() const {
    if (sourceMesh && sourceMesh->GetState() == Mesh::State::Ready && sourceDataSerial != sourceMesh->GetDataSerial()) {
        return true;
    }
    return false;
}

bool MeshInstance::EnsureCurrent() {
    // 원본 메시가 준비가 되었는지 확인
    if (!sourceMesh || sourceMesh->GetState() != Mesh::State::Ready) {
        return false;
    }

    // Static 메시 인스턴스는 원본 메시가 수정되었는지 여부와 상관없이 항상 최신 상태로 간주한다. (즉, Static 메시 인스턴스는 원본 메시가 수정되었더라도 갱신할 필요가 없다)
    if (IsStatic()) {
        return true;
    }

    // 원본 메시가 수정되었는지 확인
    if (sourceDataSerial == sourceMesh->GetDataSerial()) {
        return true;
    }

    // 원본 메시가 수정되었으면, 인스턴스의 skinning matrix 배열을 갱신하고 포즈 이력을 초기화
    return RefreshFromSource();
}

bool MeshInstance::RefreshFromSource() {
    // 호출자는 sourceMesh 가 Ready 상태임을 보장해야 한다.
    assert(sourceMesh);

    // Skinned 메시 인스턴스인 경우에만 인스턴스 데이터들을 갱신한다.
    if (type == Type::Skinned) {
        if (!HasSkinningData()) {
            return false;
        }

        ResizeJointMats(sourceMesh->NumJoints());
        InvalidateSkinningJointCache();
    } else {
        jointMats.Clear();
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

void MeshInstance::ResizeJointMats(int numMatrices) {
    int oldCount = jointMats.Count();
    if (oldCount == numMatrices) {
        return;
    }

    jointMats.SetCount(numMatrices, false);

    // 추가된 jointMats 는 단위 행렬로 초기화한다.
    for (int i = oldCount; i < numMatrices; ++i) {
        jointMats[i].SetIdentity();
    }

    // SkinningJointCache 를 재생성한다.
    skinningJointCache = numMatrices > 0 ? MakeUnique<SkinningJointCache>(numMatrices) : nullptr;
}

bool MeshInstance::SetJointMats(const BE1::Mat3x4 *matrices, int numMatrices) {
    if (type != Type::Skinned || !matrices || numMatrices != jointMats.Count()) {
        return false;
    }

    BE1::simdProcessor->Memcpy(jointMats.Ptr(), matrices, sizeof(BE1::Mat3x4) * numMatrices);
    jointMatsSerial++;
    return true;
}

bool MeshInstance::UpdateSkinningJointCache(const Skeleton *skeleton) {
    if (type != Type::Skinned || !skinningJointCache) {
        return false;
    }

    return skinningJointCache->Update(skeleton, jointMats.Ptr());
}

void MeshInstance::InvalidateSkinningJointCache() {
    if (skinningJointCache) {
        skinningJointCache->Invalidate();
    }
}
