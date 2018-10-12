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
#include "Components/ComReflectionProbe.h"
#include "Game/GameWorld.h"
#include "Game/TagLayerSettings.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Reflection Probe", ComReflectionProbe, Component)
BEGIN_EVENTS(ComReflectionProbe)
END_EVENTS

void ComReflectionProbe::RegisterProperties() {
    /*PROPERTY_INT("importance", "Importance", "", 1, PropertyInfo::EditorFlag),
    PROPERTY_ENUM("resolution", "Resolution", "", "16;32;64;128;256;1024;2048", 3, PropertyInfo::EditorFlag),
    PROPERTY_BOOL("hdr", "HDR", "", true, PropertyInfo::EditorFlag),
    PROPERTY_ENUM("clear", "Clear", "", "Color;Skybox", 1, PropertyInfo::EditorFlag),
    PROPERTY_COLOR3("clearColor", "Clear Color", "", Color3(0, 0, 0), PropertyInfo::EditorFlag),
    PROPERTY_FLOAT("clearAlpha", "Clear Alpha", "", 0.0f, PropertyInfo::EditorFlag),
    PROPERTY_RANGED_FLOAT("near", "Near", "near plane distance", Rangef(0.01, 1000, 10), 0.3f, PropertyInfo::EditorFlag),
    PROPERTY_RANGED_FLOAT("far", "Far", "far plane distance", Rangef(1, 1000, 10), 1000.f, PropertyInfo::EditorFlag),
    PROPERTY_VEC3("boxOffset", "Box Offset", "", Vec3(0, 0, 0), PropertyInfo::EditorFlag),
    PROPERTY_VEC3("boxSize", "Box Size", "", Vec3(10, 10, 10), PropertyInfo::EditorFlag);*/
}

ComReflectionProbe::ComReflectionProbe() {
    sphereHandle = -1;
    sphereMesh = nullptr;
    memset(&sphereDef, 0, sizeof(sphereDef));
}

ComReflectionProbe::~ComReflectionProbe() {
    Purge(false);
}

void ComReflectionProbe::Purge(bool chainPurge) {
    if (sphereDef.mesh) {
        meshManager.ReleaseMesh(sphereDef.mesh);
        sphereDef.mesh = nullptr;
    }

    if (sphereMesh) {
        meshManager.ReleaseMesh(sphereMesh);
        sphereMesh = nullptr;
    }

    if (sphereHandle != -1) {
        renderWorld->RemoveRenderObject(sphereHandle);
        sphereHandle = -1;
    }

    if (chainPurge) {
        Component::Purge();
    }
}

void ComReflectionProbe::Init() {
    Component::Init();

    renderWorld = GetGameWorld()->GetRenderWorld();

    ComTransform *transform = GetEntity()->GetTransform();

    // sphereDef
    sphereMesh = meshManager.GetMesh("_defaultSphereMesh");

    memset(&sphereDef, 0, sizeof(sphereDef));
    sphereDef.layer = TagLayerSettings::EditorLayer;
    sphereDef.maxVisDist = MeterToUnit(50.0f);

    Texture *spriteTexture = textureManager.GetTexture("Data/EditorUI/Camera2.png", Texture::Clamp | Texture::HighQuality);
    sphereDef.materials.SetCount(1);
    sphereDef.materials[0] = materialManager.GetSingleTextureMaterial(spriteTexture, Material::SpriteHint);
    textureManager.ReleaseTexture(spriteTexture);

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

    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComReflectionProbe::TransformUpdated, SignalObject::Unique);

    // Mark as initialized
    SetInitialized(true);

    UpdateVisuals();
}

void ComReflectionProbe::OnActive() {
    UpdateVisuals();
}

void ComReflectionProbe::OnInactive() {
    renderWorld->RemoveRenderObject(sphereHandle);
    sphereHandle = -1;
}

bool ComReflectionProbe::HasRenderEntity(int renderEntityHandle) const {
    if (this->sphereHandle == renderEntityHandle) {
        return true;
    }

    return false;
}

bool ComReflectionProbe::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    return false;
}

void ComReflectionProbe::DrawGizmos(const RenderView::State &viewState, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    if (selected) {
        /*OBB cameraBox;
        cameraBox.SetAxis(this->viewParms.axis);
        cameraBox.SetCenter(this->viewParms.origin + this->viewParms.axis[0] * sizeZ);
        cameraBox.SetExtents(Vec3(sizeZ, this->viewParms.sizeX, this->viewParms.sizeY));

        renderWorld->SetDebugColor(Color4(0.5f, 0.5f, 0.5f, 0.5f), Color4::zero);
        renderWorld->DebugOBB(cameraBox, 1.0f, false, false, true);*/
    }
}

const AABB ComReflectionProbe::GetAABB() {
    return Sphere(Vec3::origin, MeterToUnit(0.5f)).ToAABB();
}

void ComReflectionProbe::UpdateVisuals() {
    if (!IsInitialized() || !IsActiveInHierarchy()) {
        return;
    }

    if (sphereHandle == -1) {
        sphereHandle = renderWorld->AddRenderObject(&sphereDef);
    } else {
        renderWorld->UpdateRenderObject(sphereHandle, &sphereDef);
    }
}

void ComReflectionProbe::TransformUpdated(const ComTransform *transform) {
    //viewParms.origin = transform->GetOrigin();
    //viewParms.axis = transform->GetAxis();

    sphereDef.origin = transform->GetOrigin();

    UpdateVisuals();
}

BE_NAMESPACE_END
