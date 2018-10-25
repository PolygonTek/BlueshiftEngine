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
#include "GoogleMobileAds/GoogleMobileAds.h"

class AdMob {
public:
    class BannerAd;
    class InterstitialAd;
    class RewardBasedVideoAd;

    static void Init(const char *appID, const char *testDevices = "");

    static void RegisterLuaModule(LuaCpp::State *state, UIViewController<GADBannerViewDelegate, GADInterstitialDelegate, GADRewardBasedVideoAdDelegate> *viewController);

    static UIViewController<GADBannerViewDelegate, GADInterstitialDelegate, GADRewardBasedVideoAdDelegate> *viewController;

    static BannerAd bannerAd;
    static InterstitialAd interstitialAd;
    static RewardBasedVideoAd rewardBasedVideoAd;

    static BE1::StrArray testDeviceList;
};

class AdMob::BannerAd {
public:
    void Init();

    void Request(const char *unitID, int adWidth, int adHeight);

    void Show(bool showOnBottomOfScreen, float offsetX, float offsetY);

    void Hide();

private:
    GADBannerView *bannerView;
};

class AdMob::InterstitialAd {
public:
    void Init();

    void Request(const char *unitID);

    bool IsReady() const;

    void Present();

private:
    GADInterstitial *interstitial;
};

class AdMob::RewardBasedVideoAd {
public:
    void Init();

    void Request(const char *unitID);
    
    bool IsReady() const;

    void Present();
};
