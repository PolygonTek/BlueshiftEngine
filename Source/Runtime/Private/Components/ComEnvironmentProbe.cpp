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
#include "Components/ComEnvironmentProbe.h"
#include "Game/GameWorld.h"
#include "Game/TagLayerSettings.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"
#include "Platform/PlatformTime.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Environment Probe", ComEnvironmentProbe, Component)
BEGIN_EVENTS(ComEnvironmentProbe)
END_EVENTS

void ComEnvironmentProbe::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("type", "Type", EnvProbe::Type, GetType, SetType, EnvProbe::Baked,
        "", PropertyInfo::EditorFlag).SetEnumString("Baked;Realtime");
    REGISTER_ACCESSOR_PROPERTY("refreshMode", "Refresh Mode", EnvProbe::RefreshMode, GetRefreshMode, SetRefreshMode, EnvProbe::OnAwake,
        "", PropertyInfo::EditorFlag).SetEnumString("OnAwake;EveryFrame");
    REGISTER_ACCESSOR_PROPERTY("timeSlicing", "Time Slicing", EnvProbe::TimeSlicing, GetTimeSlicing, SetTimeSlicing, EnvProbe::AllFacesAtOnce,
        "", PropertyInfo::EditorFlag).SetEnumString("All faces at once;Individual faces;No time slicing");
    REGISTER_ACCESSOR_PROPERTY("importance", "Importance", int, GetImportance, SetImportance, 1,
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("resolution", "Resolution", EnvProbe::Resolution, GetResolution, SetResolution, EnvProbe::Resolution128,
        "", PropertyInfo::EditorFlag).SetEnumString("16;32;64;128;256;512;1024;2048");
    REGISTER_ACCESSOR_PROPERTY("hdr", "HDR", bool, IsHDR, SetHDR, true,
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("cullingMask", "Culling Mask", int, GetLayerMask, SetLayerMask, -1,
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("clear", "Clear", EnvProbe::ClearMethod, GetClearMethod, SetClearMethod, EnvProbe::SkyClear,
        "", PropertyInfo::EditorFlag).SetEnumString("Color;Skybox");
    REGISTER_MIXED_ACCESSOR_PROPERTY("clearColor", "Clear Color", Color3, GetClearColor, SetClearColor, Color3(0.18, 0.30, 0.47),
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("clearAlpha", "Clear Alpha", float, GetClearAlpha, SetClearAlpha, 0.0f,
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("near", "Near", float, GetClippingNear, SetClippingNear, 0.1,
        "Near clipping plane distance", PropertyInfo::EditorFlag).SetRange(0.01, 10000, 0.02);
    REGISTER_ACCESSOR_PROPERTY("far", "Far", float, GetClippingFar, SetClippingFar, 500,
        "Far clipping plane distance", PropertyInfo::EditorFlag).SetRange(0.01, 10000, 0.02);
    REGISTER_ACCESSOR_PROPERTY("boxProjection", "Box Projection", bool, IsBoxProjection, SetBoxProjection, false,
        "", PropertyInfo::EditorFlag);
    REGISTER_MIXED_ACCESSOR_PROPERTY("boxOffset", "Box Offset", Vec3, GetBoxOffset, SetBoxOffset, Vec3(0, 0, 0),
        "The center of the box in which the reflections will be applied to objects", PropertyInfo::EditorFlag);
    REGISTER_MIXED_ACCESSOR_PROPERTY("boxExtent", "Box Extent", Vec3, GetBoxExtent, SetBoxExtent, Vec3(10, 10, 10),
        "The size of the box in which the reflections will be applied to objects", PropertyInfo::EditorFlag).SetRange(0, 1e8, 0.05);
    REGISTER_MIXED_ACCESSOR_PROPERTY("bakedDiffuseProbeTexture", "Baked Diffuse Probe", Guid, GetBakedDiffuseProbeTextureGuid, SetBakedDiffuseProbeTextureGuid, Guid::zero,
        "", PropertyInfo::NonCopying).SetMetaObject(&TextureAsset::metaObject);
    REGISTER_MIXED_ACCESSOR_PROPERTY("bakedSpecularProbeTexture", "Baked Specular Probe", Guid, GetBakedSpecularProbeTextureGuid, SetBakedSpecularProbeTextureGuid, Guid::zero,
        "", PropertyInfo::NonCopying).SetMetaObject(&TextureAsset::metaObject);
}

ComEnvironmentProbe::ComEnvironmentProbe() {
    probeHandle = -1;
    sphereHandle = -1;
    sphereMesh = nullptr;
}

ComEnvironmentProbe::~ComEnvironmentProbe() {
    Purge(false);
}

void ComEnvironmentProbe::Purge(bool chainPurge) {
    if (probeDef.bakedDiffuseProbeTexture) {
        textureManager.ReleaseTexture(probeDef.bakedDiffuseProbeTexture);
        probeDef.bakedDiffuseProbeTexture = nullptr;
    }

    if (probeDef.bakedSpecularProbeTexture) {
        textureManager.ReleaseTexture(probeDef.bakedSpecularProbeTexture);
        probeDef.bakedSpecularProbeTexture = nullptr;
    }

    if (sphereDef.mesh) {
        meshManager.ReleaseMesh(sphereDef.mesh);
        sphereDef.mesh = nullptr;
    }

    if (sphereMesh) {
        meshManager.ReleaseMesh(sphereMesh);
        sphereMesh = nullptr;
    }

    for (int i = 0; i < sphereDef.materials.Count(); i++) {
        materialManager.ReleaseMaterial(sphereDef.materials[i]);
    }
    sphereDef.materials.Clear();

    if (sphereHandle != -1) {
        renderWorld->RemoveRenderObject(sphereHandle);
        sphereHandle = -1;
    }

    if (chainPurge) {
        Component::Purge();
    }
}

void ComEnvironmentProbe::Init() {
    Component::Init();

    renderWorld = GetGameWorld()->GetRenderWorld();

    ComTransform *transform = GetEntity()->GetTransform();

    probeDef.origin = transform->GetOrigin();

    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComEnvironmentProbe::TransformUpdated, SignalObject::Unique);

    sphereDef.layer = TagLayerSettings::EditorLayer;
    sphereDef.maxVisDist = MeterToUnit(50.0f);

    sphereMesh = meshManager.GetMesh("_defaultSphereMesh");

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

void ComEnvironmentProbe::OnActive() {
    UpdateVisuals();
}

void ComEnvironmentProbe::OnInactive() {
    renderWorld->RemoveRenderObject(sphereHandle);
    sphereHandle = -1;
}

bool ComEnvironmentProbe::HasRenderEntity(int renderEntityHandle) const {
    if (this->sphereHandle == renderEntityHandle) {
        return true;
    }
    return false;
}

void ComEnvironmentProbe::Awake() {
    if (!IsActiveInHierarchy()) {
        return;
    }

    if (probeDef.type == EnvProbe::Type::Realtime) {
        if (probeDef.refreshMode == EnvProbe::RefreshMode::OnAwake) {
            renderSystem.ScheduleToRefreshEnvProbe(renderWorld, probeHandle);
        }
    }
}

void ComEnvironmentProbe::Update() {
    if (!IsActiveInHierarchy()) {
        return;
    }

    if (probeDef.type == EnvProbe::Type::Realtime) {
        if (probeDef.refreshMode == EnvProbe::RefreshMode::EveryFrame) {
            renderSystem.ScheduleToRefreshEnvProbe(renderWorld, probeHandle);
        }
    }
}

void ComEnvironmentProbe::DrawGizmos(const RenderCamera::State &viewState, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    if (selected) {
        AABB aabb = AABB(-probeDef.boxExtent, probeDef.boxExtent);
        aabb += probeDef.origin + probeDef.boxOffset;
        
        renderWorld->SetDebugColor(Color4(0.0f, 0.5f, 1.0f, 1.0f), Color4::zero);
        renderWorld->DebugAABB(aabb, 1.0f, false, true, true);

        gizmoCurrentTime = PlatformTime::Milliseconds();

        if (gizmoCurrentTime > gizmoRefreshTime + 3000) {
            gizmoRefreshTime = gizmoCurrentTime;

            if (probeDef.type == EnvProbe::Type::Realtime) {
                renderSystem.ScheduleToRefreshEnvProbe(renderWorld, probeHandle);
            }
        }
    }
}

const AABB ComEnvironmentProbe::GetAABB() {
    return Sphere(Vec3::origin, MeterToUnit(0.5f)).ToAABB();
}

void ComEnvironmentProbe::UpdateVisuals() {
    if (!IsInitialized() || !IsActiveInHierarchy()) {
        return;
    }

    if (probeHandle == -1) {
        probeHandle = renderWorld->AddEnvProbe(&probeDef);
    } else {
        renderWorld->UpdateEnvProbe(probeHandle, &probeDef);
    }

    if (sphereDef.materials.Count() == 0) {
        EnvProbe *envProbe = renderWorld->GetEnvProbe(probeHandle);
        Texture *specularProbeTexture = envProbe->GetSpecularProbeTexture();

        sphereDef.materials.SetCount(1);
        sphereDef.materials[0] = materialManager.GetSingleTextureMaterial(specularProbeTexture, Material::EnvCubeMapHint);
    }

    if (sphereHandle == -1) {
        sphereHandle = renderWorld->AddRenderObject(&sphereDef);
    } else {
        renderWorld->UpdateRenderObject(sphereHandle, &sphereDef);
    }
}

void ComEnvironmentProbe::TransformUpdated(const ComTransform *transform) {
    probeDef.origin = transform->GetOrigin();

    sphereDef.origin = transform->GetOrigin();

    UpdateVisuals();
}

EnvProbe::Type ComEnvironmentProbe::GetType() const {
    return probeDef.type;
}

void ComEnvironmentProbe::SetType(EnvProbe::Type type) {
    probeDef.type = type;

    UpdateVisuals();
}

EnvProbe::RefreshMode ComEnvironmentProbe::GetRefreshMode() const {
    return probeDef.refreshMode;
}

void ComEnvironmentProbe::SetRefreshMode(EnvProbe::RefreshMode refreshMode) {
    probeDef.refreshMode = refreshMode;

    UpdateVisuals();
}

EnvProbe::TimeSlicing ComEnvironmentProbe::GetTimeSlicing() const {
    return probeDef.timeSlicing;
}

void ComEnvironmentProbe::SetTimeSlicing(EnvProbe::TimeSlicing timeSlicing) {
    probeDef.timeSlicing = timeSlicing;

    UpdateVisuals();
}

int ComEnvironmentProbe::GetImportance() const {
    return probeDef.importance;
}

void ComEnvironmentProbe::SetImportance(int importance) {
    probeDef.importance = Max(importance, 0);

    UpdateVisuals();
}

EnvProbe::Resolution ComEnvironmentProbe::GetResolution() const {
    return probeDef.resolution;
}

void ComEnvironmentProbe::SetResolution(EnvProbe::Resolution resolution) {
    probeDef.resolution = resolution;

    UpdateVisuals();
}

bool ComEnvironmentProbe::IsHDR() const {
    return probeDef.useHDR;
}

void ComEnvironmentProbe::SetHDR(bool useHDR) {
    probeDef.useHDR = useHDR;

    UpdateVisuals();
}

int ComEnvironmentProbe::GetLayerMask() const {
    return probeDef.layerMask;
}

void ComEnvironmentProbe::SetLayerMask(int layerMask) {
    probeDef.layerMask = layerMask;

    UpdateVisuals();
}

EnvProbe::ClearMethod ComEnvironmentProbe::GetClearMethod() const {
    return probeDef.clearMethod;
}

void ComEnvironmentProbe::SetClearMethod(EnvProbe::ClearMethod clearMethod) {
    probeDef.clearMethod = clearMethod;

    UpdateVisuals();
}

Color3 ComEnvironmentProbe::GetClearColor() const {
    return probeDef.clearColor.ToColor3();
}

void ComEnvironmentProbe::SetClearColor(const Color3 &clearColor) {
    probeDef.clearColor.ToColor3() = clearColor;

    UpdateVisuals();
}

float ComEnvironmentProbe::GetClearAlpha() const {
    return probeDef.clearColor.a;
}

void ComEnvironmentProbe::SetClearAlpha(float clearAlpha) {
    probeDef.clearColor.a = clearAlpha;

    UpdateVisuals();
}

float ComEnvironmentProbe::GetClippingNear() const {
    return probeDef.clippingNear;
}

void ComEnvironmentProbe::SetClippingNear(float clippingNear) {
    probeDef.clippingNear = Max(clippingNear, 0.01f);

    if (probeDef.clippingNear > probeDef.clippingFar) {
        SetProperty("far", probeDef.clippingNear);
    }

    UpdateVisuals();
}

float ComEnvironmentProbe::GetClippingFar() const {
    return probeDef.clippingFar;
}

void ComEnvironmentProbe::SetClippingFar(float clippingFar) {
    if (clippingFar >= probeDef.clippingNear) {
        probeDef.clippingFar = clippingFar;
    }

    UpdateVisuals();
}

bool ComEnvironmentProbe::IsBoxProjection() const {
    return probeDef.useBoxProjection;
}

void ComEnvironmentProbe::SetBoxProjection(bool useBoxProjection) {
    probeDef.useBoxProjection = useBoxProjection;

    UpdateVisuals();
}

Vec3 ComEnvironmentProbe::GetBoxExtent() const {
    return probeDef.boxExtent;
}

void ComEnvironmentProbe::SetBoxExtent(const Vec3 &boxExtent) {
    probeDef.boxExtent = boxExtent;

    // The origin must be included in the box range.
    // So if it doesn't we need to adjust box offset.
    Vec3 adjustedBoxOffset = probeDef.boxOffset;

    for (int i = 0; i < 3; i++) {
        float delta = probeDef.boxOffset[i] - probeDef.boxExtent[i];
        if (delta > 0) {
            adjustedBoxOffset[i] = probeDef.boxOffset[i] - delta;
        }
    }

    if (adjustedBoxOffset != probeDef.boxExtent) {
        SetProperty("boxOffset", adjustedBoxOffset);
    }

    UpdateVisuals();
}

Vec3 ComEnvironmentProbe::GetBoxOffset() const {
    return probeDef.boxOffset;
}

void ComEnvironmentProbe::SetBoxOffset(const Vec3 &boxOffset) {
    probeDef.boxOffset = boxOffset;

    // The origin must be included in the box range.
    // So if it doesn't we need to adjust box size.
    Vec3 adjustedBoxExtent = probeDef.boxExtent;

    for (int i = 0; i < 3; i++) {
        float delta = probeDef.boxOffset[i] - probeDef.boxExtent[i];
        if (delta > 0) {
            adjustedBoxExtent[i] = probeDef.boxExtent[i] + delta;
        }
    }

    if (adjustedBoxExtent != probeDef.boxExtent) {
        SetProperty("boxExtent", adjustedBoxExtent);
    }

    UpdateVisuals();
}

Guid ComEnvironmentProbe::GetBakedDiffuseProbeTextureGuid() const {
    if (probeDef.bakedDiffuseProbeTexture) {
        const Str texturePath = probeDef.bakedDiffuseProbeTexture->GetHashName();
        return resourceGuidMapper.Get(texturePath);
    }
    return Guid();
}

void ComEnvironmentProbe::SetBakedDiffuseProbeTextureGuid(const Guid &textureGuid) {
    if (probeDef.bakedDiffuseProbeTexture) {
        textureManager.ReleaseTexture(probeDef.bakedDiffuseProbeTexture);
    }

    if (!textureGuid.IsZero()) {
        const Str texturePath = resourceGuidMapper.Get(textureGuid);

        probeDef.bakedDiffuseProbeTexture = textureManager.GetTexture(texturePath);

        if (probeDef.bakedDiffuseProbeTexture->IsDefaultTexture()) {
            probeDef.bakedDiffuseProbeTexture = nullptr;
        }
    }

    UpdateVisuals();
}

Guid ComEnvironmentProbe::GetBakedSpecularProbeTextureGuid() const {
    if (probeDef.bakedSpecularProbeTexture) {
        const Str texturePath = probeDef.bakedSpecularProbeTexture->GetHashName();
        return resourceGuidMapper.Get(texturePath);
    }
    return Guid();
}

void ComEnvironmentProbe::SetBakedSpecularProbeTextureGuid(const Guid &textureGuid) {
    if (probeDef.bakedSpecularProbeTexture) {
        textureManager.ReleaseTexture(probeDef.bakedSpecularProbeTexture);
    }

    if (!textureGuid.IsZero()) {
        const Str texturePath = resourceGuidMapper.Get(textureGuid);

        probeDef.bakedSpecularProbeTexture = textureManager.GetTexture(texturePath);

        if (probeDef.bakedSpecularProbeTexture->IsDefaultTexture()) {
            probeDef.bakedSpecularProbeTexture = nullptr;
        }
    }

    UpdateVisuals();
}

Texture *ComEnvironmentProbe::GetDiffuseProbeTexture() const {
    EnvProbe *envProbe = renderWorld->GetEnvProbe(probeHandle);

    return envProbe->GetDiffuseProbeTexture();
}

Texture *ComEnvironmentProbe::GetSpecularProbeTexture() const {
    EnvProbe *envProbe = renderWorld->GetEnvProbe(probeHandle);

    return envProbe->GetSpecularProbeTexture();
}

void ComEnvironmentProbe::ForceToRefresh() {
    renderSystem.ForceToRefreshEnvProbe(renderWorld, probeHandle);
}

Str ComEnvironmentProbe::WriteDiffuseProbeTexture(const Str &probesDir) const {
    Str diffuseProbeFilename = probesDir;
    diffuseProbeFilename.AppendPath(va("DiffuseProbe-%i.dds", probeHandle));

    const Texture *diffuseProbeTexture = GetDiffuseProbeTexture();
    Image diffuseProbeImage;
    Texture::GetCubeImageFromCubeTexture(diffuseProbeTexture, 1, diffuseProbeImage);

    diffuseProbeImage.WriteDDS(diffuseProbeFilename);

    return diffuseProbeFilename;
}

Str ComEnvironmentProbe::WriteSpecularProbeTexture(const Str &probesDir) const {
    Str specularProbeFilename = probesDir;
    specularProbeFilename.AppendPath(va("SpecularProbe-%i.dds", probeHandle));

    const Texture *specularProbeTexture = GetSpecularProbeTexture();

    Image specularProbeImage;
    int numMipLevels = Math::Log(2, specularProbeTexture->GetWidth()) + 1;
    Texture::GetCubeImageFromCubeTexture(specularProbeTexture, numMipLevels, specularProbeImage);

    specularProbeImage.WriteDDS(specularProbeFilename);

    return specularProbeFilename;
}

BE_NAMESPACE_END
