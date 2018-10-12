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

#pragma once

#ifdef __WIN32__

#include "OpenGL/WinOpenGL.h"

#elif defined(__XAMARIN__)

#include "OpenGL/XamarinOpenGL.h"
#include "RHI/EGLUtil.h"

#elif defined(__MACOSX__)

#include <OpenGL/OpenGL.h>
#include <CoreGraphics/CoreGraphics.h>
#include "OpenGL/MacOSOpenGL.h"

OBJC_CLASS(NSView);
OBJC_CLASS(NSOpenGLContext);
OBJC_CLASS(GLView);

#elif defined(__IOS__)

#include <CoreGraphics/CoreGraphics.h>
#include "OpenGL/IOSOpenGL.h"

OBJC_CLASS(EAGLContext);
OBJC_CLASS(UIView);
OBJC_CLASS(EAGLView);

#elif defined(__ANDROID__)

#include "OpenGL/AndroidOpenGL.h"

#endif

#include "Core/CVars.h"

BE_NAMESPACE_BEGIN

#define BUFFER_OFFSET(i)    ((byte *)nullptr + (i))

struct GLVertexAttrib {
    RHI::Handle         vertexBufferHandle;
    GLint               components;
    GLenum              type;
    GLboolean           normalize;
    GLsizei             stride;
    const void *        ptr;
    GLuint              divisor;

    GLVertexAttrib() : vertexBufferHandle(RHI::NullBuffer), components(0), type(0), normalize(false), stride(-1), ptr(nullptr), divisor(-1) {}
};

struct GLState {
    int                 tmu; // current texture map unit
    RHI::Handle         textureHandles[RHI::MaxTMU];
    RHI::Handle         shaderHandle;
    RHI::Handle         bufferHandles[RHI::MaxBufferTypes];
    RHI::Handle         indexedBufferHandles[2]; // 0: UniformBuffer, 1: TransformFeedbackBuffer
    RHI::Handle         vertexFormatHandle;
    GLVertexAttrib      vertexAttribs[RHI::VertexElement::MaxUsages];
    RHI::Handle         renderTargetHandle;
    RHI::Handle         renderTargetHandleStack[16];
    int                 renderTargetHandleStackDepth;
    RHI::Handle         stencilStateHandle;

    unsigned int        renderState;
    int                 cull;
    Rect                viewportRect;
    Rect                scissorRect;
    int                 oldUnpackAlignment;
    int                 newUnpackAlignment;

    GLState() : tmu(0), 
        shaderHandle(RHI::NullShader), vertexFormatHandle(RHI::NullVertexFormat), renderTargetHandle(RHI::NullRenderTarget), renderTargetHandleStackDepth(0), stencilStateHandle(RHI::NullStencilState), 
        renderState(0), cull(RHI::BackCull), viewportRect(Rect::empty), scissorRect(Rect::empty), oldUnpackAlignment(0), newUnpackAlignment(0) {
        memset(textureHandles, 0, sizeof(textureHandles));
        memset(bufferHandles, 0, sizeof(textureHandles));
        memset(indexedBufferHandles, 0, sizeof(indexedBufferHandles));
    }
};

struct GLContext {
    RHI::Handle         handle;
    RHI::DisplayContextFunc displayFunc;
    void *              displayFuncDataPtr;
    bool                onDemandDrawing;
#if defined(__IOS__) || defined(__IOS_SIMULATOR__)
    UIView *            rootView;
    EAGLView *          eaglView;
    EAGLContext *       eaglContext;
#elif defined(__ANDROID__)
    ANativeWindow *     nativeWindow;
    EGLDisplay          eglDisplay;
    EGLConfig           eglConfig;
    EGLContext          eglContext;
    EGLSurface          eglSurface;
#elif defined(__WIN32__)
    HWND                hwnd;
    WNDPROC             oldWndProc;
    HDC                 hdc;
    HGLRC               hrc;
#ifdef USE_DESKTOP_EGL
    EGLDisplay          eglDisplay;
    EGLConfig           eglConfig;
    EGLSurface          eglSurface;
    EGLContext          eglContext;
#endif
#elif defined(__XAMARIN__)
    EGLUtil *           rootView;
#elif defined(__MACOSX__)
    CGDirectDisplayID   display;
    NSView *            contentView;
    GLView *            glView;
    //NSString *        windowTitle;
    //CGSize            windowSize;
    NSOpenGLContext *   nsglContext;
    CGLContextObj       cglContext;
#endif
    GLState *           state;
    GLuint              defaultFramebuffer;
    GLuint              defaultVAO;
};

struct GLStencilState {
    GLenum              func[2];
    int                 writeMask;
    int                 readMask;
    GLenum              fail[2];
    GLenum              zfail[2];
    GLenum              zpass[2];
};

struct GLTexture {
    int                 type;
    GLenum              target;
    GLuint              object;
};

struct GLBuffer {
    int                 type;
    GLenum              target;
    GLenum              usage;
    GLuint              object;
    int                 size;
    int                 pitch;
    int                 writeOffset;
    int                 bindingIndex;
    int                 bindingOffset;
    int                 bindingSize;
};

struct GLSync {
    GLsync              sync;
};

struct GLSampler {
    bool                operator==(const GLSampler &other) const { return Str::Cmp(name, other.name) == 0; }
    bool                operator<(const GLSampler &other) const { return Str::Cmp(name, other.name) < 0; }
    bool                operator>(const GLSampler &other) const { return Str::Cmp(name, other.name) > 0; }
    
    char *              name;
    unsigned int        unit;
};

struct GLUniform {
    bool                operator==(const GLUniform &other) const { return Str::Cmp(name, other.name) == 0; }
    bool                operator<(const GLUniform &other) const { return Str::Cmp(name, other.name) < 0; }
    bool                operator>(const GLUniform &other) const { return Str::Cmp(name, other.name) > 0; }

    char *              name;
    GLint               location;
    GLenum              type;
    int                 count;
};

struct GLUniformBlock {
    bool                operator==(const GLUniformBlock &other) const { return Str::Cmp(name, other.name) == 0; }
    bool                operator<(const GLUniformBlock &other) const { return Str::Cmp(name, other.name) < 0; }
    bool                operator>(const GLUniformBlock &other) const { return Str::Cmp(name, other.name) > 0; }

    char *              name;
    GLuint              index;
    GLint               size;
    int                 numUniforms;
    bool                referencedByVS;
    bool                referencedByFS;
};

struct GLShader {
    char                name[64];
    GLuint              programObject;
    int                 numSamplers;
    GLSampler *         samplers;
    int                 numUniforms;
    GLUniform *         uniforms;
    int                 numUniformBlocks;
    GLUniformBlock *    uniformBlocks;
    static Str          programCacheDir;
};

struct GLVertexElementInternal {
    int                 stream;
    int                 offset;
    int                 components;
    GLenum              type;
    GLboolean           normalize;
    bool                shouldConvertToFloat;
    int                 divisor;
};

struct GLVertexFormat {
    int                 vertexSize[RHI::MaxVertexStream];
    GLVertexElementInternal vertexElements[RHI::VertexElement::MaxUsages];
};

struct GLRenderTarget {
    int                 type;
    int                 flags;
    int                 numColorTextures;
    RHI::Handle         colorTextureHandles[16];
    RHI::Handle         depthTextureHandle;
    bool                sRGB;
    GLuint              fbo;
    GLuint              depthRenderBuffer;      // depth render buffer or depth/stencil render buffer
    GLuint              stencilRenderBuffer;    // just stencil render buffer
};

struct GLQuery {
    GLuint              id;
};

extern CVar             gl_sRGB;

BE_NAMESPACE_END
