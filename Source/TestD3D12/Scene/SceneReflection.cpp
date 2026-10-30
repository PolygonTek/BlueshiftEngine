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
#include "Scene/SceneReflection.h"

#ifdef ENABLE_IMGUI
#include "imgui/imgui.h"
#endif

static constexpr float HoverHeight = 0.0f;
static constexpr uint64_t RotationPeriodUsec = 8000000; // 45 degrees per second.
static const BE1::Vec3 ObjectScales[] = { BE1::Vec3(1.8f), BE1::Vec3(1.6f), BE1::Vec3(1.5f, 1.5f, 1.0f), BE1::Vec3(1.8f) };

static SharedPtr<Material> CreateReflectionMaterial(const char *name, const BE1::Color3 &color, float roughness, float metallic, bool twoSided = false) {
    // A previous render frame may still retain these materials when the scene is reopened.
    SharedPtr<Material> material = materialManager.FindMaterial(name);
    if (material) {
        return material;
    }

    char text[512];
    BE1::Str::snPrintf(text, sizeof(text),
        "{ %s shaderTemplate \"Source/TestD3D12/Shaders/Lit\" properties { "
        "_BASECOLOR \"0\" baseColorFactor \"%f %f %f 1\" "
        "_ROUGHNESS \"0\" roughnessFactor \"%f\" "
        "_METALLIC \"0\" metallicFactor \"%f\" _NORMAL \"0\" } }",
        twoSided ? "cull twoSided" : "", color.r, color.g, color.b, roughness, metallic);
    material = materialManager.AllocMaterial(name);
    if (!material->Create(text, nullptr)) {
        BE_WARNLOG("SceneReflection::Init: failed to create material '%s'", name);
        return materialManager.whiteLitMaterial;
    }
    return material;
}

void SceneReflection::Init(RenderWorld *renderWorld, BE1::PhysicsWorld *physicsWorld) {
    this->renderWorld = renderWorld;
    (void)physicsWorld;

    previousElapsedUsec = UINT64_MAX;
    rotationUsec = 0;
    rotateObjects = true;

    EnvironmentLight::Desc envLightDesc;
    envLightDesc.intensity = 0.35f;
    renderWorld->SetSkyboxMaterial(materialManager.defaultSkyboxMaterial);
    renderWorld->SetSkyboxLighting(envLightDesc.linearColor, envLightDesc.intensity);
    renderWorld->SetSkyboxRotation(envLightDesc.axis);
    app.SetCameraPitchYaw(8.0f, 0.0f);
    app.SetCameraDistance(12.0f);

    planeMesh = meshManager.GetMesh("_defaultPlaneMesh");
    primitiveMeshes[0] = meshManager.GetMesh("_defaultSphereMesh");
    primitiveMeshes[1] = meshManager.GetMesh("_defaultBoxMesh");
    primitiveMeshes[2] = meshManager.GetMesh("_defaultCylinderMesh");
    primitiveMeshes[3] = meshManager.GetMesh("_defaultQuadMesh");

    planeMaterial = CreateReflectionMaterial("_reflectionPlaneMaterial", BE1::Color3(0.5f), 0.25f, 1.0f);
    const BE1::Vec3 planeSize = planeMesh->GetAABB()[1] - planeMesh->GetAABB()[0];
    RenderObject::Desc planeDesc;
    planeDesc.flags = RenderObject::Flag::Static | RenderObject::Flag::ReceiveShadows;
    planeDesc.worldMatrix.SetTRS(BE1::Vec3::origin, BE1::Mat3::identity, BE1::Vec3(30.0f / planeSize.x, 30.0f / planeSize.y, 1.0f));
    planeDesc.meshInstance = planeMesh->GetStaticInstance();
    planeDesc.aabb = planeMesh->GetAABB();
    planeDesc.materials.SetCount(1);
    planeDesc.materials[0] = planeMaterial;
    renderWorld->AddRenderObject(planeDesc);

    static const BE1::Color3 colors[ObjectCount] = {
        { 0.95f, 0.03f, 0.06f }, { 0.02f, 0.65f, 0.2f }, { 0.04f, 0.15f, 0.95f }, { 0.95f, 0.8f, 0.03f }
    };

    for (int objectIndex = 0; objectIndex < ObjectCount; ++objectIndex) {
        const SharedPtr<Mesh> &mesh = primitiveMeshes[objectIndex];
        char materialName[64];
        BE1::Str::snPrintf(materialName, sizeof(materialName), "_reflectionObjectMaterial%i", objectIndex);
        // Keep the zero-thickness quad visible from either side while orbiting the scene or rotating objects.
        const bool twoSided = objectIndex == 3;
        objectMaterials[objectIndex] = CreateReflectionMaterial(materialName, colors[objectIndex], 1.0f, 0.0f, twoSided);

        RenderObject::Desc desc;
        desc.flags = RenderObject::Flag::CastShadows | RenderObject::Flag::ReceiveShadows;
        const float originZ = HoverHeight - mesh->GetAABB()[0].z * ObjectScales[objectIndex].z;
        desc.worldMatrix.SetTRS(BE1::Vec3(0.0f, (objectIndex - (ObjectCount - 1) * 0.5f) * 3.2f, originZ), BE1::Mat3::identity, ObjectScales[objectIndex]);
        desc.meshInstance = mesh->GetStaticInstance();
        desc.aabb = mesh->GetAABB();
        desc.materials.SetCount(1);
        desc.materials[0] = objectMaterials[objectIndex];
        objects[objectIndex].worldMatrix = desc.worldMatrix;
        objects[objectIndex].renderObjectHandle = renderWorld->AddRenderObject(desc);
    }

    directionalLight.type = RenderLight::Type::Directional;
    directionalLight.flags = RenderLight::Flag::CastShadows;
    directionalLight.linearColor = BE1::Color3::white;
    directionalLight.intensity = 3.0f;
    directionalLightEnabled = true;
    UpdateDirectionalLightDirection();
    directionalLightEnabled = directionalLight.SetEnabled(renderWorld, directionalLightEnabled);

    if (renderWorld->AddEnvironmentLight(envLightDesc)) {
        renderWorld->RequestEnvironmentLightCubemapRegeneration(BE1::Vec3::origin);
    }
}

void SceneReflection::Shutdown() {
    if (renderWorld) {
        renderWorld->ClearScene();
        renderWorld = nullptr;
    }
    directionalLight.renderLightHandle = -1;
    directionalLightEnabled = false;
    for (auto &object : objects) {
        object.renderObjectHandle = -1;
    }
    planeMesh.reset();
    planeMaterial.reset();
    for (auto &mesh : primitiveMeshes) {
        mesh.reset();
    }
    for (auto &material : objectMaterials) {
        material.reset();
    }
}

void SceneReflection::Update(uint64_t elapsedUsec) {
    const uint64_t frameUsec = elapsedUsec > previousElapsedUsec ? elapsedUsec - previousElapsedUsec : 0;
    previousElapsedUsec = elapsedUsec;
    if (!renderWorld || !rotateObjects || frameUsec == 0) {
        return;
    }

    rotationUsec = (rotationUsec + frameUsec) % RotationPeriodUsec;
    const float angle = BE1::Math::TwoPi * (static_cast<float>(rotationUsec) / static_cast<float>(RotationPeriodUsec));
    const BE1::Mat3 axis = BE1::Mat3::FromRotationZ(angle);

    for (int objectIndex = 0; objectIndex < ObjectCount; ++objectIndex) {
        GameObject &object = objects[objectIndex];
        object.worldMatrix.SetTRS(object.worldMatrix.ToTranslationVec3(), axis, ObjectScales[objectIndex]);
        renderWorld->UpdateRenderObjectTransform(object.renderObjectHandle, object.worldMatrix);
    }
}

void SceneReflection::UpdateDirectionalLightDirection() {
    // The UI describes the direction toward the sun or moon, while axis[0] is the direction in which light rays travel.
    directionalLight.axis = BE1::Angles(0.0f, directionalLightElevation, directionalLightAzimuth + 180.0f).ToMat3();
}

#ifdef ENABLE_IMGUI
void SceneReflection::BuildImGui() {
    const float itemWidth = ImGui::GetFontSize() * 17.0f;

    ImGui::Text("SSR reflection test");
    ImGui::Checkbox("Rotate Objects", &rotateObjects);

    static constexpr Material::RenderingMode renderingModes[] = {
        Material::RenderingMode::Opaque, Material::RenderingMode::AlphaCutoff, Material::RenderingMode::Dithered, Material::RenderingMode::Blend
    };
    int renderingModeIndex = 0;
    for (const auto &material : objectMaterials) {
        if (material && material != materialManager.whiteLitMaterial) {
            const Material::RenderingMode renderingMode = material->GetRenderingMode();
            for (int modeIndex = 0; modeIndex < COUNT_OF(renderingModes); ++modeIndex) {
                if (renderingModes[modeIndex] == renderingMode) {
                    renderingModeIndex = modeIndex;
                    break;
                }
            }
            break;
        }
    }
    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::Combo("Rendering Mode", &renderingModeIndex, "Opaque\0AlphaCutoff\0Dithered\0Blend\0")) {
        for (const auto &material : objectMaterials) {
            if (material && material != materialManager.whiteLitMaterial) {
                material->SetRenderingMode(renderingModes[renderingModeIndex]);
            }
        }
    }

    static const char *const alphaLabels[ObjectCount] = { "Sphere Alpha", "Box Alpha", "Cylinder Alpha", "Quad Alpha" };
    static const char *const fadeLabels[ObjectCount] = { "Sphere Fade Opacity", "Box Fade Opacity", "Cylinder Fade Opacity", "Quad Fade Opacity" };
    for (int objectIndex = 0; objectIndex < ObjectCount; ++objectIndex) {
        const int renderObjectHandle = objects[objectIndex].renderObjectHandle;
        const RenderObject *renderObject = renderWorld && renderObjectHandle >= 0 ? renderWorld->GetRenderObject(renderObjectHandle) : nullptr;
        if (renderObject) {
            float fadeOpacity = renderObject->GetFadeOpacity();
            ImGui::SetNextItemWidth(itemWidth);
            if (ImGui::SliderFloat(fadeLabels[objectIndex], &fadeOpacity, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
                renderWorld->UpdateRenderObjectFadeOpacity(renderObjectHandle, fadeOpacity);
            }
        }

        const auto &material = objectMaterials[objectIndex];
        if (!material || material == materialManager.whiteLitMaterial) {
            continue;
        }

        const int baseColorPropertyIndex = material->FindPropertyIndex(ShaderTemplate::StandardProperty::BaseColorFactor);
        const ShaderTemplate::PropertyValue *baseColorValue = material->GetPropertyValue(baseColorPropertyIndex);
        if (baseColorValue && baseColorValue->GetType() == BE1::Variant::Type::Color4) {
            BE1::Color4 baseColor = baseColorValue->As<BE1::Color4>();
            ImGui::SetNextItemWidth(itemWidth);
            if (ImGui::SliderFloat(alphaLabels[objectIndex], &baseColor.a, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
                material->SetPropertyValue(baseColorPropertyIndex, baseColor);
            }
        }
    }

    ImGui::Separator();
    ImGui::Text("Floor Material");

    if (planeMaterial && planeMaterial != materialManager.whiteLitMaterial) {
        ImGui::PushID("FloorMaterial");

        const int baseColorPropertyIndex = planeMaterial->FindPropertyIndex(ShaderTemplate::StandardProperty::BaseColorFactor);
        const ShaderTemplate::PropertyValue *baseColorValue = planeMaterial->GetPropertyValue(baseColorPropertyIndex);
        if (baseColorValue && baseColorValue->GetType() == BE1::Variant::Type::Color4) {
            BE1::Color4 baseColor = baseColorValue->As<BE1::Color4>();
            ImGui::SetNextItemWidth(itemWidth);
            if (ImGui::ColorEdit3("Base Color", baseColor.Ptr(), ImGuiColorEditFlags_Float)) {
                planeMaterial->SetPropertyValue(baseColorPropertyIndex, baseColor);
            }
        }

        const int metallicPropertyIndex = planeMaterial->FindPropertyIndex(ShaderTemplate::StandardProperty::MetallicFactor);
        const ShaderTemplate::PropertyValue *metallicValue = planeMaterial->GetPropertyValue(metallicPropertyIndex);
        if (metallicValue && metallicValue->GetType() == BE1::Variant::Type::Float) {
            float metallic = metallicValue->As<float>();
            ImGui::SetNextItemWidth(itemWidth);
            if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
                planeMaterial->SetPropertyValue(metallicPropertyIndex, metallic);
            }
        }

        const int roughnessPropertyIndex = planeMaterial->FindPropertyIndex(ShaderTemplate::StandardProperty::RoughnessFactor);
        const ShaderTemplate::PropertyValue *roughnessValue = planeMaterial->GetPropertyValue(roughnessPropertyIndex);
        if (roughnessValue && roughnessValue->GetType() == BE1::Variant::Type::Float) {
            float roughness = roughnessValue->As<float>();
            ImGui::SetNextItemWidth(itemWidth);
            if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
                planeMaterial->SetPropertyValue(roughnessPropertyIndex, roughness);
            }
        }

        ImGui::PopID();
    } else {
        ImGui::TextDisabled("Floor material is unavailable.");
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
    if (ImGui::DragFloat("Illuminance##DirectionalLight", &directionalLight.intensity, BE1::Max(directionalLight.intensity * 0.01f, 0.01f), 0.0f, 200000.0f, "%.3f lux", ImGuiSliderFlags_AlwaysClamp)) {
        directionalLightLightingChanged = true;
    }

    if (directionalLightDirectionChanged) {
        UpdateDirectionalLightDirection();
        renderWorld->UpdateRenderLightTransform(directionalLight.renderLightHandle, directionalLight.origin, directionalLight.axis);
    }

    if (directionalLightLightingChanged) {
        renderWorld->UpdateRenderLightLighting(directionalLight.renderLightHandle, directionalLight.linearColor, directionalLight.intensity);
    }

    ImGui::EndDisabled();

    if (const EnvironmentLight *envLight = renderWorld ? renderWorld->GetEnvironmentLight() : nullptr) {
        const EnvironmentLight::Desc &currentDesc = envLight->GetDesc();
        bool enabled = currentDesc.enabled;
        float intensity = currentDesc.intensity;
        BE1::Color3 linearColor = currentDesc.linearColor;
        const float envLightItemWidth = ImGui::GetFontSize() * 17.0f;

        ImGui::Separator();
        ImGui::TextUnformatted("Environment Light");
        ImGui::PushID("EnvironmentLight");

        const bool enabledChanged = ImGui::Checkbox("Enabled", &enabled);
        ImGui::BeginDisabled(!enabled);

        ImGui::SetNextItemWidth(envLightItemWidth);
        bool lightingChanged = ImGui::SliderFloat("Intensity", &intensity, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

        ImGui::SetNextItemWidth(envLightItemWidth);
        lightingChanged |= ImGui::ColorEdit3("Color", linearColor.Ptr(), ImGuiColorEditFlags_Float);

        ImGui::SetNextItemWidth(envLightItemWidth);
        const bool rotationChanged = ImGui::SliderFloat("Rotation", &envLightRotation, 0.0f, 360.0f, "%.1f deg", ImGuiSliderFlags_AlwaysClamp);

        ImGui::EndDisabled();
        ImGui::PopID();

        if (enabledChanged || lightingChanged || rotationChanged) {
            EnvironmentLight::Desc desc = currentDesc;
            desc.enabled = enabled;
            desc.intensity = intensity;
            desc.linearColor = linearColor;
            if (rotationChanged) {
                desc.axis = BE1::Mat3::FromRotationZ(DEG2RAD(envLightRotation));
                renderWorld->SetSkyboxRotation(desc.axis);
            }
            renderWorld->UpdateEnvironmentLight(desc);
            if (lightingChanged) {
                renderWorld->SetSkyboxLighting(desc.linearColor, desc.intensity);
            }
        }
    }
}
#endif
