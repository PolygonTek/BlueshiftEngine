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

Application::Application() {
    mainRenderContext = nullptr;
    gameWorld = nullptr;
    state = nullptr;
}

static void RegisterApp(LuaCpp::Module &module) {
    LuaCpp::Selector _App = module["App"];

    _App.SetObj(app);
    _App.AddObjMembers(app,
        "load_map", &Application::LoadMap);
}

void Application::Init() {
    BE1::cmdSystem.AddCommand(L"map", Cmd_Map_f);

    BE1::prefabManager.Init();

    BE1::GameSettings::Init();

    gameWorld = (BE1::GameWorld *)BE1::GameWorld::CreateInstance();
    gameWorld->SetDebuggable(true);
    gameWorld->GetLuaVM().RegisterEngineModuleCallback(RegisterApp);
    gameWorld->GetLuaVM().InitEngineModule(gameWorld);

    BE1::GameSettings::LoadSettings(gameWorld);
}

void Application::Shutdown() {
    if (gameWorld->IsGameStarted()) {
        gameWorld->StopGame();
    }

    sandbox = LuaCpp::Selector();

    BE1::prefabManager.Shutdown();

    BE1::GameWorld::DestroyInstance(gameWorld, true);

    BE1::GameSettings::Shutdown();

    BE1::cmdSystem.RemoveCommand(L"map");
}

bool Application::LoadAppScript(const char *sandboxName) {
    state = &gameWorld->GetLuaVM().State();

    const BE1::Guid appScriptGuid = BE1::GameSettings::playerSettings->GetProperty("appScript").As<BE1::Guid>();
    const BE1::Str appScriptPath = BE1::resourceGuidMapper.Get(appScriptGuid);

    char *data;
    size_t size = BE1::fileSystem.LoadFile(appScriptPath, true, (void **)&data);
    if (!data) {
        return false;
    }

    BE1::Str name = appScriptPath;

    if (!state->LoadBuffer(name.c_str(), data, size, sandboxName)) {
        BE1::fileSystem.FreeFile(data);
        return false;
    }

    BE1::fileSystem.FreeFile(data);

    state->Run();
    
    sandbox = (*state)[sandboxName];

    return true;
}

void Application::StartAppScript() {
    LuaCpp::Selector startFunc = sandbox["start"];
    if (startFunc.IsFunction()) {
        startFunc();
    }
}

void Application::OnApplicationResize(int width, int height) {
    if (gameWorld) {
        gameWorld->OnApplicationResize(width, height);
    }
}

void Application::OnApplicationTerminate() {
    if (gameWorld) {
        gameWorld->OnApplicationTerminate();
    }
}

void Application::OnApplicationPause(bool pause) {
    if (gameWorld) {
        gameWorld->OnApplicationPause(pause);
    }
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

    gameWorld->Render();

    gameWorld->GetPhysicsWorld()->DebugDraw();

    BE1::gameClient.DrawConsole();

    mainRenderContext->EndFrame();
}

void Application::LoadMap(const char *mapName) {
    gameWorld->LoadMap(mapName, BE1::GameWorld::LoadSceneMode::Single);

    gameWorld->StartGame();
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
