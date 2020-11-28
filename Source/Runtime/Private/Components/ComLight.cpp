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
#include "Asset/Resource.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Light", ComLight, Component)
BEGIN_EVENTS(ComLight)
END_EVENTS

void ComLight::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("lightType", "Light Type", RenderLight::Type::Enum, GetLightType, SetLightType, RenderLight::Type::Point,
        "", PropertyInfo::Flag::Editor).SetEnumString("Point;Spot;Directional");
    REGISTER_ACCESSOR_PROPERTY("primaryLight", "Is Main Light", bool, IsPrimaryLight, SetPrimaryLight, false,
        "", PropertyInfo::Flag::Editor);
    REGISTER_MIXED_ACCESSOR_PROPERTY("material", "Material", Guid, GetMaterialGuid, SetMaterialGuid, GuidMapper::zeroClampLightMaterialGuid, 
        "", PropertyInfo::Flag::Editor).SetMetaObject(&MaterialResource::metaObject);
    REGISTER_MIXED_ACCESSOR_PROPERTY("color", "Color", Color3, GetColor, SetColor, Color3::white, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("intensity", "Intensity", float, GetIntensity, SetIntensity, 2.f,
        "", PropertyInfo::Flag::Editor).SetRange(0, 100, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("fallOffExponent", "Fall Off Exponent", float, GetFallOffExponent, SetFallOffExponent, 1.25f,
        "", PropertyInfo::Flag::Editor).SetRange(0.01f, 100, 0.1f);
    REGISTER_MIXED_ACCESSOR_PROPERTY("lightSize", "Light Size", Vec3, GetLightSize, SetLightSize, Vec3(MeterToUnit(3.0f)),
        "", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("lightZNear", "Near Distance", float, GetLightZNear, SetLightZNear, MeterToUnit(0.1f),
        "", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor).SetRange(MeterToUnit(0.01f), 100, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("castShadows", "Shadows/Cast Shadows", bool, IsCastShadows, SetCastShadows, false,
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("shadowOffsetFactor", "Shadows/Offset Factor", float, GetShadowOffsetFactor, SetShadowOffsetFactor, 3.f,
        "", PropertyInfo::Flag::Editor).SetRange(0, 16, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("shadowOffsetUnits", "Shadows/Offset Units", float, GetShadowOffsetUnits, SetShadowOffsetUnits, 200.f,
        "", PropertyInfo::Flag::Editor).SetRange(0, 1000, 1);
    REGISTER_ACCESSOR_PROPERTY("timerOffset", "Time Offset", float, GetTimeOffset, SetTimeOffset, 0.f, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("timeScale", "Time Scale", float, GetTimeScale, SetTimeScale, 1.f, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("maxVisDist", "Max Visible Distance", float, GetMaxVisDist, SetMaxVisDist, 1000.f, 
        "", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
}

ComLight::ComLight() {
    renderLightHandle = -1;

#if WITH_EDITOR
    spriteHandle = -1;
    spriteMesh = nullptr;
#endif
}

ComLight::~ComLight() {
    Purge(false);
}

void ComLight::Purge(bool chainPurge) {
#if WITH_EDITOR
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

    if (renderLightDef.material) {
        materialManager.ReleaseMaterial(renderLightDef.material);
        renderLightDef.material = nullptr;
    }

    if (renderLightHandle != -1) {
        renderWorld->RemoveRenderLight(renderLightHandle);
        renderLightHandle = -1;
    }

    if (chainPurge) {
        Component::Purge();
    }
}

static const char *LightSpriteTexturePath(RenderLight::Type::Enum lightType) {
    switch (lightType) {
    case RenderLight::Type::Point:
        return "Data/EditorUI/OmniLight.png";
    case RenderLight::Type::Spot:
        return "Data/EditorUI/ProjectedLight.png";
    case RenderLight::Type::Directional:
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
    renderLightDef.staticMask = GetEntity()->GetStaticMask();

    ComTransform *transform = GetEntity()->GetTransform();
    renderLightDef.origin = transform->GetOrigin();
    renderLightDef.axis = transform->GetAxis();

    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComLight::TransformUpdated, SignalObject::ConnectionType::Unique);

#if WITH_EDITOR
    // 3d sprite for editor
    spriteMesh = meshManager.GetMesh("_defaultQuadMesh");

    spriteDef.flags = RenderObject::Flag::Billboard;
    spriteDef.layer = TagLayerSettings::BuiltInLayer::Editor;
    spriteDef.maxVisDist = MeterToUnit(50.0f);

    Texture *spriteTexture = textureManager.GetTextureWithoutTextureInfo(LightSpriteTexturePath(renderLightDef.type), Texture::Flag::Clamp | Texture::Flag::HighQuality);
    spriteDef.materials.SetCount(1);
    spriteDef.materials[0] = materialManager.GetSingleTextureMaterial(spriteTexture, Material::TextureHint::Sprite);
    textureManager.ReleaseTexture(spriteTexture);
    
    spriteDef.mesh = spriteMesh->InstantiateMesh(Mesh::Type::Static);
    spriteDef.aabb = spriteMesh->GetAABB();
    spriteDef.worldMatrix = transform->GetMatrixNoScale();
    spriteDef.materialParms[RenderObject::MaterialParm::Red] = renderLightDef.materialParms[RenderObject::MaterialParm::Red];
    spriteDef.materialParms[RenderObject::MaterialParm::Green] = renderLightDef.materialParms[RenderObject::MaterialParm::Green];
    spriteDef.materialParms[RenderObject::MaterialParm::Blue] = renderLightDef.materialParms[RenderObject::MaterialParm::Blue];
    spriteDef.materialParms[RenderObject::MaterialParm::Alpha] = 1.0f;
    spriteDef.materialParms[RenderObject::MaterialParm::TimeOffset] = renderLightDef.materialParms[RenderObject::MaterialParm::TimeOffset];
    spriteDef.materialParms[RenderObject::MaterialParm::TimeScale] = renderLightDef.materialParms[RenderObject::MaterialParm::TimeScale];
#endif

    GetEntity()->Connect(&Entity::SIG_LayerChanged, this, (SignalCallback)&ComLight::LayerChanged, SignalObject::ConnectionType::Unique);
    GetEntity()->Connect(&Entity::SIG_StaticMaskChanged, this, (SignalCallback)&ComLight::StaticMaskChanged, SignalObject::ConnectionType::Unique);

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

#if WITH_EDITOR
    if (spriteHandle != -1) {
        renderWorld->RemoveRenderObject(spriteHandle);
        spriteHandle = -1;
    }
#endif
}

bool ComLight::HasRenderObject(int renderObjectHandle) const { 
#if WITH_EDITOR
    if (spriteHandle == renderObjectHandle) {
        return true;
    }
#endif

    return false;
}

#if WITH_EDITOR
void ComLight::DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) {
    const Color4 lightColor = Color4(GetColor(), 1.0f);

    if (renderLightDef.type == RenderLight::Type::Directional || renderLightDef.type == RenderLight::Type::Spot) {
        if (selectedByParent) {
            renderWorld->SetDebugColor(Color4::white, Color4::zero);
        } else {
            renderWorld->SetDebugColor(lightColor, Color4::zero);
        }

        if (selectedByParent) {
            renderWorld->SetDebugColor(lightColor, Color4::zero);

            if (renderLightDef.type == RenderLight::Type::Directional) {
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
    } else if (renderLightDef.type == RenderLight::Type::Point) {
        if (selectedByParent) {
            renderWorld->SetDebugColor(Color4::white, Color4::zero);
        } else {
            renderWorld->SetDebugColor(lightColor, Color4::zero);
        }

        if (selectedByParent) {
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
    float alpha = Clamp(spriteDef.worldMatrix.ToTranslationVec3().Distance(camera->GetState().origin) / MeterToUnit(8.0f), 0.01f, 1.0f);

    spriteDef.materials[0]->GetPass()->constantColor[3] = alpha;
}
#endif

const AABB ComLight::GetAABB() const {
    return Sphere(Vec3::origin, MeterToUnit(0.5f)).ToAABB();
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

#if WITH_EDITOR
    if (spriteHandle == -1) {
        spriteHandle = renderWorld->AddRenderObject(&spriteDef);
    } else {
        renderWorld->UpdateRenderObject(spriteHandle, &spriteDef);
    }
#endif
}

void ComLight::LayerChanged(const Entity *entity) {
    renderLightDef.layer = entity->GetLayer();

    UpdateVisuals();
}

void ComLight::StaticMaskChanged(const Entity *entity) {
    renderLightDef.staticMask = entity->GetStaticMask();

    UpdateVisuals();
}

void ComLight::TransformUpdated(const ComTransform *transform) {
    renderLightDef.origin = transform->GetOrigin();
    renderLightDef.axis = transform->GetAxis();

#if WITH_EDITOR
    spriteDef.worldMatrix.SetTranslation(renderLightDef.origin);
#endif

    UpdateVisuals();
}

RenderLight::Type::Enum ComLight::GetLightType() const {
    return renderLightDef.type;
}

void ComLight::SetLightType(RenderLight::Type::Enum type) {
    renderLightDef.type = type;

    if (IsInitialized()) {
#if WITH_EDITOR
        materialManager.ReleaseMaterial(spriteDef.materials[0]);

        Texture *spriteTexture = textureManager.GetTextureWithoutTextureInfo(LightSpriteTexturePath(renderLightDef.type), Texture::Flag::Clamp | Texture::Flag::HighQuality);
        spriteDef.materials[0] = materialManager.GetSingleTextureMaterial(spriteTexture, Material::TextureHint::Sprite);
        textureManager.ReleaseTexture(spriteTexture);
#endif

        UpdateVisuals();
    }
}

bool ComLight::IsPrimaryLight() const {
    return !!(renderLightDef.flags & RenderLight::Flag::PrimaryLight);
}

void ComLight::SetPrimaryLight(bool isPrimaryLight) {
    if (isPrimaryLight) {
        renderLightDef.flags |= RenderLight::Flag::PrimaryLight;
    } else {
        renderLightDef.flags &= ~RenderLight::Flag::PrimaryLight;
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
    // Release the previously used material
    if (renderLightDef.material) {
        materialManager.ReleaseMaterial(renderLightDef.material);
    }

    // Get the new material
    const Str materialPath = resourceGuidMapper.Get(materialGuid);
    renderLightDef.material = materialManager.GetMaterial(materialPath);

    UpdateVisuals();
}

Color3 ComLight::GetColor() const {
    return Color3(&renderLightDef.materialParms[RenderObject::MaterialParm::Red]);
}

void ComLight::SetColor(const Color3 &color) {
    renderLightDef.materialParms[RenderObject::MaterialParm::Red] = color.r;
    renderLightDef.materialParms[RenderObject::MaterialParm::Green] = color.g;
    renderLightDef.materialParms[RenderObject::MaterialParm::Blue] = color.b;

#if WITH_EDITOR
    spriteDef.materialParms[RenderObject::MaterialParm::Red] = color.r;
    spriteDef.materialParms[RenderObject::MaterialParm::Green] = color.g;
    spriteDef.materialParms[RenderObject::MaterialParm::Blue] = color.b;
#endif

    UpdateVisuals();
}

float ComLight::GetTimeOffset() const {
    return renderLightDef.materialParms[RenderObject::MaterialParm::TimeOffset];
}

void ComLight::SetTimeOffset(float timeOffset) {
    renderLightDef.materialParms[RenderObject::MaterialParm::TimeOffset] = timeOffset;

    UpdateVisuals();
}

float ComLight::GetTimeScale() const {
    return renderLightDef.materialParms[RenderObject::MaterialParm::TimeScale];
}

void ComLight::SetTimeScale(float timeScale) {
    renderLightDef.materialParms[RenderObject::MaterialParm::TimeScale] = timeScale;

    UpdateVisuals();
}

bool ComLight::IsCastShadows() const {
    return !!(renderLightDef.flags & RenderLight::Flag::CastShadows);
}

void ComLight::SetCastShadows(bool castShadows) {
    if (castShadows) {
        renderLightDef.flags |= RenderLight::Flag::CastShadows;
    } else {
        renderLightDef.flags &= ~RenderLight::Flag::CastShadows;
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
