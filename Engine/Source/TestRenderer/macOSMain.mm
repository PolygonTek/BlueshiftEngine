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
#include "Application.h"

//#define CREATE_SUB_WINDOW
#define USE_SHARED_CONTEXT      true

@interface MyWindow : NSWindow

@property (nonatomic, assign) BE1::Renderer::Handle context;
@property (nonatomic, assign) BE1::Renderer::Handle renderTarget;

@end

@implementation MyWindow

- (void)moveToCenter {
    NSRect mainDisplayRect = [[NSScreen mainScreen] frame];
    
    NSRect windowRect = [self frame];
    
    NSPoint newPos = NSMakePoint(MAX(0, (mainDisplayRect.size.width - windowRect.size.width) / 2),
                                 MAX(0, (mainDisplayRect.size.height - windowRect.size.height) / 2));
    
    [self setFrameOrigin:newPos];
}

- (void)cascade {
    static NSPoint cascadePos = NSMakePoint(0, 0);
    
    if (cascadePos.x == 0 && cascadePos.y == 0) {
        [self moveToCenter];
    }
    
    cascadePos = [self cascadeTopLeftFromPoint:cascadePos];
}

@end

//---------------------------------------------------------------------------------------

__strong MyWindow *mainWindow;
__strong MyWindow *subWindow;

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate> {
}

@end

@implementation AppDelegate

- (void)processEvent:(NSEvent *)event {
    NSEventType eventType = [event type];
    
    switch (eventType) {
        case NSKeyDown: {
            unichar ch = [[event charactersIgnoringModifiers] characterAtIndex:0];
            if (ch == 27) {
                break;
            }
            break;
        }
        case NSMouseMoved: {
            //NSPoint location = [event locationInWindow];
            break;
        }
        default:
            break;
    }
    
    [NSApp sendEvent: event];
}

- (MyWindow *)createGLWindow:(NSSize)size title:(NSString *)title sharedContext:(bool)shared displayFunc:(BE1::Renderer::DisplayContextFunc)displayFunc {
    NSUInteger styleMask = NSTitledWindowMask | NSClosableWindowMask | NSResizableWindowMask;
    
    NSRect contentRect = NSMakeRect(0, 0, size.width, size.height);
    
    MyWindow *window = [[MyWindow alloc] initWithContentRect:contentRect
                                                   styleMask:styleMask
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    
    [window setDelegate:self];
    
    [window setTitle:title];
    [window setBackgroundColor:[NSColor grayColor]];
    [window setOpaque:YES];
    [window setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces | NSWindowCollectionBehaviorFullScreenPrimary];
    
    [window cascade];
    
    NSView *contentView = [window contentView];
    
    window.context = BE1::glr.CreateContext((__bridge BE1::Renderer::WindowHandle)contentView, shared);
    
    BE1::glr.SetContextDisplayFunc(window.context, displayFunc, NULL, true);
    
    [window makeKeyAndOrderFront:nil];
    
    [window setReleasedWhenClosed:NO];
    
    return window;
}

static void SystemLog(int logLevel, const wchar_t *msg) {
    NSString *nsmsg = (__bridge NSString *)WideStringToCFString(msg);
    NSLog(@"%@", nsmsg);
}

static void SystemError(int errLevel, const wchar_t *msg) {
    NSString *nsmsg = (__bridge NSString *)WideStringToCFString(msg);
    
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:[NSString stringWithUTF8String:"Error"]];
    [alert setInformativeText:nsmsg];
    [alert setAlertStyle:errLevel == BE1::FatalErr ? NSCriticalAlertStyle : NSWarningAlertStyle];
    [alert runModal];
    
    if (errLevel == BE1::FatalErr) {
        exit(0);
    }
}

static void DisplayMainContext(BE1::Renderer::Handle context, void *dataPtr) {
    static float t0 = BE1::PlatformTime::Milliseconds() / 1000.0f;
    float t = BE1::PlatformTime::Milliseconds() / 1000.0f - t0;
    
    ::app.Draw(context, mainWindow.renderTarget, t);
}

#ifdef CREATE_SUB_WINDOW
static void DisplaySubContext(BE1::Renderer::Handle context, void *dataPtr) {
    static float t0 = BE1::PlatformTime::Milliseconds() / 1000.0f;
    float t = BE1::PlatformTime::Milliseconds() / 1000.0f - t0;
    
    t = -t;
    
    ::app.Draw(context, subWindow.renderTarget, t);
}
#endif

- (void)initInstance {
    BE1::Str enginePath = BE1::PlatformFile::ExecutablePath();
    enginePath.AppendPath("../../.."); // Strip "Bin/macOS/<Configuration>"
    enginePath.CleanPath();
    BE1::Engine::InitBase(enginePath, false, SystemLog, SystemError);
    
    ::app.Init();
    
    ::app.LoadResources();
    
    mainWindow = [self createGLWindow:NSMakeSize(640, 480) title:@"Main Window" sharedContext:USE_SHARED_CONTEXT displayFunc:DisplayMainContext];
    
    // FBO cannot be shared, so we should create FBO for each context
    mainWindow.renderTarget = ::app.CreateRenderTarget(mainWindow.context);

#ifdef CREATE_SUB_WINDOW
    subWindow = [self createGLWindow:NSMakeSize(320, 240) title:@"Sub Window" sharedContext:USE_SHARED_CONTEXT displayFunc:DisplaySubContext];

    subWindow.renderTarget = ::app.CreateRenderTarget(subWindow.context);
#endif
}

- (void)shutdownInstance {
    ::app.FreeResources();
    
    if (mainWindow.context) {
        BE1::glr.DeleteRenderTarget(mainWindow.renderTarget);
        BE1::glr.DestroyContext(mainWindow.context);
        mainWindow.context = BE1::Renderer::NullContext;
    }
   
#ifdef CREATE_SUB_WINDOW
    if (subWindow.context) {
        BE1::glr.DeleteRenderTarget(subWindow.renderTarget);
        BE1::glr.DestroyContext(subWindow.context);
        subWindow.context = BE1::Renderer::NullContext;
    }
#endif
    
    ::app.Shutdown();
    
    BE1::Engine::ShutdownBase();
}

- (void)windowWillClose:(NSNotification *)notification {
    MyWindow *window = [notification object];
    
    if (window.context) {
        BE1::glr.DeleteRenderTarget(window.renderTarget);
        BE1::glr.DestroyContext(window.context);
        window.context = BE1::Renderer::NullContext;
    }
}

- (void)windowWillEnterFullScreen:(NSNotification *)notification {
    MyWindow *window = [notification object];
    
    // set window is resizable to make fullscreen window
    //NSInteger oldStyleMask = [window styleMask];
    //[window setStyleMask:oldStyleMask | NSResizableWindowMask];
    
    NSSize size = [[window contentView] frame].size;
    
    BE1::glr.SetFullscreen(window.context, size.width, size.height);
}

- (void)windowWillExitFullScreen:(NSNotification *)notification {
    MyWindow *window = [notification object];
    
    // set window is non-resizable not to allow resizable window
    //NSInteger oldStyleMask = [window styleMask];
    //[window setStyleMask:oldStyleMask & ~NSResizableWindowMask];
    
    BE1::glr.ResetFullscreen(window.context);
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    [self initInstance];
    
    while (1) {
        @autoreleasepool {
            while (NSEvent *event = [NSApp nextEventMatchingMask: NSAnyEventMask
                                                       untilDate: nil
                                                          inMode: NSDefaultRunLoopMode
                                                         dequeue: YES]) {
                [self processEvent:event];
            }
        }
        
        ::app.RunFrame();
        
        if (mainWindow.context) {
            BE1::glr.DisplayContext(mainWindow.context);
        }
#ifdef CREATE_SUB_WINDOW
        if (subWindow.context) {
            BE1::glr.DisplayContext(subWindow.context);
        }
#endif
    }
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    [self shutdownInstance];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end

int main(int argc, const char *argv[]) {
    BE1::Str workingDir = argv[0];
    workingDir.StripFileName();
    workingDir.AppendPath("../../.."); // Strip "TestRenderer.app/Contents/MacOS"
    workingDir.CleanPath();
    chdir(workingDir.c_str());
    
    Class appDelegateClass = NSClassFromString(@"AppDelegate");
    id appDelegate = [[appDelegateClass alloc] init];
    [[NSApplication sharedApplication] setDelegate:appDelegate];
    
    return NSApplicationMain(argc, argv);
}
