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
#ifdef USE_ADMOB_REWARD_BASED_VIDEO
#include "iOSAdMob.h"
#endif

#define SuppressPerformSelectorLeakWarning(stuff) \
    do { \
        _Pragma("clang diagnostic push") \
        _Pragma("clang diagnostic ignored \"-Warc-performSelector-leaks\"") \
        stuff; \
        _Pragma("clang diagnostic pop") \
    } while (0)

#ifdef USE_ADMOB_REWARD_BASED_VIDEO
@interface RootViewController : UIViewController<GADRewardBasedVideoAdDelegate> {
#else
@interface RootViewController : UIViewController {
#endif
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
    if (elapsedMsec > 1000) {
        elapsedMsec = 1000;
    }

    t0 = t;
    
    BE1::Engine::RunFrame(elapsedMsec);
    
    BE1::gameClient.RunFrame();
    
    app.Update();
    
    BE1::gameClient.EndFrame();
    
    app.Draw();
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

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
    [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];

    // This is basically an animation block
    [coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context) {

        // Get the new orientation if you want
        UIInterfaceOrientation orientation = [[UIApplication sharedApplication] statusBarOrientation];

        // Adjust your views
        float nativeScale = [[UIScreen mainScreen] nativeScale];
        app.mainRenderContext->OnResize(size.width * nativeScale, size.height * nativeScale);

    } completion:^(id<UIViewControllerTransitionCoordinatorContext> context) {
        // Anything else you need to do at the end
    }];
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
        
        BE1::platform->QueEvent(BE1::Platform::KeyEvent, BE1::KeyCode::Mouse1, true, 0, nullptr);
        BE1::platform->QueEvent(BE1::Platform::MouseMoveEvent, location.x, location.y, 0, nullptr);
        BE1::platform->QueEvent(BE1::Platform::TouchBeganEvent, touchId, locationQword, 0, nullptr);
        
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
        
        BE1::platform->QueEvent(BE1::Platform::MouseMoveEvent, location.x, location.y, 0, nullptr);
        BE1::platform->QueEvent(BE1::Platform::TouchMovedEvent, touchId, locationQword, 0, nullptr);
        
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
        
        BE1::platform->QueEvent(BE1::Platform::KeyEvent, BE1::KeyCode::Mouse1, false, 0, nullptr);
        BE1::platform->QueEvent(BE1::Platform::TouchEndedEvent, touchId, locationQword, 0, nullptr);
        
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
        
        BE1::platform->QueEvent(BE1::Platform::KeyEvent, BE1::KeyCode::Mouse1, false, 0, nullptr);
        BE1::platform->QueEvent(BE1::Platform::TouchCanceledEvent, touchId, 0, 0, nullptr);
        
        touch = [enumerator nextObject];
    }
}

#ifdef USE_ADMOB_BANNER

/// Tells the delegate an ad request loaded an ad.
- (void)adViewDidReceiveAd:(GADBannerView *)adView {
}

/// Tells the delegate an ad request failed.
- (void)adView:(GADBannerView *)adView didFailToReceiveAdWithError:(GADRequestError *)error {
    const char *errorDescription = (const char *)[error.localizedDescription cStringUsingEncoding:NSUTF8StringEncoding];    
}

/// Tells the delegate that a full-screen view will be presented in response
/// to the user clicking on an ad.
- (void)adViewWillPresentScreen:(GADBannerView *)adView {
}

/// Tells the delegate that the full-screen view will be dismissed.
- (void)adViewWillDismissScreen:(GADBannerView *)adView {
}

/// Tells the delegate that the full-screen view has been dismissed.
- (void)adViewDidDismissScreen:(GADBannerView *)adView {
}

/// Tells the delegate that a user click will open another app (such as
/// the App Store), backgrounding the current app.
- (void)adViewWillLeaveApplication:(GADBannerView *)adView {
}

#endif

#ifdef USE_ADMOB_REWARD_BASED_VIDEO

- (void)rewardBasedVideoAd:(GADRewardBasedVideoAd *)rewardBasedVideoAd didRewardUserWithReward:(GADAdReward *)reward {
    const char *rewardType = (const char *)[reward.type cStringUsingEncoding:NSUTF8StringEncoding];
    int rewardAmount = [reward.amount intValue];
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["did_reward_user"];
    if (function.IsFunction()) {
       function(rewardType, rewardAmount);
    }
}

- (void)rewardBasedVideoAdDidReceiveAd:(GADRewardBasedVideoAd *)rewardBasedVideoAd {
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["did_receive_ad"];
    if (function.IsFunction()) {
        function();
    }
}

- (void)rewardBasedVideoAdDidOpen:(GADRewardBasedVideoAd *)rewardBasedVideoAd {
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["did_open"];
    if (function.IsFunction()) {
        function();
    }
    
    [self stopAnimation];
}

- (void)rewardBasedVideoAdDidStartPlaying:(GADRewardBasedVideoAd *)rewardBasedVideoAd {
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["did_start_playing"];
    if (function.IsFunction()) {
        function();
    }
}

- (void)rewardBasedVideoAdDidClose:(GADRewardBasedVideoAd *)rewardBasedVideoAd {
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["did_close"];
    if (function.IsFunction()) {
        function();
    }
    
    [self startAnimation];
}

- (void)rewardBasedVideoAdWillLeaveApplication:(GADRewardBasedVideoAd *)rewardBasedVideoAd {
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["will_leave_application"];
    if (function.IsFunction()) {
        function();
    }
}

- (void)rewardBasedVideoAd:(GADRewardBasedVideoAd *)rewardBasedVideoAd didFailToLoadWithError:(NSError *)error {
    const char *errorDescription = (const char *)[error.description cStringUsingEncoding:NSUTF8StringEncoding];
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["did_fail_to_load"];
    if (function.IsFunction()) {
        function(errorDescription);
    }
}

#endif

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
    BE1::IOSDevice::Type deviceType = BE1::IOSDevice::GetIOSDeviceType();
    if (deviceType == BE1::IOSDevice::IOS_Unknown) {
        assert(0);
    }
    
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
    
    BE1::gameClient.Init((__bridge BE1::RHI::WindowHandle)mainWindow, false);
    
    float retinaScale = [[UIScreen mainScreen] scale];
    int renderWidth = screenBounds.size.width * retinaScale;
    int renderHeight = screenBounds.size.height * retinaScale;
    
    BE1::Vec2 screenScaleFactor(1.0f, 1.0f);
    int deviceWidth;
    int deviceHeight;
    if (BE1::IOSDevice::IsIPad(deviceType)) {
        if (deviceType < BE1::IOSDevice::IOS_IPadAir2) {
            screenScaleFactor.x = BE1::Min(1280.0f / renderWidth, 1.0f);
            screenScaleFactor.y = BE1::Min(720.0f / renderHeight, 1.0f);
        } else {
            screenScaleFactor.x = BE1::Min(2048.0f / renderWidth, 1.0f);
            screenScaleFactor.y = BE1::Min(1536.0f / renderHeight, 1.0f);
        }
    }
    renderWidth = renderWidth * screenScaleFactor.x;
    renderHeight = renderHeight * screenScaleFactor.y;
    
    app.mainRenderContext = BE1::renderSystem.AllocRenderContext(true);
    app.mainRenderContext->Init((__bridge BE1::RHI::WindowHandle)[rootViewController view],
                                renderWidth, renderHeight, DisplayContext, nullptr);
    
    app.Init();
    
#ifdef USE_ADMOB_REWARD_BASED_VIDEO
    RewardBasedVideoAd::RegisterLuaModule(&app.gameWorld->GetLuaVM().State(), rootViewController);
#endif
    
    app.LoadAppScript("Application");
    
    app.StartAppScript();
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

