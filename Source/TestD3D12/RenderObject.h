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

#pragma once

struct DbvtProxy;
class Mesh;
class RenderWorld;

enum class MeshType : uint8_t {
    None,
    CubeMesh
};

class RenderObject {
    friend class RenderWorld;

public:
    struct Decl {
        ALIGN_AS32 BE1::Mat3x4  worldMatrix = BE1::Mat3x4::identity;
        ALIGN_AS32 BE1::AABB    aabb = BE1::AABB::empty; ///< non-scaled AABB in local space

        MeshType                meshType = MeshType::None;
        std::shared_ptr<Mesh>   mesh;
    };

    RenderObject() = default;
    ~RenderObject();

                                /// Returns object index in world.
    int                         GetIndex() const { return index; }

                                /// Returns object input definition.
    Decl &                      GetDecl() { return decl; }
    const Decl &                GetDecl() const { return decl; }

                                /// Returns AABB in world space.
    const BE1::AABB &           GetWorldAABB() const { return worldAABB; }

                                /// Returns local to world matrix.
    const BE1::Mat3x4 &         GetWorldMatrix() const { return decl.worldMatrix; }

private:
    void                        Update(const Decl &decl);

    Decl                        decl;

    BE1::AABB                   worldAABB = BE1::AABB::empty;
    int                         index = -1;         // RenderWorld::renderObjects 의 인덱스

#ifdef USE_DBVT
    DbvtProxy *                 proxy = nullptr;    // proxy for render object
#endif
};
