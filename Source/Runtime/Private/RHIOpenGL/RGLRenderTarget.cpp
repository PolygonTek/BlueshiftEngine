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

static bool CheckFBOStatus() {
    GLenum status = gglCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status == GL_FRAMEBUFFER_COMPLETE) {
        return true;
    }
    
    switch (status) {
    case GL_FRAMEBUFFER_UNSUPPORTED:
        BE_WARNLOG(L"FBO format unsupported\n");
        return false;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        BE_WARNLOG(L"FBO missing an image attachment\n");
        return false;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        BE_WARNLOG(L"FBO has one or several incomplete image attachments\n");
        return false;
#ifdef GL_EXT_framebuffer_object
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
        BE_WARNLOG(L"FBO has one or several image attachments with different dimensions\n");
        return false;
    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
        BE_WARNLOG(L"FBO has one or several image attachments with different internal formats\n");
        return false;
#endif
#ifdef GL_VERSION_3_0
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        BE_WARNLOG(L"FBO has invalid draw buffer\n");
        return false;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        BE_WARNLOG(L"FBO has invalid read buffer\n");
        return false;
#endif
#ifdef GL_VERSION_3_2
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
        BE_WARNLOG(L"FBO missing layer target\n");
        return false;
#endif
#ifdef GL_EXT_geometry_shader4
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT_EXT:
        BE_WARNLOG(L"FBO incomplete layer count\n");
        return false;
#endif
    }

    BE_WARNLOG(L"FBO unknown status %i\n", status);
    return false;
}

RHI::Handle OpenGLRHI::CreateRenderTarget(RenderTargetType type, int width, int height, int numColorTextures, Handle *colorTextureHandles, Handle depthTextureHandle, bool sRGB, int flags) {
    GLuint fbo;
    GLuint colorRenderBuffer = 0;
    GLuint depthRenderBuffer = 0;
    GLuint stencilRenderBuffer = 0;

    GLint oldFBO;
    gglGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFBO);

    gglGenFramebuffers(1, &fbo);
    gglBindFramebuffer(GL_FRAMEBUFFER, fbo);

    GLenum target = 0;
    switch (type) {
    case RenderTarget2D:
        target = GL_TEXTURE_2D;
        break;
    case RenderTargetCubeMap:
        target = GL_TEXTURE_CUBE_MAP;
        break;
    case RenderTarget2DArray:
        target = GL_TEXTURE_2D_ARRAY;
        break;
    default:
        assert(0);
        break;
    }

    if (numColorTextures > 0) {
        for (int i = 0; i < numColorTextures; i++) {
            GLuint textureObject = textureList[colorTextureHandles[i]]->object;
            
            switch (target) {
            case GL_TEXTURE_2D:
                gglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, target, textureObject, 0);
                break;
            case GL_TEXTURE_3D:
            case GL_TEXTURE_2D_ARRAY:
                gglFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, textureObject, 0, 0);
                break;
            case GL_TEXTURE_CUBE_MAP:
            case GL_TEXTURE_CUBE_MAP_ARRAY:
#ifdef GL_VERSION_3_2
                gglFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, textureObject, 0);
#else
                gglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, target, textureObject, 0);
#endif
                break;
            default:
                assert(0);
                break;
            }
        }

        GLenum color0 = GL_COLOR_ATTACHMENT0;
        gglDrawBuffers(1, &color0);
        gglReadBuffer(GL_COLOR_ATTACHMENT0);
    } else if (flags & HasColorBuffer) {
        gglGenRenderbuffers(1, &colorRenderBuffer);
        gglBindRenderbuffer(GL_RENDERBUFFER, colorRenderBuffer);
        gglRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);
        gglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderBuffer);
    }

    if (depthTextureHandle != NullTexture) {
        GLuint textureObject = textureList[depthTextureHandle]->object;
        
        switch (target) {
        case GL_TEXTURE_2D:
            gglFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, textureObject, 0);
            break;
        case GL_TEXTURE_3D:
        case GL_TEXTURE_2D_ARRAY:
            gglFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, textureObject, 0, 0);
            break;
        case GL_TEXTURE_CUBE_MAP:
        case GL_TEXTURE_CUBE_MAP_ARRAY:
#ifdef GL_VERSION_3_2
            gglFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, textureObject, 0);
#else
            gglFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, textureObject, 0);
#endif
            break;
        default:
            assert(0);
            break;
        }

        if (numColorTextures == 0) {
            // NOTE: this is per-FBO state
            OpenGL::DrawBuffer(GL_NONE);
            gglReadBuffer(GL_NONE);
        }
    } else if (flags & HasDepthBuffer) {
        if (flags & HasStencilBuffer) {
            gglGenRenderbuffers(1, &depthRenderBuffer);
            gglBindRenderbuffer(GL_RENDERBUFFER, depthRenderBuffer);
            gglRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
            gglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderBuffer);
            gglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRenderBuffer);
        } else {
            gglGenRenderbuffers(1, &depthRenderBuffer);
            gglBindRenderbuffer(GL_RENDERBUFFER, depthRenderBuffer);
            gglRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
            gglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderBuffer);
        }
    }

    if (!CheckFBOStatus()) {
        if (depthRenderBuffer) {
            gglDeleteRenderbuffers(1, &depthRenderBuffer);
        }

        if (stencilRenderBuffer) {
            gglDeleteRenderbuffers(1, &stencilRenderBuffer);
        }

        if (fbo) {
            gglDeleteFramebuffers(1, &fbo);
        }

        return NullRenderTarget;
    }
    
//    OpenGL::BindDefaultFBO();
    gglBindFramebuffer(GL_FRAMEBUFFER, oldFBO);

    GLRenderTarget *renderTarget = new GLRenderTarget;
    renderTarget->type = type;
    renderTarget->flags = flags;
    renderTarget->numColorTextures = numColorTextures;
    for (int i = 0; i < numColorTextures; i++) {
        renderTarget->colorTextureHandles[i] = colorTextureHandles[i];
    }
    renderTarget->depthTextureHandle = depthTextureHandle;
    renderTarget->sRGB = sRGB;
    renderTarget->fbo = fbo;
    renderTarget->depthRenderBuffer = depthRenderBuffer;
    renderTarget->stencilRenderBuffer = stencilRenderBuffer;

    int handle = renderTargetList.FindNull();
    if (handle == -1) {
        handle = renderTargetList.Append(renderTarget);
    } else {
        renderTargetList[handle] = renderTarget;
    }

    return (Handle)handle;
}

void OpenGLRHI::DestroyRenderTarget(Handle renderTargetHandle) {
    if (renderTargetHandle == NullRenderTarget) {
        BE_WARNLOG(L"OpenGLRHI::DestroyRenderTarget: invalid render target\n");
        return;
    }
    
    if (currentContext->state->renderTargetHandleStackDepth > 0 && 
        currentContext->state->renderTargetHandleStack[currentContext->state->renderTargetHandleStackDepth - 1] == renderTargetHandle) {
        BE_WARNLOG(L"OpenGLRHI::DestroyRenderTarget: render target is using\n");
        return;
    }

    GLRenderTarget *renderTarget = renderTargetList[renderTargetHandle];
    if (renderTarget->depthRenderBuffer) {
        gglDeleteRenderbuffers(1, &renderTarget->depthRenderBuffer);
    }
    if (!renderTarget->stencilRenderBuffer) {
        gglDeleteRenderbuffers(1, &renderTarget->stencilRenderBuffer);
    }
    if (renderTarget->fbo) {
        gglDeleteFramebuffers(1, &renderTarget->fbo);
    }

    delete renderTarget;
    renderTargetList[renderTargetHandle] = nullptr;
}

void OpenGLRHI::BeginRenderTarget(Handle renderTargetHandle, int level, int sliceIndex, unsigned int mrtBitMask) {
    if (currentContext->state->renderTargetHandleStackDepth > 0 && currentContext->state->renderTargetHandleStack[currentContext->state->renderTargetHandleStackDepth - 1] == renderTargetHandle) {
        BE_WARNLOG(L"OpenGLRHI::BeginRenderTarget: same render target\n");
    }

    const GLRenderTarget *renderTarget = renderTargetList[renderTargetHandle];
    currentContext->state->renderTargetHandleStack[currentContext->state->renderTargetHandleStackDepth++] = currentContext->state->renderTargetHandle;
    currentContext->state->renderTargetHandle = renderTargetHandle;

    gglBindFramebuffer(GL_FRAMEBUFFER, renderTarget->fbo);

    if (renderTarget->type == RenderTarget2DArray) {
        if (renderTarget->numColorTextures > 0) {
            for (int i = 0; i < renderTarget->numColorTextures; i++) {
                GLuint textureObject = textureList[renderTarget->colorTextureHandles[i]]->object;
                gglFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, textureObject, level, sliceIndex);
            }
        }

        if (renderTarget->depthTextureHandle != NullTexture) {
            GLuint textureObject = textureList[renderTarget->depthTextureHandle]->object;
            gglFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, textureObject, level, sliceIndex);
        }
    } else {
        GLenum target = renderTarget->type == RenderTargetCubeMap ? (GL_TEXTURE_CUBE_MAP_POSITIVE_X + sliceIndex) : GL_TEXTURE_2D;

        if (renderTarget->numColorTextures > 0) {
            for (int i = 0; i < renderTarget->numColorTextures; i++) {
                GLuint textureObject = textureList[renderTarget->colorTextureHandles[i]]->object;
                gglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, target, textureObject, level);
            }
        }

        if (renderTarget->depthTextureHandle != NullTexture) {
            GLuint textureObject = textureList[renderTarget->depthTextureHandle]->object;
            gglFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, textureObject, level);
        }
    } 

    if (renderTarget->numColorTextures > 1 && mrtBitMask != 0) {
        GLenum drawBuffers[16];
        int numDrawBuffers = 0;

        for (int i = 0; i < renderTarget->numColorTextures; i++) {
            if (mrtBitMask & 1) {
                drawBuffers[numDrawBuffers] = GL_COLOR_ATTACHMENT0 + numDrawBuffers;
                numDrawBuffers++;
            }
            mrtBitMask >>= 1;
        }

        gglDrawBuffers(numDrawBuffers, drawBuffers);
    }

    if (gl_sRGB.GetBool()) {
        if (renderTarget->sRGB) {
            SetSRGBWrite(true);
        } else {
            SetSRGBWrite(false);
        }
    }
}

void OpenGLRHI::EndRenderTarget() {
    if (currentContext->state->renderTargetHandleStackDepth == 0) {
        BE_WARNLOG(L"unmatched BeginRenderTarget() / EndRenderTarget()\n");
        return;
    }
    
    Handle oldRenderTargetHandle = currentContext->state->renderTargetHandleStack[--currentContext->state->renderTargetHandleStackDepth];
    const GLRenderTarget *oldRenderTarget = renderTargetList[oldRenderTargetHandle];

    currentContext->state->renderTargetHandle = oldRenderTargetHandle;

    gglBindFramebuffer(GL_FRAMEBUFFER, oldRenderTarget->fbo);

    if (gl_sRGB.GetBool()) {
        if (oldRenderTarget->sRGB) {
            SetSRGBWrite(true);
        } else {
            SetSRGBWrite(false);
        }
    }
}

void OpenGLRHI::BlitRenderTarget(Handle srcRenderTargetHandle, const Rect &srcRect, Handle dstRenderTargetHandle, const Rect &dstRect, int mask, int filter) const {
    const GLRenderTarget *srcRenderTarget = renderTargetList[srcRenderTargetHandle];
    const GLRenderTarget *dstRenderTarget = renderTargetList[dstRenderTargetHandle];

    GLbitfield glmask = 0;
    if (mask & ColorBlitMask) {
        glmask |= GL_COLOR_BUFFER_BIT;
    }

    if (mask & DepthBlitMask) {
        glmask |= GL_DEPTH_BUFFER_BIT;
    }

    assert(glmask);
    if (!glmask) {
        BE_WARNLOG(L"OpenGLRHI::BlitRenderTarget: NULL mask\n");
        return;
    }

    gglBindFramebuffer(GL_READ_FRAMEBUFFER, srcRenderTarget->fbo);
    gglBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstRenderTarget->fbo);

    gglBlitFramebuffer(
        srcRect.x, srcRect.y, srcRect.X2(), srcRect.Y2(),
        dstRect.x, dstRect.y, dstRect.X2(), dstRect.Y2(),
        glmask, filter == NearestBlitFilter ? GL_NEAREST : GL_LINEAR);

    gglBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    gglBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

BE_NAMESPACE_END
