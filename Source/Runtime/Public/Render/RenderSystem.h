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

#pragma once

BE_NAMESPACE_BEGIN

/*
-------------------------------------------------------------------------------

    Render System

-------------------------------------------------------------------------------
*/

class CmdArgs;
class VisibleView;

class RenderSystem {
    friend class RenderContext;
    friend class RenderWorld;
    friend class PhysDebugDraw;

public:
    RenderSystem();

    void                    Init(void *windowHandle, const RHI::Settings *settings);
    void                    Shutdown();

    bool                    IsInitialized() const { return initialized; }

    bool                    IsFullscreen() const;

    void                    SetGamma(double gamma);
    void                    RestoreGamma();

    RenderContext *         AllocRenderContext(bool isMainContext = false);
    void                    FreeRenderContext(RenderContext *renderContext);

                            // valid only between BeginFrame()/EndFrame()
    RenderContext *         GetCurrentRenderContext() { return currentContext; }

    RenderContext *         GetMainRenderContext() { return mainContext; }

    RenderWorld *           AllocRenderWorld();
    void                    FreeRenderWorld(RenderWorld *renderWorld);

    RenderWorld *           GetRenderWorld() const { return primaryWorld; }

    void                    CheckModifiedCVars();

private:
    void                    RecreateScreenMapRT();
    void                    RecreateHDRMapRT();
    void                    RecreateShadowMapRT();
    void *                  GetCommandBuffer(int bytes);
    void                    IssueCommands();

    void                    CmdDrawView(const VisibleView *visView);
    void                    CmdScreenshot(int x, int y, int width, int height, const char *filename);

    bool                    initialized;
    unsigned short          savedGammaRamp[768];

    RenderWorld *           primaryWorld;
    
    Array<RenderContext *>  renderContexts;
    RenderContext *         currentContext;
    RenderContext *         mainContext;

    static void             Cmd_ScreenShot(const CmdArgs &args);
};

BE_INLINE RenderSystem::RenderSystem() {
    initialized = false;
}

extern RenderSystem         renderSystem;

BE_NAMESPACE_END
