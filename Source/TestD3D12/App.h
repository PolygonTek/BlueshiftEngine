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
class Texture;
class Mesh;
class Font;

class App {
public:
    void                            Init(void *mainWindowHandle);
    void                            Shutdown();

    void                            Update(uint64_t frameUsec);
    void                            Render(uint64_t frameUsec);

    void                            UpdateFrameTime(uint64_t frameUsec);

    void                            OnResize(int width, int height);

    void                            InitGameObjects(uint32_t countX, uint32_t countY, uint32_t countZ);
    void                            ClearGameObjects();
    void                            ChangeGameObjectCount(uint32_t countX, uint32_t countY, uint32_t countZ);
    void                            UpdateCamera(uint64_t frameUsec);

    RenderContext *                 GetMainRenderContext() const { return mainRenderContext; }

    void                            AddCameraMouseDelta(int deltaX, int deltaY);
    void                            AddCameraPanDelta(int deltaX, int deltaY);
    void                            AddCameraDistance(int delta);

    void                            TakeScreenshot();

private:
    static constexpr int            ConvolutionBloomKernelCount = 43;
    static constexpr int            DefaultConvolutionBloomKernelIndex = 0;

    int                             GetConvolutionBloomKernelCount() const { return ConvolutionBloomKernelCount; }
    int                             GetConvolutionBloomKernelIndex() const { return convolutionBloomKernelIndex; }
    void                            GetConvolutionBloomKernelDisplayName(int index, char *buffer, int bufferSize) const;
    void                            GetConvolutionBloomKernelFilename(int index, char *buffer, int bufferSize) const;
    void                            SetConvolutionBloomKernelIndex(int index);
    void                            SetConvolutionBloomKernelFromFile(const char *filename);
    void                            RotateGameObjects();
    void                            UpdateGameFrameTime(uint64_t frameUsec);
    void                            UpdateRenderFrontendFrameTime(uint64_t frameUsec);
    void                            UpdateGameObjects();
    void                            RenderScene();
    void                            RenderText();
#ifdef ENABLE_IMGUI
    void                            BuildImGuiOverlay();
    void                            BuildImGuiTextureViewer();
    void                            UpdateImGuiStatsPlotHistory();
    void                            BuildImGuiStatsPlotWindow();
#endif

    RenderContext *                 mainRenderContext = nullptr;
    SharedPtr<Font>                 mainFont;
    RenderCamera *                  renderCamera = nullptr;
    RenderWorld *                   renderWorld = nullptr;

    SharedPtr<Texture>              convolutionBloomKernelTexture;
    int                             convolutionBloomKernelIndex = DefaultConvolutionBloomKernelIndex;
    SharedPtr<Mesh>                 testMesh;

    BE1::Array<GameObject *>        gameObjects;

    uint64_t                        elapsedUsec = 0;

    uint64_t                        statFrameElapsedUsec = 0;
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
    BE1::Vec3                       targetCameraCenter = BE1::Vec3::origin;

    float                           cameraYaw;
    float                           cameraPitch;
    float                           cameraDistance;
    BE1::Vec3                       cameraCenter = BE1::Vec3::origin;

    uint32_t                        goCountX = 1;
    uint32_t                        goCountY = 1;
    uint32_t                        goCountZ = 1;

#ifdef ENABLE_IMGUI
    static constexpr int            ImGuiStatsPlotSeriesCount = 5;
    static constexpr int            ImGuiStatsPlotHistoryLength = 300;

    int                             imguiGoCountX = 1;
    int                             imguiGoCountY = 1;
    bool                            imguiShowDemoWindow = false;
    bool                            imguiShowStatsPlotWindow = false;
    float                           imguiStatsPlotSamples[ImGuiStatsPlotSeriesCount][ImGuiStatsPlotHistoryLength] = {};
    int                             imguiStatsPlotWriteIndex = 0;
    int                             imguiStatsPlotSampleCount = 0;
#endif
};

extern App                          app;
