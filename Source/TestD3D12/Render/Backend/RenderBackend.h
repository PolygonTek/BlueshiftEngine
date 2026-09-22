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

#include "RHI/RHI.h"
#include "Render/GpuTiming.h"
#include "Render/GpuOceanSurface.h"
#include "Render/HiZResources.h"

class GpuFFT;
class RenderFrame;
class RenderContext;
class ShaderPipeline;
struct RenderFrameSettings;
struct VisCamera;
struct RenderPass;
struct VisSurf;
struct RenderPassDrawSurf;

class RenderBackend {
public:
    void                            Init();
    void                            Shutdown();

    void                            Execute(RenderFrame *frame);

    float                           GetStatRenderAverageMsec() const { return statRenderAverageUsec.load() * 0.001f; }
    void                            UpdateRenderFrameTime(uint64_t frameUsec);

    void                            DiscardGpuFrameTimings(const RenderContext *renderContext);

                                    // Current execution state for backend render modules; valid during dispatch.
    RHI::CommandList *              GetCommandList() const { return mainCommandList; }
    RHI::FrameThreadData *          GetFrameThreadData() const;
    const RHI::ConstantBuffer *     GetFrameConstantBuffer() const { return frameConstantBuffer; }
    const RHI::ConstantBuffer *     GetCameraConstantBuffer() const { return cameraConstantBuffer; }
    const VisCamera *               GetVisCamera() const { return currentVisCamera; }
    RHI::Texture *                  GetPostProcessInputTexture() const { return currentPostProcessInputTexture; }
    const RenderFrameSettings &     GetFrameSettings() const;
    uint32_t                        GetFrameIndex() const;
    GpuTiming &                     GetGpuTiming() { return gpuTiming; }

private:
    static constexpr uint32_t       MaxInstancesPerDraw = RenderHiZCullMaxInstanceCount;

    struct SurfaceInstanceBatch {
        uint32_t                    drawSurfIndex = 0;
        uint32_t                    instanceCount = 0;
        RHI::Buffer *               instanceToShaderSurfaceTable = nullptr;
        uint32_t                    instanceBaseOffset = 0;
        RHI::Buffer *               indirectArgsBuffer = nullptr;
        uint32_t                    indirectArgsOffset = 0;
        bool                        valid = false;
    };

    struct HiZCullPassRange {
        uint32_t                    firstBatchIndex = 0;
        uint32_t                    numBatches = 0;
    };

    struct DrawSurfsTaskDesc {
        int32_t                     threadIndex = -1;
        const RenderPassDrawSurf ** drawSurfs = nullptr;
        uint32_t                    numDrawSurfs = 0;
        const RHI::ConstantBuffer * viewConstantBuffer = nullptr;
        uint32_t                    firstDrawSurfIndex = 0;
        const SurfaceInstanceBatch *hiZCullBatches = nullptr;
        uint32_t                    numHiZCullBatches = 0;
        RHI::CommandList *          commandList = nullptr;
        uint32_t                    drawCount = 0;
        uint64_t                    drawTriangleCount = 0;
        uint32_t                    instanceTableAllocCount = 0;
        uint32_t                    instanceTableUploadBytes = 0;
    };

    void                            InitDebugDraw();

    const void *                    ExecuteRenderWork(const void *data);
    const void *                    ExecuteBeginContext(const void *data);
    const void *                    ExecuteDrawCamera(const void *data);
    const void *                    ExecuteScreenshot(const void *data);
    const void *                    ExecuteFinalizeFrame(const void *data);
    const void *                    ExecuteSwapBuffers(const void *data);

    void                            UpdateFramePSOs();

    void                            ExecuteDrawCamera2D(RHI::ClearFlag clearFlags, const BE1::Color4 &clearColor);
    void                            ExecuteDrawCamera3D(RHI::ClearFlag clearFlags, const BE1::Color4 &clearColor, bool useExplicitClearColor);
    void                            ExecuteDrawCamera3DShadowPass();
    void                            ExecuteDrawCamera3DDepthPrepass(RHI::RenderPassImage::LoadAction depthLoadAction);
    void                            ExecuteDrawCamera3DScenePass(RHI::ClearFlag clearFlags, const BE1::Color4 &clearColor, bool useExplicitClearColor, RHI::RenderPassImage::LoadAction depthLoadAction);
    void                            CaptureSceneSnapshot();
    void                            ExecuteDrawCamera3DPostOpaquePass();

    void                            DrawCamera3DDepthPrepass();
    void                            DrawCamera3DScenePass();
    void                            DrawCamera3DPostOpaquePass();
    void                            DrawCamera2D();

    void                            PostProcessAndBlitToOutput(bool velocityReady, const RHI::Texture *outputOverrideTexture);
    void                            BlitTextureToRenderTarget(const RHI::Texture *texture, const RHI::Texture *renderTargetTexture, RHI::ViewID renderTargetViewID, RHI::GPUResourceState renderTargetBeforeState, RHI::GPUResourceState renderTargetAfterState, const BE1::Rect &destinationRect, RHI::PipelineState *imagePSO);
    void                            BlitTextureToSwapChain(const RHI::Texture *texture, const BE1::Rect &destinationRect);

    void                            DrawGenericPass(const RenderPass *renderPass, const RHI::ConstantBuffer *viewConstantBuffer);
    void                            DrawSkyPass(const RenderPass *renderPass);
    void                            DrawGpuOcean();
    void                            DrawGuiPass(const RenderPass *renderPass);
    void                            DrawDebugLights();
    void                            DrawDebugPrimitives();

    void                            SetPassVertexBuffers(RHI::CommandList *commandList, const RenderPassDrawSurf *drawSurf) const;

    void                            DrawGenericSurfaces(const RenderPassDrawSurf **drawSurfs, uint32_t numDrawSurfs, const RHI::ConstantBuffer *viewConstantBuffer, const SurfaceInstanceBatch *hiZCullBatches = nullptr, uint32_t hiZCullBatchCount = 0);
    void                            DrawSkySurfaces(const RenderPassDrawSurf **drawSurfs, uint32_t numDrawSurfs);

    void                            DrawGenericSurfacesWithoutTask(const RenderPassDrawSurf **drawSurfs, uint32_t numDrawSurfs, const RHI::ConstantBuffer *viewConstantBuffer, const SurfaceInstanceBatch *hiZCullBatches = nullptr, uint32_t hiZCullBatchCount = 0);
    void                            DrawGenericSurfacesWithTask(const RenderPassDrawSurf **drawSurfs, uint32_t numDrawSurfs, const RHI::ConstantBuffer *viewConstantBuffer, const SurfaceInstanceBatch *hiZCullBatches = nullptr, uint32_t hiZCullBatchCount = 0);
    void                            DrawGenericSurfacesByTask(RenderBackend::DrawSurfsTaskDesc *taskDesc);
    static void                     DrawGenericSurfacesByTaskFunction(void *data);

    bool                            PrepareHiZCulling();
    bool                            DispatchHiZCullInstances(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, uint32_t instanceCount, RHI::Buffer *inputInstanceToShaderSurfaceTable, uint32_t instanceBaseOffset, uint32_t indirectArgsOffset);
    void                            FinishHiZCulling();
    bool                            DispatchGenHiZ();
    bool                            DispatchHiZDebugPreview();
    bool                            DispatchGenScreenSpaceDepthPyramid();
    bool                            DispatchSSRTrace();
    bool                            DispatchScreenSpaceAO();
    bool                            PrepareAOInputs();
    bool                            DispatchMSAAResolveDepth(RHI::Texture *outputDepthTexture, RHI::GPUResourceState inputState, RHI::Texture *outputNormalTexture = nullptr);
    bool                            DispatchMSAAResolveVelocity();
    bool                            DispatchMSAAResolveDepthHistory();

    bool                            DispatchLuminance();
    bool                            DispatchTAAResolve();
    bool                            DispatchBloom();
    bool                            DispatchFinalPostProcess(bool useToneMapping, bool useBloom);

    bool                            DispatchLightCulling();

    uint32_t                        FindSurfaceInstanceBatchEnd(const RenderPassDrawSurf **drawSurfs, uint32_t numDrawSurfs, uint32_t startIndex) const;
    RHI::Buffer *                   AllocInstanceToShaderSurfaceTable(RHI::FrameThreadData *frameThreadData, const RenderPassDrawSurf **drawSurfs, uint32_t instanceCount) const;
    bool                            PrepareSurfaceInstanceBatch(RHI::FrameThreadData *frameThreadData, const RenderPassDrawSurf **drawSurfs, uint32_t drawSurfIndex, uint32_t instanceCount, SurfaceInstanceBatch &batch, uint32_t &instanceTableAllocCount, uint32_t &instanceTableUploadBytes) const;
    void                            DrawSurfaceInstanceBatch(RHI::CommandList *commandList, const RenderPassDrawSurf **drawSurfs, const SurfaceInstanceBatch &batch, uint32_t &drawCount, uint64_t &drawTriangleCount);

    void                            RecordDrawStats(uint32_t drawCount, uint64_t drawTriangleCount, uint32_t instanceTableAllocCount, uint32_t instanceTableUploadBytes);
    void                            UpdateFrameStats();
    bool                            UploadFrameConstants();
    bool                            UploadCameraConstants();

    RenderFrame *                   frame = nullptr; // 현재 백엔드가 처리할 프레임 데이터
    RenderContext *                 currentContext = nullptr;
    RHI::CommandList *              mainCommandList = nullptr;
    uint32_t                        frameCount = 0;
    bool                            gpuTimingEnabled = false;
    bool                            textureStreamingFeedbackEnabled = false;
    bool                            swapChainOutputEnabled = false;

    RHI::PipelineState *            frameImagePSO = nullptr;
    RHI::PipelineState *            frameHDROutputPSO = nullptr;
    RHI::PipelineState *            frameLightCullingPSO = nullptr;
    RHI::PipelineState *            frameSSRTracePSO = nullptr;
    RHI::PipelineState *            frameMSAAResolveDepthPSO = nullptr;
    RHI::PipelineState *            frameMSAAResolveDepthNormalPSO = nullptr;
    RHI::PipelineState *            frameMSAAResolveVelocityPSO = nullptr;
    RHI::PipelineState *            frameFinalPostProcessPSO = nullptr;
    RHI::PipelineState *            frameTAAResolvePSO = nullptr;

    RHI::ConstantBuffer *           frameConstantBuffer = nullptr;
    RHI::ConstantBuffer *           cameraConstantBuffer = nullptr;

    const VisCamera *               currentVisCamera = nullptr;
    RHI::Texture *                  currentPostProcessInputTexture = nullptr;

    uint32_t                        drawGroupId = -1;
    BE1::Array<DrawSurfsTaskDesc>   drawSurfsTaskDescs;
    BE1::Array<SurfaceInstanceBatch> hiZCullDrawBatches;
    BE1::Array<HiZCullPassRange>    hiZCullPassRanges;

    GpuOceanSurface::QueryReadbackFrame *oceanSurfaceQueryReadbackFrame = nullptr;

    uint64_t                        statRenderElapsedUsec = 0;
    int                             statRenderFrameCount = 0;
    std::atomic<float>              statRenderAverageUsec = 0;
    uint32_t                        statDrawCount = 0;
    uint64_t                        statDrawTriangleCount = 0;
    uint32_t                        statInstanceTableAllocCount = 0;
    uint32_t                        statInstanceTableUploadBytes = 0;

    GpuTiming                       gpuTiming;
};
