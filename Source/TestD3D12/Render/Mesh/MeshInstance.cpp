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
#include "Render/Animation/Skeleton.h"
#include "Render/Animation/SkinningPose.h"

void MeshInstance::Reset() {
    type = Type::Static;
    sourceMesh.reset();
    sourceDataSerial = InvalidDataSerial;
    skeletonDataSerial = InvalidDataSerial;
    hasCompatibleSkinningData = false;
    skinningPose.reset();
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

bool MeshInstance::InitSkinned(SharedPtr<Mesh> newSourceMesh, SharedPtr<SkinningPose> newSkinningPose) {
    Reset();

    if (!newSourceMesh || !newSkinningPose || !newSkinningPose->GetSkeleton()) {
        return false;
    }

    type = Type::Skinned;
    sourceMesh = std::move(newSourceMesh);
    skinningPose = std::move(newSkinningPose);

    return IsSourceReady() && skinningPose->GetSkeleton()->IsReady() ? SyncWithSource() : true;
}

bool MeshInstance::NeedsRefreshFromSource() const {
    if (!IsSourceReady()) {
        return false;
    }

    if (sourceDataSerial != sourceMesh->GetDataSerial()) {
        return true;
    }

    const Skeleton *skeleton = skinningPose ? skinningPose->GetSkeleton() : nullptr;
    return skeleton && skeleton->IsReady() && skeletonDataSerial != skeleton->GetDataSerial();
}

bool MeshInstance::SyncWithSource() {
    // 원본 메시가 준비가 되었는지 확인
    if (!sourceMesh || sourceMesh->GetState() != Mesh::State::Ready) {
        hasCompatibleSkinningData = false;
        sourceDataSerial = InvalidDataSerial;
        return false;
    }

    // Static 메시 인스턴스는 원본 메시가 수정되었는지 여부와 상관없이 항상 최신 상태로 간주한다. (즉, Static 메시 인스턴스는 원본 메시가 수정되었더라도 갱신할 필요가 없다)
    if (IsStatic()) {
        return true;
    }

    if (!skinningPose || !skinningPose->SyncWithSkeleton()) {
        hasCompatibleSkinningData = false;
        skeletonDataSerial = InvalidDataSerial;
        return false;
    }

    // 이름/부모 관계의 전체 검사는 메시 또는 skeleton 데이터가 변경된 경우에만 수행한다.
    if (sourceDataSerial != sourceMesh->GetDataSerial() || skeletonDataSerial != skinningPose->GetSkeleton()->GetDataSerial()) {
        return RefreshFromSource();
    }

    return hasCompatibleSkinningData;
}

bool MeshInstance::RefreshFromSource() {
    // 호출자는 sourceMesh 가 Ready 상태임을 보장해야 한다.
    assert(sourceMesh);

    sourceDataSerial = sourceMesh->GetDataSerial();

    if (type == Type::Skinned) {
        const Skeleton *skeleton = skinningPose->GetSkeleton();
        skeletonDataSerial = skeleton->GetDataSerial();
        hasCompatibleSkinningData = HasSkinningData() && sourceMesh->IsCompatibleSkeleton(skeleton);
        return hasCompatibleSkinningData;
    }

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

