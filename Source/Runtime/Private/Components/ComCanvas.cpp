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
#include "Components/ComLogic.h"
#include "Components/ComScript.h"
#include "Game/Entity.h"
#include "Game/GameWorld.h"
#include "Game/TagLayerSettings.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Canvas", ComCanvas, Component)
BEGIN_EVENTS(ComCanvas)
END_EVENTS

void ComCanvas::RegisterProperties() {
}

ComCanvas::ComCanvas() {
    renderCamera = nullptr;

    mousePointerState.oldHitEntityGuid = Guid::zero;
    mousePointerState.captureEntityGuid = Guid::zero;
}

ComCanvas::~ComCanvas() {
    Purge(false);
}

void ComCanvas::Purge(bool chainPurge) {
    if (renderCamera) {
        delete renderCamera;
        renderCamera = nullptr;
    }

    mousePointerState.oldHitEntityGuid = Guid::zero;
    mousePointerState.captureEntityGuid = Guid::zero;

    touchPointerStateTable.Clear();

    if (chainPurge) {
        Component::Purge();
    }
}

void ComCanvas::Init() {
    Component::Init();

    if (!renderCamera) {
        renderCamera = new RenderCamera;
    }

    renderCameraDef.flags = RenderCamera::Flag::TexturedMode | RenderCamera::Flag::NoSubViews;

    renderCameraDef.clearMethod = RenderCamera::ClearMethod::DepthOnly;

    renderCameraDef.origin = Vec3::origin;
    renderCameraDef.axis = Angles(0, 90, 90).ToMat3();
    renderCameraDef.axis.FixDegeneracies();

    renderCameraDef.zNear = -1000.0f;
    renderCameraDef.zFar = 1000.0f;

    renderCameraDef.orthogonal = true;

    renderCameraDef.layerMask = BIT(TagLayerSettings::BuiltInLayer::UI);

    // Mark as initialized
    SetInitialized(true);
}

void ComCanvas::OnInactive() {
    touchPointerStateTable.Clear();
}

#if WITH_EDITOR
void ComCanvas::DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) {
    int screenWidth = 320;
    int screenHeight = 200;

    const RenderContext *ctx = renderSystem.GetMainRenderContext();
    if (ctx) {
        screenWidth = ctx->GetScreenWidth();
        screenHeight = ctx->GetScreenHeight();
    }

    renderCameraDef.sizeX = screenWidth * 0.5f;
    renderCameraDef.sizeY = screenHeight * 0.5f;
}
#endif

const AABB ComCanvas::GetAABB() const {
    int screenWidth = 320;
    int screenHeight = 200;

    const RenderContext *ctx = renderSystem.GetMainRenderContext();
    if (ctx) {
        screenWidth = ctx->GetScreenWidth();
        screenHeight = ctx->GetScreenHeight();
    }

    Vec3 mins(-screenWidth * 0.5f, -screenHeight * 0.5f, 0);
    Vec3 maxs(+screenWidth * 0.5f, +screenHeight * 0.5f, 0);

    return AABB(mins, maxs);
}

const Point ComCanvas::WorldToScreen(const Vec3 &worldPos) const {
    int screenWidth = 320;
    int screenHeight = 200;

    const RenderContext *ctx = renderSystem.GetMainRenderContext();
    if (ctx) {
        screenWidth = ctx->GetScreenWidth();
        screenHeight = ctx->GetScreenHeight();
    }

    Vec3 localPos = renderCameraDef.axis.TransposedMulVec(worldPos - renderCameraDef.origin);

    // Compute right/down normalized screen coordinates [-1.0, +1.0]
    float ndx = -localPos.x / screenWidth;
    float ndy = -localPos.y / screenHeight;

    // Compute screen coordinates
    Point screenPoint;
    screenPoint.x = screenWidth * (ndx + 1.0f) * 0.5f;
    screenPoint.y = screenHeight * (ndy + 1.0f) * 0.5f;

    return screenPoint;
}

const Ray ComCanvas::ScreenPointToRay(const Point &screenPoint) {
    int screenWidth = 320;
    int screenHeight = 200;

    const RenderContext *ctx = renderSystem.GetMainRenderContext();
    if (ctx) {
        screenWidth = ctx->GetScreenWidth();
        screenHeight = ctx->GetScreenHeight();
    }

    renderCameraDef.sizeX = screenWidth * 0.5f;
    renderCameraDef.sizeY = screenHeight * 0.5f;

    Rect screenRect(0, 0, screenWidth, screenHeight);
    
    return RenderCamera::RayFromScreenPoint(renderCameraDef, screenRect, screenPoint);
}

bool ComCanvas::ProcessMousePointerInput() {
    return InputUtils::ProcessMousePointerInput(mousePointerState, [this](const Point &screenPoint) {
        // Convert screen point to ray.
        Ray ray = ScreenPointToRay(screenPoint);
        // Cast ray to detect entity.
        return GetEntity()->RayCastRect(ray);
    });
}

bool ComCanvas::ProcessTouchPointerInput() {
    return InputUtils::ProcessTouchPointerInput(touchPointerStateTable, [this](const Point &screenPoint) {
        // Convert screen point to ray.
        Ray ray = ScreenPointToRay(screenPoint);
        // Cast ray to detect entity.
        return GetEntity()->RayCastRect(ray);
    });
}

void ComCanvas::Render() {
    // Get current render context which is unique for each OS-level window in general.
    const RenderContext *ctx = renderSystem.GetCurrentRenderContext();
    if (!ctx) {
        return;
    }

    int screenWidth = ctx->GetScreenWidth();
    int screenHeight = ctx->GetScreenHeight();

    ComRectTransform *rectTransform = GetEntity()->GetComponent<ComRectTransform>();
    if (rectTransform) {
        if (rectTransform->GetSizeDelta() != Vec2(screenWidth, screenHeight)) {
            rectTransform->SetProperty("sizeDelta", Vec2(screenWidth, screenHeight));
        }
    }

    // Get the actual screen rendering resolution.
    float renderingWidth = ctx->GetRenderingWidth();
    float renderingHeight = ctx->GetRenderingHeight();

    renderCameraDef.renderRect.x = 0;
    renderCameraDef.renderRect.y = 0;
    renderCameraDef.renderRect.w = renderingWidth;
    renderCameraDef.renderRect.h = renderingHeight;

    renderCameraDef.sizeX = renderingWidth * 0.5f;
    renderCameraDef.sizeY = renderingHeight * 0.5f;

    // Update render camera with the given parameters.
    renderCamera->Update(&renderCameraDef);

    // Render scene with the given camera.
    GetGameWorld()->GetRenderWorld()->RenderScene(renderCamera);
}

BE_NAMESPACE_END
