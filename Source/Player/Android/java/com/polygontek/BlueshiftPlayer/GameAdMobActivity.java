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

import android.graphics.drawable.BitmapDrawable;
import android.media.AudioManager;
import android.os.Bundle;
import android.support.annotation.Keep;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Gravity;
import android.view.SurfaceView;
import android.view.View;
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
    private static final String TAG = GameAdMobActivity.class.getName();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        mActivity = this;

        super.onCreate(savedInstanceState);

        ViewGroup.MarginLayoutParams params = new ViewGroup.MarginLayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        params.setMargins(0, 0, 0, 0);
        mActivityLayout = new LinearLayout(this);
        setContentView(mActivityLayout, params);

        // tell Android that we want volume controls to change the media volume, aka music
        setVolumeControlStream(AudioManager.STREAM_MUSIC);

        String Language = java.util.Locale.getDefault().toString();

        Log.d(TAG, "Android version is " + android.os.Build.VERSION.RELEASE);
        Log.d(TAG, "Android manufacturer is " + android.os.Build.MANUFACTURER);
        Log.d(TAG, "Android model is " + android.os.Build.MODEL);
        Log.d(TAG, "OS language is set to " + Language);

        // Tell the activity's window that we want to do our own drawing
        // to its surface.  This prevents the view hierarchy from drawing to
        // it, though we can still add views to capture input if desired.
        getWindow().takeSurface(null);

        mSurfaceView = new SurfaceView(this);
        mSurfaceView.getHolder().addCallback(this);
        setContentView(mSurfaceView);
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

    public void requestBannerAd(final String unitID, final String testDevices[]) {
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
                mBannerAdView = new AdView(mActivity);
                mBannerAdView.setAdSize(AdSize.BANNER); // set size
                mBannerAdView.setAdListener(mBannerAdListener);
                mBannerAdView.setAdUnitId(unitID);

                final DisplayMetrics dm = getResources().getDisplayMetrics();
                final float scale = dm.density;
                mAdPopupWindow = new PopupWindow(mActivity);
                mAdPopupWindow.setWidth((int)(320 * scale));
                mAdPopupWindow.setHeight((int)(50 * scale));
                mAdPopupWindow.setWindowLayoutMode(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
                mAdPopupWindow.setClippingEnabled(false);

                LinearLayout popupLayout = new LinearLayout(mActivity);
                final int padding = (int)(-5 * scale);
                popupLayout.setPadding(padding, padding, padding, padding);
                popupLayout.setOrientation(LinearLayout.VERTICAL);

                ViewGroup.MarginLayoutParams params = new ViewGroup.MarginLayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
                params.setMargins(0,0,0,0);
                popupLayout.addView(mBannerAdView, params);

                mAdPopupWindow.setContentView(popupLayout);

                mBannerAdView.loadAd(request);
            }
        });
    }

    public void showBannerAd(final boolean showOnBottomOfScreen, final int offsetX, final int offsetY) {
        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mAdPopupWindow.isShowing()) {
                    return;
                }

                mAdPopupWindow.showAtLocation(mActivityLayout, showOnBottomOfScreen ? Gravity.BOTTOM : Gravity.TOP, offsetX, offsetY);
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
                mRewardedVideoAd.setRewardedVideoAdListener(mActivity);
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

    private SurfaceView mSurfaceView;
    private LinearLayout mActivityLayout = null;
    private PopupWindow mAdPopupWindow = null;
    private AdView mBannerAdView = null;
    private AdListener mBannerAdListener = null;
    private InterstitialAd mInterstitialAd = null;
    private AdListener mInterstitialAdListener = null;
    private boolean mInterstitialAdLoaded = false;
    private RewardedVideoAd mRewardedVideoAd = null;
    private boolean mRewardedVideoAdLoaded = false;

    private GameAdMobActivity mActivity;
}
