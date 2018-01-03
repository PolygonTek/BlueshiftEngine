// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Application.h"

Application app;

void Application::Init() {
    BE1::cmdSystem.AddCommand(L"map", Cmd_Map_f);

    BE1::GameSettings::Init();
    
    gameFont = nullptr;
    //gameFont = BE1::fontManager.GetFont("fonts/NanumGothic.ttf");

    gameWorld = (BE1::GameWorld *)BE1::GameWorld::CreateInstance();

    BE1::GameSettings::LoadSettings(gameWorld);
    
    gameState = GAMESTATE_NOMAP;

    LoadMap("Contents/Maps/title.map");
}

void Application::Shutdown() {
    if (gameWorld->IsGameStarted()) {
        gameWorld->StopGame();
    }
    
    gameState = GAMESTATE_SHUTDOWN;

    BE1::GameWorld::DestroyInstance(gameWorld, true);
    gameWorld = nullptr;

    //BE1::fontManager.ReleaseFont(gameFont);

    BE1::GameSettings::Shutdown();

    BE1::cmdSystem.RemoveCommand(L"map");
}

void Application::OnApplicationTerminate() {
    gameWorld->OnApplicationTerminate();
}

void Application::OnApplicationPause(bool pause) {
    gameWorld->OnApplicationPause(pause);
}

void Application::Update() {
    gameWorld->Update(BE1::common.frameTime);

    if (!BE1::platform->IsCursorLocked()) {
        gameWorld->ProcessPointerInput();
    }
}

void Application::Draw() {
    gameWorld->GetRenderWorld()->ClearDebugPrimitives(BE1::common.realTime);
    gameWorld->GetRenderWorld()->ClearDebugText(BE1::common.realTime);

    mainRenderContext->BeginFrame();

    gameWorld->RenderCamera();

    gameWorld->GetPhysicsWorld()->DebugDraw();

    BE1::gameClient.DrawConsole();

    mainRenderContext->EndFrame();
}

void Application::LoadMap(const char *mapName) {
    gameState = GAMESTATE_STARTUP;

    this->mapName = mapName;

    gameWorld->LoadMap(mapName);

    gameWorld->StartGame();

    gameState = GAMESTATE_ACTIVE;
}

void Application::Cmd_Map_f(const BE1::CmdArgs &args) {
    if (args.Argc() != 2) {
        BE_LOG(L"map <filename>\n");
        return;
    }

    BE1::Str filename;
    filename.sPrintf("maps/%s", BE1::WStr::ToStr(args.Argv(1)).c_str());
    filename.DefaultFileExtension(".map");

    app.LoadMap(filename.c_str());	
}
