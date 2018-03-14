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
#include "Core/Heap.h"

BE_NAMESPACE_BEGIN

static LinearKernel<5>  gaussKernel5x;
static LinearKernel<7>  gaussKernel7x;
static LinearKernel<15> gaussKernel15x;

static Vec3             ssaoRandomKernel[8];
                        
// for camera motion blur
static const int        sphereLats = 32;
static const int        sphereLongs = 32;
static RHI::Handle      sphereVB;

void PP_Init() {
    R_ComputeGaussianWeights(5, gaussKernel5x.discreteWeights);
    gaussKernel5x.Compute();
    
    R_ComputeGaussianWeights(7, gaussKernel7x.discreteWeights);
    gaussKernel7x.Compute();
    
    R_ComputeGaussianWeights(15, gaussKernel15x.discreteWeights);
    gaussKernel15x.Compute();

    ssaoRandomKernel[0] = Vec3(+1.0f, +1.0f, +1.0f);
    ssaoRandomKernel[1] = Vec3(-1.0f, -1.0f, -1.0f);
    ssaoRandomKernel[2] = Vec3(-1.0f, -1.0f, +1.0f);
    ssaoRandomKernel[3] = Vec3(-1.0f, +1.0f, -1.0f);
    ssaoRandomKernel[4] = Vec3(-1.0f, +1.0f, +1.0f);
    ssaoRandomKernel[5] = Vec3(+1.0f, -1.0f, -1.0f);
    ssaoRandomKernel[6] = Vec3(+1.0f, -1.0f, +1.0f);
    ssaoRandomKernel[7] = Vec3(+1.0f, +1.0f, -1.0f);

    for (int i = 0; i < COUNT_OF(ssaoRandomKernel); i++) {
        float scale = (float)i / COUNT_OF(ssaoRandomKernel);
        scale = Lerp(0.1f, 1.0f, scale * scale);

        ssaoRandomKernel[i].Normalize();
        ssaoRandomKernel[i] *= scale;
    }

    const int size = sphereLats * (sphereLongs + 1) * 2 * sizeof(Vec3);
    Vec3 *verts = (Vec3 *)Mem_Alloc16(size);
    R_GenerateSphereTriangleStripVerts(Sphere(Vec3::origin, 1.0f), sphereLats, sphereLongs, verts);

    sphereVB = rhi.CreateBuffer(RHI::VertexBuffer, RHI::Static, size, 0, verts);

    Mem_AlignedFree(verts);
}

void PP_Free() {
    rhi.DeleteBuffer(sphereVB);
}

void PP_PassThruPass(const Texture *srcTexture, float s, float t, float s2, float t2, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::postPassThruShader;

    shader->Bind();
    shader->SetTexture("tex0", srcTexture);

    RB_DrawClipRect(s, t, s2, t2);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_Downscale2x2(const Texture *srcTexture, RenderTarget *dstRT) {
    float s, t, s2, t2;
    Vec2 sampleOffsets[4];
    Rect prevViewportRect = rhi.GetViewport();

    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::downscale2x2Shader;

    if (!(srcTexture->GetFlags() & Texture::Nearest)) {
        sampleOffsets[0].x = 0.0f;
        sampleOffsets[0].y = 0.0f;

        shader->Bind();
        shader->SetTexture("tex0", srcTexture);
        shader->SetConstantArray2f("sampleOffsets", 1, sampleOffsets);

        s = 0;
        t = 0;
        s2 = 1.0f;
        t2 = 1.0f;
    } else {
        float texelOffsetX = 1.0f / srcTexture->GetWidth();
        float texelOffsetY = 1.0f / srcTexture->GetHeight();

        Vec2 *ptr = sampleOffsets;

        for (int y = -1; y < 1; y++) {
            for (int x = -1; x < 1; x++, ptr++) {
                ptr->x = x * texelOffsetX;
                ptr->y = y * texelOffsetY;
            }
        }

        shader->Bind();
        shader->SetTexture("tex0", srcTexture);
        shader->SetConstantArray2f("sampleOffsets", 4, sampleOffsets);

        s = 0;
        t = 0;
        s2 = 1.0f;
        t2 = 1.0f;
    }

    RB_DrawClipRect(s, t, s2, t2);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_Downscale4x4(const Texture *srcTexture, RenderTarget *dstRT) {
    float s, t, s2, t2;
    Vec2 sampleOffsets[16];
    Rect prevViewportRect = rhi.GetViewport();

    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::downscale4x4Shader;

    if (!(srcTexture->GetFlags() & Texture::Nearest)) {
        float texelOffsetX = 2.0f / srcTexture->GetWidth();
        float texelOffsetY = 2.0f / srcTexture->GetHeight();

        Vec2 *ptr = sampleOffsets;

        for (int y = 0; y < 2; y++) {
            for (int x = 0; x < 2; x++, ptr++) {
                ptr->x = x * texelOffsetX;
                ptr->y = y * texelOffsetY;
            }
        }

        shader->Bind();
        shader->SetTexture("tex0", srcTexture);
        shader->SetConstantArray2f("sampleOffsets", 4, sampleOffsets);

        s = -1.0f / srcTexture->GetWidth();
        t = -1.0f / srcTexture->GetHeight();
        s2 = 1.0f - 1.0f / srcTexture->GetWidth();
        t2 = 1.0f - 1.0f / srcTexture->GetHeight();
    } else {
        float texelOffsetX = 1.0f / srcTexture->GetWidth();
        float texelOffsetY = 1.0f / srcTexture->GetHeight();

        Vec2 *ptr = sampleOffsets;

        for (int y = -2; y < 2; y++) {
            for (int x = -2; x < 2; x++, ptr++) {
                ptr->x = x * texelOffsetX;
                ptr->y = y * texelOffsetY;
            }
        }

        shader->Bind();
        shader->SetTexture("tex0", srcTexture);
        shader->SetConstantArray2f("sampleOffsets", 16, sampleOffsets);

        s = 0.0f;
        t = 0.0f;
        s2 = 1.0f;
        t2 = 1.0f;
    }

    RB_DrawClipRect(s, t, s2, t2);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_Downscale4x4LogLum(const Texture *srcTexture, float s, float t, float s2, float t2, RenderTarget *dstRT) {
    //float s, t, s2, t2;
    Vec2 sampleOffsets[16];
    Rect prevViewportRect = rhi.GetViewport();

    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::downscale4x4LogLumShader;
    
    if (!(srcTexture->GetFlags() & Texture::Nearest)) {
        float texelOffsetX = 1.0f / srcTexture->GetWidth();
        float texelOffsetY = 1.0f / srcTexture->GetHeight();

        Vec2 *ptr = sampleOffsets;

        for (int y = 0; y < 2; y++) {
            for (int x = 0; x < 2; x++, ptr++) {
                ptr->x = x * texelOffsetX;
                ptr->y = y * texelOffsetY;
            }
        }

        shader->Bind();
        shader->SetTexture("tex0", srcTexture);
        shader->SetConstantArray2f("sampleOffsets", 4, sampleOffsets);

        s = -1.0f / srcTexture->GetWidth();
        t = -1.0f / srcTexture->GetHeight();
        s2 = 1.0f - 1.0f / srcTexture->GetWidth();
        t2 = 1.0f - 1.0f / srcTexture->GetHeight();
    } else {
        float texelOffsetX = 1.0f / srcTexture->GetWidth();
        float texelOffsetY = 1.0f / srcTexture->GetHeight();

        Vec2 *ptr = sampleOffsets;

        for (int y = -2; y < 2; y++) {
            for (int x = -2; x < 2; x++, ptr++) {
                ptr->x = x * texelOffsetX;
                ptr->y = y * texelOffsetY;
            }
        }

        shader->Bind();
        shader->SetTexture("tex0", srcTexture);
        shader->SetConstantArray2f("sampleOffsets", 16, sampleOffsets);

        s = 0.0f;
        t = 0.0f;
        s2 = 1.0f;
        t2 = 1.0f;
    }

    RB_DrawClipRect(s, t, s2, t2);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_Downscale4x4ExpLum(const Texture *srcTexture, RenderTarget *dstRT) {
    float s, t, s2, t2;
    Vec2 sampleOffsets[16];
    Rect prevViewportRect = rhi.GetViewport();

    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::downscale4x4ExpLumShader;
  
    if (!(srcTexture->GetFlags() & Texture::Nearest)) {
        float texelOffsetX = 2.0f / srcTexture->GetWidth();
        float texelOffsetY = 2.0f / srcTexture->GetHeight();

        Vec2 *ptr = sampleOffsets;

        for (int y = 0; y < 2; y++) {
            for (int x = 0; x < 2; x++, ptr++) {
                ptr->x = x * texelOffsetX;
                ptr->y = y * texelOffsetY;
            }
        }

        shader->Bind();
        shader->SetTexture("tex0", srcTexture);
        shader->SetConstantArray2f("sampleOffsets", 4, sampleOffsets);

        s = -1.0f / srcTexture->GetWidth();
        t = -1.0f / srcTexture->GetHeight();
        s2 = 1.0f - 1.0f / srcTexture->GetWidth();
        t2 = 1.0f - 1.0f / srcTexture->GetHeight();
    } else {
        float texelOffsetX = 1.0f / srcTexture->GetWidth();
        float texelOffsetY = 1.0f / srcTexture->GetHeight();

        Vec2 *ptr = sampleOffsets;

        for (int y = -2; y < 2; y++) {
            for (int x = -2; x < 2; x++, ptr++) {
                ptr->x = x * texelOffsetX;
                ptr->y = y * texelOffsetY;
            }
        }

        shader->Bind();
        shader->SetTexture("tex0", srcTexture);
        shader->SetConstantArray2f("sampleOffsets", 16, sampleOffsets);

        s = 0.0f;
        t = 0.0f;
        s2 = 1.0f;
        t2 = 1.0f;
    }

    RB_DrawClipRect(s, t, s2, t2);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_HBlur5x(const Texture *srcTexture, const float *weights, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::blur5xShader;

    shader->Bind();
    shader->SetTexture("tex0", srcTexture);
    shader->SetConstantArray1f("weights", 5, weights);
    shader->SetConstant2f("texelOffset", Vec2(1.0f / srcTexture->GetWidth(), 0.0f));

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_VBlur5x(const Texture *srcTexture, const float *weights, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::blur5xShader;

    shader->Bind();
    shader->SetTexture("tex0", srcTexture);
    shader->SetConstantArray1f("weights", 5, weights);
    shader->SetConstant2f("texelOffset", Vec2(0.0f, 1.0f / srcTexture->GetHeight()));
    
    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_HBlur7x(const Texture *srcTexture, const float *weights, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::blur7xShader;

    shader->Bind();
    shader->SetTexture("tex0", srcTexture);
    shader->SetConstantArray1f("weights", 7, weights);
    shader->SetConstant2f("texelOffset", Vec2(1.0f / srcTexture->GetWidth(), 0.0f));

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_VBlur7x(const Texture *srcTexture, const float *weights, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::blur7xShader;

    shader->Bind();
    shader->SetTexture("tex0", srcTexture);
    shader->SetConstantArray1f("weights", 7, weights);
    shader->SetConstant2f("texelOffset", Vec2(0.0f, 1.0f / srcTexture->GetHeight()));

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_HBlur15x(const Texture *srcTexture, const float *weights, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::blur15xShader;

    shader->Bind();
    shader->SetTexture("tex0", srcTexture);
    shader->SetConstantArray1f("weights", 15, weights);
    shader->SetConstant2f("texelOffset", Vec2(1.0f / srcTexture->GetWidth(), 0.0f));

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_VBlur15x(const Texture *srcTexture, const float *weights, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::blur15xShader;

    shader->Bind();
    shader->SetTexture("tex0", srcTexture);
    shader->SetConstantArray1f("weights", 15, weights);
    shader->SetConstant2f("texelOffset", Vec2(0.0f, 1.0f / srcTexture->GetHeight()));

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_HBlurBilinear3x(const Texture *srcTexture, const Vec2 *sampleOffsets, const float *weights, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::blurBilinear3xShader;

    shader->Bind();
    shader->SetTexture("tex0", srcTexture);
    shader->SetConstantArray1f("weights", 3, weights);
    shader->SetConstantArray2f("sampleOffsets", 3, sampleOffsets);
    shader->SetConstant2f("texelOffset", Vec2(1.0f / srcTexture->GetWidth(), 0.0f));

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_VBlurBilinear3x(const Texture *srcTexture, const Vec2 *sampleOffsets, const float *weights, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::blurBilinear3xShader;
    
    shader->Bind();
    shader->SetTexture("tex0", srcTexture);
    shader->SetConstantArray1f("weights", 3, weights);
    shader->SetConstantArray2f("sampleOffsets", 3, sampleOffsets);
    shader->SetConstant2f("texelOffset", Vec2(0.0f, 1.0f / srcTexture->GetHeight()));

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_HBlurBilinear4x(const Texture *srcTexture, const Vec2 *sampleOffsets, const float *weights, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::blurBilinear4xShader;

    shader->Bind();
    shader->SetTexture("tex0", srcTexture);
    shader->SetConstantArray1f("weights", 4, weights);
    shader->SetConstantArray2f("sampleOffsets", 4, sampleOffsets);
    shader->SetConstant2f("texelOffset", Vec2(1.0f / srcTexture->GetWidth(), 0.0f));

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_VBlurBilinear4x(const Texture *srcTexture, const Vec2 *sampleOffsets, const float *weights, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::blurBilinear4xShader;
   
    shader->Bind();
    shader->SetTexture("tex0", srcTexture);
    shader->SetConstantArray1f("weights", 4, weights);
    shader->SetConstantArray2f("sampleOffsets", 4, sampleOffsets);
    shader->SetConstant2f("texelOffset", Vec2(0.0f, 1.0f / srcTexture->GetHeight()));

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_HBlurBilinear8x(const Texture *srcTexture, const Vec2 *sampleOffsets, const float *weights, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::blurBilinear8xShader;

    shader->Bind();
    shader->SetTexture("tex0", srcTexture);
    shader->SetConstantArray1f("weights", 8, weights);
    shader->SetConstantArray2f("sampleOffsets", 8, sampleOffsets);
    shader->SetConstant2f("texelOffset", Vec2(1.0f / srcTexture->GetWidth(), 0.0f));

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_VBlurBilinear8x(const Texture *srcTexture, const Vec2 *sampleOffsets, const float *weights, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::blurBilinear8xShader;

    shader->Bind();
    shader->SetTexture("tex0", srcTexture);
    shader->SetConstantArray1f("weights", 8, weights);
    shader->SetConstantArray2f("sampleOffsets", 8, sampleOffsets);
    shader->SetConstant2f("texelOffset", Vec2(0.0f, 1.0f / srcTexture->GetHeight()));

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_HBlurAlphaMaskedBilinear8x(const Texture *srcTexture, const Texture *maskTexture, const Vec2 *sampleOffsets, const float *weights, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::blurAlphaMaskedBilinear8xShader;

    shader->Bind();
    shader->SetTexture("tex0", srcTexture);
    shader->SetTexture("tex1", maskTexture);
    shader->SetConstantArray1f("weights", 8, weights);
    shader->SetConstantArray2f("sampleOffsets", 8, sampleOffsets);
    shader->SetConstant2f("texelOffset", Vec2(1.0f / srcTexture->GetWidth(), 0.0f));

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_VBlurAlphaMaskedBilinear8x(const Texture *srcTexture, const Texture *maskTexture, const Vec2 *sampleOffsets, const float *weights, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::blurAlphaMaskedBilinear8xShader;

    shader->Bind();
    shader->SetTexture("tex0", srcTexture);
    shader->SetTexture("tex1", maskTexture);
    shader->SetConstantArray1f("weights", 8, weights);
    shader->SetConstantArray2f("sampleOffsets", 8, sampleOffsets);
    shader->SetConstant2f("texelOffset", Vec2(0.0f, 1.0f / srcTexture->GetHeight()));
    
    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_GaussBlur5x5(const Texture *srcTexture, RenderTarget *tempRT, RenderTarget *dstRT) {
    if (srcTexture->GetFlags() & Texture::Nearest) {
        PP_HBlur5x(srcTexture, gaussKernel5x.discreteWeights, tempRT);
        PP_VBlur5x(tempRT->ColorTexture(), gaussKernel5x.discreteWeights, dstRT);
    } else {
        PP_HBlurBilinear3x(srcTexture, gaussKernel5x.hOffsets, gaussKernel5x.linearWeights, tempRT);
        PP_VBlurBilinear3x(tempRT->ColorTexture(), gaussKernel5x.vOffsets, gaussKernel5x.linearWeights, dstRT);
    }
}

void PP_GaussBlur7x7(const Texture *srcTexture, RenderTarget *tempRT, RenderTarget *dstRT) {
    if (srcTexture->GetFlags() & Texture::Nearest) {
        PP_HBlur7x(srcTexture, gaussKernel7x.discreteWeights, tempRT);
        PP_VBlur7x(tempRT->ColorTexture(), gaussKernel7x.discreteWeights, dstRT);
    } else {
        PP_HBlurBilinear4x(srcTexture, gaussKernel7x.hOffsets, gaussKernel7x.linearWeights, tempRT);
        PP_VBlurBilinear4x(tempRT->ColorTexture(), gaussKernel7x.vOffsets, gaussKernel7x.linearWeights, dstRT);
    }
}

void PP_GaussBlur15x15(const Texture *srcTexture, RenderTarget *tempRT, RenderTarget *dstRT) {
    if (srcTexture->GetFlags() & Texture::Nearest) {
        PP_HBlur15x(srcTexture, gaussKernel15x.discreteWeights, tempRT);
        PP_VBlur15x(tempRT->ColorTexture(), gaussKernel15x.discreteWeights, dstRT);
    } else {
        PP_HBlurBilinear8x(srcTexture, gaussKernel15x.hOffsets, gaussKernel15x.linearWeights, tempRT);
        PP_VBlurBilinear8x(tempRT->ColorTexture(), gaussKernel15x.vOffsets, gaussKernel15x.linearWeights, dstRT);
    }
}

void PP_KawaseBlur(const Texture *srcTexture, int iteration, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);
    
    float offset_x = ((float)iteration + 0.5f) / srcTexture->GetWidth();
    float offset_y = ((float)iteration + 0.5f) / srcTexture->GetHeight();

    Vec2 sampleOffsets[4];
    sampleOffsets[0].x = -offset_x;
    sampleOffsets[0].y = -offset_y;

    sampleOffsets[1].x = +offset_x;
    sampleOffsets[1].y = -offset_y;

    sampleOffsets[2].x = -offset_x;
    sampleOffsets[2].y = +offset_y;

    sampleOffsets[3].x = +offset_x;
    sampleOffsets[3].y = +offset_y;

    const Shader *shader = ShaderManager::kawaseBlurShader;

    shader->Bind();
    shader->SetTexture("tex0", srcTexture);
    shader->SetConstantArray2f("sampleOffsets", 4, sampleOffsets);

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_AoBlur(const Texture *aoMap, const Texture *depthTexture, RenderTarget *tempRT, const Mat4 &projectionMatrix, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    tempRT->Begin();
    rhi.SetViewport(Rect(0, 0, tempRT->GetWidth(), tempRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::aoBlurShader;
    
    shader->Bind();
    shader->SetTexture("colorMap", aoMap);
    shader->SetTexture("depthMap", depthTexture);
    shader->SetConstant2f("texelOffset", Vec2(0.0f, 1.0f / aoMap->GetHeight()));
    shader->SetConstant2f("projComp", Vec2(projectionMatrix[2][2], projectionMatrix[2][3]));

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    rhi.SetViewport(prevViewportRect);
    tempRT->End();

    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite);
    rhi.SetCullFace(RHI::NoCull);

    shader->Bind();
    shader->SetTexture("colorMap", tempRT->ColorTexture());
    shader->SetTexture("depthMap", depthTexture);
    shader->SetConstant2f("texelOffset", Vec2(1.0f / aoMap->GetWidth(), 0.0f));
    shader->SetConstant2f("projComp", Vec2(projectionMatrix[2][2], projectionMatrix[2][3]));

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_LinearizeDepth(const Texture *depthTexture, float zNear, float zFar, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::linearizeDepthShader;
   
    shader->Bind();
    shader->SetTexture("depthMap", depthTexture);
    shader->SetConstant2f("depthRange", Vec2(zNear, zFar));
    
    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_CopyCocToAlpha(const Texture *depthTexture, float zFar, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::copyDownscaledCocToAlphaShader;
   
    shader->Bind();
    shader->SetTexture("depthMap", depthTexture);
    shader->SetConstant2f("maxBlurDistance", Vec2(r_DOF_maxBlurNear.GetFloat(), r_DOF_maxBlurFar.GetFloat()));
    shader->SetConstant1f("focalDistance", r_DOF_focalDistance.GetFloat());
    shader->SetConstant1f("noBlurFraction", r_DOF_noBlurFraction.GetFloat());
    shader->SetConstant1f("cocScale", r_DOF_cocScale.GetFloat());
    shader->SetConstant1f("zFar", zFar);

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_CopyColorAndCoc(const Texture *colorTexture, const Texture *depthTexture, float zFar, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::copyColorAndCocShader;
   
    shader->Bind();
    shader->SetTexture("colorMap", colorTexture);
    shader->SetTexture("depthMap", depthTexture);
    shader->SetConstant2f("maxBlurDistance", Vec2(r_DOF_maxBlurNear.GetFloat(), r_DOF_maxBlurFar.GetFloat()));
    shader->SetConstant1f("focalDistance", r_DOF_focalDistance.GetFloat());
    shader->SetConstant1f("noBlurFraction", r_DOF_noBlurFraction.GetFloat());
    shader->SetConstant1f("cocScale", r_DOF_cocScale.GetFloat());
    shader->SetConstant1f("zFar", zFar);

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_ApplyDOF(const Texture *tex0, const Texture *tex1, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::applyDofShader;
    
    shader->Bind();
    shader->SetTexture("tex0", tex0);
    shader->SetTexture("tex1", tex1);
    shader->SetConstant2f("tex0texelSize", Vec2(1.0f / tex0->GetWidth(), 1.0f / tex0->GetHeight()));
    shader->SetConstant2f("tex1texelSize", Vec2(1.0f / tex1->GetWidth(), 1.0f / tex1->GetHeight()));

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_SunShaftsMaskGen(const Texture *colorTexture, const Texture *depthTexture, float s, float t, float s2, float t2, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::sunShaftsMaskGenShader;
    
    shader->Bind();
    shader->SetTexture("colorMap", colorTexture);
    shader->SetTexture("depthMap", depthTexture);
    shader->SetConstant2f("texelSize", Vec2(1.0f / colorTexture->GetWidth(), 1.0f / colorTexture->GetHeight()));

    RB_DrawClipRect(s, t, s2, t2);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_SunShaftsGen(const Texture *srcTexture, const Mat4 &viewProjectionMatrix, const Vec3 &worldSunPos, float shaftScale, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::sunShaftsGenShader;
    
    shader->Bind();
    shader->SetTexture("sunShaftsMap", srcTexture);
    shader->SetConstant3f("worldSunPos", worldSunPos);
    shader->SetConstant1f("shaftScale", shaftScale);
    shader->SetConstant4x4f("viewProjectionMatrix", true, viewProjectionMatrix);

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_SunShaftsDisplay(const Texture *screenTexture, const Texture *tex1, const Vec4 &sunColor, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::sunShaftsDisplayShader;
    
    shader->Bind();
    shader->SetTexture("screenMap", screenTexture);
    shader->SetTexture("sunShaftsMap", tex1);
    shader->SetConstant4f("sunColor", sunColor);

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_ObjectMotionBlur(const Texture *srcTexture, const Texture *velocityTexture, float s, float t, float s2, float t2, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::postObjectMotionBlurShader;
    
    shader->Bind();
    shader->SetTexture("colorMap", srcTexture);
    //shader->SetTexture("depthMap", backEnd.ctx->screenDepthTexture);
    shader->SetTexture("velocityMap", velocityTexture);
    shader->SetConstant1f("motionBlurScale", r_motionBlur_Scale.GetFloat());
    shader->SetConstant1f("motionBlurMax", r_motionBlur_MaxBlur.GetFloat());

    RB_DrawClipRect(0.0, 0.0, 1.0, 1.0);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_CameraMotionBlur(const Texture *srcTexture, const Texture *depthTexture, const Mat4 viewProjectionMatrix[2], const Vec3 &cameraPos, float blurScale, float frameTime, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::postCameraMotionBlurShader;
    
    shader->Bind();
    shader->SetTexture("colorMap", srcTexture);
    shader->SetTexture("depthMap", depthTexture);
    shader->SetConstant3f("worldViewPos", cameraPos);
    shader->SetConstant1f("shutterSpeed", r_motionBlur_ShutterSpeed.GetFloat() / frameTime);
    shader->SetConstant1f("blurScale", blurScale);
    shader->SetConstant1f("maxBlur", r_motionBlur_MaxBlur.GetFloat());
    
    shader->SetConstant4x4f("currViewProjectionMatrix", true, viewProjectionMatrix[0]);
    shader->SetConstant4x4f("prevViewProjectionMatrix", true, viewProjectionMatrix[1]);
    
    rhi.BindBuffer(RHI::VertexBuffer, sphereVB);
    rhi.SetVertexFormat(vertexFormats[VertexFormat::Xyz].vertexFormatHandle);
    rhi.SetStreamSource(0, sphereVB, 0, sizeof(Vec3));
    rhi.DrawArrays(RHI::TriangleStripPrim, 0, 2 * sphereLats * sphereLongs);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_MeasureLuminance(const Texture *srcTexture, const float *screenTc, RenderTarget *dstRT) {
    // luminance 값을 구하면서 4분의 1로 downscale
    PP_Downscale4x4LogLum(srcTexture, screenTc[0], screenTc[1], screenTc[2], screenTc[3], backEnd.ctx->hdrLumAverageRT[0]);

    // 4x4 이하가 될때까지 downsampling
    RenderTarget *lsrc = backEnd.ctx->hdrLumAverageRT[0];
    RenderTarget *ldst = backEnd.ctx->hdrLumAverageRT[1];

    int i;
    for (i = 1; lsrc->GetWidth() > 4 || lsrc->GetHeight() > 4; i++) {
        PP_Downscale4x4(lsrc->ColorTexture(), ldst);

        lsrc = ldst;
        ldst = backEnd.ctx->hdrLumAverageRT[i + 1];
    }

    // 1x1 로 최종 luminance 값을 구함
    PP_Downscale4x4ExpLum(lsrc->ColorTexture(), dstRT);
}

void PP_LuminanceAdaptation(const Texture *srcTexture0, const Texture *srcTexture1, float frameTime, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::luminanceAdaptationShader;
    
    shader->Bind();
    shader->SetTexture("currLuminanceMap", srcTexture0);
    shader->SetTexture("prevLuminanceMap", srcTexture1);
    shader->SetConstant1f("frameTime", frameTime);

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_BrightFilter(const Texture *srcTexture, const Texture *luminanceTexture, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::brightFilterShader;
   
    shader->Bind();
    shader->SetTexture("tex0", srcTexture);
    //shader->SetConstant2f("texelOffset", Vec2(1.0f / srcTexture->GetWidth(), 1.0f / srcTexture->GetHeight()));
    shader->SetTexture("luminanceMap", luminanceTexture);
    shader->SetConstant1f("brightLevel", r_HDR_brightLevel.GetFloat());
    shader->SetConstant1f("brightThrehold", r_HDR_brightThrehold.GetFloat());
    shader->SetConstant1f("brightOffset", r_HDR_brightOffset.GetFloat());
    shader->SetConstant1f("brightMax", r_HDR_brightMax.GetFloat());

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_ChromaShift(const Texture *srcTexture, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);

    const Shader *shader = ShaderManager::chromaShiftShader;
    
    shader->Bind();
    shader->SetTexture("tex0", srcTexture);
    shader->SetConstant3f("shift", Vec3(0.02f, 0.01f, 0.02f));

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

void PP_SSAO(const Texture *depthTexture, const Texture *downscaledDepthTexture, const view_t *view, RenderTarget *dstRT) {
    Rect prevViewportRect = rhi.GetViewport();
    dstRT->Begin();
    rhi.SetViewport(Rect(0, 0, dstRT->GetWidth(), dstRT->GetHeight()));

    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::NoCull);
    
    const Shader *shader = ShaderManager::ssaoShader;
    
    shader->Bind();
    shader->SetTexture("depthMap", depthTexture);
    shader->SetTexture("downscaledDepthMap", downscaledDepthTexture);
    shader->SetTexture("randomDir4x4Map", textureManager.randomDir4x4Texture);
    shader->SetConstantArray3f("randomKernel", 8, ssaoRandomKernel);
    shader->SetConstant2f("screenSize", Vec2(view->def->parms.renderRect.w, view->def->parms.renderRect.h));
    shader->SetConstant2f("screenTanHalfFov", Vec2(view->def->frustum.GetLeft() / view->def->frustum.GetFarDistance(), view->def->frustum.GetUp() / view->def->frustum.GetFarDistance()));
    shader->SetConstant4f("projComp1", Vec4(view->def->projMatrix[0][0], view->def->projMatrix[1][1], view->def->projMatrix[0][2], view->def->projMatrix[1][2]));
    shader->SetConstant4f("projComp2", Vec4(view->def->projMatrix[2][2], view->def->projMatrix[2][3], 0.0f, 0.0f));
    shader->SetConstant1f("radius", CentiToUnit(r_SSAO_radius.GetFloat()));
    shader->SetConstant1f("validRange", CentiToUnit(r_SSAO_validRange.GetFloat()));
    shader->SetConstant1f("threshold", CentiToUnit(r_SSAO_threshold.GetFloat()));
    shader->SetConstant1f("power", r_SSAO_power.GetFloat());
    shader->SetConstant1f("intensity", r_SSAO_intensity.GetFloat());

    RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

    dstRT->End();
    rhi.SetViewport(prevViewportRect);
}

BE_NAMESPACE_END
