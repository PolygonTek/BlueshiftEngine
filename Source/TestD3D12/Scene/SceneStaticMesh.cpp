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

    const float meshRadius = BE1::Max(meshAABB.Extents().Length(), RenderLight::MinLightSize);

    pointLight.type = RenderLight::Type::Point;
    pointLight.flags = RenderLight::Flag::CastShadows;
    pointLight.linearColor = BE1::Color3(1.0f, 0.8f, 0.19f);
    pointLight.intensity = 2.0f;
    pointLight.size.Set(2.0f, 2.0f, 2.0f);
    pointLight.zNear = BE1::Max(meshRadius * 0.01f, RenderLight::MinLightSize);
    pointLight.origin.Set(0.217f, -0.181f, 0.017f);
    pointLightEnabled = pointLight.SetEnabled(renderWorld, true);
}

void SceneStaticMesh::Shutdown() {
    if (renderWorld) {
        renderWorld->ClearScene();
    }
    pointLight.renderLightHandle = -1;
    pointLightEnabled = false;
    testMesh.reset();
    renderWorld = nullptr;
}

void SceneStaticMesh::Update(uint64_t elapsedUsec) {
    (void)elapsedUsec;
}

#ifdef ENABLE_IMGUI
void SceneStaticMesh::BuildImGui() {
    const float itemWidth = ImGui::GetFontSize() * 17.0f;

    ImGui::TextWrapped("Mesh: %s", meshPath);
    if (!testMesh) {
        ImGui::TextDisabled("Mesh is unavailable.");
    }

    ImGui::Separator();
    ImGui::Text("Point Light");
    ImGui::BeginDisabled(!testMesh);

    if (ImGui::Checkbox("Enabled##PointLight", &pointLightEnabled)) {
        pointLightEnabled = pointLight.SetEnabled(renderWorld, pointLightEnabled);
    }

    ImGui::BeginDisabled(!pointLightEnabled);

    bool pointLightLightingChanged = false;
    bool pointLightTransformChanged = false;
    bool pointLightShapeChanged = false;
    bool pointLightShadowChanged = false;

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::DragFloat3("Position##PointLight", pointLight.origin.Ptr(), BE1::Max(pointLight.size.x * 0.002f, 0.001f), 0.0f, 0.0f, "%.3f")) {
        pointLightTransformChanged = true;
    }

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::DragFloat("Radius##PointLight", &pointLight.size.x, BE1::Max(pointLight.size.x * 0.002f, 0.001f), RenderLight::MinLightSize, 10000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
        pointLight.size.Set(pointLight.size.x, pointLight.size.x, pointLight.size.x);
        pointLightShapeChanged = true;
    }

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::ColorEdit3("Color##PointLight", pointLight.linearColor.Ptr(), ImGuiColorEditFlags_Float)) {
        pointLightLightingChanged = true;
    }

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::SliderFloat("Intensity##PointLight", &pointLight.intensity, 0.0f, 10.0f, "%.3f")) {
        pointLightLightingChanged = true;
    }

    if (pointLightTransformChanged) {
        renderWorld->UpdateRenderLightTransform(pointLight.renderLightHandle, pointLight.origin, pointLight.axis);
    }

    if (pointLightShapeChanged) {
        renderWorld->UpdateRenderLightShape(pointLight.renderLightHandle, pointLight.type, pointLight.size, pointLight.zNear);
    }

    if (pointLightLightingChanged) {
        renderWorld->UpdateRenderLightLighting(pointLight.renderLightHandle, pointLight.linearColor, pointLight.intensity);
    }

    if (pointLightShadowChanged) {
        renderWorld->UpdateRenderLightShadow(pointLight.renderLightHandle, pointLight.shadowOffsetUnits, pointLight.shadowOffsetFactor);
    }

    ImGui::EndDisabled();
    ImGui::EndDisabled();
}
#endif
