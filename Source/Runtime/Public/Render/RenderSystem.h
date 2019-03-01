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

BE_NAMESPACE_BEGIN

/*
-------------------------------------------------------------------------------

    Render System

-------------------------------------------------------------------------------
*/

class CmdArgs;
class VisCamera;
class ReflectionProbe;
class RenderSystem;

class ReflectionProbeJob {
    friend class RenderSystem;

public:
    RenderWorld *           GetRenderWorld() const { return renderWorld; }

    ReflectionProbe *       GetReflectionProbe() const { return reflectionProbe; }

    bool                    IsFinished() const;

    void                    Refresh();

private:
    void                    RevalidateDiffuseConvolutionCubemap();
    void                    RevalidateSpecularConvolutionCubemap();

    RenderWorld *           renderWorld;
    ReflectionProbe *       reflectionProbe;
    bool                    diffuseConvolutionCubemapComputed = false;
    int                     specularConvolutionCubemapComputedLevel = -1;
    int                     specularConvolutionCubemapComputedLevel0Face = -1;
    int                     specularConvolutionCubemapMaxLevel;
};

class RenderSystem {
    friend class RenderContext;
    friend class RenderWorld;
    friend class ReflectionProbeJob;
    friend class PhysDebugDraw;

public:
    RenderSystem();

    void                    Init(void *windowHandle, const RHI::Settings *settings);
    void                    Shutdown();

    bool                    IsInitialized() const { return initialized; }

    bool                    IsFullscreen() const;

    void                    SetGamma(double gamma);
    void                    RestoreGamma();

    RenderContext *         AllocRenderContext(bool isMainContext = false);
    void                    FreeRenderContext(RenderContext *renderContext);

                            // valid only between BeginFrame()/EndFrame()
    RenderContext *         GetCurrentRenderContext() { return currentContext; }

    RenderContext *         GetMainRenderContext() { return mainContext; }

    void                    BeginCommands(RenderContext *renderContext);
    void                    EndCommands();

    RenderWorld *           AllocRenderWorld();
    void                    FreeRenderWorld(RenderWorld *renderWorld);

    RenderWorld *           GetRenderWorld() const { return primaryWorld; }

    void                    CheckModifiedCVars();

    void                    UpdateReflectionProbes();

                            /// Schedule to refresh reflection probe in the next frame.
    void                    ScheduleToRefreshReflectionProbe(RenderWorld *renderWorld, int probeHandle);

    void                    TakeEnvShot(const char *filename, RenderWorld *renderWorld, int layerMask, int staticMask, const Vec3 &origin, int size = 256);
    void                    TakeIrradianceEnvShot(const char *filename, RenderWorld *renderWorld, int layerMask, int staticMask, const Vec3 &origin);
    void                    TakePrefilteredEnvShot(const char *filename, RenderWorld *renderWorld, int layerMask, int staticMask, const Vec3 &origin);

    void                    WriteGGXDFGSum(const char *filename, int size) const;

                            // Capture environment cubemap
    void                    CaptureEnvCubeRT(RenderWorld *renderWorld, int layerMask, int staticMask, const Vec3 &origin, float zNear, float zFar, RenderTarget *targetCubeRT);
    void                    CaptureEnvCubeRTFace(RenderWorld *renderWorld, int layerMask, int staticMask, const Vec3 &origin, float zNear, float zFar, RenderTarget *targetCubeRT, int faceIndex);

                            // Capture environment cubemap image
    void                    CaptureEnvCubeImage(RenderWorld *renderWorld, int layerMask, int staticMask, const Vec3 &origin, int size, Image &envCubeImage);

                            // Generate irradiance environment cubemap using SH convolution method
    void                    GenerateSHConvolvIrradianceEnvCubeRT(const Texture *envCubeTexture, RenderTarget *targetCubeRT) const;

                            // Generate irradiance environment cubemap
    void                    GenerateIrradianceEnvCubeRT(const Texture *envCubeTexture, RenderTarget *targetCubeRT) const;

                            // Generate Phong specular prefiltered environment cubemap
    void                    GeneratePhongSpecularLDSumRT(const Texture *envCubeTexture, int maxSpecularPower, RenderTarget *targetCubeRT) const;

                            // Generate GGX specular prefiltered environment cubemap
    void                    GenerateGGXLDSumRT(const Texture *envCubeTexture, RenderTarget *targetCubeRT) const;
    void                    GenerateGGXLDSumRTLevel(const Texture *envCubeTexture, RenderTarget *targetCubeRT, int numMipLevels, int mipLevel) const;

                            // Generate GGX DFG integration 2D LUT
    void                    GenerateGGXDFGSumImage(int size, Image &integrationImage) const;

private:
    void                    RecreateScreenMapRT();
    void                    RecreateHDRMapRT();
    void                    RecreateShadowMapRT();
    void *                  GetCommandBuffer(int bytes);
    void                    IssueCommands();

    void                    GenerateGGXLDSumRTFirstLevel(const Texture *envCubeTexture, RenderTarget *targetCubeRT) const;
    void                    GenerateGGXLDSumRTRestLevel(const Texture *envCubeTexture, RenderTarget *targetCubeRT, int numMipLevels, int mipLevel) const;

    void                    CmdDrawCamera(const VisCamera *camera);
    void                    CmdScreenshot(int x, int y, int width, int height, const char *filename);

    bool                    initialized;
    unsigned short          savedGammaRamp[768];

    RenderWorld *           primaryWorld;
    
    Array<RenderContext *>  renderContexts;
    RenderContext *         currentContext;
    RenderContext *         mainContext;

    Array<ReflectionProbeJob> reflectionProbeJobs;

    static void             Cmd_ScreenShot(const CmdArgs &args);
};

BE_INLINE RenderSystem::RenderSystem() {
    initialized = false;
}

extern RenderSystem         renderSystem;

BE_NAMESPACE_END
