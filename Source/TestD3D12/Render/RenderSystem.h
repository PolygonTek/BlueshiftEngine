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

#include "RHI/RHI.h"
#include "GpuFFT.h"
#include "GpuOcean.h"
#include "GpuOceanSurface.h"
#include "GpuSphericalHarmonics.h"
#include "HiZ.h"
#include "ScreenSpaceAO.h"
#include "ScreenSpaceDepthPyramid.h"
#include "Bloom.h"
#include "PostLuminance.h"
#include "ConvolutionBloomKernelGenerator.h"
#include "IBLBaker.h"
#include "ProbeBakeSystem.h"
#include "ShaderPipelineBootstrap.h"
#include "RenderWork/RenderWorkQueue.h"

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
    friend class RenderContext;
    friend class RenderBackend;
    friend class ProbeBakeSystem;

public:
    void                            Init(void *mainWindowHandle);
    void                            Shutdown();

    void                            BeginGameFrame();
    void                            UpdateFrame();

    void                            UpdateShaderPipelineBootstrap() { shaderPipelineBootstrap.Update(); }
    ShaderPipelineBootstrap::State  GetShaderPipelineBootstrapState() const { return shaderPipelineBootstrap.GetState(); }
    float                           GetShaderPipelineBootstrapProgress() const { return shaderPipelineBootstrap.GetProgress(); }

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
    uint64_t                        RequestConvolutionBloomKernelGeneration(const SharedPtr<Texture> &apertureTexture, float apertureScale, float dispersionAmount);
    void                            CompleteConvolutionBloomKernelGeneration(uint64_t requestSerial, bool succeeded);
    bool                            GetConvolutionBloomKernelGenerationResult(uint64_t requestSerial, bool &succeeded) const;

    GpuFFT &                        GetGpuFFT() { return gpuFFT; }
    const GpuFFT &                  GetGpuFFT() const { return gpuFFT; }

    GpuOcean &                      GetGpuOcean() { return gpuOcean; }
    const GpuOcean &                GetGpuOcean() const { return gpuOcean; }

    GpuOceanSurface &               GetGpuOceanSurface() { return gpuOceanSurface; }
    const GpuOceanSurface &         GetGpuOceanSurface() const { return gpuOceanSurface; }

    GpuSphericalHarmonics &         GetGpuSphericalHarmonics() { return gpuSphericalHarmonics; }
    const GpuSphericalHarmonics &   GetGpuSphericalHarmonics() const { return gpuSphericalHarmonics; }

    Bloom &                         GetBloom() { return bloom; }
    const Bloom &                   GetBloom() const { return bloom; }

    PostLuminance &                 GetPostLuminance() { return postLuminance; }
    const PostLuminance &           GetPostLuminance() const { return postLuminance; }

    HiZ &                           GetHiZ() { return hiZ; }
    const HiZ &                     GetHiZ() const { return hiZ; }

    ScreenSpaceAO &                 GetScreenSpaceAO() { return screenSpaceAO; }
    const ScreenSpaceAO &           GetScreenSpaceAO() const { return screenSpaceAO; }

    ScreenSpaceDepthPyramid &       GetScreenSpaceDepthPyramid() { return screenSpaceDepthPyramid; }
    const ScreenSpaceDepthPyramid & GetScreenSpaceDepthPyramid() const { return screenSpaceDepthPyramid; }

    IBLBaker &                      GetIBLBaker() { return iblBaker; }
    const IBLBaker &                GetIBLBaker() const { return iblBaker; }

    ProbeBakeSystem &               GetProbeBakeSystem() { return probeBakeSystem; }
    const ProbeBakeSystem &         GetProbeBakeSystem() const { return probeBakeSystem; }

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

    void                            PrepareProbeBakes(RenderContext *renderContext);

                                    // render work 를 대기 큐에 추가한다.
    void                            EnqueueRenderWork(UniquePtr<RenderWork> work);

                                    // 완료된 frameData 의 render work 를 완료 처리한다.
    void                            CompleteRenderWorks(RenderFrameData *frameData);

                                    // 대기 중인 render work 를 메인 스왑 체인 프레임의 렌더 커맨드에 밀어 넣는다.
    void                            AttachPendingRenderWorks(RenderFrameData *frameData);

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
    RenderWorkQueue                 renderWorkQueue;

    BE1::RWLock                     frameSyncLock;
    BE1::Condition                  renderCompletedCondition;
    BE1::Condition                  updateCompletedCondition;
    BE1::PlatformThread *           renderThread = nullptr;
    bool                            isStoppingRenderThread = false;
    static thread_local bool        isRenderThread;

    SharedPtr<ShaderTemplate>       msaaResolveDepthShaderTemplate;
    SharedPtr<ShaderTemplate>       msaaResolveDepthNormalShaderTemplate;
    SharedPtr<ShaderTemplate>       msaaResolveVelocityShaderTemplate;
    SharedPtr<ShaderTemplate>       lightCullingShaderTemplate;
    SharedPtr<ShaderTemplate>       ssrTraceShaderTemplate;
    SharedPtr<ShaderTemplate>       taaResolveShaderTemplate;
    SharedPtr<ShaderTemplate>       finalPostProcessShaderTemplate;

    SharedPtr<ShaderPipeline>       msaaResolveDepthPipeline;
    SharedPtr<ShaderPipeline>       msaaResolveDepthNormalPipeline;
    SharedPtr<ShaderPipeline>       msaaResolveVelocityPipeline;
    SharedPtr<ShaderPipeline>       lightCullingPipeline;
    SharedPtr<ShaderPipeline>       ssrTracePipeline;
    SharedPtr<ShaderPipeline>       taaResolvePipeline;
    SharedPtr<ShaderPipeline>       finalPostProcessPipeline;

    ConvolutionBloomKernelGenerator convolutionBloomKernelGenerator;
    uint64_t                        nextConvolutionBloomKernelGenerationSerial = 0;
    uint64_t                        completedConvolutionBloomKernelGenerationSerial = 0;
    bool                            completedConvolutionBloomKernelGenerationSucceeded = false;

    BE1::Image                      colorGradingLUTImage;
    SharedPtr<Texture>              colorGradingLUTTexture;
    bool                            colorGradingLUTDirty = false;

    SharedPtr<Texture>              dfgSumGGXTexture;

    GpuFFT                          gpuFFT;
    GpuOcean                        gpuOcean;
    GpuOceanSurface                 gpuOceanSurface;
    GpuSphericalHarmonics           gpuSphericalHarmonics;
    HiZ                             hiZ;
    ScreenSpaceAO                   screenSpaceAO;
    ScreenSpaceDepthPyramid         screenSpaceDepthPyramid;
    Bloom                           bloom;
    PostLuminance                   postLuminance;
    IBLBaker                        iblBaker;
    ProbeBakeSystem                 probeBakeSystem;
    ShaderPipelineBootstrap         shaderPipelineBootstrap;
};

extern RenderSystem *               renderSystem;
