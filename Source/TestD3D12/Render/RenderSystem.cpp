// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "RenderSystem.h"
#include "RenderContext.h"
#include "RenderBackend.h"
#include "D3D12Renderer/D3D12Renderer.h"
#include "Sampler.h"
#include "Texture.h"
#include "Font.h"
#include "Mesh.h"

RenderSystem *      renderSystem = nullptr;

void RenderSystem::Init(void *mainWindowHandle) {
    BE1::cmdSystem.AddCommand("screenshot", Cmd_ScreenShot);

    RHI::renderer = new D3D12Renderer;
    RHI::renderer->Init(mainWindowHandle);

    samplerManager.Init();

    textureManager.Init();

    fontManager.Init();

    meshManager.Init();

    backEnd = new RenderBackend;
    backEnd->Init();
}

void RenderSystem::Shutdown() {
    BE1::cmdSystem.RemoveCommand("screenshot");

    backEnd->Shutdown();
    SAFE_DELETE(backEnd);

    meshManager.Shutdown();

    fontManager.Shutdown();

    textureManager.Shutdown();

    samplerManager.Shutdown();

    RHI::renderer->Shutdown();
    SAFE_DELETE(RHI::renderer);
}

RenderContext *RenderSystem::CreateRenderContext(void *windowHandle, bool isMain) {
    RenderContext *renderContext = new RenderContext;
#ifdef USE_RENDER_THREAD
    bool useRenderThread = true;
#else
    bool useRenderThread = false;
#endif
    renderContext->Init(windowHandle, useRenderThread);

    if (isMain) {
        assert(!mainRenderContext);
        mainRenderContext = renderContext;
    }

    return renderContext;
}

void RenderSystem::DestroyRenderContext(RenderContext *renderContext) {
    renderContext->Shutdown();
    delete renderContext;
}

void RenderSystem::Cmd_ScreenShot(const BE1::CmdArgs &args) {
    char path[1024];

    BE1::Str documentDir = BE1::fileSystem.GetUserDocumentDir();

    if (args.Argc() > 1) {
        BE1::Str::snPrintf(path, sizeof(path), "%s/Screenshots/%s", documentDir.c_str(), args.Argv(1));
    } else {
        char filename[16];
        strcpy(filename, "shot000.png");

        int index;
        for (index = 0; index <= 999; index++) {
            filename[4] = '0' + index / 100;
            filename[5] = '0' + (index % 100) / 10;
            filename[6] = '0' + index % 10;
            BE1::Str::snPrintf(path, sizeof(path), "%s/Screenshots/%s", documentDir.c_str(), filename);
            if (!BE1::fileSystem.FileExists(path)) {
                break;
            }
        }

        if (index == 1000) {
            BE_WARNLOG("too many screenshot exist\n");
            return;
        }
    }

    RenderContext *renderContext = renderSystem->mainRenderContext;
    if (renderContext->IsUsingRenderThread()) {
        renderContext->WaitRenderCompleted();
    }

    RenderFrameData *currentFrameData = renderContext->GetCurrentFrameData();
    currentFrameData->CmdScreenshot(0, 0, renderContext->GetSwapChain()->GetWidth(), renderContext->GetSwapChain()->GetHeight(), path);
}
