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
        "The color to use when rendering.", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("alpha", "Alpha", float, GetAlpha, SetAlpha, 1.f, 
        "The alpha to use when rendering.", PropertyInfo::EditorFlag).SetRange(0, 1, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("billboard", "Billboard", bool, IsBillboard, SetBillboard, false, 
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("timeOffset", "Time Offset", float, GetTimeOffset, SetTimeOffset, 0.f, 
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("timeScale", "Time Scale", float, GetTimeScale, SetTimeScale, 1.f, 
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("maxVisDist", "Max Visible Distance", float, GetMaxVisDist, SetMaxVisDist, 1000.f, 
        "", PropertyInfo::SystemUnits | PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("skipSelection", "Skip Selection", bool, IsSkipSelection, SetSkipSelection, false, 
        "", PropertyInfo::SkipSerializationFlag);
}

ComRenderable::ComRenderable() {
    renderObjectHandle = -1;
    memset(&renderObjectDef, 0, sizeof(renderObjectDef));
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
    renderObjectDef.wireframeColor.Set(1, 1, 1, 1);

    ComTransform *transform = GetEntity()->GetTransform();
    renderObjectDef.origin = transform->GetOrigin();
    renderObjectDef.scale = transform->GetScale();
    renderObjectDef.axis = transform->GetAxis();

    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComRenderable::TransformUpdated, SignalObject::Unique);

    GetEntity()->Connect(&Entity::SIG_LayerChanged, this, (SignalCallback)&ComRenderable::LayerChanged, SignalObject::Unique);
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

bool ComRenderable::HasRenderEntity(int renderObjectHandle) const { 
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
    return renderWorld->GetViewCount() - renderObject->viewCount <= 1;
}

const AABB ComRenderable::GetAABB() {
    const ComTransform *transform = GetEntity()->GetTransform();
    return renderObjectDef.localAABB * transform->GetScale();
}

bool ComRenderable::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    if (!renderObjectDef.mesh) {
        return false;
    }

    Vec3 localStart = renderObjectDef.axis.TransposedMulVec(start - renderObjectDef.origin) / renderObjectDef.scale;
    Vec3 localDir = renderObjectDef.axis.TransposedMulVec(dir) / renderObjectDef.scale;
    localDir.Normalize();

    float localS = renderObjectDef.mesh->GetAABB().RayIntersection(localStart, localDir);
    if (localS == FLT_MAX) {
        return false;
    }

    float s = localS * (localDir * renderObjectDef.scale).Length();
    if (s > lastScale) {
        return false;
    }

    if (renderObjectDef.mesh->RayIntersection(localStart, localDir, backFaceCull, localS)) {
        s = localS * (localDir * renderObjectDef.scale).Length();
        if (s > 0.0f && s < lastScale) {
            lastScale = s;
            return true;
        }
    }

    return false;
}

void ComRenderable::SetWireframeColor(const Color4 &color) {
    renderObjectDef.wireframeColor = color;

    UpdateVisuals();
}

void ComRenderable::SetWireframeMode(RenderObject::WireframeMode wireframeMode) {
    renderObjectDef.wireframeMode = wireframeMode;

    UpdateVisuals();
}

void ComRenderable::LayerChanged(const Entity *entity) {
    renderObjectDef.layer = entity->GetProperty("layer").As<int>();
    
    UpdateVisuals();
}

void ComRenderable::TransformUpdated(const ComTransform *transform) {
    renderObjectDef.origin = transform->GetOrigin();
    renderObjectDef.axis = transform->GetAxis();
    renderObjectDef.scale = transform->GetScale();

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
    return Color3(&renderObjectDef.materialParms[RenderObject::RedParm]);
}

void ComRenderable::SetColor(const Color3 &color) {
    renderObjectDef.materialParms[RenderObject::RedParm] = color.r;
    renderObjectDef.materialParms[RenderObject::GreenParm] = color.g;
    renderObjectDef.materialParms[RenderObject::BlueParm] = color.b;
    
    UpdateVisuals();
}

float ComRenderable::GetAlpha() const {
    return renderObjectDef.materialParms[RenderObject::AlphaParm];
}

void ComRenderable::SetAlpha(float alpha) {
    renderObjectDef.materialParms[RenderObject::AlphaParm] = alpha;

    UpdateVisuals();
}

float ComRenderable::GetTimeOffset() const {
    return renderObjectDef.materialParms[RenderObject::TimeOffsetParm];
}

void ComRenderable::SetTimeOffset(float timeOffset) {
    renderObjectDef.materialParms[RenderObject::TimeOffsetParm] = timeOffset;

    UpdateVisuals();
}

float ComRenderable::GetTimeScale() const {
    return renderObjectDef.materialParms[RenderObject::TimeScaleParm];
}

void ComRenderable::SetTimeScale(float timeScale) {
    renderObjectDef.materialParms[RenderObject::TimeScaleParm] = timeScale;

    UpdateVisuals();
}

bool ComRenderable::IsBillboard() const {
    return !!(renderObjectDef.flags & RenderObject::BillboardFlag);
}

void ComRenderable::SetBillboard(bool billboard) {
    if (billboard) {
        renderObjectDef.flags |= RenderObject::BillboardFlag;
    } else {
        renderObjectDef.flags &= ~RenderObject::BillboardFlag;
    }

    UpdateVisuals();
}

bool ComRenderable::IsSkipSelection() const {
    return !!(renderObjectDef.flags & RenderObject::SkipSelectionFlag);
}

void ComRenderable::SetSkipSelection(bool skip) {
    if (skip) {
        renderObjectDef.flags |= RenderObject::SkipSelectionFlag;
    } else {
        renderObjectDef.flags &= ~RenderObject::SkipSelectionFlag;
    }

    UpdateVisuals();
}

BE_NAMESPACE_END
