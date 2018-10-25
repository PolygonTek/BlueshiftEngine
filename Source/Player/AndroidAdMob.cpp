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
#include "android_native_app_glue.h"
#include "AndroidAdMob.h"

AdMob::BannerAd AdMob::bannerAd;
AdMob::InterstitialAd AdMob::interstitialAd;
AdMob::RewardBasedVideoAd AdMob::rewardBasedVideoAd;

BE1::StrArray AdMob::testDeviceList;

static jmethodID javaMethod_initializeAds = nullptr;

static jmethodID javaMethod_initializeBannerAd = nullptr;
static jmethodID javaMethod_requestBannerAd = nullptr;
static jmethodID javaMethod_showBannerAd = nullptr;
static jmethodID javaMethod_hideBannerAd = nullptr;

static jmethodID javaMethod_initializeInterstitialAd = nullptr;
static jmethodID javaMethod_requestInterstitialAd = nullptr;
static jmethodID javaMethod_isInterstitialAdLoaded = nullptr;
static jmethodID javaMethod_showInterstitialAd = nullptr;

static jmethodID javaMethod_initializeRewardedVideoAd = nullptr;
static jmethodID javaMethod_requestRewardedVideoAd = nullptr;
static jmethodID javaMethod_isRewardedVideoAdLoaded = nullptr;
static jmethodID javaMethod_showRewardedVideoAd = nullptr;

static std::queue<std::function<void()>> callbackQueue;
static std::mutex callbackQueueMutex;

void AdMob::RegisterLuaModule(LuaCpp::State *state) {
    JNIEnv *env = BE1::AndroidJNI::GetJavaEnv();

    jclass javaClassActivity = env->GetObjectClass(BE1::AndroidJNI::activity->clazz);

    javaMethod_initializeAds = BE1::AndroidJNI::FindMethod(env, javaClassActivity, "initializeAds", "(Ljava/lang/String;)V", false);

    javaMethod_initializeBannerAd = BE1::AndroidJNI::FindMethod(env, javaClassActivity, "initializeBannerAd", "()V", false);
    javaMethod_requestBannerAd = BE1::AndroidJNI::FindMethod(env, javaClassActivity, "requestBannerAd", "(Ljava/lang/String;[Ljava/lang/String;II)V", false);
    javaMethod_showBannerAd = BE1::AndroidJNI::FindMethod(env, javaClassActivity, "showBannerAd", "(ZFF)V", false);
    javaMethod_hideBannerAd = BE1::AndroidJNI::FindMethod(env, javaClassActivity, "hideBannerAd", "()V", false);

    javaMethod_initializeInterstitialAd = BE1::AndroidJNI::FindMethod(env, javaClassActivity, "initializeInterstitialAd", "()V", false);
    javaMethod_requestInterstitialAd = BE1::AndroidJNI::FindMethod(env, javaClassActivity, "requestInterstitialAd", "(Ljava/lang/String;[Ljava/lang/String;)V", false);
    javaMethod_isInterstitialAdLoaded = BE1::AndroidJNI::FindMethod(env, javaClassActivity, "isInterstitialAdLoaded", "()Z", false);
    javaMethod_showInterstitialAd = BE1::AndroidJNI::FindMethod(env, javaClassActivity, "showInterstitialAd", "()V", false);

    javaMethod_initializeRewardedVideoAd = BE1::AndroidJNI::FindMethod(env, javaClassActivity, "initializeRewardedVideoAd", "()V", false);
    javaMethod_requestRewardedVideoAd = BE1::AndroidJNI::FindMethod(env, javaClassActivity, "requestRewardedVideoAd", "(Ljava/lang/String;[Ljava/lang/String;)V", false);
    javaMethod_isRewardedVideoAdLoaded = BE1::AndroidJNI::FindMethod(env, javaClassActivity, "isRewardedVideoAdLoaded", "()Z", false);
    javaMethod_showRewardedVideoAd = BE1::AndroidJNI::FindMethod(env, javaClassActivity, "showRewardedVideoAd", "()V", false);

    env->DeleteLocalRef(javaClassActivity);

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

void AdMob::Init(const char *appID, const char *testDevices) {
    JNIEnv *env = BE1::AndroidJNI::GetJavaEnv();

    jstring javaAppID = BE1::Str(appID).ToJavaString(env);

    BE1::AndroidJNI::CallVoidMethod(env, BE1::AndroidJNI::activity->clazz, javaMethod_initializeAds, javaAppID);

    env->DeleteLocalRef(javaAppID);

    BE1::SplitStringIntoList(testDeviceList, testDevices, " ");
}

void AdMob::ProcessQueue() {
    std::lock_guard<std::mutex> lock(callbackQueueMutex);

    while (!callbackQueue.empty()) {
        callbackQueue.front()();
        callbackQueue.pop();
    }
}

//-------------------------------------------------------------------------------------------------

void AdMob::BannerAd::Init() {
    JNIEnv *env = BE1::AndroidJNI::GetJavaEnv();

    BE1::AndroidJNI::CallVoidMethod(env, BE1::AndroidJNI::activity->clazz, javaMethod_initializeBannerAd);
}

void AdMob::BannerAd::Request(const char *unitID, int adWidth, int adHeight) {
    // ad unit of interstitial provided by Google for testing purposes.
    static const char *testUnitID = "ca-app-pub-3940256099942544/6300978111";
#ifdef _DEBUG
    unitID = testUnitID;
#else
    if (!unitID || !unitID[0]) {
        unitID = testUnitID;
    }
#endif

    JNIEnv *env = BE1::AndroidJNI::GetJavaEnv();

    jobjectArray javaTestDevices = (jobjectArray)env->NewObjectArray(testDeviceList.Count(), env->FindClass("java/lang/String"), env->NewStringUTF(""));

    for (int i = 0; i < testDeviceList.Count(); i++) {
        env->SetObjectArrayElement(javaTestDevices, i, env->NewStringUTF(testDeviceList[i].c_str()));
    }

    jstring javaUnitID = BE1::Str(unitID).ToJavaString(env);

    BE1::AndroidJNI::CallVoidMethod(env, BE1::AndroidJNI::activity->clazz, javaMethod_requestBannerAd, javaUnitID, javaTestDevices, adWidth, adHeight);

    env->DeleteLocalRef(javaUnitID);
}

void AdMob::BannerAd::Show(bool showOnBottomOfScreen, float offsetX, float offsetY) {
    JNIEnv *env = BE1::AndroidJNI::GetJavaEnv();

    BE1::AndroidJNI::CallVoidMethod(env, BE1::AndroidJNI::activity->clazz, javaMethod_showBannerAd, showOnBottomOfScreen, offsetX, offsetY);
}

void AdMob::BannerAd::Hide() {
    JNIEnv *env = BE1::AndroidJNI::GetJavaEnv();

    BE1::AndroidJNI::CallVoidMethod(env, BE1::AndroidJNI::activity->clazz, javaMethod_hideBannerAd);
}

//-------------------------------------------------------------------------------------------------

void AdMob::InterstitialAd::Init() {
    JNIEnv *env = BE1::AndroidJNI::GetJavaEnv();

    BE1::AndroidJNI::CallVoidMethod(env, BE1::AndroidJNI::activity->clazz, javaMethod_initializeInterstitialAd);
}

void AdMob::InterstitialAd::Request(const char *unitID) {
    // ad unit of interstitial provided by Google for testing purposes.
    static const char *testUnitID = "ca-app-pub-3940256099942544/1033173712";
#ifdef _DEBUG
    unitID = testUnitID;
#else
    if (!unitID || !unitID[0]) {
        unitID = testUnitID;
    }
#endif

    JNIEnv *env = BE1::AndroidJNI::GetJavaEnv();

    jobjectArray javaTestDevices = (jobjectArray)env->NewObjectArray(testDeviceList.Count(), env->FindClass("java/lang/String"), env->NewStringUTF(""));

    for (int i = 0; i < testDeviceList.Count(); i++) {
        env->SetObjectArrayElement(javaTestDevices, i, env->NewStringUTF(testDeviceList[i].c_str()));
    }

    jstring javaUnitID = BE1::Str(unitID).ToJavaString(env);

    BE1::AndroidJNI::CallVoidMethod(env, BE1::AndroidJNI::activity->clazz, javaMethod_requestInterstitialAd, javaUnitID, javaTestDevices);

    env->DeleteLocalRef(javaUnitID);
}

bool AdMob::InterstitialAd::IsReady() const {
    JNIEnv *env = BE1::AndroidJNI::GetJavaEnv();

    return BE1::AndroidJNI::CallBooleanMethod(env, BE1::AndroidJNI::activity->clazz, javaMethod_isInterstitialAdLoaded);
}

void AdMob::InterstitialAd::Present() {
    JNIEnv *env = BE1::AndroidJNI::GetJavaEnv();

    BE1::AndroidJNI::CallVoidMethod(env, BE1::AndroidJNI::activity->clazz, javaMethod_showInterstitialAd);
}

//-------------------------------------------------------------------------------------------------

void AdMob::RewardBasedVideoAd::Init() {
    JNIEnv *env = BE1::AndroidJNI::GetJavaEnv();

    BE1::AndroidJNI::CallVoidMethod(env, BE1::AndroidJNI::activity->clazz, javaMethod_initializeRewardedVideoAd);
}

void AdMob::RewardBasedVideoAd::Request(const char *unitID) {
    // ad unit of rewarded video provided by Google for testing purposes.
    static const char *testUnitID = "ca-app-pub-3940256099942544/5224354917";
#ifdef _DEBUG
    unitID = testUnitID;
#else
    if (!unitID || !unitID[0]) {
        unitID = testUnitID;
    }
#endif

    JNIEnv *env = BE1::AndroidJNI::GetJavaEnv();

    jobjectArray javaTestDevices = (jobjectArray)env->NewObjectArray(testDeviceList.Count(), env->FindClass("java/lang/String"), env->NewStringUTF(""));

    for (int i = 0; i < testDeviceList.Count(); i++) {
        env->SetObjectArrayElement(javaTestDevices, i, env->NewStringUTF(testDeviceList[i].c_str()));
    }

    jstring javaUnitID = BE1::Str(unitID).ToJavaString(env);

    BE1::AndroidJNI::CallVoidMethod(env, BE1::AndroidJNI::activity->clazz, javaMethod_requestRewardedVideoAd, javaUnitID, javaTestDevices);

    env->DeleteLocalRef(javaUnitID);
}

bool AdMob::RewardBasedVideoAd::IsReady() const {
    JNIEnv *env = BE1::AndroidJNI::GetJavaEnv();

    return BE1::AndroidJNI::CallBooleanMethod(env, BE1::AndroidJNI::activity->clazz, javaMethod_isRewardedVideoAdLoaded);
}

void AdMob::RewardBasedVideoAd::Present() {
    JNIEnv *env = BE1::AndroidJNI::GetJavaEnv();

    BE1::AndroidJNI::CallVoidMethod(env, BE1::AndroidJNI::activity->clazz, javaMethod_showRewardedVideoAd);
}

//
// Internal functions to call script functions
//

// Called when an ad request has successfully loaded.
static void bannerAdLoaded() {
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["BannerAd"]["on_loaded"];
    if (function.IsFunction()) {
        function();
    }
}

// Called when an ad request failed to load.
static void bannerAdFailedToLoad(const BE1::Str &errorMessage) {
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["BannerAd"]["on_failed_to_load"];
    if (function.IsFunction()) {
        function(errorMessage.c_str());
    }
}

// Called when an ad is shown.
static void bannerAdOpened() {
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["BannerAd"]["on_opening"];
    if (function.IsFunction()) {
        function();
    }
}

// Called when the ad is closed.
static void bannerAdClosed() {
    // Create new banner object and request again
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["BannerAd"]["on_closed"];
    if (function.IsFunction()) {
        function();
    }
}

// Called when the ad click caused the user to leave the application.
static void bannerAdLeftApplication() {
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["BannerAd"]["on_leaving_application"];
    if (function.IsFunction()) {
        function();
    }
}

// Called when an ad request has successfully loaded.
static void interstitialAdLoaded() {
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["InterstitialAd"]["on_loaded"];
    if (function.IsFunction()) {
        function();
    }
}

// Called when an ad request failed to load.
static void interstitialAdFailedToLoad(const BE1::Str &errorMessage) {
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["InterstitialAd"]["on_failed_to_load"];
    if (function.IsFunction()) {
        function(errorMessage.c_str());
    }
}

// Called when an ad is shown.
static void interstitialAdOpened() {
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["InterstitialAd"]["on_opening"];
    if (function.IsFunction()) {
        function();
    }
}

// Called when the ad is closed.
static void interstitialAdClosed() {
    // Create new interstitial object and request again
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["InterstitialAd"]["on_closed"];
    if (function.IsFunction()) {
        function();
    }
}

// Called when the ad click caused the user to leave the application.
static void interstitialAdLeftApplication() {
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["InterstitialAd"]["on_leaving_application"];
    if (function.IsFunction()) {
        function();
    }
}

// Called when an ad request has successfully loaded.
static void rewardBasedVideoAdLoaded() {
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["on_loaded"];
    if (function.IsFunction()) {
        function();
    }
}

// Called when an ad request failed to load.
static void rewardBasedVideoAdFailedToLoad(const BE1::Str &errorMessage) {
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["on_failed_to_load"];
    if (function.IsFunction()) {
        function(errorMessage.c_str());
    }
}

// Called when an ad is shown.
static void rewardBasedVideoAdOpened() {
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["on_opening"];
    if (function.IsFunction()) {
        function();
    }
}

// Called when the ad starts to play.
static void rewardBasedVideoAdStarted() {
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["on_started"];
    if (function.IsFunction()) {
        function();
    }
}

// Called when the user should be rewarded for watching a video.
static void rewardBasedVideoAdRewarded(const BE1::Str &type, int amount) {
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["on_rewarded"];
    if (function.IsFunction()) {
        function(type.c_str(), amount);
    }
}

// Called when the ad is closed.
static void rewardBasedVideoAdClosed() {
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["on_closed"];
    if (function.IsFunction()) {
        function();
    }
}

// Called when the ad click caused the user to leave the application.
static void rewardBasedVideoAdLeftApplication() {
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["on_leaving_application"];
    if (function.IsFunction()) {
        function();
    }
}

//
// JNI callbacks invoked from the GameAdMobActivity class
//

extern "C" {

// native void GameAdMobActivity::bannerAdLoaded()
JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_bannerAdLoaded(JNIEnv *env, jobject thiz) {
    std::lock_guard<std::mutex> lock(callbackQueueMutex);

    callbackQueue.push([] { bannerAdLoaded(); });
}

// native void GameAdMobActivity::bannerAdFailedToLoad(String errorMessage)
JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_bannerAdFailedToLoad(JNIEnv *env, jobject thiz, jstring javaErrMsg) {
    std::lock_guard<std::mutex> lock(callbackQueueMutex);

    const char *errMsg = env->GetStringUTFChars(javaErrMsg, nullptr);
    BE1::Str errMsgStr = errMsg;

    env->ReleaseStringUTFChars(javaErrMsg, errMsg);

    callbackQueue.push([errMsgStr] { bannerAdFailedToLoad(errMsgStr); });
}

// native void GameAdMobActivity::bannerAdOpened()
JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_bannerAdOpened(JNIEnv *env, jobject thiz) {
    std::lock_guard<std::mutex> lock(callbackQueueMutex);

    callbackQueue.push([] { bannerAdOpened(); });
}

// native void GameAdMobActivity::bannerAdClosed()
JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_bannerAdClosed(JNIEnv *env, jobject thiz) {
    std::lock_guard<std::mutex> lock(callbackQueueMutex);

    callbackQueue.push([] { bannerAdClosed(); });
}

// native void GameAdMobActivity::bannerAdLeftApplication()
JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_bannerAdLeftApplication(JNIEnv *env, jobject thiz) {
    std::lock_guard<std::mutex> lock(callbackQueueMutex);

    callbackQueue.push([] { bannerAdLeftApplication(); });
}

// native void GameAdMobActivity::interstitialAdLoaded()
JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_interstitialAdLoaded(JNIEnv *env, jobject thiz) {
    std::lock_guard<std::mutex> lock(callbackQueueMutex);

    callbackQueue.push([] { interstitialAdLoaded(); });
}

// native void GameAdMobActivity::interstitialAdFailedToLoad(String errorMessage)
JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_interstitialAdFailedToLoad(JNIEnv *env, jobject thiz, jstring javaErrMsg) {
    std::lock_guard<std::mutex> lock(callbackQueueMutex);

    const char *errMsg = env->GetStringUTFChars(javaErrMsg, nullptr);
    BE1::Str errMsgStr = errMsg;

    env->ReleaseStringUTFChars(javaErrMsg, errMsg);

    callbackQueue.push([errMsgStr] { interstitialAdFailedToLoad(errMsgStr); });
}

// native void GameAdMobActivity::interstitialAdOpened()
JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_interstitialAdOpened(JNIEnv *env, jobject thiz) {
    std::lock_guard<std::mutex> lock(callbackQueueMutex);

    callbackQueue.push([] { interstitialAdOpened(); });
}

// native void GameAdMobActivity::interstitialAdClosed()
JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_interstitialAdClosed(JNIEnv *env, jobject thiz) {
    std::lock_guard<std::mutex> lock(callbackQueueMutex);

    callbackQueue.push([] { interstitialAdClosed(); });
}

// native void GameAdMobActivity::interstitialAdLeftApplication()
JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_interstitialAdLeftApplication(JNIEnv *env, jobject thiz) {
    std::lock_guard<std::mutex> lock(callbackQueueMutex);

    callbackQueue.push([] { interstitialAdLeftApplication(); });
}

// native void GameAdMobActivity::rewardBasedVideoAdLoaded()
JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_rewardBasedVideoAdLoaded(JNIEnv *env, jobject thiz) {
    std::lock_guard<std::mutex> lock(callbackQueueMutex);

    callbackQueue.push([] { rewardBasedVideoAdLoaded(); });
}

// native void GameAdMobActivity::rewardBasedVideoAdFailedToLoad(String errorMessage)
JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_rewardBasedVideoAdFailedToLoad(JNIEnv *env, jobject thiz, jstring javaErrMsg) {
    std::lock_guard<std::mutex> lock(callbackQueueMutex);

    const char *errMsg = env->GetStringUTFChars(javaErrMsg, nullptr);
    BE1::Str errMsgStr = errMsg;

    env->ReleaseStringUTFChars(javaErrMsg, errMsg);

    callbackQueue.push([errMsgStr] { rewardBasedVideoAdFailedToLoad(errMsgStr); });
}

// native void GameAdMobActivity::rewardBasedVideoAdOpened()
JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_rewardBasedVideoAdOpened(JNIEnv *env, jobject thiz) {
    std::lock_guard<std::mutex> lock(callbackQueueMutex);

    callbackQueue.push([] { rewardBasedVideoAdOpened(); });
}

// native void GameAdMobActivity::rewardBasedVideoAdStarted()
JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_rewardBasedVideoAdStarted(JNIEnv *env, jobject thiz) {
    std::lock_guard<std::mutex> lock(callbackQueueMutex);

    callbackQueue.push([] { rewardBasedVideoAdStarted(); });
}

// native void GameAdMobActivity::rewardBasedVideoAdRewarded(String type, int amount)
JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_rewardBasedVideoAdRewarded(JNIEnv *env, jobject thiz, jstring javaType, jint javaAmount) {
    std::lock_guard<std::mutex> lock(callbackQueueMutex);

    const char *type = env->GetStringUTFChars(javaType, nullptr);
    BE1::Str typeStr = type;
    int amount = (int)javaAmount;

    env->ReleaseStringUTFChars(javaType, type);

    callbackQueue.push([typeStr, amount] { rewardBasedVideoAdRewarded(typeStr, amount); });
}

// native void GameAdMobActivity::rewardBasedVideoAdClosed()
JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_rewardBasedVideoAdClosed(JNIEnv *env, jobject thiz) {
    std::lock_guard<std::mutex> lock(callbackQueueMutex);

    callbackQueue.push([] { rewardBasedVideoAdClosed(); });
}

// native void GameAdMobActivity::rewardBasedVideoAdLeftApplication()
JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_rewardBasedVideoAdLeftApplication(JNIEnv *env, jobject thiz) {
    std::lock_guard<std::mutex> lock(callbackQueueMutex);

    callbackQueue.push([] { rewardBasedVideoAdLeftApplication(); });
}

}