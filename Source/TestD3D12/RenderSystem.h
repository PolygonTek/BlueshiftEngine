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

class RenderContext;
class RenderBackEnd;

class RenderSystem {
public:
    void                        Init(void *mainWindowHandle);
    void                        Shutdown();

    RenderContext *             CreateRenderContext(void *windowHandle, bool isMain = false);
    void                        DestroyRenderContext(RenderContext *renderContext);

    RenderBackEnd *             GetBackEnd() const { return backEnd; }

private:
    static void                 Cmd_ScreenShot(const BE1::CmdArgs &args);

    RenderContext *             mainRenderContext = nullptr;
    RenderBackEnd *             backEnd = nullptr;
};

extern RenderSystem *           renderSystem;
