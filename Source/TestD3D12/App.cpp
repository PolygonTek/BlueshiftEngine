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
#include "App.h"
#include "RenderSystem.h"
#include "RenderContext.h"
#include "RenderWorld.h"
#include "RenderCamera.h"
#include "GameObject.h"
#include "Mesh.h"
#include "Texture.h"

static constexpr int        CubeDimensionX = 64;//92;
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

    renderWorld = new RenderWorld;
    renderCamera = new RenderCamera;

    InitGameObjects();
}

void App::Shutdown() {
    RHI::renderer->Finish(RHI::CommandQueueType::Graphics);

    ClearGameObjects();

    SAFE_DELETE(renderCamera);
    SAFE_DELETE(renderWorld);

    renderSystem->DestroyRenderContext(mainRenderContext);

    renderSystem->Shutdown();
    delete renderSystem;
}

void App::RunFrame(int frameMsec) {
    PROFILER_CPU_SCOPED_EVENT("App::RunFrame", 0);

    elapsedMsec += frameMsec;

    UpdateGameObjects();

    RHI::renderer->FreePendingResources();

    BE1::cmdSystem.ExecuteCommandBuffer();
}

void App::Render() {
    PROFILER_CPU_SCOPED_EVENT("App::Render", 1);

    float w = mainRenderContext->GetWidth();
    float h = mainRenderContext->GetHeight();
    float aspectRatio = w / h;

    RenderCameraDesc cameraDesc;
    cameraDesc.orthogonal = false;
    cameraDesc.renderRect.Set(0, 0, w, h);
    cameraDesc.origin.Set(30 + (BE1::Math::Sin(MILLI2SEC(elapsedMsec) * 0.5f) + 1.0f) * 0.5f * 100, 0, 0);
    cameraDesc.axis[0].Set(-1, 0, 0);
    cameraDesc.axis[1].Set(0, -1, 0);
    cameraDesc.axis[2].Set(0, 0, 1);
    cameraDesc.fovY = 45;
    cameraDesc.fovX = cameraDesc.fovY * aspectRatio;
    cameraDesc.zNear = BE1::CmToUnit(10.0f);
    cameraDesc.zFar = BE1::MeterToUnit(1000.0f);
    renderCamera->Update(cameraDesc);

    mainRenderContext->BeginFrame();

    renderWorld->RenderScene(renderCamera);

    renderWorld->SetColor(BE1::Color4::brown);
    renderWorld->DrawBar(0, 0, 100, 100);
    renderWorld->SetColor(BE1::Color4::lightSkyBlue);
    renderWorld->DrawBar(100, 0, 100, 100);
    renderWorld->SetColor(BE1::Color4::green);
    renderWorld->DrawBar(200, 0, 100, 100);
    renderWorld->RenderGUI();

    mainRenderContext->EndFrame();
}

void App::InitGameObjects() {
    InitCubes();

    UpdateCubes();
}

void App::ClearGameObjects() {
    for (GameObject *gameObject : gameObjects) {
        renderWorld->RemoveRenderObject(gameObject->renderObjectHandle);

        meshManager.ReleaseMesh(gameObject->renderObjectDesc.mesh);
    }

    gameObjects.DeleteContents(true);
}

void App::UpdateGameObjects() {
    PROFILER_CPU_SCOPED_EVENT("App::UpdateGameObjects", 1);

    UpdateCubes();
}

void App::TakeScreenshot() {
    BE1::cmdSystem.BufferCommandText(BE1::CmdSystem::Execution::Now, "screenshot");
}

void App::InitCubes() {
    gameObjects.Reserve(CubeCount);

    for (int i = 0; i < CubeCount; ++i) {
        GameObject *gameObject = new GameObject;
        gameObjects.Append(gameObject);

        RenderObjectDesc &desc = gameObject->renderObjectDesc;
        desc.worldMatrix.SetIdentity();
        desc.mesh = meshManager.GetMesh("_defaultBoxMesh");
        desc.aabb = desc.mesh->GetAABB();
        desc.textures.SetCount(1);
        desc.textures[0] = textureManager.defaultTexture;

        gameObject->renderObjectHandle = renderWorld->AddRenderObject(gameObject->renderObjectDesc);
    }
}

void App::UpdateCubes() {
    float elapsedSeconds = MILLI2SEC(elapsedMsec);

    for (int y = 0; y < CubeDimensionY; ++y) {
        for (int x = 0; x < CubeDimensionX; ++x) {
            int index = CubeDimensionX * y + x;

            float t = elapsedSeconds + index * 0.1f;

            GameObject *gameObject = gameObjects[index];
            gameObject->renderObjectDesc.worldMatrix.SetTranslationRotation(BE1::Vec3(0, CubeStartX + CubeSpacing * x, CubeStartY + CubeSpacing * y), BE1::Mat3::FromRotationZYX(t * 1.0f, 0, t * 0.25f), false);

            renderWorld->UpdateRenderObject(gameObject->renderObjectHandle, gameObject->renderObjectDesc);
        }
    }
}
