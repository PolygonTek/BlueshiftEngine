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
#define USE_SHARED_CONTEXT      false

@interface MyWindow : NSWindow

@property (nonatomic, assign) BE1::RHI::Handle context;
@property (nonatomic, assign) BE1::RHI::Handle renderTarget;

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

static __strong MyWindow *  mainWindow;
static __strong MyWindow *  subWindow;

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

- (MyWindow *)createGLWindow:(NSSize)size title:(NSString *)title {
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

static void DisplayMainContext(BE1::RHI::Handle context, void *dataPtr) {
    static float t0 = BE1::PlatformTime::Milliseconds() / 1000.0f;
    float t = BE1::PlatformTime::Milliseconds() / 1000.0f - t0;
    
    ::app.Draw(context, mainWindow.renderTarget, t);
}

#ifdef CREATE_SUB_WINDOW
static void DisplaySubContext(BE1::RHI::Handle context, void *dataPtr) {
    static float t0 = BE1::PlatformTime::Milliseconds() / 1000.0f;
    float t = BE1::PlatformTime::Milliseconds() / 1000.0f - t0;
    
    t = -t;
    
    ::app.Draw(context, subWindow.renderTarget, t);
}
#endif

- (void)initInstance {
    BE1::Str basePath = BE1::PlatformFile::ExecutablePath();
    basePath.AppendPath("../../.."); // Strip "Bin/macOS/<Configuration>"
    basePath.CleanPath();
    BE1::Engine::InitBase(basePath, false, SystemLog, SystemError);
    
    mainWindow = [self createGLWindow:NSMakeSize(640, 480) title:@"Main Window"];
    NSView *mainContentView = [mainWindow contentView];

    ::app.Init((__bridge BE1::RHI::WindowHandle)mainContentView);
    
    ::app.LoadResources();
    
    mainWindow.context = BE1::rhi.CreateContext((__bridge BE1::RHI::WindowHandle)mainContentView, USE_SHARED_CONTEXT);    
    BE1::rhi.SetContextDisplayFunc(mainWindow.context, DisplayMainContext, NULL, true);
    
    // FBO cannot be shared, so we should create FBO for each context
    mainWindow.renderTarget = ::app.CreateRenderTarget(mainWindow.context);

#ifdef CREATE_SUB_WINDOW
    subWindow = [self createGLWindow:NSMakeSize(320, 240) title:@"Sub Window"];
    NSView *subContentView = [subWindow contentView];

    subWindow.context = BE1::rhi.CreateContext((__bridge BE1::RHI::WindowHandle)subContentView, USE_SHARED_CONTEXT);    
    BE1::rhi.SetContextDisplayFunc(subWindow.context, DisplaySubContext, NULL, true);

    subWindow.renderTarget = ::app.CreateRenderTarget(subWindow.context);
#endif
}

- (void)shutdownInstance {
    ::app.FreeResources();
    
    if (mainWindow.context) {
        BE1::rhi.DeleteRenderTarget(mainWindow.renderTarget);
        BE1::rhi.DestroyContext(mainWindow.context);
        mainWindow.context = BE1::RHI::NullContext;
    }
   
#ifdef CREATE_SUB_WINDOW
    if (subWindow.context) {
        BE1::rhi.DeleteRenderTarget(subWindow.renderTarget);
        BE1::rhi.DestroyContext(subWindow.context);
        subWindow.context = BE1::RHI::NullContext;
    }
#endif
    
    ::app.Shutdown();
    
    BE1::Engine::ShutdownBase();
}

- (void)windowWillClose:(NSNotification *)notification {
    MyWindow *window = [notification object];
    
    if (window.context) {
        BE1::rhi.DeleteRenderTarget(window.renderTarget);
        BE1::rhi.DestroyContext(window.context);
        window.context = BE1::RHI::NullContext;
    }
}

- (void)windowWillEnterFullScreen:(NSNotification *)notification {
    MyWindow *window = [notification object];
    
    // set window is resizable to make fullscreen window
    //NSInteger oldStyleMask = [window styleMask];
    //[window setStyleMask:oldStyleMask | NSResizableWindowMask];
    
    NSSize size = [[window contentView] frame].size;
    
    BE1::rhi.SetFullscreen(window.context, size.width, size.height);
}

- (void)windowWillExitFullScreen:(NSNotification *)notification {
    MyWindow *window = [notification object];
    
    // set window is non-resizable not to allow resizable window
    //NSInteger oldStyleMask = [window styleMask];
    //[window setStyleMask:oldStyleMask & ~NSResizableWindowMask];
    
    BE1::rhi.ResetFullscreen(window.context);
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
            BE1::rhi.DisplayContext(mainWindow.context);
        }
#ifdef CREATE_SUB_WINDOW
        if (subWindow.context) {
            BE1::rhi.DisplayContext(subWindow.context);
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
