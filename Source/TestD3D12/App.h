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

    void                            Update(int frameMsec);
    void                            Render();

    void                            UpdateFrameTime(int frameMsec);

    void                            OnResize(int width, int height);

    void                            InitGameObjects();
    void                            ClearGameObjects();
    void                            UpdateGameObjects();
    void                            UpdateCamera(int frameMsec);

    RenderContext *                 GetMainRenderContext() const { return mainRenderContext; }

    void                            AddCameraMouseDelta(int deltaX, int deltaY);
    void                            AddCameraDistance(int delta);

    void                            TakeScreenshot();

private:
    void                            UpdateGameFrameTime(uint64_t frameUsec);
    void                            UpdateRenderFrontendFrameTime(uint64_t frameUsec);
    void                            UpdateCubes();

    RenderContext *                 mainRenderContext = nullptr;
    std::shared_ptr<Font>           mainFont;
    RenderCamera *                  renderCamera = nullptr;
    RenderWorld *                   renderWorld = nullptr;

    std::shared_ptr<Mesh>           testMesh;

    BE1::Array<GameObject *>        gameObjects;

    int                             elapsedMsec = 0;

    uint32_t                        statFrameElapsedMsec = 0;
    uint32_t                        statFrameCount = 0;
    float                           statFrameAverageMsec = 0;

    uint64_t                        statGameElapsedUsec = 0;
    uint32_t                        statGameFrameCount = 0;
    float                           statGameAverageMsec = 0.0f;

    uint64_t                        statRenderFrontendElapsedUsec = 0;
    uint32_t                        statRenderFrontendFrameCount = 0;
    float                           statRenderFrontendAverageMsec = 0.0f;

    float                           targetCameraYaw = 0.0f;
    float                           targetCameraPitch = 45.0f;
    float                           targetCameraDistance = 205.0f;

    float                           cameraYaw;
    float                           cameraPitch;
    float                           cameraDistance;
};

extern App                          app;
