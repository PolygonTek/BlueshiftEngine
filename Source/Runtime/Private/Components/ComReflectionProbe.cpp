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
#include "Components/ComReflectionProbe.h"
#include "Game/GameWorld.h"
#include "Game/TagLayerSettings.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Reflection Probe", ComReflectionProbe, Component)
BEGIN_EVENTS(ComReflectionProbe)
END_EVENTS

void ComReflectionProbe::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("type", "Type", ReflectionProbe::Type, GetType, SetType, ReflectionProbe::Baked,
        "", PropertyInfo::EditorFlag).SetEnumString("Baked;Realtime");
    REGISTER_ACCESSOR_PROPERTY("importance", "Importance", int, GetImportance, SetImportance, 1,
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("resolution", "Resolution", int, GetResolution, SetResolution, 3,
        "", PropertyInfo::EditorFlag).SetEnumString("16;32;64;128;256;1024;2048");
    REGISTER_ACCESSOR_PROPERTY("hdr", "HDR", bool, IsHDR, SetHDR, true,
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("clear", "Clear", ReflectionProbe::ClearMethod, GetClearMethod, SetClearMethod, 1,
        "", PropertyInfo::EditorFlag).SetEnumString("Color;Skybox");
    REGISTER_MIXED_ACCESSOR_PROPERTY("clearColor", "Clear Color", Color3, GetClearColor, SetClearColor, Color3(0, 0, 0),
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("clearAlpha", "Clear Alpha", float, GetClearAlpha, SetClearAlpha, 0.0f,
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("near", "Near", float, GetClippingNear, SetClippingNear, 0.3f,
        "Near clipping plane distance", PropertyInfo::EditorFlag).SetRange(0.01, 1000, 10);
    REGISTER_ACCESSOR_PROPERTY("far", "Far", float, GetClippingFar, SetClippingFar, 1000.f,
        "Far clipping plane distance", PropertyInfo::EditorFlag).SetRange(1, 1000, 10);
    REGISTER_ACCESSOR_PROPERTY("boxProjection", "Box Projection", bool, IsBoxProjection, SetBoxProjection, false,
        "", PropertyInfo::EditorFlag);
    REGISTER_MIXED_ACCESSOR_PROPERTY("boxOffset", "Box Offset", Vec3, GetBoxOffset, SetBoxOffset, Vec3(0, 0, 0), 
        "The center of the box in which the reflections will be applied to objects", PropertyInfo::EditorFlag),
    REGISTER_MIXED_ACCESSOR_PROPERTY("boxSize", "Box Size", Vec3, GetBoxSize, SetBoxSize, Vec3(10, 10, 10), 
        "The size of the box in which the reflections will be applied to objects", PropertyInfo::EditorFlag);
}

ComReflectionProbe::ComReflectionProbe() {
    sphereHandle = -1;
    sphereMesh = nullptr;
}

ComReflectionProbe::~ComReflectionProbe() {
    Purge(false);
}

void ComReflectionProbe::Purge(bool chainPurge) {
    if (sphereDef.mesh) {
        meshManager.ReleaseMesh(sphereDef.mesh);
        sphereDef.mesh = nullptr;
    }

    if (sphereMesh) {
        meshManager.ReleaseMesh(sphereMesh);
        sphereMesh = nullptr;
    }

    if (sphereHandle != -1) {
        renderWorld->RemoveRenderObject(sphereHandle);
        sphereHandle = -1;
    }

    if (chainPurge) {
        Component::Purge();
    }
}

void ComReflectionProbe::Init() {
    Component::Init();

    renderWorld = GetGameWorld()->GetRenderWorld();

    ComTransform *transform = GetEntity()->GetTransform();

    // sphereDef
    sphereMesh = meshManager.GetMesh("_defaultSphereMesh");

    sphereDef.layer = TagLayerSettings::EditorLayer;
    sphereDef.maxVisDist = MeterToUnit(50.0f);

    // FIXME
    Texture *spriteTexture = textureManager.GetTexture("Data/EditorUI/ReflectionProbe.png", Texture::Clamp | Texture::HighQuality);
    sphereDef.materials.SetCount(1);
    sphereDef.materials[0] = materialManager.GetSingleTextureMaterial(spriteTexture);
    textureManager.ReleaseTexture(spriteTexture);
    //

    sphereDef.mesh = sphereMesh->InstantiateMesh(Mesh::StaticMesh);
    sphereDef.localAABB = sphereMesh->GetAABB();
    sphereDef.origin = transform->GetOrigin();
    sphereDef.scale = Vec3(1, 1, 1);
    sphereDef.axis = Mat3::identity;
    sphereDef.materialParms[RenderObject::RedParm] = 1.0f;
    sphereDef.materialParms[RenderObject::GreenParm] = 1.0f;
    sphereDef.materialParms[RenderObject::BlueParm] = 1.0f;
    sphereDef.materialParms[RenderObject::AlphaParm] = 1.0f;
    sphereDef.materialParms[RenderObject::TimeOffsetParm] = 0.0f;
    sphereDef.materialParms[RenderObject::TimeScaleParm] = 1.0f;

    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComReflectionProbe::TransformUpdated, SignalObject::Unique);

    // Mark as initialized
    SetInitialized(true);

    UpdateVisuals();
}

void ComReflectionProbe::OnActive() {
    UpdateVisuals();
}

void ComReflectionProbe::OnInactive() {
    renderWorld->RemoveRenderObject(sphereHandle);
    sphereHandle = -1;
}

bool ComReflectionProbe::HasRenderEntity(int renderEntityHandle) const {
    if (this->sphereHandle == renderEntityHandle) {
        return true;
    }

    return false;
}

bool ComReflectionProbe::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    return false;
}

void ComReflectionProbe::DrawGizmos(const RenderCamera::State &viewState, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    if (selected) {
        //AABB aabb;
        //renderWorld->SetDebugColor(Color4(1.0f, 1.0f, 0.5f, 0.5f), Color4::zero);
        //renderWorld->DebugAABB(aabb, 1.0f, false, false, true);
    }
}

const AABB ComReflectionProbe::GetAABB() {
    return Sphere(Vec3::origin, MeterToUnit(0.5f)).ToAABB();
}

void ComReflectionProbe::UpdateVisuals() {
    if (!IsInitialized() || !IsActiveInHierarchy()) {
        return;
    }

    if (sphereHandle == -1) {
        sphereHandle = renderWorld->AddRenderObject(&sphereDef);
    } else {
        renderWorld->UpdateRenderObject(sphereHandle, &sphereDef);
    }
}

void ComReflectionProbe::TransformUpdated(const ComTransform *transform) {
    //viewParms.origin = transform->GetOrigin();
    //viewParms.axis = transform->GetAxis();

    sphereDef.origin = transform->GetOrigin();

    UpdateVisuals();
}

ReflectionProbe::Type ComReflectionProbe::GetType() const {
    return ReflectionProbe::Type::Baked;
}

void ComReflectionProbe::SetType(ReflectionProbe::Type type) { 
}

int ComReflectionProbe::GetImportance() const { 
    return 1;
}

void ComReflectionProbe::SetImportance(int importance) { 
}

int ComReflectionProbe::GetResolution() const { 
    return 3;
}

void ComReflectionProbe::SetResolution(int resolution) { 
}

bool ComReflectionProbe::IsHDR() const { 
    return true;
}

void ComReflectionProbe::SetHDR(bool hdr) { 
}

ReflectionProbe::ClearMethod ComReflectionProbe::GetClearMethod() const { 
    return ReflectionProbe::ClearMethod::SkyClear;
}

void ComReflectionProbe::SetClearMethod(ReflectionProbe::ClearMethod clearMethod) { 
}

Color3 ComReflectionProbe::GetClearColor() const { 
    return Color3::black;
}

void ComReflectionProbe::SetClearColor(const Color3 &clearColor) { 
}

float ComReflectionProbe::GetClearAlpha() const { 
    return 0.0f;
}

void ComReflectionProbe::SetClearAlpha(float clearAlpha) { 
}

float ComReflectionProbe::GetClippingNear() const { 
    return 0.3f;
}

void ComReflectionProbe::SetClippingNear(float clippingNear) { 
}

float ComReflectionProbe::GetClippingFar() const { 
    return 1000.0f;
}

void ComReflectionProbe::SetClippingFar(float clippingFar) { 
}

bool ComReflectionProbe::IsBoxProjection() const { 
    return false;
}

void ComReflectionProbe::SetBoxProjection(bool boxProjection) { 
}

Vec3 ComReflectionProbe::GetBoxOffset() const { 
    return Vec3(0, 0, 0);
}

void ComReflectionProbe::SetBoxOffset(const Vec3 &boxOffset) { 
}

Vec3 ComReflectionProbe::GetBoxSize() const {
    return Vec3(10, 10, 10);
}

void ComReflectionProbe::SetBoxSize(const Vec3 &boxSize) { 
}

BE_NAMESPACE_END
