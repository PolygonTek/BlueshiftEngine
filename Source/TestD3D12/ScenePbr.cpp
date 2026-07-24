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
#include "Render/Mesh/MeshManager.h"
#include "Render/Mesh/MeshInstance.h"
#include "ScenePbr.h"

#ifdef ENABLE_IMGUI
#include "imgui/imgui.h"
#endif

void ScenePbr::Init(RenderWorld *renderWorld) {
    assert(!this->renderWorld);
    assert(renderWorld);

    this->renderWorld = renderWorld;
    renderWorld->SetSkyboxMaterial(materialManager.defaultSkyboxMaterial);
    app.SetCameraDistance(10.0f);

    materialBallMesh = meshManager.GetMesh("Data/Test/MaterialBall.bmesh", false);

    RenderObject::Desc &materialBallDesc = materialBall.renderObjectDesc;
    materialBallDesc.worldMatrix.SetTranslationRotation(BE1::Vec3(0, 0, -materialBallMesh->GetAABB().Center().z), BE1::Mat3::FromRotationZ(DEG2RAD(180.0f)), false);
    materialBallDesc.meshInstance = materialBallMesh->GetStaticInstance();
    materialBallDesc.aabb = materialBallMesh->GetAABB();
    materialBallDesc.materials.SetCount(1);
    materialBallDesc.materials[0] = materialManager.GetSingleTextureMaterial("Data/Test/Brick_Wall_Worn.dds");
    materialBall.renderObjectHandle = renderWorld->AddRenderObject(materialBallDesc);

    RenderLight::Desc &directionalLightDesc = directionalLight.renderLightDesc;
    directionalLightDesc.type = RenderLight::Type::Directional;
    directionalLightDesc.flags = RenderLight::Flag::None;
    directionalLightDesc.linearColor = BE1::Color3::white;
    directionalLightDesc.intensity = 2.0f;
    directionalLightDesc.projectionPolicy = RHI::renderer->GetProjectionPolicy();
    directionalLightEnabled = true;
    UpdateDirectionalLightDirection();
    directionalLight.renderLightHandle = renderWorld->AddRenderLight(directionalLightDesc);

    EnvironmentLight::Desc environmentLightDesc;
    environmentLightAdded = renderWorld->AddEnvironmentLight(environmentLightDesc);
    if (environmentLightAdded) {
        renderWorld->RequestEnvironmentLightCubemapRegeneration(BE1::Vec3::origin);
    }
}

void ScenePbr::Shutdown() {
    if (!renderWorld) {
        return;
    }

    if (materialBall.renderObjectHandle >= 0) {
        renderWorld->RemoveRenderObject(materialBall.renderObjectHandle);
        materialBall.renderObjectHandle = -1;
    }
    materialBallMesh.reset();

    if (directionalLight.renderLightHandle >= 0) {
        renderWorld->RemoveRenderLight(directionalLight.renderLightHandle);
        directionalLight.renderLightHandle = -1;
    }

    if (environmentLightAdded) {
        renderWorld->RemoveEnvironmentLight();
        environmentLightAdded = false;
    }

    renderWorld = nullptr;
}

void ScenePbr::Update(uint64_t elapsedUsec) {
    PROFILER_CPU_SCOPED_EVENT("ScenePbr::Update");
}

void ScenePbr::UpdateDirectionalLightDirection() {
    // The UI describes the direction toward the sun or moon, while axis[0] is the direction in which light rays travel.
    directionalLight.renderLightDesc.axis = BE1::Angles(0.0f, directionalLightElevation, directionalLightAzimuth + 180.0f).ToMat3();
}

#ifdef ENABLE_IMGUI
void ScenePbr::BuildImGui() {
    const float itemWidth = ImGui::GetFontSize() * 10.0f;

    ImGui::Text("Directional Light");

    if (ImGui::Checkbox("Enabled##DirectionalLight", &directionalLightEnabled)) {
        if (directionalLightEnabled) {
            directionalLight.renderLightHandle = renderWorld->AddRenderLight(directionalLight.renderLightDesc);
        } else if (directionalLight.renderLightHandle >= 0) {
            renderWorld->RemoveRenderLight(directionalLight.renderLightHandle);
            directionalLight.renderLightHandle = -1;
        }
    }

    ImGui::BeginDisabled(!directionalLightEnabled);
    ImGui::TextDisabled("Azimuth: 0 = +X, 90 = +Y; Elevation: 0 = horizon");

    bool directionalLightChanged = false;
    bool directionalLightDirectionChanged = false;

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::SliderFloat("Azimuth##DirectionalLight", &directionalLightAzimuth, 0.0f, 360.0f, "%.1f deg")) {
        directionalLightDirectionChanged = true;
    }

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::SliderFloat("Elevation##DirectionalLight", &directionalLightElevation, -90.0f, 90.0f, "%.1f deg")) {
        directionalLightDirectionChanged = true;
    }

    RenderLight::Desc &directionalLightDesc = directionalLight.renderLightDesc;

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::ColorEdit3("Color##DirectionalLight", directionalLightDesc.linearColor.Ptr(), ImGuiColorEditFlags_Float)) {
        directionalLightChanged = true;
    }

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::SliderFloat("Intensity##DirectionalLight", &directionalLightDesc.intensity, 0.0f, 10.0f, "%.3f")) {
        directionalLightChanged = true;
    }

    if (directionalLightDirectionChanged) {
        UpdateDirectionalLightDirection();
        directionalLightChanged = true;
    }

    if (directionalLightChanged) {
        renderWorld->UpdateRenderLight(directionalLight.renderLightHandle, directionalLightDesc, RenderLight::Desc::UpdateFlag::Transform);
    }

    ImGui::EndDisabled();
}
#endif
