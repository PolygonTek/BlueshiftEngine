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
#include "Render/RenderCvars.h"
#include "Render/RenderWorld.h"
#include "Render/RenderCamera.h"
#include "Render/Material/MaterialManager.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"
#include "Render/Texture/TextureManager.h"
#include "Render/Font/FontManager.h"
#include "LoadingSystem.h"
#include "Scene.h"
#include "SceneRegistry.h"
#include "App.h"

BE1::CVar       app_showFps("app_showFps", "1", BE1::CVar::Flag::Bool, "show FPS");
BE1::CVar       app_showStats("app_showStats", "0", BE1::CVar::Flag::Bool, "show render statistics");
BE1::CVar       app_convolutionBloomKernelSource("app_convolutionBloomKernelSource", "1", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "convolution bloom kernel source", static_cast<int>(App::ConvolutionBloomKernelSource::Preset), static_cast<int>(App::ConvolutionBloomKernelSource::GenerateFromAperture));
BE1::CVar       app_convolutionBloomKernelPreset("app_convolutionBloomKernelPreset", "0", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "convolution bloom kernel preset", 0, App::ConvolutionBloomKernelPresetCount - 1);
BE1::CVar       app_convolutionBloomAperturePreset("app_convolutionBloomAperturePreset", "5", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "convolution bloom aperture preset", static_cast<int>(App::AperturePreset::Triangle), static_cast<int>(App::AperturePreset::Circle));

App             app;

void App::Init(void *mainWindowHandle) {
    PROFILER_INIT();

    loadingSystem.Init();

    renderSystem = new RenderSystem;
    renderSystem->Init(mainWindowHandle);

    mainRenderContext = renderSystem->CreateRenderContext(mainWindowHandle, true);

    mainFont = fontManager.GetFont("Data/EngineFonts/consola.ttf", 22);

    renderWorld = new RenderWorld;
    renderWorld->SetSkyboxMaterial(materialManager.GetMaterial("_defaultSkyboxMaterial"));

    renderCamera = new RenderCamera;

    SharedPtr<Texture> bloomDirtMaskTexture = textureManager.GetTexture("Data/EngineTextures/LensDirt02.png", Texture::Flag::HighQuality | Texture::Flag::NonPowerOfTwo | Texture::Flag::NoMipmaps | Texture::Flag::Compute, false);
    if (bloomDirtMaskTexture && bloomDirtMaskTexture != textureManager.defaultTexture) {
        renderCamera->SetBloomDirtMaskTexture(bloomDirtMaskTexture);
    }

    cameraYaw = targetCameraYaw;
    cameraPitch = targetCameraPitch;
    cameraDistance = targetCameraDistance;
    cameraCenter = targetCameraCenter;

    const BE1::Mat3 targetCameraAxis = BE1::Angles(0.0f, targetCameraPitch, targetCameraYaw).ToMat3();
    targetCameraOrigin = targetCameraCenter - targetCameraAxis[0] * targetCameraDistance;
    cameraOrigin = targetCameraOrigin;

    app_convolutionBloomKernelPreset.SetModified();
    app_convolutionBloomAperturePreset.SetModified();
    app_convolutionBloomKernelSource.SetModified();

    ChangeScene(0);
}

void App::Shutdown() {
    DestroyScene();

    SAFE_DELETE(renderCamera);
    SAFE_DELETE(renderWorld);

    convolutionBloomApertureTexture.reset();
    convolutionBloomGeneratedKernelTexture.reset();

    mainFont.reset();

    loadingSystem.Shutdown();

    renderSystem->DestroyRenderContext(mainRenderContext);
    mainRenderContext = nullptr;

    renderSystem->Shutdown();
    SAFE_DELETE(renderSystem);

    PROFILER_SHUTDOWN();
}

void App::Update(uint64_t frameUsec) {
    PROFILER_CPU_SCOPED_EVENT("App::Update");

    uint64_t t0 = BE1::PlatformTime::Microseconds();

    loadingSystem.ProcessLoadCompletions();
    loadingSystem.ProcessPendingReloads();

    shaderTemplateManager.ProcessPendingGlobalDefineChanges();

    ProcessPendingSceneChange();

    elapsedUsec += frameUsec;

    scene->Update(elapsedUsec);

    UpdateCamera(frameUsec);

    BE1::cmdSystem.ExecuteCommandBuffer();

    CheckModifiedCVars();

    uint64_t t1 = BE1::PlatformTime::Microseconds();
    UpdateGameFrameTime(t1 - t0);
}

void App::RequestSceneChange(int sceneIndex) {
    assert(sceneIndex >= 0 && sceneIndex < SceneRegistry::GetCount());

    pendingSceneIndex = sceneIndex != this->sceneIndex ? sceneIndex : -1;
}

void App::ProcessPendingSceneChange() {
    if (pendingSceneIndex == -1) {
        return;
    }

    const int nextSceneIndex = pendingSceneIndex;
    pendingSceneIndex = -1;

    ChangeScene(nextSceneIndex);
}

void App::ChangeScene(int sceneIndex) {
    assert(sceneIndex >= 0 && sceneIndex < SceneRegistry::GetCount());

    if (sceneIndex == this->sceneIndex) {
        return;
    }

    DestroyScene();

    scene = SceneRegistry::Create(sceneIndex);
    scene->Init(renderWorld);
    this->sceneIndex = sceneIndex;
}

void App::DestroyScene() {
    if (!scene) {
        return;
    }

    scene->Shutdown();
    SAFE_DELETE(scene);

    sceneIndex = -1;
}

void App::CheckModifiedCVars() {
    const bool kernelSourceModified = app_convolutionBloomKernelSource.IsModified();
    if (kernelSourceModified) {
        app_convolutionBloomKernelSource.ClearModified();
    }

    const bool kernelPresetModified = app_convolutionBloomKernelPreset.IsModified();
    if (kernelPresetModified) {
        app_convolutionBloomKernelPreset.ClearModified();
    }

    const bool aperturePresetModified = app_convolutionBloomAperturePreset.IsModified();
    if (aperturePresetModified) {
        app_convolutionBloomAperturePreset.ClearModified();
    }

    if (app_convolutionBloomKernelSource.GetInteger() == static_cast<int>(App::ConvolutionBloomKernelSource::Preset) && (kernelSourceModified || kernelPresetModified)) {
        SetConvolutionBloomKernelPreset(app_convolutionBloomKernelPreset.GetInteger());
    }

    if (app_convolutionBloomKernelSource.GetInteger() == static_cast<int>(App::ConvolutionBloomKernelSource::GenerateFromAperture) && (kernelSourceModified || aperturePresetModified)) {
        SetConvolutionBloomAperturePreset((App::AperturePreset)app_convolutionBloomAperturePreset.GetInteger());
        RequestConvolutionBloomKernelGeneration();
    }
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

    ProcessPendingConvolutionBloomKernelReadback();

    mainRenderContext->BeginFrame(frameTime, elapsedTime, elapsedPrevTime, statEnabled);

    uint64_t t0 = BE1::PlatformTime::Microseconds();

    renderSystem->UpdateFrame();

    renderWorld->BeginFrame();

    RenderScene();

    RenderGUI();

    renderWorld->EndFrame();

    if (pendingConvolutionBloomKernelGeneration) {
        EnqueuePendingConvolutionBloomKernelGeneration();
    }

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

void App::UpdateCamera(uint64_t frameUsec) {
    constexpr float CameraSmoothingSpeed = 18.0f;
    constexpr float CameraFlyFastSpeedScale = 4.0f;

    float dt = (float)MICRO2SEC((double)frameUsec);
    float alpha = 1.0f - BE1::Math::Exp(-CameraSmoothingSpeed * dt);

    if (cameraControlMode == CameraControlMode::Fly) {
        const BE1::Mat3 targetCameraAxis = BE1::Angles(0.0f, targetCameraPitch, targetCameraYaw).ToMat3();
        BE1::Vec3 moveDir = BE1::Vec3::zero;

        if (IsCameraMoveFlagSet(CameraMoveFlag::Forward)) {
            moveDir = moveDir + targetCameraAxis[0];
        }
        if (IsCameraMoveFlagSet(CameraMoveFlag::Backward)) {
            moveDir = moveDir - targetCameraAxis[0];
        }
        if (IsCameraMoveFlagSet(CameraMoveFlag::Left)) {
            moveDir = moveDir + targetCameraAxis[1];
        }
        if (IsCameraMoveFlagSet(CameraMoveFlag::Right)) {
            moveDir = moveDir - targetCameraAxis[1];
        }
        if (IsCameraMoveFlagSet(CameraMoveFlag::Up)) {
            moveDir = moveDir + targetCameraAxis[2];
        }
        if (IsCameraMoveFlagSet(CameraMoveFlag::Down)) {
            moveDir = moveDir - targetCameraAxis[2];
        }

        const float moveDirLengthSqr = moveDir.LengthSqr();
        if (moveDirLengthSqr > 0.0f) {
            const float speedScale = IsCameraMoveFlagSet(CameraMoveFlag::Fast) ? CameraFlyFastSpeedScale : 1.0f;
            moveDir = moveDir * BE1::Math::InvSqrt(moveDirLengthSqr);
            targetCameraOrigin = targetCameraOrigin + moveDir * (cameraFlySpeed * speedScale * dt);
        }
    }

    cameraYaw += (targetCameraYaw - cameraYaw) * alpha;
    cameraPitch += (targetCameraPitch - cameraPitch) * alpha;
    cameraDistance += (targetCameraDistance - cameraDistance) * alpha;

    if (cameraControlMode == CameraControlMode::Fly) {
        cameraOrigin = cameraOrigin + (targetCameraOrigin - cameraOrigin) * alpha;
    } else {
        cameraCenter = cameraCenter + (targetCameraCenter - cameraCenter) * alpha;
    }
}

void App::SetCameraControlMode(CameraControlMode mode) {
    if (cameraControlMode == mode) {
        return;
    }

    const BE1::Mat3 cameraAxis = BE1::Angles(0.0f, cameraPitch, cameraYaw).ToMat3();
    const BE1::Mat3 targetCameraAxis = BE1::Angles(0.0f, targetCameraPitch, targetCameraYaw).ToMat3();

    if (mode == CameraControlMode::Fly) {
        cameraOrigin = cameraCenter - cameraAxis[0] * cameraDistance;
        targetCameraOrigin = targetCameraCenter - targetCameraAxis[0] * targetCameraDistance;
    } else {
        cameraCenter = cameraOrigin + cameraAxis[0] * cameraDistance;
        targetCameraCenter = targetCameraOrigin + targetCameraAxis[0] * targetCameraDistance;
    }

    cameraControlMode = mode;
}

bool App::IsCameraMoveFlagSet(CameraMoveFlag flag) const {
    return (cameraMoveFlags & static_cast<uint32_t>(flag)) != 0;
}

void App::SetCameraMoveFlag(CameraMoveFlag flag, bool enabled) {
    const uint32_t flagBit = static_cast<uint32_t>(flag);
    if (enabled) {
        cameraMoveFlags |= flagBit;
    } else {
        cameraMoveFlags &= ~flagBit;
    }
}

void App::ClearCameraMoveFlags() {
    cameraMoveFlags = 0;
}

void App::AddCameraMouseDelta(int deltaX, int deltaY) {
    constexpr float MouseSensitivity = 0.16f;

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
    constexpr float MouseWheelZoomRatio = 0.08f;

    float wheelSteps = (float)delta / MouseWheelDelta;
    targetCameraDistance *= BE1::Math::Pow(1.0f - MouseWheelZoomRatio, wheelSteps);
    BE1::Clamp(targetCameraDistance, 1.0f, 1000.0f);
}

void App::GetCameraOriginAxis(BE1::Vec3 &outOrigin, BE1::Mat3 &outAxis) const {
    outAxis = BE1::Angles(0.0f, cameraPitch, cameraYaw).ToMat3();
    if (cameraControlMode == CameraControlMode::Fly) {
        outOrigin = cameraOrigin;
    } else {
        outOrigin = cameraCenter - outAxis[0] * cameraDistance;
    }
}

void App::TakeScreenshot() {
    BE1::cmdSystem.BufferCommandText(BE1::CmdSystem::Execution::Now, "screenshot");
}

void App::RenderScene() {
    uint32_t w = mainRenderContext->GetWidth();
    uint32_t h = mainRenderContext->GetHeight();
    float aspectRatio = (float)w / h;

    RenderCamera::Desc cameraDesc;
    cameraDesc.projectionPolicy = RHI::renderer->GetProjectionPolicy();
    cameraDesc.infiniteZFar = true;
    cameraDesc.clearMethod = RenderCamera::ClearMethod::Skybox;
    //cameraDesc.clearColorMode = RenderCamera::ClearColorMode::RenderTargetDefault;
    cameraDesc.orthogonal = false;
    cameraDesc.outputRect.Set(0, 0, w, h);
    cameraDesc.renderWidth = w;
    cameraDesc.renderHeight = h;
    GetCameraOriginAxis(cameraDesc.origin, cameraDesc.axis);
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

        const RenderContext::Stats renderStats = mainRenderContext->GetStats();

        const uint64_t statLightCullingZBinBytes = static_cast<uint64_t>(renderStats.lightCullingZBinCount) * sizeof(uint32_t);
        const uint64_t statLightCullingWordBytes = static_cast<uint64_t>(renderStats.lightCullingWordCount) * sizeof(uint32_t);

        mainRenderContext->DrawString(x, y, BE1::va("Frame: %.2f ms", statFrameAverageMsec), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += textLineHeight;
        mainRenderContext->DrawString(x, y, BE1::va("Game: %.2f ms", statGameAverageMsec), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += textLineHeight;
        mainRenderContext->DrawString(x, y, BE1::va("Render Frontend: %.2f ms", statRenderFrontendMsec), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += textLineHeight;
        mainRenderContext->DrawString(x, y, BE1::va("Render Backend: %.2f ms", statRenderBackendMsec), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += textLineHeight;
        mainRenderContext->DrawString(x, y, BE1::va("GPU: %.2f ms", statGpuMsec), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += textLineHeight;

        mainRenderContext->DrawString(x, y, BE1::va("Objects: %u, Draws: %u, Tris: %llu", renderStats.visibleObjectCount, renderStats.drawCount, renderStats.drawTriangleCount), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += textLineHeight;
        mainRenderContext->DrawString(x, y, BE1::va("Instance Tables: %u, Upload: %s", renderStats.instanceTableAllocCount, BE1::Str::FormatBytes(renderStats.instanceTableUploadBytes).c_str()), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += textLineHeight;

        mainRenderContext->DrawString(x, y, BE1::va("Lights: %u (Point: %u, Spot: %u, BoxDir: %u)", renderStats.visibleLightCount, renderStats.visiblePointLightCount, renderStats.visibleSpotLightCount, renderStats.visibleBoxDirLightCount), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += textLineHeight;
        mainRenderContext->DrawString(x, y, BE1::va("Light Culling Thread Groups: %llu", renderStats.lightCullingThreadGroupCount), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += textLineHeight;
        mainRenderContext->DrawString(x, y, BE1::va("Light Culling Z-bins: %u, Memory: %s", renderStats.lightCullingZBinCount, BE1::Str::FormatBytes(statLightCullingZBinBytes).c_str()), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += textLineHeight;
        mainRenderContext->DrawString(x, y, BE1::va("Light Culling Tile Masks: %u words, Memory: %s", renderStats.lightCullingWordCount, BE1::Str::FormatBytes(statLightCullingWordBytes).c_str()), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += textLineHeight;

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

void App::SetConvolutionBloomAperturePreset(App::AperturePreset aperturePreset) {
    char filename[128];
    switch (aperturePreset) {
    case App::AperturePreset::Triangle:
        BE1::Str::snPrintf(filename, sizeof(filename), "EngineTextures/Apertures/Triangle.png");
        break;
    case App::AperturePreset::Square:
        BE1::Str::snPrintf(filename, sizeof(filename), "EngineTextures/Apertures/Square.png");
        break;
    case App::AperturePreset::HorzRectangle:
        BE1::Str::snPrintf(filename, sizeof(filename), "EngineTextures/Apertures/RectangleHorz.png");
        break;
    case App::AperturePreset::VertRectangle:
        BE1::Str::snPrintf(filename, sizeof(filename), "EngineTextures/Apertures/RectangleVert.png");
        break;
    case App::AperturePreset::Pentagon:
        BE1::Str::snPrintf(filename, sizeof(filename), "EngineTextures/Apertures/Pentagon.png");
        break;
    case App::AperturePreset::Hexagon:
        BE1::Str::snPrintf(filename, sizeof(filename), "EngineTextures/Apertures/Hexagon.png");
        break;
    case App::AperturePreset::Heptagon:
        BE1::Str::snPrintf(filename, sizeof(filename), "EngineTextures/Apertures/Heptagon.png");
        break;
    case App::AperturePreset::Octagon:
        BE1::Str::snPrintf(filename, sizeof(filename), "EngineTextures/Apertures/Octagon.png");
        break;
    case App::AperturePreset::Circle:
        BE1::Str::snPrintf(filename, sizeof(filename), "EngineTextures/Apertures/Circle.png");
        break;
    default:
        return;
    }

    convolutionBloomApertureTexture = textureManager.GetTexture(filename, Texture::Flag::HighQuality | Texture::Flag::NoMipmaps);
    renderSystem->GetConvolutionBloomKernelGenerator().SetApertureTexture(convolutionBloomApertureTexture);
}

void App::RequestConvolutionBloomKernelGeneration() {
    pendingConvolutionBloomKernelGeneration = true;
}

void App::EnqueuePendingConvolutionBloomKernelGeneration() {
    if (app_convolutionBloomKernelSource.GetInteger() != static_cast<int>(App::ConvolutionBloomKernelSource::GenerateFromAperture)) {
        pendingConvolutionBloomKernelGeneration = false;
        return;
    }

    if (!convolutionBloomApertureTexture || !convolutionBloomApertureTexture->IsReadyForRendering()) {
        return;
    }

    RenderFrameData *frameData = mainRenderContext->GetCurrentFrontendFrameData();
    frameData->CmdGenerateConvolutionBloomKernel(r_convolutionBloomApertureScale.GetFloat(), r_convolutionBloomDiffractionPSFScale.GetFloat(), r_convolutionBloomDispersionAmount.GetFloat());

    pendingConvolutionBloomKernelGeneration = false;
    pendingConvolutionBloomKernelReadbackFrameData = frameData;
}

void App::ProcessPendingConvolutionBloomKernelReadback() {
    if (!pendingConvolutionBloomKernelReadbackFrameData) {
        return;
    }

    if (app_convolutionBloomKernelSource.GetInteger() != static_cast<int>(App::ConvolutionBloomKernelSource::GenerateFromAperture)) {
        pendingConvolutionBloomKernelReadbackFrameData = nullptr;
        return;
    }

    renderSystem->WaitFrameRenderCompleted(pendingConvolutionBloomKernelReadbackFrameData);

    BE1::Image kernelImage;
    if (!renderSystem->GetConvolutionBloomKernelGenerator().ReadKernelImage(kernelImage)) {
        BE_WARNLOG("App::ProcessPendingConvolutionBloomKernelReadback: failed to read generated convolution bloom kernel\n");
        pendingConvolutionBloomKernelReadbackFrameData = nullptr;
        return;
    }

    char kernelTextureName[128];
    BE1::Str::snPrintf(kernelTextureName, sizeof(kernelTextureName), "_Generated/ConvolutionBloom/ApertureKernel_%u", ++convolutionBloomGeneratedKernelSerial);
    if (renderCamera->SetConvolutionBloomKernelFromImage(kernelTextureName, kernelImage)) {
        convolutionBloomGeneratedKernelTexture = renderCamera->GetFFTBloomKernelTexture();
    } else {
        BE_WARNLOG("App::ProcessPendingConvolutionBloomKernelReadback: failed to create generated convolution bloom kernel texture\n");
    }

    pendingConvolutionBloomKernelReadbackFrameData = nullptr;
}

void App::GetConvolutionBloomKernelPresetName(int presetIndex, char *buffer, int bufferSize) const {
    BE1::Str::snPrintf(buffer, bufferSize, "CGV_Bloom_%02i.EXR", presetIndex + 1);
}

void App::SetConvolutionBloomKernelPreset(int presetIndex) {
    char filename[128];
    GetConvolutionBloomKernelPresetName(presetIndex, filename, sizeof(filename));

    BE1::Str filePath;
    filePath.AppendPath("BloomKernels");
    filePath.AppendPath(filename);

    if (renderCamera->SetConvolutionBloomKernelFromFile(filePath)) {
        convolutionBloomGeneratedKernelTexture.reset();
    }
}
