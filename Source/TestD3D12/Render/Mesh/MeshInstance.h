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

#pragma once

#include "Mesh.h"

class SkinningPose;

/*
---------------------------------------------------------------------------------------------------

    Mesh Instance

    원본 Mesh 에 렌더링 인스턴스별 상태를 덧붙여 관리한다.
    Static MeshInstance 는 원본 Mesh 를 그대로 참조하여 사용한다.
    Skinned MeshInstance 는 공유 SkinningPose 와 원본 Mesh 의 호환성을 관리한다.
    SkinningPose 는 입력 joint matrix 배열과 현재/이전 pose 의 CPU skinning 캐시를 소유한다.

---------------------------------------------------------------------------------------------------
*/

class MeshInstance {
public:
    using Type = Mesh::InstanceType;

    MeshInstance() = default;
    MeshInstance(const MeshInstance &rhs) = delete;
    MeshInstance &operator=(const MeshInstance &rhs) = delete;

    uint64_t                        GetRetainedFrameSerial() const { return retainedFrameSerial; }
    void                            SetRetainedFrameSerial(uint64_t frameSerial) { retainedFrameSerial = frameSerial; }

    void                            Reset();

    bool                            InitStatic(SharedPtr<Mesh> sourceMesh);
    bool                            InitSkinned(SharedPtr<Mesh> sourceMesh, SharedPtr<SkinningPose> skinningPose);

    Type                            GetType() const { return type; }
    bool                            IsSkinned() const { return type == Type::Skinned; }
    bool                            IsStatic() const { return type == Type::Static; }

                                    // 원본 메시를 리턴한다.
    const SharedPtr<Mesh> &         GetSourceMesh() const { return sourceMesh; }

                                    // 원본 메시가 준비되었는지 여부를 리턴한다.
    bool                            IsSourceReady() const { return !sourceMesh || sourceMesh->GetState() != Mesh::State::Ready ? false : true; }

                                    // 원본 메시의 변경 사항을 인스턴스에 반영해야 하는지 여부를 리턴한다.
    bool                            NeedsRefreshFromSource() const;

                                    // 메시/skeleton 이 변경되었으면 호환성을 다시 검사한다.
    bool                            SyncWithSource();

    int                             NumSurfaces() const;
    const Mesh::Surface *           GetSurface(int surfaceIndex) const;
    SubMesh *                       GetSubMesh(int surfaceIndex) const;
    int32_t                         GetMaterialIndex(int surfaceIndex) const;

    const BE1::AABB &               GetAABB() const;

    SkinningPose *                  GetSkinningPose() { return skinningPose.get(); }
    const SkinningPose *            GetSkinningPose() const { return skinningPose.get(); }

private:
    bool                            RefreshFromSource();
    bool                            HasSkinningData() const;

    static constexpr uint32_t       InvalidDataSerial = UINT32_MAX;

    Type                            type = Type::Static;
    SharedPtr<Mesh>                 sourceMesh;
    uint32_t                        sourceDataSerial = InvalidDataSerial;
    uint32_t                        skeletonDataSerial = InvalidDataSerial;
    bool                            hasCompatibleSkinningData = false;
    uint64_t                        retainedFrameSerial = 0;
    SharedPtr<SkinningPose>         skinningPose;
};
