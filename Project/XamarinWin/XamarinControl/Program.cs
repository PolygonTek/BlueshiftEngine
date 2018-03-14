using System;
using System.Runtime.InteropServices;


namespace XamarinControl
{
	class MainClass
	{


        delegate void CallbackTouch(int x, int y);
        [DllImport("libControl")]
        extern static void RegisterTouch(CallbackTouch callback);
        static void ProcessTouch(int x, int y)
        {
            SendSound2D("boing_x.wav");
        }


        public static void Main (string[] args)
		{
            RegisterTouch(ProcessTouch);

            for (;;)
            {
                if (!RecvFrame())
                    break;
                SendFrame();
            }
        }

        [DllImport("libControl")]
        extern static bool RecvFrame();
        [DllImport("libControl")]
        extern static void SendFrame();
        [DllImport("libControl")]
        extern static void SendSound2D(string name);

    }



}
