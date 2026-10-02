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

static constexpr const char *meshPath = "Data/Test/House/House_scene_01.bmesh ";

void SceneStaticMesh::Init(RenderWorld *renderWorld, BE1::PhysicsWorld *physicsWorld) {
    this->renderWorld = renderWorld;

    renderWorld->SetSkyboxMaterial(materialManager.defaultSkyboxMaterial);

    app.SetCameraPitchYaw(15.0f, -120.0f);
    app.SetCameraDistance(5.5f);

    EnvironmentLight::Desc envLightDesc;
    envLightDesc.intensity = 0.1f;
    if (renderWorld->AddEnvironmentLight(envLightDesc)) {
        renderWorld->RequestEnvironmentLightCubemapRegeneration(BE1::Vec3::origin);
    }

    testMesh = meshManager.GetMesh(meshPath, false);
    if (!testMesh || !testMesh->IsReady() || testMesh == meshManager.defaultMesh) {
        BE_WARNLOG("SceneStaticMesh::Init: failed to load '%s'", meshPath);
        testMesh.reset();
        return;
    }

    RenderObject::Desc meshDesc;
    meshDesc.meshInstance = testMesh->GetStaticInstance();
    if (!meshDesc.meshInstance) {
        BE_WARNLOG("SceneStaticMesh::Init: failed to instantiate '%s'", meshPath);
        testMesh.reset();
        return;
    }

    const BE1::AABB &meshAABB = testMesh->GetAABB();
    meshDesc.flags = RenderObject::Flag::Static | RenderObject::Flag::CastShadows;
    meshDesc.worldMatrix.SetTranslation(-meshAABB.Center());
    meshDesc.aabb = meshAABB;

    int materialCount = 1;
    const int numSurfaces = testMesh->NumSurfaces();
    for (int surfaceIndex = 0; surfaceIndex < numSurfaces; ++surfaceIndex) {
        const int materialIndex = testMesh->GetSurface(surfaceIndex)->materialIndex;
        if (materialIndex >= 0 && materialIndex < RenderObject::MaxMaterials) {
            materialCount = BE1::Max(materialCount, materialIndex + 1);
        }
    }
    meshDesc.materials.SetCount(materialCount);
    for (int materialIndex = 0; materialIndex < materialCount; ++materialIndex) {
        meshDesc.materials[materialIndex] = materialManager.whiteLitMaterial;
    }

    renderWorld->AddRenderObject(meshDesc);

    pointLight.type = RenderLight::Type::Point;
    pointLight.flags = RenderLight::Flag::CastShadows;
    pointLight.origin.Set(0.217f, -0.181f, 0.017f);
    pointLight.range = 3.0f;
    pointLight.linearColor = BE1::Color3(1.0f, 0.8f, 0.19f);
    pointLight.intensity = 1.0f;
    pointLightEnabled = pointLight.SetEnabled(renderWorld, true);

    spotPitch = 45.0f;
    spotYaw = -20.0f;
    spotLight.type = RenderLight::Type::Spot;
    spotLight.flags = RenderLight::Flag::CastShadows;
    spotLight.origin.Set(-3.5f, 2.0f, 1.5f);
    spotLight.axis = BE1::Angles(0.0f, spotPitch, spotYaw).ToMat3();
    spotLight.range = 5.0f;
    spotLight.innerConeAngle = 10.0f;
    spotLight.outerConeAngle = 20.0f;
    spotLight.linearColor = BE1::Color3(1.0f, 0.5f, 0.2f);
    spotLight.intensity = 30.0f;
    spotLightEnabled = spotLight.SetEnabled(renderWorld, true);
}

void SceneStaticMesh::Shutdown() {
    if (renderWorld) {
        renderWorld->ClearScene();
    }
    pointLight.renderLightHandle = -1;
    pointLightEnabled = false;
    spotLight.renderLightHandle = -1;
    spotLightEnabled = false;
    testMesh.reset();
    renderWorld = nullptr;
}

void SceneStaticMesh::Update(uint64_t elapsedUsec) {
    (void)elapsedUsec;
}

#ifdef ENABLE_IMGUI
static void BuildLightControls(RenderWorld *renderWorld, GameLight &light, bool &enabled, float *pitch = nullptr, float *yaw = nullptr) {
    const float itemWidth = ImGui::GetFontSize() * 17.0f;

    if (ImGui::Checkbox("Enabled", &enabled)) {
        enabled = light.SetEnabled(renderWorld, enabled);
    }

    ImGui::BeginDisabled(!enabled);

    bool lightingChanged = false;
    bool transformChanged = false;
    bool shapeChanged = false;

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::DragFloat3("Position", light.origin.Ptr(), BE1::Max(light.range * 0.002f, 0.001f), 0.0f, 0.0f, "%.3f")) {
        transformChanged = true;
    }

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::DragFloat("Range", &light.range, BE1::Max(light.range * 0.002f, 0.001f), RenderLight::MinLightSize, 10000.0f, "%.3f m", ImGuiSliderFlags_AlwaysClamp)) {
        shapeChanged = true;
    }

    if (light.type == RenderLight::Type::Spot) {
        assert(pitch && yaw);
        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::SliderFloat("Pitch", pitch, -90.0f, 90.0f, "%.1f deg", ImGuiSliderFlags_AlwaysClamp)) {
            transformChanged = true;
        }
        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::SliderFloat("Yaw", yaw, -180.0f, 180.0f, "%.1f deg", ImGuiSliderFlags_AlwaysClamp)) {
            transformChanged = true;
        }
        if (transformChanged) {
            light.axis = BE1::Angles(0.0f, *pitch, *yaw).ToMat3();
        }

        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::SliderFloat("Outer Cone Angle", &light.outerConeAngle, RenderLight::MinSpotOuterConeAngle, RenderLight::MaxSpotOuterConeAngle, "%.1f deg", ImGuiSliderFlags_AlwaysClamp)) {
            light.innerConeAngle = BE1::Min(light.innerConeAngle, light.outerConeAngle - RenderLight::MinSpotConeAngleGap);
            shapeChanged = true;
        }
        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::SliderFloat("Inner Cone Angle", &light.innerConeAngle, 0.0f, light.outerConeAngle - RenderLight::MinSpotConeAngleGap, "%.1f deg", ImGuiSliderFlags_AlwaysClamp)) {
            shapeChanged = true;
        }
        ImGui::TextDisabled("Cone angles are measured from the forward axis.");
    }

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::ColorEdit3("Color", light.linearColor.Ptr(), ImGuiColorEditFlags_Float)) {
        lightingChanged = true;
    }

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::DragFloat("Intensity", &light.intensity, BE1::Max(light.intensity * 0.01f, 0.01f), 0.0f, 100000.0f, "%.3f cd", ImGuiSliderFlags_AlwaysClamp)) {
        lightingChanged = true;
    }

    if (transformChanged) {
        renderWorld->UpdateRenderLightTransform(light.renderLightHandle, light.origin, light.axis);
    }

    if (shapeChanged) {
        renderWorld->UpdateRenderLightShape(light.renderLightHandle, light.type, light.range, light.zNear, light.innerConeAngle, light.outerConeAngle);
    }

    if (lightingChanged) {
        renderWorld->UpdateRenderLightLighting(light.renderLightHandle, light.linearColor, light.intensity);
    }

    ImGui::EndDisabled();
}

void SceneStaticMesh::BuildImGui() {
    ImGui::TextWrapped("Mesh: %s", meshPath);
    if (!testMesh) {
        ImGui::TextDisabled("Mesh is unavailable.");
    }

    ImGui::BeginDisabled(!testMesh);

    ImGui::Separator();
    ImGui::Text("Point Light");
    ImGui::PushID("PointLight");
    BuildLightControls(renderWorld, pointLight, pointLightEnabled);
    ImGui::PopID();

    ImGui::Separator();
    ImGui::Text("Spot Light");
    ImGui::PushID("SpotLight");
    BuildLightControls(renderWorld, spotLight, spotLightEnabled, &spotPitch, &spotYaw);
    ImGui::PopID();

    ImGui::EndDisabled();
}
#endif
