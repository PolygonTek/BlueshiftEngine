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
#include "Render/Frontend/HeightFog.h"
#include "Render/Frontend/LocalFogVolume.h"
#include "Render/Frontend/RenderWorld.h"
#include "Render/Material/MaterialManager.h"
#include "Render/Mesh/MeshInstance.h"
#include "Render/Mesh/MeshManager.h"
#include "Render/Texture/TextureManager.h"
#include "Scene/SceneShadows.h"

#ifdef ENABLE_IMGUI
#include "imgui/imgui.h"
#endif

static constexpr const char *meshPath = "Data/Test/House/House_scene_01.bmesh ";
static constexpr const char *fogDensityNoisePath = "Data/Test/Volume/3DTexture_CheckerPride.btex";

void SceneShadows::Init(RenderWorld *renderWorld, BE1::PhysicsWorld *physicsWorld) {
    this->renderWorld = renderWorld;

    EnvironmentLight::Desc envLightDesc;
    envLightDesc.intensity = 0.025f;
    renderWorld->SetSkyboxMaterial(materialManager.defaultSkyboxMaterial);
    renderWorld->SetSkyboxLighting(envLightDesc.linearColor, envLightDesc.intensity);
    renderWorld->SetSkyboxRotation(envLightDesc.axis);

    app.SetCameraPitchYaw(15.0f, -120.0f);
    app.SetCameraDistance(5.5f);

    if (renderWorld->AddEnvironmentLight(envLightDesc)) {
        renderWorld->RequestEnvironmentLightCubemapRegeneration(BE1::Vec3::origin);
    }

    HeightFog::Desc fogDesc;
    fogDesc.baseHeight = 0.0f;
    fogDesc.heightFalloff = 0.1f;
    fogDesc.medium.anisotropy = 0.6f;
    renderWorld->AddHeightFog(fogDesc);

    testMesh = meshManager.GetMesh(meshPath, false);
    if (!testMesh || !testMesh->IsReady() || testMesh == meshManager.missingMesh) {
        BE_WARNLOG("SceneShadows::Init: failed to load '%s'", meshPath);
        testMesh.reset();
        return;
    }

    RenderObject::Desc meshDesc;
    meshDesc.meshInstance = testMesh->GetStaticInstance();
    if (!meshDesc.meshInstance) {
        BE_WARNLOG("SceneShadows::Init: failed to instantiate '%s'", meshPath);
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
    pointLight.flags = RenderLight::Flag::CastShadows | RenderLight::Flag::CastVolumetricShadows | RenderLight::Flag::StaticShadows;
    pointLight.origin.Set(0.217f, -0.181f, 0.017f);
    pointLight.range = 3.0f;
    pointLight.shadowOffsetUnits = 500.0f;
    pointLight.shadowOffsetFactor = 5.0f;
    pointLight.linearColor = BE1::Color3(1.0f, 0.8f, 0.19f);
    pointLight.intensity = 1.0f;
    pointLightEnabled = pointLight.SetEnabled(renderWorld, true);

    spotPitch = 25.0f;
    spotYaw = -25.0f;
    spotLight.type = RenderLight::Type::Spot;
    spotLight.flags = RenderLight::Flag::CastShadows | RenderLight::Flag::CastVolumetricShadows | RenderLight::Flag::StaticShadows;
    spotLight.origin.Set(-2.8f, 1.9f, 0.0f);
    spotLight.axis = BE1::Angles(0.0f, spotPitch, spotYaw).ToMat3();
    spotLight.range = 5.0f;
    spotLight.innerConeAngle = 30.0f;
    spotLight.outerConeAngle = 45.0f;
    spotLight.shadowOffsetUnits = 500.0f;
    spotLight.shadowOffsetFactor = 5.0f;
    spotLight.linearColor = BE1::Color3(0.2f, 0.45f, 1.0f);
    spotLight.intensity = 15.0f;
    spotLightEnabled = spotLight.SetEnabled(renderWorld, true);

    fogDensityNoiseTexture = textureManager.GetTexture(fogDensityNoisePath, Texture::Flag::HighQuality, false);
    if (!fogDensityNoiseTexture || fogDensityNoiseTexture == textureManager.missingTexture ||
        !fogDensityNoiseTexture->IsReady() || fogDensityNoiseTexture->GetType() != RHI::TextureType::Texture3D) {
        BE_WARNLOG("SceneShadows::Init: failed to load 3D density mask '%s'\n", fogDensityNoisePath);
        fogDensityNoiseTexture.reset();
    }

    LocalFogVolume::Desc boxFogDesc;
    boxFogDesc.shape = LocalFogVolume::Shape::Box;
    boxFogDesc.origin = BE1::Vec3(-1.0f, -0.25f, 0.0f);
    boxFogDesc.halfExtents = BE1::Vec3(0.5f, 0.5f, 0.5f);
    boxFogDesc.boundaryFade = 0.1f;
    boxFogDesc.medium.baseExtinction = 10.0f;
    boxFogDesc.medium.scatteringAlbedo = float3(1.0f, 1.0f, 1.0f);
    boxFogDesc.medium.anisotropy = 0.15f;
    boxFogDesc.maskTexture = fogDensityNoiseTexture;
    boxFogDesc.maskScrollSpeed = BE1::Vec3(0.4f, 0.1f, 0.0f);
    boxFogDesc.heightFogFalloff = 0.0f;
    boxFogDesc.heightFogOffset = 0.0f;
    boxFogHandle = renderWorld->AddLocalFogVolume(boxFogDesc);
}

void SceneShadows::Shutdown() {
    if (renderWorld) {
        renderWorld->ClearScene();
    }
    pointLight.renderLightHandle = -1;
    pointLightEnabled = false;
    spotLight.renderLightHandle = -1;
    spotLightEnabled = false;
    boxFogHandle = -1;
    fogDensityNoiseTexture.reset();
    testMesh.reset();
    renderWorld = nullptr;
}

void SceneShadows::Update(uint64_t elapsedUsec) {
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
    bool shadowChanged = false;

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
    }

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::ColorEdit3("Color", light.linearColor.Ptr(), ImGuiColorEditFlags_Float)) {
        lightingChanged = true;
    }

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::DragFloat("Intensity", &light.intensity, BE1::Max(light.intensity * 0.01f, 0.01f), 0.0f, 100000.0f, "%.3f cd", ImGuiSliderFlags_AlwaysClamp)) {
        lightingChanged = true;
    }

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::DragFloat("Shadow Offset Units", &light.shadowOffsetUnits, 1.0f, 0.0f, 0.0f, "%.0f")) {
        shadowChanged = true;
    }

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::DragFloat("Shadow Offset Factor", &light.shadowOffsetFactor, 0.01f, 0.0f, 0.0f, "%.3f")) {
        shadowChanged = true;
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

    if (shadowChanged) {
        renderWorld->UpdateRenderLightShadow(light.renderLightHandle, light.shadowOffsetUnits, light.shadowOffsetFactor);
    }

    ImGui::EndDisabled();
}

static void BuildBoxFogControls(RenderWorld *renderWorld, int handle, const char *label, const SharedPtr<Texture> &noiseTexture) {
    if (!renderWorld || handle < 0) {
        return;
    }
    const LocalFogVolume *volume = renderWorld->GetLocalFogVolume(handle);
    if (!volume) {
        return;
    }

    ImGui::TextUnformatted(label);
    ImGui::PushID(label);
    LocalFogVolume::Desc desc = volume->GetDesc();
    ImGui::PushItemWidth(ImGui::GetFontSize() * 17.0f);

    bool changed = ImGui::Checkbox("Enabled", &desc.enabled);
    ImGui::BeginDisabled(!desc.enabled);
    changed |= ImGui::DragFloat3("Position", desc.origin.Ptr(), 0.01f, 0.0f, 0.0f, "%.3f m");

    changed |= ImGui::DragFloat3("Half extents", desc.halfExtents.Ptr(), 0.01f, 0.01f, 100.0f, "%.3f m", ImGuiSliderFlags_AlwaysClamp);
    changed |= ImGui::SliderFloat("Boundary fade", &desc.boundaryFade, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("Fraction of the minimum half extent that fades inward from each face. 0 gives a hard edge; 1 uses the full minimum half extent as the fade distance.");

    changed |= ImGui::SliderFloat("Base extinction", &desc.medium.baseExtinction, 0.0f, 1000.0f, "%.4f /m", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("Extinction before boundary fade, height falloff, and mask alpha are applied.");
    changed |= ImGui::ColorEdit3("Scattering albedo", &desc.medium.scatteringAlbedo.x, ImGuiColorEditFlags_Float);
    ImGui::SetItemTooltip("Multiplied by the fog mask RGB. A8 masks keep this color unchanged.");
    changed |= ImGui::SliderFloat("Anisotropy (g)", &desc.medium.anisotropy, -0.9f, 0.9f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("0 is isotropic. Positive values favor forward scattering when looking toward a light.");
    if (ImGui::ColorEdit3("Base emission", &desc.medium.baseEmission.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR)) {
        desc.medium.baseEmission.x = BE1::Max(desc.medium.baseEmission.x, 0.0f);
        desc.medium.baseEmission.y = BE1::Max(desc.medium.baseEmission.y, 0.0f);
        desc.medium.baseEmission.z = BE1::Max(desc.medium.baseEmission.z, 0.0f);
        changed = true;
    }
    ImGui::SetItemTooltip("RGB radiance emitted per meter before boundary fade, height falloff, and mask alpha are applied. Independent of scattering albedo and mask RGB.");

    const int maskIndex = desc.maskTexture ? 1 : 0;
    static constexpr const char *maskNames[] = { "None", "Noise" };
    if (ImGui::BeginCombo("Fog mask", maskNames[maskIndex])) {
        for (int i = 0; i < COUNT_OF(maskNames); ++i) {
            ImGui::BeginDisabled(i == 1 && !noiseTexture);
            if (ImGui::Selectable(maskNames[i], maskIndex == i)) {
                desc.maskTexture = i == 0 ? nullptr : noiseTexture;
                changed = true;
            }
            ImGui::EndDisabled();
            if (maskIndex == i) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("Linear 3D mask: RGB multiplies scattering albedo; alpha multiplies density. A8 changes density only. None disables masking. Noise uses a tileable A8 density pattern.");
    if (!noiseTexture) {
        ImGui::TextDisabled("Noise texture is unavailable.");
    }
    ImGui::BeginDisabled(!desc.maskTexture);
    changed |= ImGui::DragFloat3("Mask tiling", desc.maskTiling.Ptr(), 0.05f, -16.0f, 16.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("Repetitions across the full volume along local X, Y, Z. Negative values mirror an axis; zero removes variation along that axis.");
    changed |= ImGui::DragFloat3("Mask scroll speed", desc.maskScrollSpeed.Ptr(), 0.005f, -10.0f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("Texture cycles per second along local X, Y, Z. With positive tiling, positive speed moves the pattern along the positive local axis. Zero pauses that axis.");
    ImGui::EndDisabled();

    changed |= ImGui::SliderFloat("Height fog falloff", &desc.heightFogFalloff, 0.0f, 200.0f, "%.3f /m", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetItemTooltip("Above the reference plane, density halves every 1 / falloff meters upward along world Z. Density stays constant below it. 0 disables height falloff.");
    changed |= ImGui::DragFloat("Height fog offset", &desc.heightFogOffset, 0.01f, 0.0f, 0.0f, "%.3f m");
    ImGui::SetItemTooltip("Height where density starts decreasing, relative to the volume center along world Z. Positive values raise the reference plane.");
    ImGui::EndDisabled();

    if (changed) {
        renderWorld->UpdateLocalFogVolume(handle, desc);
    }
    ImGui::PopItemWidth();
    ImGui::PopID();
}

void SceneShadows::BuildImGui() {
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

    if (const HeightFog *fog = renderWorld ? renderWorld->GetHeightFog() : nullptr) {
        HeightFog::Desc desc = fog->GetDesc();
        ImGui::Separator();
        ImGui::TextUnformatted("Height Fog");
        ImGui::PushID("HeightFog");
        ImGui::PushItemWidth(ImGui::GetFontSize() * 17.0f);

        bool changed = ImGui::Checkbox("Enabled", &desc.enabled);
        ImGui::BeginDisabled(!desc.enabled);
        changed |= ImGui::DragFloat("Base height", &desc.baseHeight, 0.1f, 0.0f, 0.0f, "%.2f m");
        ImGui::SetItemTooltip("World Z height at which base extinction and base emission are defined.");
        changed |= ImGui::SliderFloat("Height falloff", &desc.heightFalloff, 0.0f, 10.0f, "%.3f /m", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp);
        ImGui::SetItemTooltip("Density halves for every 1 / falloff meters above the base height and increases below it. Zero gives a uniform medium.");
        changed |= ImGui::SliderFloat("Base extinction", &desc.medium.baseExtinction, 0.0f, 1000.0f, "%.4f /m", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp);
        ImGui::SetItemTooltip("Extinction at the base height, before height falloff.");
        changed |= ImGui::DragFloat("Max distance", &desc.maxDistance, 10.0f, 0.01f, 1000000.0f, "%.1f m", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SetItemTooltip("Maximum fog ray distance for surfaces and sky. Independent of the froxel far depth.");
        changed |= ImGui::ColorEdit3("Scattering albedo", &desc.medium.scatteringAlbedo.x, ImGuiColorEditFlags_Float);
        changed |= ImGui::SliderFloat("Anisotropy (g)", &desc.medium.anisotropy, -0.9f, 0.9f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SetItemTooltip("0 is isotropic. Positive values favor forward scattering when looking toward a light.");
        if (ImGui::ColorEdit3("Base emission", &desc.medium.baseEmission.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR)) {
            desc.medium.baseEmission.x = BE1::Max(desc.medium.baseEmission.x, 0.0f);
            desc.medium.baseEmission.y = BE1::Max(desc.medium.baseEmission.y, 0.0f);
            desc.medium.baseEmission.z = BE1::Max(desc.medium.baseEmission.z, 0.0f);
            changed = true;
        }
        ImGui::SetItemTooltip("RGB radiance emitted per meter at the base height. Scales with height falloff and is independent of scattering albedo.");
        changed |= ImGui::ColorEdit3("Ambient color", desc.ambientColor.Ptr(), ImGuiColorEditFlags_Float);
        changed |= ImGui::DragFloat("Ambient intensity", &desc.ambientIntensity, 0.05f, 0.0f, 32.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SetItemTooltip("Uniform ambient incident radiance for height fog. Scattering follows extinction, height falloff, and albedo.");
        ImGui::EndDisabled();

        if (changed) {
            renderWorld->UpdateHeightFog(desc);
        }
        ImGui::PopItemWidth();
        ImGui::PopID();
    }

    ImGui::Separator();
    BuildBoxFogControls(renderWorld, boxFogHandle, "Box Fog Volume", fogDensityNoiseTexture);
}
#endif
