/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.AndroidPlayer;
//package com.android.gles3jni;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;

import java.io.File;

import android.Manifest;
import android.support.v4.app.ActivityCompat;
import android.support.v4.app.FragmentTransaction;
import android.support.v7.app.AppCompatActivity;
import android.content.pm.PackageManager;

import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.MobileAds;
import com.google.android.gms.ads.reward.RewardItem;
import com.google.android.gms.ads.reward.RewardedVideoAd;
import com.google.android.gms.ads.reward.RewardedVideoAdListener;


public class AndroidPlayer extends Activity implements RewardedVideoAdListener {

    private static final String AD_UNIT_ID = "ca-app-pub-3940256099942544/5224354917";
    private static final String APP_ID = "ca-app-pub-3940256099942544~3347511713";
	private static final int REQUEST_STORAGE = 1;
    GLES3JNIView mView;
    private RewardedVideoAd mRewardedVideoAd;
    private boolean mIsLoadedRewardedVideoAd = false;

    @Override protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        if (GLES3JNILib._ENGINE) {
			Log.v(GLES3JNILib.TAG, "OnCreate");
            if (!getPackageName().equals("com.AndroidPlayer.AndroidPlayer"))
            {
                GLES3JNILib.SetAssetManager(this, getAssets(), getFilesDir().getAbsolutePath());
            }
			else if (ActivityCompat.checkSelfPermission(this,
                Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {

				if (ActivityCompat.shouldShowRequestPermissionRationale(this,
						Manifest.permission.WRITE_EXTERNAL_STORAGE)) {
				} else {
					ActivityCompat.requestPermissions(this,
							new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE},
							REQUEST_STORAGE);
				}
                GLES3JNILib.SetAssetManager(this, getAssets(), getFilesDir().getAbsolutePath());
			}
            else
            {
                GLES3JNILib.SetAssetManager(this, getAssets(), "/sdcard/blueshift");
            }

        }
        mView = new GLES3JNIView(getApplication());
        setContentView(mView);
        if (GLES3JNILib._ENGINE) {
			// Initialize the Mobile Ads SDK.
			Log.v(GLES3JNILib.TAG, "Admob Initializing");
			MobileAds.initialize(this);

			mRewardedVideoAd = MobileAds.getRewardedVideoAdInstance(this);
			mRewardedVideoAd.setRewardedVideoAdListener(this);
			Log.v(GLES3JNILib.TAG, "Admob Initialized");
			//loadRewardedVideoAd();
        }

    }

    @Override protected void onPause() {
        super.onPause();
        mView.onPause();
		if (GLES3JNILib._ENGINE) {
			mRewardedVideoAd.pause(this);
        }
    }

    @Override protected void onResume() {
        super.onResume();
        mView.onResume();
		if (GLES3JNILib._ENGINE) {
			mRewardedVideoAd.resume(this);
        }
    }
    public void loadRewardedVideoAd() {
		runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				Log.v(GLES3JNILib.TAG, "loadRewardedVideoAd");
				GLES3JNILib.flag = true;
				if (!mRewardedVideoAd.isLoaded()) {
					mRewardedVideoAd.loadAd(AD_UNIT_ID, new AdRequest.Builder().build());
				}
			}
		});
    }
    public void showRewardedVideoAd() {
		runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				Log.v(GLES3JNILib.TAG, "showRewardedVideoAd");
				if (mRewardedVideoAd.isLoaded()) {
					mRewardedVideoAd.show();
				}
			}
		});
    }
    public boolean isLoadedRewardedVideoAd() {
        return mIsLoadedRewardedVideoAd;
    }
    @Override
    public void onRewardedVideoAdLeftApplication() {
		// GLES3JNILib.WillLeaveApplication();
    }

    @Override
    public void onRewardedVideoAdClosed() {
			//Toast.makeText(this, "onRewardedVideoAdClosed", Toast.LENGTH_SHORT).show();
			// Preload the next video ad.
		mIsLoadedRewardedVideoAd = false;        
		// GLES3JNILib.DidClose();
			//loadRewardedVideoAd();
    }

    @Override
    public void onRewardedVideoAdFailedToLoad(int errorCode) {
		//GLES3JNILib.DidFailToLoad(errorCode);
    		//Toast.makeText(this, "onRewardedVideoAdFailedToLoad", Toast.LENGTH_SHORT).show();
    }

    @Override
    public void onRewardedVideoAdLoaded() {
		mIsLoadedRewardedVideoAd = true;        
		//GLES3JNILib.DidReceiveAd();
			//Toast.makeText(this, "onRewardedVideoAdLoaded", Toast.LENGTH_SHORT).show();
    }

    @Override
    public void onRewardedVideoAdOpened() {
			//Toast.makeText(this, "onRewardedVideoAdOpened", Toast.LENGTH_SHORT).show();
		//GLES3JNILib.DidOpen();
    }

    @Override
    public void onRewarded(RewardItem reward) {
		//GLES3JNILib.DidRewardUser(reward.getType(), reward.getAmount());
			//Toast.makeText(this,
			//        String.format(" onRewarded! currency: %s amount: %d", reward.getType(),
			//                reward.getAmount()),
			//        Toast.LENGTH_SHORT).show();
    }

    @Override
    public void onRewardedVideoStarted() {
		//GLES3JNILib.DidStartPlaying();
			//Toast.makeText(this, "onRewardedVideoStarted", Toast.LENGTH_SHORT).show();
    }
}
