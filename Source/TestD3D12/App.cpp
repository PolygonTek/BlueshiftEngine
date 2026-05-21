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
#include "Render/Mesh.h"
#include "Render/Texture.h"
#include "Render/Font.h"
#include "LoadingSystem.h"
#include "GameObject.h"
#include "App.h"

static constexpr int        CubeDimensionX = 64;//105;
static constexpr int        CubeDimensionY = 64;//68;
static constexpr int        CubeCount = CubeDimensionX * CubeDimensionY;
static constexpr float      CubeSpacing = 1.5538;
static constexpr float      CubeStartX = -CubeSpacing * (CubeDimensionX - 1) * 0.5f;
static constexpr float      CubeStartY = -CubeSpacing * (CubeDimensionY - 1) * 0.5f;

App                         app;

void App::Init(void *mainWindowHandle) {
    renderSystem = new RenderSystem;
    renderSystem->Init(mainWindowHandle);

    mainRenderContext = renderSystem->CreateRenderContext(mainWindowHandle, true);

    loadingSystem.Init();

    mainFont = fontManager.GetFont("Data/EngineFonts/Galmuri11-Bold.ttf", 20);

    renderWorld = new RenderWorld;
    renderCamera = new RenderCamera;

    InitGameObjects();
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

void App::RunFrame(int frameMsec) {
    PROFILER_CPU_SCOPED_EVENT("App::RunFrame");

    loadingSystem.ProcessLoadCompletions();

    static int fpsElapsedMsec = 0;
    static int fpsFrames = 0;

    fpsElapsedMsec += frameMsec;
    fpsFrames++;

    if (fpsElapsedMsec >= 250) {
        fps = fpsFrames / MILLI2SEC(fpsElapsedMsec);
        fpsFrames = 0;
        fpsElapsedMsec = 0;
    }

    elapsedMsec += frameMsec;

    UpdateGameObjects();

    BE1::cmdSystem.ExecuteCommandBuffer();
}

void App::Render() {
    PROFILER_CPU_SCOPED_EVENT("App::Render");

    mainRenderContext->BeginFrame();

    renderSystem->UpdateFrame();

    //renderWorld->ClearDebugLines();

    float w = mainRenderContext->GetWidth();
    float h = mainRenderContext->GetHeight();
    float aspectRatio = w / h;
    float cameraX = 4 + (BE1::Math::Sin(MILLI2SEC(elapsedMsec) * 0.5f) + 1.0f) * 0.5f * 162;

    RenderCameraDesc cameraDesc;
    cameraDesc.orthogonal = false;
    cameraDesc.renderRect.Set(0, 0, w, h);
    cameraDesc.origin.Set(cameraX, 0, 0);
    cameraDesc.axis[0].Set(-1, 0, 0);
    cameraDesc.axis[1].Set(0, -1, 0);
    cameraDesc.axis[2].Set(0, 0, 1);
    RenderCamera::CalculateFov(60, 1.7778f, aspectRatio, &cameraDesc.fovX, &cameraDesc.fovY);
    cameraDesc.zNear = BE1::MeterToUnit(0.5f);
    cameraDesc.zFar = BE1::MeterToUnit(1000.0f);
    if (cameraDesc.orthogonal) {
        cameraDesc.sizeX = cameraX * BE1::Math::Tan(cameraDesc.fovX * 0.5f) * 0.5f;
        cameraDesc.sizeY = cameraDesc.sizeX / aspectRatio;
    }
    renderCamera->Update(cameraDesc);

    textureManager.UpdateFrame();

    meshManager.UpdateFrame();

    fontManager.UpdateFrame();

    renderWorld->RenderScene(renderCamera);

    const uint64_t videoMemBudgetBytes = renderSystem->GetVideoMemoryUsage().budgetBytes;
    const uint64_t videoMemUsageBytes = renderSystem->GetVideoMemoryUsage().usageBytes;

    const uint64_t textureStreamingBudgetBytes = textureManager.GetStreamingTextureMemoryBudgetBytes();
    const uint64_t textureStreamingUsageBytes = textureManager.GetStreamingTextureMemoryUsageBytes();

    mainRenderContext->SetFont(mainFont);
    mainRenderContext->SetColor(BE1::Color4::white);
    mainRenderContext->SetTextShadowColor(BE1::Color4::black);
    mainRenderContext->SetTextShadowOffset(1, 1);
    mainRenderContext->DrawString(0, 5, BE1::va("FPS: %i (%f ms)", fps, 1000.0f / fps), DrawTextFlag::Right | DrawTextFlag::AddOutlines);
    mainRenderContext->DrawString(0, 25, BE1::va("Video Memory: %s / %s", BE1::Str::FormatBytes(videoMemUsageBytes).c_str(), BE1::Str::FormatBytes(videoMemBudgetBytes).c_str()), DrawTextFlag::Right | DrawTextFlag::AddOutlines);
    mainRenderContext->DrawString(0, 45, BE1::va("Mipmap Streaming: %s / %s", BE1::Str::FormatBytes(textureStreamingUsageBytes).c_str(), BE1::Str::FormatBytes(textureStreamingBudgetBytes).c_str()), DrawTextFlag::Right | DrawTextFlag::AddOutlines);

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
        "Data/Textures/Brick_Wall_Worn.dds",
        "Data/Textures/Construction_Gravel.dds",
        "Data/Textures/Dry_Cracked_Mud.dds",
        "Data/Textures/Flagstone_Wall.dds",
        "Data/Textures/Lawn_Grass.dds",
        "Data/Textures/Roman_Stone_Floor.dds",
        "Data/Textures/Stone_Floor.dds",
        "Data/Textures/Stone_Tiles_Facade.dds",
        "Data/Textures/Arid_Gravel_Ground.dds",
        "Data/Textures/yd0kejgga.dds",
        "Data/Textures/T_sc2nbisc.dds",
        "Data/Textures/Gouged_Rock_Cliff.dds",
        "Data/Textures/Scratched_Polypropylene_Plastic.dds"
    };

    gameObjects.Reserve(CubeCount);

    // 큐브
    for (int i = 0; i < CubeCount; ++i) {
        GameObject *gameObject = new GameObject;
        gameObjects.Append(gameObject);

        RenderObject::Desc &desc = gameObject->renderObjectDesc;
        desc.worldMatrix.SetIdentity();
        desc.mesh = meshManager.GetMesh("_defaultBoxMesh");
        desc.aabb = desc.mesh->GetAABB();
        desc.textures.SetCount(1);
        desc.textures[0] = textureManager.GetTexture(textureFilenames[BE1::Math::RandomRange(0, COUNT_OF(textureFilenames) - 1)], Texture::Flag::Streaming);

        gameObject->renderObjectHandle = renderWorld->AddRenderObject(gameObject->renderObjectDesc);
    }

    // Text 오브젝트
    /*GameObject *gameObject = new GameObject;
    gameObjects.Append(gameObject);

    RenderObject::Desc &desc = gameObject->renderObjectDesc;
    desc.worldMatrix.SetTRS(BE1::Vec3(-10, 0, 0), BE1::Mat3::identity, BE1::Vec3::one);
    desc.font = mainFont->AddRefCount();
    desc.text = "Hello World";
    desc.textParams.bits.textAnchor = (uint16_t)RenderObject::TextAnchor::MiddleCenter;

    gameObject->renderObjectHandle = renderWorld->AddRenderObject(gameObject->renderObjectDesc);*/

    UpdateCubes();
}

void App::ClearGameObjects() {
    for (GameObject *gameObject : gameObjects) {
        renderWorld->RemoveRenderObject(gameObject->renderObjectHandle);
    }
    gameObjects.DeleteContents(true);
}

void App::UpdateGameObjects() {
    PROFILER_CPU_SCOPED_EVENT("App::UpdateGameObjects");

    UpdateCubes();
}

void App::TakeScreenshot() {
    BE1::cmdSystem.BufferCommandText(BE1::CmdSystem::Execution::Now, "screenshot");
}

void App::UpdateCubes() {
    float elapsedSeconds = MILLI2SEC(elapsedMsec);

    for (int y = 0; y < CubeDimensionY; ++y) {
        for (int x = 0; x < CubeDimensionX; ++x) {
            int index = CubeDimensionX * y + x;

            float t = elapsedSeconds + index * 0.1f;

            GameObject *gameObject = gameObjects[index];
            gameObject->renderObjectDesc.worldMatrix.SetTranslationRotation(BE1::Vec3(0, CubeStartX + CubeSpacing * x, CubeStartY + CubeSpacing * y), BE1::Mat3::FromRotationZYX(t * 1.0f, 0, t * 0.25f), false);

            renderWorld->UpdateRenderObject(gameObject->renderObjectHandle, gameObject->renderObjectDesc, RenderObject::Desc::UpdateFlag::Transform);
        }
    }
}
