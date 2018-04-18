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
    REGISTER_MIXED_ACCESSOR_PROPERTY("color", "Color", Color3, GetColor, SetColor, Color3::white, 
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("alpha", "Alpha", float, GetAlpha, SetAlpha, 1.f, 
        "", PropertyInfo::EditorFlag).SetRange(0, 1, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("billboard", "Billboard", bool, IsBillboard, SetBillboard, false, 
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("timeOffset", "Time Offset", float, GetTimeOffset, SetTimeOffset, 0.f, 
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("timeScale", "Time Scale", float, GetTimeScale, SetTimeScale, 1.f, 
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("maxVisDist", "Max Visible Distance", float, GetMaxVisDist, SetMaxVisDist, 20000.f, 
        "", PropertyInfo::SystemUnits | PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("skipSelection", "Skip Selection", bool, IsSkipSelection, SetSkipSelection, false, 
        "", PropertyInfo::SkipSerializationFlag);
}

ComRenderable::ComRenderable() {
    sceneObjectHandle = -1;
    memset(&sceneObjectParms, 0, sizeof(sceneObjectParms));
    renderWorld = nullptr;
}

ComRenderable::~ComRenderable() {
    Purge(false);
}

void ComRenderable::Purge(bool chainPurge) {
    for (int i = 0; i < sceneObjectParms.materials.Count(); i++) {
        materialManager.ReleaseMaterial(sceneObjectParms.materials[i]);
    }
    sceneObjectParms.materials.Clear();

    if (sceneObjectHandle != -1) {
        renderWorld->RemoveObject(sceneObjectHandle);
        sceneObjectHandle = -1;
        renderWorld = nullptr;
    }

    if (chainPurge) {
        Component::Purge();
    }
}

void ComRenderable::Init() {
    Component::Init();

    renderWorld = GetGameWorld()->GetRenderWorld();

    sceneObjectParms.layer = GetEntity()->GetLayer();
    sceneObjectParms.wireframeColor.Set(1, 1, 1, 1);

    ComTransform *transform = GetEntity()->GetTransform();
    sceneObjectParms.origin = transform->GetOrigin();
    sceneObjectParms.scale = transform->GetScale();
    sceneObjectParms.axis = transform->GetAxis();

    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComRenderable::TransformUpdated, SignalObject::Unique);

    GetEntity()->Connect(&Entity::SIG_LayerChanged, this, (SignalCallback)&ComRenderable::LayerChanged, SignalObject::Unique);
}

void ComRenderable::OnActive() {
    UpdateVisuals();
}

void ComRenderable::OnInactive() {
    if (sceneObjectHandle != -1) {
        renderWorld->RemoveObject(sceneObjectHandle);
        sceneObjectHandle = -1;
    }
}

bool ComRenderable::HasRenderEntity(int sceneObjectHandle) const { 
    if (this->sceneObjectHandle == sceneObjectHandle) {
        return true;
    }

    return false;
}

void ComRenderable::UpdateVisuals() {
    if (!IsInitialized() || !IsActiveInHierarchy()) {
        return;
    }

    if (sceneObjectHandle == -1) {
        sceneObjectHandle = renderWorld->AddObject(&sceneObjectParms);
    } else {
        renderWorld->UpdateObject(sceneObjectHandle, &sceneObjectParms);
    }
}

const AABB ComRenderable::GetAABB() {
    const ComTransform *transform = GetEntity()->GetTransform();
    return sceneObjectParms.aabb * transform->GetScale();
}

bool ComRenderable::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    if (!sceneObjectParms.mesh) {
        return false;
    }

    Vec3 localStart = sceneObjectParms.axis.TransposedMulVec(start - sceneObjectParms.origin) / sceneObjectParms.scale;
    Vec3 localDir = sceneObjectParms.axis.TransposedMulVec(dir) / sceneObjectParms.scale;
    localDir.Normalize();

    float localS = sceneObjectParms.mesh->GetAABB().RayIntersection(localStart, localDir);
    if (localS == FLT_MAX) {
        return false;
    }

    float s = localS * (localDir * sceneObjectParms.scale).Length();
    if (s > lastScale) {
        return false;
    }

    if (sceneObjectParms.mesh->RayIntersection(localStart, localDir, backFaceCull, localS)) {
        s = localS * (localDir * sceneObjectParms.scale).Length();
        if (s > 0.0f && s < lastScale) {
            lastScale = s;
            return true;
        }
    }

    return false;
}

void ComRenderable::SetWireframeColor(const Color4 &color) {
    sceneObjectParms.wireframeColor = color;

    UpdateVisuals();
}

void ComRenderable::ShowWireframe(SceneObject::WireframeMode wireframeMode) {
    sceneObjectParms.wireframeMode = wireframeMode;
    
    UpdateVisuals();
}

void ComRenderable::LayerChanged(const Entity *entity) {
    sceneObjectParms.layer = entity->GetProperty("layer").As<int>();
    
    UpdateVisuals();
}

void ComRenderable::TransformUpdated(const ComTransform *transform) {
    sceneObjectParms.origin = transform->GetOrigin();
    sceneObjectParms.axis = transform->GetAxis();
    sceneObjectParms.scale = transform->GetScale();

    UpdateVisuals();
}

float ComRenderable::GetMaxVisDist() const {
    return sceneObjectParms.maxVisDist;
}

void ComRenderable::SetMaxVisDist(float maxVisDist) {
    sceneObjectParms.maxVisDist = maxVisDist;

    UpdateVisuals();
}

Color3 ComRenderable::GetColor() const {
    return Color3(&sceneObjectParms.materialParms[SceneObject::RedParm]);
}

void ComRenderable::SetColor(const Color3 &color) {
    sceneObjectParms.materialParms[SceneObject::RedParm] = color.r;
    sceneObjectParms.materialParms[SceneObject::GreenParm] = color.g;
    sceneObjectParms.materialParms[SceneObject::BlueParm] = color.b;
    
    UpdateVisuals();
}

float ComRenderable::GetAlpha() const {
    return sceneObjectParms.materialParms[SceneObject::AlphaParm];
}

void ComRenderable::SetAlpha(float alpha) {
    sceneObjectParms.materialParms[SceneObject::AlphaParm] = alpha;

    UpdateVisuals();
}

float ComRenderable::GetTimeOffset() const {
    return sceneObjectParms.materialParms[SceneObject::TimeOffsetParm];
}

void ComRenderable::SetTimeOffset(float timeOffset) {
    sceneObjectParms.materialParms[SceneObject::TimeOffsetParm] = timeOffset;

    UpdateVisuals();
}

float ComRenderable::GetTimeScale() const {
    return sceneObjectParms.materialParms[SceneObject::TimeScaleParm];
}

void ComRenderable::SetTimeScale(float timeScale) {
    sceneObjectParms.materialParms[SceneObject::TimeScaleParm] = timeScale;

    UpdateVisuals();
}

bool ComRenderable::IsBillboard() const {
    return sceneObjectParms.billboard;
}

void ComRenderable::SetBillboard(bool billboard) {
    sceneObjectParms.billboard = billboard;

    UpdateVisuals();
}

bool ComRenderable::IsSkipSelection() const {
    return sceneObjectParms.skipSelectionBuffer;
}

void ComRenderable::SetSkipSelection(bool skip) {
    sceneObjectParms.skipSelectionBuffer = skip;

    UpdateVisuals();
}

BE_NAMESPACE_END
