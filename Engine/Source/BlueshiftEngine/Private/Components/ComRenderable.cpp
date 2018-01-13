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
#include "Components/ComRenderable.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

ABSTRACT_DECLARATION("Renderable", ComRenderable, Component)
BEGIN_EVENTS(ComRenderable)
END_EVENTS

void ComRenderable::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("color", "Color", Color3, GetColor, SetColor, Color3::white, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("alpha", "Alpha", float, GetAlpha, SetAlpha, 1.f, "", PropertyInfo::EditorFlag)
        .SetRange(0, 1, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("billboard", "Billboard", bool, IsBillboard, SetBillboard, false, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("timeOffset", "Time Offset", float, GetTimeOffset, SetTimeOffset, 0.f, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("timeScale", "Time Scale", float, GetTimeScale, SetTimeScale, 1.f, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("maxVisDist", "Max Visible Distance", float, GetMaxVisDist, SetMaxVisDist, 20000.f, "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("skipSelection", "Skip Selection", bool, IsSkipSelection, SetSkipSelection, false, "", PropertyInfo::SkipSerializationFlag);
}

ComRenderable::ComRenderable() {
    sceneEntityHandle = -1;
    memset(&sceneEntity, 0, sizeof(sceneEntity));
    renderWorld = nullptr;
}

ComRenderable::~ComRenderable() {
    Purge(false);
}

void ComRenderable::Purge(bool chainPurge) {
    for (int i = 0; i < sceneEntity.materials.Count(); i++) {
        materialManager.ReleaseMaterial(sceneEntity.materials[i]);
    }
    sceneEntity.materials.Clear();

    if (sceneEntityHandle != -1) {
        renderWorld->RemoveEntity(sceneEntityHandle);
        sceneEntityHandle = -1;
        renderWorld = nullptr;
    }

    if (chainPurge) {
        Component::Purge();
    }
}

void ComRenderable::Init() {
    Component::Init();

    renderWorld = GetGameWorld()->GetRenderWorld();

    sceneEntity.layer = GetEntity()->GetLayer();
    sceneEntity.wireframeColor.Set(1, 1, 1, 1);

    ComTransform *transform = GetEntity()->GetTransform();
    sceneEntity.origin = transform->GetOrigin();
    sceneEntity.scale = transform->GetScale();
    sceneEntity.axis = transform->GetAxis();

    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComRenderable::TransformUpdated, SignalObject::Unique);

    GetEntity()->Connect(&Entity::SIG_LayerChanged, this, (SignalCallback)&ComRenderable::LayerChanged, SignalObject::Unique);
}

void ComRenderable::OnActive() {
    UpdateVisuals();
}

void ComRenderable::OnInactive() {
    if (sceneEntityHandle != -1) {
        renderWorld->RemoveEntity(sceneEntityHandle);
        sceneEntityHandle = -1;
    }
}

bool ComRenderable::HasRenderEntity(int sceneEntityHandle) const { 
    if (this->sceneEntityHandle == sceneEntityHandle) {
        return true;
    }

    return false;
}

void ComRenderable::UpdateVisuals() {
    if (!IsInitialized() || !IsActiveInHierarchy()) {
        return;
    }

    if (sceneEntityHandle == -1) {
        sceneEntityHandle = renderWorld->AddEntity(&sceneEntity);
    } else {
        renderWorld->UpdateEntity(sceneEntityHandle, &sceneEntity);
    }
}

const AABB ComRenderable::GetAABB() {
    const ComTransform *transform = GetEntity()->GetTransform();
    return sceneEntity.aabb * transform->GetScale();
}

bool ComRenderable::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    if (!sceneEntity.mesh) {
        return false;
    }

    Vec3 localStart = sceneEntity.axis.TransposedMulVec(start - sceneEntity.origin) / sceneEntity.scale;
    Vec3 localDir = sceneEntity.axis.TransposedMulVec(dir) / sceneEntity.scale;
    localDir.Normalize();

    float localS = sceneEntity.mesh->GetAABB().RayIntersection(localStart, localDir);
    if (localS == FLT_MAX) {
        return false;
    }

    float s = localS * (localDir * sceneEntity.scale).Length();
    if (s > lastScale) {
        return false;
    }

    if (sceneEntity.mesh->RayIntersection(localStart, localDir, backFaceCull, localS)) {
        s = localS * (localDir * sceneEntity.scale).Length();
        if (s > 0.0f && s < lastScale) {
            lastScale = s;
            return true;
        }
    }

    return false;
}

void ComRenderable::SetWireframeColor(const Color4 &color) {
    sceneEntity.wireframeColor = color;

    UpdateVisuals();
}

void ComRenderable::ShowWireframe(SceneEntity::WireframeMode wireframeMode) {
    sceneEntity.wireframeMode = wireframeMode;
    
    UpdateVisuals();
}

void ComRenderable::LayerChanged(const Entity *entity) {
    sceneEntity.layer = entity->GetProperty("layer").As<int>();
    
    UpdateVisuals();
}

void ComRenderable::TransformUpdated(const ComTransform *transform) {
    sceneEntity.origin = transform->GetOrigin();
    sceneEntity.axis = transform->GetAxis();
    sceneEntity.scale = transform->GetScale();

    UpdateVisuals();
}

float ComRenderable::GetMaxVisDist() const {
    return sceneEntity.maxVisDist;
}

void ComRenderable::SetMaxVisDist(float maxVisDist) {
    sceneEntity.maxVisDist = maxVisDist;

    UpdateVisuals();
}

Color3 ComRenderable::GetColor() const {
    return Color3(&sceneEntity.materialParms[SceneEntity::RedParm]);
}

void ComRenderable::SetColor(const Color3 &color) {
    sceneEntity.materialParms[SceneEntity::RedParm] = color.r;
    sceneEntity.materialParms[SceneEntity::GreenParm] = color.g;
    sceneEntity.materialParms[SceneEntity::BlueParm] = color.b;
    
    UpdateVisuals();
}

float ComRenderable::GetAlpha() const {
    return sceneEntity.materialParms[SceneEntity::AlphaParm];
}

void ComRenderable::SetAlpha(float alpha) {
    sceneEntity.materialParms[SceneEntity::AlphaParm] = alpha;

    UpdateVisuals();
}

float ComRenderable::GetTimeOffset() const {
    return sceneEntity.materialParms[SceneEntity::TimeOffsetParm];
}

void ComRenderable::SetTimeOffset(float timeOffset) {
    sceneEntity.materialParms[SceneEntity::TimeOffsetParm] = timeOffset;

    UpdateVisuals();
}

float ComRenderable::GetTimeScale() const {
    return sceneEntity.materialParms[SceneEntity::TimeScaleParm];
}

void ComRenderable::SetTimeScale(float timeScale) {
    sceneEntity.materialParms[SceneEntity::TimeScaleParm] = timeScale;

    UpdateVisuals();
}

bool ComRenderable::IsBillboard() const {
    return sceneEntity.billboard;
}

void ComRenderable::SetBillboard(bool billboard) {
    sceneEntity.billboard = billboard;

    UpdateVisuals();
}

bool ComRenderable::IsSkipSelection() const {
    return sceneEntity.skipSelectionBuffer;
}

void ComRenderable::SetSkipSelection(bool skip) {
    sceneEntity.skipSelectionBuffer = skip;

    UpdateVisuals();
}

BE_NAMESPACE_END
