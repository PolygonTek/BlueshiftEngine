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
#include "Render/RenderSystem.h"
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

BE1::CVar       app_showFps("app_showFps", "1", BE1::CVar::Flag::Bool, "show FPS");
BE1::CVar       app_showStats("app_showStats", "0", BE1::CVar::Flag::Bool, "show render statistics");
BE1::CVar       app_convolutionBloomKernelIndex("app_convolutionBloomKernelIndex", "0", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "convolution bloom kernel index", 0, App::ConvolutionBloomKernelCount - 1);
BE1::CVar       app_convolutionBloomAperture("app_convolutionBloomAperture", "5", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "convolution bloom aperture", App::Triangle, App::Circle);

App             app;

void App::Init(void *mainWindowHandle) {
    PROFILER_INIT();

    loadingSystem.Init();

    renderSystem = new RenderSystem;
    renderSystem->Init(mainWindowHandle);

    mainRenderContext = renderSystem->CreateRenderContext(mainWindowHandle, true);

    mainFont = fontManager.GetFont("Data/EngineFonts/consola.ttf", 22);

    mainSkyboxMaterial = materialManager.GetMaterial("Source/TestD3D12/Materials/skybox.material");

    renderWorld = new RenderWorld;
    renderWorld->SetSkyboxMaterial(mainSkyboxMaterial);

    renderCamera = new RenderCamera;

    SetConvolutionBloomKernelIndex(app_convolutionBloomKernelIndex.GetInteger());
    SetConvolutionBloomAperture((App::Aperture)app_convolutionBloomAperture.GetInteger());
 
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

    convolutionBloomApertureTexture.reset();
    convolutionBloomKernelTexture.reset();
    mainFont.reset();
    mainSkyboxMaterial.reset();

    loadingSystem.Shutdown();

    renderSystem->DestroyRenderContext(mainRenderContext);
    mainRenderContext = nullptr;

    renderSystem->Shutdown();
    SAFE_DELETE(renderSystem);

    PROFILER_SHUTDOWN();
}

void App::GetConvolutionBloomKernelDisplayName(int index, char *buffer, int bufferSize) const {
    BE1::Str::snPrintf(buffer, bufferSize, "CGV Bloom %02i", index + 1);
}

void App::SetConvolutionBloomKernelIndex(int index) {
    const int currentIndex = app_convolutionBloomKernelIndex.GetInteger();

    app_convolutionBloomKernelIndex.SetInteger(index);
    index = app_convolutionBloomKernelIndex.GetInteger();

    if (currentIndex == index && convolutionBloomKernelTexture) {
        return;
    }

    char filename[128];
    BE1::Str::snPrintf(filename, sizeof(filename), "BloomKernels/CGV_Bloom_%02i.EXR", index + 1);

    SetConvolutionBloomKernelFromFile(filename);
}

void App::SetConvolutionBloomKernelFromFile(const char *filename) {
    BE1::Image kernelImage = BE1::Image::ImageFromFile(filename);
    if (kernelImage.IsEmpty()) {
        return;
    }

    float normalizationFactor;
    BE1::Vec2 kernelExtents;
    RenderCamera::CalcConvolutionBloomKernelParams(kernelImage, normalizationFactor, kernelExtents);

    convolutionBloomKernelTexture = textureManager.GetTexture(filename, Texture::Flag::HighQuality | Texture::Flag::NoMipmaps);
    renderCamera->SetConvolutionBloomKernelTexture(convolutionBloomKernelTexture, normalizationFactor, kernelExtents);
}

void App::SetConvolutionBloomAperture(App::Aperture aperture) {
    char filename[128];
    switch (aperture) {
    case App::Aperture::Triangle:
        BE1::Str::snPrintf(filename, sizeof(filename), "EngineTextures/Apertures/Triangle.png");
        break;
    case App::Aperture::Square:
        BE1::Str::snPrintf(filename, sizeof(filename), "EngineTextures/Apertures/Square.png");
        break;
    case App::Aperture::HorzRectangle:
        BE1::Str::snPrintf(filename, sizeof(filename), "EngineTextures/Apertures/RectangleHorz.png");
        break;
    case App::Aperture::VertRectangle:
        BE1::Str::snPrintf(filename, sizeof(filename), "EngineTextures/Apertures/RectangleVert.png");
        break;
    case App::Aperture::Pentagon:
        BE1::Str::snPrintf(filename, sizeof(filename), "EngineTextures/Apertures/Pentagon.png");
        break;
    case App::Aperture::Hexagon:
        BE1::Str::snPrintf(filename, sizeof(filename), "EngineTextures/Apertures/Hexagon.png");
        break;
    case App::Aperture::Heptagon:
        BE1::Str::snPrintf(filename, sizeof(filename), "EngineTextures/Apertures/Heptagon.png");
        break;
    case App::Aperture::Octagon:
        BE1::Str::snPrintf(filename, sizeof(filename), "EngineTextures/Apertures/Octagon.png");
        break;
    case App::Aperture::Circle:
        BE1::Str::snPrintf(filename, sizeof(filename), "EngineTextures/Apertures/Circle.png");
        break;
    default:
        return;
    }

    SetConvolutionBloomApertureFromFile(filename);
}

void App::SetConvolutionBloomApertureFromFile(const char *filename) {
    convolutionBloomApertureTexture = textureManager.GetTexture(filename, Texture::Flag::HighQuality | Texture::Flag::NoMipmaps);
    renderCamera->SetConvolutionBloomApertureTexture(convolutionBloomApertureTexture);
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

    bool statEnabled = app_showStats.GetBool();
#ifdef ENABLE_IMGUI
    statEnabled |= imguiShowStatsPlotWindow;
#endif

    mainRenderContext->BeginFrame(frameTime, elapsedTime, elapsedPrevTime, statEnabled);

    uint64_t t0 = BE1::PlatformTime::Microseconds();

    renderSystem->UpdateFrame();

    //renderWorld->ClearDebugLines();

    RenderScene();

    RenderGUI();

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
    uint32_t w = mainRenderContext->GetWidth();
    uint32_t h = mainRenderContext->GetHeight();
    float aspectRatio = (float)w / h;

    RenderCameraDesc cameraDesc;
    cameraDesc.clearMethod = RenderCameraClearMethod::Skybox;
    //cameraDesc.clearColorMode = RenderCameraClearColorMode::RenderTargetDefault;
    cameraDesc.orthogonal = false;
    cameraDesc.outputRect.Set(0, 0, w, h);
    cameraDesc.renderWidth = w;
    cameraDesc.renderHeight = h;
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

void App::RenderGUI() {
    mainRenderContext->SetFont(mainFont);
    mainRenderContext->SetColor(BE1::Color4::white);
    mainRenderContext->SetTextShadowColor(BE1::Color4::black);
    mainRenderContext->SetTextShadowOffset(1, 1);

    constexpr int textLineHeight = 30;
    int x = 4;
    int y = 4;

    if (app_showFps.GetBool()) {
        const float statFps = statFrameAverageMsec > 0.0f ? 1000.0f / statFrameAverageMsec : 0.0f;

        mainRenderContext->DrawString(x, y, BE1::va("FPS: %.1f", statFps), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += textLineHeight;
    }

    if (app_showStats.GetBool()) {
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

    renderWorld->RenderGUI(mainRenderContext->GetGuiMesh());

#ifdef ENABLE_IMGUI
    mainRenderContext->RenderImGui([this]() {
        BuildImGuiOverlay();
    });
#endif
}
