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

package com.polygontek.BlueshiftPlayer;

import java.util.concurrent.Semaphore;

import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.support.annotation.Keep;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.PopupWindow;

import com.google.android.gms.ads.AdListener;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.AdSize;
import com.google.android.gms.ads.AdView;
import com.google.android.gms.ads.MobileAds;
import com.google.android.gms.ads.InterstitialAd;
import com.google.android.gms.ads.reward.RewardItem;
import com.google.android.gms.ads.reward.RewardedVideoAd;
import com.google.android.gms.ads.reward.RewardedVideoAdListener;

// References
// Banner Ads: https://developers.google.com/admob/android/banner?hl=ko
// Interstitial Ads: https://developers.google.com/admob/android/interstitial?hl=ko
// Rewarded Video Ads: https://developers.google.com/admob/android/rewarded-video?hl=ko

@Keep
public class GameAdMobActivity extends GameActivity implements RewardedVideoAdListener {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    protected void onDestroy() {
        if (mBannerAdView != null) {
            mBannerAdView.destroy();
            mBannerAdView = null;
        }
        if (mRewardedVideoAd != null) {
            mRewardedVideoAd.destroy(this);
            mRewardedVideoAd = null;
        }
        super.onDestroy();
    }

    @Override
    protected void onPause() {
        if (mBannerAdView != null) {
            mBannerAdView.pause();
        }
        if (mRewardedVideoAd != null) {
            mRewardedVideoAd.pause(this);
        }
        super.onPause();
    }

    @Override
    protected void onResume() {
        if (mBannerAdView != null) {
            mBannerAdView.resume();
        }
        if (mRewardedVideoAd != null) {
            mRewardedVideoAd.resume(this);
        }
        super.onResume();
    }

    public void initializeAds(final String appID) {
        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                MobileAds.initialize(mActivity, appID);
            }
        });
    }

    public void initializeBannerAd() {
        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                // Set up event notifications for banner ads
                mBannerAdListener = new AdListener() {
                    @Override
                    public void onAdLoaded() {
                        // Code to be executed when an ad finishes loading.
                        bannerAdLoaded();
                    }

                    @Override
                    public void onAdFailedToLoad(int errorCode) {
                        // Code to be executed when an ad request fails.
                        String errorMessage;
                        switch (errorCode) {
                            case AdRequest.ERROR_CODE_INTERNAL_ERROR:
                                errorMessage = "Internal error";
                                break;
                            case AdRequest.ERROR_CODE_INVALID_REQUEST:
                                errorMessage = "Invalid request";
                                break;
                            case AdRequest.ERROR_CODE_NETWORK_ERROR:
                                errorMessage = "Network Error";
                                break;
                            case AdRequest.ERROR_CODE_NO_FILL:
                                errorMessage = "No fill";
                                break;
                            default:
                                errorMessage = String.format("Unexpected error code: %s", errorCode);
                                break;
                        }
                        bannerAdFailedToLoad(errorMessage);
                    }

                    @Override
                    public void onAdOpened() {
                        // Code to be executed when an ad opens an overlay that
                        // covers the screen.
                        bannerAdOpened();
                    }

                    @Override
                    public void onAdClosed() {
                        // Code to be executed when when the user is about to return
                        // to the app after tapping on an ad.
                        bannerAdClosed();
                    }

                    @Override
                    public void onAdLeftApplication() {
                        // Code to be executed when the user has left the app.
                        bannerAdLeftApplication();
                    }
                };
            }
        });
    }

    public void requestBannerAd(final String unitID, final String testDevices[], final int adWidth, final int adHeight) {
        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                AdRequest.Builder requestBuilder = new AdRequest.Builder();
                requestBuilder.addTestDevice(AdRequest.DEVICE_ID_EMULATOR);
                for (int i = 0; i < testDevices.length; i++) {
                    requestBuilder.addTestDevice(testDevices[i]);
                }
                AdRequest request = requestBuilder.build();

                if (mBannerAdView != null) {
                    mBannerAdView.destroy();
                }

                final DisplayMetrics dm = getResources().getDisplayMetrics();

                // NOTE: adWidth <= 0 means full screen width
                mBannerAdView = new AdView(mActivity);
                mBannerAdView.setAdSize(new AdSize(adWidth, adHeight));
                mBannerAdView.setAdListener(mBannerAdListener);
                mBannerAdView.setAdUnitId(unitID);

                mAdPopupWindow = new PopupWindow(mActivity);
                mAdPopupWindow.setWidth(adWidth > 0 ? (int)(adWidth * dm.density) : dm.widthPixels);
                mAdPopupWindow.setHeight((int)(adHeight * dm.density));
                //mAdPopupWindow.setWindowLayoutMode(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
                mAdPopupWindow.setClippingEnabled(false);
                mAdPopupWindow.setBackgroundDrawable(new ColorDrawable(android.graphics.Color.TRANSPARENT));

                LinearLayout popupLayout = new LinearLayout(mActivity);
                popupLayout.setDividerPadding(0);
                popupLayout.setPadding(0, 0, 0, 0);
                popupLayout.setOrientation(LinearLayout.VERTICAL);

                ViewGroup.MarginLayoutParams params = new ViewGroup.MarginLayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
                params.setMargins(0,0,0,0);
                popupLayout.addView(mBannerAdView, params);

                mAdPopupWindow.setContentView(popupLayout);

                mBannerAdView.loadAd(request);
            }
        });
    }

    public void showBannerAd(final boolean showOnBottomOfScreen, final float offsetX, final float offsetY) {
        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mAdPopupWindow.isShowing()) {
                    return;
                }

                final DisplayMetrics dm = getResources().getDisplayMetrics();

                int x = (int)(offsetX * dm.widthPixels + 0.5f);
                int y = (int)(offsetY * dm.heightPixels + 0.5f);

                mAdPopupWindow.showAtLocation(mActivityLayout, showOnBottomOfScreen ? Gravity.BOTTOM : Gravity.TOP, x, y);
                mAdPopupWindow.update();

                mBannerAdView.resume();
            }
        });
    }

    public void hideBannerAd() {
        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (!mAdPopupWindow.isShowing()) {
                    return;
                }

                mAdPopupWindow.dismiss();
                mAdPopupWindow.update();

                mBannerAdView.pause();
            }
        });
    }

    public void initializeInterstitialAd() {
        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                // Set up event notifications for interstitial ads
                mInterstitialAdListener = new AdListener() {
                    @Override
                    public void onAdLoaded() {
                        // Code to be executed when an ad finishes loading.
                        interstitialAdLoaded();
                    }

                    @Override
                    public void onAdFailedToLoad(int errorCode) {
                        // Code to be executed when an ad request fails.
                        String errorMessage;
                        switch (errorCode) {
                            case AdRequest.ERROR_CODE_INTERNAL_ERROR:
                                errorMessage = "Internal error";
                                break;
                            case AdRequest.ERROR_CODE_INVALID_REQUEST:
                                errorMessage = "Invalid request";
                                break;
                            case AdRequest.ERROR_CODE_NETWORK_ERROR:
                                errorMessage = "Network Error";
                                break;
                            case AdRequest.ERROR_CODE_NO_FILL:
                                errorMessage = "No fill";
                                break;
                            default:
                                errorMessage = String.format("Unexpected error code: %s", errorCode);
                                break;
                        }
                        interstitialAdFailedToLoad(errorMessage);
                    }

                    @Override
                    public void onAdOpened() {
                        // Code to be executed when the ad is displayed.
                        interstitialAdOpened();
                    }

                    @Override
                    public void onAdClosed() {
                        // Code to be executed when when the interstitial ad is closed.
                        interstitialAdClosed();
                    }

                    @Override
                    public void onAdLeftApplication() {
                        // Code to be executed when the user has left the app.
                        interstitialAdLeftApplication();
                    }
                };
            }
        });
    }

    public void requestInterstitialAd(final String unitID, final String testDevices[]) {
        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                AdRequest.Builder requestBuilder = new AdRequest.Builder();
                requestBuilder.addTestDevice(AdRequest.DEVICE_ID_EMULATOR);
                for (int i = 0; i < testDevices.length; i++) {
                    requestBuilder.addTestDevice(testDevices[i]);
                }
                AdRequest request = requestBuilder.build();

                mInterstitialAd = new InterstitialAd(mActivity);
                mInterstitialAd.setAdListener(mInterstitialAdListener);
                mInterstitialAd.setAdUnitId(unitID);
                mInterstitialAd.loadAd(request);
            }
        });
    }

    public boolean isInterstitialAdLoaded() {
        final Semaphore mutex = new Semaphore(0);
        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mInterstitialAdLoaded = mInterstitialAd.isLoaded();
                mutex.release();
            }
        });

        try {
            mutex.acquire();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        return mInterstitialAdLoaded;
    }

    public void showInterstitialAd() {
        final Runnable showRunnable = new Runnable() {
            @Override
            public void run() {
                mInterstitialAd.show();
                synchronized (this) {
                    this.notify();
                }
            }
        };

        synchronized (showRunnable) {
            this.runOnUiThread(showRunnable);
            try {
                showRunnable.wait();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public void initializeRewardedVideoAd() {
        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                // Use an activity context to get the rewarded video instance.
                mRewardedVideoAd = MobileAds.getRewardedVideoAdInstance(mActivity);
                mRewardedVideoAd.setRewardedVideoAdListener((GameAdMobActivity)mActivity);
                mRewardedVideoAd.setImmersiveMode(true);
            }
        });
    }

    public void requestRewardedVideoAd(final String unitID, final String testDevices[]) {
        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                AdRequest.Builder requestBuilder = new AdRequest.Builder();
                requestBuilder.addTestDevice(AdRequest.DEVICE_ID_EMULATOR);
                for (int i = 0; i < testDevices.length; i++) {
                    requestBuilder.addTestDevice(testDevices[i]);
                }
                AdRequest request = requestBuilder.build();

                mRewardedVideoAd.loadAd(unitID, request);
            }
        });
    }

    public boolean isRewardedVideoAdLoaded() {
        final Semaphore mutex = new Semaphore(0);
        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mRewardedVideoAdLoaded = mRewardedVideoAd.isLoaded();
                mutex.release();
            }
        });

        try {
            mutex.acquire();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        return mRewardedVideoAdLoaded;
    }

    public void showRewardedVideoAd() {
        final Runnable showRunnable = new Runnable() {
            @Override
            public void run() {
                mRewardedVideoAd.show();
                synchronized (this) {
                    this.notify();
                }
            }
        };

        synchronized (showRunnable) {
            this.runOnUiThread(showRunnable);
            try {
                showRunnable.wait();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    //
    // Set up event notifications for rewarded video ads
    //
    @Override
    public void onRewardedVideoAdLoaded() {
        rewardBasedVideoAdLoaded();
    }

    @Override
    public void onRewardedVideoAdFailedToLoad(int errorCode) {
        String errorMessage;
        switch (errorCode) {
            case AdRequest.ERROR_CODE_INTERNAL_ERROR:
                errorMessage = "Internal error";
                break;
            case AdRequest.ERROR_CODE_INVALID_REQUEST:
                errorMessage = "Invalid request";
                break;
            case AdRequest.ERROR_CODE_NETWORK_ERROR:
                errorMessage = "Network Error";
                break;
            case AdRequest.ERROR_CODE_NO_FILL:
                errorMessage = "No fill";
                break;
            default:
                errorMessage = String.format("Unexpected error code: %s", errorCode);
                break;
        }
        rewardBasedVideoAdFailedToLoad(errorMessage);
    }

    @Override
    public void onRewardedVideoAdOpened() {
        rewardBasedVideoAdOpened();
    }

    @Override
    public void onRewardedVideoStarted() {
        rewardBasedVideoAdStarted();
    }

    @Override
    public void onRewarded(RewardItem reward) {
        rewardBasedVideoAdRewarded(reward.getType(), reward.getAmount());
    }

    @Override
    public void onRewardedVideoAdClosed() {
        rewardBasedVideoAdClosed();
    }

    @Override
    public void onRewardedVideoAdLeftApplication() {
        rewardBasedVideoAdLeftApplication();
    }

    //
    // JNI functions for passing events to script functions.
    //
    private native void bannerAdLoaded();
    private native void bannerAdFailedToLoad(String errorMessage);
    private native void bannerAdOpened();
    private native void bannerAdClosed();
    private native void bannerAdLeftApplication();

    private native void interstitialAdLoaded();
    private native void interstitialAdFailedToLoad(String errorMessage);
    private native void interstitialAdOpened();
    private native void interstitialAdClosed();
    private native void interstitialAdLeftApplication();

    private native void rewardBasedVideoAdLoaded();
    private native void rewardBasedVideoAdFailedToLoad(String errorMessage);
    private native void rewardBasedVideoAdOpened();
    private native void rewardBasedVideoAdStarted();
    private native void rewardBasedVideoAdRewarded(String type, int amount);
    private native void rewardBasedVideoAdClosed();
    private native void rewardBasedVideoAdLeftApplication();

    private PopupWindow mAdPopupWindow = null;
    private AdView mBannerAdView = null;
    private AdListener mBannerAdListener = null;
    private InterstitialAd mInterstitialAd = null;
    private AdListener mInterstitialAdListener = null;
    private boolean mInterstitialAdLoaded = false;
    private RewardedVideoAd mRewardedVideoAd = null;
    private boolean mRewardedVideoAdLoaded = false;
}
