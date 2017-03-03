#define _ENGINE
//#define _SAMPLE
using System;
using System.Diagnostics;

using Foundation;
using GLKit;
using OpenGLES;
using OpenTK;


#if _SAMPLE
using OpenTK.Graphics.ES20;
#endif
#if _ENGINE
using OpenTK.Graphics.ES30;
using UIKit;
using System.Collections.Generic;
using System.Runtime.InteropServices;
//using System.Drawing;
using CoreGraphics;
using XMBindingLibrarySample;
#endif

namespace XamarinPlayer
{
	[Register ("GameViewController")]
	public class GameViewController : GLKViewController, IGLKViewDelegate
	{
		#if _SAMPLE
		enum Uniform
		{
			ModelViewProjection_Matrix,
			Normal_Matrix,
			Count
		}

		enum Attribute
		{
			Vertex,
			Normal,
			Count
		}

		int[] uniforms = new int [(int)Uniform.Count];

		float[] cubeVertexData = {
			// Data layout for each line below is:
			// positionX, positionY, positionZ,     normalX, normalY, normalZ,
			0.5f, -0.5f, -0.5f,        1.0f, 0.0f, 0.0f,
			0.5f, 0.5f, -0.5f,         1.0f, 0.0f, 0.0f,
			0.5f, -0.5f, 0.5f,         1.0f, 0.0f, 0.0f,
			0.5f, -0.5f, 0.5f,         1.0f, 0.0f, 0.0f,
			0.5f, 0.5f, -0.5f,          1.0f, 0.0f, 0.0f,
			0.5f, 0.5f, 0.5f,         1.0f, 0.0f, 0.0f,

			0.5f, 0.5f, -0.5f,         0.0f, 1.0f, 0.0f,
			-0.5f, 0.5f, -0.5f,        0.0f, 1.0f, 0.0f,
			0.5f, 0.5f, 0.5f,          0.0f, 1.0f, 0.0f,
			0.5f, 0.5f, 0.5f,          0.0f, 1.0f, 0.0f,
			-0.5f, 0.5f, -0.5f,        0.0f, 1.0f, 0.0f,
			-0.5f, 0.5f, 0.5f,         0.0f, 1.0f, 0.0f,

			-0.5f, 0.5f, -0.5f,        -1.0f, 0.0f, 0.0f,
			-0.5f, -0.5f, -0.5f,       -1.0f, 0.0f, 0.0f,
			-0.5f, 0.5f, 0.5f,         -1.0f, 0.0f, 0.0f,
			-0.5f, 0.5f, 0.5f,         -1.0f, 0.0f, 0.0f,
			-0.5f, -0.5f, -0.5f,       -1.0f, 0.0f, 0.0f,
			-0.5f, -0.5f, 0.5f,        -1.0f, 0.0f, 0.0f,

			-0.5f, -0.5f, -0.5f,       0.0f, -1.0f, 0.0f,
			0.5f, -0.5f, -0.5f,        0.0f, -1.0f, 0.0f,
			-0.5f, -0.5f, 0.5f,        0.0f, -1.0f, 0.0f,
			-0.5f, -0.5f, 0.5f,        0.0f, -1.0f, 0.0f,
			0.5f, -0.5f, -0.5f,        0.0f, -1.0f, 0.0f,
			0.5f, -0.5f, 0.5f,         0.0f, -1.0f, 0.0f,

			0.5f, 0.5f, 0.5f,          0.0f, 0.0f, 1.0f,
			-0.5f, 0.5f, 0.5f,         0.0f, 0.0f, 1.0f,
			0.5f, -0.5f, 0.5f,         0.0f, 0.0f, 1.0f,
			0.5f, -0.5f, 0.5f,         0.0f, 0.0f, 1.0f,
			-0.5f, 0.5f, 0.5f,         0.0f, 0.0f, 1.0f,
			-0.5f, -0.5f, 0.5f,        0.0f, 0.0f, 1.0f,

			0.5f, -0.5f, -0.5f,        0.0f, 0.0f, -1.0f,
			-0.5f, -0.5f, -0.5f,       0.0f, 0.0f, -1.0f,
			0.5f, 0.5f, -0.5f,         0.0f, 0.0f, -1.0f,
			0.5f, 0.5f, -0.5f,         0.0f, 0.0f, -1.0f,
			-0.5f, -0.5f, -0.5f,       0.0f, 0.0f, -1.0f,
			-0.5f, 0.5f, -0.5f,        0.0f, 0.0f, -1.0f
		};

		int program;

		Matrix4 modelViewProjectionMatrix;
		Matrix3 normalMatrix;
		float rotation;

		uint vertexArray;
		uint vertexBuffer;

		EAGLContext context { get; set; }

		GLKBaseEffect effect { get; set; }
		#endif
		#if _ENGINE
		[DllImport("__Internal")]
		private static extern int XamarinInit(int width, int height, int windowWidth, int windowHeight);

		[DllImport("__Internal")]
		private static extern int XamarinDeinit();

		[DllImport("__Internal")]
		private static extern int XamarinRender();

		[DllImport("__Internal")]
		private static extern int XamarinTouchesBegan(UInt64 touchId, int x, int y);

		[DllImport("__Internal")]
		private static extern int XamarinTouchesMoved(UInt64 touchId, int x, int y);

		[DllImport("__Internal")]
		private static extern int XamarinTouchesEnded(UInt64 touchId, int x, int y);

		[DllImport("__Internal")]
		private static extern int XamarinTouchesCancelled(UInt64 touchId);

		EAGLContext context { get; set; }
		bool Initialized;
		XMUtilities Utility;
		#endif

		[Export ("initWithCoder:")]
		public GameViewController (NSCoder coder) : base (coder)
		{
		}

		public override void ViewDidLoad ()
		{
			base.ViewDidLoad ();

			#if _SAMPLE
			context = new EAGLContext (EAGLRenderingAPI.OpenGLES2);
			#endif
			#if _ENGINE
			context = new EAGLContext (EAGLRenderingAPI.OpenGLES3);
			#endif

			if (context == null) {
				Debug.WriteLine ("Failed to create ES context");
			}

			var view = (GLKView)View;
			view.Context = context;
			view.DrawableDepthFormat = GLKViewDrawableDepthFormat.Format24;
			#if _ENGINE
			view.MultipleTouchEnabled = true;
			int scaling = (int)UIScreen.MainScreen.Scale;
			if (scaling > 1) 
				view.ContentScaleFactor = (nfloat) 1.5;
			#endif
			SetupGL ();
		}
		#if _ENGINE
		public override void TouchesBegan (NSSet touches, UIEvent evt)
		{
			foreach (UITouch touch in touches.ToArray<UITouch> ()){
				CGPoint location = touch.LocationInView (View);
				UInt64 touchId = touch.GetNativeHash();
				XamarinTouchesBegan(touchId, (int) location.X, (int) location.Y);
			}
		} 

		public override void TouchesMoved (NSSet touches, UIEvent evt)
		{
			foreach (UITouch touch in touches.ToArray<UITouch> ()){
				CGPoint location = touch.LocationInView (View);
				UInt64 touchId = touch.GetNativeHash();
				XamarinTouchesMoved(touchId, (int) location.X, (int) location.Y);
			}
		} 

		public override void TouchesEnded (NSSet touches, UIEvent evt)
		{
			foreach (UITouch touch in touches.ToArray<UITouch> ()){
				CGPoint location = touch.LocationInView (View);
				UInt64 touchId = touch.GetNativeHash();
				XamarinTouchesEnded(touchId, (int) location.X, (int) location.Y);
			}
		} 

		public override void TouchesCancelled (NSSet touches, UIEvent evt)
		{
			foreach (UITouch touch in touches.ToArray<UITouch> ()){
				UInt64 touchId = touch.GetNativeHash();
				XamarinTouchesCancelled(touchId);
			}
		} 

		#endif
		protected override void Dispose (bool disposing)
		{
			base.Dispose (disposing);

			TearDownGL ();

			if (EAGLContext.CurrentContext == context)
				EAGLContext.SetCurrentContext (null);
		}

		public override void DidReceiveMemoryWarning ()
		{
			base.DidReceiveMemoryWarning ();

			if (IsViewLoaded && View.Window == null) {
				View = null;

				TearDownGL ();

				if (EAGLContext.CurrentContext == context) {
					EAGLContext.SetCurrentContext (null);
				}
			}

			// Dispose of any resources that can be recreated.
		}

		public override bool PrefersStatusBarHidden ()
		{
			return true;
		}

		void SetupGL ()
		{
			EAGLContext.SetCurrentContext (context);

			#if _SAMPLE
			LoadShaders ();

			effect = new GLKBaseEffect ();
			effect.Light0.Enabled = true;
			effect.Light0.DiffuseColor = new Vector4 (1.0f, 0.4f, 0.4f, 1.0f);

			GL.Enable (EnableCap.DepthTest);

			GL.Oes.GenVertexArrays (1, out vertexArray);
			GL.Oes.BindVertexArray (vertexArray);

			GL.GenBuffers (1, out vertexBuffer);
			GL.BindBuffer (BufferTarget.ArrayBuffer, vertexBuffer);
			GL.BufferData (BufferTarget.ArrayBuffer, (IntPtr)(cubeVertexData.Length * sizeof(float)), cubeVertexData, BufferUsage.StaticDraw);

			GL.EnableVertexAttribArray ((int)GLKVertexAttrib.Position);
			GL.VertexAttribPointer ((int)GLKVertexAttrib.Position, 3, VertexAttribPointerType.Float, false, 24, new IntPtr (0));
			GL.EnableVertexAttribArray ((int)GLKVertexAttrib.Normal);
			GL.VertexAttribPointer ((int)GLKVertexAttrib.Normal, 3, VertexAttribPointerType.Float, false, 24, new IntPtr (12));

			GL.Oes.BindVertexArray (0);
			#endif
		}

		void TearDownGL ()
		{
			EAGLContext.SetCurrentContext (context);
			#if _SAMPLE
			GL.DeleteBuffers (1, ref vertexBuffer);
			GL.Oes.DeleteVertexArrays (1, ref vertexArray);

			effect = null;

			if (program > 0) {
				GL.DeleteProgram (program);
				program = 0;
			}
			#endif
			#if _ENGINE
			if (Initialized) {
				Initialized = false;
				XamarinDeinit();
			}
			#endif
		}

		#region GLKView and GLKViewController delegate methods

		public override void Update ()
		{
			#if _SAMPLE
			var aspect = (float)Math.Abs (View.Bounds.Size.Width / View.Bounds.Size.Height);
			var projectionMatrix = Matrix4.CreatePerspectiveFieldOfView (MathHelper.DegreesToRadians (65.0f), aspect, 0.1f, 100.0f);

			effect.Transform.ProjectionMatrix = projectionMatrix;

			var baseModelViewMatrix = Matrix4.CreateTranslation (0.0f, 0.0f, -4.0f);
			baseModelViewMatrix = Matrix4.CreateFromAxisAngle (new Vector3 (0.0f, 1.0f, 0.0f), rotation) * baseModelViewMatrix;

			// Compute the model view matrix for the object rendered with GLKit
			var modelViewMatrix = Matrix4.CreateTranslation (0.0f, 0.0f, -1.5f);
			modelViewMatrix = Matrix4.CreateFromAxisAngle (new Vector3 (1.0f, 1.0f, 1.0f), rotation) * modelViewMatrix;
			modelViewMatrix = modelViewMatrix * baseModelViewMatrix;

			effect.Transform.ModelViewMatrix = modelViewMatrix;

			// Compute the model view matrix for the object rendered with ES2
			modelViewMatrix = Matrix4.CreateTranslation (0.0f, 0.0f, 1.5f);
			modelViewMatrix = Matrix4.CreateFromAxisAngle (new Vector3 (1.0f, 1.0f, 1.0f), rotation) * modelViewMatrix;
			modelViewMatrix = modelViewMatrix * baseModelViewMatrix;

			normalMatrix = new Matrix3 (Matrix4.Transpose (Matrix4.Invert (modelViewMatrix)));

			modelViewProjectionMatrix = modelViewMatrix * projectionMatrix;

			rotation += (float)TimeSinceLastUpdate * 0.5f;
			#endif
		}

		void IGLKViewDelegate.DrawInRect (GLKView view, CoreGraphics.CGRect rect)
		{
			#if _SAMPLE
			GL.ClearColor (0.65f, 0.65f, 0.65f, 1.0f);
			GL.Clear (ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit);

			GL.Oes.BindVertexArray (vertexArray);

			// Render the object with GLKit
			effect.PrepareToDraw ();

			GL.DrawArrays (BeginMode.Triangles, 0, 36);

			// Render the object again with ES2
			GL.UseProgram (program);

			GL.UniformMatrix4 (uniforms [(int)Uniform.ModelViewProjection_Matrix], false, ref modelViewProjectionMatrix);
			GL.UniformMatrix3 (uniforms [(int)Uniform.Normal_Matrix], false, ref normalMatrix);

			GL.DrawArrays (BeginMode.Triangles, 0, 36);
			#endif
			#if _ENGINE
			if (!Initialized) {
				Initialized = true;

				int [] viewport = new int[4];
				GL.GetInteger(GetPName.Viewport, viewport);
				//CGSize size = View.Bounds.Size;
				//int DeviceWidth = (int) UIScreen.MainScreen.Bounds.Width ;
				//int DeviceHeight = (int) UIScreen.MainScreen.Bounds.Height;
				Utility = new XMUtilities();
				XamarinInit(viewport[2], viewport[3], (int) rect.Width, (int) rect.Height);
			}
			XamarinRender();
			#endif
		}
		#if _SAMPLE
		bool LoadShaders ()
		{
			int vertShader, fragShader;

			// Create shader program.
			program = GL.CreateProgram ();

			// Create and compile vertex shader.
			if (!CompileShader (ShaderType.VertexShader, LoadResource ("Shader", "vsh"), out vertShader)) {
				Console.WriteLine ("Failed to compile vertex shader");
				return false;
			}
			// Create and compile fragment shader.
			if (!CompileShader (ShaderType.FragmentShader, LoadResource ("Shader", "fsh"), out fragShader)) {
				Console.WriteLine ("Failed to compile fragment shader");
				return false;
			}

			// Attach vertex shader to program.
			GL.AttachShader (program, vertShader);

			// Attach fragment shader to program.
			GL.AttachShader (program, fragShader);

			// Bind attribute locations.
			// This needs to be done prior to linking.
			GL.BindAttribLocation (program, (int)GLKVertexAttrib.Position, "position");
			GL.BindAttribLocation (program, (int)GLKVertexAttrib.Normal, "normal");

			// Link program.
			if (!LinkProgram (program)) {
				Console.WriteLine ("Failed to link program: {0:x}", program);

				if (vertShader != 0)
					GL.DeleteShader (vertShader);

				if (fragShader != 0)
					GL.DeleteShader (fragShader);

				if (program != 0) {
					GL.DeleteProgram (program);
					program = 0;
				}
				return false;
			}

			// Get uniform locations.
			uniforms [(int)Uniform.ModelViewProjection_Matrix] = GL.GetUniformLocation (program, "modelViewProjectionMatrix");
			uniforms [(int)Uniform.Normal_Matrix] = GL.GetUniformLocation (program, "normalMatrix");

			// Release vertex and fragment shaders.
			if (vertShader != 0) {
				GL.DetachShader (program, vertShader);
				GL.DeleteShader (vertShader);
			}

			if (fragShader != 0) {
				GL.DetachShader (program, fragShader);
				GL.DeleteShader (fragShader);
			}

			return true;
		}

		string LoadResource (string name, string type)
		{
			var path = NSBundle.MainBundle.PathForResource (name, type);
			return System.IO.File.ReadAllText (path);
		}
		#endif
		#endregion
		#if _SAMPLE

		bool CompileShader (ShaderType type, string src, out int shader)
		{
			shader = GL.CreateShader (type);
			GL.ShaderSource (shader, src);
			GL.CompileShader (shader);

			#if DEBUG
			int logLength = 0;
			GL.GetShader (shader, ShaderParameter.InfoLogLength, out logLength);
			if (logLength > 0) {
				Console.WriteLine ("Shader compile log:\n{0}", GL.GetShaderInfoLog (shader));
			}
			#endif

			int status = 0;
			GL.GetShader (shader, ShaderParameter.CompileStatus, out status);
			if (status == 0) {
				GL.DeleteShader (shader);
				return false;
			}

			return true;
		}

		bool LinkProgram (int prog)
		{
			GL.LinkProgram (prog);

			#if DEBUG
			int logLength = 0;
			GL.GetProgram (prog, ProgramParameter.InfoLogLength, out logLength);
			if (logLength > 0)
				Console.WriteLine ("Program link log:\n{0}", GL.GetProgramInfoLog (prog));
			#endif
			int status = 0;
			GL.GetProgram (prog, ProgramParameter.LinkStatus, out status);
			return status != 0;
		}

		bool ValidateProgram (int prog)
		{
			int logLength, status = 0;

			GL.ValidateProgram (prog);
			GL.GetProgram (prog, ProgramParameter.InfoLogLength, out logLength);
			if (logLength > 0) {
				var log = new System.Text.StringBuilder (logLength);
				GL.GetProgramInfoLog (prog, logLength, out logLength, log);
				Console.WriteLine ("Program validate log:\n{0}", log);
			}

			GL.GetProgram (prog, ProgramParameter.LinkStatus, out status);
			return status != 0;
		}
		#endif
	}
}

