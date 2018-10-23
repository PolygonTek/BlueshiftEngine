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
import android.os.Bundle;
import android.support.annotation.Keep;
import com.google.android.gms.ads.AdListener;
import com.google.android.gms.ads.AdRequest;
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

        mActivity = this;
    }

    @Override
    protected void onDestroy() {
        if (mRewardedVideoAd != null) {
            mRewardedVideoAd.destroy(this);
            mRewardedVideoAd = null;
        }
        super.onDestroy();
    }

    @Override
    protected void onPause() {
        if (mRewardedVideoAd != null) {
            mRewardedVideoAd.pause(this);
        }
        super.onPause();
    }

    @Override
    protected void onResume() {
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

    private InterstitialAd mInterstitialAd = null;
    private AdListener mInterstitialAdListener = null;
    private boolean mInterstitialAdLoaded = false;
    private RewardedVideoAd mRewardedVideoAd = null;
    private boolean mRewardedVideoAdLoaded = false;

    private GameAdMobActivity mActivity;
}
