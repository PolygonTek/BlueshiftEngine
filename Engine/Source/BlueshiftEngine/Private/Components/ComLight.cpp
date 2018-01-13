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
#include "Components/ComRigidBody.h"
#include "Components/ComLight.h"
#include "Game/GameWorld.h"
#include "Game/TagLayerSettings.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Light", ComLight, Component)
BEGIN_EVENTS(ComLight)
END_EVENTS

void ComLight::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("lightType", "Light Type", SceneLight::Type, GetLightType, SetLightType, 0, "", PropertyInfo::EditorFlag)
        .SetEnumString("Point;Spot;Directional");
    REGISTER_MIXED_ACCESSOR_PROPERTY("material", "Material", Guid, GetMaterialGuid, SetMaterialGuid, GuidMapper::zeroClampLightMaterialGuid, "", PropertyInfo::EditorFlag)
        .SetMetaObject(&MaterialAsset::metaObject);
    REGISTER_MIXED_ACCESSOR_PROPERTY("color", "Color", Color3, GetColor, SetColor, Color3::white, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("turnOn", "Turn On", bool, IsTurnOn, SetTurnOn, true, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("castShadows", "Cast Shadows", bool, IsCastShadows, SetCastShadows, false, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("shadowOffsetFactor", "Shadow Offset Factor", float, GetShadowOffsetFactor, SetShadowOffsetFactor, 3.f, "", PropertyInfo::EditorFlag)
        .SetRange(0, 16, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("shadowOffsetUnits", "Shadow Offset Units", float, GetShadowOffsetUnits, SetShadowOffsetUnits, 200.f, "", PropertyInfo::EditorFlag)
        .SetRange(0, 1000, 1);
    REGISTER_ACCESSOR_PROPERTY("primaryLight", "Is Main Light", bool, IsPrimaryLight, SetPrimaryLight, false, "", PropertyInfo::EditorFlag);
    REGISTER_MIXED_ACCESSOR_PROPERTY("lightSize", "Light Size", Vec3, GetLightSize, SetLightSize, Vec3(200, 200, 200), "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("fallOffExponent", "Fall Off Exponent", float, GetFallOffExponent, SetFallOffExponent, 1.25f, "", PropertyInfo::EditorFlag)
        .SetRange(0.01f, 100, 0.1f);
    REGISTER_ACCESSOR_PROPERTY("intensity", "Intensity", float, GetIntensity, SetIntensity, 2.f, "", PropertyInfo::EditorFlag)
        .SetRange(0, 8, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("lightZNear", "Near", float, GetLightZNear, SetLightZNear, 10.f, "", PropertyInfo::EditorFlag)
        .SetRange(1, 200, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("timerOffset", "Time Offset", float, GetTimeOffset, SetTimeOffset, 0.f, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("timeScale", "Time Scale", float, GetTimeScale, SetTimeScale, 1.f, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("maxVisDist", "Max Visible Distance", float, GetMaxVisDist, SetMaxVisDist, 16384.f, "", PropertyInfo::EditorFlag);
}

ComLight::ComLight() {
    sceneLightHandle = -1;
    memset(&sceneLight, 0, sizeof(sceneLight));

    spriteHandle = -1;
    spriteMesh = nullptr;
    memset(&sprite, 0, sizeof(sprite));
}

ComLight::~ComLight() {
    Purge(false);
}

void ComLight::Purge(bool chainPurge) {
    if (sceneLight.material) {
        materialManager.ReleaseMaterial(sceneLight.material);
        sceneLight.material = nullptr;
    }

    if (sceneLightHandle != -1) {
        renderWorld->RemoveLight(sceneLightHandle);
        sceneLightHandle = -1;
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

static const char *LightSpriteTexturePath(SceneLight::Type lightType) {
    switch (lightType) {
    case SceneLight::PointLight:
        return "Data/EditorUI/OmniLight.png"; 
    case SceneLight::SpotLight:
        return "Data/EditorUI/ProjectedLight.png"; 
    case SceneLight::DirectionalLight: 
        return "Data/EditorUI/DirectionalLight.png";
    default:
        assert(0);
        return "";
    }
}

void ComLight::Init() {
    Component::Init();

    renderWorld = GetGameWorld()->GetRenderWorld();

    sceneLight.layer = GetEntity()->GetLayer();

    ComTransform *transform = GetEntity()->GetTransform();
    sceneLight.origin = transform->GetOrigin();
    sceneLight.axis = transform->GetAxis();

    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComLight::TransformUpdated, SignalObject::Unique);

    // 3d sprite for editor
    spriteMesh = meshManager.GetMesh("_defaultQuadMesh");

    memset(&sprite, 0, sizeof(sprite));
    sprite.layer = TagLayerSettings::EditorLayer;
    sprite.maxVisDist = MeterToUnit(50);
    sprite.billboard = true;

    Texture *spriteTexture = textureManager.GetTexture(LightSpriteTexturePath(sceneLight.type), Texture::Clamp | Texture::HighQuality);
    sprite.materials.SetCount(1);
    sprite.materials[0] = materialManager.GetSingleTextureMaterial(spriteTexture, Material::SpriteHint);
    textureManager.ReleaseTexture(spriteTexture);
    
    sprite.mesh = spriteMesh->InstantiateMesh(Mesh::StaticMesh);
    sprite.aabb = spriteMesh->GetAABB();
    sprite.origin = transform->GetOrigin();
    sprite.scale = Vec3(1, 1, 1);
    sprite.axis = Mat3::identity;
    sprite.materialParms[SceneEntity::RedParm] = sceneLight.materialParms[SceneEntity::RedParm];
    sprite.materialParms[SceneEntity::GreenParm] = sceneLight.materialParms[SceneEntity::GreenParm];
    sprite.materialParms[SceneEntity::BlueParm] = sceneLight.materialParms[SceneEntity::BlueParm];
    sprite.materialParms[SceneEntity::AlphaParm] = 1.0f;
    sprite.materialParms[SceneEntity::TimeOffsetParm] = sceneLight.materialParms[SceneEntity::TimeOffsetParm];
    sprite.materialParms[SceneEntity::TimeScaleParm] = sceneLight.materialParms[SceneEntity::TimeScaleParm];
    //

    GetEntity()->Connect(&Entity::SIG_LayerChanged, this, (SignalCallback)&ComLight::LayerChanged, SignalObject::Unique);

    // Mark as initialized
    SetInitialized(true);

    UpdateVisuals();
}

void ComLight::OnActive() {
    UpdateVisuals();
}

void ComLight::OnInactive() {
    if (sceneLightHandle != -1) {
        renderWorld->RemoveLight(sceneLightHandle);
        sceneLightHandle = -1;
    }

    if (spriteHandle != -1) {
        renderWorld->RemoveEntity(spriteHandle);
        spriteHandle = -1;
    }
}

bool ComLight::HasRenderEntity(int renderEntityHandle) const { 
    if (spriteHandle == renderEntityHandle) {
        return true;
    }

    return false;
}

void ComLight::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    const Color4 lightColor = Color4(GetColor(), 1.0f);

    if (sceneLight.type == SceneLight::DirectionalLight || sceneLight.type == SceneLight::SpotLight) {
        if (selected) {
            renderWorld->SetDebugColor(Color4::white, Color4::zero);
        } else {
            renderWorld->SetDebugColor(lightColor, Color4::zero);
        }

        if (selected) {
            renderWorld->SetDebugColor(lightColor, Color4::zero);

            if (sceneLight.type == SceneLight::DirectionalLight) {
                OBB box;
                box.SetCenter(sceneLight.origin + sceneLight.axis[0] * sceneLight.value[0] * 0.5f);
                box.SetExtents(Vec3(sceneLight.value[0] * 0.5f, sceneLight.value[1], sceneLight.value[2]));
                box.SetAxis(sceneLight.axis);
                renderWorld->DebugOBB(box, 1.0f, true, true);
            } else {
                Frustum frustum;
                frustum.SetOrigin(sceneLight.origin);
                frustum.SetAxis(sceneLight.axis);
                frustum.SetSize(sceneLight.zNear, sceneLight.value[0], sceneLight.value[1], sceneLight.value[2]);
                renderWorld->DebugFrustum(frustum, false, 1.0f, true, true);
            }
        }
    } else if (sceneLight.type == SceneLight::PointLight) {
        if (selected) {
            renderWorld->SetDebugColor(Color4::white, Color4::zero);
        } else {
            renderWorld->SetDebugColor(lightColor, Color4::zero);
        }

        if (selected) {
            renderWorld->SetDebugColor(lightColor, Color4::zero);
            renderWorld->DebugEllipse(sceneLight.origin, sceneLight.axis[0], sceneLight.axis[1], sceneLight.value[0], sceneLight.value[1], 1, true, true);
            renderWorld->DebugEllipse(sceneLight.origin, sceneLight.axis[1], sceneLight.axis[2], sceneLight.value[1], sceneLight.value[2], 1, true, true);
            renderWorld->DebugEllipse(sceneLight.origin, sceneLight.axis[0], sceneLight.axis[2], sceneLight.value[0], sceneLight.value[2], 1, true, true);

            Color4 lightColor2 = lightColor;
            lightColor2.a = 0.2f;
            renderWorld->SetDebugColor(lightColor2, Color4::zero);
            renderWorld->DebugLine(sceneLight.origin - sceneLight.axis[0] * sceneLight.value[0] * 0.2f, sceneLight.origin - sceneLight.axis[0] * sceneLight.value[0], 1, true);
            renderWorld->DebugLine(sceneLight.origin + sceneLight.axis[0] * sceneLight.value[0] * 0.2f, sceneLight.origin + sceneLight.axis[0] * sceneLight.value[0], 1, true);	
            renderWorld->DebugLine(sceneLight.origin - sceneLight.axis[1] * sceneLight.value[1] * 0.2f, sceneLight.origin - sceneLight.axis[1] * sceneLight.value[1], 1, true);
            renderWorld->DebugLine(sceneLight.origin + sceneLight.axis[1] * sceneLight.value[1] * 0.2f, sceneLight.origin + sceneLight.axis[1] * sceneLight.value[1], 1, true);
            renderWorld->DebugLine(sceneLight.origin - sceneLight.axis[2] * sceneLight.value[2] * 0.2f, sceneLight.origin - sceneLight.axis[2] * sceneLight.value[2], 1, true);			
            renderWorld->DebugLine(sceneLight.origin + sceneLight.axis[2] * sceneLight.value[2] * 0.2f, sceneLight.origin + sceneLight.axis[2] * sceneLight.value[2], 1, true);
        }
    }

    // Fade icon alpha in near distance
    float alpha = BE1::Clamp(sprite.origin.Distance(sceneView.origin) / MeterToUnit(8), 0.01f, 1.0f);

    sprite.materials[0]->GetPass()->constantColor[3] = alpha;
}

const AABB ComLight::GetAABB() {
    return Sphere(Vec3::origin, MeterToUnit(0.5)).ToAABB();
}

bool ComLight::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    return false;
}

void ComLight::UpdateVisuals() {
    if (!IsInitialized() || !IsActiveInHierarchy()) {
        return;
    }

    if (sceneLightHandle == -1) {
        sceneLightHandle = renderWorld->AddLight(&sceneLight);
    } else {
        renderWorld->UpdateLight(sceneLightHandle, &sceneLight);
    }

    if (spriteHandle == -1) {
        spriteHandle = renderWorld->AddEntity(&sprite);
    } else {
        renderWorld->UpdateEntity(spriteHandle, &sprite);
    }
}

void ComLight::LayerChanged(const Entity *entity) {
    sceneLight.layer = entity->GetProperty("layer").As<int>();

    UpdateVisuals();
}

void ComLight::TransformUpdated(const ComTransform *transform) {
    sceneLight.origin = transform->GetOrigin();
    sceneLight.axis = transform->GetAxis();
    
    sprite.origin = sceneLight.origin;
    
    UpdateVisuals();
}

SceneLight::Type ComLight::GetLightType() const {
    return sceneLight.type;
}

void ComLight::SetLightType(SceneLight::Type type) {
    sceneLight.type = type;

    if (IsInitialized()) {
        materialManager.ReleaseMaterial(sprite.materials[0]);

        Texture *spriteTexture = textureManager.GetTexture(LightSpriteTexturePath(sceneLight.type), Texture::Clamp | Texture::HighQuality);
        sprite.materials[0] = materialManager.GetSingleTextureMaterial(spriteTexture, Material::SpriteHint);
        textureManager.ReleaseTexture(spriteTexture);

        UpdateVisuals();
    }
}

bool ComLight::IsPrimaryLight() const {
    return sceneLight.isPrimaryLight;
}

void ComLight::SetPrimaryLight(bool isPrimaryLight) {
    sceneLight.isPrimaryLight = isPrimaryLight;

    UpdateVisuals();
}

bool ComLight::IsTurnOn() const {
    return sceneLight.turnOn;
}

void ComLight::SetTurnOn(bool turnOn) {
    sceneLight.turnOn = turnOn;

    UpdateVisuals();
}

Vec3 ComLight::GetLightSize() const {
    return Vec3(sceneLight.value);
}

void ComLight::SetLightSize(const Vec3 &lightSize) {
    sceneLight.value = lightSize;
    sceneLight.value[0] = Max(sceneLight.value[0], 1.0f);
    sceneLight.value[1] = Max(sceneLight.value[1], 1.0f);
    sceneLight.value[2] = Max(sceneLight.value[2], 1.0f);

    UpdateVisuals();
}

float ComLight::GetLightZNear() const {
    return sceneLight.zNear;
}

void ComLight::SetLightZNear(float lightZNear) {
    sceneLight.zNear = lightZNear;
    
    UpdateVisuals();
}

float ComLight::GetMaxVisDist() const {
    return sceneLight.maxVisDist;
}

void ComLight::SetMaxVisDist(float maxVisDist) {
    sceneLight.maxVisDist = maxVisDist;

    UpdateVisuals();
}

Guid ComLight::GetMaterialGuid() const {
    if (sceneLight.material) {
        const Str materialPath = sceneLight.material->GetHashName();
        return resourceGuidMapper.Get(materialPath);
    }
    return Guid();
}

void ComLight::SetMaterialGuid(const Guid &materialGuid) {
    if (sceneLight.material) {
        materialManager.ReleaseMaterial(sceneLight.material);
    }

    const Str materialPath = resourceGuidMapper.Get(materialGuid);
    sceneLight.material = materialManager.GetMaterial(materialPath);

    UpdateVisuals();
}

Color3 ComLight::GetColor() const {
    return Color3(&sceneLight.materialParms[SceneEntity::RedParm]);
}

void ComLight::SetColor(const Color3 &color) {
    sceneLight.materialParms[SceneEntity::RedParm] = color.r;
    sceneLight.materialParms[SceneEntity::GreenParm] = color.g;
    sceneLight.materialParms[SceneEntity::BlueParm] = color.b;

    sprite.materialParms[SceneEntity::RedParm] = color.r;
    sprite.materialParms[SceneEntity::GreenParm] = color.g;
    sprite.materialParms[SceneEntity::BlueParm] = color.b;

    UpdateVisuals();
}

float ComLight::GetTimeOffset() const {
    return sceneLight.materialParms[SceneEntity::TimeOffsetParm];
}

void ComLight::SetTimeOffset(float timeOffset) {
    sceneLight.materialParms[SceneEntity::TimeOffsetParm] = timeOffset;

    UpdateVisuals();
}

float ComLight::GetTimeScale() const {
    return sceneLight.materialParms[SceneEntity::TimeScaleParm];
}

void ComLight::SetTimeScale(float timeScale) {
    sceneLight.materialParms[SceneEntity::TimeScaleParm] = timeScale;

    UpdateVisuals();
}

bool ComLight::IsCastShadows() const {
    return sceneLight.castShadows;
}

void ComLight::SetCastShadows(bool castShadows) {
    sceneLight.castShadows = castShadows;

    UpdateVisuals();
}

float ComLight::GetShadowOffsetFactor() const {
    return sceneLight.shadowOffsetFactor;
}

void ComLight::SetShadowOffsetFactor(float factor) {
    sceneLight.shadowOffsetFactor = factor;

    UpdateVisuals();
}

float ComLight::GetShadowOffsetUnits() const {
    return sceneLight.shadowOffsetUnits;
}

void ComLight::SetShadowOffsetUnits(float units) {
    sceneLight.shadowOffsetUnits = units;

    UpdateVisuals();
}

float ComLight::GetFallOffExponent() const {
    return sceneLight.fallOffExponent;
}

void ComLight::SetFallOffExponent(float fallOff) {
    sceneLight.fallOffExponent = fallOff;

    UpdateVisuals();
}

float ComLight::GetIntensity() const {
    return sceneLight.intensity;
}

void ComLight::SetIntensity(float intensity) {
    sceneLight.intensity = intensity;

    UpdateVisuals();
}

const Vec3 &ComLight::GetRadius() const {
    return sceneLight.value;
}

void ComLight::SetRadius(const Vec3 &radius) {
    sceneLight.value = GetProperty("radius").As<Vec3>();

    UpdateVisuals();
}

BE_NAMESPACE_END
