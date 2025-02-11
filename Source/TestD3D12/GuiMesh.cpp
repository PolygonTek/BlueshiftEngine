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
#include "GuiMesh.h"
#include "Texture.h"

void GuiMesh::Clear() {
    surfaces.SetCount(0, false);

    currentSurf = nullptr;
}

void GuiMesh::PrepareNewSurf() {
    Surface &newSurf = surfaces.Alloc();
    newSurf.texture = currentSurf ? currentSurf->texture : textureManager.defaultTexture;
    newSurf.numVerts = 0;
    newSurf.numIndexes = 0;
    newSurf.vertexBuffer = nullptr;
    newSurf.indexBuffer = nullptr;

    currentSurf = &newSurf;
}

void GuiMesh::DrawPic(RHI::FrameThreadData *frameThreadData, float x, float y, float w, float h, float s1, float t1, float s2, float t2, const Texture *texture, uint32_t color) {
    if (coordFrame == CoordFrame::CoordFrame2D && !clipRect.IsEmpty()) {
        const float cx = clipRect.x - x;
        if (cx > 0) {
            s1 += (s2 - s1) * cx / w;
            w -= cx;
            x = clipRect.x;
        }

        const float cy = clipRect.y - y;
        if (cy > 0) {
            t1 += (t2 - t1) * cy / h;
            h -= cy;
            y = clipRect.y;
        }

        const float cx2 = x + w - clipRect.X2();
        if (cx2 > 0) {
            s2 -= (s2 - s1) * cx2 / w;
            w -= cx2;
        }

        const float cy2 = y + h - clipRect.Y2();
        if (cy2 > 0) {
            t2 -= (t2 - t1) * cy2 / h;
            h -= cy2;
        }
    }

    if (w <= 0 || h <= 0) {
        // Completely clipped away.
        return;
    }

    ALIGN_AS16 BE1::VertexGeneric localVerts[4];

    if (coordFrame == CoordFrame::CoordFrame2D) {
        // 2D frame
        //  +-----> +X
        //  |
        //  |
        // +Y
        localVerts[0].xyz.Set(x, y, 0);
        localVerts[1].xyz.Set(x, y + h, 0);
        localVerts[2].xyz.Set(x + w, y + h, 0);
        localVerts[3].xyz.Set(x + w, y, 0);
    } else {
        // 3D frame
        // +Z
        //  |
        //  |
        //  +-----> +Y
        localVerts[0].xyz = BE1::Coords2D::To3D(x, -y);
        localVerts[1].xyz = BE1::Coords2D::To3D(x, -(y + h));
        localVerts[2].xyz = BE1::Coords2D::To3D(x + w, -(y + h));
        localVerts[3].xyz = BE1::Coords2D::To3D(x + w, -y);
    }

    // TODO: SIMD optimization
    const BE1::float16_t hs1 = BE1::F16Converter::FromF32(s1);
    const BE1::float16_t ht1 = BE1::F16Converter::FromF32(t1);
    const BE1::float16_t hs2 = BE1::F16Converter::FromF32(s2);
    const BE1::float16_t ht2 = BE1::F16Converter::FromF32(t2);

    localVerts[0].st[0] = hs1;
    localVerts[0].st[1] = ht1;
    *reinterpret_cast<uint32_t *>(localVerts[0].color) = color;

    localVerts[1].st[0] = hs1;
    localVerts[1].st[1] = ht2;
    *reinterpret_cast<uint32_t *>(localVerts[1].color) = color;

    localVerts[2].st[0] = hs2;
    localVerts[2].st[1] = ht2;
    *reinterpret_cast<uint32_t *>(localVerts[2].color) = color;

    localVerts[3].st[0] = hs2;
    localVerts[3].st[1] = ht1;
    *reinterpret_cast<uint32_t *>(localVerts[3].color) = color;

    DrawQuad(frameThreadData, localVerts, texture);
}

void GuiMesh::DrawQuad(RHI::FrameThreadData *frameThreadData, const BE1::VertexGeneric *verts, const Texture *texture) {
    if (!verts || !texture) {
        return;
    }

    // 다이나믹 버텍스 버퍼에 Quad 버텍스 데이터를 복사하기만 하고, 실제로 draw 하지는 않는다.
    // 다이나믹 인덱스 버퍼는 CacheIndexes() 에서 모든 Quad 에 대한 인덱스 데이터가 한꺼번에 만들어진다.
    if (!currentSurf || texture != currentSurf->texture) {
        PrepareNewSurf();
    } else {
        assert(currentSurf->vertexBuffer);
        if (frameThreadData->AppendVertex(currentSurf->vertexBuffer, sizeof(BE1::VertexGeneric), 4, verts)) {
            currentSurf->numVerts += 4;
            currentSurf->numIndexes += 6;
            return;
        }
        PrepareNewSurf();
    }

    currentSurf->texture = texture;
    currentSurf->vertexBuffer = frameThreadData->AllocVertex(sizeof(BE1::VertexGeneric), 4, verts);
    currentSurf->numVerts += 4;
    currentSurf->numIndexes += 6;
}

void GuiMesh::CacheIndexes(RHI::FrameThreadData *frameThreadData) {
    if (surfaces.Count() == 0) {
        return;
    }

    constexpr BE1::VertIndex quadTrisIndexes[6] = { 0, 1, 2, 0, 2, 3 };

    // 모든 surf 에 대해서 index buffer 를 채운다.
    for (int surfaceIndex = 0; surfaceIndex < surfaces.Count(); surfaceIndex++) {
        Surface *surf = &surfaces[surfaceIndex];
        assert(surf->numIndexes % 6 == 0);

        if (!surf->indexBuffer) {
            surf->indexBuffer = frameThreadData->AllocIndex(sizeof(quadTrisIndexes[0]), surf->numIndexes);
            BE1::VertIndex *indexPtr = reinterpret_cast<BE1::VertIndex *>(surf->indexBuffer->writePtr);

            // 현재 surf 의 첫번째 버텍스가 위치한 곳을 index 로 나타낸 값
            int baseVertexIndex = 0;//surf->vertexBuffer->GetOffset() / sizeof(BE1::VertexGeneric);

            for (int index = 0; index < surf->numIndexes; index += 6) {
                *indexPtr++ = baseVertexIndex + quadTrisIndexes[0];
                *indexPtr++ = baseVertexIndex + quadTrisIndexes[1];
                *indexPtr++ = baseVertexIndex + quadTrisIndexes[2];
                *indexPtr++ = baseVertexIndex + quadTrisIndexes[3];
                *indexPtr++ = baseVertexIndex + quadTrisIndexes[4];
                *indexPtr++ = baseVertexIndex + quadTrisIndexes[5];

                baseVertexIndex += 4;
            }
        }
    }
}
