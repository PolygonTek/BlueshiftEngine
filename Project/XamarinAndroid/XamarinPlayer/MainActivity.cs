/*
 * Copyright (C) 2009 The Android Open Source Project
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
 *
 * This is a small port of the "San Angeles Observation" demo
 * program for OpenGL ES 1.x. For more details, see:
 *
 *    http://jet.ro/visuals/san-angeles-observation/
 *
 * This program demonstrates how to use a GLSurfaceView from Java
 * along with native OpenGL calls to perform frame rendering.
 *
 * Touching the screen will start/stop the animation.
 *
 * Note that the demo runs much faster on the emulator than on
 * real devices, this is mainly due to the following facts:
 *
 * - the demo sends bazillions of polygons to OpenGL without
 *   even trying to do culling. Most of them are clearly out
 *   of view.
 *
 * - on a real device, the GPU bus is the real bottleneck
 *   that prevent the demo from getting acceptable performance.
 *
 * - the software OpenGL engine used in the emulator uses
 *   the system bus instead, and its code rocks :-)
 *
 * Fixing the program to send less polygons to the GPU is left
 * as an exercise to the reader. As always, patches welcomed :-)
 */
#define _ENGINE 

using System;

using Javax.Microedition.Khronos.Egl;
using Javax.Microedition.Khronos.Opengles;

using Android.App;
using Android.Content;
using Android.Opengl;
using Android.OS;
using Android.Views;
using System.Runtime.InteropServices;

using Android.Runtime;
using Android.Content.Res;


namespace XamarinPlayer
{
#if !PORTRAIT
    [Activity(Label = "XamarinPlayer sample", MainLauncher = true, ScreenOrientation = Android.Content.PM.ScreenOrientation.Landscape)]
#else
    [Activity(Label = "XamarinPlayer sample", MainLauncher = true, ScreenOrientation = Android.Content.PM.ScreenOrientation.Portrait)]
#endif
    public class DemoActivity : Activity {

        [DllImport("XamarinPlayer")]
        static extern void SetAssetManager(IntPtr env, IntPtr asset, string path);

        protected override void OnCreate (Bundle savedInstanceState) 
		{
			base.OnCreate (savedInstanceState);

            Window.RequestFeature(WindowFeatures.NoTitle);
            Window.AddFlags(WindowManagerFlags.Fullscreen);

            mGLView = new DemoGLSurfaceView(this);
            SetContentView(mGLView);

            if (!PackageName.Equals("com.polygontek.devtech.AndroidPlayer"))
            {
                SetAssetManager(JNIEnv.Handle, Assets.Handle, FilesDir.AbsolutePath);
            }
            else
            {
                SetAssetManager(JNIEnv.Handle, Assets.Handle, "/sdcard/blueshift");
            }

        }

        protected override void OnPause () 
		{
			base.OnPause();
			mGLView.OnPause ();
		}
		
		protected override void OnResume () 
		{
			base.OnResume ();
			mGLView.OnResume ();
		}
		
		private GLSurfaceView mGLView;
	}

	class DemoGLSurfaceView : GLSurfaceView
	{
        public DemoGLSurfaceView (Context context) 
				: base (context)
		{
#if _ENGINE
            SetEGLContextClientVersion(3);
#endif

            mRenderer = new DemoRenderer();
            SetRenderer(mRenderer);
        }

        public override bool OnTouchEvent (MotionEvent evt) 
		{
#if _ENGINE
            if (evt.ActionMasked == MotionEventActions.Down || evt.ActionMasked == MotionEventActions.Pointer1Down)
            {
                int x = (int)evt.GetX(evt.ActionIndex);
                int y = (int)evt.GetY(evt.ActionIndex);
                int touch_id = evt.GetPointerId(evt.ActionIndex);
                nativeTouchBegin(touch_id, x, y);
            }
            else if (evt.ActionMasked == MotionEventActions.Move)
            {
                for (int index = 0; index < evt.PointerCount; index++)
                {
                    int x = (int)evt.GetX(index);
                    int y = (int)evt.GetY(index);
                    int touch_id = evt.GetPointerId(index);

                    nativeTouchMove(touch_id, x, y);
                }
            }
            else if (evt.ActionMasked == MotionEventActions.Up || evt.ActionMasked == MotionEventActions.Pointer1Up)
            {
                int x = (int)evt.GetX(evt.ActionIndex);
                int y = (int)evt.GetY(evt.ActionIndex);
                int touch_id = evt.GetPointerId(evt.ActionIndex);
                nativeTouchEnd(touch_id, x, y);
            }
            else if (evt.ActionMasked == MotionEventActions.Cancel)
            {
                for (int index = 0; index < evt.PointerCount; index++)
                {
                    int touch_id = evt.GetPointerId(index);

                    nativeTouchCancel(touch_id);
                }
            }

#else
            if (evt.Action == MotionEventActions.Down)
				nativePause (IntPtr.Zero);
#endif
            return true;
		}
		
		DemoRenderer mRenderer;

		[DllImport ("XamarinPlayer", EntryPoint = "Java_com_example_XamarinPlayer_DemoGLSurfaceView_nativePause")]
		static extern void nativePause (IntPtr jnienv);
        [DllImport("XamarinPlayer", EntryPoint = "XamarinPlayer_TouchBegin")]
        static extern void nativeTouchBegin(Int32 id, Int32 x, Int32 y);
        [DllImport("XamarinPlayer", EntryPoint = "XamarinPlayer_TouchMove")]
        static extern void nativeTouchMove(Int32 id, Int32 x, Int32 y);
        [DllImport("XamarinPlayer", EntryPoint = "XamarinPlayer_TouchEnd")]
        static extern void nativeTouchEnd(Int32 id, Int32 x, Int32 y);
        [DllImport("XamarinPlayer", EntryPoint = "XamarinPlayer_TouchCancel")]
        static extern void nativeTouchCancel(Int32 id);
    }

    class DemoRenderer : Java.Lang.Object, GLSurfaceView.IRenderer
	{
        delegate int MyCallback1(int a);
        delegate string MyCallback2(string a);

        [DllImport("XamarinPlayer")]
        extern static void RegisterCallback1(MyCallback1 callback1);
        [DllImport("XamarinPlayer")]
        extern static void RegisterCallback2(MyCallback2 callback2);

        static int Add1(int a)
        {
            return a;
        }
        static string Add2(string a)
        {
            return a + "#";
        }

        public void OnSurfaceCreated (IGL10 gl, Javax.Microedition.Khronos.Egl.EGLConfig config) 
		{
            RegisterCallback1(Add1);
            RegisterCallback2(Add2);

            nativeInit(IntPtr.Zero);
		}
		
		public void OnSurfaceChanged (IGL10 gl, int w, int h) 
		{
			//gl.glViewport(0, 0, w, h);
			nativeResize (IntPtr.Zero, IntPtr.Zero, w, h);
		}
		
		public void OnDrawFrame (IGL10 gl) 
		{
            nativeRender (IntPtr.Zero);
#if _ENGINE && !DEBUG
            System.GC.Collect(GC.MaxGeneration, GCCollectionMode.Forced);
#endif
        }

        [DllImport ("XamarinPlayer", EntryPoint = "Java_com_example_XamarinPlayer_DemoRenderer_nativeInit")]
		private static extern void nativeInit (IntPtr jnienv);
		[DllImport ("XamarinPlayer", EntryPoint = "Java_com_example_XamarinPlayer_DemoRenderer_nativeResize")]
		private static extern void nativeResize (IntPtr jnienv, IntPtr thiz, int w, int h);
		[DllImport ("XamarinPlayer", EntryPoint = "Java_com_example_XamarinPlayer_DemoRenderer_nativeRender")]
		private static extern void nativeRender (IntPtr jnienv);
		[DllImport ("XamarinPlayer", EntryPoint = "Java_com_example_XamarinPlayer_DemoRenderer_nativeDone")]
		private static extern void nativeDone (IntPtr jnienv);
    }
}
