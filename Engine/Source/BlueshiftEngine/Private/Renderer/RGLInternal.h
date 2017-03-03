#pragma once

#ifdef __WIN32__

//#define USE_EGLWIN

#ifdef USE_EGLWIN
#include "OpenGL/OpenGLES3.h"
#include "OpenGL/EGL/egl.h"
#include "OpenGL/EGL/eglplatform.h"
#else
#include "OpenGL/WinOpenGL.h"
#endif

#endif // __WIN32__

#if defined(__XAMARIN__) && !defined(__WIN32__)

#include "OpenGL/XamarinOpenGL.h"
#include "Renderer/EGLUtil.h"

#else // __XAMARIN__

#ifdef __MACOSX__

#include <OpenGL/OpenGL.h>
#include <CoreGraphics/CoreGraphics.h>
#include "OpenGL/MacOSOpenGL.h"

OBJC_CLASS(NSView);
OBJC_CLASS(NSOpenGLContext);
OBJC_CLASS(GLView);

#endif // __MACOSX__

#ifdef __IOS__

#include <CoreGraphics/CoreGraphics.h>
#include "OpenGL/IOSOpenGL.h"

OBJC_CLASS(EAGLContext);
OBJC_CLASS(UIView);
OBJC_CLASS(EAGLView);

#endif // __IOS__

#ifdef __ANDROID__

#include "OpenGL/AndroidOpenGL.h"
#include <nvidia/nv_egl_util/nv_egl_util.h>

#endif // __ANDROID__

#endif // __XAMARIN__

#include "Core/CVars.h"

BE_NAMESPACE_BEGIN

#define BUFFER_OFFSET(i)            ((byte *)nullptr + (i))

struct GLState {
    int                 tmu; // current texture map unit
    Renderer::Handle    textureHandles[Renderer::MaxTMU];
    Renderer::Handle    shaderHandle;
    Renderer::Handle    bufferHandles[Renderer::MaxBufferTypes];
    Renderer::Handle    vertexFormatHandle;
    Renderer::Handle    renderTargetHandle;
    Renderer::Handle    renderTargetHandleStack[16];
    int                 renderTargetHandleStackDepth;
    Renderer::Handle    stencilStateHandle;

    unsigned int        renderState;
    float               alphaRef;
    bool                alphaRefChanged;
    int                 cull;
    Rect                viewportRect;
    Rect                scissorRect;
    int                 oldUnpackAlignment;
    int                 newUnpackAlignment;
};

struct GLContext {
    Renderer::Handle    handle;
    Renderer::DisplayContextFunc displayFunc;
    void *              displayFuncDataPtr;
    bool                onDemandDrawing;
#if defined __XAMARIN__ && !defined __WIN32__
    EGLUtil *           rootView;
#elif defined(__WIN32__)
    HWND                hwnd;
    WNDPROC             oldWndProc;
    HDC                 hdc;
#	ifndef USE_EGLWIN
    HGLRC               hrc;
#	else
    EGLDisplay          eglDisplay;
    EGLSurface          eglSurface;
    EGLContext          eglContext;
#	endif
#elif defined(__MACOSX__)
    CGDirectDisplayID   display;
    NSView *            contentView;
    GLView *            glView;
    //NSString *        windowTitle;
    //CGSize            windowSize;
    NSOpenGLContext *   nsglContext;
    CGLContextObj       cglContext;
#elif defined(__IOS__) || defined(__IOS_SIMULATOR__)
    UIView *            rootView;
    EAGLView *          eaglView;
    EAGLContext *       eaglContext;
#elif defined(__ANDROID__)
    NvEGLUtil *         rootView;
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

struct GLShader {
    char                name[64];
    GLuint              programObject;
    int                 numSamplers;
    GLSampler *         samplers;
    int                 numUniforms;
    GLUniform *         uniforms;
    static Str          programCacheDir;
};

struct GLVertexElementInternal {
    int                 stream;
    int                 offset;
    int                 components;
    GLenum              type;
    GLboolean           normalize;
};

struct GLVertexFormat {
    int                 vertexSize[Renderer::MaxVertexStream];
    GLVertexElementInternal vertexElements[Renderer::VertexElement::MaxUsages];
};

struct GLRenderTarget {
    int                 type;
    int                 flags;
    int                 numColorTextures;
    Renderer::Handle    colorTextureHandles[16];
    Renderer::Handle    depthTextureHandle;
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
