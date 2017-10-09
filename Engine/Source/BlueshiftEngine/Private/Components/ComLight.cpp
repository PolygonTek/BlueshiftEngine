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
#include "Game/Entity.h"
#include "Game/GameWorld.h"
#include "Game/GameSettings/TagLayerSettings.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Light", ComLight, Component)
BEGIN_EVENTS(ComLight)
END_EVENTS
BEGIN_PROPERTIES(ComLight)
    // NOTE: lightType_t enum 과 같은 순서여야 함
    PROPERTY_ENUM("lightType", "Light Type", "", "Point;Spot;Directional", "0", PropertySpec::ReadWrite),
    PROPERTY_OBJECT("material", "Material", "", GuidMapper::zeroClampLightMaterialGuid.ToString(), MaterialAsset::metaObject, PropertySpec::ReadWrite),
    PROPERTY_COLOR3("color", "Color", "", "1 1 1", PropertySpec::ReadWrite),
    PROPERTY_BOOL("turnOn", "Turn On", "", "true", PropertySpec::ReadWrite),
    PROPERTY_BOOL("castShadows", "Cast Shadows", "", "false", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("shadowOffsetFactor", "Shadow Offset Factor", "scale value for shadow map drawing", Rangef(0, 16, 0.01f), "3", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("shadowOffsetUnits", "Shadow Offset Unit", "bias value added to depth test for shadow map drawing", Rangef(0, 1000, 1), "200", PropertySpec::ReadWrite),
    PROPERTY_BOOL("primaryLight", "Is Primary Light", "", "false", PropertySpec::ReadWrite),
    PROPERTY_VEC3("lightSize", "Size", "", "200 200 200", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("fallOffExponent", "Fall Off Exponent", "", Rangef(0.01f, 100, 0.1f), "1.25", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("intensity", "Intensity", "", Rangef(0, 8, 0.01f), "2.0", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("lightZNear", "Near", "", Rangef(1, 200, 0.01f), "10", PropertySpec::ReadWrite),
    PROPERTY_FLOAT("timeOffset", "Time Offset", "", "0", PropertySpec::ReadWrite),
    PROPERTY_FLOAT("timeScale", "Time Scale", "", "1", PropertySpec::ReadWrite),
    PROPERTY_FLOAT("maxVisDist", "Max Visible Distance", "max visible distance from viewer", "16384", PropertySpec::ReadWrite),
END_PROPERTIES

void ComLight::RegisterProperties() {
#ifdef NEW_PROPERTY_SYSTEM
    REGISTER_ENUM_ACCESSOR_PROPERTY("Light Type", "Point;Spot;Directional", GetLightType, SetLightType, 0, "", PropertySpec::ReadWrite);
    REGISTER_MIXED_ACCESSOR_PROPERTY("Material", ObjectRef, GetMaterialRef, SetMaterialRef, ObjectRef(MaterialAsset::metaObject, GuidMapper::zeroClampLightMaterialGuid), "", PropertySpec::ReadWrite);
    REGISTER_MIXED_ACCESSOR_PROPERTY("Color", Color3, GetColor, SetColor, Color3::white, "", PropertySpec::ReadWrite);
    REGISTER_ACCESSOR_PROPERTY("Turn On", bool, IsTurnOn, SetTurnOn, true, "", PropertySpec::ReadWrite);
    REGISTER_ACCESSOR_PROPERTY("Cast Shadows", bool, IsCastShadows, SetCastShadows, false, "", PropertySpec::ReadWrite);
    REGISTER_ACCESSOR_PROPERTY("Shadow Offset Factor", float, GetShadowOffsetFactor, SetShadowOffsetFactor, 3.f, "", PropertySpec::ReadWrite).SetRange(0, 16, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("Shadow Offset Units", float, GetShadowOffsetUnits, SetShadowOffsetUnits, 200.f, "", PropertySpec::ReadWrite).SetRange(0, 1000, 1);
    REGISTER_ACCESSOR_PROPERTY("Is Main Light", bool, IsPrimaryLight, SetPrimaryLight, false, "", PropertySpec::ReadWrite);
    REGISTER_MIXED_ACCESSOR_PROPERTY("Light Size", Vec3, GetLightSize, SetLightSize, Vec3(200, 200, 200), "", PropertySpec::ReadWrite);
    REGISTER_ACCESSOR_PROPERTY("Fall Off Exponent", float, GetFallOffExponent, SetFallOffExponent, 1.25f, "", PropertySpec::ReadWrite).SetRange(0.01f, 100, 0.1f);
    REGISTER_ACCESSOR_PROPERTY("Intensity", float, GetIntensity, SetIntensity, 2.f, "", PropertySpec::ReadWrite).SetRange(0, 8, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("Near", float, GetLightZNear, SetLightZNear, 10.f, "", PropertySpec::ReadWrite).SetRange(1, 200, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("Time Offset", float, GetTimeOffset, SetTimeOffset, 0.f, "", PropertySpec::ReadWrite);
    REGISTER_ACCESSOR_PROPERTY("Time Scale", float, GetTimeScale, SetTimeScale, 1.f, "", PropertySpec::ReadWrite);
    REGISTER_ACCESSOR_PROPERTY("Max Visible Distance", float, GetMaxVisDist, SetMaxVisDist, 16384.f, "", PropertySpec::ReadWrite);
#endif
}

ComLight::ComLight() {
    sceneLightHandle = -1;
    memset(&sceneLight, 0, sizeof(sceneLight));

    spriteHandle = -1;
    spriteMesh = nullptr;
    memset(&sprite, 0, sizeof(sprite));

#ifndef NEW_PROPERTY_SYSTEM
    Connect(&Properties::SIG_PropertyChanged, this, (SignalCallback)&ComLight::PropertyChanged);
#endif
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
    Purge();

    Component::Init();

    renderWorld = GetGameWorld()->GetRenderWorld();

    memset(&sceneLight, 0, sizeof(sceneLight));

    sceneLight.layer = GetEntity()->GetLayer();
    sceneLight.isStaticLight = true; //
    sceneLight.type = props->Get("lightType").As<SceneLight::Type>();
    
    // radius for omni light
    sceneLight.value = props->Get("lightSize").As<Vec3>();

    const Guid materialGuid = props->Get("material").As<Guid>();
    const Str materialPath = resourceGuidMapper.Get(materialGuid);
    sceneLight.material = materialManager.GetMaterial(materialPath);

    sceneLight.isPrimaryLight = props->Get("primaryLight").As<bool>();
    sceneLight.zNear = props->Get("lightZNear").As<float>();
    sceneLight.fallOffExponent = props->Get("fallOffExponent").As<float>();

    Color3 color = props->Get("color").As<Color3>();
    sceneLight.materialParms[SceneEntity::RedParm] = color.r;
    sceneLight.materialParms[SceneEntity::GreenParm] = color.g;
    sceneLight.materialParms[SceneEntity::BlueParm] = color.b;
    sceneLight.materialParms[SceneEntity::AlphaParm] = 1.0f;
    sceneLight.materialParms[SceneEntity::TimeOffsetParm] = props->Get("timeOffset").As<float>();
    sceneLight.materialParms[SceneEntity::TimeScaleParm] = props->Get("timeScale").As<float>();

    sceneLight.intensity = props->Get("intensity").As<float>();

    sceneLight.maxVisDist = props->Get("maxVisDist").As<float>();

    sceneLight.turnOn = props->Get("turnOn").As<bool>();

    sceneLight.castShadows = props->Get("castShadows").As<bool>();
    sceneLight.shadowOffsetFactor = props->Get("shadowOffsetFactor").As<float>();
    sceneLight.shadowOffsetUnits = props->Get("shadowOffsetUnits").As<float>();

    ComTransform *transform = GetEntity()->GetTransform();
    sceneLight.origin = transform->GetOrigin();
    sceneLight.axis = transform->GetAxis();

    // 3d sprite
    spriteMesh = meshManager.GetMesh("_defaultQuadMesh");

    memset(&sprite, 0, sizeof(sprite));
    sprite.layer = TagLayerSettings::EditorLayer;
    sprite.maxVisDist = MeterToUnit(50);
    sprite.billboard = true;

    Texture *spriteTexture = textureManager.GetTexture(LightSpriteTexturePath(sceneLight.type), Texture::Clamp | Texture::HighQuality);
    sprite.customMaterials.SetCount(1);
    sprite.customMaterials[0] = materialManager.GetSingleTextureMaterial(spriteTexture, Material::SpriteHint);
    textureManager.ReleaseTexture(spriteTexture);
    
    sprite.mesh = spriteMesh->InstantiateMesh(Mesh::StaticMesh);
    sprite.aabb = spriteMesh->GetAABB();
    sprite.origin = transform->GetOrigin();
    sprite.scale = Vec3(1, 1, 1);
    sprite.axis = Mat3::identity;
    sprite.materialParms[SceneEntity::RedParm] = color.r;
    sprite.materialParms[SceneEntity::GreenParm] = color.g;
    sprite.materialParms[SceneEntity::BlueParm] = color.b;
    sprite.materialParms[SceneEntity::AlphaParm] = 1.0f;	
    sprite.materialParms[SceneEntity::TimeOffsetParm] = props->Get("timeOffset").As<float>();
    sprite.materialParms[SceneEntity::TimeScaleParm] = props->Get("timeScale").As<float>();

    GetEntity()->Connect(&Entity::SIG_LayerChanged, this, (SignalCallback)&ComLight::LayerChanged, SignalObject::Unique);

    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComLight::TransformUpdated, SignalObject::Unique);
    
    UpdateVisuals();
}

void ComLight::SetEnable(bool enable) {
    if (enable) {
        if (!IsEnabled()) {
            UpdateVisuals();
            Component::SetEnable(true);
        }
    } else {
        if (IsEnabled()) {
            renderWorld->RemoveLight(sceneLightHandle);
            sceneLightHandle = -1;
            renderWorld->RemoveEntity(spriteHandle);
            spriteHandle = -1;
            Component::SetEnable(false);
        }
    }
}

bool ComLight::HasRenderEntity(int renderEntityHandle) const { 
    if (this->spriteHandle == renderEntityHandle) {
        return true;
    }

    return false;
}

void ComLight::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    const Color4 lightColor = *(Color4 *)&sceneLight.materialParms[SceneEntity::RedParm];

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

    sprite.customMaterials[0]->GetPass()->constantColor[3] = alpha;
}

const AABB ComLight::GetAABB() {
    return Sphere(Vec3::origin, MeterToUnit(0.5)).ToAABB();
}

bool ComLight::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    return false;
}

void ComLight::UpdateVisuals() {
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
    sceneLight.layer = entity->GetProperties()->Get("layer").As<int>();

    UpdateVisuals();
}

void ComLight::TransformUpdated(const ComTransform *transform) {
    sceneLight.origin = transform->GetOrigin();
    sceneLight.axis = transform->GetAxis();
    
    sprite.origin = sceneLight.origin;
    
    UpdateVisuals();
}

void ComLight::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
        return;
    }

    if (!Str::Cmp(propName, "lightType")) {
        SetLightType(props->Get("lightType").As<SceneLight::Type>());
        return;
    }

    if (!Str::Cmp(propName, "primaryLight")) {
        SetPrimaryLight(props->Get("primaryLight").As<bool>());
        return;
    }

    if (!Str::Cmp(propName, "turnOn")) {
        SetTurnOn(props->Get("turnOn").As<bool>());
        return;
    }

    if (!Str::Cmp(propName, "lightSize")) {
        SetLightSize(props->Get("lightSize").As<Vec3>());
        return;
    }

    if (!Str::Cmp(propName, "lightZNear")) {
        SetLightZNear(props->Get("lightZNear").As<float>());        
        return;
    } 

    if (!Str::Cmp(propName, "maxVisDist")) {
        SetMaxVisDist(props->Get("maxVisDist").As<float>());        
        return;
    } 

    if (!Str::Cmp(propName, "material")) {
        SetMaterial(props->Get("material").As<Guid>());        
        return;
    }

    if (!Str::Cmp(propName, "color")) {
        SetColor(props->Get("color").As<Color3>());        
        return;
    } 

    if (!Str::Cmp(propName, "timeOffset")) {
        SetTimeOffset(props->Get("timeOffset").As<float>());        
        return;
    } 
    
    if (!Str::Cmp(propName, "timeScale")) {
        SetTimeOffset(props->Get("timeScale").As<float>());       
        return;
    } 

    if (!Str::Cmp(propName, "castShadows")) {
        SetCastShadows(props->Get("castShadows").As<bool>());        
        return;
    }

    if (!Str::Cmp(propName, "shadowOffsetFactor")) {
        SetShadowOffsetFactor(props->Get("shadowOffsetFactor").As<float>());        
        return;
    } 

    if (!Str::Cmp(propName, "shadowOffsetUnits")) {
        SetShadowOffsetUnits(props->Get("shadowOffsetUnits").As<float>());
        return;
    } 

    if (!Str::Cmp(propName, "fallOffExponent")) {
        SetFallOffExponent(props->Get("fallOffExponent").As<float>());
        return;
    } 

    if (!Str::Cmp(propName, "intensity")) {
        SetIntensity(props->Get("intensity").As<float>());
        return;
    } 

    if (!Str::Cmp(propName, "radius")) {
        SetRadius(props->Get("radius").As<Vec3>());
        return;
    }

    Component::PropertyChanged(classname, propName);
}

int ComLight::GetLightType() const {
    return (int)sceneLight.type;
}

void ComLight::SetLightType(int type) {
    sceneLight.type = (SceneLight::Type)type;
    materialManager.ReleaseMaterial(sprite.customMaterials[0]);
    Texture *spriteTexture = textureManager.GetTexture(LightSpriteTexturePath(sceneLight.type), Texture::Clamp | Texture::HighQuality);
    sprite.customMaterials[0] = materialManager.GetSingleTextureMaterial(spriteTexture, Material::SpriteHint);
    textureManager.ReleaseTexture(spriteTexture);
    UpdateVisuals();
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

Guid ComLight::GetMaterial() const {
    const Str materialPath = sceneLight.material->GetHashName();
    return resourceGuidMapper.Get(materialPath);
}

void ComLight::SetMaterial(const Guid &materialGuid) {
    materialManager.ReleaseMaterial(sceneLight.material);

    const Str materialPath = resourceGuidMapper.Get(materialGuid);
    sceneLight.material = materialManager.GetMaterial(materialPath);
    UpdateVisuals();
}

ObjectRef ComLight::GetMaterialRef() const {
    const Str materialPath = sceneLight.material->GetHashName();
    return ObjectRef(MaterialAsset::metaObject, resourceGuidMapper.Get(materialPath));
}

void ComLight::SetMaterialRef(const ObjectRef &objectRef) {
    materialManager.ReleaseMaterial(sceneLight.material);

    const Str materialPath = resourceGuidMapper.Get(objectRef.objectGuid);
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
    sceneLight.value = props->Get("radius").As<Vec3>();
    UpdateVisuals();
}

BE_NAMESPACE_END
