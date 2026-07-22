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

#include "Precompiled.h"
#include "RHI/RHI.h"
#include "GpuFFT.h"
#include "GpuSphericalHarmonics.h"
#include "ColorGradingLUTGenerator.h"
#include "ConvolutionBloomKernelGenerator.h"
#include "IBLGenerator.h"

class RenderContext;
class RenderBackend;
class RenderFrameData;
class ShaderPipeline;
class ShaderTemplate;
class Texture;

/*
---------------------------------------------------------------------------------------------------

    Render System

    렌더러 전역 상태와 렌더 스레드를 관리한다.

    - RenderContext 생성/파괴
    - 렌더 스레드와 프레임 동기화
    - 제출된 프레임 데이터를 제출 순서대로 백엔드에서 실행
    - 백엔드 실행과 지연 해제 리소스 정리

---------------------------------------------------------------------------------------------------
*/

class RenderSystem {
    friend class RenderFrameData;
    friend class RenderBackend;
    friend class ConvolutionBloomKernelGenerator;

public:
    void                            Init(void *mainWindowHandle);
    void                            Shutdown();

    void                            UpdateFrame();

    const RHI::MemoryUsage &        GetVideoMemoryUsage() const { return videoMemoryUsage; }
    bool                            IsUsingRenderThread() const { return !!renderThread; }

    static bool                     IsInRenderThread() { return isRenderThread; }

    [[nodiscard]] RenderContext *   CreateRenderContext(void *windowHandle, bool isMain = false);
    void                            DestroyRenderContext(RenderContext *renderContext);
    RenderContext *                 FindRenderContextByWindowHandle(void *windowHandle) const;

                                    // 프레임 데이터를 렌더 스레드에 제출한다. 제출된 프레임 데이터는 렌더 스레드에서 순서대로 처리된다.
    void                            SubmitFrame(RenderContext *renderContext, RenderFrameData *frameData);

                                    // 주어진 프레임 데이터의 렌더링이 완료될 때까지 기다린다.
    void                            WaitFrameRenderCompleted(const RenderFrameData *frameData);

                                    // 주어진 렌더 컨텍스트의 모든 프레임 렌더링이 완료될 때까지 기다린다.
    void                            WaitContextRenderCompleted(const RenderContext *renderContext);

                                    // 모든 렌더 컨텍스트의 모든 프레임 렌더링이 완료될 때까지 기다린다.
    void                            WaitAllRenderCompleted();

    void                            RequestScreenshot(RenderContext *renderContext, const char *filename);

    RenderBackend *                 GetBackend() const { return backEnd; }

    const SharedPtr<Texture> &      GetColorGradingLUTTexture() const { return colorGradingLUTTexture; }
    void                            RequestColorGradingLUTUpdate() { colorGradingLUTDirty = true; }

    ConvolutionBloomKernelGenerator &GetConvolutionBloomKernelGenerator() { return convolutionBloomKernelGenerator; }
    const ConvolutionBloomKernelGenerator &GetConvolutionBloomKernelGenerator() const { return convolutionBloomKernelGenerator; }

    GpuSphericalHarmonics &         GetGpuSphericalHarmonics() { return gpuSphericalHarmonics; }
    const GpuSphericalHarmonics &   GetGpuSphericalHarmonics() const { return gpuSphericalHarmonics; }

    IBLGenerator &                  GetIBLGenerator() { return iblGenerator; }
    const IBLGenerator &            GetIBLGenerator() const { return iblGenerator; }

private:
    struct SubmittedFrame {
        RenderContext *             renderContext = nullptr;
        RenderFrameData *           frameData = nullptr;
    };

    void                            InitRenderThread();
    void                            ShutdownRenderThread();
    void                            InitComputePipelines();
    void                            EnsureComputePipelines();
    void                            ReleaseComputePipelines();
    bool                            CreateColorGradingLUT();
    void                            UpdateColorGradingLUT();
    void                            DestroyColorGradingLUT();

    void                            MarkRenderCompleted(RenderFrameData *frameData);
    bool                            WaitSubmittedFrame(SubmittedFrame &submittedFrame);

    static unsigned int             RenderThreadProc(void *param);

    static void                     Cmd_ScreenShot(const BE1::CmdArgs &args);

    BE1::Array<RenderContext *>     renderContexts;
    RenderContext *                 mainRenderContext = nullptr;
    RenderBackend *                 backEnd = nullptr;
    RHI::MemoryUsage                videoMemoryUsage = {};

    BE1::Array<SubmittedFrame>      submittedFrames;
    int                             submittedFrameReadIndex = 0;

    BE1::PlatformSRWLock *          frameSyncLock = nullptr;
    BE1::PlatformCondition *        renderCompletedCondition = nullptr;
    BE1::PlatformCondition *        updateCompletedCondition = nullptr;
    BE1::PlatformThread *           renderThread = nullptr;
    bool                            isStoppingRenderThread = false;
    static thread_local bool        isRenderThread;

    SharedPtr<ShaderTemplate>       genHiZMipFirstBatchShaderTemplate;
    SharedPtr<ShaderTemplate>       genHiZMipFirstBatchMSAAShaderTemplate;
    SharedPtr<ShaderTemplate>       genHiZMipBatchShaderTemplate;
    SharedPtr<ShaderTemplate>       genHiZMipOddDimShaderTemplate;
    SharedPtr<ShaderTemplate>       hiZCullInstancesShaderTemplate;
    SharedPtr<ShaderTemplate>       lightCullingShaderTemplate;
    SharedPtr<ShaderTemplate>       hiZDebugPreviewShaderTemplate;
    SharedPtr<ShaderTemplate>       depthHistoryResolveShaderTemplate;
    SharedPtr<ShaderTemplate>       surfaceOutputMSAAResolveNormalShaderTemplate;
    SharedPtr<ShaderTemplate>       surfaceOutputMSAAResolveVelocityShaderTemplate;
    SharedPtr<ShaderTemplate>       surfaceOutputMSAAResolveNormalVelocityShaderTemplate;
    SharedPtr<ShaderTemplate>       finalPostProcessShaderTemplate;
    SharedPtr<ShaderTemplate>       taaResolveShaderTemplate;
    SharedPtr<ShaderTemplate>       luminancePass1ShaderTemplate;
    SharedPtr<ShaderTemplate>       luminancePass2ShaderTemplate;

    SharedPtr<ShaderTemplate>       bloomPrefilterShaderTemplate;
    SharedPtr<ShaderTemplate>       bloomDownsampleShaderTemplate;
    SharedPtr<ShaderTemplate>       bloomUpsampleShaderTemplate;

    SharedPtr<ShaderTemplate>       convolutionBloomPrefilterShaderTemplate;
    SharedPtr<ShaderTemplate>       convolutionBloomKernelPrepareShaderTemplate;
    SharedPtr<ShaderTemplate>       convolutionBloomResolveShaderTemplate;
    SharedPtr<ShaderTemplate>       convolutionBloomAperturePrepareShaderTemplate;
    SharedPtr<ShaderTemplate>       convolutionBloomReduceApertureEnergyShaderTemplate;
    SharedPtr<ShaderTemplate>       convolutionBloomResolveDiffractionPSFShaderTemplate;
    SharedPtr<ShaderTemplate>       convolutionBloomApplyDispersionShaderTemplate;

    SharedPtr<ShaderPipeline>       genHiZMipFirstBatchPipeline;
    SharedPtr<ShaderPipeline>       genHiZMipFirstBatchMSAAPipeline;
    SharedPtr<ShaderPipeline>       genHiZMipBatchPipeline;
    SharedPtr<ShaderPipeline>       genHiZMipOddDimPipeline;
    SharedPtr<ShaderPipeline>       hiZCullInstancesPipeline;
    SharedPtr<ShaderPipeline>       lightCullingPipeline;
    SharedPtr<ShaderPipeline>       hiZDebugPreviewPipeline;
    SharedPtr<ShaderPipeline>       depthHistoryResolvePipeline;
    SharedPtr<ShaderPipeline>       surfaceOutputMSAAResolveNormalPipeline;
    SharedPtr<ShaderPipeline>       surfaceOutputMSAAResolveVelocityPipeline;
    SharedPtr<ShaderPipeline>       surfaceOutputMSAAResolveNormalVelocityPipeline;
    SharedPtr<ShaderPipeline>       finalPostProcessPipeline;
    SharedPtr<ShaderPipeline>       taaResolvePipeline;
    SharedPtr<ShaderPipeline>       luminancePass1Pipeline;
    SharedPtr<ShaderPipeline>       luminancePass2Pipeline;

    SharedPtr<ShaderPipeline>       bloomPrefilterPipeline;
    SharedPtr<ShaderPipeline>       bloomDownsamplePipeline;
    SharedPtr<ShaderPipeline>       bloomUpsamplePipeline;

    SharedPtr<ShaderPipeline>       convolutionBloomPrefilterPipeline;
    SharedPtr<ShaderPipeline>       convolutionBloomKernelPreparePipeline;
    SharedPtr<ShaderPipeline>       convolutionBloomResolvePipeline;
    SharedPtr<ShaderPipeline>       convolutionBloomAperturePreparePipeline;
    SharedPtr<ShaderPipeline>       convolutionBloomReduceApertureEnergyPipeline;
    SharedPtr<ShaderPipeline>       convolutionBloomResolveDiffractionPSFPipeline;
    SharedPtr<ShaderPipeline>       convolutionBloomApplyDispersionPipeline;

    ConvolutionBloomKernelGenerator convolutionBloomKernelGenerator;

    BE1::Image                      colorGradingLUTImage;
    SharedPtr<Texture>              colorGradingLUTTexture;
    bool                            colorGradingLUTDirty = false;

    GpuFFT                          gpuFFT;
    GpuSphericalHarmonics           gpuSphericalHarmonics;
    IBLGenerator                    iblGenerator;
};

extern RenderSystem *               renderSystem;
