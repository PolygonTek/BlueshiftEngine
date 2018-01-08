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

RewardBasedVideoAd rewardBasedVideoAd;

void RewardBasedVideoAd::RegisterLuaModule(LuaCpp::State *state, UIViewController<GADRewardBasedVideoAdDelegate> *viewController) {
    state->RegisterModule("admob", [](LuaCpp::Module &module) {
        LuaCpp::Selector _RewardBasedVideoAd = module["RewardBasedVideoAd"];
        
        _RewardBasedVideoAd.SetObj(rewardBasedVideoAd);
        _RewardBasedVideoAd.AddObjMembers(rewardBasedVideoAd,
                                          "request", &RewardBasedVideoAd::Request,
                                          "is_ready", &RewardBasedVideoAd::IsReady,
                                          "present", &RewardBasedVideoAd::Present);
    });
    
    rewardBasedVideoAd.selector = (*state)["admob"]["RewardBasedVideoAd"];
    rewardBasedVideoAd.viewController = viewController;
}

void RewardBasedVideoAd::Request(const char *unitID, int numTestDevices, const char *testDevices[]) {
    NSString *nsTestDevices[64];
    for (int i = 0; i < numTestDevices; i++) {
        nsTestDevices[i] = [[NSString alloc] initWithBytes:testDevices[i] length:strlen(testDevices[i]) encoding:NSUTF8StringEncoding];
    }
    GADRequest *request = [GADRequest request];
    request.testDevices = [NSArray arrayWithObjects:nsTestDevices count:numTestDevices]; //@[@"684e00567d950ffd10f284ce38d1eaa2"];
    
    //NSString *nsTestUnitID = @"ca-app-pub-3940256099942544/1712485313";
    NSString *nsUnitID = [[NSString alloc] initWithBytes:unitID length:strlen(unitID) encoding:NSUTF8StringEncoding];
    // Set up event notification
    // Required to set the delegate prior to loading an ad.
    [GADRewardBasedVideoAd sharedInstance].delegate = viewController;
    [[GADRewardBasedVideoAd sharedInstance] loadRequest:request
                                            withAdUnitID:nsUnitID];
}
    
bool RewardBasedVideoAd::IsReady() const {
    return [[GADRewardBasedVideoAd sharedInstance] isReady];
}

void RewardBasedVideoAd::Present() {
    [[GADRewardBasedVideoAd sharedInstance] presentFromRootViewController:viewController];
}

void RewardBasedVideoAd::DidRewardUser(const char *rewardType, int rewardAmount) {
    LuaCpp::Selector function = selector["did_reward_user"];
    if (function.IsFunction()) {
        function(rewardType, rewardAmount);
    }
}

void RewardBasedVideoAd::DidReceiveAd() {
    LuaCpp::Selector function = selector["did_receive_ad"];
    if (function.IsFunction()) {
        function();
    }
}

void RewardBasedVideoAd::DidOpen() {
    LuaCpp::Selector function = selector["did_open"];
    if (function.IsFunction()) {
        function();
    }
}

void RewardBasedVideoAd::DidStartPlaying() {
    LuaCpp::Selector function = selector["did_start_playing"];
    if (function.IsFunction()) {
        function();
    }
}

void RewardBasedVideoAd::DidClose() {
    LuaCpp::Selector function = selector["did_close"];
    if (function.IsFunction()) {
        function();
    }
}

void RewardBasedVideoAd::WillLeaveApplication() {
    LuaCpp::Selector function = selector["will_leave_application"];
    if (function.IsFunction()) {
        function();
    }
}

void RewardBasedVideoAd::DidFailToLoad(const char *errorDescription) {
    LuaCpp::Selector function = selector["did_fail_to_load"];
    if (function.IsFunction()) {
        function(errorDescription);
    }
}

