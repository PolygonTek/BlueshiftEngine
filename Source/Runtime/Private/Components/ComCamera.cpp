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
    REGISTER_ACCESSOR_PROPERTY("projection", "Projection", int, GetProjectionMethod, SetProjectionMethod, 0, 
        "", PropertyInfo::EditorFlag).SetEnumString("Perspective;Orthographic");
    REGISTER_ACCESSOR_PROPERTY("near", "Near", float, GetNear, SetNear, MeterToUnit(0.1f),
        "", PropertyInfo::SystemUnits | PropertyInfo::EditorFlag).SetRange(CentiToUnit(1.0f), MeterToUnit(100.0f), CentiToUnit(10.0f));
    REGISTER_ACCESSOR_PROPERTY("far", "Far", float, GetFar, SetFar, MeterToUnit(500.0f),
        "", PropertyInfo::SystemUnits | PropertyInfo::EditorFlag).SetRange(CentiToUnit(1.0f), MeterToUnit(1000.0f), CentiToUnit(10.0f));
    REGISTER_PROPERTY("fov", "FOV", float, fov, 60.f, 
        "", PropertyInfo::EditorFlag).SetRange(1, 179, 1);
    REGISTER_PROPERTY("size", "Size", float, size, 1000.f, 
        "", PropertyInfo::EditorFlag).SetRange(1, 16384, 1);
    REGISTER_PROPERTY("x", "Viewport Rect/X", float, nx, 0.f, 
        "", PropertyInfo::EditorFlag).SetRange(0, 1.0f, 0.01f);
    REGISTER_PROPERTY("y", "Viewport Rect/Y", float, ny, 0.f, 
        "", PropertyInfo::EditorFlag).SetRange(0, 1.0f, 0.01f);
    REGISTER_PROPERTY("w", "Viewport Rect/W", float, nw, 1.f, 
        "", PropertyInfo::EditorFlag).SetRange(0, 1.0f, 0.01f);
    REGISTER_PROPERTY("h", "Viewport Rect/H", float, nh, 1.f, 
        "", PropertyInfo::EditorFlag).SetRange(0, 1.0f, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("clear", "Clear/Clear Method", RenderView::ClearMethod, GetClearMethod, SetClearMethod, 1, 
        "", PropertyInfo::EditorFlag).SetEnumString("No Clear;Depth Only;Color;Skybox");
    REGISTER_ACCESSOR_PROPERTY("clearColor", "Clear/Color", Color3, GetClearColor, SetClearColor, Color3::black, 
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("clearAlpha", "Clear/Alpha", float, GetClearAlpha, SetClearAlpha, 0.f, 
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("layerMask", "Layer Mask", int, GetLayerMask, SetLayerMask, (int)(BIT(TagLayerSettings::DefaultLayer) | BIT(TagLayerSettings::UILayer)),
        "", PropertyInfo::EditorFlag);
    REGISTER_PROPERTY("order", "Order", int, order, 0,
        "", PropertyInfo::EditorFlag);
}

ComCamera::ComCamera() {
    memset(&renderViewDef, 0, sizeof(renderViewDef));
    renderView = nullptr;

    spriteHandle = -1;
    spriteMesh = nullptr;
    memset(&spriteDef, 0, sizeof(spriteDef));

    mousePointerState.oldHitEntityGuid = Guid::zero;
    mousePointerState.captureEntityGuid = Guid::zero;
}

ComCamera::~ComCamera() {
    Purge(false);
}

void ComCamera::Purge(bool chainPurge) {
    if (renderView) {
        delete renderView;
        renderView = nullptr;
    }

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

    if (!renderView) {
        renderView = new RenderView;
    }

    renderViewDef.flags = RenderView::Flag::TexturedMode | RenderView::Flag::NoSubViews;
    if (!(renderViewDef.layerMask & BIT(TagLayerSettings::DefaultLayer))) {
        renderViewDef.flags |= RenderView::Flag::NoShadows | RenderView::Flag::NoSubViews | RenderView::Flag::SkipPostProcess;
    }
    
    ComTransform *transform = GetEntity()->GetTransform();
    
    renderViewDef.origin = transform->GetOrigin();
    renderViewDef.axis = transform->GetAxis();

    // 3d sprite for editor
    spriteMesh = meshManager.GetMesh("_defaultQuadMesh");

    memset(&spriteDef, 0, sizeof(spriteDef));
    spriteDef.flags = RenderObject::BillboardFlag;
    spriteDef.layer = TagLayerSettings::EditorLayer;
    spriteDef.maxVisDist = MeterToUnit(50.0f);

    Texture *spriteTexture = textureManager.GetTexture("Data/EditorUI/Camera2.png", Texture::Clamp | Texture::HighQuality);
    spriteDef.materials.SetCount(1);
    spriteDef.materials[0] = materialManager.GetSingleTextureMaterial(spriteTexture, Material::SpriteHint);
    textureManager.ReleaseTexture(spriteTexture);
    
    spriteDef.mesh = spriteMesh->InstantiateMesh(Mesh::StaticMesh);
    spriteDef.localAABB = spriteMesh->GetAABB();
    spriteDef.origin = transform->GetOrigin();
    spriteDef.scale = Vec3(1, 1, 1);
    spriteDef.axis = Mat3::identity;
    spriteDef.materialParms[RenderObject::RedParm] = 1.0f;
    spriteDef.materialParms[RenderObject::GreenParm] = 1.0f;
    spriteDef.materialParms[RenderObject::BlueParm] = 1.0f;
    spriteDef.materialParms[RenderObject::AlphaParm] = 1.0f;
    spriteDef.materialParms[RenderObject::TimeOffsetParm] = 0.0f;
    spriteDef.materialParms[RenderObject::TimeScaleParm] = 1.0f;

    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComCamera::TransformUpdated, SignalObject::Unique);

    // Mark as initialized
    SetInitialized(true);

    UpdateVisuals();
}

void ComCamera::OnActive() {
    UpdateVisuals();
}

void ComCamera::OnInactive() {
    touchPointerStateTable.Clear();

    if (spriteHandle != -1) {
        renderWorld->RemoveRenderObject(spriteHandle);
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
    renderViewDef.time = GetGameWorld()->GetTime();
}

bool ComCamera::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    return false;
}

void ComCamera::DrawGizmos(const RenderView::State &renderViewDef, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();
    
    if (selected) {
        int renderingWidth = 100;
        int renderingHeight = 100;

        const RenderContext *ctx = renderSystem.GetMainRenderContext();
        if (ctx) {
            renderingWidth = ctx->GetRenderingWidth();
            renderingHeight = ctx->GetRenderingHeight();
        }

        float w = renderingWidth * nw;
        float h = renderingHeight * nh;
        float aspectRatio = w / h;

        if (this->renderViewDef.orthogonal) {
            this->renderViewDef.sizeX = size;
            this->renderViewDef.sizeY = size / aspectRatio;
            float sizeZ = (this->renderViewDef.zNear + this->renderViewDef.zFar) * 0.5f;

            OBB cameraBox;
            cameraBox.SetAxis(this->renderViewDef.axis);
            cameraBox.SetCenter(this->renderViewDef.origin + this->renderViewDef.axis[0] * sizeZ);
            cameraBox.SetExtents(Vec3(sizeZ, this->renderViewDef.sizeX, this->renderViewDef.sizeY));

            renderWorld->SetDebugColor(Color4::white, Color4::zero);
            renderWorld->DebugOBB(cameraBox, 1.0f, false, false, true);
        } else {
            RenderView::ComputeFov(fov, 1.25f, aspectRatio, &this->renderViewDef.fovX, &this->renderViewDef.fovY);

            Frustum cameraFrustum;
            cameraFrustum.SetOrigin(this->renderViewDef.origin);
            cameraFrustum.SetAxis(this->renderViewDef.axis);
            cameraFrustum.SetSize(this->renderViewDef.zNear, this->renderViewDef.zFar,
                this->renderViewDef.zFar * Math::Tan(DEG2RAD(this->renderViewDef.fovX * 0.5f)), this->renderViewDef.zFar * Math::Tan(DEG2RAD(this->renderViewDef.fovY * 0.5f)));

            renderWorld->SetDebugColor(Color4::white, Color4::zero);
            renderWorld->DebugFrustum(cameraFrustum, false, 1.0f, false, true);
        }

        /*if (ctx) {
            float upscaleFactorX = ctx->GetUpscaleFactorX();
            float upscaleFactorY = ctx->GetUpscaleFactorY();
            
            int w = renderingWidth * 0.25f;
            int h = renderingHeight * 0.25f;
            int x = renderingWidth - (w + 10 / upscaleFactorX);
            int y = renderingHeight - (h + 10 / upscaleFactorY);

            RenderView::State previewViewState = this->renderViewDef;

            previewViewState.renderRect.Set(x, y, w, h);
            previewViewState.flags |= RenderView::SkipDebugDraw;

            static RenderView previewView;
            previewView.Update(&previewViewState);

            GetGameWorld()->GetRenderWorld()->RenderScene(&previewView);
        }*/
    }

    // Fade icon alpha in near distance
    float alpha = BE1::Clamp(spriteDef.origin.Distance(renderViewDef.origin) / MeterToUnit(8.0f), 0.01f, 1.0f);

    spriteDef.materials[0]->GetPass()->constantColor[3] = alpha;
}

const AABB ComCamera::GetAABB() {
    return Sphere(Vec3::origin, MeterToUnit(0.5f)).ToAABB();
}

float ComCamera::GetAspectRatio() const {
    const RenderContext *ctx = renderSystem.GetMainRenderContext();
    if (!ctx) {
        return 1.0f;
    }

    int screenWidth = ctx->GetScreenWidth();
    int screenHeight = ctx->GetScreenHeight();

    return (float)screenWidth / screenHeight;
}

const Point ComCamera::WorldToScreen(const Vec3 &worldPos) const {
    int screenWidth = 100;
    int screenHeight = 100;

    const RenderContext *ctx = renderSystem.GetMainRenderContext();
    if (ctx) {
        screenWidth = ctx->GetScreenWidth();
        screenHeight = ctx->GetScreenHeight();
    }

    Vec3 localPos = renderViewDef.axis.TransposedMulVec(worldPos - renderViewDef.origin);
    float aspectRatio = (float)screenWidth / screenHeight;
    float ndx, ndy;

    if (renderViewDef.orthogonal) {
        // Compute right/down normalized screen coordinates [ -1.0 ~ 1.0 ]
        ndx = -localPos.y / size;
        ndy = -localPos.z / (size / aspectRatio);
    } else {
        float tanFovX = Math::Tan(DEG2RAD(fov * 0.5f));
        float tanFovY = tanFovX / 1.25f;

        tanFovX = tanFovY * aspectRatio;

        // Compute right/down normalized screen coordinates [ -1.0 ~ 1.0 ]
        ndx = -localPos.y / (localPos.x * tanFovX);
        ndy = -localPos.z / (localPos.x * tanFovY);
    }

    // Compute screen coordinates
    Point screenPoint;
    screenPoint.x = screenWidth * (ndx + 1.0f) * 0.5f;
    screenPoint.y = screenHeight * (ndy + 1.0f) * 0.5f;
    
    return screenPoint;
}

const Ray ComCamera::ScreenToRay(const Point &screenPoint) {
    int screenWidth = 100;
    int screenHeight = 100;

    const RenderContext *ctx = renderSystem.GetMainRenderContext();
    if (ctx) {
        screenWidth = ctx->GetScreenWidth();
        screenHeight = ctx->GetScreenHeight();
    }

    Rect screenRect;
    screenRect.x = screenWidth * nx;
    screenRect.y = screenHeight * ny;
    screenRect.w = screenWidth * nw;
    screenRect.h = screenHeight * nh;

    // right/down normalized screen coordinates [ -1.0 ~ 1.0 ]
    float ndx = ((float)(screenPoint.x - screenRect.x) / screenRect.w) * 2.0f - 1.0f;
    float ndy = ((float)(screenPoint.y - screenRect.y) / screenRect.h) * 2.0f - 1.0f;

    float aspectRatio = (float)screenWidth / screenHeight;

    if (renderViewDef.orthogonal) {
        renderViewDef.sizeX = size;
        renderViewDef.sizeY = size / aspectRatio;
    } else {
        RenderView::ComputeFov(fov, 1.25f, aspectRatio, &renderViewDef.fovX, &renderViewDef.fovY);
    }

    return RenderView::RayFromScreenND(renderViewDef, ndx, ndy);
}

bool ComCamera::ProcessMousePointerInput(const Point &screenPoint) {
    if (!inputSystem.IsMouseExist()) {
        return false;
    }

    Ray ray = ScreenToRay(screenPoint);

    Entity *hitTestEntity = nullptr;
    CastResultEx castResult;

    if (GetGameWorld()->GetPhysicsWorld()->RayCast(nullptr, ray.origin, ray.GetDistancePoint(MeterToUnit(1000.0f)), GetLayerMask(), castResult)) {
        ComRigidBody *hitTestRigidBody = castResult.GetRigidBody();
        if (hitTestRigidBody) {
            hitTestEntity = hitTestRigidBody->GetEntity();
        }
    }

    Entity *captureEntity = (Entity *)Entity::FindInstance(mousePointerState.captureEntityGuid);

    if (inputSystem.IsKeyUp(KeyCode::Mouse1)) {
        if (captureEntity) {
            ComponentPtrArray scriptComponents = captureEntity->GetComponents(&ComScript::metaObject);

            for (int i = 0; i < scriptComponents.Count(); i++) {
                ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

                scriptComponent->OnPointerUp();

                if (hitTestEntity == captureEntity) {
                    scriptComponent->OnPointerClick();
                }
            }
        }

        mousePointerState.captureEntityGuid = Guid::zero;
    }

    Entity *oldHitEntity = (Entity *)Entity::FindInstance(mousePointerState.oldHitEntityGuid);

    if (oldHitEntity) {
        ComponentPtrArray scriptComponents = oldHitEntity->GetComponents(&ComScript::metaObject);

        for (int i = 0; i < scriptComponents.Count(); i++) {
            ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

            if (oldHitEntity == hitTestEntity) {
                scriptComponent->OnPointerOver();
            } else {
                scriptComponent->OnPointerExit();
            }
        }
    }

    if (hitTestEntity) {
        ComponentPtrArray scriptComponents = hitTestEntity->GetComponents(&ComScript::metaObject);

        for (int i = 0; i < scriptComponents.Count(); i++) {
            ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

            if (hitTestEntity != oldHitEntity) {
                scriptComponent->OnPointerEnter();
            }

            if (inputSystem.IsKeyDown(KeyCode::Mouse1)) {
                scriptComponent->OnPointerDown();
            } else if (inputSystem.IsKeyPressed(KeyCode::Mouse1)) {
                scriptComponent->OnPointerDrag();
            }
        }

        if (inputSystem.IsKeyDown(KeyCode::Mouse1)) {
            mousePointerState.captureEntityGuid = hitTestEntity->GetGuid();
        }

        mousePointerState.oldHitEntityGuid = hitTestEntity->GetGuid();
    } else {
        mousePointerState.oldHitEntityGuid = Guid::zero;
    }

    return (hitTestEntity || !mousePointerState.captureEntityGuid.IsZero());
}

bool ComCamera::ProcessTouchPointerInput() {
    bool processed = false;

    for (int touchIndex = 0; touchIndex < inputSystem.GetTouchCount(); touchIndex++) {
        InputSystem::Touch touch = inputSystem.GetTouch(touchIndex);

        Entity *hitTestEntity = nullptr;

        if (touch.phase == InputSystem::Touch::Started ||
            touch.phase == InputSystem::Touch::Ended ||
            touch.phase == InputSystem::Touch::Moved) {
            Ray ray = ScreenToRay(touch.position);

            CastResultEx castResult;

            if (GetGameWorld()->GetPhysicsWorld()->RayCast(nullptr, ray.origin, ray.GetDistancePoint(MeterToUnit(1000.0f)), GetLayerMask(), castResult)) {
                ComRigidBody *hitTestRigidBody = castResult.GetRigidBody();
                if (hitTestRigidBody) {
                    hitTestEntity = hitTestRigidBody->GetEntity();
                }
            }
        }

        if (touch.phase == InputSystem::Touch::Started) {
            if (hitTestEntity) {
                processed = true;

                PointerState touchPointerState;
                touchPointerState.oldHitEntityGuid = hitTestEntity->GetGuid();
                touchPointerState.captureEntityGuid = hitTestEntity->GetGuid();

                touchPointerStateTable.Set(touch.id, touchPointerState);

                ComponentPtrArray scriptComponents = hitTestEntity->GetComponents(&ComScript::metaObject);

                for (int i = 0; i < scriptComponents.Count(); i++) {
                    ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

                    scriptComponent->OnPointerEnter();
                    scriptComponent->OnPointerDown();
                }
            }
        } else if (touch.phase == InputSystem::Touch::Ended || touch.phase == InputSystem::Touch::Canceled) {
            PointerState touchPointerState;

            if (touchPointerStateTable.Get(touch.id, &touchPointerState)) {
                Entity *capturedEntity = (Entity *)Entity::FindInstance(touchPointerState.captureEntityGuid);

                if (capturedEntity) {
                    processed = true;

                    ComponentPtrArray scriptComponents = capturedEntity->GetComponents(&ComScript::metaObject);

                    for (int i = 0; i < scriptComponents.Count(); i++) {
                        ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

                        scriptComponent->OnPointerUp();

                        if (touch.phase == InputSystem::Touch::Ended && hitTestEntity == capturedEntity) {
                            scriptComponent->OnPointerClick();
                        }

                        scriptComponent->OnPointerExit();
                    }
                }

                touchPointerStateTable.Remove(touch.id);
            }
        } else if (touch.phase == InputSystem::Touch::Moved) {
            PointerState touchPointerState;

            if (touchPointerStateTable.Get(touch.id, &touchPointerState)) {
                Entity *capturedEntity = (Entity *)Entity::FindInstance(touchPointerState.captureEntityGuid);
                Entity *oldHitEntity = (Entity *)Entity::FindInstance(touchPointerState.oldHitEntityGuid);

                if (capturedEntity) {
                    processed = true;

                    ComponentPtrArray scriptComponents = capturedEntity->GetComponents(&ComScript::metaObject);

                    for (int i = 0; i < scriptComponents.Count(); i++) {
                        ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

                        if (hitTestEntity != oldHitEntity) {
                            if (hitTestEntity == capturedEntity) {
                                scriptComponent->OnPointerEnter();
                            } else if (oldHitEntity == capturedEntity) {
                                scriptComponent->OnPointerExit();
                            }
                        }

                        scriptComponent->OnPointerDrag();
                    }
                }

                touchPointerState.oldHitEntityGuid = hitTestEntity ? hitTestEntity->GetGuid() : Guid::zero;
                touchPointerStateTable.Set(touch.id, touchPointerState);
            }
        }
    }

    return processed;
}

void ComCamera::RenderScene() {
    // Get current render context which is unique for each OS-level window in general
    const RenderContext *ctx = renderSystem.GetCurrentRenderContext();
    if (!ctx) {
        return;
    }

    // Get the actual screen rendering resolution
    float renderingWidth = ctx->GetRenderingWidth();
    float renderingHeight = ctx->GetRenderingHeight();

    // Offset and scale with the normalized [0, 1] screen fraction values
    renderViewDef.renderRect.x = renderingWidth * nx;
    renderViewDef.renderRect.y = renderingHeight * ny;
    renderViewDef.renderRect.w = renderingWidth * nw;
    renderViewDef.renderRect.h = renderingHeight * nh;

    // Get the aspect ratio from device screen size
    float aspectRatio = (float)ctx->GetScreenWidth() / ctx->GetScreenHeight();

    if (renderViewDef.orthogonal) {
        // Compute viewport rectangle size in orthogonal projection
        renderViewDef.sizeX = size;
        renderViewDef.sizeY = size / aspectRatio;
    } else {
        // Compute fovX, fovY with the given fov and aspect ratio
        RenderView::ComputeFov(fov, 1.25f, aspectRatio, &renderViewDef.fovX, &renderViewDef.fovY);
    }

    // Update render view with parameters
    renderView->Update(&renderViewDef);

    // Render scene scene view
    GetGameWorld()->GetRenderWorld()->RenderScene(renderView);
}

void ComCamera::UpdateVisuals() {
    if (!IsInitialized() || !IsActiveInHierarchy()) {
        return;
    }

    if (spriteHandle == -1) {
        spriteHandle = renderWorld->AddRenderObject(&spriteDef);
    } else {
        renderWorld->UpdateRenderObject(spriteHandle, &spriteDef);
    }
}

void ComCamera::TransformUpdated(const ComTransform *transform) {
    renderViewDef.origin = transform->GetOrigin();
    renderViewDef.axis = transform->GetAxis();

    spriteDef.origin = renderViewDef.origin;
    
    UpdateVisuals();
}

int ComCamera::GetLayerMask() const {
    return renderViewDef.layerMask;
}

void ComCamera::SetLayerMask(int layerMask) {
    renderViewDef.layerMask = layerMask;

    UpdateVisuals();
}

int ComCamera::GetProjectionMethod() const {
    return renderViewDef.orthogonal ? 1 : 0;
}

void ComCamera::SetProjectionMethod(const int projectionMethod) {
    renderViewDef.orthogonal = projectionMethod == 1 ? true : false;

    UpdateVisuals();
}

float ComCamera::GetNear() const {
    return renderViewDef.zNear;
}

void ComCamera::SetNear(float zNear) {
    renderViewDef.zNear = zNear;

    UpdateVisuals();
}

float ComCamera::GetFar() const {
    return renderViewDef.zFar;
}

void ComCamera::SetFar(float zFar) {
    renderViewDef.zFar = zFar;

    UpdateVisuals();
}

RenderView::ClearMethod ComCamera::GetClearMethod() const {
    return renderViewDef.clearMethod;
}

void ComCamera::SetClearMethod(RenderView::ClearMethod clearMethod) {
    renderViewDef.clearMethod = clearMethod;

    UpdateVisuals();
}

const Color3 &ComCamera::GetClearColor() const {
    return renderViewDef.clearColor.ToColor3();
}

void ComCamera::SetClearColor(const Color3 &clearColor) {
    renderViewDef.clearColor.ToColor3() = clearColor;

    UpdateVisuals();
}

float ComCamera::GetClearAlpha() const {
    return renderViewDef.clearColor.a;
}

void ComCamera::SetClearAlpha(const float clearAlpha) {
    renderViewDef.clearColor.a = clearAlpha;

    UpdateVisuals();
}

BE_NAMESPACE_END
