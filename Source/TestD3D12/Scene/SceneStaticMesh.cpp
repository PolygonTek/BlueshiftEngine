// Copyright(c) 2017 POLYGONTEK
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "App.h"
#include "Render/Frontend/EnvironmentLight.h"
#include "Render/Frontend/RenderWorld.h"
#include "Render/Material/MaterialManager.h"
#include "Render/Mesh/MeshInstance.h"
#include "Render/Mesh/MeshManager.h"
#include "Scene/SceneStaticMesh.h"

#ifdef ENABLE_IMGUI
#include "imgui/imgui.h"
#endif

static constexpr const char *meshPath = "Data/Test/Museum/hintze-hall_UV_pack01.bmesh ";

void SceneStaticMesh::Init(RenderWorld *renderWorld, BE1::PhysicsWorld *physicsWorld) {
    this->renderWorld = renderWorld;

    renderWorld->SetSkyboxMaterial(materialManager.defaultSkyboxMaterial);

    app.SetCameraPitchYaw(25.0f, 30.0f);
    app.SetCameraDistance(5.0f);

    directionalLight.type = RenderLight::Type::Directional;
    directionalLight.flags = RenderLight::Flag::CastShadows;
    directionalLight.linearColor = BE1::Color3::white;
    directionalLight.intensity = 2.0f;
    UpdateDirectionalLightDirection();
    directionalLightEnabled = directionalLight.SetEnabled(renderWorld, false);

    EnvironmentLight::Desc envLightDesc;
    if (renderWorld->AddEnvironmentLight(envLightDesc)) {
        renderWorld->RequestEnvironmentLightCubemapRegeneration(BE1::Vec3::origin);
    }

    testMesh = meshManager.GetMesh(meshPath, false);
    if (!testMesh || !testMesh->IsReady() || testMesh == meshManager.defaultMesh) {
        BE_WARNLOG("SceneStaticMesh::Init: failed to load '%s'", meshPath);
        testMesh.reset();
        return;
    }

    RenderObject::Desc desc;
    desc.meshInstance = testMesh->GetStaticInstance();
    if (!desc.meshInstance) {
        BE_WARNLOG("SceneStaticMesh::Init: failed to instantiate '%s'", meshPath);
        testMesh.reset();
        return;
    }

    const BE1::AABB &meshAABB = testMesh->GetAABB();
    desc.flags = RenderObject::Flag::Static | RenderObject::Flag::CastShadows;
    desc.worldMatrix.SetTranslation(-meshAABB.Center());
    desc.aabb = meshAABB;

    int materialCount = 1;
    const int numSurfaces = testMesh->NumSurfaces();
    for (int surfaceIndex = 0; surfaceIndex < numSurfaces; ++surfaceIndex) {
        const int materialIndex = testMesh->GetSurface(surfaceIndex)->materialIndex;
        if (materialIndex >= 0 && materialIndex < RenderObject::MaxMaterials) {
            materialCount = BE1::Max(materialCount, materialIndex + 1);
        }
    }
    desc.materials.SetCount(materialCount);
    for (int materialIndex = 0; materialIndex < materialCount; ++materialIndex) {
        desc.materials[materialIndex] = materialManager.whiteLitMaterial;
    }

    renderWorld->AddRenderObject(desc);

    app.SetCameraDistance(BE1::Max(1.0f, meshAABB.Extents().Length() * 3.5f));
}

void SceneStaticMesh::Shutdown() {
    if (renderWorld) {
        renderWorld->ClearScene();
    }
    directionalLight.renderLightHandle = -1;
    directionalLightEnabled = false;
    testMesh.reset();
    renderWorld = nullptr;
}

void SceneStaticMesh::Update(uint64_t elapsedUsec) {
    (void)elapsedUsec;
}

void SceneStaticMesh::UpdateDirectionalLightDirection() {
    // The UI describes the direction toward the sun or moon, while axis[0] is the direction in which light rays travel.
    directionalLight.axis = BE1::Angles(0.0f, directionalLightElevation, directionalLightAzimuth + 180.0f).ToMat3();
}

#ifdef ENABLE_IMGUI
void SceneStaticMesh::BuildImGui() {
    const float itemWidth = ImGui::GetFontSize() * 17.0f;

    ImGui::TextWrapped("Mesh: %s", meshPath);
    if (!testMesh) {
        ImGui::TextDisabled("Mesh is unavailable.");
    }

    ImGui::Separator();
    ImGui::Text("Directional Light");

    if (ImGui::Checkbox("Enabled##DirectionalLight", &directionalLightEnabled)) {
        directionalLightEnabled = directionalLight.SetEnabled(renderWorld, directionalLightEnabled);
    }

    ImGui::BeginDisabled(!directionalLightEnabled);
    ImGui::TextDisabled("Azimuth: 0 = +X, 90 = +Y; Elevation: 0 = horizon");

    bool directionalLightLightingChanged = false;
    bool directionalLightDirectionChanged = false;

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::SliderFloat("Azimuth##DirectionalLight", &directionalLightAzimuth, 0.0f, 360.0f, "%.1f deg")) {
        directionalLightDirectionChanged = true;
    }

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::SliderFloat("Elevation##DirectionalLight", &directionalLightElevation, -90.0f, 90.0f, "%.1f deg")) {
        directionalLightDirectionChanged = true;
    }

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::ColorEdit3("Color##DirectionalLight", directionalLight.linearColor.Ptr(), ImGuiColorEditFlags_Float)) {
        directionalLightLightingChanged = true;
    }

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::SliderFloat("Intensity##DirectionalLight", &directionalLight.intensity, 0.0f, 10.0f, "%.3f")) {
        directionalLightLightingChanged = true;
    }

    if (directionalLightDirectionChanged) {
        UpdateDirectionalLightDirection();
        renderWorld->UpdateRenderLightTransform(directionalLight.renderLightHandle, directionalLight.origin, directionalLight.axis);
    }

    if (directionalLightLightingChanged) {
        renderWorld->UpdateRenderLightLighting(directionalLight.renderLightHandle, directionalLight.linearColor, directionalLight.intensity, directionalLight.fallOffExponent);
    }

    ImGui::EndDisabled();
}
#endif
