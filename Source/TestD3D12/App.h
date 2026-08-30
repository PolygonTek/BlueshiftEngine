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
class Scene;
class Texture;
class Font;

class App {
public:
    enum class CameraControlMode {
        Orbit,
        Fly
    };

    enum class CameraMoveFlag : uint32_t {
        Forward                     = BIT(0),
        Backward                    = BIT(1),
        Left                        = BIT(2),
        Right                       = BIT(3),
        Up                          = BIT(4),
        Down                        = BIT(5),
        Fast                        = BIT(6)
    };

    enum class ConvolutionBloomKernelSource {
        Preset,
        GenerateFromAperture,
        ExternalFile
    };

    enum class AperturePreset {
        Triangle,
        Square,
        HorzRectangle,
        VertRectangle,
        Pentagon,
        Hexagon,
        Heptagon,
        Octagon,
        Circle
    };

    static constexpr int            ConvolutionBloomKernelPresetCount = 43;

    void                            Init(void *mainWindowHandle);
    void                            Shutdown();

    void                            Update(uint64_t frameUsec);
    void                            Render(uint64_t frameUsec);

    void                            UpdateFrameTime(uint64_t frameUsec);

    void                            OnResize(int width, int height);
    void                            OnDisplayChange();

    void                            UpdateCamera(uint64_t frameUsec);

    RenderContext *                 GetMainRenderContext() const { return mainRenderContext; }

    CameraControlMode               GetCameraControlMode() const { return cameraControlMode; }
    void                            SetCameraControlMode(CameraControlMode mode);
    bool                            IsCameraMoveFlagSet(CameraMoveFlag flag) const;
    void                            SetCameraMoveFlag(CameraMoveFlag flag, bool enabled);
    void                            ClearCameraMoveFlags();

    void                            AddCameraMouseDelta(int deltaX, int deltaY);
    void                            AddCameraPanDelta(int deltaX, int deltaY);
    void                            AddCameraDistance(int delta);
    void                            SetCameraPitchYaw(float pitch, float yaw);
    void                            SetCameraDistance(float distance);

    void                            GetCameraOriginAxis(BE1::Vec3 &origin, BE1::Mat3 &axis) const;

    void                            TakeScreenshot();

private:
    enum class State : uint8_t {
        Bootstrapping,
        BootstrapReady,
        Running,
        BootstrapFailed
    };

    void                            FinishBootstrap();
    void                            RenderRunningFrame(uint64_t frameUsec);
    void                            RenderBootstrapFrame(uint64_t frameUsec);
    void                            SetConvolutionBloomAperturePreset(AperturePreset aperturePreset);
    void                            RegenerateConvolutionBloomKernel();
    void                            ProcessConvolutionBloomKernelGenerationResult();
    void                            CheckModifiedCVars();
    void                            GetConvolutionBloomKernelPresetName(int presetIndex, char *buffer, int bufferSize) const;
    void                            SetConvolutionBloomKernelPreset(int presetIndex);
    void                            RequestSceneChange(int sceneIndex);
    void                            ProcessPendingSceneChange();
    void                            ChangeScene(int sceneIndex);
    void                            DestroyScene();
    void                            ResetCamera();
    void                            UpdateGameFrameTime(uint64_t frameUsec);
    void                            UpdateRenderFrontendFrameTime(uint64_t frameUsec);
    void                            RenderPendingCaptureRequests();
    void                            RenderBootstrapScreen();
    void                            RenderScene();
    void                            RenderGUI();

#ifdef ENABLE_IMGUI
    void                            BuildImGuiOverlay();
    void                            BuildImGuiTextureViewer();
    void                            UpdateImGuiStatsPlotHistory();
    void                            BuildImGuiStatsPlotWindow();

    bool                            pendingScreenshotCapture = false;
    bool                            pendingEnvShotCapture = false;
    bool                            pendingDiffuseIrradianceEnvShotCapture = false;
    bool                            pendingSpecularPrefilteredEnvShotCapture = false;
    bool                            pendingDFGSumGGXGeneration = false;
#endif

    RenderContext *                 mainRenderContext = nullptr;
    RenderCamera *                  renderCamera = nullptr;
    RenderWorld *                   renderWorld = nullptr;
    Scene *                         scene = nullptr;
    int                             sceneIndex = -1;
    int                             pendingSceneIndex = -1;
    State                           state = State::Bootstrapping;

    SharedPtr<Font>                 mainFont;
    SharedPtr<Texture>              convolutionBloomApertureTexture;
    SharedPtr<Texture>              convolutionBloomGeneratedKernelTexture;

    bool                            convolutionBloomKernelNeedsRegeneration = false;
    uint64_t                        convolutionBloomKernelGenerationRequestSerial = 0;
    uint32_t                        convolutionBloomGeneratedKernelSerial = 0;

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
    float                           targetCameraDistance = 300.0f;
    BE1::Vec3                       targetCameraCenter = BE1::Vec3::origin;
    BE1::Vec3                       targetCameraOrigin = BE1::Vec3::origin;

    float                           cameraYaw;
    float                           cameraPitch;
    float                           cameraDistance;
    BE1::Vec3                       cameraCenter = BE1::Vec3::origin;
    BE1::Vec3                       cameraOrigin = BE1::Vec3::origin;

    CameraControlMode               cameraControlMode = CameraControlMode::Orbit;
    uint32_t                        cameraMoveFlags = 0;
    float                           cameraFlySpeed = 30.0f;

#ifdef ENABLE_IMGUI
    static constexpr int            ImGuiStatsPlotSeriesCount = 5;
    static constexpr int            ImGuiStatsPlotHistoryLength = 300;

    bool                            imguiShowDemoWindow = false;
    bool                            imguiShowStatsPlotWindow = false;
    float                           imguiStatsPlotSamples[ImGuiStatsPlotSeriesCount][ImGuiStatsPlotHistoryLength] = {};
    int                             imguiStatsPlotWriteIndex = 0;
    int                             imguiStatsPlotSampleCount = 0;
#endif
};

extern App                          app;
