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
    REGISTER_ACCESSOR_PROPERTY("near", "Near", float, GetNear, SetNear, 0.1,
        "", PropertyInfo::SystemUnits | PropertyInfo::EditorFlag).SetRange(0.01, 10000, 0.02);
    REGISTER_ACCESSOR_PROPERTY("far", "Far", float, GetFar, SetFar, 500,
        "", PropertyInfo::SystemUnits | PropertyInfo::EditorFlag).SetRange(0.01, 10000, 0.02);
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
    REGISTER_ACCESSOR_PROPERTY("clear", "Clear/Clear Method", RenderCamera::ClearMethod, GetClearMethod, SetClearMethod, 1, 
        "", PropertyInfo::EditorFlag).SetEnumString("No Clear;Depth Only;Color;Skybox");
    REGISTER_ACCESSOR_PROPERTY("clearColor", "Clear/Color", Color3, GetClearColor, SetClearColor, Color3::black, 
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("clearAlpha", "Clear/Alpha", float, GetClearAlpha, SetClearAlpha, 0.f, 
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("layerMask", "Layer Mask", int, GetLayerMask, SetLayerMask, (int)(BIT(TagLayerSettings::DefaultLayer) | BIT(TagLayerSettings::UILayer)),
        "", PropertyInfo::EditorFlag);
    REGISTER_PROPERTY("order", "Order", int, order, 0,
        "", PropertyInfo::EditorFlag);
    REGISTER_PROPERTY("applyPostProcessing", "Apply Post Processing", bool, applyPostProcessing, true,
        "", PropertyInfo::EditorFlag);
}

ComCamera::ComCamera() {
    renderCamera = nullptr;

    mousePointerState.oldHitEntityGuid = Guid::zero;
    mousePointerState.captureEntityGuid = Guid::zero;

#if 1
    spriteHandle = -1;
    spriteMesh = nullptr;
#endif
}

ComCamera::~ComCamera() {
    Purge(false);
}

void ComCamera::Purge(bool chainPurge) {
#if 1
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

#if 1
    // 3d sprite for editor
    spriteMesh = meshManager.GetMesh("_defaultQuadMesh");

    spriteDef.flags = RenderObject::BillboardFlag;
    spriteDef.layer = TagLayerSettings::EditorLayer;
    spriteDef.maxVisDist = MeterToUnit(50.0f);

    Texture *spriteTexture = textureManager.GetTexture("Data/EditorUI/Camera2.png", Texture::Clamp | Texture::HighQuality);
    spriteDef.materials.SetCount(1);
    spriteDef.materials[0] = materialManager.GetSingleTextureMaterial(spriteTexture, Material::SpriteHint);
    textureManager.ReleaseTexture(spriteTexture);
    
    spriteDef.mesh = spriteMesh->InstantiateMesh(Mesh::StaticMesh);
    spriteDef.aabb = spriteMesh->GetAABB();
    spriteDef.worldMatrix = transform->GetMatrixNoScale();
    spriteDef.materialParms[RenderObject::RedParm] = 1.0f;
    spriteDef.materialParms[RenderObject::GreenParm] = 1.0f;
    spriteDef.materialParms[RenderObject::BlueParm] = 1.0f;
    spriteDef.materialParms[RenderObject::AlphaParm] = 1.0f;
    spriteDef.materialParms[RenderObject::TimeOffsetParm] = 0.0f;
    spriteDef.materialParms[RenderObject::TimeScaleParm] = 1.0f;
#endif

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

#if 1
    if (spriteHandle != -1) {
        renderWorld->RemoveRenderObject(spriteHandle);
        spriteHandle = -1;
    }
#endif
}

bool ComCamera::HasRenderEntity(int renderEntityHandle) const { 
#if 1
    if (this->spriteHandle == renderEntityHandle) {
        return true;
    }
#endif

    return false;
}

void ComCamera::Update() {
    renderCameraDef.time = GetGameWorld()->GetTime();
}

#if 1
void ComCamera::DrawGizmos(const RenderCamera::State &renderCameraDef, bool selected) {
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

        if (this->renderCameraDef.orthogonal) {
            this->renderCameraDef.sizeX = size;
            this->renderCameraDef.sizeY = size / aspectRatio;
            float sizeZ = (this->renderCameraDef.zNear + this->renderCameraDef.zFar) * 0.5f;

            OBB cameraBox;
            cameraBox.SetAxis(this->renderCameraDef.axis);
            cameraBox.SetCenter(this->renderCameraDef.origin + this->renderCameraDef.axis[0] * sizeZ);
            cameraBox.SetExtents(Vec3(sizeZ, this->renderCameraDef.sizeX, this->renderCameraDef.sizeY));

            renderWorld->SetDebugColor(Color4::white, Color4::zero);
            renderWorld->DebugOBB(cameraBox, 1.0f, false, false, true);
        } else {
            RenderCamera::ComputeFov(fov, 1.25f, aspectRatio, &this->renderCameraDef.fovX, &this->renderCameraDef.fovY);

            Frustum cameraFrustum;
            cameraFrustum.SetOrigin(this->renderCameraDef.origin);
            cameraFrustum.SetAxis(this->renderCameraDef.axis);
            cameraFrustum.SetSize(this->renderCameraDef.zNear, this->renderCameraDef.zFar,
                this->renderCameraDef.zFar * Math::Tan(DEG2RAD(this->renderCameraDef.fovX * 0.5f)), this->renderCameraDef.zFar * Math::Tan(DEG2RAD(this->renderCameraDef.fovY * 0.5f)));

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

            RenderCamera::State previewViewState = this->renderCameraDef;

            previewViewState.renderRect.Set(x, y, w, h);
            previewViewState.flags |= RenderCamera::SkipDebugDraw;

            static RenderCamera previewView;
            previewView.Update(&previewViewState);

            GetGameWorld()->GetRenderWorld()->RenderScene(&previewView);
        }*/
    }

    // Fade icon alpha in near distance
    float alpha = BE1::Clamp(spriteDef.worldMatrix.ToTranslationVec3().Distance(renderCameraDef.origin) / MeterToUnit(8.0f), 0.01f, 1.0f);

    spriteDef.materials[0]->GetPass()->constantColor[3] = alpha;
}
#endif 

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

    // right/down normalized screen coordinates [-1.0, +1.0]
    float ndx = ((float)(screenPoint.x - screenRect.x) / screenRect.w) * 2.0f - 1.0f;
    float ndy = ((float)(screenPoint.y - screenRect.y) / screenRect.h) * 2.0f - 1.0f;

    float aspectRatio = (float)screenWidth / screenHeight;

    if (renderCameraDef.orthogonal) {
        renderCameraDef.sizeX = size;
        renderCameraDef.sizeY = size / aspectRatio;
    } else {
         RenderCamera::ComputeFov(fov, 1.25f, aspectRatio, &renderCameraDef.fovX, &renderCameraDef.fovY);
    }

    return RenderCamera::RayFromScreenND(renderCameraDef, ndx, ndy);
}

bool ComCamera::ProcessMousePointerInput(const Point &screenPoint) {
    if (!inputSystem.IsMouseExist()) {
        return false;
    }

    Ray ray = ScreenToRay(screenPoint);

    Entity *hitTestEntity = nullptr;
    CastResultEx castResult;

    if (GetGameWorld()->GetPhysicsWorld()->RayCast(nullptr, ray.origin, ray.GetPoint(MeterToUnit(1000.0f)), GetLayerMask(), castResult)) {
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

            if (GetGameWorld()->GetPhysicsWorld()->RayCast(nullptr, ray.origin, ray.GetPoint(MeterToUnit(1000.0f)), GetLayerMask(), castResult)) {
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

    // Get the aspect ratio from device screen size.
    float aspectRatio = (float)ctx->GetScreenWidth() / ctx->GetScreenHeight();

    if (renderCameraDef.orthogonal) {
        // Compute viewport rectangle size in orthogonal projection.
        renderCameraDef.sizeX = size;
        renderCameraDef.sizeY = size / aspectRatio;
    } else {
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

#if 1
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

#if 1
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

    if (renderCameraDef.zNear > renderCameraDef.zFar) {
        SetProperty("far", renderCameraDef.zNear);
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

RenderCamera::ClearMethod ComCamera::GetClearMethod() const {
    return renderCameraDef.clearMethod;
}

void ComCamera::SetClearMethod(RenderCamera::ClearMethod clearMethod) {
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
