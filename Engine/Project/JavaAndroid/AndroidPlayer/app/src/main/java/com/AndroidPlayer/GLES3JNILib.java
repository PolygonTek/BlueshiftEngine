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

// Wrapper for native library

public class GLES3JNILib {

     static {
          System.loadLibrary("AndroidPlayerLibrary");
     }

     public static native void init();
     public static native void resize(int width, int height);
     public static native void step();
	 // _ENGINE
	public static native void done();
	public static native void SetAssetManager(Object asset, String path);
	public static native void TouchBegin(int id, int x, int y);
	public static native void TouchMove(int id, int x, int y);
	public static native void TouchEnd(int id, int x, int y);
	public static native void TouchCancel(int id);
}
