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
#include "Render/Animation/SkinningJointCache.h"

/*
---------------------------------------------------------------------------------------------------

    Mesh Instance

    원본 Mesh 에 렌더링 인스턴스별 상태를 덧붙여 관리한다.
    Static MeshInstance 는 원본 Mesh 를 그대로 참조하여 사용한다.
    Skinned MeshInstance 는 원본 Mesh 에 맞춰 joint matrix 배열과 캐시를 갱신하여 사용한다.
    SkinningJointCache 는 현재/이전 joint pose 의 CPU skinning 결과를 소유한다.

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

    bool                            InitStatic(const SharedPtr<Mesh> &sourceMesh);
    bool                            InitSkinned(const SharedPtr<Mesh> &sourceMesh);

    Type                            GetType() const { return type; }
    bool                            IsSkinned() const { return type == Type::Skinned; }
    bool                            IsStatic() const { return type == Type::Static; }

                                    // 원본 메시를 리턴한다.
    const SharedPtr<Mesh> &         GetSourceMesh() const { return sourceMesh; }

                                    // 원본 메시가 준비되었는지 여부를 리턴한다.
    bool                            IsSourceReady() const;

                                    // 원본 메시의 변경 사항을 인스턴스에 반영해야 하는지 여부를 리턴한다.
    bool                            NeedsRefreshFromSource() const;

                                    // 원본 메시가 수정되었으면, 인스턴스의 skinning matrix 배열을 갱신하고 포즈 이력을 초기화한다.
    bool                            EnsureCurrent();

    int                             NumSurfaces() const;
    const Mesh::Surface *           GetSurface(int surfaceIndex) const;
    SubMesh *                       GetSubMesh(int surfaceIndex) const;
    int32_t                         GetMaterialIndex(int surfaceIndex) const;

    const BE1::AABB &               GetAABB() const;

    int                             NumJointMats() const { return jointMats.Count(); }
    const BE1::Mat3x4 *             GetJointMats() const { return jointMats.Ptr(); }
    uint64_t                        GetJointMatsSerial() const { return jointMatsSerial; }
    bool                            SetJointMats(const BE1::Mat3x4 *matrices, int numMatrices);

    const SkinningJointCache *      GetSkinningJointCache() const { return skinningJointCache.get(); }
    SkinningJointCache *            GetSkinningJointCache() { return skinningJointCache.get(); }
    bool                            UpdateSkinningJointCache(const Skeleton *skeleton);
    void                            InvalidateSkinningJointCache();

private:
    bool                            RefreshFromSource();
    bool                            HasSkinningData() const;
    void                            ResizeJointMats(int numMatrices);

    static constexpr uint32_t       InvalidDataSerial = UINT32_MAX;

    Type                            type = Type::Static;
    SharedPtr<Mesh>                 sourceMesh;
    uint32_t                        sourceDataSerial = InvalidDataSerial;
    uint64_t                        retainedFrameSerial = 0;
    BE1::Array<BE1::Mat3x4>         jointMats = BE1::Array<BE1::Mat3x4>(64);
    uint64_t                        jointMatsSerial = 0;
    UniquePtr<SkinningJointCache>   skinningJointCache;
};
