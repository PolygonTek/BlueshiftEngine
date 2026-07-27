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

    testMesh = meshManager.GetMesh("Data/Test/MaterialBall.bmesh", false);

    RenderObject::Desc &materialBallDesc = materialBall.renderObjectDesc;
    materialBallDesc.worldMatrix.SetTranslationRotation(BE1::Vec3(0, 0, -testMesh->GetAABB().Center().z), BE1::Mat3::FromRotationZ(DEG2RAD(180.0f)), false);
    materialBallDesc.meshInstance = testMesh->GetStaticInstance();
    materialBallDesc.aabb = testMesh->GetAABB();
    materialBallDesc.materials.SetCount(1);
    testMaterial = materialManager.GetMaterial("Source/TestD3D12/Materials/materialBall.material");
    litShaderTemplate = shaderTemplateManager.GetShaderTemplate("Source/TestD3D12/Shaders/Lit");
    litDirectShaderTemplate = shaderTemplateManager.GetShaderTemplate("Source/TestD3D12/Shaders/LitDirect");
    indirectLightingEnabled = true;
    materialBallDesc.materials[0] = testMaterial;
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
    testMesh.reset();
    testMaterial.reset();
    litShaderTemplate.reset();
    litDirectShaderTemplate.reset();

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

void ScenePbr::UpdateEnvironmentLightRotation() {
    const EnvironmentLight *environmentLight = renderWorld->GetEnvironmentLight();
    if (!environmentLight) {
        return;
    }

    EnvironmentLight::Desc environmentLightDesc = environmentLight->GetDesc();
    environmentLightDesc.axis = BE1::Mat3::FromRotationZ(DEG2RAD(environmentLightRotation));
    renderWorld->UpdateEnvironmentLight(environmentLightDesc);
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

void ScenePbr::BuildImGui() {
    const float itemWidth = ImGui::GetFontSize() * 17.0f;

    ImGui::Text("Material Ball");

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

    ImGui::Separator();
    ImGui::Text("Environment Light");

    ImGui::BeginDisabled(!environmentLightAdded);
    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::SliderFloat("Rotation##EnvironmentLight", &environmentLightRotation, 0.0f, 360.0f, "%.1f deg")) {
        UpdateEnvironmentLightRotation();
    }
    ImGui::EndDisabled();
}
#endif
