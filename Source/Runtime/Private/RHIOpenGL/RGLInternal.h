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

#include "imgui/imgui.h"

#include "Core/CVars.h"

struct ImGuiContext;

BE_NAMESPACE_BEGIN

#define BUFFER_OFFSET(i)    ((byte *)nullptr + (i))

struct GLVertexAttrib {
    RHI::Handle             vertexBufferHandle;
    GLint                   components;
    GLenum                  type;
    GLboolean               normalize;
    GLsizei                 stride;
    const void *            ptr;
    GLuint                  divisor;

    GLVertexAttrib() : vertexBufferHandle(RHI::NullBuffer), components(0), type(0), normalize(false), stride(-1), ptr(nullptr), divisor(-1) {}
};

struct GLState {
    int                     tmu; // current texture map unit
    RHI::Handle             textureHandles[RHI::MaxTMU];
    RHI::Handle             shaderHandle;
    RHI::Handle             bufferHandles[RHI::BufferType::Count];
    RHI::Handle             indexedBufferHandles[2]; // 0: UniformBuffer, 1: TransformFeedbackBuffer
    RHI::Handle             vertexFormatHandle;
    GLVertexAttrib          vertexAttribs[RHI::VertexElement::Usage::Count];
    RHI::Handle             renderTargetHandle;
    RHI::Handle             renderTargetHandleStack[16];
    int                     renderTargetHandleStackDepth;
    RHI::Handle             stencilStateHandle;

    unsigned int            renderState;
    int                     cull;
    Rect                    viewportRect;
    Rect                    scissorRect;
    int                     oldUnpackAlignment;
    int                     newUnpackAlignment;
    bool                    sRGBWriteEnabled;

    GLState() : tmu(0), 
        shaderHandle(RHI::NullShader), vertexFormatHandle(RHI::NullVertexFormat), renderTargetHandle(RHI::NullRenderTarget), renderTargetHandleStackDepth(0), stencilStateHandle(RHI::NullStencilState), 
        renderState(0), cull(RHI::CullType::Back), viewportRect(Rect::zero), scissorRect(Rect::zero), oldUnpackAlignment(0), newUnpackAlignment(0), sRGBWriteEnabled(true) {
        memset(textureHandles, 0, sizeof(textureHandles));
        memset(bufferHandles, 0, sizeof(textureHandles));
        memset(indexedBufferHandles, 0, sizeof(indexedBufferHandles));
    }
};

struct GLContext {
    RHI::Handle             handle;
    RHI::DisplayContextFunc displayFunc;
    void *                  displayFuncDataPtr;
    bool                    onDemandDrawing;
#if defined(__WIN32__)
    HWND                    hwnd;
    WNDPROC                 oldWndProc;
    HDC                     hdc;
    HGLRC                   hrc;
    #ifdef USE_DESKTOP_EGL
        EGLDisplay          eglDisplay;
        EGLConfig           eglConfig;
        EGLContext          eglContext;
        EGLSurface          eglSurface;
    #endif
#elif defined(__MACOSX__)
    CGDirectDisplayID       display;
    NSView *                contentView;
    GLView *                glView;
    //NSString *            windowTitle;
    //CGSize                windowSize;
    NSOpenGLContext *       nsglContext;
    CGLContextObj           cglContext;
#elif defined(__IOS__) || defined(__IOS_SIMULATOR__)
    UIView *                rootView;
    EAGLView *              eaglView;
    EAGLContext *           eaglContext;
#elif defined(__ANDROID__)
    ANativeWindow *         nativeWindow;
    EGLDisplay              eglDisplay;
    EGLConfig               eglConfig;
    EGLContext              eglContext;
    EGLSurface              eglSurface;
#endif
    GLState *               state;
    GLuint                  defaultFramebuffer;
    GLuint                  defaultVAO;
    ImGuiContext *          imGuiContext;
    double                  imGuiLastTime;
};

struct GLStencilState {
    GLenum                  func[2];
    int                     writeMask;
    int                     readMask;
    GLenum                  fail[2];
    GLenum                  zfail[2];
    GLenum                  zpass[2];
};

struct GLTexture {
    RHI::TextureType::Enum  type;
    GLenum                  target;
    GLuint                  object;
};

struct GLBuffer {
    RHI::BufferType::Enum   type;
    GLenum                  target;
    GLenum                  usage;
    GLuint                  object;
    int                     size;
    int                     pitch;
    int                     writeOffset;
    int                     bindingIndex;
    int                     bindingOffset;
    int                     bindingSize;
};

struct GLSync {
    GLsync                  sync;
};

struct GLUniformTexture {
    bool                    operator==(const GLUniformTexture &other) const { return Str::Cmp(name, other.name) == 0; }
    bool                    operator<(const GLUniformTexture &other) const { return Str::Cmp(name, other.name) < 0; }
    bool                    operator>(const GLUniformTexture &other) const { return Str::Cmp(name, other.name) > 0; }
    
    char *                  name;
    unsigned int            unit;   // texture unit
};

struct GLUniformConstant {
    bool                    operator==(const GLUniformConstant &other) const { return Str::Cmp(name, other.name) == 0; }
    bool                    operator<(const GLUniformConstant &other) const { return Str::Cmp(name, other.name) < 0; }
    bool                    operator>(const GLUniformConstant &other) const { return Str::Cmp(name, other.name) > 0; }

    char *                  name;
    GLint                   location;
    GLenum                  type;
    int                     count;
};

struct GLUniformBlock {
    bool                    operator==(const GLUniformBlock &other) const { return Str::Cmp(name, other.name) == 0; }
    bool                    operator<(const GLUniformBlock &other) const { return Str::Cmp(name, other.name) < 0; }
    bool                    operator>(const GLUniformBlock &other) const { return Str::Cmp(name, other.name) > 0; }

    char *                  name;
    GLuint                  index;
    GLint                   size;
    int                     numUniforms;
    bool                    referencedByVS;
    bool                    referencedByFS;
};

struct GLShader {
    char                    name[64];
    GLuint                  programObject;
    int                     numUniformTextures;
    GLUniformTexture *      uniformTextures;
    int                     numUniformConstants;
    GLUniformConstant *     uniformConstants;
    int                     numUniformBlocks;
    GLUniformBlock *        uniformBlocks;
    static Str              programCacheDir;
};

struct GLVertexElementInternal {
    int                     stream;
    int                     offset;
    int                     components;
    GLenum                  type;
    GLboolean               normalize;
    bool                    useFloatAttribute;
    int                     divisor;
};

struct GLVertexFormat {
    int                     vertexSize[RHI::MaxVertexStream];
    GLVertexElementInternal vertexElements[RHI::VertexElement::Usage::Count];
};

struct GLRenderTarget {
    int                     type;
    int                     flags;
    int                     numColorTextures;
    RHI::Handle             colorTextureHandles[16];
    RHI::Handle             depthTextureHandle;
    GLuint                  fbo;
    GLuint                  depthRenderBuffer;      // depth render buffer or depth/stencil render buffer
    GLuint                  stencilRenderBuffer;    // just stencil render buffer
};

struct GLQuery {
    RHI::QueryType::Enum    queryType;
    GLuint                  id;
};

// ImGui OpenGL implementation
bool ImGui_ImplOpenGL_Init(const char *glsl_version);
void ImGui_ImplOpenGL_Shutdown();
void ImGui_ImplOpenGL_ValidateFrame();
void ImGui_ImplOpenGL_RenderDrawData(ImDrawData *draw_data);

BE_NAMESPACE_END
