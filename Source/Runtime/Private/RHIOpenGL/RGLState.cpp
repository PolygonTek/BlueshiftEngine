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
#include "RHI/RHIOpenGL.h"
#include "RGLInternal.h"

BE_NAMESPACE_BEGIN

const GLenum toGLStencilFunc[] = {
    GL_ALWAYS,
    GL_NEVER,
    GL_LESS,
    GL_LEQUAL,
    GL_GREATER,
    GL_GEQUAL,
    GL_EQUAL,
    GL_NOTEQUAL
};

const GLenum toGLStencilOp[] = {
    GL_KEEP,
    GL_ZERO,
    GL_REPLACE,
    GL_INCR,
    GL_INCR_WRAP,
    GL_DECR,
    GL_DECR_WRAP,
    GL_INVERT
};

void OpenGLRHI::SetDefaultState() {
    *currentContext->state = GLState();

    renderTargetList[0]->fbo = currentContext->defaultFramebuffer;
    renderTargetList[0]->sRGB = true;// !linearFrameBuffer;
    
    gglBindVertexArray(currentContext->defaultVAO);
    
    for (int i = 0; i < VertexElement::MaxUsages; i++) {
        gglDisableVertexAttribArray(i);
    }
    
    if (OpenGL::SupportsPolygonMode()) {
        OpenGL::PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    gglDisable(GL_DEPTH_TEST);
    gglDisable(GL_BLEND);
    //gglDisable(GL_ALPHA_TEST);
    gglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    gglDepthMask(GL_FALSE);
    
    SetStateBits(ColorWrite | AlphaWrite | DepthWrite | DF_LEqual);
    
    gglFrontFace(GL_CCW);
    SetCullFace(BackCull);
    
    //gglDisable(GL_MULTISAMPLE);
    
    if (OpenGL::SupportsLineSmooth()) {
        gglHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    }
    
#ifdef GL_VERSION_3_2
    // NVIDIA DRIVER BUG: GL_TEXTURE_CUBE_MAP_SEAMLESS 가 켜진 상태에서 some internal format 의 cubemap 을 read access 할때 crash 된다.
    gglEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
#endif
    gglEnable(GL_DITHER);
    gglDisable(GL_STENCIL_TEST);
    //gglDisable(GL_LIGHTING);
    gglStencilMask(0);
    OpenGL::DepthRange(0, 1);
    
    // deprecated at OpenGL 3.0
    //gglShadeModel(GL_SMOOTH);
    //gglHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    //gglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    OpenGL::ClearDepth(1.0);
    gglClearStencil(0);
    gglClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    gglActiveTexture(GL_TEXTURE0);
    
    gglUseProgram(0);
}

void OpenGLRHI::SetStateBits(unsigned int stateBits) {
    unsigned int    state_delta;
    unsigned int    bits;
    GLenum          blend_src;
    GLenum          blend_dst;
    GLboolean       redWrite, greenWrite, blueWrite, alphaWrite;
    
    state_delta = currentContext->state->renderState ^ stateBits;
    //state_delta = ~0;
    
    if (state_delta) {
        // polygon mode
        if (OpenGL::SupportsPolygonMode() && (state_delta & MaskPM)) {
            bits = (stateBits & MaskPM);
            switch (bits) {
                case PM_Point:
                    OpenGL::PolygonMode(GL_FRONT_AND_BACK, GL_POINT);
                    break;
                case PM_Wireframe:
                    OpenGL::PolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                    break;
                case PM_Solid:
                    OpenGL::PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                    break;
                default:
                    BE_FATALERROR(L"OpenGLRHI::SetStateBits: invalid polygon mode state bits");
                    break;
            }
        }
        
        // depth func
        if (state_delta & MaskDF) {
            if (stateBits & MaskDF) {
                bits = (stateBits & MaskDF);
                switch (bits) {
                    case DF_Always:
                        gglDepthFunc(GL_ALWAYS);
                        break;
                    case DF_Less:
                        gglDepthFunc(GL_LESS);
                        break;
                    case DF_LEqual:
                        gglDepthFunc(GL_LEQUAL);
                        break;
                    case DF_Equal:
                        gglDepthFunc(GL_EQUAL);
                        break;
                    case DF_NotEqual:
                        gglDepthFunc(GL_EQUAL);
                        break;
                    case DF_GEqual:
                        gglDepthFunc(GL_GEQUAL);
                        break;
                    case DF_Greater:
                        gglDepthFunc(GL_GREATER);
                        break;
                    case DF_Never:
                        gglDepthFunc(GL_NEVER);
                        break;
                    default:
                        BE_FATALERROR(L"OpenGLRHI::SetStateBits: invalid depth func state bits");
                        break;
                }
                
                gglEnable(GL_DEPTH_TEST);
            } else {
                gglDisable(GL_DEPTH_TEST);
            }
        }
        
        // blend func
        if (state_delta & MaskBF) {
            if (stateBits & MaskBF) {
                bits = (stateBits & MaskBS);
                switch (bits) {
                    case BS_Zero:
                        blend_src = GL_ZERO;
                        break;
                    case BS_One:
                        blend_src = GL_ONE;
                        break;
                    case BS_DstColor:
                        blend_src = GL_DST_COLOR;
                        break;
                    case BS_OneMinusDstColor:
                        blend_src = GL_ONE_MINUS_DST_COLOR;
                        break;
                    case BS_SrcAlpha:
                        blend_src = GL_SRC_ALPHA;
                        break;
                    case BS_OneMinusSrcAlpha:
                        blend_src = GL_ONE_MINUS_SRC_ALPHA;
                        break;
                    case BS_DstAlpha:
                        blend_src = GL_DST_ALPHA;
                        break;
                    case BS_OneMinusDstAlpha:
                        blend_src = GL_ONE_MINUS_DST_ALPHA;
                        break;
                    case BS_SrcAlphaSaturate:
                        blend_src = GL_SRC_ALPHA_SATURATE;
                        break;
                    default:
                        blend_src = GL_ONE;
                        BE_FATALERROR(L"OpenGLRHI::SetStateBits: invalid src blend state bits");
                        break;
                }
                
                bits = (stateBits & MaskBD);
                switch (bits) {
                    case BD_Zero:
                        blend_dst = GL_ZERO;
                        break;
                    case BD_One:
                        blend_dst = GL_ONE;
                        break;
                    case BD_SrcColor:
                        blend_dst = GL_SRC_COLOR;
                        break;
                    case BD_OneMinusSrcColor:
                        blend_dst = GL_ONE_MINUS_SRC_COLOR;
                        break;
                    case BD_SrcAlpha:
                        blend_dst = GL_SRC_ALPHA;
                        break;
                    case BD_OneMinusSrcAlpha:
                        blend_dst = GL_ONE_MINUS_SRC_ALPHA;
                        break;
                    case BD_DstAlpha:
                        blend_dst = GL_DST_ALPHA;
                        break;
                    case BD_OneMinusDstAlpha:
                        blend_dst = GL_ONE_MINUS_DST_ALPHA;
                        break;
                    default:
                        blend_dst = GL_ONE;
                        BE_FATALERROR(L"OpenGLRHI::SetStateBits: invalid dst blend state bits");
                        break;
                }
                
                gglEnable(GL_BLEND);
                gglBlendFunc(blend_src, blend_dst);
                //glBlendFuncSeparateEXT(blend_src, blend_dst, blend_src, blend_dst);
            } else {
                gglDisable(GL_BLEND);
            }
        }
        
        // color mask
        if (state_delta & (ColorWrite | AlphaWrite)) {
            redWrite    = stateBits & RedWrite ? true : false;
            greenWrite  = stateBits & GreenWrite ? true : false;
            blueWrite   = stateBits & BlueWrite ? true : false;
            alphaWrite  = stateBits & AlphaWrite ? true : false;
            gglColorMask(redWrite, greenWrite, blueWrite, alphaWrite);
        }
        
        // depth mask
        if (state_delta & DepthWrite) {
            gglDepthMask((stateBits & DepthWrite) ? GL_TRUE : GL_FALSE);
        }
        
        currentContext->state->renderState = stateBits;
    }
}

void OpenGLRHI::SetCullFace(int cull) {
    if (cull != currentContext->state->cull) {
        currentContext->state->cull = cull;
        if (cull == NoCull) {
            gglDisable(GL_CULL_FACE);
        } else {
            gglEnable(GL_CULL_FACE);
            if (cull == BackCull) {
                gglCullFace(GL_BACK);
            } else {
                gglCullFace(GL_FRONT);
            }
        }
    }
}

void OpenGLRHI::SetDepthBias(float slopeScaleBias, float constantBias) {
    if (slopeScaleBias != 0.0f || constantBias != 0.0f) {
        if (OpenGL::SupportsPolygonMode()) {
            gglEnable(GL_POLYGON_OFFSET_POINT);
            gglEnable(GL_POLYGON_OFFSET_LINE);
        }
        gglEnable(GL_POLYGON_OFFSET_FILL);
        gglPolygonOffset(slopeScaleBias, constantBias);
    } else {
        if (OpenGL::SupportsPolygonMode()) {
            gglDisable(GL_POLYGON_OFFSET_POINT);
            gglDisable(GL_POLYGON_OFFSET_LINE);
        }
        gglDisable(GL_POLYGON_OFFSET_FILL);
    }
}

void OpenGLRHI::SetDepthRange(float znear, float zfar) {
    OpenGL::DepthRange(znear, zfar);
}

void OpenGLRHI::SetDepthClamp(bool enable) {
    if (OpenGL::SupportsDepthClamp()) {
        if (enable) {
            gglEnable(GL_DEPTH_CLAMP);
        } else {
            gglDisable(GL_DEPTH_CLAMP);
        }
    }
}

void OpenGLRHI::SetDepthBounds(float zmin, float zmax) {
#ifdef GL_EXT_depth_bounds_test
    if (OpenGL::SupportsDepthBoundsTest()) {
        if (zmin > 0.0f || zmax < 1.0f) {
            gglEnable(GL_DEPTH_BOUNDS_TEST_EXT);
            gglDepthBoundsEXT(zmin, zmax);
        } else {
            gglDisable(GL_DEPTH_BOUNDS_TEST_EXT);
        }
    }
#endif
}

void OpenGLRHI::SetViewport(const Rect &viewportRect) {
    currentContext->state->viewportRect = viewportRect;
    gglViewport(viewportRect.x, viewportRect.y, viewportRect.w, viewportRect.h);
}

// scissorRect 의 x, y 는 lower left corner
void OpenGLRHI::SetScissor(const Rect &scissorRect) {
    if (!scissorRect.IsEmpty()) {
        currentContext->state->scissorRect = scissorRect;
        gglScissor(scissorRect.x, scissorRect.y, scissorRect.w, scissorRect.h);
        gglEnable(GL_SCISSOR_TEST);
    } else {
        currentContext->state->scissorRect.Set(0, 0, 0, 0);
        gglDisable(GL_SCISSOR_TEST);
    }
}

void OpenGLRHI::SetSRGBWrite(bool enable) {
    if (OpenGL::SupportsFrameBufferSRGB()) {
        // This extension adds a framebuffer capability for sRGB framebuffer update and blending. When
        // blending is disabled but the new sRGB updated mode is enabled (assume the framebuffer supports
        // the capability), high-precision linear color component values for red, green, and blue generated
        // by fragment coloring are encoded for sRGB prior to being written into the framebuffer.  When
        // blending is enabled along with the new sRGB update mode, red, green, and  blue framebuffer color
        // components are treated as sRGB values that are converted to linear color values, blended with
        // the high-precision color values generated by fragment coloring, and then the blend result is
        // encoded for sRGB just prior to being written into the framebuffer.
        if (enable) {
            gglEnable(GL_FRAMEBUFFER_SRGB);
        } else {
            gglDisable(GL_FRAMEBUFFER_SRGB);
        }
    }
};

void OpenGLRHI::EnableLineSmooth(bool enable) {
    if (OpenGL::SupportsLineSmooth()) {
        if (enable) {
            gglEnable(GL_LINE_SMOOTH);
        } else {
            gglDisable(GL_LINE_SMOOTH);
        }
    }
}

float OpenGLRHI::GetLineWidth() const {
    float width;
    gglGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, &width);
    return width;
}

void OpenGLRHI::SetLineWidth(float width) {
    //gglLineWidth(Max(width, 0.0f));
}

RHI::Handle OpenGLRHI::CreateStencilState(int readMask, int writeMask, StencilFunc funcBack, int failBack, int zfailBack, int zpassBack, StencilFunc funcFront, int failFront, int zfailFront, int zpassFront) {
    GLStencilState *stencilState = new GLStencilState;
    stencilState->readMask  = readMask;
    stencilState->writeMask = writeMask;
    stencilState->func[0]   = toGLStencilFunc[funcBack];
    stencilState->func[1]   = toGLStencilFunc[funcFront];
    stencilState->fail[0]   = toGLStencilOp[failBack];
    stencilState->zfail[0]  = toGLStencilOp[zfailBack];
    stencilState->zpass[0]  = toGLStencilOp[zpassBack];
    stencilState->fail[1]   = toGLStencilOp[failFront];
    stencilState->zfail[1]  = toGLStencilOp[zfailFront];
    stencilState->zpass[1]  = toGLStencilOp[zpassFront];
    
    int handle = stencilStateList.FindNull();
    if (handle == -1) {
        handle = stencilStateList.Append(stencilState);
    } else {
        stencilStateList[handle] = stencilState;
    }
    
    return (Handle)handle;
}

void OpenGLRHI::DestroyStencilState(Handle stencilStateHandle) {
    delete stencilStateList[stencilStateHandle];
    stencilStateList[stencilStateHandle] = nullptr;
}

void OpenGLRHI::SetStencilState(Handle stencilStateHandle, int ref) {
    if (currentContext->state->stencilStateHandle == stencilStateHandle) {
        return;
    }
    
    currentContext->state->stencilStateHandle = stencilStateHandle;
    
    if (stencilStateHandle == NullStencilState) {
        gglDisable(GL_STENCIL_TEST);
        gglStencilMask(0);
    } else {
        const GLStencilState *stencilState = stencilStateList[stencilStateHandle];
        
        gglEnable(GL_STENCIL_TEST);
        gglStencilMask(stencilState->writeMask);
        
        // back 과 front 의 stencilState 가 다르면 양면 스텐실 적용
        gglStencilOpSeparate(GL_BACK, stencilState->fail[0], stencilState->zfail[0], stencilState->zpass[0]);
        gglStencilOpSeparate(GL_FRONT, stencilState->fail[1], stencilState->zfail[1], stencilState->zpass[1]);
        gglStencilFuncSeparate(stencilState->func[1], stencilState->func[0], ref, stencilState->readMask);
    }
}

unsigned int OpenGLRHI::GetStateBits() const { 
    return currentContext->state->renderState; 
}

const Rect &OpenGLRHI::GetViewport() const { 
    return currentContext->state->viewportRect; 
}

int OpenGLRHI::GetCullFace() const { 
    return currentContext->state->cull; 
}

const Rect &OpenGLRHI::GetScissor() const { 
    return currentContext->state->scissorRect; 
}

BE_NAMESPACE_END
