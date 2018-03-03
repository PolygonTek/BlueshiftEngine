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

#define SuppressPerformSelectorLeakWarning(Stuff) \
    do { \
        _Pragma("clang diagnostic push") \
        _Pragma("clang diagnostic ignored \"-Warc-performSelector-leaks\"") \
        Stuff; \
        _Pragma("clang diagnostic pop") \
    } while (0)

static BE1::RHI::Handle mainContext;
static BE1::RHI::Handle mainRenderTarget;

@interface RootViewController : UIViewController {
}

@property(nonatomic, weak) UIView *eaglView;

@end

@implementation RootViewController

- (void)loadView {
    CGRect frame = [[UIScreen mainScreen] bounds];
    
    self.view = [[UIView alloc] initWithFrame:frame];
    self.view.opaque = YES;
    self.view.clearsContextBeforeDrawing = NO;
}

static void DisplayContext(BE1::RHI::Handle context, void *dataPtr) {
    static float t0 = BE1::PlatformTime::Milliseconds() / 1000.0f;
    float t = BE1::PlatformTime::Milliseconds() / 1000.0f - t0;

    ::app.RunFrame(); //
    
    ::app.Draw(context, mainRenderTarget, t);
}

// Notifies that its view was added to a view hierarchy.
- (void)viewDidAppear:(BOOL)animated {
    mainContext = BE1::rhi.CreateContext((__bridge BE1::RHI::WindowHandle)self.view, false);
    
    _eaglView = [self.view subviews][0]; // EAGLView
    
    BE1::rhi.SetContextDisplayFunc(mainContext, DisplayContext, NULL, false);
}

// Notifies that its view is about to be removed from a view hierarchy.
- (void)viewWillDisappear:(BOOL)animated {
    BE1::rhi.DestroyContext(mainContext);
}

- (NSUInteger)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskAll;//UIInterfaceOrientationMaskLandscape;
}

- (UIInterfaceOrientation)preferredInterfaceOrientationForPresentation {
    return UIInterfaceOrientationPortrait;//UIInterfaceOrientationLandscapeRight;
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
    [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
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
    UITouch *touch = [[event touchesForView:_eaglView] anyObject];
    CGPoint location = [touch locationInView:_eaglView];
    NSLog(@"%.1f %.1f", location.x, location.y);
}

// Handles the continuation of a touch.
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
    UITouch *touch = [[event touchesForView:_eaglView] anyObject];
    CGPoint previousLocation = [touch previousLocationInView:_eaglView];
    CGPoint location = [touch locationInView:_eaglView];
    NSLog(@"%.1f %.1f ~ %.1f %.1f", previousLocation.x, previousLocation.y, location.x, location.y);
}

// Handles the end of a touch event when the touch is a tap.
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
}

// Handles the end of a touch event.
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
    // If appropriate, add code necessary to save the state of the application.
}

@end // @implementation RootViewController

@interface AppDelegate : UIResponder <UIApplicationDelegate> {
    float osVersion;
    UIWindow *mainWindow;
    RootViewController *rootViewController;
}

@end

@implementation AppDelegate

static void SystemLog(int logLevel, const wchar_t *msg) {
    NSString *nsmsg = [[NSString alloc] initWithBytes:msg
                                               length:wcslen(msg) * sizeof(*msg)
                                             encoding:NSUTF32LittleEndianStringEncoding];
    NSLog(@"%@", nsmsg);
}

static void SystemError(int errLevel, const wchar_t *msg) {
    NSString *nsmsg = [[NSString alloc] initWithBytes:msg
                                               length:wcslen(msg) * sizeof(*msg)
                                             encoding:NSUTF32LittleEndianStringEncoding];
    
    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Error"
                                                    message:nsmsg
                                                   delegate:nil
                                          cancelButtonTitle:@"OK"
                                          otherButtonTitles: nil];
    [alert show];
    
    if (errLevel == BE1::FatalErr) {
        exit(0);
    }
}

- (void)initInstance {
    BE1::Str basePath = BE1::PlatformFile::ExecutablePath(); // main bundle path
    BE1::Engine::InitBase(basePath, false, SystemLog, SystemError);
    
    osVersion = [[[UIDevice currentDevice] systemVersion] floatValue];
       
    CGRect screenBounds = [[UIScreen mainScreen] bounds];
    mainWindow = [[UIWindow alloc] initWithFrame:screenBounds];
    mainWindow.backgroundColor = [UIColor blackColor];
    
    rootViewController = [[RootViewController alloc] init];
    mainWindow.rootViewController = rootViewController;
    
    [mainWindow makeKeyAndVisible];

    ::app.Init((__bridge BE1::RHI::WindowHandle)[rootViewController view]);
    
    ::app.LoadResources();
    
    mainRenderTarget = ::app.CreateRenderTarget(mainContext);
}

- (void)shutdownInstance {
    BE1::rhi.DeleteRenderTarget(mainRenderTarget);
    
    ::app.FreeResources();
    
    ::app.Shutdown();
    
    BE1::Engine::ShutdownBase();
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    [self initInstance];
    
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application {
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
    [rootViewController stopAnimation];
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
}

- (void)applicationWillTerminate:(UIApplication *)application {
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
    [self shutdownInstance];
}

@end // @implementation AppDelegate

int main(int argc, char *argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}
