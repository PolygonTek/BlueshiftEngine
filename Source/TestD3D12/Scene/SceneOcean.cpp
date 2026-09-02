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
#include "Render/RenderSystem.h"
#include "Render/Frontend/EnvironmentLight.h"
#include "Render/Frontend/RenderWorld.h"
#include "Render/Material/MaterialManager.h"
#include "Render/Mesh/MeshInstance.h"
#include "Render/Mesh/MeshManager.h"
#include "Scene/SceneOcean.h"

#ifdef ENABLE_IMGUI
#include "imgui/imgui.h"
#endif

void SceneOcean::Init(RenderWorld *renderWorld) {
    this->renderWorld = renderWorld;

    renderWorld->SetSkyboxMaterial(materialManager.GetMaterial("Source/TestD3D12/Materials/skybox.material"));

    app.SetCameraPitchYaw(10, 145);
    app.SetCameraDistance(150.0f);

    struct BoxInitDesc {
        BE1::Vec2 queryPosition;
        BE1::Vec3 size;
    };
    static const BoxInitDesc boxInitDescs[BoxCount] = {
        { BE1::Vec2(-20.0f, -20.0f), BE1::Vec3(2.0f) },
        { BE1::Vec2(20.0f, -20.0f), BE1::Vec3(2.0f) },
        { BE1::Vec2(20.0f, 20.0f), BE1::Vec3(2.0f) },
        { BE1::Vec2(-20.0f, 20.0f), BE1::Vec3(2.0f) },
        { BE1::Vec2(0.0f, 0.0f), BE1::Vec3(20.0f) }
    };

    boxMesh = meshManager.GetMesh("_defaultBoxMesh");
    boxMaterial = materialManager.GetSingleTextureMaterial("Data/Test/Brick_Wall_Worn.dds");

    for (uint32_t queryId = 0; queryId < BoxCount; ++queryId) {
        const BoxInitDesc &initDesc = boxInitDescs[queryId];
        Box &box = boxes[queryId];
        box.queryPosition = initDesc.queryPosition;
        box.size = initDesc.size;
        box.gameObject.worldMatrix.SetTRS(
            BE1::Vec3(initDesc.queryPosition.x, initDesc.queryPosition.y, oceanDesc.surface.height),
            BE1::Mat3::identity,
            initDesc.size);

        RenderObject::Desc boxDesc;
        boxDesc.worldMatrix = box.gameObject.worldMatrix;
        boxDesc.meshInstance = boxMesh->GetStaticInstance();
        boxDesc.aabb = boxMesh->GetAABB();
        boxDesc.materials.SetCount(1);
        boxDesc.materials[0] = boxMaterial;
        box.gameObject.renderObjectHandle = renderWorld->AddRenderObject(boxDesc);
    }

    oceanAdded = renderWorld->AddOceanSurface(oceanDesc);

    directionalLight.type = RenderLight::Type::Directional;
    directionalLight.flags = RenderLight::Flag::None;
    directionalLight.linearColor = BE1::Color3::white;
    directionalLight.intensity = 2.0f;
    directionalLightElevation = 20;
    UpdateDirectionalLightDirection();

    RenderLight::Desc directionalLightDesc;
    directionalLightDesc.type = directionalLight.type;
    directionalLightDesc.flags = directionalLight.flags;
    directionalLightDesc.layer = directionalLight.layer;
    directionalLightDesc.origin = directionalLight.origin;
    directionalLightDesc.axis = directionalLight.axis;
    directionalLightDesc.size = directionalLight.size;
    directionalLightDesc.zNear = directionalLight.zNear;
    directionalLightDesc.projectionPolicy = RHI::renderer->GetProjectionPolicy();
    directionalLightDesc.linearColor = directionalLight.linearColor;
    directionalLightDesc.intensity = directionalLight.intensity;
    directionalLightDesc.fallOffExponent = directionalLight.fallOffExponent;
    directionalLightDesc.shadowOffsetFactor = directionalLight.shadowOffsetFactor;
    directionalLightDesc.shadowOffsetUnits = directionalLight.shadowOffsetUnits;
    directionalLight.renderLightHandle = renderWorld->AddRenderLight(directionalLightDesc);

    EnvironmentLight::Desc environmentLightDesc;
    environmentLightAdded = renderWorld->AddEnvironmentLight(environmentLightDesc);
    if (environmentLightAdded) {
        renderWorld->RequestEnvironmentLightCubemapRegeneration(BE1::Vec3::origin);
    }
}

void SceneOcean::Shutdown() {
    if (!renderWorld) {
        return;
    }

    for (Box &box : boxes) {
        if (box.gameObject.renderObjectHandle >= 0) {
            renderWorld->RemoveRenderObject(box.gameObject.renderObjectHandle);
            box.gameObject.renderObjectHandle = -1;
        }
    }
    boxMaterial.reset();
    boxMesh.reset();

    if (directionalLight.renderLightHandle >= 0) {
        renderWorld->RemoveRenderLight(directionalLight.renderLightHandle);
        directionalLight.renderLightHandle = -1;
    }

    if (environmentLightAdded) {
        renderWorld->RemoveEnvironmentLight();
        environmentLightAdded = false;
    }

    if (oceanAdded) {
        renderWorld->RemoveOceanSurface();
        oceanAdded = false;
    }

    renderWorld = nullptr;
}

void SceneOcean::Update(uint64_t elapsedUsec) {
    PROFILER_CPU_SCOPED_EVENT("SceneOcean::Update");

    (void)elapsedUsec;
    UpdateSurfaceQueries();
}

void SceneOcean::UpdateSurfaceQueries() {
    if (!oceanAdded) {
        return;
    }

    GpuOcean &gpuOcean = renderSystem->GetGpuOcean();

    for (uint32_t queryId = 0; queryId < BoxCount; ++queryId) {
        OceanSurfaceQueryResult result;
        if (!gpuOcean.GetSurfaceQueryResult(queryId, result)) {
            continue;
        }

        const BE1::Vec3 surfacePosition(result.surfacePosition.x, result.surfacePosition.y, result.surfacePosition.z);
        const BE1::Vec3 surfaceNormal(result.surfaceNormal.x, result.surfaceNormal.y, result.surfaceNormal.z);

        BE1::Vec3 surfaceTangent;
        BE1::Vec3 surfaceBitangent;
        surfaceNormal.OrthogonalBasis(surfaceTangent, surfaceBitangent);
        const BE1::Mat3 surfaceAxis(surfaceTangent, surfaceBitangent, surfaceNormal);

        Box &box = boxes[queryId];
        box.gameObject.worldMatrix.SetTRS(surfacePosition, surfaceAxis, box.size);
        if (box.gameObject.renderObjectHandle >= 0) {
            renderWorld->UpdateRenderObjectTransform(box.gameObject.renderObjectHandle, box.gameObject.worldMatrix);
        }
    }

    OceanSurfaceQueryInput queries[BoxCount] = {};
    for (uint32_t queryId = 0; queryId < BoxCount; ++queryId) {
        const Box &box = boxes[queryId];
        queries[queryId].worldPositionXY = float2(box.queryPosition.x, box.queryPosition.y);
        queries[queryId].sampleDiameter = BE1::Max(box.size.x, box.size.y);
        queries[queryId].queryId = queryId;
    }
    renderWorld->SetOceanSurfaceQueries(queries, BoxCount);
}

void SceneOcean::UpdateDirectionalLightDirection() {
    // UI 에서는 태양이 있는 방향을 나타내지만 axis[0] 은 빛이 진행하는 방향이다.
    directionalLight.axis = BE1::Angles(0.0f, directionalLightElevation, directionalLightAzimuth + 180.0f).ToMat3();
}

#ifdef ENABLE_IMGUI
void SceneOcean::BuildImGui() {
    const OceanSurface *oceanSurface = renderWorld->GetOceanSurface();
    if (!oceanSurface) {
        return;
    }

    const float fontSize = ImGui::GetFontSize();
    const auto roundedCollapsingHeader = [fontSize](const char *label) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, fontSize * 0.25f);
        const bool isOpen = ImGui::CollapsingHeader(label);
        ImGui::PopStyleVar();
        return isOpen;
    };

    OceanSurface::Desc oceanSettings = oceanSurface->GetDesc();
    GpuOcean::SimulationSettings &simulationSettings = oceanSettings.simulation;
    GpuOcean::SurfaceSettings &surfaceSettings = oceanSettings.surface;
    bool oceanSettingsChanged = false;
    bool oceanCascadeSettingsChanged = false;
    const float itemWidth = fontSize * 12.0f;

    ImGui::TextUnformatted("Ocean");

    static const char *fftSizeNames[] = { "256", "512", "1024", "2048" };
    int fftSizeIndex = static_cast<int>(simulationSettings.fftSize);
    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::Combo("FFT size", &fftSizeIndex, fftSizeNames, COUNT_OF(fftSizeNames))) {
        simulationSettings.fftSize = static_cast<GpuFFT::FFTSize>(fftSizeIndex);
        simulationSettings.InitCascades(simulationSettings.cascades[0].patchSize, simulationSettings.cascadeBeta);
        oceanSettingsChanged = true;
    }

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::SliderFloat("Displacement scale", &simulationSettings.displacementScale, 0.0f, 5.0f, "%.3f")) {
        simulationSettings.displacementScale = BE1::Max(simulationSettings.displacementScale, 0.0f);
        oceanSettingsChanged = true;
    }

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::SliderFloat("Choppiness", &simulationSettings.choppiness, 0.0f, 5.0f, "%.3f")) {
        oceanSettingsChanged = true;
    }

    if (ImGui::Checkbox("Use cross derivative (∂Dx/∂y)", &simulationSettings.crossDerivativeEnabled)) {
        oceanSettingsChanged = true;
    }

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::SliderFloat("Time scale", &simulationSettings.timeScale, 0.0f, 10.0f, "%.3f")) {
        oceanSettingsChanged = true;
    }

    if (roundedCollapsingHeader("Spectrum##OceanSpectrumHeader")) {
        static const char *spectrumModelNames[] = { "Phillips", "TMA" };
        int spectrumModelIndex = static_cast<int>(simulationSettings.spectrumModel);
        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::Combo("Spectrum model", &spectrumModelIndex, spectrumModelNames, COUNT_OF(spectrumModelNames))) {
            simulationSettings.spectrumModel = static_cast<GpuOcean::SpectrumModel>(spectrumModelIndex);
            oceanSettingsChanged = true;
        }

        if (simulationSettings.spectrumModel == GpuOcean::SpectrumModel::Phillips) {
            ImGui::SetNextItemWidth(itemWidth);
            if (ImGui::DragFloat("Spectrum scale", &simulationSettings.phillips.spectrumScale, 0.00001f, 0.0f, 0.01f, "%.6f")) {
                simulationSettings.phillips.spectrumScale = BE1::Max(simulationSettings.phillips.spectrumScale, 0.0f);
                oceanSettingsChanged = true;
            }

            ImGui::SetNextItemWidth(itemWidth);
            if (ImGui::SliderFloat("Opposite wave damping", &simulationSettings.phillips.oppositeWaveDamping, 0.0f, 1.0f, "%.3f")) {
                oceanSettingsChanged = true;
            }
        } else {
            ImGui::SetNextItemWidth(itemWidth);
            if (ImGui::DragFloat("Spectrum scale", &simulationSettings.tma.spectrumScale, 0.01f, 0.0f, 100.0f, "%.4f")) {
                simulationSettings.tma.spectrumScale = BE1::Max(simulationSettings.tma.spectrumScale, 0.0f);
                oceanSettingsChanged = true;
            }

            float fetchKilometers = simulationSettings.tma.fetch * 0.001f;
            ImGui::SetNextItemWidth(itemWidth);
            if (ImGui::DragFloat("Fetch", &fetchKilometers, 1.0f, 0.1f, 1000.0f, "%.1f km")) {
                simulationSettings.tma.fetch = BE1::Max(fetchKilometers, 0.1f) * 1000.0f;
                oceanSettingsChanged = true;
            }

            ImGui::SetNextItemWidth(itemWidth);
            if (ImGui::SliderFloat("Peak enhancement (gamma)", &simulationSettings.tma.peakEnhancement, 1.0f, 7.0f, "%.2f")) {
                oceanSettingsChanged = true;
            }

            static const char *directionalSpreadModelNames[] = { "Positive cosine squared", "Hasselmann", "Donelan-Banner", "Mitsuyasu" };
            int directionalSpreadModelIndex = static_cast<int>(simulationSettings.tma.directionalSpreadModel);
            ImGui::SetNextItemWidth(itemWidth);
            if (ImGui::Combo("Directional model", &directionalSpreadModelIndex, directionalSpreadModelNames, COUNT_OF(directionalSpreadModelNames))) {
                simulationSettings.tma.directionalSpreadModel = static_cast<GpuOcean::DirectionalSpreadModel>(directionalSpreadModelIndex);
                oceanSettingsChanged = true;
            }

            if (simulationSettings.tma.directionalSpreadModel == GpuOcean::DirectionalSpreadModel::Hasselmann) {
                ImGui::SetNextItemWidth(itemWidth);
                if (ImGui::SliderFloat("Swell", &simulationSettings.tma.swell, 0.0f, 1.0f, "%.3f")) {
                    oceanSettingsChanged = true;
                }
            }

            ImGui::SetNextItemWidth(itemWidth);
            if (ImGui::SliderFloat("Isotropic mix", &simulationSettings.tma.spread, 0.0f, 1.0f, "%.3f")) {
                oceanSettingsChanged = true;
            }
        }

        float windDirectionDegrees = simulationSettings.windDirection.ToAngle() * BE1::Math::MulRadianToDegree;
        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::SliderFloat("Wind direction (from)", &windDirectionDegrees, -180.0f, 180.0f, "%.1f deg")) {
            simulationSettings.windDirection.SetFromAngle(windDirectionDegrees * BE1::Math::MulDegreeToRadian);
            oceanSettingsChanged = true;
        }

        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::SliderFloat("Wind speed", &simulationSettings.windSpeed, 0.01f, 60.0f, "%.2f m/s")) {
            oceanSettingsChanged = true;
        }

        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::SliderFloat("Gravity", &simulationSettings.gravity, 1.0f, 20.0f, "%.2f m/s^2")) {
            oceanSettingsChanged = true;
        }

        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::DragFloat("Depth", &simulationSettings.depth, 1.0f, 0.01f, 10000.0f, "%.2f m")) {
            simulationSettings.depth = BE1::Max(simulationSettings.depth, 0.01f);
            oceanSettingsChanged = true;
        }

        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::SliderFloat("Small wave damping", &simulationSettings.smallWaveDamping, 0.0f, 0.5f, "%.4f m")) {
            oceanSettingsChanged = true;
        }
    }

    if (roundedCollapsingHeader("Cascades##OceanCascadesHeader")) {
        float largestCascadePatchSize = simulationSettings.cascades[0].patchSize;
        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::DragFloat("Largest cascade patch size", &largestCascadePatchSize, 1.0f, 0.01f, 100000.0f, "%.2f m")) {
            largestCascadePatchSize = BE1::Max(largestCascadePatchSize, 0.01f);
            oceanCascadeSettingsChanged = true;
        }

        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::SliderFloat("Cascade beta", &simulationSettings.cascadeBeta, 0.01f, 1.0f, "%.3f")) {
            oceanCascadeSettingsChanged = true;
        }

        if (oceanCascadeSettingsChanged) {
            simulationSettings.InitCascades(largestCascadePatchSize, simulationSettings.cascadeBeta);
            oceanSettingsChanged = true;
        }

        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::DragFloat("Cascade fade scale", &surfaceSettings.cascadeFadeScale, 0.1f, 0.01f, 100.0f, "%.2f")) {
            surfaceSettings.cascadeFadeScale = BE1::Max(surfaceSettings.cascadeFadeScale, 0.01f);
            oceanSettingsChanged = true;
        }

        const uint32_t seedStep = 1u;
        const uint32_t seedFastStep = 100u;
        for (uint32_t cascadeIndex = 0; cascadeIndex < GpuOcean::CascadeCount; ++cascadeIndex) {
            ImGui::PushID(static_cast<int>(cascadeIndex));
            GpuOcean::CascadeDesc &cascadeDesc = simulationSettings.cascades[cascadeIndex];
            if (ImGui::TreeNodeEx("CascadeSettings", ImGuiTreeNodeFlags_DefaultOpen, "Cascade %u (%.2f m)", cascadeIndex, cascadeDesc.patchSize)) {
                ImGui::SetNextItemWidth(itemWidth);
                if (ImGui::InputScalar("Random seed", ImGuiDataType_U32, &cascadeDesc.randomSeed, &seedStep, &seedFastStep, "%u")) {
                    oceanSettingsChanged = true;
                }

                ImGui::SetNextItemWidth(itemWidth);
                if (ImGui::DragFloat("Update rate", &simulationSettings.cascadeSettings[cascadeIndex].updateRate, 0.5f, 1.0f, 240.0f, "%.1f Hz")) {
                    simulationSettings.cascadeSettings[cascadeIndex].updateRate = BE1::Max(simulationSettings.cascadeSettings[cascadeIndex].updateRate, 1.0f);
                    oceanSettingsChanged = true;
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
    }

    if (roundedCollapsingHeader("Surface##OceanSurfaceHeader")) {
        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::DragFloat("Surface height", &surfaceSettings.height, 0.1f, -10000.0f, 10000.0f, "%.2f m")) {
            oceanSettingsChanged = true;
        }

        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::DragFloat("Surface grid spacing", &surfaceSettings.gridSpacing, 0.05f, 0.01f, 100.0f, "%.2f m")) {
            surfaceSettings.gridSpacing = BE1::Max(surfaceSettings.gridSpacing, 0.01f);
            oceanSettingsChanged = true;
        }

        int surfaceGridTileCountPerAxis = static_cast<int>(surfaceSettings.gridTileCountPerAxis);
        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::SliderInt("Surface grid tiles per axis", &surfaceGridTileCountPerAxis, 1, GpuOcean::MaxSurfaceGridTileCountPerAxis)) {
            surfaceSettings.gridTileCountPerAxis = static_cast<uint32_t>(surfaceGridTileCountPerAxis);
            oceanSettingsChanged = true;
        }

        oceanSettingsChanged |= ImGui::Checkbox("Wireframe", &surfaceSettings.wireframe);
        oceanSettingsChanged |= ImGui::Checkbox("Surface frustum culling", &surfaceSettings.frustumCulling);

        ImGui::BeginDisabled(!surfaceSettings.frustumCulling);
        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::DragFloat("Surface max displacement", &surfaceSettings.maxDisplacement, 0.1f, 0.0f, 1000.0f, "%.2f m")) {
            surfaceSettings.maxDisplacement = BE1::Max(surfaceSettings.maxDisplacement, 0.0f);
            oceanSettingsChanged = true;
        }
        ImGui::EndDisabled();

        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::SliderFloat("Surface roughness", &surfaceSettings.roughness, 0.02f, 1.0f, "%.3f")) {
            oceanSettingsChanged = true;
        }

        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::ColorEdit3("Water color", surfaceSettings.waterColor.Ptr(), ImGuiColorEditFlags_Float)) {
            oceanSettingsChanged = true;
        }

        oceanSettingsChanged |= ImGui::Checkbox("Refraction", &surfaceSettings.refractionEnabled);
        ImGui::BeginDisabled(!surfaceSettings.refractionEnabled);
        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::DragFloat("Refraction distance", &surfaceSettings.refractionDistance, 0.1f, 0.01f, 1000.0f, "%.2f m")) {
            surfaceSettings.refractionDistance = BE1::Max(surfaceSettings.refractionDistance, 0.01f);
            oceanSettingsChanged = true;
        }
        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::DragFloat("Absorption distance", &surfaceSettings.absorptionDistance, 0.1f, 0.01f, 10000.0f, "%.2f m")) {
            surfaceSettings.absorptionDistance = BE1::Max(surfaceSettings.absorptionDistance, 0.01f);
            oceanSettingsChanged = true;
        }
        ImGui::EndDisabled();

        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::ColorEdit3("Scatter color", surfaceSettings.scatterColor.Ptr(), ImGuiColorEditFlags_Float)) {
            oceanSettingsChanged = true;
        }

        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::DragFloat("Scatter height", &surfaceSettings.scatterHeight, 0.05f, 0.01f, 1000.0f, "%.2f m")) {
            surfaceSettings.scatterHeight = BE1::Max(surfaceSettings.scatterHeight, 0.01f);
            oceanSettingsChanged = true;
        }

        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::SliderFloat("Fresnel F0", &surfaceSettings.fresnelF0, 0.0f, 1.0f, "%.3f")) {
            oceanSettingsChanged = true;
        }
    }

    if (roundedCollapsingHeader("Foam##OceanFoamHeader")) {
        oceanSettingsChanged |= ImGui::Checkbox("Enabled##OceanFoamEnabled", &simulationSettings.foamEnabled);

        ImGui::BeginDisabled(!simulationSettings.foamEnabled);
        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::ColorEdit3("Foam color", surfaceSettings.foamColor.Ptr(), ImGuiColorEditFlags_Float)) {
            oceanSettingsChanged = true;
        }

        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::DragFloat("Detail map size", &surfaceSettings.foamDetailMapSize, 0.1f, 0.01f, 1000.0f, "%.2f m")) {
            surfaceSettings.foamDetailMapSize = BE1::Max(surfaceSettings.foamDetailMapSize, 0.01f);
            oceanSettingsChanged = true;
        }

        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::SliderFloat("Detail strength", &surfaceSettings.foamDetailStrength, 0.0f, 1.0f, "%.3f")) {
            oceanSettingsChanged = true;
        }

        for (uint32_t cascadeIndex = 0; cascadeIndex < GpuOcean::CascadeCount; ++cascadeIndex) {
            ImGui::PushID(static_cast<int>(cascadeIndex));
            const GpuOcean::CascadeDesc &cascadeDesc = simulationSettings.cascades[cascadeIndex];
            GpuOcean::CascadeSettings &cascadeSettings = simulationSettings.cascadeSettings[cascadeIndex];
            if (ImGui::TreeNodeEx("FoamCascadeSettings", ImGuiTreeNodeFlags_DefaultOpen, "Cascade %u (%.2f m)", cascadeIndex, cascadeDesc.patchSize)) {
                ImGui::SetNextItemWidth(itemWidth);
                if (ImGui::SliderFloat("Threshold", &cascadeSettings.foamThreshold, -1.0f, 1.0f, "%.3f")) {
                    oceanSettingsChanged = true;
                }

                ImGui::SetNextItemWidth(itemWidth);
                if (ImGui::DragFloat("Generation rate", &cascadeSettings.foamGenerationRate, 0.05f, 0.0f, 20.0f, "%.3f 1/s")) {
                    cascadeSettings.foamGenerationRate = BE1::Max(cascadeSettings.foamGenerationRate, 0.0f);
                    oceanSettingsChanged = true;
                }

                ImGui::SetNextItemWidth(itemWidth);
                if (ImGui::DragFloat("Half-life", &cascadeSettings.foamHalfLife, 0.01f, 0.01f, 60.0f, "%.3f s")) {
                    cascadeSettings.foamHalfLife = BE1::Max(cascadeSettings.foamHalfLife, 0.01f);
                    oceanSettingsChanged = true;
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
        ImGui::EndDisabled();
    }

    if (roundedCollapsingHeader("Debug##OceanDebugHeader")) {
        enum class DebugTexture : int {
            InitialSpectrum,
            Fields,
            FoamHistory
        };

        enum class DebugField : int {
            Displacement,
            Derivative
        };

        static const char *debugTextureNames[] = {
            "Initial Spectrum",
            "Fields",
            "Foam History"
        };
        static const char *debugFieldNames[] = {
            "Displacement",
            "Derivative"
        };
        static int selectedDebugTexture = static_cast<int>(DebugTexture::InitialSpectrum);
        static int selectedDebugField = static_cast<int>(DebugField::Displacement);
        static int selectedDebugCascade = 0;

        ImGui::SetNextItemWidth(itemWidth);
        ImGui::Combo("Texture##OceanDebugTexture", &selectedDebugTexture, debugTextureNames, COUNT_OF(debugTextureNames));

        selectedDebugCascade = BE1::Clamp(selectedDebugCascade, 0, static_cast<int>(GpuOcean::CascadeCount) - 1);
        ImGui::SetNextItemWidth(itemWidth);
        ImGui::SliderInt("Cascade##OceanDebugCascade", &selectedDebugCascade, 0, static_cast<int>(GpuOcean::CascadeCount) - 1);

        const DebugTexture debugTextureType = static_cast<DebugTexture>(selectedDebugTexture);
        if (debugTextureType == DebugTexture::Fields) {
            ImGui::SetNextItemWidth(itemWidth);
            ImGui::Combo("Field##OceanDebugField", &selectedDebugField, debugFieldNames, COUNT_OF(debugFieldNames));
        }

        const GpuOcean *gpuOcean = renderSystem ? &renderSystem->GetGpuOcean() : nullptr;
        const RHI::Texture *debugTexture = nullptr;
        if (gpuOcean) {
            switch (static_cast<DebugTexture>(selectedDebugTexture)) {
            case DebugTexture::InitialSpectrum:
                debugTexture = gpuOcean->GetInitialSpectrumTextureArray();
                break;
            case DebugTexture::Fields:
                debugTexture = gpuOcean->GetFieldTextureArray();
                break;
            case DebugTexture::FoamHistory:
                debugTexture = gpuOcean->GetFoamHistoryTextureArray();
                break;
            }
        }

        if (!debugTexture || debugTexture->GetStatus() != RHI::GPUResourceStatus::Ready) {
            ImGui::TextDisabled("Ocean debug texture is unavailable.");
        } else {
            const DebugField debugFieldType = static_cast<DebugField>(selectedDebugField);
            const uint32_t cascadeIndex = static_cast<uint32_t>(selectedDebugCascade);
            const uint32_t sliceIndex = debugTextureType == DebugTexture::Fields
                ? cascadeIndex * 2u + static_cast<uint32_t>(debugFieldType)
                : cascadeIndex;
            const float patchSize = simulationSettings.cascades[cascadeIndex].patchSize;

            ImGui::Text("%u x %u", debugTexture->GetWidth(), debugTexture->GetHeight());
            ImGui::Text("Cascade %u (%.2f m)", cascadeIndex, patchSize);
            switch (debugTextureType) {
            case DebugTexture::InitialSpectrum:
                ImGui::TextWrapped("RG = h0(k), BA = conj(h0(-k))");
                break;
            case DebugTexture::Fields:
                if (debugFieldType == DebugField::Displacement) {
                    ImGui::TextWrapped("R=Dx, G=Dy, B=Dz, A=dDx/dy");
                } else {
                    ImGui::TextWrapped("R=dDz/dx, G=dDz/dy, B=dDx/dx, A=dDy/dy");
                }
                break;
            case DebugTexture::FoamHistory:
                ImGui::TextUnformatted("R = accumulated foam");
                break;
            }

            ImGui::TextDisabled("Raw HDR preview (?)");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Signed and out-of-range values are clipped.");
            }
            if (sliceIndex >= debugTexture->GetArraySize() ||
                !App::DrawImGuiTexturePreview(debugTexture, RHI::ViewID::PrimaryID, fontSize * 24.0f, true, sliceIndex)) {
                ImGui::TextDisabled("Ocean debug preview is unavailable.");
            }
        }
    }

    if (oceanSettingsChanged) {
        const bool settingsApplied = renderWorld->UpdateOceanSurface(oceanSettings);
        assert(settingsApplied);
        (void)settingsApplied;
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Directional Light");
    ImGui::TextDisabled("Azimuth: 0 = +X, 90 = +Y; Elevation: 0 = horizon");

    const float directionalLightItemWidth = fontSize * 17.0f;
    bool directionChanged = false;
    bool lightingChanged = false;

    ImGui::SetNextItemWidth(directionalLightItemWidth);
    directionChanged |= ImGui::SliderFloat("Azimuth##OceanDirectionalLight", &directionalLightAzimuth, 0.0f, 360.0f, "%.1f deg");

    ImGui::SetNextItemWidth(directionalLightItemWidth);
    directionChanged |= ImGui::SliderFloat("Elevation##OceanDirectionalLight", &directionalLightElevation, -90.0f, 90.0f, "%.1f deg");

    ImGui::SetNextItemWidth(directionalLightItemWidth);
    lightingChanged |= ImGui::ColorEdit3("Color##OceanDirectionalLight", directionalLight.linearColor.Ptr(), ImGuiColorEditFlags_Float);

    ImGui::SetNextItemWidth(directionalLightItemWidth);
    if (ImGui::DragFloat("Intensity##OceanDirectionalLight", &directionalLight.intensity, 0.01f, 0.0f, 100.0f, "%.2f")) {
        directionalLight.intensity = BE1::Max(directionalLight.intensity, 0.0f);
        lightingChanged = true;
    }

    if (directionalLight.renderLightHandle < 0) {
        return;
    }

    if (directionChanged) {
        UpdateDirectionalLightDirection();
        renderWorld->UpdateRenderLightTransform(directionalLight.renderLightHandle, directionalLight.origin, directionalLight.axis);
    }

    if (lightingChanged) {
        renderWorld->UpdateRenderLightLighting(directionalLight.renderLightHandle, directionalLight.linearColor, directionalLight.intensity, directionalLight.fallOffExponent);
    }
}
#endif
