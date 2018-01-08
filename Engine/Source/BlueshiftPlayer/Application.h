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

#pragma once

class Application {
public:
    Application();

    void                    Init();
    void                    Shutdown();

    void                    Update();

    void                    Draw();

    void                    LoadMap(const char *mapName);

    bool                    LoadAppScript(const char *sandboxName);
    void                    StartAppScript();
    
    void                    OnApplicationPause(bool pause);
    void                    OnApplicationTerminate();

    BE1::RenderContext *    mainRenderContext;

    BE1::GameWorld *        gameWorld;

    LuaCpp::State *         state;
    LuaCpp::Selector        sandbox;
    
    static void             Cmd_Map_f(const BE1::CmdArgs &args);
};

extern Application          app;
