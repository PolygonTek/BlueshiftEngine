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

#include "EnvProbe.h"

BE_NAMESPACE_BEGIN

/*
-------------------------------------------------------------------------------

    Render System

-------------------------------------------------------------------------------
*/

class CmdArgs;
class VisCamera;
class RenderSystem;

class EnvProbeJob {
    friend class RenderSystem;

public:
    RenderWorld *           GetRenderWorld() const { return renderWorld; }

    EnvProbe *              GetEnvProbe() const { return envProbe; }

    bool                    IsFinished() const;

    bool                    Refresh(EnvProbe::TimeSlicing::Enum timeSlicing);

private:
    void                    RevalidateDiffuseProbeRT(bool clearToBlack);
    void                    RevalidateSpecularProbeRT(bool clearToBlack);
    void                    RevalidateEnvProbeRT();
    void                    RevalidateEnvProbeTexture();

    RenderWorld *           renderWorld;
    EnvProbe *              envProbe;

    bool                    diffuseProbeCubemapComputed = false;
    int                     specularProbeCubemapComputedLevel = -1;
    int                     specularProbeCubemapComputedLevel0Face = -1;
    int                     specularProbeCubemapMaxLevel = 0;
};

class RenderSystem {
    friend class RenderContext;
    friend class RenderWorld;
    friend class EnvProbeJob;
    friend class PhysDebugDraw;

public:
    RenderSystem();

    void                    InitRHI(void *windowHandle);

    void                    Init();
    void                    Shutdown();

    bool                    IsInitialized() const { return initialized; }

    bool                    IsFullscreen() const;

    void                    SetGamma(double gamma);
    void                    RestoreGamma();

    RenderContext *         AllocRenderContext(bool isMainContext = false);
    void                    FreeRenderContext(RenderContext *renderContext);

                            /// This function is valid only between BeginFrame()/EndFrame()
    RenderContext *         GetCurrentRenderContext() { return currentContext; }

    RenderContext *         GetMainRenderContext() { return mainContext; }

    void                    BeginCommands(RenderContext *renderContext);
    void                    EndCommands();

    RenderWorld *           AllocRenderWorld();
    void                    FreeRenderWorld(RenderWorld *renderWorld);

    RenderWorld *           GetRenderWorld() const { return primaryWorld; }

    void                    CheckModifiedCVars();

                            /// Schedules to refresh environment probe in the next frame.
    void                    ScheduleToRefreshEnvProbe(RenderWorld *renderWorld, int probeHandle);

                            /// Forces to refresh environment probe immediately.
    void                    ForceToRefreshEnvProbe(RenderWorld *renderWorld, int probeHandle);

                            /// Captures screen.
    void                    CaptureScreenRT(RenderWorld *renderWorld, int layerMask, bool colorClear, const Color4 &clearColor, const Vec3 &origin, const Mat3 &axis, float fov, int width, int height, RenderTarget *targetRT);

                            /// Captures screen texture.
    Texture *               CaptureScreenTexture(RenderWorld *renderWorld, int layerMask, bool colorClear, const Color4 &clearColor, const Vec3 &origin, const Mat3 &axis, float fov, bool useHDR, int width, int height);

                            /// Capture screen image.
    void                    CaptureScreenImage(RenderWorld *renderWorld, int layerMask, bool colorClear, const Color4 &clearColor, const Vec3 &origin, const Mat3 &axis, float fov, bool useHDR, int width, int height, Image &screenImage);

                            /// Captures environment cubemap for specific face.
    void                    CaptureEnvCubeFaceRT(RenderWorld *renderWorld, int layerMask, int staticMask, bool colorClear, const Color4 &clearColor, const Vec3 &origin, float zNear, float zFar, RenderTarget *targetCubeRT, int faceIndex);

                            /// Captures environment cubemap.
    void                    CaptureEnvCubeRT(RenderWorld *renderWorld, int layerMask, int staticMask, bool colorClear, const Color4 &clearColor, const Vec3 &origin, float zNear, float zFar, RenderTarget *targetCubeRT);

                            /// Captures environment cubemap texture.
    Texture *               CaptureEnvCubeTexture(RenderWorld *renderWorld, int layerMask, int staticMask, bool colorClear, const Color4 &clearColor, const Vec3 &origin, float zNear, float zFar, bool useHDR, int size);

                            /// Captures environment cubemap image.
    void                    CaptureEnvCubeImage(RenderWorld *renderWorld, int layerMask, int staticMask, bool colorClear, const Color4 &clearColor, const Vec3 &origin, float zNear, float zFar, bool useHDR, int size, Image &envCubeImage);

                            /// Generates irradiance environment cubemap using SH convolution method.
    void                    GenerateSHConvolvIrradianceEnvCubeRT(const Texture *envCubeTexture, RenderTarget *targetCubeRT) const;

                            /// Generates irradiance environment cubemap.
    void                    GenerateIrradianceEnvCubeRT(const Texture *envCubeTexture, RenderTarget *targetCubeRT) const;

                            /// Generates Phong specular prefiltered environment cubemap.
    void                    GeneratePhongSpecularLDSumRT(const Texture *envCubeTexture, int maxSpecularPower, RenderTarget *targetCubeRT) const;

                            /// Generates GGX specular prefiltered environment cubemap for specific mip level.
    void                    GenerateGGXLDSumRTLevel(const Texture *envCubeTexture, RenderTarget *targetCubeRT, int numMipLevels, int mipLevel) const;

                            /// Generates GGX specular prefiltered environment cubemap.
    void                    GenerateGGXLDSumRT(const Texture *envCubeTexture, RenderTarget *targetCubeRT) const;

                            /// Generates GGX DFG integration 2D LUT.
    void                    GenerateGGXDFGSumImage(int size, Image &integrationImage) const;

                            /// Writes GGX DFG integration 2D LUT.
    void                    WriteGGXDFGSum(const char *filename, int size) const;

    void                    TakeScreenShot(const char *filename, RenderWorld *renderWorld, int layerMask, const Vec3 &origin, const Mat3 &axis, float fov, bool useHDR, int width, int height);
    void                    TakeEnvShot(const char *filename, RenderWorld *renderWorld, int layerMask, int staticMask, const Vec3 &origin, bool useHDR, int size = 256);
    void                    TakeIrradianceEnvShot(const char *filename, RenderWorld *renderWorld, int layerMask, int staticMask, const Vec3 &origin, bool useHDR, int size = 16, int envSize = 256);
    void                    TakePrefilteredEnvShot(const char *filename, RenderWorld *renderWorld, int layerMask, int staticMask, const Vec3 &origin, bool useHDR, int size = 256, int envSize = 256);

private:
    void                    RecreateScreenMapRT();
    void                    RecreateHDRMapRT();
    void                    RecreateShadowMapRT();
    void *                  GetCommandBuffer(int bytes);
    void                    IssueCommands();

    void                    UpdateEnvProbes();

    void                    GenerateGGXLDSumRTFirstLevel(const Texture *envCubeTexture, RenderTarget *targetCubeRT) const;
    void                    GenerateGGXLDSumRTRestLevel(const Texture *envCubeTexture, RenderTarget *targetCubeRT, int numMipLevels, int mipLevel) const;

    void                    CmdDrawCamera(const VisCamera *camera);
    void                    CmdScreenshot(int x, int y, int width, int height, const char *filename);

    bool                    initialized;
    unsigned short          savedGammaRamp[768];

    int                     renderWorldCount = 0;

    RenderWorld *           primaryWorld;
    
    Array<RenderContext *>  renderContexts;
    RenderContext *         currentContext;
    RenderContext *         mainContext;

    Array<EnvProbeJob>      envProbeJobs;

    static void             Cmd_GenerateDFGSumGGX(const CmdArgs &args);
    static void             Cmd_ScreenShot(const CmdArgs &args);
};

BE_INLINE RenderSystem::RenderSystem() {
    initialized = false;
}

extern RenderSystem         renderSystem;

BE_NAMESPACE_END
