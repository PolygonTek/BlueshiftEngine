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
#include "iOSAdMob.h"

AdMob::BannerAd AdMob::bannderAd;
AdMob::InterstitialAd AdMob::interstitialAd;
AdMob::RewardBasedVideoAd AdMob::rewardBasedVideoAd;

void AdMob::RegisterLuaModule(LuaCpp::State *state, UIViewController<GADBannerViewDelegate, GADInterstitialDelegate, GADRewardBasedVideoAdDelegate> *viewController) {
    AdMob::viewController = viewController;

    state->RegisterModule("admob", [](LuaCpp::Module &module) {
        module["init"].SetFunc(AdMob::Init);

        LuaCpp::Selector _BannerAd = module["BannerAd"];

        _BannerAd.SetObj(bannerAd);
        _BannerAd.AddObjMembers(bannerAd,
            "init", &BannerAd::Init,
            "request", &BannerAd::Request,
            "show", &BannerAd::Show,
            "hide", &BannerAd::Hide);

        LuaCpp::Selector _InterstitialAd = module["InterstitialAd"];
        
        _InterstitialAd.SetObj(interstitialAd);
        _InterstitialAd.AddObjMembers(interstitialAd,
            "init", &InterstitialAd::Init,
            "request", &InterstitialAd::Request,
            "is_ready", &InterstitialAd::IsReady,
            "present", &InterstitialAd::Present);

        LuaCpp::Selector _RewardBasedVideoAd = module["RewardBasedVideoAd"];
        
        _RewardBasedVideoAd.SetObj(rewardBasedVideoAd);
        _RewardBasedVideoAd.AddObjMembers(rewardBasedVideoAd,
            "init", &RewardBasedVideoAd::Init,
            "request", &RewardBasedVideoAd::Request,
            "is_ready", &RewardBasedVideoAd::IsReady,
            "present", &RewardBasedVideoAd::Present);
    });
}

void AdMob::Init(const char *appID) {
    // Initialize Google mobile ads
    NSString *nsAppID = [[NSString alloc] initWithBytes:appID length:strlen(appID) encoding:NSUTF8StringEncoding];
    [GADMobileAds configureWithApplicationID:nsAppID];
}

//-------------------------------------------------------------------------------------------------

void AdMob::BannerAd::Init() {
}

void AdMob::BannerAd::Request(const char *unitID, const char *testDevices) {
    GADRequest *request = [GADRequest request];

    BE1::StrArray testDeviceList;
    BE1::SplitStringIntoList(testDeviceList, testDevices, " ");

    if (testDeviceList.Count() > 0) {
        NSString *nsTestDevices[256];
        for (int i = 0; i < testDeviceList.Count(); i++) {
            nsTestDevices[i] = [[NSString alloc] initWithBytes:testDeviceList[i].c_str() length:testDeviceList[i].Length() encoding:NSUTF8StringEncoding];
        }
    
        request.testDevices = [NSArray arrayWithObjects:nsTestDevices count:testDeviceList.Count()];
    }
    
    if (!unitID || !unitID[0]) {
        // ad unit of interstitial provided by Google for testing purposes.
        unitID = "ca-app-pub-3940256099942544/2934735716";
    }

    NSString *nsUnitID = [[NSString alloc] initWithBytes:unitID length:strlen(unitID) encoding:NSUTF8StringEncoding];

    //GADAdSize size = GADAdSizeFromCGSize(CGSizeMake(320, 50));
    bannerView = [[GADBannerView alloc] initWithAdSize:kGADAdSizeBanner]; // set ad size.
    bannerView.translatesAutoresizingMaskIntoConstraints = NO;
    bannerView.adUnitID = nsUnitID;

    // This view controller is used to present an overlay when the ad is clicked. 
    // It should normally be set to the view controller that contains the GADBannerView.
    bannerView.rootViewController = AdMob::viewController;

    bannerView.delegate = AdMob::viewController;

    [bannerView loadRequest:request];
}

void AdMob::BannerAd::Show(bool showOnBottomOfScreen, int offsetX, int offsetY) {
    if (bannerView.superview) {
        return;
    }

    UIView *view = AdMob::viewController.view;

    [view addSubview:bannerView];

    // Position the banner. Stick it to the bottom of the Safe Area.
    // Make it constrained to the edges of the safe area.
    UILayoutGuide *guide = view.safeAreaLayoutGuide;

    [NSLayoutConstraint activateConstraints:@[
        [guide.leftAnchor constraintEqualToAnchor:bannerView.leftAnchor],
        [guide.rightAnchor constraintEqualToAnchor:bannerView.rightAnchor],
        [guide.bottomAnchor constraintEqualToAnchor:bannerView.bottomAnchor]
    ]];
}

void AdMob::BannerAd::Hide() {
    if (bannerView.superview) {
        [bannerView removeFromSuperview];
    }
}

//-------------------------------------------------------------------------------------------------

void AdMob::InterstitialAd::Init() {
}

void AdMob::InterstitialAd::Request(const char *unitID, const char *testDevices) {
    GADRequest *request = [GADRequest request];

    BE1::StrArray testDeviceList;
    BE1::SplitStringIntoList(testDeviceList, testDevices, " ");

    if (testDeviceList.Count() > 0) {
        NSString *nsTestDevices[256];
        for (int i = 0; i < testDeviceList.Count(); i++) {
            nsTestDevices[i] = [[NSString alloc] initWithBytes:testDeviceList[i].c_str() length:testDeviceList[i].Length() encoding:NSUTF8StringEncoding];
        }
    
        request.testDevices = [NSArray arrayWithObjects:nsTestDevices count:testDeviceList.Count()];
    }
    
    if (!unitID || !unitID[0]) {
        // ad unit of interstitial provided by Google for testing purposes.
        unitID = "ca-app-pub-3940256099942544/4411468910";
    }

    NSString *nsUnitID = [[NSString alloc] initWithBytes:unitID length:strlen(unitID) encoding:NSUTF8StringEncoding];

    interstitial = [[GADInterstitial alloc] initWithAdUnitID:nsUnitID];
    interstitial.delegate = AdMob::viewController;

    [interstitial loadRequest:request];
}

bool AdMob::InterstitialAd::IsReady() const {
    return [interstitial isReady];
}

void AdMob::InterstitialAd::Present() {
    [interstitial presentFromRootViewController:AdMob::viewController];
}

//-------------------------------------------------------------------------------------------------

void AdMob::RewardBasedVideoAd::Init() {
}

void AdMob::RewardBasedVideoAd::Request(const char *unitID, const char *testDevices) {
    GADRequest *request = [GADRequest request];

    BE1::StrArray testDeviceList;
    BE1::SplitStringIntoList(testDeviceList, testDevices, " ");

    if (testDeviceList.Count() > 0) {
        NSString *nsTestDevices[256];
        for (int i = 0; i < testDeviceList.Count(); i++) {
            nsTestDevices[i] = [[NSString alloc] initWithBytes:testDeviceList[i].c_str() length:testDeviceList[i].Length() encoding:NSUTF8StringEncoding];
        }
    
        request.testDevices = [NSArray arrayWithObjects:nsTestDevices count:testDeviceList.Count()];
    }
    
    if (!unitID || !unitID[0]) {
        // ad unit of rewarded video provided by Google for testing purposes.
        unitID = "ca-app-pub-3940256099942544/1712485313";
    }

    NSString *nsUnitID = [[NSString alloc] initWithBytes:unitID length:strlen(unitID) encoding:NSUTF8StringEncoding];

    // Required to set the delegate prior to loading an ad.
    [GADRewardBasedVideoAd sharedInstance].delegate = AdMob::viewController;

    // Request to load an ad.
    [[GADRewardBasedVideoAd sharedInstance] loadRequest:request withAdUnitID:nsUnitID];
}

bool AdMob::RewardBasedVideoAd::IsReady() const {
    return [[GADRewardBasedVideoAd sharedInstance] isReady];
}

void AdMob::RewardBasedVideoAd::Present() {
    [[GADRewardBasedVideoAd sharedInstance] presentFromRootViewController:AdMob::viewController];
}
