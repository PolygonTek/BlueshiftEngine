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
#include "StaticBatching/StaticBatch.h"

BE_NAMESPACE_BEGIN

ABSTRACT_DECLARATION("Renderable", ComRenderable, Component)
BEGIN_EVENTS(ComRenderable)
END_EVENTS

void ComRenderable::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("color", "Color", Color3, GetColor, SetColor, Color3::white, 
        "The color to use when rendering.", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("alpha", "Alpha", float, GetAlpha, SetAlpha, 1.f, 
        "The alpha to use when rendering.", PropertyInfo::Flag::Editor).SetRange(0, 1, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("billboard", "Billboard", bool, IsBillboard, SetBillboard, false, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("timeOffset", "Time Offset", float, GetTimeOffset, SetTimeOffset, 0.f, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("timeScale", "Time Scale", float, GetTimeScale, SetTimeScale, 1.f, 
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("maxVisDist", "Max Visible Distance", float, GetMaxVisDist, SetMaxVisDist, 4000.f, 
        "", PropertyInfo::Flag::SystemUnits | PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("skipSelection", "Skip Selection", bool, IsSkipSelection, SetSkipSelection, false, 
        "", PropertyInfo::Flag::SkipSerialization);
}

ComRenderable::ComRenderable() {
    renderObjectHandle = -1;
    staticBatchIndex = -1;
    renderWorld = nullptr;
}

ComRenderable::~ComRenderable() {
    Purge(false);
}

void ComRenderable::Purge(bool chainPurge) {
    for (int i = 0; i < renderObjectDef.materials.Count(); i++) {
        materialManager.ReleaseMaterial(renderObjectDef.materials[i]);
    }
    renderObjectDef.materials.Clear();

    if (renderObjectHandle != -1) {
        renderWorld->RemoveRenderObject(renderObjectHandle);
        renderObjectHandle = -1;
        renderWorld = nullptr;
    }

    staticBatchIndex = -1;

    if (chainPurge) {
        Component::Purge();
    }
}

void ComRenderable::Init() {
    Component::Init();

    renderWorld = GetGameWorld()->GetRenderWorld();

    renderObjectDef.layer = GetEntity()->GetLayer();
    renderObjectDef.staticMask = GetEntity()->GetStaticMask();
    renderObjectDef.wireframeColor.Set(1, 1, 1, 1);

    ComTransform *transform = GetEntity()->GetTransform();
    renderObjectDef.worldMatrix = transform->GetMatrix();

    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComRenderable::TransformUpdated, SignalObject::ConnectionType::Unique);

    GetEntity()->Connect(&Entity::SIG_LayerChanged, this, (SignalCallback)&ComRenderable::LayerChanged, SignalObject::ConnectionType::Unique);
    GetEntity()->Connect(&Entity::SIG_StaticMaskChanged, this, (SignalCallback)&ComRenderable::StaticMaskChanged, SignalObject::ConnectionType::Unique);
}

void ComRenderable::OnActive() {
    UpdateVisuals();
}

void ComRenderable::OnInactive() {
    if (renderObjectHandle != -1) {
        renderWorld->RemoveRenderObject(renderObjectHandle);
        renderObjectHandle = -1;
    }
}

bool ComRenderable::HasRenderObject(int renderObjectHandle) const { 
    if (this->renderObjectHandle == renderObjectHandle) {
        return true;
    }

    return false;
}

void ComRenderable::UpdateVisuals() {
    if (!IsInitialized() || !IsActiveInHierarchy()) {
        return;
    }

    if (staticBatchIndex >= 0) {
        const StaticBatch *staticBatch = StaticBatch::GetStaticBatchByIndex(staticBatchIndex);
        if (staticBatch->GetRootEntity() != GetEntity()) {
            return;
        }
    }

    if (renderObjectHandle == -1) {
        renderObjectHandle = renderWorld->AddRenderObject(&renderObjectDef);
    } else {
        renderWorld->UpdateRenderObject(renderObjectHandle, &renderObjectDef);
    }
}

bool ComRenderable::IsVisibleInPreviousFrame() const {
    if (renderObjectHandle == -1) {
        return false;
    }
    
    const RenderObject *renderObject = renderWorld->GetRenderObject(renderObjectHandle);
    return renderWorld->GetViewCount() - renderObject->GetViewCount() <= 1;
}

const AABB ComRenderable::GetAABB() const {
    return renderObjectDef.aabb;
}

bool ComRenderable::IntersectRay(const Ray &ray, bool backFaceCull, float *hitDist) const {
    if (!renderObjectDef.mesh) {
        return false;
    }

    Mat3x4 worldToLocal = renderObjectDef.worldMatrix.Inverse();

    Ray localRay;
    localRay.origin = worldToLocal.Transform(ray.origin);
    localRay.dir = worldToLocal.TransformNormal(ray.dir);
    //localRay.dir.Normalize();

    if (!renderObjectDef.mesh->GetAABB().IntersectRay(localRay, hitDist)) {
        return false;
    }

    if (!renderObjectDef.mesh->IntersectRay(localRay, backFaceCull, hitDist)) {
        return false;
    }

    return true;
}

void ComRenderable::SetWireframeColor(const Color4 &color) {
    renderObjectDef.wireframeColor = color;

    UpdateVisuals();
}

void ComRenderable::SetWireframeMode(RenderObject::WireframeMode::Enum wireframeMode) {
    renderObjectDef.wireframeMode = wireframeMode;

    UpdateVisuals();
}

void ComRenderable::LayerChanged(const Entity *entity) {
    renderObjectDef.layer = entity->GetLayer();
    
    UpdateVisuals();
}

void ComRenderable::StaticMaskChanged(const Entity *entity) {
    renderObjectDef.staticMask = entity->GetStaticMask();

    UpdateVisuals();
}

void ComRenderable::TransformUpdated(const ComTransform *transform) {
    renderObjectDef.worldMatrix = transform->GetMatrix();

    UpdateVisuals();
}

float ComRenderable::GetMaxVisDist() const {
    return renderObjectDef.maxVisDist;
}

void ComRenderable::SetMaxVisDist(float maxVisDist) {
    renderObjectDef.maxVisDist = maxVisDist;

    UpdateVisuals();
}

Color3 ComRenderable::GetColor() const {
    return Color3(&renderObjectDef.materialParms[RenderObject::MaterialParm::Red]);
}

void ComRenderable::SetColor(const Color3 &color) {
    renderObjectDef.materialParms[RenderObject::MaterialParm::Red] = color.r;
    renderObjectDef.materialParms[RenderObject::MaterialParm::Green] = color.g;
    renderObjectDef.materialParms[RenderObject::MaterialParm::Blue] = color.b;
    
    UpdateVisuals();
}

float ComRenderable::GetAlpha() const {
    return renderObjectDef.materialParms[RenderObject::MaterialParm::Alpha];
}

void ComRenderable::SetAlpha(float alpha) {
    renderObjectDef.materialParms[RenderObject::MaterialParm::Alpha] = alpha;

    UpdateVisuals();
}

float ComRenderable::GetTimeOffset() const {
    return renderObjectDef.materialParms[RenderObject::MaterialParm::TimeOffset];
}

void ComRenderable::SetTimeOffset(float timeOffset) {
    renderObjectDef.materialParms[RenderObject::MaterialParm::TimeOffset] = timeOffset;

    UpdateVisuals();
}

float ComRenderable::GetTimeScale() const {
    return renderObjectDef.materialParms[RenderObject::MaterialParm::TimeScale];
}

void ComRenderable::SetTimeScale(float timeScale) {
    renderObjectDef.materialParms[RenderObject::MaterialParm::TimeScale] = timeScale;

    UpdateVisuals();
}

bool ComRenderable::IsBillboard() const {
    return !!(renderObjectDef.flags & RenderObject::Flag::Billboard);
}

void ComRenderable::SetBillboard(bool billboard) {
    if (billboard) {
        renderObjectDef.flags |= RenderObject::Flag::Billboard;
    } else {
        renderObjectDef.flags &= ~RenderObject::Flag::Billboard;
    }

    UpdateVisuals();
}

bool ComRenderable::IsSkipSelection() const {
    return !!(renderObjectDef.flags & RenderObject::Flag::SkipSelection);
}

void ComRenderable::SetSkipSelection(bool skip) {
    if (skip) {
        renderObjectDef.flags |= RenderObject::Flag::SkipSelection;
    } else {
        renderObjectDef.flags &= ~RenderObject::Flag::SkipSelection;
    }

    UpdateVisuals();
}

void ComRenderable::SetRenderingOrder(int order) {
    renderObjectDef.flags |= RenderObject::Flag::UseRenderingOrder;
    renderObjectDef.renderingOrder = order;

    UpdateVisuals();
}

BE_NAMESPACE_END
