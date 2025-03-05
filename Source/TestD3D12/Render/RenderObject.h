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
class Font;
class RenderWorld;
class VisObject;

class RenderObject {
    friend class RenderWorld;

public:
    enum class TextDrawMode : uint8_t {
        Normal,
        DropShadows,
        AddOutlines
    };

    enum class TextAnchor : uint8_t {
        UpperLeft,
        UpperCenter,
        UpperRight,
        MiddleLeft,
        MiddleCenter,
        MiddleRight,
        LowerLeft,
        LowerCenter,
        LowerRight
    };

    enum class TextHorzAlignment : uint8_t {
        Left,
        Center,
        Right
    };

    enum class TextVertAlignment : uint8_t {
        Top,
        Middle,
        Bottom
    };

    enum class TextHorzOverflow : uint8_t {
        Wrap,
        Overflow
    };

    enum class TextVertOverflow : uint8_t {
        Truncate,
        Overflow
    };

    union TextParams {
        struct {
            uint16_t            textDrawMode : 2;
            uint16_t            textAnchor : 4;
            uint16_t            textHorzAlignment : 2;
            uint16_t            textVertAlignment : 2;
            uint16_t            textHorzOverflow : 2;
            uint16_t            textVertOverflow : 2;
        } bits;
        uint16_t                value = 0;
    };

    struct Desc {
        BE1::Mat3x4             worldMatrix = BE1::Mat3x4::identity;
        BE1::AABB               aabb = BE1::AABB::empty; ///< non-scaled AABB in local space

        Mesh *                  mesh = nullptr;
        BE1::Array<Texture *>   textures;

        Font *                  font = nullptr;
        BE1::Str                text; ///< UTF8 encoded string
        BE1::RectF              textRect = BE1::RectF::zero;
        BE1::Vec2               textShadowOffset = BE1::Vec2(1, 1);
        BE1::Color4             textShadowColor = BE1::Color4::black;
        TextParams              textParams;
        float                   textScale = 1.0f;
        float                   textLineSpacing = 1.0f;
    };

    RenderObject() = default;
    ~RenderObject();

                                /// Returns object index in world.
    int                         GetIndex() const { return index; }

                                /// Returns local to world matrix.
    const BE1::Mat3x4 &         GetWorldMatrix() const { return desc.worldMatrix; }

                                /// Returns AABB in world space.
    const BE1::AABB &           GetWorldAABB() const { return worldAABB; }

                                /// Update this render object.
    void                        Update(const Desc &desc);

private:
    Desc                        desc;
    BE1::AABB                   worldAABB = BE1::AABB::empty;

    int                         index = -1;                 // RenderWorld::renderObjects 의 인덱스
#ifdef USE_DBVT
    DbvtProxy *                 proxy = nullptr;            // proxy for render object

    int                         numMeshSurfProxies = 0;     // number of proxies for static sub mesh
    DbvtProxy *                 meshSurfProxies = nullptr;  // proxies for static sub mesh
#endif

    mutable uint32_t            viewCount = 0;              // 현재 실행 중인 DrawCamera or DrawGUICamera 함수의 실행 카운터
    mutable VisObject *         visObject = nullptr;        // viewCount 가 현재 RenderWorld::viewCount 와 같아야 valid 하다.
};
