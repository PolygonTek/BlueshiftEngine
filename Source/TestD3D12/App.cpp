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
#include "Platform/PlatformSystem.h"
#include "Render/RenderSystem.h"
#include "Render/RenderCvars.h"
#include "Render/RenderContext.h"
#include "Render/RenderBackend.h"
#include "Render/RenderWorld.h"
#include "Render/RenderCamera.h"
#include "Render/Texture.h"
#include "Render/Material.h"
#include "Render/MeshInstance.h"
#include "Render/Skeleton.h"
#include "Render/Anim.h"
#include "Render/Font.h"
#include "LoadingSystem.h"
#include "GameObject.h"
#include "App.h"

#ifdef ENABLE_IMGUI
#include "imgui/imgui.h"
#endif

static BE1::CVar        stat_fps("stat_fps", "1", BE1::CVar::Flag::Bool, "show FPS");
static BE1::CVar        stat_render("stat_render", "0", BE1::CVar::Flag::Bool, "show render statistics");

App                     app;

void App::Init(void *mainWindowHandle) {
    PROFILER_INIT();

    loadingSystem.Init();

    renderSystem = new RenderSystem;
    renderSystem->Init(mainWindowHandle);

    mainRenderContext = renderSystem->CreateRenderContext(mainWindowHandle, true);

    mainFont = fontManager.GetFont("Data/EngineFonts/consola.ttf", 22);

    renderWorld = new RenderWorld;
    renderWorld->SetSkyboxMaterial(materialManager.defaultSkyboxMaterial);

    renderCamera = new RenderCamera;

    InitGameObjects(100, 100, 1);

    cameraYaw = targetCameraYaw;
    cameraPitch = targetCameraPitch;
    cameraDistance = targetCameraDistance;
    cameraCenter = targetCameraCenter;
}

void App::Shutdown() {
    ClearGameObjects();

    SAFE_DELETE(renderCamera);
    SAFE_DELETE(renderWorld);

    mainFont.reset();

    loadingSystem.Shutdown();

    renderSystem->DestroyRenderContext(mainRenderContext);
    mainRenderContext = nullptr;

    renderSystem->Shutdown();
    delete renderSystem;

    PROFILER_SHUTDOWN();
}

void App::Update(uint64_t frameUsec) {
    PROFILER_CPU_SCOPED_EVENT("App::Update");

    uint64_t t0 = BE1::PlatformTime::Microseconds();

    loadingSystem.ProcessLoadCompletions();

    elapsedUsec += frameUsec;

    UpdateGameObjects();

    UpdateCamera(frameUsec);

    BE1::cmdSystem.ExecuteCommandBuffer();

    uint64_t t1 = BE1::PlatformTime::Microseconds();
    UpdateGameFrameTime(t1 - t0);
}

void App::Render(uint64_t frameUsec) {
    PROFILER_CPU_SCOPED_EVENT("App::Render");

    const float frameTime = (float)MICRO2SEC((double)frameUsec);
    const float elapsedTime = (float)MICRO2SEC((double)elapsedUsec);
    const float elapsedPrevTime = (float)MICRO2SEC((double)(elapsedUsec - frameUsec));

    mainRenderContext->BeginFrame(frameTime, elapsedTime, elapsedPrevTime, stat_render.GetBool());

    uint64_t t0 = BE1::PlatformTime::Microseconds();

    renderSystem->UpdateFrame();

    //renderWorld->ClearDebugLines();

    RenderScene();

    RenderText();

    renderWorld->RenderGUI(mainRenderContext->GetGuiMesh());

#ifdef ENABLE_IMGUI
    mainRenderContext->RenderImGui([this]() {
        BuildImGuiOverlay();
    });
#endif

    mainRenderContext->EndFrame();

    uint64_t t1 = BE1::PlatformTime::Microseconds();
    UpdateRenderFrontendFrameTime(t1 - t0);
}

void App::UpdateFrameTime(uint64_t frameUsec) {
    static constexpr uint64_t StatUpdateIntervalUsec = 500000;

    statFrameElapsedUsec += frameUsec;
    statFrameCount++;

    if (statFrameElapsedUsec >= StatUpdateIntervalUsec) {
        statFrameAverageMsec = (float)statFrameElapsedUsec / statFrameCount * 0.001f;

        statFrameCount = 0;
        statFrameElapsedUsec = 0;
    }
}

void App::UpdateGameFrameTime(uint64_t frameUsec) {
    static constexpr uint32_t StatUpdateFrameCount = 60;

    statGameElapsedUsec += frameUsec;
    statGameFrameCount++;

    if (statGameFrameCount >= StatUpdateFrameCount) {
        statGameAverageMsec = (float)statGameElapsedUsec / statGameFrameCount * 0.001f;

        statGameElapsedUsec = 0;
        statGameFrameCount = 0;
    }
}

void App::UpdateRenderFrontendFrameTime(uint64_t frameUsec) {
    static constexpr uint32_t StatUpdateFrameCount = 60;

    statRenderFrontendElapsedUsec += frameUsec;
    statRenderFrontendFrameCount++;

    if (statRenderFrontendFrameCount >= StatUpdateFrameCount) {
        statRenderFrontendAverageMsec = (float)statRenderFrontendElapsedUsec / statRenderFrontendFrameCount * 0.001f;

        statRenderFrontendElapsedUsec = 0;
        statRenderFrontendFrameCount = 0;
    }
}

void App::OnResize(int width, int height) {
    if (mainRenderContext) {
        mainRenderContext->OnResize(width, height);
    }
}

void App::InitGameObjects(uint32_t countX, uint32_t countY, uint32_t countZ) {
    goCountX = countX;
    goCountY = countY;
    goCountZ = countZ;

#ifdef ENABLE_IMGUI
    imguiGoCountX = static_cast<int>(countX);
    imguiGoCountY = static_cast<int>(countY);
#endif

    uint32_t goCount = goCountX * goCountY * goCountZ;
    gameObjects.Reserve(goCount);

    testMesh = meshManager.GetMesh("_defaultBoxMesh");

    const BE1::Str textureFilenames[] = {
        "Data/Test/Brick_Wall_Worn.dds",
        "Data/Test/Construction_Gravel.dds",
        "Data/Test/Dry_Cracked_Mud.dds",
        "Data/Test/Flagstone_Wall.dds",
        "Data/Test/Lawn_Grass.dds",
        "Data/Test/Roman_Stone_Floor.dds",
        "Data/Test/Stone_Floor.dds",
        "Data/Test/Stone_Tiles_Facade.dds",
        "Data/Test/Arid_Gravel_Ground.dds",
        "Data/Test/yd0kejgga.dds",
        "Data/Test/T_sc2nbisc.dds",
        "Data/Test/Gouged_Rock_Cliff.dds",
        "Data/Test/Scratched_Polypropylene_Plastic.dds",
        "Data/Test/Rippled_Sand.dds",
        "Data/Test/Dirty_Sidewalk_Tiles.dds",
        "Data/Test/Curated_Stone_Facade.dds",
    };

    for (int i = 0; i < goCount; ++i) {
        GameObject *gameObject = new GameObject;
        gameObjects.Append(gameObject);

        RenderObject::Desc &desc = gameObject->renderObjectDesc;
        desc.worldMatrix.SetIdentity();
        desc.meshInstance = testMesh->GetStaticInstance();
        desc.aabb = testMesh->GetAABB();
        desc.materials.SetCount(1);
        desc.materials[0] = materialManager.GetSingleTextureMaterial(textureFilenames[BE1::Math::RandomRange(0, COUNT_OF(textureFilenames) - 1)]);

        gameObject->renderObjectHandle = renderWorld->AddRenderObject(gameObject->renderObjectDesc);
    }

    // Text 오브젝트
    /*GameObject *gameObject = new GameObject;
    gameObjects.Append(gameObject);

    RenderObject::Desc &desc = gameObject->renderObjectDesc;
    desc.worldMatrix.SetTRS(BE1::Vec3(0, 0, 10), BE1::Mat3::identity, BE1::Vec3::one);
    RenderObject::TextDesc &textDesc = desc.GetTextDesc();
    textDesc.font = mainFont;
    textDesc.text = "Hello World";
    textDesc.textParams.bits.textAnchor = (uint16_t)RenderObject::TextAnchor::MiddleCenter;

    gameObject->renderObjectHandle = renderWorld->AddRenderObject(gameObject->renderObjectDesc);*/

    RotateGameObjects();
}

void App::ClearGameObjects() {
    for (GameObject *gameObject : gameObjects) {
        renderWorld->RemoveRenderObject(gameObject->renderObjectHandle);
    }
    renderWorld->ClearScene();

    gameObjects.DeleteContents(true);
    gameObjects.Clear();

    testMesh.reset();
}

void App::ChangeGameObjectCount(uint32_t countX, uint32_t countY, uint32_t countZ) {
    ClearGameObjects();

    InitGameObjects(countX, countY, countZ);
}

void App::UpdateGameObjects() {
    PROFILER_CPU_SCOPED_EVENT("App::UpdateGameObjects");

    //RotateGameObjects();
}

void App::RotateGameObjects() {
    float elapsedSeconds = (float)MICRO2SEC((double)elapsedUsec);

    const float goSpacing = 1.414214f;
    const float goStartX = -goSpacing * (goCountX - 1) * 0.5f;
    const float goStartY = -goSpacing * (goCountY - 1) * 0.5f;
    const float goStartZ = -goSpacing * (goCountZ - 1) * 0.5f;

    float zRotationSpeed = 0.25f;

    for (int z = 0; z < goCountZ; ++z) {
        for (int y = 0; y < goCountY; ++y) {
            for (int x = 0; x < goCountX; ++x) {
                int index = goCountX * goCountY * z + goCountX * y + x;

                float t = elapsedSeconds + index * 0.1f;

                GameObject *gameObject = gameObjects[index];
                gameObject->renderObjectDesc.worldMatrix.SetTranslationRotation(BE1::Vec3(goStartX + goSpacing * x, goStartY + goSpacing * y, goStartZ + goSpacing * z), BE1::Mat3::FromRotationZ(t * zRotationSpeed), false);

                renderWorld->UpdateRenderObject(gameObject->renderObjectHandle, gameObject->renderObjectDesc, RenderObject::Desc::UpdateFlag::Transform);
            }
        }
    }
}

void App::UpdateCamera(uint64_t frameUsec) {
    constexpr float CameraSmoothingSpeed = 18.0f;

    float dt = (float)MICRO2SEC((double)frameUsec);
    float alpha = 1.0f - BE1::Math::Exp(-CameraSmoothingSpeed * dt);

    cameraYaw += (targetCameraYaw - cameraYaw) * alpha;
    cameraPitch += (targetCameraPitch - cameraPitch) * alpha;
    cameraDistance += (targetCameraDistance - cameraDistance) * alpha;
    cameraCenter = cameraCenter + (targetCameraCenter - cameraCenter) * alpha;
}

void App::AddCameraMouseDelta(int deltaX, int deltaY) {
    constexpr float MouseSensitivity = 0.18f;

    targetCameraYaw -= deltaX * MouseSensitivity;
    targetCameraPitch += deltaY * MouseSensitivity;
    BE1::Clamp(targetCameraPitch, -90.0f, 90.0f);
}

void App::AddCameraPanDelta(int deltaX, int deltaY) {
    if (!mainRenderContext || mainRenderContext->GetWidth() <= 0 || mainRenderContext->GetHeight() <= 0) {
        return;
    }

    const float width = static_cast<float>(mainRenderContext->GetWidth());
    const float height = static_cast<float>(mainRenderContext->GetHeight());
    const float aspectRatio = width / height;

    float fovX;
    float fovY;
    RenderCamera::CalculateFov(60.0f, 16.0f / 9.0f, aspectRatio, &fovX, &fovY);

    const float unitsPerPixelX = 2.0f * targetCameraDistance * BE1::Math::Tan(DEG2RAD(fovX * 0.5f)) / width;
    const float unitsPerPixelY = 2.0f * targetCameraDistance * BE1::Math::Tan(DEG2RAD(fovY * 0.5f)) / height;
    const BE1::Mat3 cameraAxis = BE1::Angles(0.0f, targetCameraPitch, targetCameraYaw).ToMat3();

    targetCameraCenter = targetCameraCenter + cameraAxis[1] * (deltaX * unitsPerPixelX) + cameraAxis[2] * (deltaY * unitsPerPixelY);
}

void App::AddCameraDistance(int delta) {
    constexpr float MouseWheelDelta = 120.0f;
    constexpr float MouseWheelZoomRatio = 0.12f;

    float wheelSteps = (float)delta / MouseWheelDelta;
    targetCameraDistance *= BE1::Math::Pow(1.0f - MouseWheelZoomRatio, wheelSteps);
    BE1::Clamp(targetCameraDistance, 1.0f, 1000.0f);
}

void App::TakeScreenshot() {
    BE1::cmdSystem.BufferCommandText(BE1::CmdSystem::Execution::Now, "screenshot");
}

void App::RenderScene() {
    float w = mainRenderContext->GetWidth();
    float h = mainRenderContext->GetHeight();
    float aspectRatio = w / h;

    RenderCameraDesc cameraDesc;
    cameraDesc.clearMethod = RenderCameraClearMethod::Skybox;
    //cameraDesc.clearColorMode = RenderCameraClearColorMode::RenderTargetDefault;
    cameraDesc.orthogonal = false;
    cameraDesc.outputRect.Set(0, 0, w, h);
    cameraDesc.renderWidth = static_cast<uint32_t>(w);
    cameraDesc.renderHeight = static_cast<uint32_t>(h);
    cameraDesc.axis = BE1::Angles(0.0f, cameraPitch, cameraYaw).ToMat3();
    cameraDesc.origin = cameraCenter - cameraDesc.axis[0] * cameraDistance;
    RenderCamera::CalculateFov(60, 16.0f / 9.0f, aspectRatio, &cameraDesc.fovX, &cameraDesc.fovY);
    cameraDesc.zNear = BE1::MeterToUnit(0.1f);
    cameraDesc.zFar = BE1::MeterToUnit(2000.0f);
    if (cameraDesc.orthogonal) {
        cameraDesc.sizeX = cameraDistance * BE1::Math::Tan(cameraDesc.fovX * 0.5f) * 0.5f;
        cameraDesc.sizeY = cameraDesc.sizeX / aspectRatio;
    }
    renderCamera->Update(cameraDesc);

    renderWorld->RenderScene(renderCamera);
}

void App::RenderText() {
    mainRenderContext->SetFont(mainFont);
    mainRenderContext->SetColor(BE1::Color4::white);
    mainRenderContext->SetTextShadowColor(BE1::Color4::black);
    mainRenderContext->SetTextShadowOffset(1, 1);

    constexpr int textLineHeight = 30;
    int x = 4;
    int y = 4;

    if (stat_fps.GetBool()) {
        const float statFps = statFrameAverageMsec > 0.0f ? 1000.0f / statFrameAverageMsec : 0.0f;

        mainRenderContext->DrawString(x, y, BE1::va("FPS: %.1f", statFps), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += textLineHeight;
    }

    if (stat_render.GetBool()) {
        const uint64_t videoMemBudgetBytes = renderSystem->GetVideoMemoryUsage().budgetBytes;
        const uint64_t videoMemUsageBytes = renderSystem->GetVideoMemoryUsage().usageBytes;

        const uint64_t textureStreamingBudgetBytes = textureManager.GetStreamingTextureMemoryBudgetBytes();
        const uint64_t textureStreamingUsageBytes = textureManager.GetStreamingTextureMemoryUsageBytes();

        const float statRenderFrontendMsec = statRenderFrontendAverageMsec;
        const float statRenderBackendMsec = renderSystem->GetBackend()->GetStatRenderAverageMsec();
        const float statGpuMsec = mainRenderContext->GetStatGpuAverageMsec();
        const uint32_t statRenderObjectCount = mainRenderContext->GetStatRenderObjectCount();
        const uint32_t statVisibleObjectCount = mainRenderContext->GetStatVisibleObjectCount();
        const uint32_t statDrawCount = mainRenderContext->GetStatDrawCount();
        const uint64_t statDrawTriangleCount = mainRenderContext->GetStatDrawTriangleCount();
        const uint32_t statInstanceTableAllocCount = mainRenderContext->GetStatInstanceTableAllocCount();
        const uint32_t statInstanceTableUploadBytes = mainRenderContext->GetStatInstanceTableUploadBytes();

        mainRenderContext->DrawString(x, y, BE1::va("Frame: %.2f ms", statFrameAverageMsec), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += textLineHeight;
        mainRenderContext->DrawString(x, y, BE1::va("Game: %.2f ms", statGameAverageMsec), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += textLineHeight;
        mainRenderContext->DrawString(x, y, BE1::va("Render Frontend: %.2f ms", statRenderFrontendMsec), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += textLineHeight;
        mainRenderContext->DrawString(x, y, BE1::va("Render Backend: %.2f ms", statRenderBackendMsec), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += textLineHeight;
        mainRenderContext->DrawString(x, y, BE1::va("GPU: %.2f ms", statGpuMsec), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += textLineHeight;
        mainRenderContext->DrawString(x, y, BE1::va("Objects: %u, Visible: %u", statRenderObjectCount, statVisibleObjectCount), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += textLineHeight;
        mainRenderContext->DrawString(x, y, BE1::va("Draws: %u, Tris: %llu", statDrawCount, statDrawTriangleCount), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += textLineHeight;
        mainRenderContext->DrawString(x, y, BE1::va("Instance Tables: %u, Upload: %s", statInstanceTableAllocCount, BE1::Str::FormatBytes(statInstanceTableUploadBytes).c_str()), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += textLineHeight;
        mainRenderContext->DrawString(x, y, BE1::va("Video Memory: %s / %s", BE1::Str::FormatBytes(videoMemUsageBytes).c_str(), BE1::Str::FormatBytes(videoMemBudgetBytes).c_str()), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += textLineHeight;
        mainRenderContext->DrawString(x, y, BE1::va("Mipmap Streaming: %s / %s", BE1::Str::FormatBytes(textureStreamingUsageBytes).c_str(), BE1::Str::FormatBytes(textureStreamingBudgetBytes).c_str()), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += textLineHeight;
    }
}

#ifdef ENABLE_IMGUI
void App::UpdateImGuiStatsPlotHistory() {
    const float stats[ImGuiStatsPlotSeriesCount] = {
        statFrameAverageMsec,
        statGameAverageMsec,
        statRenderFrontendAverageMsec,
        renderSystem && renderSystem->GetBackend() ? renderSystem->GetBackend()->GetStatRenderAverageMsec() : 0.0f,
        mainRenderContext ? mainRenderContext->GetStatGpuAverageMsec() : 0.0f
    };

    for (int seriesIndex = 0; seriesIndex < ImGuiStatsPlotSeriesCount; ++seriesIndex) {
        imguiStatsPlotSamples[seriesIndex][imguiStatsPlotWriteIndex] = stats[seriesIndex];
    }

    imguiStatsPlotWriteIndex = (imguiStatsPlotWriteIndex + 1) % ImGuiStatsPlotHistoryLength;
    imguiStatsPlotSampleCount = BE1::Min(imguiStatsPlotSampleCount + 1, ImGuiStatsPlotHistoryLength);
}

void App::BuildImGuiStatsPlotWindow() {
    if (!imguiShowStatsPlotWindow) {
        return;
    }

    static const char *statNames[ImGuiStatsPlotSeriesCount] = {
        "Frame",
        "Game",
        "Frontend",
        "Backend",
        "GPU"
    };
    static const ImVec4 statColors[ImGuiStatsPlotSeriesCount] = {
        ImVec4(0.90f, 0.35f, 0.25f, 1.00f),
        ImVec4(0.30f, 0.75f, 0.35f, 1.00f),
        ImVec4(0.25f, 0.55f, 1.00f, 1.00f),
        ImVec4(0.95f, 0.75f, 0.25f, 1.00f),
        ImVec4(0.75f, 0.45f, 1.00f, 1.00f)
    };

    const float fontSize = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2(fontSize * 42.0f, fontSize * 24.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Stats Plot", &imguiShowStatsPlotWindow)) {
        ImGui::End();
        return;
    }

    const int latestSampleIndex = (imguiStatsPlotWriteIndex + ImGuiStatsPlotHistoryLength - 1) % ImGuiStatsPlotHistoryLength;
    for (int seriesIndex = 0; seriesIndex < ImGuiStatsPlotSeriesCount; ++seriesIndex) {
        ImGui::ColorButton(statNames[seriesIndex], statColors[seriesIndex], ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(fontSize, fontSize));
        ImGui::SameLine();
        ImGui::Text("%s %.2f ms", statNames[seriesIndex], imguiStatsPlotSamples[seriesIndex][latestSampleIndex]);
    }

    float maxMsec = 1.0f;
    for (int seriesIndex = 0; seriesIndex < ImGuiStatsPlotSeriesCount; ++seriesIndex) {
        for (int sampleOffset = 0; sampleOffset < imguiStatsPlotSampleCount; ++sampleOffset) {
            const int sampleIndex = (imguiStatsPlotSampleCount == ImGuiStatsPlotHistoryLength)
                ? (imguiStatsPlotWriteIndex + sampleOffset) % ImGuiStatsPlotHistoryLength
                : sampleOffset;
            maxMsec = BE1::Max(maxMsec, imguiStatsPlotSamples[seriesIndex][sampleIndex]);
        }
    }
    maxMsec *= 1.15f;

    ImGui::Text("Scale 0.00 - %.2f ms", maxMsec);

    const ImVec2 plotSize(ImGui::GetContentRegionAvail().x, fontSize * 12.0f);
    ImGui::InvisibleButton("##StatsPlotCanvas", plotSize);

    const ImVec2 plotMin = ImGui::GetItemRectMin();
    const ImVec2 plotMax = ImGui::GetItemRectMax();
    ImDrawList *drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(plotMin, plotMax, IM_COL32(16, 18, 22, 220));
    drawList->AddRect(plotMin, plotMax, IM_COL32(96, 104, 116, 255));

    for (int gridIndex = 1; gridIndex < 4; ++gridIndex) {
        const float y = plotMin.y + (plotMax.y - plotMin.y) * gridIndex / 4.0f;
        drawList->AddLine(ImVec2(plotMin.x, y), ImVec2(plotMax.x, y), IM_COL32(96, 104, 116, 80));
    }

    if (imguiStatsPlotSampleCount > 1) {
        const float plotWidth = plotMax.x - plotMin.x;
        const float plotHeight = plotMax.y - plotMin.y;
        const float invMaxMsec = 1.0f / maxMsec;

        for (int seriesIndex = 0; seriesIndex < ImGuiStatsPlotSeriesCount; ++seriesIndex) {
            const ImU32 color = ImGui::ColorConvertFloat4ToU32(statColors[seriesIndex]);

            for (int sampleOffset = 1; sampleOffset < imguiStatsPlotSampleCount; ++sampleOffset) {
                const int prevSampleIndex = (imguiStatsPlotSampleCount == ImGuiStatsPlotHistoryLength)
                    ? (imguiStatsPlotWriteIndex + sampleOffset - 1) % ImGuiStatsPlotHistoryLength
                    : sampleOffset - 1;
                const int sampleIndex = (imguiStatsPlotSampleCount == ImGuiStatsPlotHistoryLength)
                    ? (imguiStatsPlotWriteIndex + sampleOffset) % ImGuiStatsPlotHistoryLength
                    : sampleOffset;

                const float x0 = plotMin.x + plotWidth * (sampleOffset - 1) / (imguiStatsPlotSampleCount - 1);
                const float x1 = plotMin.x + plotWidth * sampleOffset / (imguiStatsPlotSampleCount - 1);
                const float y0 = plotMax.y - BE1::Min(imguiStatsPlotSamples[seriesIndex][prevSampleIndex] * invMaxMsec, 1.0f) * plotHeight;
                const float y1 = plotMax.y - BE1::Min(imguiStatsPlotSamples[seriesIndex][sampleIndex] * invMaxMsec, 1.0f) * plotHeight;

                drawList->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), color, 1.5f);
            }
        }
    }

    ImGui::End();
}

struct ImGuiTextureViewerEntry {
    char                    label[64] = {};
    const RHI::Texture *    texture = nullptr;
};

static void AddImGuiTextureViewerEntry(ImGuiTextureViewerEntry *entries, int maxEntries, int &entryCount, const char *label, const RHI::Texture *texture) {
    if (entryCount >= maxEntries || !texture || texture->GetViewCount(RHI::ViewType::SRV) == 0) {
        return;
    }

    ImGuiTextureViewerEntry &entry = entries[entryCount++];
    BE1::Str::Copynz(entry.label, label, sizeof(entry.label));
    entry.texture = texture;
}

static void AppendImGuiTextureFlagName(char *text, size_t textSize, bool &hasText, RHI::TextureFlag flag) {
    if (!text || textSize == 0) {
        return;
    }

    const char *flagName = RHI::Renderer::TextureFlagName(flag);
    const char *separator = hasText ? " | " : "";
    const size_t textLength = strlen(text);
    if (textLength >= textSize - 1) {
        return;
    }

    BE1::Str::snPrintf(text + textLength, static_cast<int>(textSize - textLength), "%s%s", separator, flagName);
    hasText = true;
}

static void TextureFlagsToText(RHI::TextureFlag flags, char *text, size_t textSize) {
    if (!text || textSize == 0) {
        return;
    }

    text[0] = '\0';
    if (flags == RHI::TextureFlag::None) {
        BE1::Str::Copynz(text, RHI::Renderer::TextureFlagName(RHI::TextureFlag::None), static_cast<int>(textSize));
        return;
    }

    bool hasText = false;
    if (BE1::HasFlag(flags, RHI::TextureFlag::RenderTarget)) {
        AppendImGuiTextureFlagName(text, textSize, hasText, RHI::TextureFlag::RenderTarget);
    }
    if (BE1::HasFlag(flags, RHI::TextureFlag::DepthStencil)) {
        AppendImGuiTextureFlagName(text, textSize, hasText, RHI::TextureFlag::DepthStencil);
    }
    if (BE1::HasFlag(flags, RHI::TextureFlag::Typeless)) {
        AppendImGuiTextureFlagName(text, textSize, hasText, RHI::TextureFlag::Typeless);
    }
    if (BE1::HasFlag(flags, RHI::TextureFlag::ShaderResource)) {
        AppendImGuiTextureFlagName(text, textSize, hasText, RHI::TextureFlag::ShaderResource);
    }
    if (BE1::HasFlag(flags, RHI::TextureFlag::UnorderedAccess)) {
        AppendImGuiTextureFlagName(text, textSize, hasText, RHI::TextureFlag::UnorderedAccess);
    }
    if (BE1::HasFlag(flags, RHI::TextureFlag::SkipDefaultViews)) {
        AppendImGuiTextureFlagName(text, textSize, hasText, RHI::TextureFlag::SkipDefaultViews);
    }

    if (!hasText) {
        BE1::Str::Copynz(text, "Invalid", static_cast<int>(textSize));
    }
}

void App::BuildImGuiTextureViewer() {
    static int selectedTextureIndex = 0;
    static int selectedViewIndex = 0;

    constexpr int MaxTextureViewerEntries = 5;
    ImGuiTextureViewerEntry entries[MaxTextureViewerEntries];
    int entryCount = 0;

    if (renderCamera) {
        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "Color", renderCamera->GetColorTexture());
        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "Depth", renderCamera->GetDepthTexture());
        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "Post Process Color", renderCamera->GetPostProcessColorTexture());
        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "Bloom Down", renderCamera->GetBloomDownTexture());
        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "Bloom Up", renderCamera->GetBloomUpTexture());
    }

    if (entryCount == 0) {
        ImGui::TextUnformatted("No texture is available.");
        return;
    }

    selectedTextureIndex = BE1::Min(BE1::Max(selectedTextureIndex, 0), entryCount - 1);

    const float fontSize = ImGui::GetFontSize();
    ImGui::SetNextItemWidth(fontSize * 18.0f);
    if (ImGui::BeginCombo("Texture", entries[selectedTextureIndex].label)) {
        for (int entryIndex = 0; entryIndex < entryCount; ++entryIndex) {
            const bool selected = selectedTextureIndex == entryIndex;
            if (ImGui::Selectable(entries[entryIndex].label, selected)) {
                selectedTextureIndex = entryIndex;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    const ImGuiTextureViewerEntry &entry = entries[selectedTextureIndex];
    const RHI::Texture *texture = entry.texture;
    const uint32_t viewCount = texture->GetViewCount(RHI::ViewType::SRV);
    selectedViewIndex = BE1::Min(BE1::Max(selectedViewIndex, 0), static_cast<int>(viewCount) - 1);

    if (viewCount > 1) {
        ImGui::SetNextItemWidth(fontSize * 12.0f);
        char viewSliderLabel[32];
        BE1::Str::snPrintf(viewSliderLabel, static_cast<int>(sizeof(viewSliderLabel)), "View (0..%u)", viewCount - 1);
        ImGui::SliderInt(viewSliderLabel, &selectedViewIndex, 0, static_cast<int>(viewCount) - 1);
    }

    const RHI::ViewID viewID = texture->GetViewID(RHI::ViewType::SRV, selectedViewIndex);
    RHI::TextureViewInfo viewInfo;
    const bool hasViewInfo = texture->GetViewInfo(RHI::ViewType::SRV, viewID, viewInfo);
    const uint32_t textureID = texture->GetBindlessIndex(RHI::ViewType::SRV, viewID);

    ImGui::Separator();
    ImGui::Text("Type: %s", RHI::Renderer::TextureTypeName(texture->type));
    ImGui::Text("Format: %s", RHI::Renderer::FormatName(texture->GetFormat()));
    ImGui::Text("Size: %u x %u x %u", texture->GetWidth(), texture->GetHeight(), texture->GetDepth());
    ImGui::Text("Array: %u, Mips: %u, Samples: %u", texture->GetArraySize(), texture->GetMipLevelCount(), texture->GetSampleCount());

    char flagsText[128];
    TextureFlagsToText(texture->flags, flagsText, sizeof(flagsText));
    ImGui::TextUnformatted("Flags:");
    ImGui::SameLine();
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextUnformatted(flagsText);
    ImGui::PopTextWrapPos();

    if (hasViewInfo) {
        ImGui::Text("View ID: %u", viewInfo.viewID.value);
        if (viewInfo.format != texture->GetFormat()) {
            ImGui::Text("View Format: %s", RHI::Renderer::FormatName(viewInfo.format));
        }
        if (viewInfo.sliceCount > 1) {
            ImGui::Text("View Slice Range: %u..%u (%u)", viewInfo.firstSlice, viewInfo.firstSlice + viewInfo.sliceCount - 1, viewInfo.sliceCount);
        } else {
            ImGui::Text("View Slice: %u", viewInfo.firstSlice);
        }
        if (viewInfo.mipCount > 1) {
            ImGui::Text("View Mip Range: %u..%u (%u)", viewInfo.firstMipLevel, viewInfo.firstMipLevel + viewInfo.mipCount - 1, viewInfo.mipCount);
        } else {
            ImGui::Text("View Mip: %u", viewInfo.firstMipLevel);
        }
        ImGui::Text("View Size: %u x %u x %u", viewInfo.width, viewInfo.height, viewInfo.depth);
    } else {
        ImGui::Text("View ID: %u", viewID.value);
    }

    const uint32_t previewWidth = hasViewInfo ? viewInfo.width : texture->GetWidth();
    const uint32_t previewHeight = hasViewInfo ? viewInfo.height : texture->GetHeight();
    if (textureID == 0 || previewWidth == 0 || previewHeight == 0) {
        ImGui::TextUnformatted("Preview is unavailable.");
        return;
    }

    const float maxPreviewSize = fontSize * 24.0f;
    const float previewAspectRatio = static_cast<float>(previewWidth) / previewHeight;
    ImVec2 previewSize;
    if (previewAspectRatio > 1.0f) {
        previewSize.x = maxPreviewSize;
        previewSize.y = maxPreviewSize / previewAspectRatio;
    } else {
        previewSize.x = maxPreviewSize * previewAspectRatio;
        previewSize.y = maxPreviewSize;
    }

    const ImVec2 previewFrameSize(maxPreviewSize, maxPreviewSize);
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    if (availableWidth > previewFrameSize.x) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availableWidth - previewFrameSize.x) * 0.5f);
    }

    const ImVec2 framePos = ImGui::GetCursorScreenPos();
    const ImVec2 imagePos(
        framePos.x + (previewFrameSize.x - previewSize.x) * 0.5f,
        framePos.y + (previewFrameSize.y - previewSize.y) * 0.5f);

    ImDrawList *drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(
        framePos,
        ImVec2(framePos.x + previewFrameSize.x, framePos.y + previewFrameSize.y),
        IM_COL32(0, 0, 0, 255));
    drawList->AddImage(
        (ImTextureID)(UINT64)textureID,
        imagePos,
        ImVec2(imagePos.x + previewSize.x, imagePos.y + previewSize.y));
    ImGui::Dummy(previewFrameSize);
}

void App::BuildImGuiOverlay() {
    UpdateImGuiStatsPlotHistory();

    const float fontSize = ImGui::GetFontSize();

    //ImGui::StyleColorsClassic();
    //ImGui::StyleColorsLight();
    ImGui::StyleColorsDark();

    ImGui::Begin("TestD3D12", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    if (ImGui::BeginTabBar("TestD3D12Tabs")) {
        if (ImGui::BeginTabItem("System")) {
            ImGui::Text("CPU Cores: %i physical / %i logical",
                BE1::PlatformSystem::NumCPUCores(),
                BE1::PlatformSystem::NumCPUCoresIncludingHyperthreads());
            ImGui::Text("Render Thread: %s",
                renderSystem && renderSystem->IsUsingRenderThread() ? "Enabled" : "Disabled");
            ImGui::Text("Task Threads: %u",
                BE1::Engine::taskManager ? BE1::Engine::taskManager->NumThreads() : 0);
            ImGui::Separator();

            if (RHI::renderer) {
                const RHI::RendererInfo &rendererInfo = RHI::renderer->GetRendererInfo();

                ImGui::Text("GPU Adapter: %s", rendererInfo.name);
                ImGui::Text("Vendor ID: 0x%04X", rendererInfo.vendorId);
                ImGui::Text("Device ID: 0x%04X", rendererInfo.deviceId);
                ImGui::Separator();
                ImGui::Text("Dedicated VRAM: %s", BE1::Str::FormatBytes(rendererInfo.dedicatedVideoMemoryBytes).c_str());
                ImGui::Text("Dedicated System Memory: %s", BE1::Str::FormatBytes(rendererInfo.dedicatedSystemMemoryBytes).c_str());
                ImGui::Text("Shared System Memory: %s", BE1::Str::FormatBytes(rendererInfo.sharedSystemMemoryBytes).c_str());
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Render")) {
            bool showFps = stat_fps.GetBool();
            if (ImGui::Checkbox("Show FPS", &showFps)) {
                stat_fps.SetBool(showFps);
            }

            bool showRenderStats = stat_render.GetBool();
            if (ImGui::Checkbox("Show render statistics", &showRenderStats)) {
                stat_render.SetBool(showRenderStats);
            }

            if (ImGui::Button(imguiShowStatsPlotWindow ? "Hide Stats Plot" : "Show Stats Plot")) {
                imguiShowStatsPlotWindow = !imguiShowStatsPlotWindow;
            }

            ImGui::Separator();

            bool skipCulling = r_skipCulling.GetBool();
            if (ImGui::Checkbox("Skip culling", &skipCulling)) {
                r_skipCulling.SetBool(skipCulling);
            }

            bool skipDrawCamera2D = r_skipDrawCamera2D.GetBool();
            if (ImGui::Checkbox("Skip draw camera 2D", &skipDrawCamera2D)) {
                r_skipDrawCamera2D.SetBool(skipDrawCamera2D);
            }

            bool skipDrawCamera3D = r_skipDrawCamera3D.GetBool();
            if (ImGui::Checkbox("Skip draw camera 3D", &skipDrawCamera3D)) {
                r_skipDrawCamera3D.SetBool(skipDrawCamera3D);
            }

            bool skipPostProcess = r_skipPostProcess.GetBool();
            if (ImGui::Checkbox("Skip post process", &skipPostProcess)) {
                r_skipPostProcess.SetBool(skipPostProcess);
            }

            ImGui::Separator();

            bool toneMapping = r_toneMapping.GetBool();
            if (ImGui::Checkbox("Tone mapping", &toneMapping)) {
                r_toneMapping.SetBool(toneMapping);
            }

            int toneMapOp = BE1::Clamp(r_toneMapOp.GetInteger(), static_cast<int>(ToneMappingOperatorLinear), static_cast<int>(ToneMappingOperatorFilmicGT));
            ImGui::SetNextItemWidth(fontSize * 12.0f);
            if (ImGui::Combo("Tone map operator", &toneMapOp,
                "Linear\0"
                "Exponential\0"
                "Logarithmic\0"
                "Drago Logarithmic\0"
                "Reinhard\0"
                "Reinhard Extended\0"
                "Filmic ALU\0"
                "Filmic ACES\0"
                "Filmic Unreal3\0"
                "Filmic Uncharted 2\0"
                "Filmic GT\0\0")) {
                r_toneMapOp.SetInteger(toneMapOp);
            }

            float luminanceLogMin = r_toneMapLuminanceLogMin.GetFloat();
            float luminanceLogMax = r_toneMapLuminanceLogMax.GetFloat();
            ImGui::SetNextItemWidth(fontSize * 12.0f);
            if (ImGui::DragFloatRange2("Luminance log2 range", &luminanceLogMin, &luminanceLogMax, 0.05f, -16.0f, 16.0f, "Min: %.2f", "Max: %.2f")) {
                r_toneMapLuminanceLogMin.SetFloat(luminanceLogMin);
                r_toneMapLuminanceLogMax.SetFloat(BE1::Max(luminanceLogMax, luminanceLogMin + 0.0001f));
            }

            float luminanceAdaptationRate = r_toneMapLuminanceAdaptationRate.GetFloat();
            ImGui::SetNextItemWidth(fontSize * 12.0f);
            if (ImGui::SliderFloat("Luminance adaptation rate", &luminanceAdaptationRate, 0.0f, 10.0f, "%.2f")) {
                r_toneMapLuminanceAdaptationRate.SetFloat(luminanceAdaptationRate);
            }

            float middleGray = r_toneMapMiddleGray.GetFloat();
            ImGui::SetNextItemWidth(fontSize * 12.0f);
            if (ImGui::SliderFloat("Middle gray", &middleGray, 0.0f, 2.0f, "%.3f")) {
                r_toneMapMiddleGray.SetFloat(middleGray);
            }

            ImGui::Separator();

            int bloomMode = BE1::Clamp(r_bloom.GetInteger(), static_cast<int>(RenderBloomMode::Off), static_cast<int>(RenderBloomMode::FFTConvolution));
            ImGui::SetNextItemWidth(fontSize * 12.0f);
            if (ImGui::Combo("Bloom", &bloomMode,
                "Off\0"
                "Pyramid\0"
                "FFT Convolution\0")) {
                r_bloom.SetInteger(bloomMode);
            }

            float bloomThreshold = r_bloomThreshold.GetFloat();
            ImGui::SetNextItemWidth(fontSize * 12.0f);
            if (ImGui::SliderFloat("Bloom threshold", &bloomThreshold, 0.0f, 10.0f, "%.2f")) {
                r_bloomThreshold.SetFloat(bloomThreshold);
            }

            float bloomSoftThreshold = r_bloomSoftThreshold.GetFloat();
            ImGui::SetNextItemWidth(fontSize * 12.0f);
            if (ImGui::SliderFloat("Bloom soft threshold", &bloomSoftThreshold, 0.0f, 5.0f, "%.2f")) {
                r_bloomSoftThreshold.SetFloat(bloomSoftThreshold);
            }

            int bloomPrefilterQuality = BE1::Clamp(r_bloomPrefilterQuality.GetInteger(), 0, 1);
            ImGui::SetNextItemWidth(fontSize * 12.0f);
            if (ImGui::Combo("Bloom prefilter quality", &bloomPrefilterQuality,
                "Rotated Grid 5 Tap\0"
                "Jimenez 13 Tap\0")) {
                r_bloomPrefilterQuality.SetInteger(bloomPrefilterQuality);
            }

            float bloomIntensity = r_bloomIntensity.GetFloat();
            ImGui::SetNextItemWidth(fontSize * 12.0f);
            if (ImGui::SliderFloat("Bloom intensity", &bloomIntensity, 0.0f, 2.0f, "%.3f")) {
                r_bloomIntensity.SetFloat(bloomIntensity);
            }

            float bloomScatter = r_bloomScatter.GetFloat();
            ImGui::SetNextItemWidth(fontSize * 12.0f);
            if (ImGui::SliderFloat("Bloom scatter", &bloomScatter, 0.0f, 1.0f, "%.3f")) {
                r_bloomScatter.SetFloat(bloomScatter);
            }

            float bloomPrefilterRadius = r_bloomPrefilterRadius.GetFloat();
            ImGui::SetNextItemWidth(fontSize * 12.0f);
            if (ImGui::SliderFloat("Bloom prefilter radius", &bloomPrefilterRadius, 0.0f, 5.0f, "%.2f")) {
                r_bloomPrefilterRadius.SetFloat(bloomPrefilterRadius);
            }

            float bloomDownsampleRadius = r_bloomDownsampleRadius.GetFloat();
            ImGui::SetNextItemWidth(fontSize * 12.0f);
            if (ImGui::SliderFloat("Bloom downsample radius", &bloomDownsampleRadius, 0.0f, 5.0f, "%.2f")) {
                r_bloomDownsampleRadius.SetFloat(bloomDownsampleRadius);
            }

            float bloomUpsampleRadius = r_bloomUpsampleRadius.GetFloat();
            ImGui::SetNextItemWidth(fontSize * 12.0f);
            if (ImGui::SliderFloat("Bloom upsample radius", &bloomUpsampleRadius, 0.0f, 5.0f, "%.2f")) {
                r_bloomUpsampleRadius.SetFloat(bloomUpsampleRadius);
            }

            ImGui::Separator();

            int streamInDelay = r_textureStreamingStreamInDelay.GetInteger();
            ImGui::SetNextItemWidth(fontSize * 6.0f);
            if (ImGui::InputInt("Texture stream-in delay", &streamInDelay)) {
                r_textureStreamingStreamInDelay.SetInteger(BE1::Max(0, streamInDelay));
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Textures")) {
            BuildImGuiTextureViewer();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Scene")) {
            ImGui::SetNextItemWidth(fontSize * 10.0f);
            ImGui::SliderInt("Count X", &imguiGoCountX, 1, 250);
            const bool applyCountX = ImGui::IsItemDeactivatedAfterEdit();

            ImGui::SetNextItemWidth(fontSize * 10.0f);
            ImGui::SliderInt("Count Y", &imguiGoCountY, 1, 250);
            const bool applyCountY = ImGui::IsItemDeactivatedAfterEdit();

            if ((applyCountX || applyCountY) && (static_cast<uint32_t>(imguiGoCountX) != goCountX || static_cast<uint32_t>(imguiGoCountY) != goCountY || goCountZ != 1)) {
                ChangeGameObjectCount(static_cast<uint32_t>(imguiGoCountX), static_cast<uint32_t>(imguiGoCountY), 1);
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Debug")) {
            ImGui::Text("Mouse capture: %s", ImGui::GetIO().WantCaptureMouse ? "true" : "false");
            ImGui::Text("Keyboard capture: %s", ImGui::GetIO().WantCaptureKeyboard ? "true" : "false");
            ImGui::Separator();

            if (ImGui::Button(imguiShowDemoWindow ? "Hide ImGui Demo" : "Show ImGui Demo")) {
                imguiShowDemoWindow = !imguiShowDemoWindow;
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();

    BuildImGuiStatsPlotWindow();

    if (imguiShowDemoWindow) {
        ImGui::ShowDemoWindow(&imguiShowDemoWindow);
    }
}
#endif
