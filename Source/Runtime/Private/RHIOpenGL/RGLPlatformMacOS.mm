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
#include "Platform/PlatformTime.h"
#include "Profiler/Profiler.h"

#import <QuartzCore/CVDisplayLink.h>

#define SUPPORT_RETINA_RESOLUTION   1
#define USE_DISPLAY_LINK            0

@interface GLView : NSView

@end

@implementation GLView {
    BE1::GLContext *    glContext;
    CVDisplayLinkRef    displayLink;
    BOOL                wasAcceptingMouseMoveEvents;
}

- (void)setGLContext:(BE1::GLContext *)ctx {
    glContext = ctx;
}

- (id)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    
    [[self window] setAcceptsMouseMovedEvents:YES];

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
    // There is no autorelease pool when this method is called because it will be called from a background thread.
    // It's important to create one or you will leak objects.
    @autoreleasepool {
        //CFAbsoluteTime currentTime = CFAbsoluteTimeGetCurrent();
        //[[controller scene] advanceTimeBy:(currentTime - [controller renderTime])];
        //[controller setRenderTime:currentTime];
        [self drawView];
    }
    
    return kCVReturnSuccess;
}

// This is the renderer output callback function.
static CVReturn displayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *now, const CVTimeStamp *outputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext) {
    CVReturn result = [(__bridge GLView *)displayLinkContext getFrameForTime:outputTime];
    return result;
}

- (void)setupDisplayLink {
    // Create a display link capable of being used with all active displays.
    CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
    
    // Set the renderer output callback function.
    CVDisplayLinkSetOutputCallback(displayLink, &displayLinkCallback, (__bridge void *)(self));
    
    // Set the display link for the current renderer.
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
    // but we may be drawing on a secondary thread through the display link.
    // Add a mutex around to avoid the threads accessing the context simultaneously.
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
    // Also, when resizing the view, -reshape is called on the main thread, but we may be drawing on a secondary thread.
    // Add a mutex around to avoid the threads accessing the context simultaneously.
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
    // Ignore if the display link is still running.
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

#ifdef ENABLE_IMGUI

// Undocumented methods for creating cursors.
@interface NSCursor()
+ (id)_windowResizeNorthWestSouthEastCursor;
+ (id)_windowResizeNorthEastSouthWestCursor;
+ (id)_windowResizeNorthSouthCursor;
+ (id)_windowResizeEastWestCursor;
@end

static NSCursor*      g_MouseCursors[ImGuiMouseCursor_COUNT] = {};
static bool           g_MouseCursorHidden = false;

static void ImGui_ImplOSX_UpdateMouseCursor() {
    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) {
        return;
    }

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None) {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        if (!g_MouseCursorHidden) {
            g_MouseCursorHidden = true;
            [NSCursor hide];
        }
    } else {
        // Show OS mouse cursor
        [g_MouseCursors[g_MouseCursors[imgui_cursor] ? imgui_cursor : ImGuiMouseCursor_Arrow] set];
        if (g_MouseCursorHidden) {
            g_MouseCursorHidden = false;
            [NSCursor unhide];
        }
    }
}

static int mapCharacterToKey(int c) {
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 'A';
    }
    if (c == 25) { // SHIFT+TAB -> TAB
        return 9;
    }
    if (c >= 0 && c < 256) {
        return c;
    }
    if (c >= 0xF700 && c < 0xF700 + 256) {
        return c - 0xF700 + 256;
    }
    return -1;
}

static void resetKeys() {
    ImGuiIO &io = ImGui::GetIO();
    for (int n = 0; n < IM_ARRAYSIZE(io.KeysDown); n++) {
        io.KeysDown[n] = false;
    }
}

static bool ImGui_ImplOSX_HandleEvent(NSEvent *event, NSView *view) {
    ImGuiIO &io = ImGui::GetIO();

    if (event.type == NSEventTypeLeftMouseDown || event.type == NSEventTypeRightMouseDown || event.type == NSEventTypeOtherMouseDown) {
        int button = (int)[event buttonNumber];
        if (button >= 0 && button < IM_ARRAYSIZE(io.MouseDown)) {
            io.MouseDown[button] = true;
        }
        return io.WantCaptureMouse;
    }

    if (event.type == NSEventTypeLeftMouseUp || event.type == NSEventTypeRightMouseUp || event.type == NSEventTypeOtherMouseUp) {
        int button = (int)[event buttonNumber];
        if (button >= 0 && button < IM_ARRAYSIZE(io.MouseDown)) {
            io.MouseDown[button] = false;
        }
        return io.WantCaptureMouse;
    }

    if (event.type == NSEventTypeMouseMoved || event.type == NSEventTypeLeftMouseDragged) {
        NSPoint mousePoint = event.locationInWindow;
        mousePoint = [view convertPoint:mousePoint fromView:nil];
        mousePoint = NSMakePoint(mousePoint.x, view.bounds.size.height - mousePoint.y);
        io.MousePos = ImVec2(mousePoint.x, mousePoint.y);
    }

    if (event.type == NSEventTypeScrollWheel) {
        double wheel_dx = 0.0;
        double wheel_dy = 0.0;

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
        if (floor(NSAppKitVersionNumber) > NSAppKitVersionNumber10_6) {
            wheel_dx = [event scrollingDeltaX];
            wheel_dy = [event scrollingDeltaY];
            if ([event hasPreciseScrollingDeltas]) {
                wheel_dx *= 0.1;
                wheel_dy *= 0.1;
            }
        } else
#endif // MAC_OS_X_VERSION_MAX_ALLOWED
        {
            wheel_dx = [event deltaX];
            wheel_dy = [event deltaY];
        }

        if (fabs(wheel_dx) > 0.0) {
            io.MouseWheelH += wheel_dx * 0.1f;
        }
        if (fabs(wheel_dy) > 0.0) {
            io.MouseWheel += wheel_dy * 0.1f;
        }
        return io.WantCaptureMouse;
    }

    // FIXME: All the key handling is wrong and broken. Refer to GLFW's cocoa_init.mm and cocoa_window.mm.
    if (event.type == NSEventTypeKeyDown) {
        NSString *str = [event characters];
        int len = (int)[str length];
        for (int i = 0; i < len; i++) {
            int c = [str characterAtIndex:i];
            if (!io.KeyCtrl && !(c >= 0xF700 && c <= 0xFFFF) && c != 127) {
                io.AddInputCharacter((unsigned int)c);
            }

            // We must reset in case we're pressing a sequence of special keys while keeping the command pressed
            int key = mapCharacterToKey(c);
            if (key != -1 && key < 256 && !io.KeyCtrl) {
                resetKeys();
            }
            if (key != -1) {
                io.KeysDown[key] = true;
            }
        }
        return io.WantCaptureKeyboard;
    }

    if (event.type == NSEventTypeKeyUp) {
        NSString *str = [event characters];
        int len = (int)[str length];
        for (int i = 0; i < len; i++) {
            int c = [str characterAtIndex:i];
            int key = mapCharacterToKey(c);
            if (key != -1) {
                io.KeysDown[key] = false;
            }
        }
        return io.WantCaptureKeyboard;
    }

    if (event.type == NSEventTypeFlagsChanged) {
        ImGuiIO &io = ImGui::GetIO();
        unsigned int flags = [event modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;

        bool oldKeyCtrl = io.KeyCtrl;
        bool oldKeyShift = io.KeyShift;
        bool oldKeyAlt = io.KeyAlt;
        bool oldKeySuper = io.KeySuper;

        io.KeyCtrl      = flags & NSEventModifierFlagControl;
        io.KeyShift     = flags & NSEventModifierFlagShift;
        io.KeyAlt       = flags & NSEventModifierFlagOption;
        io.KeySuper     = flags & NSEventModifierFlagCommand;

        // We must reset them as we will not receive any keyUp event if they where pressed with a modifier
        if ((oldKeyShift && !io.KeyShift) || (oldKeyCtrl && !io.KeyCtrl) || (oldKeyAlt && !io.KeyAlt) || (oldKeySuper && !io.KeySuper)) {
            resetKeys();
        }
        return io.WantCaptureKeyboard;
    }

    return false;
}

- (void)keyDown:(NSEvent *)event {
    if (ImGui_ImplOSX_HandleEvent(event, self)) {
        return;
    }
    [super keyDown:event];
}

- (void)keyUp:(NSEvent *)event {
    if (ImGui_ImplOSX_HandleEvent(event, self)) {
        return;
    }
    [super keyUp:event];
}

- (void)flagsChanged:(NSEvent *)event {
    if (ImGui_ImplOSX_HandleEvent(event, self)) {
        return;
    }
    [super flagsChanged:event];
}

- (void)mouseDown:(NSEvent *)event {
    if (ImGui_ImplOSX_HandleEvent(event, self)) {
        return;
    }
    [super mouseDown:event];
}

- (void)mouseUp:(NSEvent *)event {
    if (ImGui_ImplOSX_HandleEvent(event, self)) {
        return;
    }
    [super mouseUp:event];
}

- (void)mouseMoved:(NSEvent *)event {
    if (ImGui_ImplOSX_HandleEvent(event, self)) {
        return;
    }
    [super mouseMoved:event];
}

- (void)mouseDragged:(NSEvent *)event {
    if (ImGui_ImplOSX_HandleEvent(event, self)) {
        return;
    }
    [super mouseDragged:event];
}

- (void)scrollWheel:(NSEvent *)event {
    if (ImGui_ImplOSX_HandleEvent(event, self)) {
        return;
    }
    [super scrollWheel:event];
}

#endif

@end // end of @implementation GLView

//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------

BE_NAMESPACE_BEGIN

static int          majorVersion = 0;
static int          minorVersion = 0;

static uint32_t     availVRAM = 0;

static CGDisplayModeRef desktopDisplayMode = nullptr;

static NSOpenGLPixelFormat *mainContextPixelFormat;

static CVar         gl_debug("gl_debug", "0", CVar::Flag::Bool, "");
static CVar         gl_debugLevel("gl_debugLevel", "3", CVar::Flag::Integer, "");
static CVar         gl_ignoreError("gl_ignoreError", "0", CVar::Flag::Bool, "");
static CVar         gl_finish("gl_finish", "0", CVar::Flag::Bool, "");
static CVar         gl_screen("gl_screen", "-1", CVar::Flag::Integer, "");
static CVar         gl_useMacMTEngine("gl_useMacMTEngine", "1", CVar::Flag::Bool, "");

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
        BE_FATALERROR("Cannot get display list -- CGGetActiveDisplayList returned %d.", err);
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
        BE_LOG("CGLQueryRendererInfo -> %d\n", err);
        return vram;
    }

    int totalRenderers = 0;
    for (int rendererInfoIndex = 0; rendererInfoIndex < rendererInfoCount && totalRenderers < rendererInfoCount; rendererInfoIndex++) {
        CGLRendererInfoObj rendererInfo = rendererInfos[rendererInfoIndex];

        int rendererCount;
        err = CGLDescribeRenderer(rendererInfo, 0, kCGLRPRendererCount, &rendererCount);
        if (err) {
            BE_LOG("CGLDescribeRenderer(kCGLRPRendererID) -> %d\n", err);
            continue;
        }

        for (int rendererIndex = 0; rendererIndex < rendererCount; rendererIndex++) {
            totalRenderers++;

            int rendererID = 0xffffffff;
            err = CGLDescribeRenderer(rendererInfo, rendererIndex, kCGLRPRendererID, &rendererID);
            if (err) {
                BE_LOG("CGLDescribeRenderer(kCGLRPRendererID) -> %d\n", err);
                continue;
            }

            int accelerated = 0;
            err = CGLDescribeRenderer(rendererInfo, rendererIndex, kCGLRPAccelerated, &accelerated);
            if (err) {
                BE_LOG("CGLDescribeRenderer(kCGLRPAccelerated) -> %d\n", err);
                continue;
            }

            if (!accelerated) {
                continue;
            }

            vram = 0;
            err = CGLDescribeRenderer(rendererInfo, rendererIndex, kCGLRPVideoMemoryMegabytes, &vram);
            if (err) {
                BE_LOG("CGLDescribeRenderer -> %d\n", err);
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
            BLib::Log(LogLevel::Normal, "CGLDestroyRendererInfo -> %d\n", err);
        }
#endif
    }

    return maxVRAM;
}

static void MacOS_ResetFullscreen() {
    if (!desktopDisplayMode) {
        return;
    }

    CGDisplaySetDisplayMode(kCGDirectMainDisplay, desktopDisplayMode, nullptr);
    CGDisplayModeRelease(desktopDisplayMode);
    CGDisplayShowCursor(kCGDirectMainDisplay);
    desktopDisplayMode = nullptr;
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
    CFArrayRef modeList = (CFArrayRef)CGDisplayCopyAllDisplayModes(kCGDirectMainDisplay, nullptr);
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
    CGDisplaySetDisplayMode(kCGDirectMainDisplay, fullscreenMode, nullptr);
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
        BE_FATALERROR("no pixel format found");
    }

    // Create NSOpenGLContext object
    mainContext->nsglContext = [[NSOpenGLContext alloc] initWithFormat:mainContextPixelFormat shareContext: nil];
    if (!mainContext->nsglContext) {
        BE_FATALERROR("Couldn't create main NSOpenGLContext");
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
        BE_FATALERROR("Minimum OpenGL extensions missing !!\nRequired OpenGL 3.3 or higher graphic card");
    }

    // gglXXX 함수 바인딩 및 확장 flag 초기화
    ggl_init(gl_debug.GetBool() && !gglext._GL_ARB_debug_output);

    // Enable debug callback
    if (gl_debug.GetBool() && gglext._GL_ARB_debug_output) {
        gglDebugMessageCallbackARB(OpenGL::DebugCallback, nullptr);
        gglEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    }

    // default FBO
    mainContext->defaultFramebuffer = 0;

    // Create default VAO for main context
    gglGenVertexArrays(1, &mainContext->defaultVAO);

#ifdef ENABLE_IMGUI
    ImGuiCreateContext(mainContext);
#endif
}

void OpenGLRHI::FreeMainContext() {
#ifdef ENABLE_IMGUI
    ImGuiDestroyContext(mainContext);
#endif

    // Delete default VAO for main context
    gglDeleteVertexArrays(1, &mainContext->defaultVAO);

    [NSOpenGLContext clearCurrentContext];

    if (CGLClearDrawable(mainContext->cglContext) != kCGLNoError) {
        BE_FATALERROR("CGLClearDrawable: failed");
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
            BE_FATALERROR("Couldn't create NSOpenGLContext");
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
    [[ctx->contentView window] setAcceptsMouseMovedEvents:YES];

    GLint fragmentGPUProcessing, vertexGPUProcessing;
    CGLGetParameter(ctx->cglContext, kCGLCPGPUVertexProcessing, &vertexGPUProcessing);
    CGLGetParameter(ctx->cglContext, kCGLCPGPUFragmentProcessing, &fragmentGPUProcessing);

    BE_DLOG("GPU vertex processing: %s\n", vertexGPUProcessing ? "YES" : "NO");
    BE_DLOG("GPU fragment processing: %s\n", fragmentGPUProcessing ? "YES" : "NO");

#ifdef ENABLE_IMGUI
    ctx->imGuiContext = mainContext->imGuiContext;
    ctx->imGuiLastTime = PlatformTime::Seconds();
#endif

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
    contextList[ctxHandle] = nullptr;
}

void OpenGLRHI::ActivateSurface(Handle ctxHandle, WindowHandle windowHandle) {
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

#ifdef ENABLE_IMGUI
    ImGui::SetCurrentContext(ctx->imGuiContext);
#endif
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

void OpenGLRHI::GetDisplayMetrics(Handle ctxHandle, DisplayMetrics *displayMetrics) const {
    const GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    CGSize windowSize = [ctx->glView bounds].size;
    displayMetrics->screenWidth = windowSize.width;
    displayMetrics->screenHeight = windowSize.height;
    
    CGSize backingSize = [ctx->glView backingPixelSize];
    displayMetrics->backingWidth = backingSize.width;
    displayMetrics->backingHeight = backingSize.height;

    displayMetrics->safeAreaInsets.Set(0, 0, 0, 0);
}

bool OpenGLRHI::IsFullscreen() const {
    return desktopDisplayMode != nullptr ? true : false;
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
    if (!gl_ignoreError.GetBool()) {
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
                gglDebugMessageCallbackARB(OpenGL::DebugCallback, nullptr);
                gglEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
            } else {
                gglDebugMessageCallbackARB(nullptr, nullptr);
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

void OpenGLRHI::ImGuiCreateContext(GLContext *ctx) {
    // Setup Dear ImGui context.
    ctx->imGuiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(ctx->imGuiContext);
    ImGuiIO &io = ImGui::GetIO();

    // Setup Dear ImGui style.
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    io.IniFilename = nullptr;

    // Setup back-end capabilities flags.
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;           // We can honor GetMouseCursor() values (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
    //io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;    // We can create multi-viewports on the Platform side (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can set io.MouseHoveredViewport correctly (optional, not easy)
    io.BackendPlatformName = "OpenGLRHI-MacOS";

    // Keyboard mapping. Dear ImGui will use those indices to peek into the io.KeyDown[] array.
    const int offset_for_function_keys = 256 - 0xF700;
    io.KeyMap[ImGuiKey_Tab]             = '\t';
    io.KeyMap[ImGuiKey_LeftArrow]       = NSLeftArrowFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_RightArrow]      = NSRightArrowFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_UpArrow]         = NSUpArrowFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_DownArrow]       = NSDownArrowFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_PageUp]          = NSPageUpFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_PageDown]        = NSPageDownFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_Home]            = NSHomeFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_End]             = NSEndFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_Insert]          = NSInsertFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_Delete]          = NSDeleteFunctionKey + offset_for_function_keys;
    io.KeyMap[ImGuiKey_Backspace]       = 127;
    io.KeyMap[ImGuiKey_Space]           = 32;
    io.KeyMap[ImGuiKey_Enter]           = 13;
    io.KeyMap[ImGuiKey_Escape]          = 27;
    io.KeyMap[ImGuiKey_KeyPadEnter]     = 13;
    io.KeyMap[ImGuiKey_A]               = 'A';
    io.KeyMap[ImGuiKey_C]               = 'C';
    io.KeyMap[ImGuiKey_V]               = 'V';
    io.KeyMap[ImGuiKey_X]               = 'X';
    io.KeyMap[ImGuiKey_Y]               = 'Y';
    io.KeyMap[ImGuiKey_Z]               = 'Z';

    // Load cursors. Some of them are undocumented.
    g_MouseCursorHidden = false;
    g_MouseCursors[ImGuiMouseCursor_Arrow] = [NSCursor arrowCursor];
    g_MouseCursors[ImGuiMouseCursor_TextInput] = [NSCursor IBeamCursor];
    g_MouseCursors[ImGuiMouseCursor_ResizeAll] = [NSCursor closedHandCursor];
    g_MouseCursors[ImGuiMouseCursor_Hand] = [NSCursor pointingHandCursor];
    g_MouseCursors[ImGuiMouseCursor_NotAllowed] = [NSCursor operationNotAllowedCursor];
    g_MouseCursors[ImGuiMouseCursor_ResizeNS] = [NSCursor respondsToSelector:@selector(_windowResizeNorthSouthCursor)] ? [NSCursor _windowResizeNorthSouthCursor] : [NSCursor resizeUpDownCursor];
    g_MouseCursors[ImGuiMouseCursor_ResizeEW] = [NSCursor respondsToSelector:@selector(_windowResizeEastWestCursor)] ? [NSCursor _windowResizeEastWestCursor] : [NSCursor resizeLeftRightCursor];
    g_MouseCursors[ImGuiMouseCursor_ResizeNESW] = [NSCursor respondsToSelector:@selector(_windowResizeNorthEastSouthWestCursor)] ? [NSCursor _windowResizeNorthEastSouthWestCursor] : [NSCursor closedHandCursor];
    g_MouseCursors[ImGuiMouseCursor_ResizeNWSE] = [NSCursor respondsToSelector:@selector(_windowResizeNorthWestSouthEastCursor)] ? [NSCursor _windowResizeNorthWestSouthEastCursor] : [NSCursor closedHandCursor];

    // Note that imgui.cpp also include default OSX clipboard handlers which can be enabled
    // by adding '#define IMGUI_ENABLE_OSX_DEFAULT_CLIPBOARD_FUNCTIONS' in imconfig.h and adding '-framework ApplicationServices' to your linker command-line.
    // Since we are already in ObjC land here, it is easy for us to add a clipboard handler using the NSPasteboard api.
    io.SetClipboardTextFn = [](void *, const char *str) -> void {
        NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard declareTypes:[NSArray arrayWithObject:NSPasteboardTypeString] owner:nil];
        [pasteboard setString:[NSString stringWithUTF8String:str] forType:NSPasteboardTypeString];
    };

    io.GetClipboardTextFn = [](void *) -> const char * {
        NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
        NSString *available = [pasteboard availableTypeFromArray: [NSArray arrayWithObject:NSPasteboardTypeString]];
        if (![available isEqualToString:NSPasteboardTypeString]) {
            return NULL;
        }

        NSString *string = [pasteboard stringForType:NSPasteboardTypeString];
        if (string == nil) {
            return NULL;
        }

        const char *string_c = (const char*)[string UTF8String];
        size_t string_len = strlen(string_c);
        static ImVector<char> s_clipboard;
        s_clipboard.resize((int)string_len + 1);
        strcpy(s_clipboard.Data, string_c);
        return s_clipboard.Data;
    };

    ImGui_ImplOpenGL_Init("#version 150");
}

void OpenGLRHI::ImGuiDestroyContext(GLContext *ctx) {
    ImGui_ImplOpenGL_Shutdown();

    ImGui::DestroyContext(ctx->imGuiContext);
}

void OpenGLRHI::ImGuiBeginFrame(Handle ctxHandle) {
    BE_SCOPE_PROFILE_CPU("OpenGLRHI::ImGuiBeginFrame");

    ImGui_ImplOpenGL_ValidateFrame();

    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    DisplayMetrics dm;
    GetDisplayMetrics(ctxHandle, &dm);

    // Setup display size
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2(dm.screenWidth, dm.screenHeight);
    io.DisplayFramebufferScale = ImVec2((float)dm.backingWidth / dm.screenWidth, (float)dm.backingHeight / dm.screenHeight);

    // Setup time step
    double currentTime = PlatformTime::Seconds();
    io.DeltaTime = currentTime - ctx->imGuiLastTime;
    ctx->imGuiLastTime = currentTime;

    ImGui_ImplOSX_UpdateMouseCursor();
    
    ImGui::NewFrame();
}

void OpenGLRHI::ImGuiRender() {
    BE_SCOPE_PROFILE_CPU("OpenGLRHI::ImGuiRender");
    BE_SCOPE_PROFILE_GPU("OpenGLRHI::ImGuiRender");

    ImGui::Render();

    bool sRGBWriteEnabled = OpenGL::SupportsFrameBufferSRGB() && IsSRGBWriteEnabled();
    if (sRGBWriteEnabled) {
        SetSRGBWrite(false);
    }

    ImGui_ImplOpenGL_RenderDrawData(ImGui::GetDrawData());

    if (sRGBWriteEnabled) {
        SetSRGBWrite(true);
    }
}

void OpenGLRHI::ImGuiEndFrame() {
    BE_SCOPE_PROFILE_CPU("OpenGLRHI::ImGuiEndFrame");

    ImGui::EndFrame();
}

BE_NAMESPACE_END
