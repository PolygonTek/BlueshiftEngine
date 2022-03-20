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
#include "Components/ComTransform.h"
#include "Components/ComLogic.h"
#include "Components/ComScript.h"
#include "Components/ComCamera.h"
#include "Game/GameWorld.h"
#include "Game/TagLayerSettings.h"
#include "Profiler/Profiler.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Camera", ComCamera, Component)
BEGIN_EVENTS(ComCamera)
END_EVENTS

void ComCamera::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("projection", "Projection", int, GetProjectionMethod, SetProjectionMethod, 0, 
        "", PropertyInfo::Flag::Editor).SetEnumString("Perspective;Orthographic");
    REGISTER_PROPERTY("fov", "FOV", float, fov, 60.f, 
        "Field of view for perspective projection mode", PropertyInfo::Flag::Editor).SetRange(1, 179, 1);
    REGISTER_PROPERTY("size", "Size", float, size, 5.f,
        "Horizontal size of orthographic projection mode", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("near", "Near", float, GetNear, SetNear, 0.3,
        "Near plane distance", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor).SetRange(0.01, 9999.99, 0.01);
    REGISTER_ACCESSOR_PROPERTY("far", "Far", float, GetFar, SetFar, 1000,
        "Far plane distance", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor).SetRange(0.02, 10000.0, 0.01);
    REGISTER_PROPERTY("x", "Viewport Rect/X", float, nx, 0.f, 
        "", PropertyInfo::Flag::Editor).SetRange(0, 1.0f, 0.01f);
    REGISTER_PROPERTY("y", "Viewport Rect/Y", float, ny, 0.f, 
        "", PropertyInfo::Flag::Editor).SetRange(0, 1.0f, 0.01f);
    REGISTER_PROPERTY("w", "Viewport Rect/W", float, nw, 1.f, 
        "", PropertyInfo::Flag::Editor).SetRange(0, 1.0f, 0.01f);
    REGISTER_PROPERTY("h", "Viewport Rect/H", float, nh, 1.f, 
        "", PropertyInfo::Flag::Editor).SetRange(0, 1.0f, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("clear", "Clear/Clear Method", RenderCamera::ClearMethod::Enum, GetClearMethod, SetClearMethod, 1, 
        "", PropertyInfo::Flag::Editor).SetEnumString("No Clear;Depth Only;Color;Skybox");
    REGISTER_ACCESSOR_PROPERTY("clearColor", "Clear/Color", Color3, GetClearColor, SetClearColor, Color3::black, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("clearAlpha", "Clear/Alpha", float, GetClearAlpha, SetClearAlpha, 0.f, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("layerMask", "Layer Mask", int, GetLayerMask, SetLayerMask, (int)(BIT(TagLayerSettings::BuiltInLayer::Default) | BIT(TagLayerSettings::BuiltInLayer::UI)),
        "", PropertyInfo::Flag::Editor);
    REGISTER_PROPERTY("order", "Order", int, order, 0,
        "", PropertyInfo::Flag::Editor);
    REGISTER_PROPERTY("applyPostProcessing", "Apply Post Processing", bool, applyPostProcessing, true,
        "", PropertyInfo::Flag::Editor);
}

ComCamera::ComCamera() {
    updatable = true;
    renderCamera = nullptr;

    mousePointerState.oldHitEntityGuid = Guid::zero;
    mousePointerState.captureEntityGuid = Guid::zero;

#if WITH_EDITOR
    spriteHandle = -1;
    spriteMesh = nullptr;
#endif
}

ComCamera::~ComCamera() {
    Purge(false);
}

void ComCamera::Purge(bool chainPurge) {
#if WITH_EDITOR
    for (int i = 0; i < spriteDef.materials.Count(); i++) {
        materialManager.ReleaseMaterial(spriteDef.materials[i]);
    }
    spriteDef.materials.Clear();

    if (spriteDef.mesh) {
        meshManager.ReleaseMesh(spriteDef.mesh);
        spriteDef.mesh = nullptr;
    }

    if (spriteMesh) {
        meshManager.ReleaseMesh(spriteMesh);
        spriteMesh = nullptr;
    }

    if (spriteHandle != -1) {
        renderWorld->RemoveRenderObject(spriteHandle);
        spriteHandle = -1;
    }
#endif

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

void ComCamera::Init() {
    Component::Init();

    renderWorld = GetGameWorld()->GetRenderWorld();

    if (!renderCamera) {
        renderCamera = new RenderCamera;
    }

    renderCameraDef.flags = RenderCamera::Flag::TexturedMode | RenderCamera::Flag::NoSubViews;
    if (!applyPostProcessing) {
        renderCameraDef.flags |= RenderCamera::Flag::SkipPostProcess;
    }
    
    ComTransform *transform = GetEntity()->GetTransform();
    
    renderCameraDef.origin = transform->GetOrigin();
    renderCameraDef.axis = transform->GetAxis();

#if WITH_EDITOR
    // 3d sprite for editor
    spriteMesh = meshManager.GetMesh("_defaultQuadMesh");

    spriteDef.flags = RenderObject::Flag::Billboard;
    spriteDef.layer = TagLayerSettings::BuiltInLayer::Editor;
    spriteDef.maxVisDist = MeterToUnit(50.0f);

    Texture *spriteTexture = textureManager.GetTextureWithoutTextureInfo("Data/EditorUI/Camera2.png", Texture::Flag::Clamp | Texture::Flag::HighQuality);
    spriteDef.materials.SetCount(1);
    spriteDef.materials[0] = materialManager.GetSingleTextureMaterial(spriteTexture, Material::TextureHint::Sprite);
    textureManager.ReleaseTexture(spriteTexture);
    
    spriteDef.mesh = spriteMesh->InstantiateMesh(Mesh::Type::Static);
    spriteDef.aabb = spriteMesh->GetAABB();
    spriteDef.worldMatrix = transform->GetMatrixNoScale();
    spriteDef.materialParms[RenderObject::MaterialParm::Red] = 1.0f;
    spriteDef.materialParms[RenderObject::MaterialParm::Green] = 1.0f;
    spriteDef.materialParms[RenderObject::MaterialParm::Blue] = 1.0f;
    spriteDef.materialParms[RenderObject::MaterialParm::Alpha] = 1.0f;
    spriteDef.materialParms[RenderObject::MaterialParm::TimeOffset] = 0.0f;
    spriteDef.materialParms[RenderObject::MaterialParm::TimeScale] = 1.0f;
#endif

    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComCamera::TransformUpdated, SignalObject::ConnectionType::Unique);

    // Mark as initialized
    SetInitialized(true);

    UpdateVisuals();
}

void ComCamera::OnActive() {
    UpdateVisuals();
}

void ComCamera::OnInactive() {
    touchPointerStateTable.Clear();

#if WITH_EDITOR
    if (spriteHandle != -1) {
        renderWorld->RemoveRenderObject(spriteHandle);
        spriteHandle = -1;
    }
#endif
}

bool ComCamera::HasRenderObject(int renderObjectHandle) const { 
#if WITH_EDITOR
    if (this->spriteHandle == renderObjectHandle) {
        return true;
    }
#endif

    return false;
}

void ComCamera::Update() {
    renderCameraDef.time = GetGameWorld()->GetTime();
}

#if WITH_EDITOR
void ComCamera::DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();
    
    if (selected) {
        int screenWidth = 320;
        int screenHeight = 200;

        const RenderContext *ctx = renderSystem.GetMainRenderContext();
        if (ctx) {
            screenWidth = ctx->GetScreenWidth();
            screenHeight = ctx->GetScreenHeight();
        }

        if (this->renderCameraDef.orthogonal) {
            Size orthoSize = GetOrthoSize();

            this->renderCameraDef.sizeX = orthoSize.w;
            this->renderCameraDef.sizeY = orthoSize.h;

            float sizeZ = (this->renderCameraDef.zNear + this->renderCameraDef.zFar) * 0.5f;

            OBB cameraBox;
            cameraBox.SetAxis(this->renderCameraDef.axis);
            cameraBox.SetCenter(this->renderCameraDef.origin + this->renderCameraDef.axis[0] * sizeZ);
            cameraBox.SetExtents(Vec3(sizeZ, this->renderCameraDef.sizeX, this->renderCameraDef.sizeY));

            renderWorld->SetDebugColor(selectedByParent ? Color4::white : Color4(1.0, 1.0, 1.0, 0.5), Color4::zero);
            renderWorld->DebugOBB(cameraBox, 1.0f, false, false, true);
        } else {
            float w = screenWidth * nw;
            float h = screenHeight * nh;
            float aspectRatio = w / h;

            RenderCamera::ComputeFov(fov, 1.25f, aspectRatio, &this->renderCameraDef.fovX, &this->renderCameraDef.fovY);

            Frustum cameraFrustum;
            cameraFrustum.SetOrigin(this->renderCameraDef.origin);
            cameraFrustum.SetAxis(this->renderCameraDef.axis);
            cameraFrustum.SetSize(this->renderCameraDef.zNear, this->renderCameraDef.zFar,
                this->renderCameraDef.zFar * Math::Tan(DEG2RAD(this->renderCameraDef.fovX * 0.5f)), this->renderCameraDef.zFar * Math::Tan(DEG2RAD(this->renderCameraDef.fovY * 0.5f)));

            renderWorld->SetDebugColor(selectedByParent ? Color4::white : Color4(1.0, 1.0, 1.0, 0.5), Color4::zero);
            renderWorld->DebugFrustum(cameraFrustum, false, 1.0f, false, true);
        }

        /*if (ctx) {
            float upscaleFactorX = ctx->GetUpscaleFactorX();
            float upscaleFactorY = ctx->GetUpscaleFactorY();

            int w = renderingWidth * 0.25f;
            int h = renderingHeight * 0.25f;
            int x = renderingWidth - (w + 10 / upscaleFactorX);
            int y = renderingHeight - (h + 10 / upscaleFactorY);

            RenderCamera::State previewViewState = this->renderCameraDef;

            previewViewState.renderRect.Set(x, y, w, h);
            previewViewState.flags |= RenderCamera::SkipDebugDraw;

            static RenderCamera previewView;
            previewView.Update(&previewViewState);

            GetGameWorld()->GetRenderWorld()->RenderScene(&previewView);
        }*/
    }

    // Fade icon alpha in near distance
    float alpha = Clamp(spriteDef.worldMatrix.ToTranslationVec3().Distance(camera->GetState().origin) / MeterToUnit(8.0f), 0.01f, 1.0f);

    spriteDef.materials[0]->GetPass()->constantColor[3] = alpha;
}
#endif 

const AABB ComCamera::GetAABB() const {
    return Sphere(Vec3::origin, MeterToUnit(0.5f)).ToAABB();
}

Size ComCamera::GetOrthoSize() const {
    int screenWidth = 320;
    int screenHeight = 200;

    const RenderContext *ctx = renderSystem.GetMainRenderContext();
    if (ctx) {
        screenWidth = ctx->GetScreenWidth();
        screenHeight = ctx->GetScreenHeight();
    }

    float invAspectRatio = (float)screenHeight / (float)screenWidth;

    Size orthoSize;
    orthoSize.w = size;
    orthoSize.h = size * invAspectRatio;

    return orthoSize;
}

float ComCamera::GetAspectRatio() const {
    const RenderContext *ctx = renderSystem.GetMainRenderContext();
    if (!ctx) {
        return 1.0f;
    }

    int screenWidth = ctx->GetScreenWidth();
    int screenHeight = ctx->GetScreenHeight();

    return (float)screenWidth / (float)screenHeight;
}

const Point ComCamera::WorldToScreenPoint(const Vec3 &worldPos) const {
    int screenWidth = 320;
    int screenHeight = 200;

    const RenderContext *ctx = renderSystem.GetMainRenderContext();
    if (ctx) {
        screenWidth = ctx->GetScreenWidth();
        screenHeight = ctx->GetScreenHeight();
    }

    Vec3 localPos = renderCameraDef.axis.TransposedMulVec(worldPos - renderCameraDef.origin);
    float aspectRatio = (float)screenWidth / screenHeight;
    float ndx, ndy;

    if (renderCameraDef.orthogonal) {
        // Compute right/down normalized screen coordinates [-1.0, +1.0]
        ndx = -localPos.y / size;
        ndy = -localPos.z / (size / aspectRatio);
    } else {
        float tanFovX = Math::Tan(DEG2RAD(fov * 0.5f));
        float tanFovY = tanFovX / 1.25f;

        tanFovX = tanFovY * aspectRatio;

        // Compute right/down normalized screen coordinates [-1.0, +1.0]
        ndx = -localPos.y / (localPos.x * tanFovX);
        ndy = -localPos.z / (localPos.x * tanFovY);
    }

    // Compute screen coordinates
    Point screenPoint;
    screenPoint.x = screenWidth * (ndx + 1.0f) * 0.5f;
    screenPoint.y = screenHeight * (ndy + 1.0f) * 0.5f;
    
    return screenPoint;
}

const Ray ComCamera::ScreenPointToRay(const Point &screenPoint) {
    int screenWidth = 320;
    int screenHeight = 200;

    const RenderContext *ctx = renderSystem.GetMainRenderContext();
    if (ctx) {
        screenWidth = ctx->GetScreenWidth();
        screenHeight = ctx->GetScreenHeight();
    }

    if (renderCameraDef.orthogonal) {
        Size orthoSize = GetOrthoSize();

        renderCameraDef.sizeX = orthoSize.w;
        renderCameraDef.sizeY = orthoSize.h;
    } else {
        float aspectRatio = (float)screenWidth / screenHeight;

         RenderCamera::ComputeFov(fov, 1.25f, aspectRatio, &renderCameraDef.fovX, &renderCameraDef.fovY);
    }

    Rect screenRect;
    screenRect.x = screenWidth * nx;
    screenRect.y = screenHeight * ny;
    screenRect.w = screenWidth * nw;
    screenRect.h = screenHeight * nh;

    return RenderCamera::RayFromScreenPoint(renderCameraDef, screenRect, screenPoint);
}

bool ComCamera::ProcessMousePointerInput() {
    return InputUtils::ProcessMousePointerInput(mousePointerState, [this](const Point &screenPoint) {
        // Convert screen point to ray.
        Ray ray = ScreenPointToRay(screenPoint);
        // Cast ray to detect entity.
        return GetGameWorld()->RayCast(ray, GetLayerMask());
    });
}

bool ComCamera::ProcessTouchPointerInput() {
    return InputUtils::ProcessTouchPointerInput(touchPointerStateTable, [this](const Point &screenPoint) {
        // Convert screen point to ray.
        Ray ray = ScreenPointToRay(screenPoint);
        // Cast ray to detect entity.
        return GetGameWorld()->RayCast(ray, GetLayerMask());
    });
}

void ComCamera::Render() {
    BE_PROFILE_CPU_SCOPE_STATIC("ComCamera::Render");

    // Get current render context which is unique for each OS-level window in general.
    const RenderContext *ctx = renderSystem.GetCurrentRenderContext();
    if (!ctx) {
        return;
    }

    // Get the actual screen rendering resolution.
    float renderingWidth = ctx->GetRenderingWidth();
    float renderingHeight = ctx->GetRenderingHeight();

    // Offset and scale with the normalized [0, 1] screen fraction values.
    renderCameraDef.renderRect.x = renderingWidth * nx;
    renderCameraDef.renderRect.y = renderingHeight * ny;
    renderCameraDef.renderRect.w = renderingWidth * nw;
    renderCameraDef.renderRect.h = renderingHeight * nh;

    // Get the logical screen resolution.
    float screenWidth = ctx->GetScreenWidth();
    float screenHeight = ctx->GetScreenHeight();

    if (renderCameraDef.orthogonal) {
        Size orthoSize = GetOrthoSize();

        renderCameraDef.sizeX = orthoSize.w;
        renderCameraDef.sizeY = orthoSize.h;
    } else {
        // Get the aspect ratio from the screen resolution.
        float aspectRatio = screenWidth / screenHeight;

        // Compute fovX, fovY with the given fov and aspect ratio.
        RenderCamera::ComputeFov(fov, 1.25f, aspectRatio, &renderCameraDef.fovX, &renderCameraDef.fovY);
    }

    // Update render camera with the given parameters.
    renderCamera->Update(&renderCameraDef);

    // Render scene with the given camera.
    GetGameWorld()->GetRenderWorld()->RenderScene(renderCamera);
}

void ComCamera::UpdateVisuals() {
    if (!IsInitialized() || !IsActiveInHierarchy()) {
        return;
    }

#if WITH_EDITOR
    if (spriteHandle == -1) {
        spriteHandle = renderWorld->AddRenderObject(&spriteDef);
    } else {
        renderWorld->UpdateRenderObject(spriteHandle, &spriteDef);
    }
#endif
}

void ComCamera::TransformUpdated(const ComTransform *transform) {
    renderCameraDef.origin = transform->GetOrigin();
    renderCameraDef.axis = transform->GetAxis();

#if WITH_EDITOR
    spriteDef.worldMatrix.SetTranslation(renderCameraDef.origin);
#endif

    UpdateVisuals();
}

int ComCamera::GetLayerMask() const {
    return renderCameraDef.layerMask;
}

void ComCamera::SetLayerMask(int layerMask) {
    renderCameraDef.layerMask = layerMask;

    UpdateVisuals();
}

int ComCamera::GetProjectionMethod() const {
    return renderCameraDef.orthogonal ? 1 : 0;
}

void ComCamera::SetProjectionMethod(const int projectionMethod) {
    renderCameraDef.orthogonal = projectionMethod == 1 ? true : false;

    UpdateVisuals();
}

float ComCamera::GetNear() const {
    return renderCameraDef.zNear;
}

void ComCamera::SetNear(float zNear) {
    renderCameraDef.zNear = zNear;

    if (renderCameraDef.zNear >= renderCameraDef.zFar) {
        SetProperty("far", renderCameraDef.zNear + 0.01f);
    }

    UpdateVisuals();
}

float ComCamera::GetFar() const {
    return renderCameraDef.zFar;
}

void ComCamera::SetFar(float zFar) {
    if (zFar >= renderCameraDef.zNear) {
        renderCameraDef.zFar = zFar;

        UpdateVisuals();
    }
}

RenderCamera::ClearMethod::Enum ComCamera::GetClearMethod() const {
    return renderCameraDef.clearMethod;
}

void ComCamera::SetClearMethod(RenderCamera::ClearMethod::Enum clearMethod) {
    renderCameraDef.clearMethod = clearMethod;

    UpdateVisuals();
}

const Color3 &ComCamera::GetClearColor() const {
    return renderCameraDef.clearColor.ToColor3();
}

void ComCamera::SetClearColor(const Color3 &clearColor) {
    renderCameraDef.clearColor.ToColor3() = clearColor;

    UpdateVisuals();
}

float ComCamera::GetClearAlpha() const {
    return renderCameraDef.clearColor.a;
}

void ComCamera::SetClearAlpha(const float clearAlpha) {
    renderCameraDef.clearColor.a = clearAlpha;

    UpdateVisuals();
}

BE_NAMESPACE_END
