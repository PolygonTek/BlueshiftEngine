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

#include "RHI.h"
#include "RenderContext.h"
#include "RenderObject.h"
#include "RenderWorld.h"

class GameObject;
class TriangleMesh;
class CubeMesh;

class App {
public:
    void                            Init();
    void                            Shutdown();

    void                            RunFrame(int elapsedMsec);

    int                             GetElapsedMsec() const { return elapsedMsec; }

    RenderContext *                 CreateRenderContext(HWND hwnd);
    void                            DestroyRenderContext(RenderContext *renderContext);

    RenderWorld *                   AllocRenderWorld();
    void                            FreeRenderWorld(RenderWorld *renderWorld);

    void                            ClearGameObjects();

    void                            InitGameObjects();
    void                            InitTriangles();
    void                            InitCubes();

    void                            UpdateGameObjects();
    void                            UpdateTriangles();
    void                            UpdateCubes();

    RenderContext *                 mainRenderContext = nullptr;

    BE1::Array<GameObject *>        gameObjects;

    RenderWorld *                   renderWorld = nullptr;

    std::shared_ptr<TriangleMesh>   triangleMesh;
    std::shared_ptr<CubeMesh>       cubeMesh;

    int                             elapsedMsec = 0;
};

extern App                          app;
