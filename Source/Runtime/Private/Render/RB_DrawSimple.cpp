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

#include "Precompiled.h"
#include "Render/Render.h"
#include "RenderInternal.h"
#include "RBackEnd.h"

BE_NAMESPACE_BEGIN

void RB_DrawRect(float x, float y, float x2, float y2, float s, float t, float s2, float t2) {
    const struct {
        Vec2 position;
        Vec2 texcoord;
    } verts[] = { 
        { Vec2(x, y), Vec2(s, t) }, 
        { Vec2(x2, y), Vec2(s2, t) }, 
        { Vec2(x2, y2), Vec2(s2, t2) }, 
        { Vec2(x, y2), Vec2(s, t2) }
    };

    rhi.BindBuffer(RHI::VertexBuffer, bufferCacheManager.streamVertexBuffer);
    rhi.BufferDiscardWrite(bufferCacheManager.streamVertexBuffer, 4 * sizeof(verts[0]), verts);

    rhi.SetVertexFormat(vertexFormats[VertexFormat::XySt].vertexFormatHandle);
    rhi.SetStreamSource(0, bufferCacheManager.streamVertexBuffer, 0, sizeof(verts[0]));
    rhi.DrawArrays(RHI::TriangleFanPrim, 0, 4);
}

void RB_DrawClipRect(float s, float t, float s2, float t2) {
    RB_DrawRect(-1.0f, -1.0f, 1.0f, 1.0f, s, t, s2, t2);
}

void RB_DrawRectSlice(float x, float y, float x2, float y2, float s, float t, float s2, float t2, float slice) {
    const struct {
        Vec2 position;
        Vec3 texcoord;
    } verts[] = { 
        { Vec2(x, y), Vec3(s, t, slice) }, 
        { Vec2(x2, y), Vec3(s2, t, slice) }, 
        { Vec2(x2, y2), Vec3(s2, t2, slice) }, 
        { Vec2(x, y2), Vec3(s, t2, slice) }
    };
        
    rhi.BindBuffer(RHI::VertexBuffer, bufferCacheManager.streamVertexBuffer);
    rhi.BufferDiscardWrite(bufferCacheManager.streamVertexBuffer, 4 * sizeof(verts[0]), verts);

    rhi.SetVertexFormat(vertexFormats[VertexFormat::XyStr].vertexFormatHandle);
    rhi.SetStreamSource(0, bufferCacheManager.streamVertexBuffer, 0, sizeof(verts[0]));
    rhi.DrawArrays(RHI::TriangleFanPrim, 0, 4);
}

static void ScreenToClipCoord(float x, float y, float *clip_x, float *clip_y) {
    *clip_x = ((float)x / backEnd.ctx->GetDeviceWidth()) * 2.0f - 1.0f;
    *clip_y = ((float)(backEnd.ctx->GetDeviceHeight() - y) / backEnd.ctx->GetDeviceHeight()) * 2.0f - 1.0f;
}

void RB_DrawScreenRect(float x, float y, float w, float h, float s, float t, float s2, float t2) {
    float clip_x, clip_y;
    float clip_x2, clip_y2;

    ScreenToClipCoord(x, y, &clip_x, &clip_y);
    ScreenToClipCoord(x + w, y + h, &clip_x2, &clip_y2);

    RB_DrawRect(clip_x, clip_y, clip_x2, clip_y2, s, t, s2, t2);
}

void RB_DrawScreenRectSlice(float x, float y, float w, float h, float s, float t, float s2, float t2, float slice) {
    float clip_x, clip_y;
    float clip_x2, clip_y2;

    ScreenToClipCoord(x, y, &clip_x, &clip_y);
    ScreenToClipCoord(x + w, y + h, &clip_x2, &clip_y2);

    RB_DrawRectSlice(clip_x, clip_y, clip_x2, clip_y2, s, t, s2, t2, slice);
}

void RB_DrawCircle(const Vec3 &origin, const Vec3 &left, const Vec3 &up, const float radius) {
    float s, c;
    int segments = (int)(Math::TwoPi * radius) / 10;
    segments = (segments + 3) & ~3;

    if (segments < 8) {
        segments = 8;
    }

    int size = (segments + 1) * sizeof(Vec3);
    Vec3 *verts = (Vec3 *)_alloca16(size);
    for (int i = 0; i < segments + 1; i++) {
        Math::SinCos(Math::TwoPi * i / segments, s, c);
        verts[i] = origin + radius * (left * c + up * s);
    }

    rhi.BindBuffer(RHI::VertexBuffer, bufferCacheManager.streamVertexBuffer);
    rhi.BufferDiscardWrite(bufferCacheManager.streamVertexBuffer, size, verts);

    rhi.SetVertexFormat(vertexFormats[VertexFormat::Xyz].vertexFormatHandle);
    rhi.SetStreamSource(0, bufferCacheManager.streamVertexBuffer, 0, sizeof(Vec3));
    rhi.DrawArrays(RHI::LineStripPrim, 0, segments);
}

void RB_DrawAABB(const AABB &aabb) {
    //static const uint16_t indices[24] = { 3, 2, 1, 0, 5, 4, 0, 1, 6, 5, 1, 2, 7, 6, 2, 3, 4, 7, 3, 0, 6, 7, 4, 5 };
    static const uint16_t indices[14] = { 7, 4, 6, 5, 1, 4, 0, 7, 3, 6, 2, 1, 3, 0 };

    Vec3 verts[8];
    aabb.ToPoints(verts);

    rhi.BindBuffer(RHI::VertexBuffer, bufferCacheManager.streamVertexBuffer);
    rhi.BufferDiscardWrite(bufferCacheManager.streamVertexBuffer, sizeof(verts), verts);

    rhi.BindBuffer(RHI::IndexBuffer, bufferCacheManager.streamIndexBuffer);
    rhi.BufferDiscardWrite(bufferCacheManager.streamIndexBuffer, sizeof(indices), indices);

    rhi.SetVertexFormat(vertexFormats[VertexFormat::Xyz].vertexFormatHandle);
    rhi.SetStreamSource(0, bufferCacheManager.streamVertexBuffer, 0, sizeof(Vec3));
    rhi.DrawElements(RHI::TriangleStripPrim, 0, COUNT_OF(indices), sizeof(indices[0]), 0);
}

void RB_DrawOBB(const OBB &obb) {	
    //static const uint16_t indices[24] = { 3, 2, 1, 0, 5, 4, 0, 1, 6, 5, 1, 2, 7, 6, 2, 3, 4, 7, 3, 0, 6, 7, 4, 5 };
    static const uint16_t indices[14] = { 7, 4, 6, 5, 1, 4, 0, 7, 3, 6, 2, 1, 3, 0 };

    Vec3 verts[8];
    obb.ToPoints(verts);

    rhi.BindBuffer(RHI::VertexBuffer, bufferCacheManager.streamVertexBuffer);
    rhi.BufferDiscardWrite(bufferCacheManager.streamVertexBuffer, sizeof(verts), verts);

    rhi.BindBuffer(RHI::IndexBuffer, bufferCacheManager.streamIndexBuffer);
    rhi.BufferDiscardWrite(bufferCacheManager.streamIndexBuffer, sizeof(indices), indices);
    
    rhi.SetVertexFormat(vertexFormats[VertexFormat::Xyz].vertexFormatHandle);
    rhi.SetStreamSource(0, bufferCacheManager.streamVertexBuffer, 0, sizeof(Vec3));
    rhi.DrawElements(RHI::TriangleStripPrim, 0, COUNT_OF(indices), sizeof(indices[0]), 0);
}

void RB_DrawFrustum(const Frustum &frustum) {
    //static const uint16_t indices[24] = { 3, 2, 1, 0, 5, 4, 0, 1, 6, 5, 1, 2, 7, 6, 2, 3, 4, 7, 3, 0, 6, 7, 4, 5 };
    static const uint16_t indices[14] = { 7, 4, 6, 5, 1, 4, 0, 7, 3, 6, 2, 1, 3, 0 };

    Vec3 verts[8];
    frustum.ToPoints(verts);

    rhi.BindBuffer(RHI::VertexBuffer, bufferCacheManager.streamVertexBuffer);
    rhi.BufferDiscardWrite(bufferCacheManager.streamVertexBuffer, sizeof(verts), verts);

    rhi.BindBuffer(RHI::IndexBuffer, bufferCacheManager.streamIndexBuffer);
    rhi.BufferDiscardWrite(bufferCacheManager.streamIndexBuffer, sizeof(indices), indices);

    rhi.SetVertexFormat(vertexFormats[VertexFormat::Xyz].vertexFormatHandle);
    rhi.SetStreamSource(0, bufferCacheManager.streamVertexBuffer, 0, sizeof(Vec3));
    rhi.DrawElements(RHI::TriangleStripPrim, 0, COUNT_OF(indices), sizeof(indices[0]), 0);
}

void RB_DrawSphere(const Sphere &sphere, int lats, int longs) {
    int size = lats * (longs + 1) * 2 * sizeof(Vec3);
    Vec3 *verts = (Vec3 *)_alloca16(size);

    R_GenerateSphereTriangleStripVerts(sphere, lats, longs, verts);
        
    rhi.BindBuffer(RHI::VertexBuffer, bufferCacheManager.streamVertexBuffer);
    rhi.BufferDiscardWrite(bufferCacheManager.streamVertexBuffer, size, verts);

    rhi.SetVertexFormat(vertexFormats[VertexFormat::Xyz].vertexFormatHandle);
    rhi.SetStreamSource(0, bufferCacheManager.streamVertexBuffer, 0, sizeof(Vec3));
    rhi.DrawArrays(RHI::TriangleStripPrim, 0, 2 * lats * longs);
}

BE_NAMESPACE_END
