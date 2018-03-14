using System;
using AppKit;
using System.Runtime.InteropServices;

namespace XamarinControl
{
	static class MainClass
	{
		delegate void CallbackTouch(int x, int y);
		[DllImport("libControl")]
		extern static void RegisterTouch(CallbackTouch callback);
		static void ProcessTouch(int x, int y)
		{
			SendSound2D("boing_x.wav");
		}

		static void Main (string[] args)
		{
			RegisterTouch(ProcessTouch);

			for (;;)
			{
				if (!RecvFrame())
					break;
				SendFrame();
			}

			NSApplication.Init ();

//			var application = NSApplication.SharedApplication;
//			application.Delegate = new AppDelegate ();
//			application.Run ();
		}

		[DllImport("libControl")]
		extern static bool RecvFrame();
		[DllImport("libControl")]
		extern static void SendFrame();
		[DllImport("libControl")]
		extern static void SendSound2D(string name);
	}
}
