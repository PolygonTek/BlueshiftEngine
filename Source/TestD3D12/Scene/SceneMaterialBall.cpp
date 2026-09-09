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
#include "Asset/GuidMapper.h"
#include "Render/Frontend/EnvironmentLight.h"
#include "Render/Frontend/RenderWorld.h"
#include "Render/Material/Material.h"
#include "Render/Material/MaterialManager.h"
#include "Render/Mesh/MeshManager.h"
#include "Render/Mesh/MeshInstance.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"
#include "Scene/SceneMaterialBall.h"

#ifdef ENABLE_IMGUI
#include "imgui/imgui.h"
#endif

void SceneMaterialBall::Init(RenderWorld *renderWorld, BE1::PhysicsWorld *physicsWorld) {
    this->renderWorld = renderWorld;
    this->physicsWorld = physicsWorld;

    renderWorld->SetSkyboxMaterial(materialManager.defaultSkyboxMaterial);

    app.SetCameraPitchYaw(25, 30);
    app.SetCameraDistance(5.0f);

    indirectLightingEnabled = true;
    directionalLightEnabled = false;

    testMaterial = materialManager.GetMaterial("Source/TestD3D12/Materials/materialBall.material");
    litShaderTemplate = shaderTemplateManager.GetShaderTemplate("Source/TestD3D12/Shaders/Lit");
    litDirectShaderTemplate = shaderTemplateManager.GetShaderTemplate("Source/TestD3D12/Shaders/LitDirect");

    testMesh = meshManager.GetMesh("Data/Test/MaterialBall.bmesh", false);

    materialBall.worldMatrix.SetTranslationRotation(BE1::Vec3(0, 0, -testMesh->GetAABB().Center().z), BE1::Mat3::FromRotationZ(DEG2RAD(180.0f)), false);

    RenderObject::Desc materialBallDesc;
    materialBallDesc.worldMatrix = materialBall.worldMatrix;
    materialBallDesc.meshInstance = testMesh->GetStaticInstance();
    materialBallDesc.aabb = testMesh->GetAABB();
    materialBallDesc.materials.SetCount(1);
    materialBallDesc.materials[0] = testMaterial;
    materialBall.renderObjectHandle = renderWorld->AddRenderObject(materialBallDesc);

    directionalLight.type = RenderLight::Type::Directional;
    directionalLight.flags = RenderLight::Flag::None;
    directionalLight.linearColor = BE1::Color3::white;
    directionalLight.intensity = 2.0f;
    UpdateDirectionalLightDirection();

    EnvironmentLight::Desc envLightDesc;
    envLightAdded = renderWorld->AddEnvironmentLight(envLightDesc);
    if (envLightAdded) {
        renderWorld->RequestEnvironmentLightCubemapRegeneration(BE1::Vec3::origin);
    }
}

void SceneMaterialBall::Shutdown() {
    if (!renderWorld) {
        physicsWorld = nullptr;
        return;
    }

    renderWorld->ClearScene();
    materialBall.renderObjectHandle = -1;
    directionalLight.renderLightHandle = -1;
    envLightAdded = false;

    testMesh.reset();
    testMaterial.reset();
    litShaderTemplate.reset();
    litDirectShaderTemplate.reset();

    physicsWorld = nullptr;
    renderWorld = nullptr;
}

void SceneMaterialBall::Update(uint64_t elapsedUsec) {
    PROFILER_CPU_SCOPED_EVENT("SceneMaterialBall::Update");

    (void)elapsedUsec;
}

void SceneMaterialBall::UpdateDirectionalLightDirection() {
    // The UI describes the direction toward the sun or moon, while axis[0] is the direction in which light rays travel.
    directionalLight.axis = BE1::Angles(0.0f, directionalLightElevation, directionalLightAzimuth + 180.0f).ToMat3();
}

void SceneMaterialBall::UpdateEnvironmentLightRotation() {
    const EnvironmentLight *envLight = renderWorld->GetEnvironmentLight();
    if (!envLight) {
        return;
    }

    EnvironmentLight::Desc envLightDesc = envLight->GetDesc();
    envLightDesc.axis = BE1::Mat3::FromRotationZ(DEG2RAD(envLightRotation));
    renderWorld->UpdateEnvironmentLight(envLightDesc);
}

#ifdef ENABLE_IMGUI
static bool IsMaterialPropertyVisible(Material *material, const ShaderPropertyInfo &propertyInfo) {
    for (int conditionIndex = 0; conditionIndex < propertyInfo.conditions.Count(); ++conditionIndex) {
        const ShaderPropertyInfo::Condition &condition = propertyInfo.conditions[conditionIndex];
        const ShaderTemplate::PropertyValue *conditionValue = material->GetPropertyValue(condition.name.c_str());
        if (!conditionValue ||
            conditionValue->GetType() != BE1::Variant::Type::Int ||
            !condition.valueList.Find(conditionValue->As<int>())) {
            return false;
        }
    }

    return true;
}

static void DrawMaterialProperties(Material *material, float itemWidth) {
    const SharedPtr<ShaderTemplate> &shaderTemplate = material->GetShaderTemplate();
    if (!shaderTemplate) {
        return;
    }

    const BE1::Array<ShaderPropertyInfo> &properties = shaderTemplate->GetProperties();
    for (int propertyIndex = 0; propertyIndex < properties.Count(); ++propertyIndex) {
        const ShaderPropertyInfo &propertyInfo = properties[propertyIndex];
        if (propertyInfo.IsHidden() || !IsMaterialPropertyVisible(material, propertyInfo)) {
            continue;
        }

        const ShaderTemplate::PropertyValue *propertyValue = material->GetPropertyValue(propertyIndex);
        if (!propertyValue) {
            continue;
        }

        ImGui::PushID(propertyInfo.GetName());

        switch (propertyInfo.GetType()) {
        case BE1::Variant::Type::Bool: {
            bool value = propertyValue->As<bool>();
            if (ImGui::Checkbox(propertyInfo.GetLabel(), &value)) {
                material->SetPropertyValue(propertyIndex, value);
            }
            break;
        }
        case BE1::Variant::Type::Int: {
            int value = propertyValue->As<int>();
            const BE1::Array<BE1::Str> &enumValues = propertyInfo.GetEnum();
            ImGui::SetNextItemWidth(itemWidth);
            if (enumValues.Count() > 0) {
                const char *previewValue = value >= 0 && value < enumValues.Count() ? enumValues[value].c_str() : "";
                if (ImGui::BeginCombo(propertyInfo.GetLabel(), previewValue)) {
                    for (int enumIndex = 0; enumIndex < enumValues.Count(); ++enumIndex) {
                        const bool selected = value == enumIndex;
                        if (ImGui::Selectable(enumValues[enumIndex].c_str(), selected)) {
                            material->SetPropertyValue(propertyIndex, enumIndex);
                        }
                        if (selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            } else if (propertyInfo.IsRanged()) {
                if (ImGui::SliderInt(propertyInfo.GetLabel(), &value, static_cast<int>(propertyInfo.GetMinValue()), static_cast<int>(propertyInfo.GetMaxValue()))) {
                    material->SetPropertyValue(propertyIndex, value);
                }
            } else if (ImGui::InputInt(propertyInfo.GetLabel(), &value)) {
                material->SetPropertyValue(propertyIndex, value);
            }
            break;
        }
        case BE1::Variant::Type::Float: {
            float value = propertyValue->As<float>();
            ImGui::SetNextItemWidth(itemWidth);
            const bool changed = propertyInfo.IsRanged()
                ? ImGui::SliderFloat(propertyInfo.GetLabel(), &value, propertyInfo.GetMinValue(), propertyInfo.GetMaxValue(), "%.3f")
                : ImGui::InputFloat(propertyInfo.GetLabel(), &value);
            if (changed) {
                material->SetPropertyValue(propertyIndex, value);
            }
            break;
        }
        case BE1::Variant::Type::Color3: {
            BE1::Color3 color = propertyValue->As<BE1::Color3>();
            ImGui::SetNextItemWidth(itemWidth);
            if (ImGui::ColorEdit3(propertyInfo.GetLabel(), color.Ptr(), ImGuiColorEditFlags_Float)) {
                material->SetPropertyValue(propertyIndex, color);
            }
            break;
        }
        case BE1::Variant::Type::Color4: {
            BE1::Color4 color = propertyValue->As<BE1::Color4>();
            ImGui::SetNextItemWidth(itemWidth);
            if (ImGui::ColorEdit4(propertyInfo.GetLabel(), color.Ptr(), ImGuiColorEditFlags_Float)) {
                material->SetPropertyValue(propertyIndex, color);
            }
            break;
        }
        case BE1::Variant::Type::Guid: {
            const BE1::Guid textureGuid = propertyValue->As<BE1::Guid>();
            BE1::Str texturePath = textureGuid.IsZero() ? "" : BE1::resourceGuidMapper.Get(textureGuid);
            if (texturePath.IsEmpty()) {
                texturePath = "<none>";
            }

            ImGui::SetNextItemWidth(itemWidth);
            ImGui::InputText(propertyInfo.GetLabel(), &texturePath[0], texturePath.Length() + 1,
                ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_ElideLeft);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
                ImGui::SetTooltip("%s", texturePath.c_str());
            }
            break;
        }
        default:
            break;
        }

        ImGui::PopID();
    }
}

void SceneMaterialBall::BuildImGui() {
    const float itemWidth = ImGui::GetFontSize() * 17.0f;

    ImGui::Text("Materials");

    if (testMaterial) {
        if (ImGui::Checkbox("Indirect Lighting", &indirectLightingEnabled)) {
            const SharedPtr<ShaderTemplate> &shaderTemplate = indirectLightingEnabled ? litShaderTemplate : litDirectShaderTemplate;
            if (!testMaterial->SetShaderTemplate(shaderTemplate)) {
                indirectLightingEnabled = !indirectLightingEnabled;
            }
        }

        DrawMaterialProperties(testMaterial.get(), itemWidth);
    } else {
        ImGui::TextDisabled("Material is unavailable.");
    }

    ImGui::Separator();
    ImGui::Text("Directional Light");

    if (ImGui::Checkbox("Enabled##DirectionalLight", &directionalLightEnabled)) {
        if (directionalLightEnabled) {
            RenderLight::Desc desc;
            desc.type = directionalLight.type;
            desc.flags = directionalLight.flags;
            desc.layer = directionalLight.layer;
            desc.origin = directionalLight.origin;
            desc.axis = directionalLight.axis;
            desc.size = directionalLight.size;
            desc.zNear = directionalLight.zNear;
            desc.projectionPolicy = RHI::renderer->GetProjectionPolicy();
            desc.linearColor = directionalLight.linearColor;
            desc.intensity = directionalLight.intensity;
            desc.fallOffExponent = directionalLight.fallOffExponent;
            desc.shadowOffsetFactor = directionalLight.shadowOffsetFactor;
            desc.shadowOffsetUnits = directionalLight.shadowOffsetUnits;
            directionalLight.renderLightHandle = renderWorld->AddRenderLight(desc);
        } else if (directionalLight.renderLightHandle >= 0) {
            renderWorld->RemoveRenderLight(directionalLight.renderLightHandle);
            directionalLight.renderLightHandle = -1;
        }
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

    ImGui::Separator();
    ImGui::Text("Environment Light");

    ImGui::BeginDisabled(!envLightAdded);
    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::SliderFloat("Rotation##EnvironmentLight", &envLightRotation, 0.0f, 360.0f, "%.1f deg")) {
        UpdateEnvironmentLightRotation();
    }
    ImGui::EndDisabled();
}
#endif
