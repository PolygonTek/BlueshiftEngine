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
#include "Render/Render.h"
#include "Components/ComCanvas.h"
#include "Components/ComRectTransform.h"
#include "Game/Entity.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Canvas", ComCanvas, Component)
BEGIN_EVENTS(ComCanvas)
END_EVENTS

void ComCanvas::RegisterProperties() {
}

ComCanvas::ComCanvas() {
    renderCamera = nullptr;
}

ComCanvas::~ComCanvas() {
}

void ComCanvas::Init() {
    Component::Init();

    if (!renderCamera) {
        renderCamera = new RenderCamera;
    }

    renderCameraDef.flags = RenderCamera::Flag::TexturedMode | RenderCamera::Flag::NoSubViews;

    renderCameraDef.origin = Vec3::origin;
    renderCameraDef.axis = Mat3::identity;

    // Mark as initialized
    SetInitialized(true);
}

#if WITH_EDITOR
void ComCanvas::DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) {
    int screenWidth = 100;
    int screenHeight = 100;

    const RenderContext *ctx = renderSystem.GetMainRenderContext();
    if (ctx) {
        screenWidth = ctx->GetScreenWidth();
        screenHeight = ctx->GetScreenHeight();
    }

    renderCameraDef.sizeX = screenWidth;
    renderCameraDef.sizeY = screenHeight;

    ComRectTransform *rectTransform = GetEntity()->GetComponent<ComRectTransform>();
    if (rectTransform) {
        rectTransform->SetProperty("sizeDelta", Vec2(screenWidth, screenHeight));
    }
}
#endif

const AABB ComCanvas::GetAABB() {
    int screenWidth = 100;
    int screenHeight = 100;

    const RenderContext *ctx = renderSystem.GetMainRenderContext();
    if (ctx) {
        screenWidth = ctx->GetScreenWidth();
        screenHeight = ctx->GetScreenHeight();
    }

    Vec3 mins(-screenWidth * 0.5f, -screenHeight * 0.5f, 0);
    Vec3 maxs(+screenWidth * 0.5f, +screenHeight * 0.5f, 0);

    return AABB(mins, maxs);
}

void ComCanvas::Render() {
    // Get current render context which is unique for each OS-level window in general.
    const RenderContext *ctx = renderSystem.GetCurrentRenderContext();
    if (!ctx) {
        return;
    }

    // Get the actual screen rendering resolution.
    float renderingWidth = ctx->GetRenderingWidth();
    float renderingHeight = ctx->GetRenderingHeight();

    renderCameraDef.sizeX = renderingWidth;
    renderCameraDef.sizeY = renderingHeight;

    // Update render camera with the given parameters.
    renderCamera->Update(&renderCameraDef);

    // Render scene with the given camera.
    GetGameWorld()->GetRenderWorld()->RenderScene(renderCamera);
}

BE_NAMESPACE_END
