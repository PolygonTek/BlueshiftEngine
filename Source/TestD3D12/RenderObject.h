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

struct DbvtProxy;
class Mesh;
class Texture;
class RenderWorld;
class VisObject;

struct RenderObjectDesc {
    BE1::Mat3x4             worldMatrix = BE1::Mat3x4::identity;
    BE1::AABB               aabb = BE1::AABB::empty; ///< non-scaled AABB in local space

    Mesh *                  mesh = nullptr;
    BE1::Array<Texture *>   textures;
};

class RenderObject {
    friend class RenderWorld;

public:
    RenderObject() = default;
    ~RenderObject();

                            /// Returns object index in world.
    int                     GetIndex() const { return index; }

                            /// Returns local to world matrix.
    const BE1::Mat3x4 &     GetWorldMatrix() const { return desc.worldMatrix; }

                            /// Returns AABB in world space.
    const BE1::AABB &       GetWorldAABB() const { return worldAABB; }

                            /// Update this render object.
    void                    Update(const RenderObjectDesc &desc);

private:
    RenderObjectDesc        desc;
    BE1::AABB               worldAABB = BE1::AABB::empty;

    int                     index = -1;                 // RenderWorld::renderObjects 의 인덱스
#ifdef USE_DBVT
    DbvtProxy *             proxy = nullptr;            // proxy for render object

    int                     numMeshSurfProxies = 0;     // number of proxies for static sub mesh
    DbvtProxy *             meshSurfProxies = nullptr;  // proxies for static sub mesh
#endif

    mutable uint32_t        viewCount = 0;              // 현재 실행 중인 DrawCamera or DrawGUICamera 함수의 실행 카운터
    mutable VisObject *     visObject = nullptr;        // viewCount 가 현재 viewCount 와 같아야 valid 하다.
};
