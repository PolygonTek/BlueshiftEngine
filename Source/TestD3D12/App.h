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

#pragma once

#include "RHI.h"

class RenderContext;
class RenderWorld;
class RenderCamera;
class GameObject;
class Mesh;
class Font;

class App {
public:
    void                            Init(void *mainWindowHandle);
    void                            Shutdown();

    void                            RunFrame(int frameMsec);
    void                            Render();

    void                            OnResize(int width, int height);

    void                            InitGameObjects();
    void                            ClearGameObjects();
    void                            UpdateGameObjects();

    RenderContext *                 GetMainRenderContext() const { return mainRenderContext; }

    void                            TakeScreenshot();

private:
    void                            UpdateCubes();

    RenderContext *                 mainRenderContext = nullptr;
    Font *                          mainFont = nullptr;
    RenderCamera *                  renderCamera = nullptr;
    RenderWorld *                   renderWorld = nullptr;

    BE1::Array<GameObject *>        gameObjects;

    int                             elapsedMsec = 0;
    int                             fps = 0;
};

extern App                          app;
