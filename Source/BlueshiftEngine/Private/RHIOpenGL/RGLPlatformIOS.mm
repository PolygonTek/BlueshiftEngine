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

#define USE_DISPLAY_LINK            1

const float userContentScaleFactor = 2.0f;

@interface EAGLView : UIView
@end

@implementation EAGLView {
    BE1::GLContext *    glContext;
    GLuint              framebuffer;
    GLuint              colorbuffer;
    GLuint              depthbuffer;
    GLint               backingWidth;
    GLint               backingHeight;
    CADisplayLink *     displayLink;
    BOOL                displayLinkStarted;
    int                 displayLinkFrameInterval;
}

+ (Class)layerClass {
    return [CAEAGLLayer class];
}

- (void)setGLContext:(BE1::GLContext *)ctx {
    glContext = ctx;
}

- (GLuint)framebuffer {
    return framebuffer;
}

- (id)initWithFrame:(CGRect)frameRect {
    self = [super initWithFrame:frameRect];
    
    // Configure EAGLDrawable (CAEAGLLayer)
    CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
    eaglLayer.opaque = YES;
    NSMutableDictionary *dict = [NSMutableDictionary dictionary];
    [dict setValue:[NSNumber numberWithBool:NO] forKey:kEAGLDrawablePropertyRetainedBacking];
    const NSString *colorFormat = BE1::gl_sRGB.GetBool() ? kEAGLColorFormatSRGBA8 : kEAGLColorFormatRGBA8;
    [dict setValue:colorFormat forKey:kEAGLDrawablePropertyColorFormat];
    eaglLayer.drawableProperties = dict;
    
    displayLinkStarted = NO;
    displayLinkFrameInterval = 1;
    
    return self;
}

- (void)dealloc {
    [self stopDisplayLink];
    
    [self freeFramebuffer];
    
#if !__has_feature(objc_arc)
    [super dealloc];
#endif
}

- (BOOL)initFramebuffer {
    const float nativeScale = [[UIScreen mainScreen] scale];
    
    self.contentScaleFactor = userContentScaleFactor;
    BE_LOG(L"Setting contentScaleFactor to %0.4f (optimal = %0.4f)", self.contentScaleFactor, nativeScale);
    
    if (self.contentScaleFactor == 1.0f || self.contentScaleFactor == 2.0f) {
        CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
        eaglLayer.magnificationFilter = kCAFilterNearest;
    }
    
    // Create FBO
    gglGenFramebuffers(1, &framebuffer);
    gglBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    
    // Create color buffer for display
    gglGenRenderbuffers(1, &colorbuffer);
    gglBindRenderbuffer(GL_RENDERBUFFER, colorbuffer);
    [glContext->eaglContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)self.layer];
    gglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorbuffer);
    
    // Get the size of the buffer
    gglGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backingWidth);
    gglGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingHeight);
    
    // Create depth buffer
    gglGenRenderbuffers(1, &depthbuffer);
    gglBindRenderbuffer(GL_RENDERBUFFER, depthbuffer);
    gglRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, backingWidth, backingHeight);
    gglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbuffer);
    
    GLenum status = gglCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        BE_FATALERROR(L"failed to make complete framebuffer object 0x%x", status);
        gglDeleteFramebuffers(1, &framebuffer);
        gglDeleteRenderbuffers(1, &colorbuffer);
        gglDeleteRenderbuffers(1, &depthbuffer);
        return NO;
    }
    
    return YES;
}

- (void)freeFramebuffer {
    gglDeleteFramebuffers(1, &framebuffer);
    gglDeleteRenderbuffers(1, &colorbuffer);
    gglDeleteRenderbuffers(1, &depthbuffer);
}

- (CGSize)backingPixelSize {
    CGSize size;
    size.width = backingWidth;
    size.height = backingHeight;
    return size;
}

- (void)setAnimationFrameInterval:(int)interval {
#if USE_DISPLAY_LINK
    if (interval <= 0 || interval == displayLinkFrameInterval) {
        return;
    }
    
    displayLinkFrameInterval = interval;
    
    if (displayLinkStarted) {
        [self stopDisplayLink];
        [self startDisplayLink];
    }
#endif
}

- (void)startDisplayLink {
#if USE_DISPLAY_LINK
    if (displayLinkStarted) {
        return;
    }
    
    displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(drawView:)];
    [displayLink setFrameInterval:displayLinkFrameInterval];
    //[displayLink setPreferredFramesPerSecond:60];
    [displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
    
    displayLinkStarted = YES;
#endif
}

- (void)stopDisplayLink {
#if USE_DISPLAY_LINK
    if (!displayLinkStarted) {
        return;
    }
    
    [displayLink invalidate];
    displayLink = nil;
    displayLinkStarted = NO;
#endif
}

- (void)layoutSubviews {
    gglBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // Allocate color buffer backing based on the current layer size
    gglBindRenderbuffer(GL_RENDERBUFFER, colorbuffer);
    [glContext->eaglContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)self.layer];
    gglGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backingWidth);
    gglGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingHeight);
    
    GLenum depthbufferStorage;
    gglBindRenderbuffer(GL_RENDERBUFFER, depthbuffer);
    gglGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, (GLint *)&depthbufferStorage);
    gglRenderbufferStorage(GL_RENDERBUFFER, depthbufferStorage, backingWidth, backingHeight);
    gglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbuffer);
    
    GLenum status = gglCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        BE_FATALERROR(L"Failed to make complete framebuffer object 0x%x", status);
    }
    
    [self drawView:nil];
}

- (void)drawView:(id)sender {
    glContext->displayFunc(glContext->handle, glContext->displayFuncDataPtr);
}

- (bool)swapBuffers {
    gglBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    
    gglBindRenderbuffer(GL_RENDERBUFFER, colorbuffer);
    
    // Discard the unncessary depth buffer for now
    const GLenum discards[]  = { GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT };
    //gglDiscardFramebufferEXT(GL_READ_FRAMEBUFFER, 2, discards);
    gglInvalidateFramebuffer(GL_READ_FRAMEBUFFER, 2, discards);
    
    bool succeeded = [glContext->eaglContext presentRenderbuffer:GL_RENDERBUFFER];
    return succeeded;
}

@end // @implementation EAGLView

//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------

BE_NAMESPACE_BEGIN

static int          majorVersion = 0;
static int          minorVersion = 0;

static CVar         gl_debug(L"gl_debug", L"1", CVar::Bool, L"");
static CVar         gl_debugLevel(L"gl_debugLevel", L"3", CVar::Integer, L"");
static CVar         gl_ignoreGLError(L"gl_ignoreGLError", L"0", CVar::Bool, L"");
static CVar         gl_finish(L"gl_finish", L"0", CVar::Bool, L"");

static void GetGLVersion(int *major, int *minor) {
    const char *verstr = (const char *)glGetString(GL_VERSION);
    if (!verstr || sscanf(verstr, "%d.%d", major, minor) != 2) {
        *major = *minor = 0;
    }
}

void OpenGLRHI::InitMainContext(WindowHandle windowHandle, const Settings *settings) {
    mainContext = new GLContext;
    mainContext->state = new GLState;
    
    // Create EAGLContext
    mainContext->eaglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
    if (!mainContext->eaglContext) {
        BE_FATALERROR(L"Couldn't create main EAGLContext");
    }
    
    // Make current context
    [EAGLContext setCurrentContext:mainContext->eaglContext];
    
    GetGLVersion(&majorVersion, &minorVersion);

    int decimalVersion = majorVersion * 10 + minorVersion;
    if (decimalVersion < 30) {
        //BLib::Error(FatalErr, L"Minimum OpenGL extensions missing !!\nRequired OpenGL 3.3 or higher graphic card");
    }

    // gglXXX 함수 바인딩 및 확장 flag 초기화
    ggl_init(gl_debug.GetBool());
    
    // default FBO
    mainContext->defaultFramebuffer = 0;
    
    // Create default VAO for main context
    gglGenVertexArrays(1, &mainContext->defaultVAO);
}

void OpenGLRHI::FreeMainContext() {
    // Delete default VAO for main context
    gglDeleteVertexArrays(1, &mainContext->defaultVAO);

    // Sets the current context to nil.
    [EAGLContext setCurrentContext:nil];

#if !__has_feature(objc_arc)
    [mainContext->eaglContext release];
#endif

    SAFE_DELETE(mainContext->state);
    SAFE_DELETE(mainContext);
}

RHI::Handle OpenGLRHI::CreateContext(RHI::WindowHandle windowHandle, bool useSharedContext) {
    GLContext *ctx = new GLContext;

    int handle = contextList.FindNull();
    if (handle == -1) {
        handle = contextList.Append(ctx);
    } else {
        contextList[handle] = ctx;
    }

    ctx->handle = (Handle)handle;
    ctx->onDemandDrawing = false;
    ctx->rootView = (__bridge UIView *)windowHandle;

    if (!useSharedContext) {
        ctx->state = mainContext->state;
        ctx->eaglContext = mainContext->eaglContext;
    } else {
        ctx->state = new GLState;
        ctx->eaglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3 sharegroup:[mainContext->eaglContext sharegroup]];
        if (!ctx->eaglContext) {
            BE_FATALERROR(L"Couldn't create main EAGLContext");
        }
    }
    
    CGRect contentRect = [ctx->rootView bounds];
    ctx->eaglView = [[EAGLView alloc] initWithFrame:CGRectMake(0, 0, contentRect.size.width, contentRect.size.height)];
    ctx->eaglView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    
    [ctx->eaglView setGLContext:ctx];
    
    [ctx->eaglView initFramebuffer];
    
    [ctx->rootView addSubview:ctx->eaglView];
    
    SetContext((Handle)handle);
    
    ctx->defaultFramebuffer = ctx->eaglView.framebuffer;
    
    if (useSharedContext) {
        // Create default VAO for shared context
        gglGenVertexArrays(1, &ctx->defaultVAO);
    } else {
        ctx->defaultVAO = mainContext->defaultVAO;
    }
    
    SetDefaultState();

    return (Handle)handle;
}

void OpenGLRHI::DestroyContext(Handle ctxHandle) {
    GLContext *ctx = contextList[ctxHandle];

    [ctx->eaglView stopDisplayLink];
    [ctx->eaglView removeFromSuperview];
    
    if (ctx->eaglContext != mainContext->eaglContext) {
        // Delete default VAO for shared context
        gglDeleteVertexArrays(1, &ctx->defaultVAO);
        
#if !__has_feature(objc_arc)
        [ctx->eaglContext release];
#endif
        
        delete ctx->state;
    }

    if (currentContext == ctx) {
        currentContext = mainContext;

        [EAGLContext setCurrentContext:mainContext->eaglContext];
    }
    
    delete ctx;
    contextList[ctxHandle] = NULL;
}

void OpenGLRHI::ActivateSurface(Handle ctxHandle, RHI::WindowHandle windowHandle) {
    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];
}

void OpenGLRHI::DeactivateSurface(Handle ctxHandle) {
    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];
}

void OpenGLRHI::SetContext(Handle ctxHandle) {
    EAGLContext *currentContext = [EAGLContext currentContext];
    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    if (currentContext != ctx->eaglContext) {
        // This ensures that previously submitted commands are delivered to the graphics hardware in a timely fashion.
        glFlush();
    }

    [EAGLContext setCurrentContext:ctx->eaglContext];

    this->currentContext = ctx;
}

void OpenGLRHI::SetContextDisplayFunc(Handle ctxHandle, DisplayContextFunc displayFunc, void *dataPtr, bool onDemandDrawing) {
    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];
    
    ctx->displayFunc = displayFunc;
    ctx->displayFuncDataPtr = dataPtr;
    ctx->onDemandDrawing = onDemandDrawing;
    
#if USE_DISPLAY_LINK
    [ctx->eaglView startDisplayLink];
#endif
}

void OpenGLRHI::DisplayContext(Handle ctxHandle) {
    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    [ctx->eaglView drawView:nil];
}

RHI::WindowHandle OpenGLRHI::GetWindowHandleFromContext(Handle ctxHandle) {
    const GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];
    
    return (__bridge WindowHandle)ctx->rootView;
}

void OpenGLRHI::GetContextSize(Handle ctxHandle, int *windowWidth, int *windowHeight, int *backingWidth, int *backingHeight) const {
    const GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];
    
    if (windowWidth || windowHeight) {
        CGSize viewSize = [ctx->eaglView bounds].size;
        *windowWidth = viewSize.width;
        *windowHeight = viewSize.height;
    }
    
    if (backingWidth || backingHeight) {
        CGSize backingSize = [ctx->eaglView backingPixelSize];
        *backingWidth = backingSize.width;
        *backingHeight = backingSize.height;
    }
}

bool OpenGLRHI::IsFullscreen() const {
    return true;
}

bool OpenGLRHI::SetFullscreen(Handle ctxHandle, int width, int height) {
    return true;
}

void OpenGLRHI::ResetFullscreen(Handle ctxHandle) {
}

void OpenGLRHI::GetGammaRamp(unsigned short ramp[768]) const {
}

void OpenGLRHI::SetGammaRamp(unsigned short ramp[768]) const {
}

bool OpenGLRHI::SwapBuffers() {
    if (!gl_ignoreGLError.GetBool()) {
        CheckError("OpenGLRHI::SwapBuffers");
    }

    if (gl_finish.GetBool()) {
        glFinish();
    }

    bool succeeded = [currentContext->eaglView swapBuffers];
    if (succeeded) {
        return false;
    }
    
    if (gl_debug.IsModified()) {
        ggl_rebind(gl_debug.GetBool());
        gl_debug.ClearModified();
    }

    return true;
}

void OpenGLRHI::SwapInterval(int interval) const {
    [currentContext->eaglView setAnimationFrameInterval:interval];
}

BE_NAMESPACE_END
