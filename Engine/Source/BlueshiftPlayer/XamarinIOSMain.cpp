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

#include "Precompiled.h"
#include "Application.h"
#include "Renderer/EGLUtil.h"

EGLUtil mEgl;

bool	Initialized;

static void DisplayContext(BE1::RHI::Handle context, void *dataPtr) {
    static int t0 = 0;
    
    if (t0 == 0) {
        t0 = BE1::PlatformTime::Milliseconds();
    }
    
    int t = BE1::PlatformTime::Milliseconds();
    int elapsedMsec = t - t0;
    if (elapsedMsec > 500)
        elapsedMsec = 500;
    
    BE1::Engine::RunFrame(elapsedMsec);
    
    BE1::gameClient.RunFrame();
    
    app.Update();
    
    BE1::gameClient.EndFrame();
    
    app.Draw();
    
    t0 = t;
}

extern "C" void XamarinInit_(int w, int h, int windowWidth, int windowHeight)
{
    mEgl.m_width = w;
    mEgl.m_height = h;
    mEgl.m_windowWidth = windowWidth;
    mEgl.m_windowHeight = windowHeight;
    if (!Initialized) {
        Initialized = true;
        // ----- Core initialization -----
        BE1::Engine::InitParms initParms;
        
        BE1::Str appDir = BE1::PlatformFile::ExecutablePath();
        initParms.baseDir = appDir;
        
        BE1::Str dataDir = appDir + "/Data";
        initParms.searchPath = dataDir;
        
        BE1::Engine::Init(&initParms);
        
        // -------------------------------
        
        //BE1::resourceGuidMapper.Read("Data/guidmap");
        BE1::resourceGuidMapper.Read("Data/guidmap");
        
        // mainWindow(UIWindow) - rootViewController.view(UIView) - eaglView(EAGLView)
        //CGRect screenBounds = [[UIScreen mainScreen] bounds];
        //mainWindow = [[UIWindow alloc] initWithFrame:screenBounds];
        //mainWindow.backgroundColor = [UIColor blackColor];
        
        //rootViewController = [[RootViewController alloc] init];
        //mainWindow.rootViewController = rootViewController;
        
        //[mainWindow makeKeyAndVisible];
        
        //BE1::gameClient.Init((__bridge BE1::RHI::WindowHandle)mainWindow, true);
        BE1::gameClient.Init(0 /*&mEgl*/, true);
        
        //float retinaScale = [[UIScreen mainScreen] scale];
        //BE1::Vec2 screenScaleFactor(0.75f, 0.75f);
        //int renderWidth = screenBounds.size.width * retinaScale * screenScaleFactor.x;
        //int renderHeight = screenBounds.size.height * retinaScale* screenScaleFactor.y;
        
        //app.mainRenderContext = BE1::renderSystem.AllocRenderContext(true);
        app.mainRenderContext = BE1::renderSystem.AllocRenderContext(true);
        //app.mainRenderContext->Init((__bridge BE1::RHI::WindowHandle)[rootViewController view],
        //	renderWidth, renderHeight, DisplayContext, NULL);
        {
            //int w = mEgl.getWidth();
            //int h = mEgl.getHeight();
            app.mainRenderContext->Init(&mEgl, w, h, DisplayContext, 0);
        }
        
        //app.Init();
        app.Init();
        
        ////BE1::cmdSystem.BufferCommandText(BE1::CmdSystem::Append, L"exec \"autoexec.cfg\"\n");

    }
}

extern "C" void XamarinDeinit_()
{
    if (Initialized)
    {
        Initialized = false;

        app.OnApplicationTerminate();
        
        app.Shutdown();
        
        app.mainRenderContext->Shutdown();
        BE1::renderSystem.FreeRenderContext(app.mainRenderContext);
        
        //DestroyRenderWindow(mainWnd);
        
        BE1::gameClient.Shutdown();
        
        BE1::Engine::Shutdown();

    }
}

extern "C" void XamarinRender_()
{
    DisplayContext(BE1::RHI::NullContext, 0);
}

extern "C" void XamarinTouchesBegan_(uint64_t touchId, int x, int y)
{
    uint64_t locationQword = BE1::MakeQWord(x, y);
    BE1::platform->QueEvent(BE1::Platform::KeyEvent, BE1::KeyCode::Mouse1, true, 0, NULL);
    BE1::platform->QueEvent(BE1::Platform::MouseMoveEvent, x, y, 0, NULL);
    BE1::platform->QueEvent(BE1::Platform::TouchBeganEvent, touchId, locationQword, 0, NULL);
    
}

extern "C" void XamarinTouchesMoved_(uint64_t touchId, int x, int y)
{
    uint64_t locationQword = BE1::MakeQWord(x, y);
    BE1::platform->QueEvent(BE1::Platform::MouseMoveEvent, x, y, 0, NULL);
    BE1::platform->QueEvent(BE1::Platform::TouchMovedEvent, touchId, locationQword, 0, NULL);
    
}

extern "C" void XamarinTouchesEnded_(uint64_t touchId, int x, int y)
{
    uint64_t locationQword = BE1::MakeQWord(x, y);
    BE1::platform->QueEvent(BE1::Platform::KeyEvent, BE1::KeyCode::Mouse1, false, 0, NULL);
    BE1::platform->QueEvent(BE1::Platform::TouchEndedEvent, touchId, locationQword, 0, NULL);
    
}

extern "C" void XamarinTouchesCancelled_(uint64_t touchId)
{
    BE1::platform->QueEvent(BE1::Platform::KeyEvent, BE1::KeyCode::Mouse1, false, 0, NULL);
    BE1::platform->QueEvent(BE1::Platform::TouchCanceledEvent, touchId, 0, 0, NULL);
    
}


