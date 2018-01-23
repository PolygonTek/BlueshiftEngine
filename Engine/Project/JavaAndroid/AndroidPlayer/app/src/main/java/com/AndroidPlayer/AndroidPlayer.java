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

public class AndroidPlayer extends Activity {

	public static final boolean _ENGINE = true;
	private static final int REQUEST_STORAGE = 1;
    GLES3JNIView mView;

    @Override protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        if (_ENGINE) {
            if (!getPackageName().equals("com.AndroidPlayer.AndroidPlayer"))
            {
                GLES3JNILib.SetAssetManager(getAssets(), getFilesDir().getAbsolutePath());
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
                GLES3JNILib.SetAssetManager(getAssets(), getFilesDir().getAbsolutePath());
			}
            else
            {
                GLES3JNILib.SetAssetManager(getAssets(), "/sdcard/blueshift");
            }
        }
        mView = new GLES3JNIView(getApplication());
        setContentView(mView);

    }

    @Override protected void onPause() {
        super.onPause();
        mView.onPause();
    }

    @Override protected void onResume() {
        super.onResume();
        mView.onResume();
    }
    @Override protected void onDestroy() {
    	if (_ENGINE) {
			GLES3JNILib.done();
    	}
        super.onDestroy();
    }
}
