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

/*
-------------------------------------------------------------------------------

    Mesh Instance

-------------------------------------------------------------------------------
*/

class MeshInstance {
public:
    using Type = Mesh::InstanceType;

    MeshInstance() = default;
    MeshInstance(const MeshInstance &rhs) = delete;
    MeshInstance &operator=(const MeshInstance &rhs) = delete;

    void                            Reset();

    bool                            InitStatic(const std::shared_ptr<Mesh> &sourceMesh);
    bool                            InitSkinned(const std::shared_ptr<Mesh> &sourceMesh);

    Type                            GetType() const { return type; }
    bool                            IsSkinned() const { return type == Type::Skinned; }
    bool                            IsStatic() const { return type == Type::Static; }

    const std::shared_ptr<Mesh> &   GetSourceMesh() const { return sourceMesh; }

    bool                            IsSourceReady() const;
    bool                            IsSourceDirty() const;
    bool                            EnsureCurrent();

    int                             NumSurfaces() const;
    const Mesh::Surface *           GetSurface(int surfaceIndex) const;
    SubMesh *                       GetSubMesh(int surfaceIndex) const;
    int32_t                         GetMaterialIndex(int surfaceIndex) const;

    const BE1::AABB &               GetAABB() const;

    int                             NumSkinningMatrices() const { return skinningMatrices.Count(); }
    const BE1::Mat3x4 *             GetSkinningMatrices() const { return skinningMatrices.Ptr(); }
    BE1::Mat3x4 *                   GetSkinningMatrices() { return skinningMatrices.Ptr(); }
    bool                            SetSkinningMatrices(const BE1::Mat3x4 *matrices, int numMatrices);

private:
    bool                            RefreshFromSource();
    bool                            HasSkinningData() const;
    void                            ResizeSkinningMatrices(int numMatrices);

    static constexpr uint32_t       InvalidDataSerial = UINT32_MAX;

    Type                            type = Type::Static;
    std::shared_ptr<Mesh>           sourceMesh;
    uint32_t                        sourceDataSerial = InvalidDataSerial;
    BE1::Array<BE1::Mat3x4>         skinningMatrices = BE1::Array<BE1::Mat3x4>(64);
};
