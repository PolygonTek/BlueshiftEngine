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
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"
#include "Platform/PlatformTime.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Environment Probe", ComEnvironmentProbe, Component)
BEGIN_EVENTS(ComEnvironmentProbe)
END_EVENTS

void ComEnvironmentProbe::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("type", "Type", EnvProbe::Type::Enum, GetType, SetType, EnvProbe::Type::Baked,
        "", PropertyInfo::Flag::Editor).SetEnumString("Baked;Realtime");
    REGISTER_ACCESSOR_PROPERTY("refreshMode", "Refresh Mode", EnvProbe::RefreshMode::Enum, GetRefreshMode, SetRefreshMode, EnvProbe::RefreshMode::OnAwake,
        "", PropertyInfo::Flag::Editor).SetEnumString("OnAwake;EveryFrame");
    REGISTER_ACCESSOR_PROPERTY("timeSlicing", "Time Slicing", EnvProbe::TimeSlicing::Enum, GetTimeSlicing, SetTimeSlicing, EnvProbe::TimeSlicing::AllFacesAtOnce,
        "", PropertyInfo::Flag::Editor).SetEnumString("All faces at once;Individual faces;No time slicing");
    REGISTER_ACCESSOR_PROPERTY("importance", "Importance", int, GetImportance, SetImportance, 1,
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("resolution", "Resolution", EnvProbe::Resolution::Enum, GetResolution, SetResolution, EnvProbe::Resolution::Resolution128,
        "", PropertyInfo::Flag::Editor).SetEnumString("16;32;64;128;256;512;1024;2048");
    REGISTER_ACCESSOR_PROPERTY("hdr", "HDR", bool, IsHDR, SetHDR, true,
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("cullingMask", "Culling Mask", int, GetLayerMask, SetLayerMask, -1,
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("clear", "Clear", EnvProbe::ClearMethod::Enum, GetClearMethod, SetClearMethod, EnvProbe::ClearMethod::Sky,
        "", PropertyInfo::Flag::Editor).SetEnumString("Color;Skybox");
    REGISTER_MIXED_ACCESSOR_PROPERTY("clearColor", "Clear Color", Color3, GetClearColor, SetClearColor, Color3(0.18, 0.30, 0.47),
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("near", "Near", float, GetClippingNear, SetClippingNear, 0.1,
        "Near clipping plane distance", PropertyInfo::Flag::Editor).SetRange(0.01, 10000, 0.02);
    REGISTER_ACCESSOR_PROPERTY("far", "Far", float, GetClippingFar, SetClippingFar, 500,
        "Far clipping plane distance", PropertyInfo::Flag::Editor).SetRange(0.01, 10000, 0.02);
    REGISTER_ACCESSOR_PROPERTY("boxProjection", "Box Projection", bool, IsBoxProjection, SetBoxProjection, false,
        "", PropertyInfo::Flag::Editor);
    REGISTER_MIXED_ACCESSOR_PROPERTY("boxOffset", "Box Offset", Vec3, GetBoxOffset, SetBoxOffset, Vec3(0, 0, 0),
        "The center of the box in which the reflections will be applied to objects", PropertyInfo::Flag::Editor);
    REGISTER_MIXED_ACCESSOR_PROPERTY("boxExtent", "Box Extent", Vec3, GetBoxExtent, SetBoxExtent, Vec3(10, 10, 10),
        "The size of the box in which the reflections will be applied to objects", PropertyInfo::Flag::Editor).SetRange(0, 1e8, 0.05);
    REGISTER_ACCESSOR_PROPERTY("blendDistance", "Blend Distance", float, GetBlendDistance, SetBlendDistance, 1.0f,
        "", PropertyInfo::Flag::Editor).SetRange(0.0f, 100.0f, 0.01f);
    REGISTER_MIXED_ACCESSOR_PROPERTY("bakedDiffuseProbeTexture", "Baked Diffuse Probe", Guid, GetBakedDiffuseProbeTextureGuid, SetBakedDiffuseProbeTextureGuid, Guid::zero,
        "", PropertyInfo::Flag::NonCopying).SetMetaObject(&TextureCubeMapResource::metaObject);
    REGISTER_MIXED_ACCESSOR_PROPERTY("bakedSpecularProbeTexture", "Baked Specular Probe", Guid, GetBakedSpecularProbeTextureGuid, SetBakedSpecularProbeTextureGuid, Guid::zero,
        "", PropertyInfo::Flag::NonCopying).SetMetaObject(&TextureCubeMapResource::metaObject);
}

ComEnvironmentProbe::ComEnvironmentProbe() {
}

ComEnvironmentProbe::~ComEnvironmentProbe() {
    Purge(false);
}

void ComEnvironmentProbe::Purge(bool chainPurge) {
#if WITH_EDITOR
    if (sphereDef.mesh) {
        meshManager.ReleaseMesh(sphereDef.mesh);
        sphereDef.mesh = nullptr;
    }

    if (sphereMesh) {
        meshManager.ReleaseMesh(sphereMesh);
        sphereMesh = nullptr;
    }

    for (int i = 0; i < sphereDef.materials.Count(); i++) {
        materialManager.ReleaseMaterial(sphereDef.materials[i], true);
    }
    sphereDef.materials.Clear();

    if (sphereHandle != -1) {
        renderWorld->RemoveRenderObject(sphereHandle);
        sphereHandle = -1;
    }
#endif

    if (probeDef.bakedDiffuseProbeTexture) {
        textureManager.ReleaseTexture(probeDef.bakedDiffuseProbeTexture);
        probeDef.bakedDiffuseProbeTexture = nullptr;
    }

    if (probeDef.bakedSpecularProbeTexture) {
        textureManager.ReleaseTexture(probeDef.bakedSpecularProbeTexture);
        probeDef.bakedSpecularProbeTexture = nullptr;
    }

    if (probeHandle != -1) {
        renderWorld->RemoveEnvProbe(probeHandle);
        probeHandle = -1;
    }

    if (chainPurge) {
        Component::Purge();
    }
}

void ComEnvironmentProbe::Init() {
    Component::Init();

    renderWorld = GetGameWorld()->GetRenderWorld();

    ComTransform *transform = GetEntity()->GetTransform();

    probeDef.guid = GetGuid();
    probeDef.origin = transform->GetOrigin();
    probeDef.bounces = 0;

    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComEnvironmentProbe::TransformUpdated, SignalObject::ConnectionType::Unique);

#if WITH_EDITOR
    sphereDef.layer = TagLayerSettings::BuiltInLayer::Editor;
    sphereDef.maxVisDist = MeterToUnit(50.0f);

    sphereMesh = meshManager.GetMesh("_defaultSphereMesh");

    sphereDef.mesh = sphereMesh->InstantiateMesh(Mesh::Type::Static);
    sphereDef.aabb = sphereMesh->GetAABB();
    sphereDef.worldMatrix.SetTRS(transform->GetOrigin(), Mat3::identity, Vec3(0.75f, 0.75f, 0.75f));
    sphereDef.materialParms[RenderObject::MaterialParm::Red] = 1.0f;
    sphereDef.materialParms[RenderObject::MaterialParm::Green] = 1.0f;
    sphereDef.materialParms[RenderObject::MaterialParm::Blue] = 1.0f;
    sphereDef.materialParms[RenderObject::MaterialParm::Alpha] = 1.0f;
    sphereDef.materialParms[RenderObject::MaterialParm::TimeOffset] = 0.0f;
    sphereDef.materialParms[RenderObject::MaterialParm::TimeScale] = 1.0f;
#endif

    // Mark as initialized
    SetInitialized(true);

    UpdateVisuals();
}

void ComEnvironmentProbe::OnActive() {
    UpdateVisuals();
}

void ComEnvironmentProbe::OnInactive() {
    if (probeHandle != -1) {
        renderWorld->RemoveEnvProbe(probeHandle);
        probeHandle = -1;
    }

#if WITH_EDITOR
    if (sphereHandle != -1) {
        renderWorld->RemoveRenderObject(sphereHandle);
        sphereHandle = -1;
    }
#endif
}

bool ComEnvironmentProbe::HasRenderObject(int renderObjectHandle) const {
#if WITH_EDITOR
    if (this->sphereHandle == renderObjectHandle) {
        return true;
    }
#endif
    return false;
}

void ComEnvironmentProbe::Awake() {
    if (!IsActiveInHierarchy()) {
        return;
    }

    if (probeDef.type == EnvProbe::Type::Realtime) {
        if (probeDef.refreshMode == EnvProbe::RefreshMode::OnAwake) {
            renderSystem.ForceToRefreshEnvProbe(renderWorld, probeHandle);
        }
    }
}

void ComEnvironmentProbe::Update() {
    if (!IsActiveInHierarchy()) {
        return;
    }

    if (ShouldUpdateEveryFrame()) {
        renderSystem.ScheduleToRefreshEnvProbe(renderWorld, probeHandle);
    }
}

#if WITH_EDITOR
void ComEnvironmentProbe::DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    if (selected) {
        AABB aabb = AABB(-probeDef.boxExtent, probeDef.boxExtent);
        aabb += probeDef.origin + probeDef.boxOffset;
        
        renderWorld->SetDebugColor(Color4(0.0f, 0.5f, 1.0f, 1.0f), Color4(0.0f, 0.5f, 1.0f, 0.1f));
        renderWorld->DebugAABB(aabb, 1.0f, true, true);

        aabb.ExpandSelf(probeDef.blendDistance);
        renderWorld->SetDebugColor(Color4(1.0f, 0.5f, 0.0f, 1.0f), Color4(1.0f, 0.5f, 0.0f, 0.1f));
        renderWorld->DebugAABB(aabb, 1.0f, true, true);

#if 0
        gizmoCurrentTime = PlatformTime::Milliseconds();

        if (gizmoCurrentTime > gizmoRefreshTime + 3000) {
            gizmoRefreshTime = gizmoCurrentTime;

            if (probeDef.type == EnvProbe::Type::Realtime) {
                renderSystem.ScheduleToRefreshEnvProbe(renderWorld, probeHandle);
            }
        }
#endif
    }
}
#endif

const AABB ComEnvironmentProbe::GetAABB() const {
    return Sphere(Vec3::origin, MeterToUnit(0.5f)).ToAABB();
}

void ComEnvironmentProbe::UpdateVisuals() {
    if (!IsInitialized() || !IsActiveInHierarchy()) {
        return;
    }

#if WITH_EDITOR
    if (sphereDef.materials.Count() > 0) {
        materialManager.ReleaseMaterial(sphereDef.materials[0], true);
    }
#endif

    if (probeHandle == -1) {
        probeHandle = renderWorld->AddEnvProbe(&probeDef);
    } else {
        renderWorld->UpdateEnvProbe(probeHandle, &probeDef);
    }

    const EnvProbe *envProbe = renderWorld->GetEnvProbe(probeHandle);
    const Texture *specularProbeTexture = envProbe->GetSpecularProbeTexture();

#if WITH_EDITOR
    sphereDef.materials.SetCount(1);
    sphereDef.materials[0] = materialManager.GetSingleTextureMaterial(specularProbeTexture, Material::TextureHint::EnvCubeMap);

    if (sphereHandle == -1) {
        sphereHandle = renderWorld->AddRenderObject(&sphereDef);
    } else {
        renderWorld->UpdateRenderObject(sphereHandle, &sphereDef);
    }
#endif
}

void ComEnvironmentProbe::TransformUpdated(const ComTransform *transform) {
    probeDef.origin = transform->GetOrigin();

#if WITH_EDITOR
    sphereDef.worldMatrix.SetTranslation(transform->GetOrigin());
#endif

    UpdateVisuals();
}

void ComEnvironmentProbe::SetType(EnvProbe::Type::Enum type) {
    probeDef.type = type;

    updatable = ShouldUpdateEveryFrame();

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

void ComEnvironmentProbe::SetRefreshMode(EnvProbe::RefreshMode::Enum refreshMode) {
    probeDef.refreshMode = refreshMode;

    updatable = ShouldUpdateEveryFrame();

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

void ComEnvironmentProbe::SetTimeSlicing(EnvProbe::TimeSlicing::Enum timeSlicing) {
    probeDef.timeSlicing = timeSlicing;

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

void ComEnvironmentProbe::SetImportance(int importance) {
    probeDef.importance = Max(importance, 0);

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

void ComEnvironmentProbe::SetResolution(EnvProbe::Resolution::Enum resolution) {
    probeDef.resolution = resolution;

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

void ComEnvironmentProbe::SetHDR(bool useHDR) {
    probeDef.useHDR = useHDR;

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

void ComEnvironmentProbe::SetLayerMask(int layerMask) {
    probeDef.layerMask = layerMask;

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

void ComEnvironmentProbe::SetClearMethod(EnvProbe::ClearMethod::Enum clearMethod) {
    probeDef.clearMethod = clearMethod;

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

void ComEnvironmentProbe::SetClearColor(const Color3 &clearColor) {
    probeDef.clearColor = clearColor;

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

void ComEnvironmentProbe::SetClippingNear(float clippingNear) {
    probeDef.clippingNear = Max(clippingNear, 0.01f);

    if (probeDef.clippingNear > probeDef.clippingFar) {
        SetProperty("far", probeDef.clippingNear);
    }

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

void ComEnvironmentProbe::SetClippingFar(float clippingFar) {
    if (clippingFar >= probeDef.clippingNear) {
        probeDef.clippingFar = clippingFar;
    }

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

void ComEnvironmentProbe::SetBoxProjection(bool useBoxProjection) {
    probeDef.useBoxProjection = useBoxProjection;

    if (IsInitialized()) {
        UpdateVisuals();
    }
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

    if (IsInitialized()) {
        UpdateVisuals();
    }
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

    if (IsInitialized()) {
        UpdateVisuals();
    }
}

void ComEnvironmentProbe::SetBlendDistance(float blendDistance) {
    probeDef.blendDistance = blendDistance;

    if (IsInitialized()) {
        UpdateVisuals();
    }
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
        probeDef.bakedDiffuseProbeTexture = nullptr;
    }

    if (!textureGuid.IsZero()) {
        const Str texturePath = resourceGuidMapper.Get(textureGuid);

        probeDef.bakedDiffuseProbeTexture = textureManager.GetTexture(texturePath);

        if (probeDef.bakedDiffuseProbeTexture->IsDefaultTexture()) {
            probeDef.bakedDiffuseProbeTexture = nullptr;
        }
    }

    if (IsInitialized()) {
        UpdateVisuals();
    }
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
        probeDef.bakedSpecularProbeTexture = nullptr;
    }

    if (!textureGuid.IsZero()) {
        const Str texturePath = resourceGuidMapper.Get(textureGuid);

        probeDef.bakedSpecularProbeTexture = textureManager.GetTexture(texturePath);

        if (probeDef.bakedSpecularProbeTexture->IsDefaultTexture()) {
            probeDef.bakedSpecularProbeTexture = nullptr;
        }
    }

    if (IsInitialized()) {
        UpdateVisuals();
    }
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
    const Texture *diffuseProbeTexture = GetDiffuseProbeTexture();

    Image diffuseProbeImage;
    Texture::GetCubeImageFromCubeTexture(diffuseProbeTexture, 1, diffuseProbeImage);

    Str diffuseProbeFilename = probesDir;
    diffuseProbeFilename.AppendPath(va("DiffuseProbe-%s.dds", GetGuid().ToString()));
    diffuseProbeImage.WriteDDS(diffuseProbeFilename);

    return diffuseProbeFilename;
}

Str ComEnvironmentProbe::WriteSpecularProbeTexture(const Str &probesDir) const {
    const Texture *specularProbeTexture = GetSpecularProbeTexture();

    Image specularProbeImage;
    int numMipLevels = Math::Log(2, specularProbeTexture->GetWidth()) + 1;
    Texture::GetCubeImageFromCubeTexture(specularProbeTexture, numMipLevels, specularProbeImage);

    Str specularProbeFilename = probesDir;
    specularProbeFilename.AppendPath(va("SpecularProbe-%s.dds", GetGuid().ToString()));
    specularProbeImage.WriteDDS(specularProbeFilename);

    return specularProbeFilename;
}

bool ComEnvironmentProbe::ShouldUpdateEveryFrame() const {
    if (probeDef.type == EnvProbe::Type::Realtime) {
        if (probeDef.refreshMode == EnvProbe::RefreshMode::EveryFrame) {
            return true;
        }
    }
    return false;
}

BE_NAMESPACE_END
