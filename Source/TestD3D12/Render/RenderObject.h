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

struct VisObject;
class MeshInstance;
class Texture;
class Material;
class Font;
class RenderWorld;

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
            uint16_t                textDrawMode : 2;
            uint16_t                textAnchor : 4;
            uint16_t                textHorzAlignment : 2;
            uint16_t                textVertAlignment : 2;
            uint16_t                textHorzOverflow : 2;
            uint16_t                textVertOverflow : 2;
        } bits;
        uint16_t                    value = 0;
    };

    struct TextDesc {
        SharedPtr<Font>             font;
        BE1::Str                    text; ///< UTF8 encoded string
        BE1::RectF                  textRect = BE1::RectF::zero;
        BE1::Vec2                   textShadowOffset = BE1::Vec2(1, 1);
        uint32_t                    textShadowColor = BE1::Color4::black.ToUInt32();
        TextParams                  textParams;
        float                       textScale = 1.0f;
        float                       textLineSpacing = 1.0f;
    };

    struct Desc {
        enum class UpdateFlag : uint16_t {
            All                     = 0xFFFF,
            Transform               = BIT(0),
            Bounds                  = BIT(1),
            Mesh                    = BIT(2),
            Materials               = BIT(3),
            Font                    = BIT(4),
            Text                    = BIT(5),
            TextStyle               = BIT(6)
        };

        TextDesc &                  GetTextDesc();

        //
        // Transform info
        //
        BE1::Mat3x4                 worldMatrix = BE1::Mat3x4::identity;
        BE1::AABB                   aabb = BE1::AABB::empty; ///< non-scaled AABB in local space

        //
        // static/skinned mesh
        //
        SharedPtr<MeshInstance>     meshInstance;

        //
        // Raw indexed triangles rendering (typically for GUI rendering)
        //
        int                         numVerts = 0;
        BE1::VertexGeneric *        verts = nullptr;
        int                         numIndexes = 0;
        BE1::VertIndex *            indexes = nullptr;

        //
        // Text rendering
        //
        UniquePtr<TextDesc>         text;

        //
        // Materials
        //
        BE1::Array<SharedPtr<Material>> materials;
    };

    RenderObject() = default;
    ~RenderObject() = default;

                                    /// Returns object index in world.
    int                             GetIndex() const { return index; }

                                    /// Returns local to world matrix.
    const BE1::Mat3x4 &             GetWorldMatrix() const { return desc.worldMatrix; }

                                    /// Returns the last committed local to world matrix for previous world-matrix history.
    const BE1::Mat3x4 &             GetPrevWorldMatrix() const { return prevWorldMatrixValid ? prevWorldMatrix : desc.worldMatrix; }

                                    /// Returns AABB in world space.
    const BE1::AABB &               GetWorldAABB() const { return worldAABB; }

private:
                                    /// Update this render object.
    void                            Update(const Desc &desc, Desc::UpdateFlag updateFlags);

    Desc                            desc;
    BE1::AABB                       worldAABB = BE1::AABB::empty;

    BE1::Mat3x4                     prevWorldMatrix = BE1::Mat3x4::identity;
    bool                            prevWorldMatrixValid = false;
    uint64_t                        prevWorldMatrixFrameIndex = 0;
    uint64_t                        visibleFrameIndex = 0;

    int                             index = -1;                     // RenderWorld::renderObjects 의 인덱스
    uint64_t                        viewCount = uint64_t(-1);       // 마지막으로 VisObject 에 등록된 DrawCamera or DrawGUICamera 함수의 실행 카운터
};

BE_INLINE RenderObject::TextDesc &RenderObject::Desc::GetTextDesc() {
    if (!text) {
        text = MakeUnique<TextDesc>();
    }
    return *text;
}

template<>
struct enable_bitmask_operators<RenderObject::Desc::UpdateFlag> {
    static constexpr bool enable = true;
};
