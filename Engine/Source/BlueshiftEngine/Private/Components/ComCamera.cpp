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
#include "Input/InputSystem.h"
#include "Render/Render.h"
#include "Components/ComTransform.h"
#include "Components/ComRigidBody.h"
#include "Components/ComLogic.h"
#include "Components/ComScript.h"
#include "Components/ComCamera.h"
#include "Game/GameWorld.h"
#include "Game/CastResult.h"
#include "Game/TagLayerSettings.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Camera", ComCamera, Component)
BEGIN_EVENTS(ComCamera)
END_EVENTS

void ComCamera::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("projection", "Projection", int, GetProjectionMethod, SetProjectionMethod, 0, "", PropertyInfo::EditorFlag)
        .SetEnumString("Perspective;Orthographic");
    REGISTER_ACCESSOR_PROPERTY("near", "Near", float, GetNear, SetNear, 10.f, "", PropertyInfo::EditorFlag)
        .SetRange(1, 20000, 10);
    REGISTER_ACCESSOR_PROPERTY("far", "Far", float, GetFar, SetFar, 8192.f, "", PropertyInfo::EditorFlag)
        .SetRange(1, 20000, 10);
    REGISTER_PROPERTY("fov", "FOV", float, fov, 60.f, "", PropertyInfo::EditorFlag)
        .SetRange(1, 179, 1);
    REGISTER_PROPERTY("size", "Size", float, size, 1000.f, "", PropertyInfo::EditorFlag)
        .SetRange(1, 16384, 1);
    REGISTER_PROPERTY("x", "X", float, nx, 0.f, "", PropertyInfo::EditorFlag)
        .SetRange(0, 1.0f, 0.01f);
    REGISTER_PROPERTY("y", "Y", float, ny, 0.f, "", PropertyInfo::EditorFlag)
        .SetRange(0, 1.0f, 0.01f);
    REGISTER_PROPERTY("w", "W", float, nw, 1.f, "", PropertyInfo::EditorFlag)
        .SetRange(0, 1.0f, 0.01f);
    REGISTER_PROPERTY("h", "H", float, nh, 1.f, "", PropertyInfo::EditorFlag)
        .SetRange(0, 1.0f, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("layerMask", "Layer Mask", int, GetLayerMask, SetLayerMask, (int)(BIT(TagLayerSettings::DefaultLayer) | BIT(TagLayerSettings::UILayer)), "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("clear", "Clear", SceneView::ClearMethod, GetClearMethod, SetClearMethod, 1, "", PropertyInfo::EditorFlag)
        .SetEnumString("No Clear;Depth Only;Color;Skybox");
    REGISTER_ACCESSOR_PROPERTY("clearColor", "Clear Color", Color3, GetClearColor, SetClearColor, Color3::black, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("clearAlpha", "Clear Alpha", float, GetClearAlpha, SetClearAlpha, 0.f, "", PropertyInfo::EditorFlag);
    REGISTER_PROPERTY("order", "Order", int, order, 0, "", PropertyInfo::EditorFlag);
}

ComCamera::ComCamera() {
    memset(&viewParms, 0, sizeof(viewParms));
    view = nullptr;

    spriteHandle = -1;
    spriteMesh = nullptr;
    memset(&sprite, 0, sizeof(sprite));

    oldHoverEntityGuid = Guid::zero;
    captureEntityGuid = Guid::zero;
}

ComCamera::~ComCamera() {
    Purge(false);
}

void ComCamera::Purge(bool chainPurge) {
    if (view) {
        delete view;
        view = nullptr;
    }

    for (int i = 0; i < sprite.materials.Count(); i++) {
        materialManager.ReleaseMaterial(sprite.materials[i]);
    }
    sprite.materials.Clear();

    if (sprite.mesh) {
        meshManager.ReleaseMesh(sprite.mesh);
        sprite.mesh = nullptr;
    }

    if (spriteMesh) {
        meshManager.ReleaseMesh(spriteMesh);
        spriteMesh = nullptr;
    }

    if (spriteHandle != -1) {
        renderWorld->RemoveEntity(spriteHandle);
        spriteHandle = -1;
    }

    if (chainPurge) {
        Component::Purge();
    }
}

void ComCamera::Init() {
    Component::Init();

    renderWorld = GetGameWorld()->GetRenderWorld();

    if (!view) {
        view = new SceneView;
    }

    viewParms.flags = SceneView::Flag::TexturedMode | SceneView::Flag::NoSubViews;
    if (!(viewParms.layerMask & BIT(TagLayerSettings::DefaultLayer))) {
        viewParms.flags |= SceneView::Flag::NoShadows | SceneView::Flag::NoSubViews | SceneView::Flag::SkipPostProcess;
    }
    
    ComTransform *transform = GetEntity()->GetTransform();
    
    viewParms.origin = transform->GetOrigin();
    viewParms.axis = transform->GetAxis();

    // 3d sprite for editor
    spriteMesh = meshManager.GetMesh("_defaultQuadMesh");

    memset(&sprite, 0, sizeof(sprite));
    sprite.layer = TagLayerSettings::EditorLayer;
    sprite.maxVisDist = MeterToUnit(50);
    sprite.billboard = true;

    Texture *spriteTexture = textureManager.GetTexture("Data/EditorUI/Camera2.png", Texture::Clamp | Texture::HighQuality);
    sprite.materials.SetCount(1);
    sprite.materials[0] = materialManager.GetSingleTextureMaterial(spriteTexture, Material::SpriteHint);
    textureManager.ReleaseTexture(spriteTexture);
    
    sprite.mesh = spriteMesh->InstantiateMesh(Mesh::StaticMesh);
    sprite.aabb = spriteMesh->GetAABB();
    sprite.origin = transform->GetOrigin();
    sprite.scale = Vec3(1, 1, 1);
    sprite.axis = Mat3::identity;
    sprite.materialParms[SceneEntity::RedParm] = 1.0f;
    sprite.materialParms[SceneEntity::GreenParm] = 1.0f;
    sprite.materialParms[SceneEntity::BlueParm] = 1.0f;
    sprite.materialParms[SceneEntity::AlphaParm] = 1.0f;
    sprite.materialParms[SceneEntity::TimeOffsetParm] = 0.0f;
    sprite.materialParms[SceneEntity::TimeScaleParm] = 1.0f;

    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComCamera::TransformUpdated, SignalObject::Unique);

    // Mark as initialized
    SetInitialized(true);

    UpdateVisuals();
}

void ComCamera::OnActive() {
    UpdateVisuals();
}

void ComCamera::OnInactive() {
    if (spriteHandle != -1) {
        renderWorld->RemoveEntity(spriteHandle);
        spriteHandle = -1;
    }
}

bool ComCamera::HasRenderEntity(int renderEntityHandle) const { 
    if (this->spriteHandle == renderEntityHandle) {
        return true;
    }

    return false;
}

void ComCamera::Update() {
    viewParms.time = GetGameWorld()->GetTime();
}

bool ComCamera::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    return false;
}

void ComCamera::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();
    
    if (selected) {
        const RenderContext *ctx = renderSystem.GetCurrentRenderContext();
        float w = ctx->GetRenderingWidth() * nw;
        float h = ctx->GetRenderingHeight() * nh;
        float aspectRatio = w / h;

        if (viewParms.orthogonal) {
            viewParms.sizeX = size;
            viewParms.sizeY = size / aspectRatio;
            float sizeZ = (viewParms.zNear + viewParms.zFar) * 0.5f;

            OBB cameraBox;
            cameraBox.SetAxis(viewParms.axis);
            cameraBox.SetCenter(viewParms.origin + viewParms.axis[0] * sizeZ);
            cameraBox.SetExtents(Vec3(sizeZ, viewParms.sizeX, viewParms.sizeY));

            renderWorld->SetDebugColor(Color4::white, Color4::zero);
            renderWorld->DebugOBB(cameraBox, 1.0f, false, false, true);
        } else {
            SceneView::ComputeFov(fov, 1.25f, aspectRatio, &viewParms.fovX, &viewParms.fovY);

            Frustum cameraFrustum;
            cameraFrustum.SetOrigin(viewParms.origin);
            cameraFrustum.SetAxis(viewParms.axis);
            cameraFrustum.SetSize(viewParms.zNear, viewParms.zFar, 
                viewParms.zFar * Math::Tan(DEG2RAD(viewParms.fovX * 0.5f)), viewParms.zFar * Math::Tan(DEG2RAD(viewParms.fovY * 0.5f)));

            renderWorld->SetDebugColor(Color4::white, Color4::zero);
            renderWorld->DebugFrustum(cameraFrustum, false, 1.0f, false, true);
        }

        if (1) {
            int w = ctx->GetRenderingWidth() * 0.25f;
            int h = ctx->GetRenderingHeight() * 0.25f;
            int x = ctx->GetRenderingWidth() - (w + 10 / ctx->GetUpscaleFactorX());
            int y = ctx->GetRenderingHeight() - (h + 10 / ctx->GetUpscaleFactorY());

            SceneView::Parms previewViewParms = this->viewParms;

            previewViewParms.renderRect.Set(x, y, w, h);
            previewViewParms.flags |= SceneView::SkipDebugDraw;

            static SceneView previewView;
            previewView.Update(&previewViewParms);

            GetGameWorld()->GetRenderWorld()->RenderScene(&previewView);
        }
    }

    // Fade icon alpha in near distance
    float alpha = BE1::Clamp(sprite.origin.Distance(sceneView.origin) / MeterToUnit(8), 0.01f, 1.0f);

    sprite.materials[0]->GetPass()->constantColor[3] = alpha;
}

const AABB ComCamera::GetAABB() {
    return Sphere(Vec3::origin, MeterToUnit(0.5)).ToAABB();
}

float ComCamera::GetAspectRatio() const {
    const RenderContext *ctx = renderSystem.GetMainRenderContext();

    const int screenWidth = ctx->GetScreenWidth();
    const int screenHeight = ctx->GetScreenHeight();

    return (float)screenWidth / screenHeight;
}

const Point ComCamera::WorldToScreen(const Vec3 &worldPos) const {
    const RenderContext *mainRenderContext = renderSystem.GetMainRenderContext();

    const int screenWidth = mainRenderContext->GetScreenWidth();
    const int screenHeight = mainRenderContext->GetScreenHeight();

    Vec3 localPos = viewParms.axis.TransposedMulVec(worldPos - viewParms.origin);
    Point screenPoint;

    float aspectRatio = (float)screenWidth / screenHeight;

    if (viewParms.orthogonal) {
        // right/down normalized screen coordinates [ -1.0 ~ 1.0 ]
        float ndx = -localPos.y / size;
        float ndy = -localPos.z / (size / aspectRatio);

        screenPoint.x = screenWidth * (ndx + 1.0f) * 0.5f;
        screenPoint.y = screenHeight * (ndy + 1.0f) * 0.5f;
    } else {
        float tanFovX = Math::Tan(DEG2RAD(fov * 0.5f));
        float tanFovY = tanFovX / 1.25f;
        tanFovX = tanFovY * aspectRatio;

        // right/down normalized screen coordinates [ -1.0 ~ 1.0 ]
        float ndx = -localPos.y / (localPos.x * tanFovX);
        float ndy = -localPos.z / (localPos.x * tanFovY);

        screenPoint.x = screenWidth * (ndx + 1.0f) * 0.5f;
        screenPoint.y = screenHeight * (ndy + 1.0f) * 0.5f;
    }
    
    return screenPoint;
}

const Ray ComCamera::ScreenToRay(const Point &screenPoint) {
    const RenderContext *mainRenderContext = renderSystem.GetMainRenderContext();

    const int screenWidth = mainRenderContext->GetScreenWidth();
    const int screenHeight = mainRenderContext->GetScreenHeight();

    Rect screenRect;
    screenRect.x = screenWidth * nx;
    screenRect.y = screenHeight * ny;
    screenRect.w = screenWidth * nw;
    screenRect.h = screenHeight * nh;

    // right/down normalized screen coordinates [ -1.0 ~ 1.0 ]
    float ndx = ((float)(screenPoint.x - screenRect.x) / screenRect.w) * 2.0f - 1.0f;
    float ndy = ((float)(screenPoint.y - screenRect.y) / screenRect.h) * 2.0f - 1.0f;

    float aspectRatio = (float)screenWidth / screenHeight;

    if (viewParms.orthogonal) {
        viewParms.sizeX = size;
        viewParms.sizeY = size / aspectRatio;
    } else {
        SceneView::ComputeFov(fov, 1.25f, aspectRatio, &viewParms.fovX, &viewParms.fovY);
    }

    return SceneView::RayFromScreenND(viewParms, ndx, ndy);
}

void ComCamera::ProcessPointerInput(const Point &screenPoint) {
    Entity *oldHoverEntity = (Entity *)Entity::FindInstance(oldHoverEntityGuid);
    Entity *hoverEntity = nullptr;

    Ray ray = ScreenToRay(screenPoint);

    CastResultEx castResult;
    // FIXME: ray cast against corresponding layer entities
    if (GetGameWorld()->GetPhysicsWorld()->RayCast(nullptr, ray.origin, ray.GetDistancePoint(MeterToUnit(100000)),
        PhysCollidable::DefaultGroup, 
        PhysCollidable::DefaultGroup | PhysCollidable::StaticGroup | PhysCollidable::KinematicGroup | PhysCollidable::CharacterGroup, castResult)) {
        ComRigidBody *hitRigidBody = castResult.GetRigidBody();
        if (hitRigidBody) {
            hoverEntity = hitRigidBody->GetEntity();
        }
    }

    Entity *captureEntity = (Entity *)Entity::FindInstance(captureEntityGuid);

    if (inputSystem.IsKeyUp(KeyCode::Mouse1)) {
        if (captureEntity) {
            ComponentPtrArray scriptComponents = captureEntity->GetComponents(&ComScript::metaObject);
            for (int i = 0; i < scriptComponents.Count(); i++) {
                ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

                scriptComponent->OnPointerUp();

                if (hoverEntity == captureEntity) {
                    scriptComponent->OnPointerClick();
                }
            }
        }

        captureEntityGuid = Guid::zero;
    }

    if (oldHoverEntity) {
        ComponentPtrArray scriptComponents = oldHoverEntity->GetComponents(&ComScript::metaObject);
        for (int i = 0; i < scriptComponents.Count(); i++) {
            ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

            if (oldHoverEntity == hoverEntity) {
                scriptComponent->OnPointerOver();
            } else {
                scriptComponent->OnPointerExit();
            }
        }
    }

    if (hoverEntity) {
        ComponentPtrArray scriptComponents = hoverEntity->GetComponents(&ComScript::metaObject);
        for (int i = 0; i < scriptComponents.Count(); i++) {
            ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

            if (hoverEntity != oldHoverEntity) {
                scriptComponent->OnPointerEnter();
            }

            if (inputSystem.IsKeyDown(KeyCode::Mouse1)) {
                scriptComponent->OnPointerDown();

                captureEntityGuid = hoverEntity->GetGuid();
            } else if (inputSystem.IsKeyPressed(KeyCode::Mouse1)) {
                scriptComponent->OnPointerDrag();
            }
        }

        oldHoverEntityGuid = hoverEntity->GetGuid();
    } else {
        oldHoverEntityGuid = Guid::zero;
    }
}

void ComCamera::RenderScene() {
    // Get current render context which is unique for each OS-level window in general
    const RenderContext *ctx = renderSystem.GetCurrentRenderContext();

    // Get the actual screen rendering resolution
    float renderingWidth = ctx->GetRenderingWidth();
    float renderingHeight = ctx->GetRenderingHeight();

    // Offset and scale with the normalized [0, 1] screen fraction values
    viewParms.renderRect.x = renderingWidth * nx;
    viewParms.renderRect.y = renderingHeight * ny;
    viewParms.renderRect.w = renderingWidth * nw;
    viewParms.renderRect.h = renderingHeight * nh;

    // Get the aspect ratio from device screen size
    float aspectRatio = (float)ctx->GetScreenWidth() / ctx->GetScreenHeight();

    if (viewParms.orthogonal) {
        // Compute viewport rectangle size in orthogonal projection
        viewParms.sizeX = size;
        viewParms.sizeY = size / aspectRatio;
    } else {
        // Compute fovX, fovY with the given fov and aspect ratio
        SceneView::ComputeFov(fov, 1.25f, aspectRatio, &viewParms.fovX, &viewParms.fovY);
    }

    // Update scene view with view parameters
    view->Update(&viewParms);

    // Render scene scene view
    GetGameWorld()->GetRenderWorld()->RenderScene(view);
}

void ComCamera::UpdateVisuals() {
    if (!IsInitialized() || !IsActiveInHierarchy()) {
        return;
    }

    if (spriteHandle == -1) {
        spriteHandle = renderWorld->AddEntity(&sprite);
    } else {
        renderWorld->UpdateEntity(spriteHandle, &sprite);
    }
}

void ComCamera::TransformUpdated(const ComTransform *transform) {
    viewParms.origin = transform->GetOrigin();
    viewParms.axis = transform->GetAxis();

    sprite.origin = viewParms.origin;
    
    UpdateVisuals();
}

int ComCamera::GetLayerMask() const {
    return viewParms.layerMask;
}

void ComCamera::SetLayerMask(int layerMask) {
    viewParms.layerMask = layerMask;

    UpdateVisuals();
}

int ComCamera::GetProjectionMethod() const {
    return viewParms.orthogonal ? 1 : 0;
}

void ComCamera::SetProjectionMethod(const int projectionMethod) {
    viewParms.orthogonal = projectionMethod == 1 ? true : false;

    UpdateVisuals();
}

float ComCamera::GetNear() const {
    return viewParms.zNear;
}

void ComCamera::SetNear(float zNear) {
    viewParms.zNear = zNear;

    UpdateVisuals();
}

float ComCamera::GetFar() const {
    return viewParms.zFar;
}

void ComCamera::SetFar(float zFar) {
    viewParms.zFar = zFar;

    UpdateVisuals();
}

SceneView::ClearMethod ComCamera::GetClearMethod() const {
    return viewParms.clearMethod;
}

void ComCamera::SetClearMethod(SceneView::ClearMethod clearMethod) {
    viewParms.clearMethod = clearMethod;

    UpdateVisuals();
}

const Color3 &ComCamera::GetClearColor() const {
    return viewParms.clearColor.ToColor3();
}

void ComCamera::SetClearColor(const Color3 &clearColor) {
    viewParms.clearColor.ToColor3() = clearColor;

    UpdateVisuals();
}

float ComCamera::GetClearAlpha() const {
    return viewParms.clearColor.a;
}

void ComCamera::SetClearAlpha(const float clearAlpha) {
    viewParms.clearColor.a = clearAlpha;

    UpdateVisuals();
}

BE_NAMESPACE_END
