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
#include <sys/types.h>
#include <sys/sysctl.h>
#include <AudioToolbox/AudioToolbox.h>
#include <AVFoundation/AVAudioSession.h>

#define SuppressPerformSelectorLeakWarning(Stuff) \
    do { \
        _Pragma("clang diagnostic push") \
        _Pragma("clang diagnostic ignored \"-Warc-performSelector-leaks\"") \
        Stuff; \
        _Pragma("clang diagnostic pop") \
    } while (0)

enum IOSDevice {
    IOS_IPhone4,
    IOS_IPhone4S,
    IOS_IPhone5, // also the iPhone5c
    IOS_IPhone5S,
    IOS_IPhone6,
    IOS_IPhone6S,
    IOS_IPhone7,
    IOS_IPodTouch4,
    IOS_IPodTouch5,
    IOS_IPodTouch6,
    IOS_IPad2,
    IOS_IPadMini,
    IOS_IPad3,
    IOS_IPad4,
    IOS_IPadAir, // also the IPad Mini Retina
    IOS_IPadMini2,
    IOS_IPadMini3,
    IOS_IPadAir2,
    IOS_IPadMini4,
    IOS_IPadPro_9_7, // iPad Pro 9.7 inch
    IOS_IPadPro_12_9, // iPad Pro 12.9 inch
    IOS_Unknown,
};

static bool IsIPhone(IOSDevice deviceType) {
    return deviceType >= IOS_IPhone4 && deviceType <= IOS_IPhone7;
}

static bool IsIPod(IOSDevice deviceType) {
    return deviceType >= IOS_IPodTouch4 && deviceType <= IOS_IPodTouch6;
}

static bool IsIPad(IOSDevice deviceType) {
    return deviceType >= IOS_IPad2 && deviceType <= IOS_IPadPro_9_7;
}

static IOSDevice GetIOSDeviceType() {
    // default to unknown
    static IOSDevice deviceType = IOS_Unknown;
    
    // if we've already figured it out, return it
    if (deviceType != IOS_Unknown) {
        return deviceType;
    }
    
    // get the device hardware type string length
    size_t deviceIDLen;
    sysctlbyname("hw.machine", NULL, &deviceIDLen, NULL, 0);
    
    // get the device hardware type
    char *deviceID = (char *)malloc(deviceIDLen);
    sysctlbyname("hw.machine", deviceID, &deviceIDLen, NULL, 0);
    
    // convert to NSString
    NSString *deviceIDString = [NSString stringWithCString:deviceID encoding:NSUTF8StringEncoding];
    free(deviceID);
    
    if ([deviceIDString hasPrefix:@"iPod"]) {
        // get major revision number
        int major = [deviceIDString characterAtIndex:4] - '0';
        
        if (major == 4) {
            deviceType = IOS_IPodTouch4;
        } else if (major == 5) {
            deviceType = IOS_IPodTouch5;
        } else if (major >= 7) {
            deviceType = IOS_IPodTouch6;
        }
    } else if ([deviceIDString hasPrefix:@"iPad"]) {
        // get major revision number
        int major = [deviceIDString characterAtIndex:4] - '0';
        int minor = [deviceIDString characterAtIndex:6] - '0';
        
        if (major == 2) {
            if (minor >= 5) {
                deviceType = IOS_IPadMini;
            } else {
                deviceType = IOS_IPad2;
            }
        } else if (major == 3) {
            if (minor <= 3) {
                deviceType = IOS_IPad3;
            } else if (minor >= 4) {
                deviceType = IOS_IPad4;
            }
        } else if (major == 4) {
            if (minor >= 8) {
                deviceType = IOS_IPadMini3;
            } else if (minor >= 4) {
                deviceType = IOS_IPadMini2;
            } else {
                deviceType = IOS_IPadAir;
            }
        } else if (major == 5) {
            if (minor >= 3) {
                deviceType = IOS_IPadAir2;
            } else {
                deviceType = IOS_IPadMini4;
            }
        } else if (major >= 6) { // Default to highest settings currently available for any future device
            if (minor >= 7) {
                deviceType = IOS_IPadPro_12_9;
            } else {
                deviceType = IOS_IPadPro_9_7;
            }
        }
    } else if ([deviceIDString hasPrefix:@"iPhone"]) {
        int major = [deviceIDString characterAtIndex:6] - '0';
        
        if (major == 3) {
            deviceType = IOS_IPhone4;
        } else if (major == 4) {
            deviceType = IOS_IPhone4S;
        } else if (major == 5) {
            deviceType = IOS_IPhone5;
        } else if (major == 6) {
            deviceType = IOS_IPhone5S;
        } else if (major == 7) {
            deviceType = IOS_IPhone6;
        } else if (major == 8) {
            deviceType = IOS_IPhone6S;
        } else if (major >= 9) {
            deviceType = IOS_IPhone7;
        }
    } else if ([deviceIDString hasPrefix:@"x86"]) { // simulator
        if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone) {
            CGSize result = [[UIScreen mainScreen] bounds].size;
            if (result.height >= 586) {
                deviceType = IOS_IPhone5;
            } else {
                deviceType = IOS_IPhone4S;
            }
        } else {
            if ([[UIScreen mainScreen] scale] > 1.0f) {
                deviceType = IOS_IPad3;
            } else {
                deviceType = IOS_IPad2;
            }
        }
    }
    
    // if this is unknown at this point, we have a problem
    if (deviceType == IOS_Unknown) {
        BE_ERRLOG(L"This IOS device type is not supported by UE4 [%s]\n", BE1::WStr(deviceIDString).c_str());
    }
    
    return deviceType;
}

@interface RootViewController : UIViewController {
}

@property(nonatomic, weak) UIView *eaglView;

@end

@implementation RootViewController

static void DisplayContext(BE1::RHI::Handle context, void *dataPtr) {
    static int t0 = 0;
    
    if (t0 == 0) {
        t0 = BE1::PlatformTime::Milliseconds();
    }
    
    int t = BE1::PlatformTime::Milliseconds();
    int elapsedMsec = t - t0;

    BE1::Engine::RunFrame(elapsedMsec);
    
    BE1::gameClient.RunFrame();
    
    app.Update();
    
    BE1::gameClient.EndFrame();
    
    app.Draw();
    
    t0 = t;
}

- (void)loadView {
    CGRect frame = [[UIScreen mainScreen] bounds];
    
    self.view = [[UIView alloc] initWithFrame:frame];
    self.view.opaque = YES;
    self.view.clearsContextBeforeDrawing = NO;
}

// Notifies that its view was added to a view hierarchy.
- (void)viewDidAppear:(BOOL)animated {
    _eaglView = [self.view subviews][0]; // EAGLView
    _eaglView.multipleTouchEnabled = YES;
}

// Notifies that its view is about to be removed from a view hierarchy.
- (void)viewWillDisappear:(BOOL)animated {
}

/*- (NSUInteger)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskLandscape;
}

- (UIInterfaceOrientation)preferredInterfaceOrientationForPresentation {
    return UIInterfaceOrientationLandscapeRight;
}*/

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
    [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
    //float nativeScale = [[UIScreen mainScreen] nativeScale];
    //game.mainRenderContext->OnResize(size.width * nativeScale, size.height * nativeScale);
}

- (BOOL)prefersStatusBarHidden {
    return YES;
}

- (void)startAnimation {
    SuppressPerformSelectorLeakWarning([_eaglView performSelector:NSSelectorFromString(@"startDisplayLink")]);
}

- (void)stopAnimation {
    SuppressPerformSelectorLeakWarning([_eaglView performSelector:NSSelectorFromString(@"stopDisplayLink")]);
}

// Handles the start of a touch
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    NSEnumerator *enumerator = [touches objectEnumerator];
    UITouch *touch = [enumerator nextObject];
    
    while (touch) {
        CGPoint location = [touch locationInView:_eaglView];
        uint64_t touchId = [touch hash];
        uint64_t locationQword = BE1::MakeQWord((int)location.x, (int)location.y);
        
        BE1::platform->QueEvent(BE1::Platform::KeyEvent, BE1::KeyCode::Mouse1, true, 0, NULL);
        BE1::platform->QueEvent(BE1::Platform::MouseMoveEvent, location.x, location.y, 0, NULL);
        BE1::platform->QueEvent(BE1::Platform::TouchBeganEvent, touchId, locationQword, 0, NULL);

        touch = [enumerator nextObject];
    }
}

// Handles the continuation of a touch.
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
    NSEnumerator *enumerator = [touches objectEnumerator];
    UITouch *touch = [enumerator nextObject];
    
    while (touch) {
        CGPoint location = [touch locationInView:_eaglView];
        uint64_t touchId = [touch hash];
        uint64_t locationQword = BE1::MakeQWord((int)location.x, (int)location.y);

        BE1::platform->QueEvent(BE1::Platform::MouseMoveEvent, location.x, location.y, 0, NULL);
        BE1::platform->QueEvent(BE1::Platform::TouchMovedEvent, touchId, locationQword, 0, NULL);

        touch = [enumerator nextObject];
    }
}

// Handles the end of a touch event when the touch is a tap.
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
    NSEnumerator *enumerator = [touches objectEnumerator];
    UITouch *touch = [enumerator nextObject];
    
    while (touch) {
        CGPoint location = [touch locationInView:_eaglView];
        uint64_t touchId = [touch hash];
        uint64_t locationQword = BE1::MakeQWord((int)location.x, (int)location.y);

        BE1::platform->QueEvent(BE1::Platform::KeyEvent, BE1::KeyCode::Mouse1, false, 0, NULL);
        BE1::platform->QueEvent(BE1::Platform::TouchEndedEvent, touchId, locationQword, 0, NULL);

        touch = [enumerator nextObject];
    }
}

// Handles the end of a touch event.
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
    // If appropriate, add code necessary to save the state of the application.
    NSEnumerator *enumerator = [touches objectEnumerator];
    UITouch *touch = [enumerator nextObject];
    
    while (touch) {
        uint64_t touchId = [touch hash];
        
        BE1::platform->QueEvent(BE1::Platform::KeyEvent, BE1::KeyCode::Mouse1, false, 0, NULL);
        BE1::platform->QueEvent(BE1::Platform::TouchCanceledEvent, touchId, 0, 0, NULL);

        touch = [enumerator nextObject];
    }
}

@end // @implementation RootViewController

//---------------------------------------------------------------------------------------

@interface AppDelegate : NSObject <UIApplicationDelegate> {
    float                   osVersion;
    UIWindow *              mainWindow;
    RootViewController *    rootViewController;
}

@end

@implementation AppDelegate

- (void)initInstance {
    IOSDevice deviceType = GetIOSDeviceType();
    
    // ----- Core initialization -----
    BE1::Engine::InitParms initParms;
    
    BE1::Str appDir = BE1::PlatformFile::ExecutablePath();
    initParms.baseDir = appDir;
    
    BE1::Str dataDir = appDir + "/Data";
    initParms.searchPath = dataDir;
    
    BE1::Engine::Init(&initParms);
    // -------------------------------
    
    BE1::resourceGuidMapper.Read("Data/guidmap");
        
    // mainWindow(UIWindow) - rootViewController.view(UIView) - eaglView(EAGLView)
    CGRect screenBounds = [[UIScreen mainScreen] bounds];
    mainWindow = [[UIWindow alloc] initWithFrame:screenBounds];
    mainWindow.backgroundColor = [UIColor blackColor];
    
    rootViewController = [[RootViewController alloc] init];
    mainWindow.rootViewController = rootViewController;
    
    [mainWindow makeKeyAndVisible];

    BE1::gameClient.Init((__bridge BE1::RHI::WindowHandle)mainWindow, true);
        
    float retinaScale = [[UIScreen mainScreen] scale];
    int renderWidth = screenBounds.size.width * retinaScale;
    int renderHeight = screenBounds.size.height * retinaScale;
    
    BE1::Vec2 screenScaleFactor;
    if ((IsIPhone(deviceType) && deviceType >= IOS_IPhone6) ||
        (IsIPod(deviceType) && deviceType >= IOS_IPodTouch6) ||
        (IsIPad(deviceType) && deviceType >= IOS_IPadAir2)) {
        screenScaleFactor.x = BE1::Min(1280.0f / renderWidth, 1.0f);
        screenScaleFactor.y = BE1::Min(720.0f / renderHeight, 1.0f);
    } else {
        //
        screenScaleFactor.x = BE1::Min(1280.0f / renderWidth, 1.0f);
        screenScaleFactor.y = BE1::Min(720.0f / renderHeight, 1.0f);
    }
    renderWidth = renderWidth * screenScaleFactor.x;
    renderHeight = renderHeight * screenScaleFactor.y;
    
    app.mainRenderContext = BE1::renderSystem.AllocRenderContext(true);
    app.mainRenderContext->Init((__bridge BE1::RHI::WindowHandle)[rootViewController view],
                                 renderWidth, renderHeight, DisplayContext, NULL);    
    
    app.Init();
    
    //BE1::cmdSystem.BufferCommandText(BE1::CmdSystem::Append, L"exec \"autoexec.cfg\"\n");
}

- (void)shutdownInstance {
    app.Shutdown();

    app.mainRenderContext->Shutdown();
    
    BE1::renderSystem.FreeRenderContext(app.mainRenderContext);
    
    BE1::gameClient.Shutdown();
        
    BE1::Engine::Shutdown();
}

- (bool)IsBackgroundAudioPlaying {
    AVAudioSession *session = [AVAudioSession sharedInstance];
    return session.otherAudioPlaying;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    [self initInstance];
    
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application {
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
    [rootViewController stopAnimation];
    
    app.OnApplicationPause(true);
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
    [rootViewController stopAnimation];
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
    [rootViewController startAnimation];
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
    [rootViewController startAnimation];
    
    app.OnApplicationPause(false);
}

- (void)applicationWillTerminate:(UIApplication *)application {
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
    app.OnApplicationTerminate();
    
    [self shutdownInstance];
}

@end // @implementation AppDelegate


int main(int argc, char *argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}
