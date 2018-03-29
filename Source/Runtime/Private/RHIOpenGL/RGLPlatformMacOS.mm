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

#import <QuartzCore/CVDisplayLink.h>

#define SUPPORT_RETINA_RESOLUTION   1
#define USE_DISPLAY_LINK            0

@interface GLView : NSView
@end

@implementation GLView {
    BE1::GLContext *    glContext;
    CVDisplayLinkRef    displayLink;
    int                 backingPixelWidth;
    int                 backingPixelHeight;
}

- (void)setGLContext:(BE1::GLContext *)ctx {
    glContext = ctx;
}

- (id)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    
#if SUPPORT_RETINA_RESOLUTION
    // Opt-In to Retina resolution
    [self setWantsBestResolutionOpenGLSurface:YES];
#endif // SUPPORT_RETINA_RESOLUTION
    
    // NSViewGlobalFrameDidChangeNotification:
    // Posted whenever an NSView object that has attached surfaces (that is, NSOpenGLContext objects) moves to a different screen,
    // or other cases where the NSOpenGLContext object needs to be updated.
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(reshape)
                                                 name:NSViewGlobalFrameDidChangeNotification
                                               object:self];
    
    return self;
}

#if USE_DISPLAY_LINK
- (CVReturn)getFrameForTime:(const CVTimeStamp *)outputTime {
    // There is no autorelease pool when this method is called because it will be called from a background thread
    // It's important to create one or you will leak objects
    @autoreleasepool {
        //CFAbsoluteTime currentTime = CFAbsoluteTimeGetCurrent();
        //[[controller scene] advanceTimeBy:(currentTime - [controller renderTime])];
        //[controller setRenderTime:currentTime];
        [self drawView];
    }
    
    return kCVReturnSuccess;
}

// This is the renderer output callback function
static CVReturn displayLinkCallback(CVDisplayLinkRef displayLink,
    const CVTimeStamp *now, const CVTimeStamp *outputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext) {
    CVReturn result = [(__bridge GLView *)displayLinkContext getFrameForTime:outputTime];
    return result;
}

- (void)setupDisplayLink {
    // Create a display link capable of being used with all active displays
    CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
    
    // Set the renderer output callback function
    CVDisplayLinkSetOutputCallback(displayLink, &displayLinkCallback, (__bridge void *)(self));
    
    // Set the display link for the current renderer
    NSOpenGLPixelFormat *pf = [glContext->nsglContext pixelFormat];
    CGLPixelFormatObj cglPixelFormat = [pf CGLPixelFormatObj];
    CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, glContext->cglContext, cglPixelFormat);
}

- (void)startDisplayLink {
    if (!CVDisplayLinkIsRunning(displayLink)) {
        CVDisplayLinkStart(displayLink);
    }
}

- (void)stopDisplayLink {
    if (CVDisplayLinkIsRunning(displayLink)) {
        CVDisplayLinkStop(displayLink);
    }
}

#endif // USE_DISPLAY_LINK

- (void)dealloc {
#if USE_DISPLAY_LINK
    // Stop and release the display link
    CVDisplayLinkStop(displayLink);
    CVDisplayLinkRelease(displayLink);
#endif
    
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:NSViewGlobalFrameDidChangeNotification
                                                  object:self];
    
#if !__has_feature(objc_arc)
    [super dealloc];
#endif
}

- (NSRect)viewRectPixels {
    // Get the view size in Points
    NSRect viewRectPoints = [self bounds];
    
#if SUPPORT_RETINA_RESOLUTION
    // Rendering at retina resolutions will reduce aliasing, but at the potential
    // cost of framerate and battery life due to the GPU needing to render more
    // pixels.
    
    // Any calculations the renderer does which use pixel dimentions, must be
    // in "retina" space.  [NSView convertRectToBacking] converts point sizes
    // to pixel sizes.  Thus the renderer gets the size in pixels, not points,
    // so that it can set it's viewport and perform and other pixel based
    // calculations appropriately.
    // viewRectPixels will be larger (2x) than viewRectPoints for retina displays.
    // viewRectPixels will be the same as viewRectPoints for non-retina displays
    return [self convertRectToBacking:viewRectPoints];
#else
    // App will typically render faster and use less power rendering at
    // non-retina resolutions since the GPU needs to render less pixels.  There
    // is the cost of more aliasing, but it will be no-worse than on a Mac
    // without a retina display.
    
    // Points:Pixels is always 1:1 when not supporting retina resolutions
    return viewRectPoints;
#endif
}

- (CGSize)backingPixelSize {
    CGSize size;
    
    GLint fixedBackingEnabled;
    CGLIsEnabled(glContext->cglContext, kCGLCESurfaceBackingSize, &fixedBackingEnabled);
    
    if (fixedBackingEnabled) {
        GLint dim[2];
        CGLGetParameter(glContext->cglContext, kCGLCPSurfaceBackingSize, dim);
        
        size.width = dim[0];
        size.height = dim[1];
    } else {
        NSRect viewRectPixels = [self viewRectPixels];
        
        size.width = viewRectPixels.size.width;
        size.height = viewRectPixels.size.height;
    }

    return size;
}

- (void)reshape {
#if USE_DISPLAY_LINK
    // This method will be called on the main thread when resizing,
    // but we may be drawing on a secondary thread through the display link
    // Add a mutex around to avoid the threads accessing the context simultaneously
    CGLLockContext(glContext->cglContext);
#endif
    
    NSSize viewSize = [self bounds].size;
    if (viewSize.width > 1 && viewSize.height > 1) {
        [glContext->nsglContext update];
    }
    
#if USE_DISPLAY_LINK
    CGLUnlockContext(glContext->cglContext);
#endif
}

- (void)drawView {
#if USE_DISPLAY_LINK
    // This method will be called on both the main thread (through -drawRect:)
    // and a secondary thread (through the display link rendering loop)
    // Also, when resizing the view, -reshape is called on the main thread, but we may be drawing on a secondary thread
    // Add a mutex around to avoid the threads accessing the context simultaneously
    CGLLockContext(glContext->cglContext);
#endif
    
    glContext->displayFunc(glContext->handle, glContext->displayFuncDataPtr);
    
#if USE_DISPLAY_LINK
    CGLUnlockContext(glContext->cglContext);
#endif
}

- (void)drawRect:(NSRect)dirtyRect {
    if (!glContext->onDemandDrawing) {
        return;
    }
    
#if USE_DISPLAY_LINK
    // Ignore if the display link is still running
    if (!CVDisplayLinkIsRunning(displayLink)) {
        [self drawView];
    }
#else
    [self drawView];
#endif
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

@end // @implementation GLView

//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------

BE_NAMESPACE_BEGIN

static int          majorVersion = 0;
static int          minorVersion = 0;

static uint32_t     availVRAM = 0;

static CGDisplayModeRef desktopDisplayMode = NULL;

static NSOpenGLPixelFormat *mainContextPixelFormat;

static CVar         gl_debug(L"gl_debug", L"1", CVar::Bool, L"");
static CVar         gl_debugLevel(L"gl_debugLevel", L"3", CVar::Integer, L"");
static CVar         gl_ignoreGLError(L"gl_ignoreGLError", L"0", CVar::Bool, L"");
static CVar         gl_finish(L"gl_finish", L"0", CVar::Bool, L"");
static CVar         gl_screen(L"gl_screen", L"-1", CVar::Integer, L"");
static CVar         gl_useMacMTEngine(L"gl_useMacMTEngine", L"1", CVar::Bool, L"");

#define MAX_DISPLAYS 32

static CGDirectDisplayID MacOS_DisplayToUse() {
    static bool gotDisplay = NO;
    static CGDirectDisplayID displayToUse;

    if (gotDisplay) {
        return displayToUse;
    }

    CGDirectDisplayID displays[MAX_DISPLAYS];
    CGDisplayCount displayCount;

    // CGGenActiveDisplayList: The first entry in the list of active displays is the main display. 
    // In case of mirroring, the first entry is the largest drawable display or, if all are the same size, 
    // the display with the greatest pixel depth.
    CGDisplayErr err = CGGetActiveDisplayList(MAX_DISPLAYS, displays, &displayCount);
    if (err != CGDisplayNoErr) {
        BE_FATALERROR(L"Cannot get display list -- CGGetActiveDisplayList returned %d.", err);
    }

    // -1, the default, means to use the main screen
    int displayIndex = gl_screen.GetInteger();

    if (displayIndex < 0 || displayIndex >= displayCount) {
        // This is documented (in CGDirectDisplay.h) to be the main display. 
        // We want to return this instead of kCGDirectMainDisplay since this will allow us to compare display IDs.
        displayToUse = displays[0];
    } else {
        displayToUse = displays[displayIndex];
    }

    gotDisplay = YES;

    return displayToUse;
}

#define MAX_RENDERER_INFO_COUNT 128

// Returns zero if there are no hardware renderers.  Otherwise, returns the max memory across all renderers (on the presumption that the screen that we'll use has the most memory).
static uint32_t MacOS_QueryVideoMemory() {
    int maxVRAM = 0;
    int vram = 0;
    int rendererInfoCount = MAX_RENDERER_INFO_COUNT;
    CGLRendererInfoObj rendererInfos[MAX_RENDERER_INFO_COUNT];

    CGLError err = CGLQueryRendererInfo(CGDisplayIDToOpenGLDisplayMask(MacOS_DisplayToUse()), rendererInfos, &rendererInfoCount);
    if (err) {
        BE_LOG(L"CGLQueryRendererInfo -> %d\n", err);
        return vram;
    }

    int totalRenderers = 0;
    for (int rendererInfoIndex = 0; rendererInfoIndex < rendererInfoCount && totalRenderers < rendererInfoCount; rendererInfoIndex++) {
        CGLRendererInfoObj rendererInfo = rendererInfos[rendererInfoIndex];

        int rendererCount;
        err = CGLDescribeRenderer(rendererInfo, 0, kCGLRPRendererCount, &rendererCount);
        if (err) {
            BE_LOG(L"CGLDescribeRenderer(kCGLRPRendererID) -> %d\n", err);
            continue;
        }

        for (int rendererIndex = 0; rendererIndex < rendererCount; rendererIndex++) {
            totalRenderers++;

            int rendererID = 0xffffffff;
            err = CGLDescribeRenderer(rendererInfo, rendererIndex, kCGLRPRendererID, &rendererID);
            if (err) {
                BE_LOG(L"CGLDescribeRenderer(kCGLRPRendererID) -> %d\n", err);
                continue;
            }

            int accelerated = 0;
            err = CGLDescribeRenderer(rendererInfo, rendererIndex, kCGLRPAccelerated, &accelerated);
            if (err) {
                BE_LOG(L"CGLDescribeRenderer(kCGLRPAccelerated) -> %d\n", err);
                continue;
            }

            if (!accelerated) {
                continue;
            }

            vram = 0;
            err = CGLDescribeRenderer(rendererInfo, rendererIndex, kCGLRPVideoMemoryMegabytes, &vram);
            if (err) {
                BE_LOG(L"CGLDescribeRenderer -> %d\n", err);
                continue;
            }

            // presumably we'll be running on the best card, so we'll take the max of the vrams
            if (vram > maxVRAM) {
                maxVRAM = vram;
            }
        }

#if 0
        err = CGLDestroyRendererInfo(rendererInfo);
        if (err) {
            BLib::Log(NormalLog, "CGLDestroyRendererInfo -> %d\n", err);
        }
#endif
	}

	return maxVRAM;
}

static void MacOS_ResetFullscreen() {
    if (!desktopDisplayMode) {
        return;
    }
    
    CGDisplaySetDisplayMode(kCGDirectMainDisplay, desktopDisplayMode, NULL);
    CGDisplayModeRelease(desktopDisplayMode);
    CGDisplayShowCursor(kCGDirectMainDisplay);
    desktopDisplayMode = NULL;
}

static bool MacOS_SetFullscreen(int displayWidth, int displayHeight, bool stretched) {
    if (desktopDisplayMode) {
        MacOS_ResetFullscreen();
    }
    
    const CFStringRef pixelFormatCFStr = CFSTR(IO32BitDirectPixels);
    const int minRefresh = 0;
    const int maxRefresh = 0;
    
    // An array of display modes that the display supports, or NULL if the display is invalid.
    // The caller is responsible for releasing the array.
    CFArrayRef modeList = (CFArrayRef)CGDisplayCopyAllDisplayModes(kCGDirectMainDisplay, NULL);
    if (!modeList) {
        NSLog(@"CGDisplayAvailableModes returned NULL -- 0x%0x is an invalid display", kCGDirectMainDisplay);
        return false;
    }
    
    long modeCount = CFArrayGetCount(modeList);
    
    // Default to the current desktop mode
    int bestModeIndex = 0xFFFFFFFF;
    
    for (int modeIndex = 0; modeIndex < modeCount; ++modeIndex) {
        CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(modeList, modeIndex);
        
        // Make sure we get the right size
        if (CGDisplayModeGetWidth(mode) != displayWidth ||
            CGDisplayModeGetHeight(mode) != displayHeight) {
            continue;
        }
        
        // Make sure that our frequency restrictions are observed
        int refresh = CGDisplayModeGetRefreshRate(mode);
        if (minRefresh && refresh < minRefresh) {
            continue;
        }
        
        if (maxRefresh && refresh > maxRefresh) {
            continue;
        }
        
        CFStringRef pixelEncoding = CGDisplayModeCopyPixelEncoding(mode);
        if (CFStringCompare(pixelEncoding, pixelFormatCFStr, 0) != kCFCompareEqualTo) {
            CFRelease(pixelEncoding);
            continue;
        }
        
        CFRelease(pixelEncoding);

        /*id object = [mode objectForKey : (id)kCGDisplayModeIsStretched];
        if (object) {
            if ([object boolValue] != stretched) {
                continue;
            }
        } else {
            if (stretched) {
                continue;
            }
        }*/
        
        bestModeIndex = modeIndex;
    }
    
    if (bestModeIndex == 0xFFFFFFFF) {
        NSLog(@"No suitable display mode available.\n");
        return nil;
    }
    
    CGDisplayModeRef fullscreenMode = (CGDisplayModeRef)CFArrayGetValueAtIndex(modeList, bestModeIndex);
    
    desktopDisplayMode = CGDisplayCopyDisplayMode(kCGDirectMainDisplay);
    CGDisplayHideCursor(kCGDirectMainDisplay);
    CGDisplaySetDisplayMode(kCGDirectMainDisplay, fullscreenMode, NULL);
    CGDisplayCapture(kCGDirectMainDisplay);
    
    CFRelease(modeList);
    
    return true;
}

#define MAX_ATTRIB_SIZE 128

static NSOpenGLPixelFormatAttribute *GetPixelFormatAttributes(bool fullScreen, int inColorBits, int inAlphaBits, int inDepthBits, int inStencilBits, int inMultiSamples) {
    static NSOpenGLPixelFormatAttribute attribs[MAX_ATTRIB_SIZE];
    unsigned int numAttribs = 0;

    // only greater or equal attributes will be selected
    attribs[numAttribs++] = NSOpenGLPFAMinimumPolicy;
    attribs[numAttribs++] = 1;

    /*if (fullScreen) {
        attribs[numAttribs++] = NSOpenGLPFAFullScreen;
    }*/

    attribs[numAttribs++] = NSOpenGLPFAScreenMask;
    attribs[numAttribs++] = CGDisplayIDToOpenGLDisplayMask(MacOS_DisplayToUse());
    
    // OpenGL core profile
    attribs[numAttribs++] = NSOpenGLPFAOpenGLProfile;
    //attribs[numAttribs++] = NSOpenGLProfileVersionLegacy;
    attribs[numAttribs++] = NSOpenGLProfileVersion3_2Core;
    //attribs[numAttribs++] = NSOpenGLProfileVersion4_1Core;

    // Require hardware acceleration
    attribs[numAttribs++] = NSOpenGLPFAAccelerated;

    // Require double-buffer
    attribs[numAttribs++] = NSOpenGLPFADoubleBuffer;

    // color bits
    attribs[numAttribs++] = NSOpenGLPFAColorSize;
    attribs[numAttribs++] = inColorBits;

    // Specify the number of depth bits
    attribs[numAttribs++] = NSOpenGLPFADepthSize;
    attribs[numAttribs++] = inDepthBits;

    // Specify the number of stencil bits
    attribs[numAttribs++] = NSOpenGLPFAStencilSize;
    attribs[numAttribs++] = inStencilBits;

    // Specify destination alpha
    attribs[numAttribs++] = NSOpenGLPFAAlphaSize;
    attribs[numAttribs++] = inAlphaBits;

    if (inMultiSamples) {
        int buffers = 1;
        attribs[numAttribs++] = NSOpenGLPFASampleBuffers;
        attribs[numAttribs++] = buffers;
        attribs[numAttribs++] = NSOpenGLPFASamples;
        attribs[numAttribs++] = inMultiSamples;
    }

    // Terminate the list
    attribs[numAttribs++] = 0;

return attribs;
}

static void GetGLVersion(int *major, int *minor) {
#if 1
    // GL_MAJOR_VERSION, GL_MINOR_VERSION 쿼리는 3.0 core context 부터 지원된다
    glGetIntegerv(GL_MAJOR_VERSION, major);
    glGetIntegerv(GL_MINOR_VERSION, minor);
#else
    const char *verstr = (const char *)glGetString(GL_VERSION);
    if (!verstr || sscanf(verstr, "%d.%d", major, minor) != 2) {
        *major = *minor = 0;
    }
#endif
}

void OpenGLRHI::InitMainContext(WindowHandle windowHandle, const Settings *settings) {
    availVRAM = MacOS_QueryVideoMemory();

    mainContext = new GLContext;
    mainContext->state = new GLState;
    mainContext->display = MacOS_DisplayToUse();
    //mainContext->glView = [[GLView alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)];
    
    //[mainContext->glView setGLContext:mainContext];
    
    // Create pixel format
    NSOpenGLPixelFormatAttribute *attribs = GetPixelFormatAttributes(false,
        settings->colorBits, settings->alphaBits, settings->depthBits, settings->stencilBits, settings->multiSamples);
    mainContextPixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes: attribs];
    if (!mainContextPixelFormat) {
        BE_FATALERROR(L"no pixel format found");
    }

    // Create NSOpenGLContext object
    mainContext->nsglContext = [[NSOpenGLContext alloc] initWithFormat:mainContextPixelFormat shareContext: nil];
    if (!mainContext->nsglContext) {
        BE_FATALERROR(L"Couldn't create main NSOpenGLContext");
    }

    // Get CGLContextObj from NSOpenGLContext object
    mainContext->cglContext = (CGLContextObj)[mainContext->nsglContext CGLContextObj];
    
#if 0 //SUPPORT_GL3
    // When we're using a CoreProfile context, crash if we call a legacy OpenGL function
    // This will make it much more obvious where and when such a function call is made so
    // that we can remove such calls.
    // Without this we'd simply get GL_INVALID_OPERATION error for calling legacy functions
    // but it would be more difficult to see where that function was called.
    CGLEnable(mainContext->cglContext, kCGLCECrashOnRemovedFunctions);
#endif

    if (gl_useMacMTEngine.GetBool()) {
        CGLEnable(mainContext->cglContext, kCGLCEMPEngine);
    }
    
    //[mainContext->nsglContext setView:mainContext->glView];

    // Make current context
    [mainContext->nsglContext makeCurrentContext];

    GetGLVersion(&majorVersion, &minorVersion);

    int decimalVersion = majorVersion * 10 + minorVersion;
    if (decimalVersion < 33) {
        BE_FATALERROR(L"Minimum OpenGL extensions missing !!\nRequired OpenGL 3.3 or higher graphic card");
    }

    // gglXXX 함수 바인딩 및 확장 flag 초기화
    ggl_init(gl_debug.GetBool() && !gglext._GL_ARB_debug_output);
    
    // Enable debug callback
    if (gl_debug.GetBool() && gglext._GL_ARB_debug_output) {
        gglDebugMessageCallbackARB(OpenGL::DebugCallback, NULL);
        gglEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    }
    
    // default FBO
    mainContext->defaultFramebuffer = 0;
    
    // Create default VAO for main context
    gglGenVertexArrays(1, &mainContext->defaultVAO);
}

void OpenGLRHI::FreeMainContext() {
    // Delete default VAO for main context
    gglDeleteVertexArrays(1, &mainContext->defaultVAO);

    [NSOpenGLContext clearCurrentContext];

    if (CGLClearDrawable(mainContext->cglContext) != kCGLNoError) {
        BE_FATALERROR(L"CGLClearDrawable: failed");
    }

    // This method disassociates the receiver from any associated NSView object.If the receiver is in full - screen or offscreen mode, it exits that mode.
    [mainContext->nsglContext clearDrawable];

#if !__has_feature(objc_arc)
    [mainContext->nsglContext release];
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
    ctx->display = mainContext->display;
    ctx->contentView = (__bridge NSView *)windowHandle;

    if (!useSharedContext) {
        ctx->state = mainContext->state;
        ctx->nsglContext = mainContext->nsglContext;
        ctx->cglContext = mainContext->cglContext;
        ctx->defaultVAO = mainContext->defaultVAO;
    } else {
        ctx->state = new GLState;

        ctx->nsglContext = [[NSOpenGLContext alloc] initWithFormat:mainContextPixelFormat shareContext:mainContext->nsglContext];
        if (!ctx->nsglContext) {
            BE_FATALERROR(L"Couldn't create NSOpenGLContext");
        }

        // Get CGLContextObj from NSOpenGLContext object
ctx->cglContext = (CGLContextObj)[ctx->nsglContext CGLContextObj];
        
#if 0 //SUPPORT_GL3
        // When we're using a CoreProfile context, crash if we call a legacy OpenGL function
        // This will make it much more obvious where and when such a function call is made so
        // that we can remove such calls.
        // Without this we'd simply get GL_INVALID_OPERATION error for calling legacy functions
        // but it would be more difficult to see where that function was called.
        CGLEnable(mainContext->cglContext, kCGLCECrashOnRemovedFunctions);
#endif

        if (gl_useMacMTEngine.GetBool()) {
            CGLEnable(ctx->cglContext, kCGLCEMPEngine);
        }

        // Set swap interval to 0
        //int swapInterval = 0;
        //[ctx->nsglContext setValues: &swapInterval forParameter: NSOpenGLCPSwapInterval];

        // Setup Opacity - can't change it dynamically later!
        //int surfaceOpacity = 0;
        //[ctx->nsglContext setValues: &surfaceOpacity forParameter: NSOpenGLCPSurfaceOpacity];
    }
    
    NSRect contentRect = [ctx->contentView bounds];
    
    ctx->glView = [[GLView alloc] initWithFrame:NSMakeRect(0, 0, contentRect.size.width, contentRect.size.height)];
    
    [ctx->glView setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];
    [ctx->glView setGLContext:ctx];
    
#if USE_DISPLAY_LINK
    [ctx->glView setupDisplayLink];
#endif

    [ctx->contentView addSubview:ctx->glView];

    [ctx->nsglContext setView:ctx->glView];

    [[ctx->contentView window] setInitialFirstResponder:ctx->glView];

    GLint fragmentGPUProcessing, vertexGPUProcessing;
    CGLGetParameter(ctx->cglContext, kCGLCPGPUVertexProcessing, &vertexGPUProcessing);
    CGLGetParameter(ctx->cglContext, kCGLCPGPUFragmentProcessing, &fragmentGPUProcessing);
    
    BE_LOG(L"GPU vertex processing: %hs\n", vertexGPUProcessing ? "YES" : "NO");
    BE_LOG(L"GPU fragment processing: %hs\n", fragmentGPUProcessing ? "YES" : "NO");

    SetContext((Handle)handle);

    ctx->defaultFramebuffer = 0;
    
    if (useSharedContext) {
        // Create default VAO for shared context
        gglGenVertexArrays(1, &ctx->defaultVAO);
    }

    SetDefaultState();

    return (Handle)handle;
}

void OpenGLRHI::DestroyContext(Handle ctxHandle) {
    GLContext *ctx = contextList[ctxHandle];
    
    if (ctx->nsglContext != mainContext->nsglContext) {
        // Delete default VAO for shared context
        gglDeleteVertexArrays(1, &ctx->defaultVAO);

        [ctx->nsglContext clearDrawable];

#if !__has_feature(objc_arc)
        [ctx->nsglContext release];
#endif

        delete ctx->state;
    }

    if (currentContext == ctx) {
        currentContext = mainContext;

        [mainContext->nsglContext makeCurrentContext];
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
    NSOpenGLContext *currentContext = [NSOpenGLContext currentContext];
    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    if (currentContext != ctx->nsglContext) {
        // This ensures that previously submitted commands are delivered to the graphics hardware in a timely fashion.
        glFlush();
    }

    [ctx->nsglContext setView:ctx->glView];

    [ctx->nsglContext makeCurrentContext];

    this->currentContext = ctx;
}

void OpenGLRHI::SetContextDisplayFunc(Handle ctxHandle, DisplayContextFunc displayFunc, void *dataPtr, bool onDemandDrawing) {
    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    ctx->displayFunc = displayFunc;
    ctx->displayFuncDataPtr = dataPtr;
    ctx->onDemandDrawing = onDemandDrawing;

#if USE_DISPLAY_LINK
    [ctx->glView startDisplayLink];
#endif
}

void OpenGLRHI::DisplayContext(Handle ctxHandle) {
    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    [ctx->glView drawView];
}

RHI::WindowHandle OpenGLRHI::GetWindowHandleFromContext(Handle ctxHandle) {
    const GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    return (__bridge WindowHandle)ctx->contentView;
}

void OpenGLRHI::GetContextSize(Handle ctxHandle, int *windowWidth, int *windowHeight, int *backingWidth, int *backingHeight) const {
    const GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    if (windowWidth || windowHeight) {
        CGSize windowSize = [ctx->glView bounds].size;
        *windowWidth = windowSize.width;
        *windowHeight = windowSize.height;
    }

    if (backingWidth || backingHeight) {
        CGSize backingSize = [ctx->glView backingPixelSize];
        *backingWidth = backingSize.width;
        *backingHeight = backingSize.height;
    }
}

bool OpenGLRHI::IsFullscreen() const {
    return desktopDisplayMode != NULL ? true : false;
}

bool OpenGLRHI::SetFullscreen(Handle ctxHandle, int width, int height) {
    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    GLint dim[2] = { width, height };
    CGLSetParameter(ctx->cglContext, kCGLCPSurfaceBackingSize, dim);
    CGLEnable(ctx->cglContext, kCGLCESurfaceBackingSize);
    
    /*NSWindow *window = [ctx->contentView window];
    
    ctx->windowTitle = [window title];
    ctx->windowSize = [window frame].size;
    
    [window setStyleMask:NSBorderlessWindowMask | NSFullSizeContentViewWindowMask];
    [window setLevel:NSMainMenuWindowLevel+1];
    [window setFrame:[[NSScreen mainScreen] frame] display:YES];
    [window setHidesOnDeactivate:YES];*/
    
    return true;
}

void OpenGLRHI::ResetFullscreen(Handle ctxHandle) {
    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    CGLDisable(ctx->cglContext, kCGLCESurfaceBackingSize);
    
    /*NSWindow *window = [ctx->contentView window];
    
    [window setStyleMask:NSTitledWindowMask | NSClosableWindowMask | NSResizableWindowMask];
    [window setLevel:NSNormalWindowLevel];
    [window setFrame:NSMakeRect(0, 0, ctx->windowSize.width, ctx->windowSize.height) display:YES];
    [window setTitle:ctx->windowTitle];
    [window setHidesOnDeactivate:NO];
    
    NSRect mainDisplayRect = [[NSScreen mainScreen] frame];
    NSRect windowRect = [window frame];
    
    NSPoint newPos = NSMakePoint(MAX(0, (mainDisplayRect.size.width - windowRect.size.width) / 2),
                                 MAX(0, (mainDisplayRect.size.height - windowRect.size.height) / 2));
    
    [window setFrameOrigin:newPos];*/
}

void OpenGLRHI::GetGammaRamp(unsigned short ramp[768]) const {
    uint32_t tableSize = 256;
    CGGammaValue *values = (CGGammaValue *)calloc(tableSize * 3, sizeof(CGGammaValue));
    
    CGGetDisplayTransferByTable(mainContext->display, tableSize, values, values + tableSize, values + tableSize * 2, &tableSize);
    
    for (uint32_t i = 0; i < tableSize; i++) {
        ramp[i]         = (unsigned short)(values[i] * 65535);
        ramp[256 + i]   = (unsigned short)(values[i + tableSize] * 65535);
        ramp[512 + i]   = (unsigned short)(values[i + tableSize * 2] * 65535);
    }
    
    free(values);
}

void OpenGLRHI::SetGammaRamp(unsigned short ramp[768]) const {
    CGGammaValue *values = (CGGammaValue *)calloc(768, sizeof(CGGammaValue));
    
    for (int i = 0; i < 256;  i++) {
        values[i]       = ramp[i] / 65535.f;
        values[256 + i] = ramp[256 + i] / 65535.f;
        values[512 + i] = ramp[512 + i] / 65535.f;
    }
    
    CGSetDisplayTransferByTable(mainContext->display, 256, values, values + 256, values + 512);
    
    free(values);
}

bool OpenGLRHI::SwapBuffers() {
    if (!gl_ignoreGLError.GetBool()) {
        CheckError("GLRenderer::SwapBuffers");
    }

    if (gl_finish.GetBool()) {
        glFinish();
    }

    [currentContext->nsglContext flushBuffer];
    
    //EAGLContext *context = [EAGLContext currentContext];
    //[context presentRenderbuffer:GL_RENDERBUFFER];

    if (gl_debug.IsModified()) {
        if (gglext._GL_ARB_debug_output) {
            if (gl_debug.GetBool()) {
                gglDebugMessageCallbackARB(OpenGL::DebugCallback, NULL);
                gglEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
            } else {
                gglDebugMessageCallbackARB(NULL, NULL);
                gglDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
            }
        } else {
            ggl_rebind(gl_debug.GetBool());
        }
        gl_debug.ClearModified();
    }

    return true;
}

void OpenGLRHI::SwapInterval(int interval) const {
	[currentContext->nsglContext setValues: &interval forParameter: NSOpenGLCPSwapInterval];
}

BE_NAMESPACE_END
