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

import android.content.Context;
import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.opengles.GL10;

class GLES3JNIView extends GLSurfaceView {
    private static final String TAG = "GLES3JNI";
    private static final boolean DEBUG = true;

    public GLES3JNIView(Context context) {
        super(context);
        // Pick an EGLConfig with RGB8 color, 16-bit depth, no stencil,
        // supporting OpenGL ES 2.0 or later backwards-compatible versions.
        setEGLConfigChooser(8, 8, 8, 0, 16, 0);
        setEGLContextClientVersion(3);
        setRenderer(new Renderer());
    }
	@Override
	public boolean onTouchEvent(MotionEvent evt)
	{
		if (AndroidPlayer._ENGINE) {
			if (evt.getActionMasked() == MotionEvent.ACTION_DOWN)
			{
				for (int index = 0; index < evt.getPointerCount(); index++)
				{
					int x = (int)evt.getX(index);
					int y = (int)evt.getY(index);
					int touch_id = evt.getPointerId(index);

					GLES3JNILib.TouchBegin(touch_id, x, y);
				}
			}
			else if (evt.getActionMasked() == MotionEvent.ACTION_MOVE)
			{
				for (int index = 0; index < evt.getPointerCount(); index++)
				{
					int x = (int)evt.getX(index);
					int y = (int)evt.getY(index);
					int touch_id = evt.getPointerId(index);

					GLES3JNILib.TouchMove(touch_id, x, y);
				}
			}
			else if (evt.getActionMasked() ==  MotionEvent.ACTION_UP)
			{
				for (int index = 0; index < evt.getPointerCount(); index++)
				{
					int x = (int)evt.getX(index);
					int y = (int)evt.getY(index);
					int touch_id = evt.getPointerId(index);

					GLES3JNILib.TouchEnd(touch_id, x, y);
				}
			}
			else if (evt.getActionMasked() == MotionEvent.ACTION_CANCEL)
			{
				for (int index = 0; index < evt.getPointerCount(); index++)
				{
					int touch_id = evt.getPointerId(index);

					GLES3JNILib.TouchCancel(touch_id);
				}
			}

			return true;
		}
		return false;
	}


    private static class Renderer implements GLSurfaceView.Renderer {
        public void onDrawFrame(GL10 gl) {
            GLES3JNILib.step();
        }

        public void onSurfaceChanged(GL10 gl, int width, int height) {
            GLES3JNILib.resize(width, height);
        }

        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            GLES3JNILib.init();
        }
    }
}
