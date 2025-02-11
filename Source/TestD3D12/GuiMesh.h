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

class Texture;

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

    void                    DrawPic(RHI::FrameThreadData *frameThreadData, float x, float y, float w, float h, float s1, float t1, float s2, float t2, const Texture *texture, uint32_t color);

    void                    CacheIndexes(RHI::FrameThreadData *frameThreadData);

private:
    void                    PrepareNewSurf();
    void                    DrawQuad(RHI::FrameThreadData *frameThreadData, const BE1::VertexGeneric *verts, const Texture *texture);

    BE1::Array<Surface>     surfaces;
    Surface *               currentSurf = nullptr;

    CoordFrame              coordFrame = CoordFrame::CoordFrame2D;
    BE1::Rect               clipRect = BE1::Rect::zero;
};
