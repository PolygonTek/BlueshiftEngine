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

#include "Containers/RingBuffer.h"
#include "RHI/RHI.h"
#include "RenderFrameSettings.h"
#include "GpuFFT.h"
#include "Ocean/GpuOcean.h"
#include "Ocean/GpuOceanSurface.h"
#include "GpuSphericalHarmonics.h"
#include "ScreenSpace/HiZ.h"
#include "ScreenSpace/ScreenSpaceAO.h"
#include "ScreenSpace/SSRHiZ.h"
#include "ScreenSpace/SSRSceneColorHistory.h"
#include "PostProcess/Bloom.h"
#include "PostProcess/PostLuminance.h"
#include "PostProcess/ConvolutionBloomKernelGenerator.h"
#include "IBLBaker.h"
#include "ProbeBakeSystem.h"
#include "ShaderPipeline/ShaderPipelineBootstrap.h"
#include "RenderWork/RenderWorkQueue.h"

class RenderContext;
class RenderBackend;
class RenderFrame;
class ShaderPipeline;
class ShaderTemplate;
class Texture;

/*
---------------------------------------------------------------------------------------------------

    Render System

    렌더러 전역 상태와 렌더 스레드를 관리한다.

    - RenderContext 생성/파괴
    - 렌더 스레드와 프레임 동기화
    - 제출된 프레임 스냅샷 (RenderFrame) 을 제출 순서대로 백엔드에서 실행
    - 백엔드 실행과 지연 해제 리소스 정리

---------------------------------------------------------------------------------------------------
*/

class RenderSystem {
    friend class RenderFrame;
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

                                    // 준비된 프레임을 렌더 스레드에 제출한다. 제출된 프레임은 렌더 스레드에서 순차적으로 처리된다.
    void                            SubmitFrame(RenderFrame *frame);

                                    // 렌더 스레드에서 주어진 프레임의 렌더링이 완료될 때까지 기다린다.
    void                            WaitFrameRenderCompleted(const RenderFrame *frame);

                                    // 렌더 스레드에서 주어진 렌더 컨텍스트의 모든 프레임의 렌더링이 완료될 때까지 기다린다.
    void                            WaitContextRenderCompleted(const RenderContext *renderContext);

                                    // 렌더 스레드에서 모든 렌더 컨텍스트들의 모든 프레임의 렌더링이 완료될 때까지 기다린다.
    void                            WaitAllRenderCompleted();

    RenderBackend *                 GetBackend() const { return backEnd; }

    const SharedPtr<Texture> &      GetColorGradingLUTTexture() const { return colorGradingLUTTexture; }
    void                            RequestColorGradingLUTUpdate() { colorGradingLUTDirty = true; }

                                    // NVIDIA STBN: https://github.com/NVIDIA-RTX/STBN (Assets/STBN.zip, License.txt).
                                    // 128x128, 64 slices, mip 1. 픽셀 좌표 & 127, frameIndex & 63 으로 Load/Point sampling 한다.
                                    // R8: void-and-cluster 스칼라 난수. 디더링, 확률적 알파 테스트, 스칼라 지터.
    const SharedPtr<Texture> &      GetSTBNScalarTexture() const { return stbnScalarTexture; }

                                    // R8: simulated annealing 으로 생성한 1차원 난수. Scalar 와 다른 시퀀스.
    const SharedPtr<Texture> &      GetSTBNVec1Texture() const { return stbnVec1Texture; }

                                    // RG8: [0, 1]^2 샘플. SSR 의 GGX VNDF 반사 방향 샘플링 입력.
    const SharedPtr<Texture> &      GetSTBNVec2Texture() const { return stbnVec2Texture; }

                                    // RGBA8: RGB 는 [0, 1]^3 샘플이며 A 는 미사용.
    const SharedPtr<Texture> &      GetSTBNVec3Texture() const { return stbnVec3Texture; }

                                    // R8: r * 2 - 1 로 -1/+1 부호 선택.
    const SharedPtr<Texture> &      GetSTBNUnitVec1Texture() const { return stbnUnitVec1Texture; }

                                    // RG8: 원 둘레 방향. normalize(rg * 2 - 1) 로 복원.
    const SharedPtr<Texture> &      GetSTBNUnitVec2Texture() const { return stbnUnitVec2Texture; }

                                    // RGBA8: 구면 방향. normalize(rgb * 2 - 1) 로 복원하며 A 는 미사용. normalize 는 8비트 양자화의 길이 오차를 보정한다.
    const SharedPtr<Texture> &      GetSTBNUnitVec3Texture() const { return stbnUnitVec3Texture; }

                                    // RGBA8: +Z 반구의 cosine-weighted 방향. Diffuse/AO/GI 용이며 normalize(rgb * 2 - 1) 을 TBN 으로 변환한다.
                                    // A 는 (pdf - pdfMin) / (pdfMax - pdfMin) 의 인코딩 값이고 범위 정보는 DDS 에 없다.
                                    // 입체각 PDF 는 복원한 로컬 방향의 max(z, 0) / PI 로 계산한다.
    const SharedPtr<Texture> &      GetSTBNUnitVec3CosineTexture() const { return stbnUnitVec3CosineTexture; }

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

    SSRHiZ &                        GetSSRHiZ() { return ssrHiZ; }
    const SSRHiZ &                  GetSSRHiZ() const { return ssrHiZ; }

    SSRSceneColorHistory &          GetSSRSceneColorHistory() { return ssrSceneColorHistory; }
    const SSRSceneColorHistory &    GetSSRSceneColorHistory() const { return ssrSceneColorHistory; }

    IBLBaker &                      GetIBLBaker() { return iblBaker; }
    const IBLBaker &                GetIBLBaker() const { return iblBaker; }

    ProbeBakeSystem &               GetProbeBakeSystem() { return probeBakeSystem; }
    const ProbeBakeSystem &         GetProbeBakeSystem() const { return probeBakeSystem; }

private:
    void                            InitRenderThread();
    void                            ShutdownRenderThread();
    void                            InitSTBNTextures();
    void                            UploadEngineTextures();
    void                            InitComputePipelines();
    void                            EnsureComputePipelines();
    void                            ReleaseComputePipelines();
    void                            CreateColorGradingLUT();
    void                            UpdateColorGradingLUT();
    void                            DestroyColorGradingLUT();

    void                            PrepareProbeBakes(RenderContext *renderContext);

                                    // render work 를 대기 큐에 추가한다.
    void                            EnqueueRenderWork(UniquePtr<RenderWork> work);

                                    // 완료된 frame 의 render work 를 완료 처리한다.
    void                            CompleteRenderWorks(RenderFrame *frame);

                                    // 대기 중인 render work 를 메인 스왑 체인 프레임의 렌더 커맨드에 밀어 넣는다.
    void                            AttachPendingRenderWorks(RenderFrame *frame);

    RenderFrame *                   WaitSubmittedFrame();
    void                            MarkRenderCompleted(RenderFrame *frame);

    static unsigned int             RenderThreadProc(void *param);

    static void                     Cmd_ScreenShot(const BE1::CmdArgs &args);

    BE1::Array<RenderContext *>     renderContexts;
    RenderContext *                 mainRenderContext = nullptr;
    RenderBackend *                 backEnd = nullptr;
    RHI::MemoryUsage                videoMemoryUsage = {};

    BE1::RingBuffer<RenderFrame *>  submittedFrames;    // 각 컨텍스트에서 제출한 프레임을 제출 순서대로 보관하는 원형 큐
    BE1::RWLock                     frameSyncLock;
    BE1::Condition                  renderCompletedCondition;
    BE1::Condition                  updateCompletedCondition;
    BE1::PlatformThread *           renderThread = nullptr;
    bool                            isStoppingRenderThread = false;
    static thread_local bool        isRenderThread;

    RenderWorkQueue                 renderWorkQueue;

    SharedPtr<ShaderTemplate>       msaaResolveDepthShaderTemplate;
    SharedPtr<ShaderTemplate>       msaaResolveDepthNormalShaderTemplate;
    SharedPtr<ShaderTemplate>       msaaResolveVelocityShaderTemplate;
    SharedPtr<ShaderTemplate>       lightCullingShaderTemplate;
    SharedPtr<ShaderTemplate>       ssrTraceShaderTemplates[RenderFrameSettings::SSRResolutionModeCount];
    SharedPtr<ShaderTemplate>       ssrRadianceResolveShaderTemplate;
    SharedPtr<ShaderTemplate>       ssrTemporalShaderTemplates[RenderFrameSettings::SSRResolutionModeCount];
    SharedPtr<ShaderTemplate>       ssrUpsampleShaderTemplates[RenderFrameSettings::SSRResolutionModeCount];
    SharedPtr<ShaderTemplate>       taaResolveShaderTemplate;
    SharedPtr<ShaderTemplate>       finalPostProcessShaderTemplate;

    SharedPtr<ShaderPipeline>       msaaResolveDepthPipeline;
    SharedPtr<ShaderPipeline>       msaaResolveDepthNormalPipeline;
    SharedPtr<ShaderPipeline>       msaaResolveVelocityPipeline;
    SharedPtr<ShaderPipeline>       lightCullingPipeline;
    SharedPtr<ShaderPipeline>       ssrTracePipelines[RenderFrameSettings::SSRResolutionModeCount];
    SharedPtr<ShaderPipeline>       ssrRadianceResolvePipeline;
    SharedPtr<ShaderPipeline>       ssrTemporalPipelines[RenderFrameSettings::SSRResolutionModeCount];
    SharedPtr<ShaderPipeline>       ssrUpsamplePipelines[RenderFrameSettings::SSRResolutionModeCount];
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
    SharedPtr<Texture>              stbnScalarTexture;
    SharedPtr<Texture>              stbnVec1Texture;
    SharedPtr<Texture>              stbnVec2Texture;
    SharedPtr<Texture>              stbnVec3Texture;
    SharedPtr<Texture>              stbnUnitVec1Texture;
    SharedPtr<Texture>              stbnUnitVec2Texture;
    SharedPtr<Texture>              stbnUnitVec3Texture;
    SharedPtr<Texture>              stbnUnitVec3CosineTexture;

    GpuFFT                          gpuFFT;
    GpuOcean                        gpuOcean;
    GpuOceanSurface                 gpuOceanSurface;
    GpuSphericalHarmonics           gpuSphericalHarmonics;
    HiZ                             hiZ;
    ScreenSpaceAO                   screenSpaceAO;
    SSRHiZ                          ssrHiZ;
    SSRSceneColorHistory            ssrSceneColorHistory;
    Bloom                           bloom;
    PostLuminance                   postLuminance;
    IBLBaker                        iblBaker;
    ProbeBakeSystem                 probeBakeSystem;
    ShaderPipelineBootstrap         shaderPipelineBootstrap;
};

extern RenderSystem *               renderSystem;
