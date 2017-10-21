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
#include "Game/GameSettings/TagLayerSettings.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Reflection Probe", ComReflectionProbe, Component)
BEGIN_EVENTS(ComReflectionProbe)
END_EVENTS
BEGIN_PROPERTIES(ComReflectionProbe)
    PROPERTY_INT("importance", "Importance", "", 1, PropertyInfo::ReadWrite),
    PROPERTY_ENUM("resolution", "Resolution", "", "16;32;64;128;256;1024;2048", 3, PropertyInfo::ReadWrite),
    PROPERTY_BOOL("hdr", "HDR", "", true, PropertyInfo::ReadWrite),
    PROPERTY_ENUM("clear", "Clear", "", "Color;Skybox", 1, PropertyInfo::ReadWrite), 
    PROPERTY_COLOR3("clearColor", "Clear Color", "", Color3(0, 0, 0), PropertyInfo::ReadWrite),
    PROPERTY_FLOAT("clearAlpha", "Clear Alpha", "", 0.0f, PropertyInfo::ReadWrite),
    PROPERTY_RANGED_FLOAT("near", "Near", "near plane distance", Rangef(0.01, 20000, 10), 0.3f, PropertyInfo::ReadWrite),
    PROPERTY_RANGED_FLOAT("far", "Far", "far plane distance", Rangef(1, 20000, 10), 1000.f, PropertyInfo::ReadWrite),
    PROPERTY_VEC3("boxOffset", "Box Offset", "", Vec3(0, 0, 0), PropertyInfo::ReadWrite),
    PROPERTY_VEC3("boxSize", "Box Size", "", Vec3(10, 10, 10), PropertyInfo::ReadWrite),
END_PROPERTIES

#ifdef NEW_PROPERTY_SYSTEM
void ComReflectionProbe::RegisterProperties() {
}
#endif

ComReflectionProbe::ComReflectionProbe() {
    sphereHandle = -1;
    sphereMesh = nullptr;
    memset(&sphere, 0, sizeof(sphere));

#ifndef NEW_PROPERTY_SYSTEM
    Connect(&Properties::SIG_PropertyChanged, this, (SignalCallback)&ComReflectionProbe::PropertyChanged);
#endif
}

ComReflectionProbe::~ComReflectionProbe() {
    Purge(false);
}

void ComReflectionProbe::Purge(bool chainPurge) {
    if (sphere.mesh) {
        meshManager.ReleaseMesh(sphere.mesh);
        sphere.mesh = nullptr;
    }

    if (sphereMesh) {
        meshManager.ReleaseMesh(sphereMesh);
        sphereMesh = nullptr;
    }

    if (sphereHandle != -1) {
        renderWorld->RemoveEntity(sphereHandle);
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

    // sphere
    sphereMesh = meshManager.GetMesh("_defaultSphereMesh");

    memset(&sphere, 0, sizeof(sphere));
    sphere.layer = TagLayerSettings::EditorLayer;
    sphere.maxVisDist = MeterToUnit(50);

    Texture *spriteTexture = textureManager.GetTexture("Data/EditorUI/Camera2.png", Texture::Clamp | Texture::HighQuality);
    sphere.materials.SetCount(1);
    sphere.materials[0] = materialManager.GetSingleTextureMaterial(spriteTexture, Material::SpriteHint);
    textureManager.ReleaseTexture(spriteTexture);

    sphere.mesh = sphereMesh->InstantiateMesh(Mesh::StaticMesh);
    sphere.aabb = sphereMesh->GetAABB();
    sphere.origin = transform->GetOrigin();
    sphere.scale = Vec3(1, 1, 1);
    sphere.axis = Mat3::identity;
    sphere.materialParms[SceneEntity::RedParm] = 1.0f;
    sphere.materialParms[SceneEntity::GreenParm] = 1.0f;
    sphere.materialParms[SceneEntity::BlueParm] = 1.0f;
    sphere.materialParms[SceneEntity::AlphaParm] = 1.0f;
    sphere.materialParms[SceneEntity::TimeOffsetParm] = 0.0f;
    sphere.materialParms[SceneEntity::TimeScaleParm] = 1.0f;

    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComReflectionProbe::TransformUpdated, SignalObject::Unique);

    // Mark as initialized
    SetInitialized(true);

    UpdateVisuals();
}

void ComReflectionProbe::SetEnable(bool enable) {
    if (enable) {
        if (!IsEnabled()) {
            UpdateVisuals();
            Component::SetEnable(true);
        }
    } else {
        if (IsEnabled()) {
            renderWorld->RemoveEntity(sphereHandle);
            sphereHandle = -1;
            Component::SetEnable(false);
        }
    }
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

void ComReflectionProbe::DrawGizmos(const SceneView::Parms &viewParms, bool selected) {
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
    return Sphere(Vec3::origin, MeterToUnit(0.5)).ToAABB();
}

void ComReflectionProbe::UpdateVisuals() {
    if (!IsInitialized() || !IsEnabled()) {
        return;
    }

    if (sphereHandle == -1) {
        sphereHandle = renderWorld->AddEntity(&sphere);
    } else {
        renderWorld->UpdateEntity(sphereHandle, &sphere);
    }
}

void ComReflectionProbe::TransformUpdated(const ComTransform *transform) {
    //viewParms.origin = transform->GetOrigin();
    //viewParms.axis = transform->GetAxis();

    sphere.origin = transform->GetOrigin();

    UpdateVisuals();
}

void ComReflectionProbe::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitialized()) {
        return;
    }

    Component::PropertyChanged(classname, propName);
}

BE_NAMESPACE_END
