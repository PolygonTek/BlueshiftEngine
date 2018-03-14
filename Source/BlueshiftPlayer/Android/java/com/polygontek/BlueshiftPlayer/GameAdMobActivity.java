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
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.MobileAds;
import com.google.android.gms.ads.reward.RewardItem;
import com.google.android.gms.ads.reward.RewardedVideoAd;
import com.google.android.gms.ads.reward.RewardedVideoAdListener;

@Keep
public class GameAdMobActivity extends GameActivity implements RewardedVideoAdListener {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        activity = this;
    }

    @Override
    protected void onDestroy() {
        if (rewardedVideoAd != null) {
            rewardedVideoAd.destroy(this);
            rewardedVideoAd = null;
        }
        super.onDestroy();
    }

    @Override
    protected void onPause() {
        if (rewardedVideoAd != null) {
            rewardedVideoAd.pause(this);
        }
        super.onPause();
    }

    @Override
    protected void onResume() {
        if (rewardedVideoAd != null) {
            rewardedVideoAd.resume(this);
        }
        super.onResume();
    }

    public void initializeAds(final String appID) {
        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                MobileAds.initialize(activity, appID);

                // Use an activity context to get the rewarded video instance.
                rewardedVideoAd = MobileAds.getRewardedVideoAdInstance(activity);
                rewardedVideoAd.setRewardedVideoAdListener(activity);
                rewardedVideoAd.setImmersiveMode(true);
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
                rewardedVideoAd.loadAd(unitID, request);
            }
        });
    }

    public boolean isRewardedVideoAdLoaded() {
        final Semaphore mutex = new Semaphore(0);
        this.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                adLoaded = rewardedVideoAd.isLoaded();
                mutex.release();
            }
        });

        try {
            mutex.acquire();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        return adLoaded;
    }

    public void showRewardedVideoAd() {
        final Runnable showRunnable = new Runnable() {
            @Override
            public void run() {
                rewardedVideoAd.show();
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

    @Override
    public void onRewarded(RewardItem reward) {
        rewardBasedVideoAdDidRewardUser(reward.getType(), reward.getAmount());
    }

    @Override
    public void onRewardedVideoAdLoaded() {
        rewardBasedVideoAdDidReceiveAd();
    }

    @Override
    public void onRewardedVideoAdOpened() {
        rewardBasedVideoAdDidOpen();
    }

    @Override
    public void onRewardedVideoStarted() {
        rewardBasedVideoAdDidStartPlaying();
    }

    @Override
    public void onRewardedVideoAdClosed() {
        rewardBasedVideoAdDidClose();
    }

    @Override
    public void onRewardedVideoAdLeftApplication() {
        rewardBasedVideoAdWillLeaveApplication();
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
        rewardBasedVideoAdDidFailToLoad(errorMessage);
    }  

    private native void rewardBasedVideoAdDidRewardUser(String type, int amount);
    private native void rewardBasedVideoAdDidReceiveAd();
    private native void rewardBasedVideoAdDidOpen();
    private native void rewardBasedVideoAdDidStartPlaying();
    private native void rewardBasedVideoAdDidClose();
    private native void rewardBasedVideoAdWillLeaveApplication();
    private native void rewardBasedVideoAdDidFailToLoad(String errorMessage);

    private RewardedVideoAd rewardedVideoAd = null;
    private boolean adLoaded = false;

    private GameAdMobActivity activity;
}


