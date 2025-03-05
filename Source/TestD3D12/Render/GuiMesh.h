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

#include "RHI.h"
#include "RenderObject.h"

class Texture;
class Font;
enum class DrawTextFlag : uint16_t;

class GuiMesh {
public:
    enum class CoordFrame : uint8_t {
        CoordFrame2D,
        CoordFrame3D
    };

    struct Surface {
        const Texture *     texture;
        int                 numVerts;
        int                 numIndexes;
        RHI::VertexBuffer * vertexBuffer;
        RHI::IndexBuffer *  indexBuffer;
    };

    GuiMesh() = default;

    CoordFrame              GetCoordFrame() const { return coordFrame; }
    void                    SetCoordFrame(CoordFrame frame) { coordFrame = frame; }

    int                     NumSurfaces() const { return surfaces.Count(); }
    const Surface *         GetSurface(int surfaceIndex) const { return &surfaces[surfaceIndex]; }

    void                    Clear();

    void                    SetClipRect(const BE1::Rect &clipRect) { this->clipRect = clipRect; }

    void                    DrawPic(RHI::FrameThreadData *frameThreadData, float x, float y, float w, float h, float s1, float t1, float s2, float t2, uint32_t color, const Texture *texture);
    float                   DrawChar(RHI::FrameThreadData *frameThreadData, float x, float y, char32_t unicodeChar);

    void                    DrawText2D(RHI::FrameThreadData *frameThreadData, const BE1::Rect &rect, int x, int y, int lineSpacing, float textScale,
                                uint32_t color, uint32_t shadowColor, const BE1::Vec2 &shadowOffset, const Font *font, const BE1::Str &text, DrawTextFlag flags);

    void                    DrawText3D(RHI::FrameThreadData *frameThreadData, RenderObject::TextDrawMode drawMode, RenderObject::TextAnchor anchor, RenderObject::TextHorzAlignment horzAlignment, float lineSpacing, float textScale,
                                uint32_t color, uint32_t shadowColor, const BE1::Vec2 &shadowOffset, const Font *font, const BE1::Str &text);

    void                    DrawText3D(RHI::FrameThreadData *frameThreadData, RenderObject::TextDrawMode drawMode, const BE1::RectF &rect, RenderObject::TextHorzAlignment horzAlignment, RenderObject::TextVertAlignment vertAlignment,
                                RenderObject::TextHorzOverflow horzOverflow, RenderObject::TextVertOverflow vertOverflow, float lineSpacing, float textScale,
                                uint32_t color, uint32_t shadowColor, const BE1::Vec2 &shadowOffset, const Font *font, const BE1::Str &text);

    void                    CacheIndexes(RHI::FrameThreadData *frameThreadData);

private:
    void                    PrepareNewSurf();
    void                    DrawQuad(RHI::FrameThreadData *frameThreadData, const BE1::VertexGeneric *verts, const Texture *texture);

    BE1::Array<Surface>     surfaces;
    Surface *               currentSurf = nullptr;

    CoordFrame              coordFrame = CoordFrame::CoordFrame2D;
    BE1::Rect               clipRect = BE1::Rect::zero;

    const Font *            currentFont = nullptr;
    float                   currentTextScale = 1.0f;
    uint32_t                currentTextColor = 0xFFFFFFFF;
    bool                    currentTextDropShadows = false;
    bool                    currentTextAddOutlines = false;
    uint32_t                currentTextShadowColor = 0;
    float                   currentTextShadowOffsetX = 0;
    float                   currentTextShadowOffsetY = 0;
};
