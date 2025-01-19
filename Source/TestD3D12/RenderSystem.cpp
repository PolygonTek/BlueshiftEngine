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
#include "RenderSystem.h"
#include "RenderContext.h"
#include "RenderBackEnd.h"
#include "D3D12Renderer/D3D12Renderer.h"

RenderSystem *      renderSystem = nullptr;

void RenderSystem::Init(void *mainWindowHandle) {
    RHI::renderer = new D3D12Renderer;
    RHI::renderer->Init(mainWindowHandle);

    backEnd = new RenderBackEnd;
    backEnd->Init();
}

void RenderSystem::Shutdown() {
    backEnd->Shutdown();
    SAFE_DELETE(backEnd);

    RHI::renderer->Shutdown();
    SAFE_DELETE(RHI::renderer);
}

RenderContext *RenderSystem::CreateRenderContext(void *windowHandle) {
    RenderContext *renderContext = new RenderContext;
#ifdef USE_RENDER_THREAD
    bool useRenderThread = true;
#else
    bool useRenderThread = false;
#endif
    renderContext->Init(windowHandle, useRenderThread);
    return renderContext;
}

void RenderSystem::DestroyRenderContext(RenderContext *renderContext) {
    renderContext->Shutdown();
    delete renderContext;
}
