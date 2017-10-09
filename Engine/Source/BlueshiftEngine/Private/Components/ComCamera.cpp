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
#include "Game/Entity.h"
#include "Game/GameWorld.h"
#include "Game/CastResult.h"
#include "Game/GameSettings/TagLayerSettings.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Camera", ComCamera, Component)
BEGIN_EVENTS(ComCamera)
END_EVENTS
BEGIN_PROPERTIES(ComCamera)
    PROPERTY_ENUM("projection", "Projection", "", "Perspective;Orthographic", "0", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("near", "Near", "near plane distance", Rangef(1, 20000, 10), "10", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("far", "Far", "far plane distance", Rangef(1, 20000, 10), "4096", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("fov", "Field Of View", "field of view", Rangef(1, 179, 1), "60", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("size", "Size", "", Rangef(1, 16384, 1), "1000", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("x", "X", "normalized screen x-coordinate", Rangef(0, 1.0f, 0.01f), "0.0", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("y", "Y", "normalized screen y-coordinate", Rangef(0, 1.0f, 0.01f), "0.0", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("w", "W", "normalized screen width", Rangef(0, 1.0f, 0.01f), "1.0", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("h", "H", "normalized screen height", Rangef(0, 1.0f, 0.01f), "1.0", PropertySpec::ReadWrite),
    PROPERTY_INT("layerMask", "Layer Mask", "", Str(BIT(TagLayerSettings::DefaultLayer)), PropertySpec::ReadWrite),
    PROPERTY_ENUM("clear", "Clear", "", "No Clear;Depth Only;Color;Skybox", "1", PropertySpec::ReadWrite),
    PROPERTY_COLOR3("clearColor", "Clear Color", "", "0 0 0", PropertySpec::ReadWrite),
    PROPERTY_FLOAT("clearAlpha", "Clear Alpha", "", "0", PropertySpec::ReadWrite),
    PROPERTY_INT("order", "Order", "", "0", PropertySpec::ReadWrite),
END_PROPERTIES

void ComCamera::RegisterProperties() {
#ifdef NEW_PROPERTY_SYSTEM
    REGISTER_ENUM_ACCESSOR_PROPERTY("Projection", "Perspective;Orthographic", GetProjectionMethod, SetProjectionMethod, 0, "", PropertySpec::ReadWrite);
    REGISTER_ACCESSOR_PROPERTY("Near", float, GetNear, SetNear, 10.f, "", PropertySpec::ReadWrite).SetRange(1, 20000, 10);
    REGISTER_ACCESSOR_PROPERTY("Far", float, GetFar, SetFar, 8192.f, "", PropertySpec::ReadWrite).SetRange(1, 20000, 10);
    REGISTER_PROPERTY("FOV", float, fov, 60.f, "", PropertySpec::ReadWrite).SetRange(1, 179, 1);
    REGISTER_PROPERTY("Size", float, size, 1000.f, "", PropertySpec::ReadWrite).SetRange(1, 16384, 1);
    REGISTER_PROPERTY("X", float, nx, 0.f, "", PropertySpec::ReadWrite).SetRange(0, 1.0f, 0.01f);
    REGISTER_PROPERTY("Y", float, ny, 0.f, "", PropertySpec::ReadWrite).SetRange(0, 1.0f, 0.01f);
    REGISTER_PROPERTY("W", float, nw, 1.f, "", PropertySpec::ReadWrite).SetRange(0, 1.0f, 0.01f);
    REGISTER_PROPERTY("H", float, nh, 1.f, "", PropertySpec::ReadWrite).SetRange(0, 1.0f, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("Layer Mask", int, GetLayerMask, SetLayerMask, BIT(TagLayerSettings::DefaultLayer) | BIT(TagLayerSettings::UILayer), "", PropertySpec::ReadWrite);
    REGISTER_ENUM_ACCESSOR_PROPERTY("Clear", "No Clear;Depth Only;Color", GetClearMethod, SetClearMethod, 1, "", PropertySpec::ReadWrite);
    REGISTER_ACCESSOR_PROPERTY("Clear Color", Color3, GetClearColor, SetClearColor, Color3::black, "", PropertySpec::ReadWrite);
    REGISTER_ACCESSOR_PROPERTY("Clear Alpha", float, GetClearAlpha, SetClearAlpha, 0.f, "", PropertySpec::ReadWrite);
    REGISTER_PROPERTY("Order", int, order, 0, "", PropertySpec::ReadWrite);
#endif
}

ComCamera::ComCamera() {
    spriteHandle = -1;
    spriteMesh = nullptr;
    memset(&sprite, 0, sizeof(sprite));
    view = nullptr;

#ifndef NEW_PROPERTY_SYSTEM
    Connect(&Properties::SIG_PropertyChanged, this, (SignalCallback)&ComCamera::PropertyChanged);
#endif
}

ComCamera::~ComCamera() {
    Purge(false);
}

void ComCamera::Purge(bool chainPurge) {
    if (view) {
        delete view;
        view = nullptr;
    }

    for (int i = 0; i < sprite.customMaterials.Count(); i++) {
        materialManager.ReleaseMaterial(sprite.customMaterials[i]);
    }
    sprite.customMaterials.Clear();

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
    Purge();

    Component::Init();

    renderWorld = GetGameWorld()->GetRenderWorld();

    view = new SceneView;

    memset(&viewParms, 0, sizeof(viewParms));

    //
    order = props->Get("order").As<int>();

    nx = props->Get("x").As<float>();
    ny = props->Get("y").As<float>();
    nw = props->Get("w").As<float>();
    nh = props->Get("h").As<float>();

    viewParms.layerMask = props->Get("layerMask").As<int>();

    viewParms.flags = SceneView::Flag::TexturedMode | SceneView::Flag::NoSubViews;
    if (!(viewParms.layerMask & BIT(TagLayerSettings::DefaultLayer))) {
        viewParms.flags |= SceneView::Flag::NoShadows | SceneView::Flag::NoSubViews | SceneView::Flag::SkipPostProcess;
    }

    viewParms.clearMethod = (SceneView::ClearMethod)props->Get("clear").As<int>();

    viewParms.clearColor.ToColor3() = props->Get("clearColor").As<Color3>();
    viewParms.clearColor.a = props->Get("clearAlpha").As<float>();

    viewParms.zNear = props->Get("near").As<float>();
    viewParms.zFar = props->Get("far").As<float>();

    viewParms.orthogonal = props->Get("projection").As<int>() == 1 ? true : false;

    fov = props->Get("fov").As<float>();
        
    size = props->Get("size").As<float>();
    
    ComTransform *transform = GetEntity()->GetTransform();
    
    viewParms.origin = transform->GetOrigin();
    viewParms.axis = transform->GetAxis();

    // 3d sprite
    spriteMesh = meshManager.GetMesh("_defaultQuadMesh");

    memset(&sprite, 0, sizeof(sprite));
    sprite.layer = TagLayerSettings::EditorLayer;
    sprite.maxVisDist = MeterToUnit(50);
    sprite.billboard = true;

    Texture *spriteTexture = textureManager.GetTexture("Data/EditorUI/Camera2.png", Texture::Clamp | Texture::HighQuality);
    sprite.customMaterials.SetCount(1);
    sprite.customMaterials[0] = materialManager.GetSingleTextureMaterial(spriteTexture, Material::SpriteHint);
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

    UpdateVisuals();
}

void ComCamera::SetEnable(bool enable) {
    if (enable) {
        if (!IsEnabled()) {
            UpdateVisuals();
            Component::SetEnable(true);
        }
    } else {
        if (IsEnabled()) {
            renderWorld->RemoveEntity(spriteHandle);
            spriteHandle = -1;
            Component::SetEnable(false);
        }
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
        float aspectRatio = 1;//w / h;

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

    sprite.customMaterials[0]->GetPass()->constantColor[3] = alpha;
}

const AABB ComCamera::GetAABB() {
    return Sphere(Vec3::origin, MeterToUnit(0.5)).ToAABB();
}

float ComCamera::GetAspectRatio() const {
    const BE1::RenderContext *ctx = BE1::renderSystem.GetMainRenderContext();

    const int screenWidth = ctx->GetScreenWidth();
    const int screenHeight = ctx->GetScreenHeight();

    return (float)screenWidth / screenHeight;
}

const Point ComCamera::WorldToScreen(const Vec3 &worldPos) const {
    const BE1::RenderContext *mainRenderContext = BE1::renderSystem.GetMainRenderContext();

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
    const BE1::RenderContext *mainRenderContext = BE1::renderSystem.GetMainRenderContext();

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
    Ray ray = ScreenToRay(screenPoint);

    Entity *hitEntity = nullptr;
    float minScale = MeterToUnit(100000);

#if 1
    CastResultEx castResult;
    if (GetGameWorld()->GetPhysicsWorld()->RayCast(nullptr, ray.origin, ray.GetDistancePoint(minScale),
        PhysCollidable::DefaultGroup, 
        PhysCollidable::DefaultGroup | PhysCollidable::StaticGroup | PhysCollidable::KinematicGroup | PhysCollidable::CharacterGroup, castResult)) {
        ComRigidBody *hitRigidBody = castResult.GetRigidBody();
        if (hitRigidBody) {
            hitEntity = hitRigidBody->GetEntity();
        }
    }
#else
    for (Entity *ent = GetGameWorld()->GetEntityHierarchy().GetChild(); ent; ent = ent->GetNode().GetNext()) {
        bool hasColliderComponent = ent->HasComponent(ComCollider::metaObject);
        if (!hasColliderComponent) {
            continue;
        }

        if (ent->RayIntersection(rayOrigin, rayDir, true, minScale)) {
            hitEntity = ent;
        }
    }
#endif

    if (hitEntity) {
        ComponentPtrArray scriptComponents = hitEntity->GetComponents(ComScript::metaObject);
        for (int i = 0; i < scriptComponents.Count(); i++) {
            ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

            if (inputSystem.IsKeyDown(KeyCode::Mouse1)) {
                scriptComponent->OnPointerDown();
            } else if (inputSystem.IsKeyUp(KeyCode::Mouse1)) {
                scriptComponent->OnPointerUp();
            } else if (inputSystem.IsKeyPressed(KeyCode::Mouse1)) {
                scriptComponent->OnPointerDrag();
            }
        }
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

    // Get the aspect ratio from screen size (logical screen size)
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

void ComCamera::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
        return;
    }

    if (!Str::Cmp(propName, "projection")) {
        SetProjectionMethod(props->Get("projection").As<int>());
        return;
    }

    if (!Str::Cmp(propName, "near")) {
        SetNear(props->Get("near").As<float>());
        return;
    }

    if (!Str::Cmp(propName, "far")) {
        SetFar(props->Get("far").As<float>());
        return;
    }    

    if (!Str::Cmp(propName, "fov")) {
        fov = props->Get("fov").As<float>();
        return;
    }

    if (!Str::Cmp(propName, "size")) {
        size = props->Get("size").As<float>();
        return;
    }

    if (!Str::Cmp(propName, "x")) {
        nx = props->Get("x").As<float>();
        return;
    }

    if (!Str::Cmp(propName, "y")) {
        ny = props->Get("y").As<float>();
        return;
    }

    if (!Str::Cmp(propName, "w")) {
        nw = props->Get("w").As<float>();
        return;
    }

    if (!Str::Cmp(propName, "h")) {
        nh = props->Get("h").As<float>();
        return;
    }

    if (!Str::Cmp(propName, "layerMask")) {
        SetLayerMask(props->Get("layerMask").As<int>());
        return;
    }

    if (!Str::Cmp(propName, "order")) {
        order = props->Get("order").As<int>();
        return;
    }

    if (!Str::Cmp(propName, "clear")) {
        SetClearMethod((SceneView::ClearMethod)props->Get("clear").As<int>());
        return;
    }

    if (!Str::Cmp(propName, "clearColor")) {
        SetClearColor(props->Get("clearColor").As<Color3>());
        return;
    }

    if (!Str::Cmp(propName, "clearAlpha")) {
        SetClearAlpha(props->Get("clearAlpha").As<float>());
        return;
    }

    Component::PropertyChanged(classname, propName);
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
    return viewParms.clearColor.a;
}

void ComCamera::SetNear(float zNear) {
    viewParms.zNear = zNear;
    UpdateVisuals();
}

float ComCamera::GetFar() const {
    return viewParms.clearColor.a;
}

void ComCamera::SetFar(float zFar) {
    viewParms.zFar = zFar;
    UpdateVisuals();
}

int ComCamera::GetClearMethod() const {
    return viewParms.clearMethod;
}

void ComCamera::SetClearMethod(int clearMethod) {
    viewParms.clearMethod = (SceneView::ClearMethod)clearMethod;
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
