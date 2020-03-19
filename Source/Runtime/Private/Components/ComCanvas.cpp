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
#include "Components/ComRenderable.h"
#include "Components/ComRectTransform.h"
#include "Components/ComLogic.h"
#include "Components/ComScript.h"
#include "Game/Entity.h"
#include "Game/GameWorld.h"
#include "Game/TagLayerSettings.h"
#include "Profiler/Profiler.h"

#ifdef ENABLE_IMGUI
#include "imgui/imgui.h"
#endif

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Canvas", ComCanvas, Component)
BEGIN_EVENTS(ComCanvas)
END_EVENTS

void ComCanvas::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("scaleMode", "Scale Mode", ScaleMode::Enum, GetScaleMode, SetScaleMode, ScaleMode::ScaleWithScreenSize,
        "", PropertyInfo::Flag::Editor).SetEnumString("Constant Pixel Size;Scale With Screen Size");
    REGISTER_PROPERTY("referenceResolution", "Reference Resolution", Size, referenceResolution, Size(1280, 720),
        "", PropertyInfo::Flag::Editor);
    REGISTER_PROPERTY("matchMode", "Match Mode", MatchMode::Enum, matchMode, MatchMode::MatchWidthOrHeight,
        "", PropertyInfo::Flag::Editor).SetEnumString("Match Width Or Height;Expand;Shrink");
    REGISTER_PROPERTY("match", "Match (Width " u8"\u27f7" " Height)", float, match, 0.0f,
        "", PropertyInfo::Flag::Editor).SetRange(0.0f, 1.0f, 0.01f);
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
    
    renderCameraDef.flags = RenderCamera::Flag::TexturedMode | RenderCamera::Flag::NoSubViews | RenderCamera::Flag::SkipPostProcess;

    renderCameraDef.clearMethod = RenderCamera::ClearMethod::DepthOnly;

    renderCameraDef.origin = Vec3::origin;
    renderCameraDef.axis[0] = -Coords2D::ZAxis();
    renderCameraDef.axis[1] = -Coords2D::XAxis();
    renderCameraDef.axis[2] = +Coords2D::YAxis();
    renderCameraDef.axis.FixDegeneracies();

    renderCameraDef.zNear = -1000.0f;
    renderCameraDef.zFar = 1000.0f;

    renderCameraDef.orthogonal = true;

    renderCameraDef.layerMask = BIT(TagLayerSettings::BuiltInLayer::UI);

#if WITH_EDITOR
    ComRectTransform *rectTransform = GetEntity()->GetRectTransform();
    if (rectTransform) {
        rectTransform->Connect(&ComRectTransform::SIG_RectTransformUpdated, this, (SignalCallback)&ComCanvas::RectTransformUpdated, SignalObject::ConnectionType::Unique);
    }
#endif

    // Mark as initialized
    SetInitialized(true);
}

void ComCanvas::Awake() {
    UpdateRenderingOrderForCanvasElements();
}

void ComCanvas::OnInactive() {
    touchPointerStateTable.Clear();
}

#if WITH_EDITOR
void ComCanvas::DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) {    
    Size orthoSize = GetOrthoSize();

    renderCameraDef.sizeX = orthoSize.w;
    renderCameraDef.sizeY = orthoSize.h;

    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    ComRectTransform *rectTransform = GetEntity()->GetRectTransform();
    if (rectTransform) {
        Vec3 worldCorners[4];
        rectTransform->GetWorldCorners(worldCorners);

        renderWorld->SetDebugColor(Color4(0.3f, 0.3f, 0.3f, 0.7f), Color4::zero);

        renderWorld->DebugLine(worldCorners[0], worldCorners[1], 1.0f, true);
        renderWorld->DebugLine(worldCorners[1], worldCorners[2], 1.0f, true);
        renderWorld->DebugLine(worldCorners[2], worldCorners[3], 1.0f, true);
        renderWorld->DebugLine(worldCorners[3], worldCorners[0], 1.0f, true);
    }
}
#endif

const AABB ComCanvas::GetAABB() const {
    Size orthoSize = GetOrthoSize();

    Vec3 mins(-orthoSize.w, -orthoSize.h, 0);
    Vec3 maxs(+orthoSize.w, +orthoSize.h, 0);

    return AABB(mins, maxs);
}

#if WITH_EDITOR
void ComCanvas::RectTransformUpdated(ComRectTransform *rectTransform) {
    if (rectTransform->GetAnchoredPosition() != Vec2::zero) {
        rectTransform->SetAnchoredPosition(Vec2::zero);
    }
}
#endif

Size ComCanvas::GetOrthoSize() const {
    int screenWidth = 320;
    int screenHeight = 200;

    const RenderContext *ctx = renderSystem.GetMainRenderContext();
    if (ctx) {
        screenWidth = ctx->GetScreenWidth();
        screenHeight = ctx->GetScreenHeight();
    }

    Size orthoSize;

    if (scaleMode == ScaleMode::ConstantPixelSize) {
        orthoSize.w = screenWidth * 0.5f;
        orthoSize.h = screenHeight * 0.5f;
    } else if (scaleMode == ScaleMode::ScaleWithScreenSize) {
        float aspectRatio = (float)screenWidth / (float)screenHeight;

        float w1 = referenceResolution.w;
        float w2 = referenceResolution.h * aspectRatio;

        float h1 = referenceResolution.w / aspectRatio;
        float h2 = referenceResolution.h;

        switch (matchMode) {
        case MatchMode::MatchWidthOrHeight:
            orthoSize.w = Math::Lerp(w1, w2, match) * 0.5f;
            orthoSize.h = Math::Lerp(h1, h2, match) * 0.5f;
            break;
        case MatchMode::Expand:
            orthoSize.w = Min(w1, w2) * 0.5f;
            orthoSize.h = Min(h1, h2) * 0.5f;
            break;
        case MatchMode::Shrink:
            orthoSize.w = Max(w1, w2) * 0.5f;
            orthoSize.h = Max(h1, h2) * 0.5f;
            break;
        }
    }

    return orthoSize;
}

ComCanvas::ScaleMode::Enum ComCanvas::GetScaleMode() const {
    return scaleMode;
}

void ComCanvas::SetScaleMode(ScaleMode::Enum scaleMode) {
    this->scaleMode = scaleMode;
}

const Point ComCanvas::WorldToScreenPoint(const Vec3 &worldPos) const {
    int screenWidth = 320;
    int screenHeight = 200;

    const RenderContext *ctx = renderSystem.GetMainRenderContext();
    if (ctx) {
        screenWidth = ctx->GetScreenWidth();
        screenHeight = ctx->GetScreenHeight();
    }

    Vec3 localPos = renderCameraDef.axis.TransposedMulVec(worldPos - renderCameraDef.origin);

    // Compute right/down normalized screen coordinates [-1.0, +1.0]
    float ndx = -localPos.y / renderCameraDef.sizeX;
    float ndy = -localPos.z / renderCameraDef.sizeY;

    // Compute screen coordinates
    Point screenPoint;
    screenPoint.x = screenWidth * (ndx + 1.0f) * 0.5f;
    screenPoint.y = screenHeight * (ndy + 1.0f) * 0.5f;

    return screenPoint;
}

const Point ComCanvas::WorldToCanvasPoint(const Vec3 &worldPos) const {
    Point screenPoint = WorldToScreenPoint(worldPos);

    return ScreenToCanvasPoint(screenPoint);
}

const Point ComCanvas::ScreenToCanvasPoint(const Point screenPoint) const {
    int screenWidth = 320;
    int screenHeight = 200;

    const RenderContext *ctx = renderSystem.GetMainRenderContext();
    if (ctx) {
        screenWidth = ctx->GetScreenWidth();
        screenHeight = ctx->GetScreenHeight();
    }

    float ndx = ((float)screenPoint.x / screenWidth) * 2.0f - 1.0f;
    float ndy = ((float)screenPoint.y / screenHeight) * 2.0f - 1.0f;

    Size orthoSize = GetOrthoSize();

    Point canvasPoint;
    canvasPoint.x = ndx * orthoSize.w;
    canvasPoint.y = ndy * orthoSize.h;

    return canvasPoint;
}

const Point ComCanvas::CanvasToScreenPoint(const Point canvasPoint) const {
    int screenWidth = 320;
    int screenHeight = 200;

    const RenderContext *ctx = renderSystem.GetMainRenderContext();
    if (ctx) {
        screenWidth = ctx->GetScreenWidth();
        screenHeight = ctx->GetScreenHeight();
    }

    Size orthoSize = GetOrthoSize();

    float fractX = (((float)canvasPoint.x / orthoSize.w) + 1.0f) * 0.5f;
    float fractY = (((float)canvasPoint.y / orthoSize.h) + 1.0f) * 0.5f;

    Point screenPoint;
    screenPoint.x = fractX * screenWidth;
    screenPoint.y = fractY * screenHeight;

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

    Size orthoSize = GetOrthoSize();

    renderCameraDef.sizeX = orthoSize.w;
    renderCameraDef.sizeY = orthoSize.h;

    Rect screenRect(0, 0, screenWidth, screenHeight);
    
    return RenderCamera::RayFromScreenPoint(renderCameraDef, screenRect, screenPoint);
}

const Ray ComCanvas::CanvasPointToRay(const Point &canvasPoint) {
    Point screenPoint = CanvasToScreenPoint(canvasPoint);

    return ScreenPointToRay(screenPoint);
}

bool ComCanvas::IsScreenPointOverChildRect(const Point &screenPoint) {
    Ray ray = ScreenPointToRay(screenPoint);

    Entity *hitEntity = GetEntity()->RayCastRect(ray);

    if (!hitEntity || hitEntity == GetEntity()) {
        return false;
    }
    return true;
}

bool ComCanvas::ProcessMousePointerInput() {
    return InputUtils::ProcessMousePointerInput(mousePointerState, [this](const Point &screenPoint) -> Entity * {
#ifdef ENABLE_IMGUI
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
            return nullptr;
        }
#endif
        // Convert screen point to ray.
        Ray ray = ScreenPointToRay(screenPoint);
        // Cast ray to detect entity.
        Entity *hitEntity = GetEntity()->RayCastRect(ray);
        // Return hit entity except for canvas entity.
        return hitEntity != GetEntity() ? hitEntity : nullptr;
    });
}

bool ComCanvas::ProcessTouchPointerInput() {
    return InputUtils::ProcessTouchPointerInput(touchPointerStateTable, [this](const Point &screenPoint) -> Entity * {
#ifdef ENABLE_IMGUI
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
            return nullptr;
        }
#endif
        // Convert screen point to ray.
        Ray ray = ScreenPointToRay(screenPoint);
        // Cast ray to detect entity.
        Entity *hitEntity = GetEntity()->RayCastRect(ray);
        // Return hit entity except for canvas entity.
        return hitEntity != GetEntity() ? hitEntity : nullptr;
    });
}

void ComCanvas::UpdateRenderingOrderForCanvasElements() const {
    int sceneNum = GetEntity()->GetSceneNum();
    int order = 0;

    UpdateRenderingOrderRecursive(GetEntity(), sceneNum, order);
}

void ComCanvas::UpdateRenderingOrderRecursive(Entity *entity, int sceneNum, int &order) const {
    for (Entity *ent = entity->GetNode().GetFirstChild(); ent; ent = ent->GetNode().GetNextSibling()) {
        ComRenderable *renderableComponent = ent->GetComponent<ComRenderable>();

        if (renderableComponent) {
            renderableComponent->SetRenderingOrder((sceneNum << 12) | order);
            order++;

            UpdateRenderingOrderRecursive(ent, sceneNum, order);
        }
    }
}

void ComCanvas::Render() {
    BE_SCOPE_PROFILE_CPU("ComCanvas::Render");

    // Get current render context which is unique for each OS-level window in general.
    const RenderContext *ctx = renderSystem.GetCurrentRenderContext();
    if (!ctx) {
        return;
    }

    Size orthoSize = GetOrthoSize();

    renderCameraDef.sizeX = orthoSize.w;
    renderCameraDef.sizeY = orthoSize.h;

    ComRectTransform *rectTransform = GetEntity()->GetComponent<ComRectTransform>();
    if (rectTransform) {
        Vec2 sizeDelta = orthoSize.ToVec2() * 2.0f;

        if (rectTransform->GetSizeDelta() != sizeDelta) {
#if WITH_EDITOR
            rectTransform->SetProperty("sizeDelta", sizeDelta);
#else
            rectTransform->SetSizeDelta(sizeDelta);
#endif
        }
    }

    // Get the actual screen rendering resolution.
    float renderingWidth = ctx->GetRenderingWidth();
    float renderingHeight = ctx->GetRenderingHeight();

    renderCameraDef.renderRect.x = 0;
    renderCameraDef.renderRect.y = 0;
    renderCameraDef.renderRect.w = renderingWidth;
    renderCameraDef.renderRect.h = renderingHeight;

    // Update render camera with the given parameters.
    renderCamera->Update(&renderCameraDef);

    // Render scene with the given camera.
    GetGameWorld()->GetRenderWorld()->RenderScene(renderCamera);
}

BE_NAMESPACE_END
