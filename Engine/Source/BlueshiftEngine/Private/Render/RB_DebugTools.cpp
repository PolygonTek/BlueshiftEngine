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
#include "Simplex.h"

BE_NAMESPACE_BEGIN

struct DebugVert { 
    Vec3            xyz; 
    byte            color[4]; 
};

struct DebugPrims {
    int             prims;
    int             startVert;
    int             numVerts;
    int             lifeTime;
    float           lineWidth;
    byte            color[4];
    bool            twoSided;
    bool            depthTest;
};

static const int    MaxDebugPrims = 16384;
static const int    MaxDebugPrimsVerts = 65536;

static DebugPrims   rb_debugPrims[MaxDebugPrims];
static int          rb_numDebugPrims = 0;
static Vec3         rb_debugPrimsVerts[MaxDebugPrimsVerts];
static int          rb_numDebugPrimsVerts = 0;
static int          rb_debugPrimsTime = 0;

static bool         useSmoothLine = true; // TEMP

struct DebugText {
    Str             text;
    Vec3            origin;
    Mat3            viewAxis;
    float           scale;
    float           lineWidth;
    byte            color[4];
    int             align;
    int             lifeTime;
    bool            depthTest;
};

static const int    MaxDebugText = 4096;
static DebugText    rb_debugText[MaxDebugText];
static int          rb_numDebugText = 0;
static int          rb_debugTextTime = 0;

void RB_ClearDebugPrimitives(int time) {
    rb_debugPrimsTime = time;

    if (!time) {
        rb_numDebugPrims = 0;
        rb_numDebugPrimsVerts = 0;
        return;
    }

    // copy any text that still needs to be drawn
    int numVerts = 0;
    int num = 0;
    DebugPrims *prims = rb_debugPrims;
    for (int i = 0; i < rb_numDebugPrims; i++, prims++) {
        if (prims->lifeTime > time) {
            if (prims->startVert != numVerts) {
                // NOTE: RB_DrawDebugPrims 에서 vertex array 를 sorting 한다면 임시 memory 에 copy 하는 식으로 바꿔야 함 
                memmove(&rb_debugPrimsVerts[numVerts], &rb_debugPrimsVerts[prims->startVert], prims->numVerts * sizeof(rb_debugPrimsVerts[0]));				
            }

            rb_debugPrims[num] = *prims;
            rb_debugPrims[num].startVert = numVerts;
            
            numVerts += prims->numVerts;
            num++;
        }
    }
    rb_numDebugPrims = num;
    rb_numDebugPrimsVerts = numVerts;
}

Vec3 *RB_ReserveDebugPrimsVerts(int prims, int numVerts, const Color4 &color, const float lineWidth, const bool twoSided, const bool depthTest, const int lifeTime) {
    DebugPrims *debugPrims;
    byte rgba[4];

    if (rb_numDebugPrimsVerts + numVerts > MaxDebugPrimsVerts) {
        return nullptr;
    }

    if (rb_numDebugPrims < MaxDebugPrims) {
        rgba[0] = color[0] * 255;
        rgba[1] = color[1] * 255;
        rgba[2] = color[2] * 255;
        rgba[3] = color[3] * 255;

        debugPrims = &rb_debugPrims[rb_numDebugPrims++];
        *reinterpret_cast<uint32_t *>(debugPrims->color) = *reinterpret_cast<uint32_t *>(rgba);
        debugPrims->prims		= prims;
        debugPrims->startVert	= rb_numDebugPrimsVerts;
        debugPrims->numVerts	= numVerts;
        debugPrims->lineWidth	= lineWidth;
        debugPrims->depthTest	= depthTest;
        debugPrims->twoSided	= twoSided;
        debugPrims->lifeTime	= rb_debugPrimsTime + lifeTime;

        rb_numDebugPrimsVerts += numVerts;

        return &rb_debugPrimsVerts[debugPrims->startVert];
    }

    return nullptr;
}

static void RB_DrawDebugPrimsElements(int numElements, const int *elements, int numVerts, float lineWidth, bool twoSided, bool depthTest) {
    int size = numVerts * sizeof(DebugVert);
    DebugVert *verts = (DebugVert *)_alloca16(size);
    DebugVert *vptr = verts;

    int prims = rb_debugPrims[elements[0]].prims;
    bool needAlphaBlend = false;
    
    for (int i = 0; i < numElements; i++) {
        const DebugPrims *debugPrims = &rb_debugPrims[elements[i]];
        
        for (int j = 0; j < debugPrims->numVerts; j++) {
            vptr->xyz = rb_debugPrimsVerts[debugPrims->startVert + j];
            *reinterpret_cast<uint32_t *>(vptr->color) = *reinterpret_cast<const uint32_t *>(debugPrims->color);
            vptr++;

            if (debugPrims->color[3] < 255) {
                needAlphaBlend = true;
            }
        }
    }

    if (prims >= RHI::LinesPrim && prims <= RHI::LineLoopPrim) {
        if (useSmoothLine) {
            rhi.EnableLineSmooth(true);
        }
        rhi.SetLineWidth(lineWidth);
    }

    const Shader *shader = ShaderManager::vertexColorShader;

    shader->Bind();
    shader->SetConstant4x4f("modelViewProjectionMatrix", true, backEnd.view->def->viewProjMatrix);	
    
    rhi.BindBuffer(RHI::VertexBuffer, bufferCacheManager.streamVertexBuffer);
    rhi.BufferDiscardWrite(bufferCacheManager.streamVertexBuffer, size, verts);

    rhi.SetVertexFormat(vertexFormats[VertexFormat::XyzColor].vertexFormatHandle);
    rhi.SetStreamSource(0, bufferCacheManager.streamVertexBuffer, 0, sizeof(DebugVert));

    int stateBits = RHI::PM_Solid | RHI::ColorWrite;
    if (depthTest) {
        stateBits |= RHI::DF_LEqual;
    }

    if (needAlphaBlend || useSmoothLine) {
        stateBits |= RHI::BS_SrcAlpha | RHI::BD_OneMinusSrcAlpha;
    } else {
        stateBits |= RHI::DepthWrite;
    }

    rhi.SetStateBits(stateBits);

    int cullMode = twoSided ? RHI::NoCull : RHI::BackCull;
    rhi.SetCullFace(cullMode);
    rhi.DrawArrays((RHI::Primitive)prims, 0, numVerts);

    if (prims >= RHI::LinesPrim && prims <= RHI::LineLoopPrim) {
        if (useSmoothLine) {
            rhi.EnableLineSmooth(false);
        }
        rhi.SetLineWidth(1);
    }
}

static int RB_CompareDebugPrims(const void *elem1, const void *elem2) {
    const DebugPrims *p1 = (DebugPrims *)elem1;
    const DebugPrims *p2 = (DebugPrims *)elem2;

    int sortKey1 = (p1->depthTest << 5) | (((p1->color[3] + 1) & 0x100) >> 4) | (p1->prims);
    int sortKey2 = (p2->depthTest << 5) | (((p2->color[3] + 1) & 0x100) >> 4) | (p2->prims);
    
    return sortKey1 - sortKey2;
}

static void RB_DrawDebugPrims() {
    static const int maxVerts = 32768;

    if (!rb_numDebugPrims) {
        return;
    }
    
    //qsort(rb_debugPrims, rb_numDebugPrims, sizeof(rb_debugPrims[0]), RB_CompareDebugPrims);

    int elements[MaxDebugPrims];
    int numElements = 0;
    int numVerts = 0;
    int prev_prims = -1;
    int prev_alpha = 0;
    float prev_lineWidth = 0;
    bool prev_twoSided = false;
    bool prev_depthTest = false;
    
    const DebugPrims *debugPrims = rb_debugPrims;
    for (int i = 0; i < rb_numDebugPrims; i++, debugPrims++) {
        int num = debugPrims->numVerts;

        if (numVerts == 0) {
            prev_prims = debugPrims->prims;
            prev_alpha = debugPrims->color[3];
            prev_lineWidth = debugPrims->lineWidth;
            prev_twoSided = debugPrims->twoSided;
            prev_depthTest = debugPrims->depthTest;
        } else {
            bool mergeablePrims = (prev_prims == debugPrims->prims) && (prev_prims == RHI::LinesPrim || prev_prims == RHI::TrianglesPrim);

            if (numVerts + num > maxVerts || !mergeablePrims || prev_alpha != debugPrims->color[3] || 
                (prev_prims == RHI::LinesPrim && prev_lineWidth != debugPrims->lineWidth) ||
                prev_twoSided != debugPrims->twoSided || prev_depthTest != debugPrims->depthTest) {
                RB_DrawDebugPrimsElements(numElements, elements, numVerts, prev_lineWidth, prev_twoSided, prev_depthTest);

                numVerts = 0;
                numElements = 0;

                prev_prims = debugPrims->prims;
                prev_alpha = debugPrims->color[3];
                prev_lineWidth = debugPrims->lineWidth;
                prev_twoSided = debugPrims->twoSided;
                prev_depthTest = debugPrims->depthTest;
            }
        }

        elements[numElements++] = i;

        numVerts += num;
    }

    if (numVerts > 0) {
        RB_DrawDebugPrimsElements(numElements, elements, numVerts, prev_lineWidth, prev_twoSided, prev_depthTest);
    }
}

void RB_ClearDebugText(int time) {
    rb_debugTextTime = time;

    if (!time) {
        // free up our strings
        DebugText *text = rb_debugText;
        for (int i = 0; i < MaxDebugText; i++, text++) {
            text->text.Clear();
        }
        rb_numDebugText = 0;
        return;
    }

    // copy any text that still needs to be drawn
    int num	= 0;
    DebugText *text = rb_debugText;
    for (int i = 0; i < rb_numDebugText; i++, text++) {
        if (text->lifeTime > time) {
            if (num != i) {
                rb_debugText[num] = *text;
            }
            num++;
        }
    }
    rb_numDebugText = num;
}

void RB_AddDebugText(const char *text, const Vec3 &origin, const Mat3 &viewAxis, float scale, float lineWidth, const Color4 &color, const int align, const int lifeTime, const bool depthTest) {
    DebugText *debugText;
    byte rgba[4];

    if (rb_numDebugText < MaxDebugText) {
        rgba[0] = color[0] * 255;
        rgba[1] = color[1] * 255;
        rgba[2] = color[2] * 255;
        rgba[3] = color[3] * 255;

        debugText = &rb_debugText[rb_numDebugText++];
        *reinterpret_cast<uint32_t *>(debugText->color) = *reinterpret_cast<uint32_t *>(rgba);	
        debugText->text			= text;
        debugText->origin		= origin;
        debugText->viewAxis		= viewAxis;	
        debugText->scale		= scale;
        debugText->lineWidth	= lineWidth;
        debugText->align		= align;
        debugText->lifeTime		= rb_debugTextTime + lifeTime;
        debugText->depthTest	= depthTest;
    }
}

// returns the length of the given text
static float RB_DrawTextLength(const char *text, float scale, int len) {
    float textLen = 0.0f;

    if (text && *text) {
        if (!len) {
            len = (int)strlen(text);
        }

        for (int i = 0; i < len; i++) {
            int charIndex = text[i] - 32;
            if (charIndex < 0 || charIndex > NUM_SIMPLEX_CHARS) {
                continue;
            }
            int num = simplex[charIndex][0] * 2;
            float spacing = simplex[charIndex][1];
            textLen += spacing * scale;  
        }
    }
    return textLen;
}

static int RB_DrawTextNumVerts(const char *text, int len) {
    int numVerts = 0;

    if (text && *text) {
        if (!len) {
            len = (int)strlen(text);
        }

        for (int i = 0; i < len; i++) {
            int charIndex = text[i] - 32;
            if (charIndex < 0 || charIndex > NUM_SIMPLEX_CHARS) {
                continue;
            }
            int num = simplex[charIndex][0] * 2;
            int index = 2;

            while (index - 2 < num) {
                if (simplex[charIndex][index] < 0) {  
                    index++;
                    continue; 
                }
                
                index += 2;
                if (simplex[charIndex][index] < 0) {
                    index++;
                    continue;
                }

                numVerts += 2;
            }
        }
    }
    return numVerts;
}

// oriented on the viewaxis
// align can be 0-left, 1-center (default), 2-right
static void RB_AddDebugTextVerts(DebugVert **dstPtr, const char *text, const Vec3 &origin, float scale, const byte *color, const Mat3 &viewAxis, const int align) {
    int line;
    float textLen;
    Vec3 org;

    scale /= 36.0f;

    if (text && *text) {
        if (text[0] == '\n') {
            line = 1;
        } else {
            line = 0;
        }

        int len = (int)strlen(text);
        for (int i = 0; i < len; i++) {
            if (i == 0 || text[i] == '\n') {
                org = origin - viewAxis[2] * (line * 36.0f * scale);
                if (align != 0) {
                    for (int j = 1; i + j <= len; j++) {
                        if (i + j == len || text[i + j] == '\n') {
                            textLen = RB_DrawTextLength(text + i, scale, j);
                            break;
                        }
                    }
                    if (align == 2) {
                        // right
                        org += viewAxis[1] * textLen;
                    } else {
                        // center
                        org += viewAxis[1] * (textLen * 0.5f);
                    }
                }
                line++;
            }

            int charIndex = text[i] - 32;
            if (charIndex < 0 || charIndex > NUM_SIMPLEX_CHARS) {
                continue;
            }

            int num = simplex[charIndex][0] * 2;
            float spacing = simplex[charIndex][1];
            int index = 2;

            while (index - 2 < num) {
                if (simplex[charIndex][index] < 0) {  
                    index++;
                    continue; 
                }
                Vec3 p1 = org + scale * simplex[charIndex][index] * -viewAxis[1] + scale * simplex[charIndex][index + 1] * viewAxis[2];
                
                index += 2;
                if (simplex[charIndex][index] < 0) {
                    index++;
                    continue;
                }
                Vec3 p2 = org + scale * simplex[charIndex][index] * -viewAxis[1] + scale * simplex[charIndex][index + 1] * viewAxis[2];

                (*dstPtr)->xyz = p1;
                *reinterpret_cast<uint32_t *>((*dstPtr)->color) = *reinterpret_cast<const uint32_t *>(color);	
                (*dstPtr)++;
                
                (*dstPtr)->xyz = p2;
                *reinterpret_cast<uint32_t *>((*dstPtr)->color) = *reinterpret_cast<const uint32_t *>(color);	
                (*dstPtr)++;
            }
            org -= viewAxis[1] * (spacing * scale);
        }
    }
}

static void RB_DrawDebugTextElements(int numElements, const int *elements, int numVerts, float lineWidth, bool depthTest) {	
    int size = numVerts * sizeof(DebugVert);
    DebugVert *verts = (DebugVert *)_alloca16(size);
    DebugVert *vptr = verts;

    bool needAlphaBlend = false;

    for (int i = 0; i < numElements; i++) {
        const DebugText *text = &rb_debugText[elements[i]];
        if (text->color[3] < 255) {
            needAlphaBlend = true;
        }
        RB_AddDebugTextVerts(&vptr, text->text, text->origin, text->scale, text->color, text->viewAxis, text->align);
    }

    if (useSmoothLine) {
        rhi.EnableLineSmooth(true);
    }
    rhi.SetLineWidth(lineWidth);

    const Shader *shader = ShaderManager::vertexColorShader;

    shader->Bind();
    shader->SetConstant4x4f("modelViewProjectionMatrix", true, backEnd.view->def->viewProjMatrix);	
    
    rhi.BindBuffer(RHI::VertexBuffer, bufferCacheManager.streamVertexBuffer);
    rhi.BufferDiscardWrite(bufferCacheManager.streamVertexBuffer, size, verts);

    rhi.SetVertexFormat(vertexFormats[VertexFormat::XyzColor].vertexFormatHandle);
    rhi.SetStreamSource(0, bufferCacheManager.streamVertexBuffer, 0, sizeof(DebugVert));

    int stateBits = RHI::PM_Solid | RHI::ColorWrite;
    if (depthTest) {
        stateBits |= RHI::DF_LEqual;
    }

    if (needAlphaBlend || useSmoothLine) {
        stateBits |= RHI::BS_SrcAlpha | RHI::BD_OneMinusSrcAlpha;
    } else {
        stateBits |= RHI::DepthWrite;
    }

    rhi.SetStateBits(stateBits);
    rhi.DrawArrays(RHI::LinesPrim, 0, numVerts);

    rhi.SetLineWidth(1);
    if (useSmoothLine) {
        rhi.EnableLineSmooth(false);
    }
}

static void RB_DrawDebugTextWithDepthTest(bool depthTest) {
    static const int maxVerts = 32768;

    int elements[MaxDebugText];
    int numElements = 0;
    
    int numVerts = 0;
    float prev_lineWidth = 0;
    byte prev_alpha = 0;

    const DebugText *text = rb_debugText;
    for (int i = 0; i < rb_numDebugText; i++, text++) {
        //if (text->origin.DistanceSqr(backEnd.view->def->parms.origin) > MeterToUnit(100*100)) {
        //	continue;
        //}

        if (text->depthTest != depthTest) {
            continue;
        }

        int num = RB_DrawTextNumVerts(text->text, 0);		

        if (numVerts == 0) {
            prev_lineWidth = text->lineWidth;
            prev_alpha = text->color[3];
        } else {
            if (numVerts + num > maxVerts || (numVerts > 0 && text->lineWidth != prev_lineWidth) || prev_alpha != text->color[3]) {
                RB_DrawDebugTextElements(numElements, elements, numVerts, prev_lineWidth, depthTest);
                numVerts = 0;
                numElements = 0;
            }
            
            prev_lineWidth = text->lineWidth;
            prev_alpha = text->color[3];
        }

        elements[numElements++] = i;

        numVerts += num;
    }

    if (numElements > 0) {
        RB_DrawDebugTextElements(numElements, elements, numVerts, prev_lineWidth, depthTest);
    }
}

static void RB_DrawDebugText() {
    if (!rb_numDebugText) {
        return;
    }

    RB_DrawDebugTextWithDepthTest(true);
    RB_DrawDebugTextWithDepthTest(false);
}

void RB_DrawTris(int numDrawSurfs, DrawSurf **drawSurfs, bool forceToDraw) {
    uint64_t            prevSortkey = -1;
    const Material *    prevMaterial = nullptr;
    const viewEntity_t *prevSpace = nullptr;
    bool                depthhack = false;
    bool                prevDepthHack = false;

    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];

        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (!forceToDraw && !(surf->flags & DrawSurf::ShowWires)) {
            continue;
        }
        
        if (surf->sortKey != prevSortkey) {
            if (surf->material->GetSort() == Material::Sort::SkySort) {
                continue;
            }

            if (surf->material != prevMaterial || surf->space != prevSpace) {
                if (prevMaterial) {
                    backEnd.rbsurf.Flush();
                }

                backEnd.rbsurf.Begin(RBSurf::TriFlush, surf->material, surf->materialRegisters, surf->space, nullptr);

                prevMaterial = surf->material;
            }

            if (surf->space != prevSpace) {
                prevSpace = surf->space;

                backEnd.modelViewMatrix = surf->space->modelViewMatrix;
                backEnd.modelViewProjMatrix = surf->space->modelViewProjMatrix;

                depthhack = surf->space->def->parms.depthHack;
            
                if (prevDepthHack != depthhack) {
                    if (depthhack) {
                        rhi.SetDepthRange(0.0f, 0.1f);
                    } else {
                        rhi.SetDepthRange(0.0f, 1.0f);
                    }

                    prevDepthHack = depthhack;
                }
            }

            prevSortkey = surf->sortKey;
        }

        backEnd.rbsurf.DrawSubMesh(surf->subMesh);
    }

    if (prevMaterial) {
        backEnd.rbsurf.Flush();
    }

    // restore depthhack
    if (depthhack) {
        rhi.SetDepthRange(0.0f, 1.0f);
    }
}

static void RB_DrawDebugLights(int mode) {
    if (mode == 2) {
        rhi.SetDepthRange(0.0f, 0.0f);
    }

    for (viewLight_t *viewLight = backEnd.viewLights; viewLight; viewLight = viewLight->next) {
        if (r_useLightOcclusionQuery.GetBool() && !viewLight->occlusionVisible) {
            continue;
        }
        
        rhi.SetStateBits(RHI::ColorWrite | RHI::BS_SrcAlpha | RHI::BD_One | RHI::DF_LEqual);
        rhi.SetCullFace(RHI::BackCull);

        const Shader *shader = ShaderManager::constantColorShader;

        shader->Bind();
        shader->SetConstant4x4f("modelViewProjectionMatrix", true, backEnd.view->def->viewProjMatrix);

        shader->SetConstant4f("color", Color4(Color3(&viewLight->def->parms.materialParms[SceneEntity::RedParm]), 0.25f));
        RB_DrawLightVolume(viewLight->def);

        rhi.SetStateBits(RHI::ColorWrite | RHI::PM_Wireframe | RHI::DF_LEqual);
        rhi.SetCullFace(RHI::NoCull);

        shader->SetConstant4f("color", &viewLight->def->parms.materialParms[SceneEntity::RedParm]);
    
        RB_DrawLightVolume(viewLight->def);

        RB_DrawAABB(viewLight->litSurfsAABB);
    }

    if (mode == 2) {
        rhi.SetDepthRange(0.0f, 1.0f);
    }
}

static void RB_DrawDebugLightScissorRects() {
    for (viewLight_t *viewLight = backEnd.viewLights; viewLight; viewLight = viewLight->next) {
        if (r_useLightOcclusionQuery.GetBool() && !viewLight->occlusionVisible) {
            continue;
        }

        rhi.SetStateBits(RHI::ColorWrite | RHI::PM_Wireframe);
        rhi.SetCullFace(RHI::NoCull);
        
        const Shader *shader = ShaderManager::postPassThruColorShader;

        shader->Bind();
        shader->SetTexture("tex0", textureManager.whiteTexture);
        shader->SetConstant3f("color", &viewLight->def->parms.materialParms[SceneEntity::RedParm]);

        Rect drawRect = viewLight->scissorRect;
        drawRect.y = backEnd.ctx->GetRenderingHeight() - drawRect.Y2();

        float x = drawRect.x * backEnd.upscaleFactor.x;
        float y = drawRect.y * backEnd.upscaleFactor.y;
        float w = drawRect.w * backEnd.upscaleFactor.x;
        float h = drawRect.h * backEnd.upscaleFactor.y;
        
        RB_DrawScreenRect(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f);
    }
}

void RB_DebugPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    /*if (r_showTangentSpace.GetBool()) {
        DrawDebugTangentSpace(r_showTangentSpace.GetInteger() - 1);
    }

    if (r_showNormals.GetBool()) {
        RB_DrawDebugNormals(r_showNormals.GetInteger());
    }

    if (r_showTangents.GetBool()) {
        RB_DrawDebugTangents(r_showTangents.GetInteger());
    }

    if (r_showUnsmoothedTangents.GetBool()) {
        if (m_material->m_flags & UnsmoothTangents) {
            GL_BindShader(BGL_NULL_SHADER);

            GL_BindBuffer(BGL_VERTEX_BUFFER, vbHandle);

            //g_textureManager.m_whiteTexture->Bind();
            GL_DisableTexture();

            bglColor4ub(255, 255, 0, 127);

            rhi.SetStateBits(ColorWrite | DF_LEqual | BS_SrcAlpha | BD_OneMinusSrcAlpha);
            rhi.SetCullFace(NoCull);

            bglDisableClientState(GL_TEXTURE_COORD_ARRAY);

            GL_ArrayOffset(BGL_VERTEX_ARRAY, 3, BGL_FLOAT, false, sizeof(cDrawVert), 0);

            DrawPrimitives();
            
            bglEnableClientState(GL_TEXTURE_COORD_ARRAY);
        }
    }*/

    if (r_showLights.GetInteger() != 0) {
        RB_DrawDebugLights(r_showLights.GetInteger());
    }

    RB_DrawDebugPrims();
    RB_DrawDebugText();
    
    if (r_showLightScissorRects.GetBool()) {
        RB_DrawDebugLightScissorRects();
    }
}

BE_NAMESPACE_END
