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
    REGISTER_ACCESSOR_PROPERTY("lightType", "Light Type", RenderLight::Type, GetLightType, SetLightType, 0, 
        "", PropertyInfo::EditorFlag).SetEnumString("Point;Spot;Directional");
    REGISTER_ACCESSOR_PROPERTY("primaryLight", "Is Main Light", bool, IsPrimaryLight, SetPrimaryLight, false,
        "", PropertyInfo::EditorFlag);
    REGISTER_MIXED_ACCESSOR_PROPERTY("material", "Material", Guid, GetMaterialGuid, SetMaterialGuid, GuidMapper::zeroClampLightMaterialGuid, 
        "", PropertyInfo::EditorFlag).SetMetaObject(&MaterialAsset::metaObject);
    REGISTER_MIXED_ACCESSOR_PROPERTY("color", "Color", Color3, GetColor, SetColor, Color3::white, 
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("intensity", "Intensity", float, GetIntensity, SetIntensity, 2.f,
        "", PropertyInfo::EditorFlag).SetRange(0, 100, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("fallOffExponent", "Fall Off Exponent", float, GetFallOffExponent, SetFallOffExponent, 1.25f,
        "", PropertyInfo::EditorFlag).SetRange(0.01f, 100, 0.1f);
    REGISTER_MIXED_ACCESSOR_PROPERTY("lightSize", "Light Size", Vec3, GetLightSize, SetLightSize, Vec3(MeterToUnit(3.0f)),
        "", PropertyInfo::SystemUnits | PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("lightZNear", "Near Distance", float, GetLightZNear, SetLightZNear, MeterToUnit(0.1f),
        "", PropertyInfo::SystemUnits | PropertyInfo::EditorFlag).SetRange(MeterToUnit(0.01f), 100, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("castShadows", "Shadows/Cast Shadows", bool, IsCastShadows, SetCastShadows, false,
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("shadowOffsetFactor", "Shadows/Offset Factor", float, GetShadowOffsetFactor, SetShadowOffsetFactor, 3.f,
        "", PropertyInfo::EditorFlag).SetRange(0, 16, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("shadowOffsetUnits", "Shadows/Offset Units", float, GetShadowOffsetUnits, SetShadowOffsetUnits, 200.f,
        "", PropertyInfo::EditorFlag).SetRange(0, 1000, 1);
    REGISTER_ACCESSOR_PROPERTY("timerOffset", "Time Offset", float, GetTimeOffset, SetTimeOffset, 0.f, 
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("timeScale", "Time Scale", float, GetTimeScale, SetTimeScale, 1.f, 
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("maxVisDist", "Max Visible Distance", float, GetMaxVisDist, SetMaxVisDist, 16384.f, 
        "", PropertyInfo::SystemUnits | PropertyInfo::EditorFlag);
}

ComLight::ComLight() {
    renderLightHandle = -1;
    memset(&renderLightDef, 0, sizeof(renderLightDef));

    spriteHandle = -1;
    spriteMesh = nullptr;
    memset(&spriteDef, 0, sizeof(spriteDef));
}

ComLight::~ComLight() {
    Purge(false);
}

void ComLight::Purge(bool chainPurge) {
    if (renderLightDef.material) {
        materialManager.ReleaseMaterial(renderLightDef.material);
        renderLightDef.material = nullptr;
    }

    if (renderLightHandle != -1) {
        renderWorld->RemoveRenderLight(renderLightHandle);
        renderLightHandle = -1;
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

    if (chainPurge) {
        Component::Purge();
    }
}

static const char *LightSpriteTexturePath(RenderLight::Type lightType) {
    switch (lightType) {
    case RenderLight::PointLight:
        return "Data/EditorUI/OmniLight.png";
    case RenderLight::SpotLight:
        return "Data/EditorUI/ProjectedLight.png";
    case RenderLight::DirectionalLight:
        return "Data/EditorUI/DirectionalLight.png";
    default:
        assert(0);
        return "";
    }
}

void ComLight::Init() {
    Component::Init();

    renderWorld = GetGameWorld()->GetRenderWorld();

    renderLightDef.layer = GetEntity()->GetLayer();

    ComTransform *transform = GetEntity()->GetTransform();
    renderLightDef.origin = transform->GetOrigin();
    renderLightDef.axis = transform->GetAxis();

    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComLight::TransformUpdated, SignalObject::Unique);

    // 3d sprite for editor
    spriteMesh = meshManager.GetMesh("_defaultQuadMesh");

    memset(&spriteDef, 0, sizeof(spriteDef));
    spriteDef.flags = RenderObject::BillboardFlag;
    spriteDef.layer = TagLayerSettings::EditorLayer;
    spriteDef.maxVisDist = MeterToUnit(50.0f);

    Texture *spriteTexture = textureManager.GetTexture(LightSpriteTexturePath(renderLightDef.type), Texture::Clamp | Texture::HighQuality);
    spriteDef.materials.SetCount(1);
    spriteDef.materials[0] = materialManager.GetSingleTextureMaterial(spriteTexture, Material::SpriteHint);
    textureManager.ReleaseTexture(spriteTexture);
    
    spriteDef.mesh = spriteMesh->InstantiateMesh(Mesh::StaticMesh);
    spriteDef.localAABB = spriteMesh->GetAABB();
    spriteDef.origin = transform->GetOrigin();
    spriteDef.scale = Vec3(1, 1, 1);
    spriteDef.axis = Mat3::identity;
    spriteDef.materialParms[RenderObject::RedParm] = renderLightDef.materialParms[RenderObject::RedParm];
    spriteDef.materialParms[RenderObject::GreenParm] = renderLightDef.materialParms[RenderObject::GreenParm];
    spriteDef.materialParms[RenderObject::BlueParm] = renderLightDef.materialParms[RenderObject::BlueParm];
    spriteDef.materialParms[RenderObject::AlphaParm] = 1.0f;
    spriteDef.materialParms[RenderObject::TimeOffsetParm] = renderLightDef.materialParms[RenderObject::TimeOffsetParm];
    spriteDef.materialParms[RenderObject::TimeScaleParm] = renderLightDef.materialParms[RenderObject::TimeScaleParm];
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
    if (renderLightHandle != -1) {
        renderWorld->RemoveRenderLight(renderLightHandle);
        renderLightHandle = -1;
    }

    if (spriteHandle != -1) {
        renderWorld->RemoveRenderObject(spriteHandle);
        spriteHandle = -1;
    }
}

bool ComLight::HasRenderEntity(int renderEntityHandle) const { 
    if (spriteHandle == renderEntityHandle) {
        return true;
    }

    return false;
}

void ComLight::DrawGizmos(const RenderView::State &viewState, bool selected) {
    const Color4 lightColor = Color4(GetColor(), 1.0f);

    if (renderLightDef.type == RenderLight::DirectionalLight || renderLightDef.type == RenderLight::SpotLight) {
        if (selected) {
            renderWorld->SetDebugColor(Color4::white, Color4::zero);
        } else {
            renderWorld->SetDebugColor(lightColor, Color4::zero);
        }

        if (selected) {
            renderWorld->SetDebugColor(lightColor, Color4::zero);

            if (renderLightDef.type == RenderLight::DirectionalLight) {
                OBB box;
                box.SetCenter(renderLightDef.origin + renderLightDef.axis[0] * renderLightDef.size[0] * 0.5f);
                box.SetExtents(Vec3(renderLightDef.size[0] * 0.5f, renderLightDef.size[1], renderLightDef.size[2]));
                box.SetAxis(renderLightDef.axis);
                renderWorld->DebugOBB(box, 1.0f, true, true);
            } else {
                Frustum frustum;
                frustum.SetOrigin(renderLightDef.origin);
                frustum.SetAxis(renderLightDef.axis);
                frustum.SetSize(renderLightDef.zNear, renderLightDef.size[0], renderLightDef.size[1], renderLightDef.size[2]);
                renderWorld->DebugFrustum(frustum, false, 1.0f, true, true);
            }
        }
    } else if (renderLightDef.type == RenderLight::PointLight) {
        if (selected) {
            renderWorld->SetDebugColor(Color4::white, Color4::zero);
        } else {
            renderWorld->SetDebugColor(lightColor, Color4::zero);
        }

        if (selected) {
            renderWorld->SetDebugColor(lightColor, Color4::zero);
            renderWorld->DebugEllipse(renderLightDef.origin, renderLightDef.axis[0], renderLightDef.axis[1], renderLightDef.size[0], renderLightDef.size[1], 1, true, true);
            renderWorld->DebugEllipse(renderLightDef.origin, renderLightDef.axis[1], renderLightDef.axis[2], renderLightDef.size[1], renderLightDef.size[2], 1, true, true);
            renderWorld->DebugEllipse(renderLightDef.origin, renderLightDef.axis[0], renderLightDef.axis[2], renderLightDef.size[0], renderLightDef.size[2], 1, true, true);

            Color4 lightColor2 = lightColor;
            lightColor2.a = 0.2f;
            renderWorld->SetDebugColor(lightColor2, Color4::zero);
            renderWorld->DebugLine(renderLightDef.origin - renderLightDef.axis[0] * renderLightDef.size[0] * 0.2f, renderLightDef.origin - renderLightDef.axis[0] * renderLightDef.size[0], 1, true);
            renderWorld->DebugLine(renderLightDef.origin + renderLightDef.axis[0] * renderLightDef.size[0] * 0.2f, renderLightDef.origin + renderLightDef.axis[0] * renderLightDef.size[0], 1, true);
            renderWorld->DebugLine(renderLightDef.origin - renderLightDef.axis[1] * renderLightDef.size[1] * 0.2f, renderLightDef.origin - renderLightDef.axis[1] * renderLightDef.size[1], 1, true);
            renderWorld->DebugLine(renderLightDef.origin + renderLightDef.axis[1] * renderLightDef.size[1] * 0.2f, renderLightDef.origin + renderLightDef.axis[1] * renderLightDef.size[1], 1, true);
            renderWorld->DebugLine(renderLightDef.origin - renderLightDef.axis[2] * renderLightDef.size[2] * 0.2f, renderLightDef.origin - renderLightDef.axis[2] * renderLightDef.size[2], 1, true);
            renderWorld->DebugLine(renderLightDef.origin + renderLightDef.axis[2] * renderLightDef.size[2] * 0.2f, renderLightDef.origin + renderLightDef.axis[2] * renderLightDef.size[2], 1, true);
        }
    }

    // Fade icon alpha in near distance
    float alpha = BE1::Clamp(spriteDef.origin.Distance(viewState.origin) / MeterToUnit(8.0f), 0.01f, 1.0f);

    spriteDef.materials[0]->GetPass()->constantColor[3] = alpha;
}

const AABB ComLight::GetAABB() {
    return Sphere(Vec3::origin, MeterToUnit(0.5f)).ToAABB();
}

bool ComLight::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    return false;
}

void ComLight::UpdateVisuals() {
    if (!IsInitialized() || !IsActiveInHierarchy()) {
        return;
    }

    if (renderLightHandle == -1) {
        renderLightHandle = renderWorld->AddRenderLight(&renderLightDef);
    } else {
        renderWorld->UpdateRenderLight(renderLightHandle, &renderLightDef);
    }

    if (spriteHandle == -1) {
        spriteHandle = renderWorld->AddRenderObject(&spriteDef);
    } else {
        renderWorld->UpdateRenderObject(spriteHandle, &spriteDef);
    }
}

void ComLight::LayerChanged(const Entity *entity) {
    renderLightDef.layer = entity->GetProperty("layer").As<int>();

    UpdateVisuals();
}

void ComLight::TransformUpdated(const ComTransform *transform) {
    renderLightDef.origin = transform->GetOrigin();
    renderLightDef.axis = transform->GetAxis();
    
    spriteDef.origin = renderLightDef.origin;
    
    UpdateVisuals();
}

RenderLight::Type ComLight::GetLightType() const {
    return renderLightDef.type;
}

void ComLight::SetLightType(RenderLight::Type type) {
    renderLightDef.type = type;

    if (IsInitialized()) {
        materialManager.ReleaseMaterial(spriteDef.materials[0]);

        Texture *spriteTexture = textureManager.GetTexture(LightSpriteTexturePath(renderLightDef.type), Texture::Clamp | Texture::HighQuality);
        spriteDef.materials[0] = materialManager.GetSingleTextureMaterial(spriteTexture, Material::SpriteHint);
        textureManager.ReleaseTexture(spriteTexture);

        UpdateVisuals();
    }
}

bool ComLight::IsPrimaryLight() const {
    return !!(renderLightDef.flags & RenderLight::PrimaryLightFlag);
}

void ComLight::SetPrimaryLight(bool isPrimaryLight) {
    if (isPrimaryLight) {
        renderLightDef.flags |= RenderLight::PrimaryLightFlag;
    } else {
        renderLightDef.flags &= ~RenderLight::PrimaryLightFlag;
    }

    UpdateVisuals();
}

Vec3 ComLight::GetLightSize() const {
    return Vec3(renderLightDef.size);
}

void ComLight::SetLightSize(const Vec3 &lightSize) {
    renderLightDef.size = lightSize;

    renderLightDef.size[0] = Max(renderLightDef.size[0], 1.0f);
    renderLightDef.size[1] = Max(renderLightDef.size[1], 1.0f);
    renderLightDef.size[2] = Max(renderLightDef.size[2], 1.0f);

    UpdateVisuals();
}

float ComLight::GetLightZNear() const {
    return renderLightDef.zNear;
}

void ComLight::SetLightZNear(float lightZNear) {
    renderLightDef.zNear = lightZNear;

    UpdateVisuals();
}

float ComLight::GetMaxVisDist() const {
    return renderLightDef.maxVisDist;
}

void ComLight::SetMaxVisDist(float maxVisDist) {
    renderLightDef.maxVisDist = maxVisDist;

    UpdateVisuals();
}

Guid ComLight::GetMaterialGuid() const {
    if (renderLightDef.material) {
        const Str materialPath = renderLightDef.material->GetHashName();
        return resourceGuidMapper.Get(materialPath);
    }
    return Guid();
}

void ComLight::SetMaterialGuid(const Guid &materialGuid) {
    if (renderLightDef.material) {
        materialManager.ReleaseMaterial(renderLightDef.material);
    }

    const Str materialPath = resourceGuidMapper.Get(materialGuid);
    renderLightDef.material = materialManager.GetMaterial(materialPath);

    UpdateVisuals();
}

Color3 ComLight::GetColor() const {
    return Color3(&renderLightDef.materialParms[RenderObject::RedParm]);
}

void ComLight::SetColor(const Color3 &color) {
    renderLightDef.materialParms[RenderObject::RedParm] = color.r;
    renderLightDef.materialParms[RenderObject::GreenParm] = color.g;
    renderLightDef.materialParms[RenderObject::BlueParm] = color.b;

    spriteDef.materialParms[RenderObject::RedParm] = color.r;
    spriteDef.materialParms[RenderObject::GreenParm] = color.g;
    spriteDef.materialParms[RenderObject::BlueParm] = color.b;

    UpdateVisuals();
}

float ComLight::GetTimeOffset() const {
    return renderLightDef.materialParms[RenderObject::TimeOffsetParm];
}

void ComLight::SetTimeOffset(float timeOffset) {
    renderLightDef.materialParms[RenderObject::TimeOffsetParm] = timeOffset;

    UpdateVisuals();
}

float ComLight::GetTimeScale() const {
    return renderLightDef.materialParms[RenderObject::TimeScaleParm];
}

void ComLight::SetTimeScale(float timeScale) {
    renderLightDef.materialParms[RenderObject::TimeScaleParm] = timeScale;

    UpdateVisuals();
}

bool ComLight::IsCastShadows() const {
    return !!(renderLightDef.flags & RenderLight::CastShadowsFlag);
}

void ComLight::SetCastShadows(bool castShadows) {
    if (castShadows) {
        renderLightDef.flags |= RenderLight::CastShadowsFlag;
    } else {
        renderLightDef.flags &= ~RenderLight::CastShadowsFlag;
    }

    UpdateVisuals();
}

float ComLight::GetShadowOffsetFactor() const {
    return renderLightDef.shadowOffsetFactor;
}

void ComLight::SetShadowOffsetFactor(float factor) {
    renderLightDef.shadowOffsetFactor = factor;

    UpdateVisuals();
}

float ComLight::GetShadowOffsetUnits() const {
    return renderLightDef.shadowOffsetUnits;
}

void ComLight::SetShadowOffsetUnits(float units) {
    renderLightDef.shadowOffsetUnits = units;

    UpdateVisuals();
}

float ComLight::GetFallOffExponent() const {
    return renderLightDef.fallOffExponent;
}

void ComLight::SetFallOffExponent(float fallOff) {
    renderLightDef.fallOffExponent = fallOff;

    UpdateVisuals();
}

float ComLight::GetIntensity() const {
    return renderLightDef.intensity;
}

void ComLight::SetIntensity(float intensity) {
    renderLightDef.intensity = intensity;

    UpdateVisuals();
}

const Vec3 &ComLight::GetRadius() const {
    return renderLightDef.size;
}

void ComLight::SetRadius(const Vec3 &radius) {
    renderLightDef.size = GetProperty("radius").As<Vec3>();

    UpdateVisuals();
}

BE_NAMESPACE_END
