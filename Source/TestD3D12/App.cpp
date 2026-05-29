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
#include "Render/RenderContext.h"
#include "Render/RenderWorld.h"
#include "Render/RenderCamera.h"
#include "Render/Texture.h"
#include "Render/MeshInstance.h"
#include "Render/Skeleton.h"
#include "Render/Anim.h"
#include "Render/Font.h"
#include "LoadingSystem.h"
#include "GameObject.h"
#include "App.h"

static constexpr int        CubeDimensionX = 100;//64;
static constexpr int        CubeDimensionY = 100;//64;
static constexpr int        CubeDimensionZ = 1;
static constexpr int        CubeCount = CubeDimensionX * CubeDimensionY * CubeDimensionZ;
static constexpr float      CubeSpacing = 1.414214f;
static constexpr float      CubeStartX = -CubeSpacing * (CubeDimensionX - 1) * 0.5f;
static constexpr float      CubeStartY = -CubeSpacing * (CubeDimensionY - 1) * 0.5f;
static constexpr float      CubeStartZ = -CubeSpacing * (CubeDimensionZ - 1) * 0.5f;
static constexpr uint32_t   StatUpdateIntervalMsec = 500;
static constexpr uint32_t   StatUpdateSampleCount = 60;

static BE1::CVar            stat_fps("stat_fps", "1", BE1::CVar::Flag::Bool, "show FPS");
static BE1::CVar            stat_unit("stat_unit", "1", BE1::CVar::Flag::Bool, "show frame/render/GPU timing statistics");

App                         app;

void App::Init(void *mainWindowHandle) {
    renderSystem = new RenderSystem;
    renderSystem->Init(mainWindowHandle);

    mainRenderContext = renderSystem->CreateRenderContext(mainWindowHandle, true);

    loadingSystem.Init();

    mainFont = fontManager.GetFont("Data/EngineFonts/Galmuri11-Bold.ttf", 22);

    renderWorld = new RenderWorld;
    renderCamera = new RenderCamera;

    InitGameObjects();

    cameraYaw = targetCameraYaw;
    cameraPitch = targetCameraPitch;
    cameraDistance = targetCameraDistance;
}

void App::Shutdown() {
    ClearGameObjects();

    SAFE_DELETE(renderCamera);
    SAFE_DELETE(renderWorld);

    mainFont.reset();

    loadingSystem.Shutdown();

    renderSystem->DestroyRenderContext(mainRenderContext);

    renderSystem->Shutdown();
    delete renderSystem;
}

void App::UpdateFrameTime(int frameMsec) {
    statFrameElapsedMsec += frameMsec;
    statFrameCount++;

    if (statFrameElapsedMsec >= StatUpdateIntervalMsec) {
        statFrameAverageMsec = (float)statFrameElapsedMsec / statFrameCount;

        statFrameCount = 0;
        statFrameElapsedMsec = 0;
    }
}

void App::UpdateGameFrameTime(uint64_t frameUsec) {
    statGameElapsedUsec += frameUsec;
    statGameFrameCount++;

    if (statGameFrameCount >= StatUpdateSampleCount) {
        statGameAverageMsec = (float)statGameElapsedUsec / statGameFrameCount * 0.001f;

        statGameElapsedUsec = 0;
        statGameFrameCount = 0;
    }
}

void App::Update(int frameMsec) {
    PROFILER_CPU_SCOPED_EVENT("App::Update");

    uint64_t t0 = BE1::PlatformTime::Microseconds();

    loadingSystem.ProcessLoadCompletions();

    elapsedMsec += frameMsec;

    UpdateGameObjects();

    UpdateCamera(frameMsec);

    BE1::cmdSystem.ExecuteCommandBuffer();

    uint64_t t1 = BE1::PlatformTime::Microseconds();
    UpdateGameFrameTime(t1 - t0);
}

void App::Render() {
    PROFILER_CPU_SCOPED_EVENT("App::Render");

    const bool statEnabled = stat_unit.GetBool();

    mainRenderContext->BeginFrame(statEnabled);

    renderSystem->UpdateFrame();

    //renderWorld->ClearDebugLines();

    float w = mainRenderContext->GetWidth();
    float h = mainRenderContext->GetHeight();
    float aspectRatio = w / h;

    RenderCameraDesc cameraDesc;
    cameraDesc.orthogonal = false;
    cameraDesc.renderRect.Set(0, 0, w, h);
    cameraDesc.axis = BE1::Angles(0.0f, cameraPitch, cameraYaw).ToMat3();
    cameraDesc.origin = BE1::Vec3::origin - cameraDesc.axis[0] * cameraDistance;
    RenderCamera::CalculateFov(60, 1.7778f, aspectRatio, &cameraDesc.fovX, &cameraDesc.fovY);
    cameraDesc.zNear = BE1::MeterToUnit(0.5f);
    cameraDesc.zFar = BE1::MeterToUnit(1000.0f);
    if (cameraDesc.orthogonal) {
        cameraDesc.sizeX = cameraDistance * BE1::Math::Tan(cameraDesc.fovX * 0.5f) * 0.5f;
        cameraDesc.sizeY = cameraDesc.sizeX / aspectRatio;
    }
    renderCamera->Update(cameraDesc);

    textureManager.UpdateFrame();

    meshManager.UpdateFrame();

    skeletonManager.UpdateFrame();

    animManager.UpdateFrame();

    fontManager.UpdateFrame();

    renderWorld->RenderScene(renderCamera);

    mainRenderContext->SetFont(mainFont);
    mainRenderContext->SetColor(BE1::Color4::white);
    mainRenderContext->SetTextShadowColor(BE1::Color4::black);
    mainRenderContext->SetTextShadowOffset(1, 1);

    int y = 5;

    if (stat_fps.GetBool()) {
        const float statFps = statFrameAverageMsec > 0.0f ? 1000.0f / statFrameAverageMsec : 0.0f;

        mainRenderContext->DrawString(0, y, BE1::va("FPS: %.1f", statFps), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += 26;
    }

    if (statEnabled) {
        const uint64_t videoMemBudgetBytes = renderSystem->GetVideoMemoryUsage().budgetBytes;
        const uint64_t videoMemUsageBytes = renderSystem->GetVideoMemoryUsage().usageBytes;

        const uint64_t textureStreamingBudgetBytes = textureManager.GetStreamingTextureMemoryBudgetBytes();
        const uint64_t textureStreamingUsageBytes = textureManager.GetStreamingTextureMemoryUsageBytes();

        const float statRenderMsec = renderSystem->GetStatRenderAverageMsec();
        const float statGpuMsec = mainRenderContext->GetStatGpuAverageMsec();

        mainRenderContext->DrawString(0, y, BE1::va("Frame: %.2f ms", statFrameAverageMsec), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += 26;
        mainRenderContext->DrawString(0, y, BE1::va("Game: %.2f ms", statGameAverageMsec), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += 26;
        mainRenderContext->DrawString(0, y, BE1::va("Render: %.2f ms", statRenderMsec), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += 26;
        mainRenderContext->DrawString(0, y, BE1::va("GPU: %.2f ms", statGpuMsec), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += 26;
        mainRenderContext->DrawString(0, y, BE1::va("Video Memory: %s / %s", BE1::Str::FormatBytes(videoMemUsageBytes).c_str(), BE1::Str::FormatBytes(videoMemBudgetBytes).c_str()), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += 26;
        mainRenderContext->DrawString(0, y, BE1::va("Mipmap Streaming: %s / %s", BE1::Str::FormatBytes(textureStreamingUsageBytes).c_str(), BE1::Str::FormatBytes(textureStreamingBudgetBytes).c_str()), DrawTextFlag::Right | DrawTextFlag::AddOutlines); y += 26;
    }

    renderWorld->RenderGUI(mainRenderContext->GetGuiMesh());

    mainRenderContext->EndFrame();
}

void App::OnResize(int width, int height) {
    if (mainRenderContext) {
        mainRenderContext->OnResize(width, height);
    }
}

void App::InitGameObjects() {
    // 텍스쳐 파일들
    BE1::Str textureFilenames[] = {
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

    gameObjects.Reserve(CubeCount);

    testMesh = meshManager.GetMesh("_defaultBoxMesh");

    // 큐브
    for (int i = 0; i < CubeCount; ++i) {
        GameObject *gameObject = new GameObject;
        gameObjects.Append(gameObject);

        RenderObject::Desc &desc = gameObject->renderObjectDesc;
        desc.worldMatrix.SetIdentity();
        desc.meshInstance = testMesh->GetStaticInstance();
        desc.aabb = testMesh->GetAABB();
        desc.textures.SetCount(1);
        desc.textures[0] = textureManager.GetTexture(textureFilenames[BE1::Math::RandomRange(0, COUNT_OF(textureFilenames) - 1)], Texture::Flag::Streaming);

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

    UpdateCubes();
}

void App::ClearGameObjects() {
    for (GameObject *gameObject : gameObjects) {
        renderWorld->RemoveRenderObject(gameObject->renderObjectHandle);
    }
    gameObjects.DeleteContents(true);

    testMesh.reset();
}

void App::UpdateGameObjects() {
    PROFILER_CPU_SCOPED_EVENT("App::UpdateGameObjects");

    UpdateCubes();
}

void App::UpdateCamera(int frameMsec) {
    constexpr float CameraSmoothingSpeed = 18.0f;

    float dt = MILLI2SEC(frameMsec);
    float alpha = 1.0f - BE1::Math::Exp(-CameraSmoothingSpeed * dt);

    cameraYaw += (targetCameraYaw - cameraYaw) * alpha;
    cameraPitch += (targetCameraPitch - cameraPitch) * alpha;
    cameraDistance += (targetCameraDistance - cameraDistance) * alpha;
}

void App::AddCameraMouseDelta(int deltaX, int deltaY) {
    constexpr float MouseSensitivity = 0.18f;

    targetCameraYaw -= deltaX * MouseSensitivity;
    targetCameraPitch += deltaY * MouseSensitivity;
    BE1::Clamp(targetCameraPitch, -90.0f, 90.0f);
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

void App::UpdateCubes() {
    float elapsedSeconds = MILLI2SEC(elapsedMsec);

    float zRotationSpeed = 0.25f;

    for (int z = 0; z < CubeDimensionZ; ++z) {
        for (int y = 0; y < CubeDimensionY; ++y) {
            for (int x = 0; x < CubeDimensionX; ++x) {
                int index = CubeDimensionX * CubeDimensionY * z + CubeDimensionX * y + x;

                float t = elapsedSeconds + index * 0.1f;

                GameObject *gameObject = gameObjects[index];
                gameObject->renderObjectDesc.worldMatrix.SetTranslationRotation(BE1::Vec3(CubeStartX + CubeSpacing * x, CubeStartY + CubeSpacing * y, CubeStartZ + CubeSpacing * z), BE1::Mat3::FromRotationZ(t * zRotationSpeed), false);

                renderWorld->UpdateRenderObject(gameObject->renderObjectHandle, gameObject->renderObjectDesc, RenderObject::Desc::UpdateFlag::Transform);
            }
        }
    }
}
