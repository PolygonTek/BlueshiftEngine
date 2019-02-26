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
    REGISTER_ACCESSOR_PROPERTY("resolution", "Resolution", ReflectionProbe::Resolution, GetResolution, SetResolution, ReflectionProbe::Resolution128,
        "", PropertyInfo::EditorFlag).SetEnumString("16;32;64;128;256;1024;2048");
    REGISTER_ACCESSOR_PROPERTY("hdr", "HDR", bool, IsHDR, SetHDR, true,
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("clear", "Clear", ReflectionProbe::ClearMethod, GetClearMethod, SetClearMethod, 1,
        "", PropertyInfo::EditorFlag).SetEnumString("Color;Skybox");
    REGISTER_MIXED_ACCESSOR_PROPERTY("clearColor", "Clear Color", Color3, GetClearColor, SetClearColor, Color3(0, 0, 0),
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("clearAlpha", "Clear Alpha", float, GetClearAlpha, SetClearAlpha, 0.0f,
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("near", "Near", float, GetClippingNear, SetClippingNear, 0.1,
        "Near clipping plane distance", PropertyInfo::EditorFlag).SetRange(0.01, 10000, 0.02);
    REGISTER_ACCESSOR_PROPERTY("far", "Far", float, GetClippingFar, SetClippingFar, 500,
        "Far clipping plane distance", PropertyInfo::EditorFlag).SetRange(0.01, 10000, 0.02);
    REGISTER_ACCESSOR_PROPERTY("boxProjection", "Box Projection", bool, IsBoxProjection, SetBoxProjection, false,
        "", PropertyInfo::EditorFlag);
    REGISTER_MIXED_ACCESSOR_PROPERTY("boxSize", "Box Size", Vec3, GetBoxSize, SetBoxSize, Vec3(10, 10, 10),
        "The size of the box in which the reflections will be applied to objects", PropertyInfo::EditorFlag).SetRange(0, 1e8, 0.05);
    REGISTER_MIXED_ACCESSOR_PROPERTY("boxOffset", "Box Offset", Vec3, GetBoxOffset, SetBoxOffset, Vec3(0, 0, 0),
        "The center of the box in which the reflections will be applied to objects", PropertyInfo::EditorFlag);
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

    probeDef.origin = transform->GetOrigin();

    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComReflectionProbe::TransformUpdated, SignalObject::Unique);

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
        AABB aabb = AABB(-probeDef.boxSize, probeDef.boxSize);
        aabb += probeDef.origin + probeDef.boxOffset;
        
        renderWorld->SetDebugColor(Color4(1.0f, 1.0f, 0.5f, 1.0f), Color4::zero);
        renderWorld->DebugAABB(aabb, 1.0f, false, true, true);
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
    probeDef.origin = transform->GetOrigin();

    sphereDef.origin = transform->GetOrigin();

    UpdateVisuals();
}

ReflectionProbe::Type ComReflectionProbe::GetType() const {
    return probeDef.type;
}

void ComReflectionProbe::SetType(ReflectionProbe::Type type) { 
    probeDef.type = type;
}

int ComReflectionProbe::GetImportance() const { 
    return probeDef.importance;
}

void ComReflectionProbe::SetImportance(int importance) { 
    probeDef.importance = importance;
}

ReflectionProbe::Resolution ComReflectionProbe::GetResolution() const {
    return probeDef.resolution;
}

void ComReflectionProbe::SetResolution(ReflectionProbe::Resolution resolution) { 
    probeDef.resolution = resolution;

    //Invalidate();
}

bool ComReflectionProbe::IsHDR() const { 
    return probeDef.useHDR;
}

void ComReflectionProbe::SetHDR(bool useHDR) {
    probeDef.useHDR = useHDR;

    //Invalidate();
}

ReflectionProbe::ClearMethod ComReflectionProbe::GetClearMethod() const { 
    return probeDef.clearMethod;
}

void ComReflectionProbe::SetClearMethod(ReflectionProbe::ClearMethod clearMethod) { 
    probeDef.clearMethod = clearMethod;

    //Invalidate();
}

Color3 ComReflectionProbe::GetClearColor() const { 
    return probeDef.clearColor.ToColor3();
}

void ComReflectionProbe::SetClearColor(const Color3 &clearColor) { 
    probeDef.clearColor.ToColor3() = clearColor;

    //Invalidate();
}

float ComReflectionProbe::GetClearAlpha() const { 
    return probeDef.clearColor.a;
}

void ComReflectionProbe::SetClearAlpha(float clearAlpha) { 
    probeDef.clearColor.a = clearAlpha;

    //Invalidate();
}

float ComReflectionProbe::GetClippingNear() const { 
    return probeDef.clippingNear;
}

void ComReflectionProbe::SetClippingNear(float clippingNear) { 
    probeDef.clippingNear = clippingNear;

    if (probeDef.clippingNear > probeDef.clippingFar) {
        SetProperty("far", probeDef.clippingNear);
    }

    //Invalidate();
}

float ComReflectionProbe::GetClippingFar() const { 
    return probeDef.clippingFar;
}

void ComReflectionProbe::SetClippingFar(float clippingFar) { 
    if (clippingFar >= probeDef.clippingNear) {
        probeDef.clippingFar = clippingFar;
    }

    //Invalidate();
}

bool ComReflectionProbe::IsBoxProjection() const { 
    return probeDef.useBoxProjection;
}

void ComReflectionProbe::SetBoxProjection(bool useBoxProjection) { 
    probeDef.useBoxProjection = useBoxProjection;
}

Vec3 ComReflectionProbe::GetBoxSize() const {
    return probeDef.boxSize;
}

void ComReflectionProbe::SetBoxSize(const Vec3 &boxSize) {
    probeDef.boxSize = boxSize;

    // The origin must be included in the box range.
    // So if it doesn't we need to adjust box offset.
    Vec3 adjustedBoxOffset = probeDef.boxOffset;

    for (int i = 0; i < 3; i++) {
        float delta = probeDef.boxOffset[i] - probeDef.boxSize[i];
        if (delta > 0) {
            adjustedBoxOffset[i] = probeDef.boxOffset[i] - delta;
        }
    }

    if (adjustedBoxOffset != probeDef.boxSize) {
        SetProperty("boxOffset", adjustedBoxOffset);
    }
}

Vec3 ComReflectionProbe::GetBoxOffset() const { 
    return probeDef.boxOffset;
}

void ComReflectionProbe::SetBoxOffset(const Vec3 &boxOffset) { 
    probeDef.boxOffset = boxOffset;

    // The origin must be included in the box range.
    // So if it doesn't we need to adjust box size.
    Vec3 adjustedBoxSize = probeDef.boxSize;

    for (int i = 0; i < 3; i++) {
        float delta = probeDef.boxOffset[i] - probeDef.boxSize[i];
        if (delta > 0) {
            adjustedBoxSize[i] = probeDef.boxSize[i] + delta;
        }
    }

    if (adjustedBoxSize != probeDef.boxSize) {
        SetProperty("boxSize", adjustedBoxSize);
    }
}

BE_NAMESPACE_END
