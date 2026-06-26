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

#include "Precompiled.h"
#include "RHI/RHI.h"
#include "RenderFrameData.h"
#include "GpuFFT.h"

class RenderFrameData;
class RenderContext;
class ShaderPipeline;
struct VisCamera;
struct RenderPass;
struct VisSurf;
struct PassDrawSurf;

class RenderBackend {
public:
    void                            Init();
    void                            Shutdown();

    void                            Execute(RenderFrameData *frameData);

    float                           GetStatRenderAverageMsec() const { return statRenderAverageUsec.load() * 0.001f; }
    void                            UpdateRenderFrameTime(uint64_t frameUsec);

    void                            DiscardGpuFrameTimings(const RenderContext *renderContext);

private:
    static constexpr int            GpuTimingSlotCount = 8;
    static constexpr int            GpuTimingSampleInterval = 4; // must be a power of two.
    static constexpr uint32_t       MaxInstancesPerDraw = 2048;

    struct GpuTimingSlot {
        RenderContext *             renderContext = nullptr;
        uint64_t                    fenceValue = 0;
        bool                        pending = false;
    };

    struct DrawSurfsTaskDesc {
        int32_t                     threadIndex = -1;
        const PassDrawSurf **       drawSurfs = nullptr;
        uint32_t                    numDrawSurfs = 0;
        RHI::CommandList *          commandList = nullptr;
        uint32_t                    drawCount = 0;
        uint64_t                    drawTriangleCount = 0;
        uint32_t                    instanceTableAllocCount = 0;
        uint32_t                    instanceTableUploadBytes = 0;
    };

    const void *                    ExecuteBeginContext(const void *data);
    const void *                    ExecuteDrawCamera(const void *data);
    const void *                    ExecuteScreenshot(const void *data);
    const void *                    ExecuteSwapBuffers(const void *data);

    void                            DrawCamera3D();
    void                            DrawCamera2D();

    void                            PostProcessAndBlitToSwapChain();
    void                            BlitRectToSwapChain(const RHI::Texture *texture, const BE1::Rect &rect);

    void                            DrawMainPass(const RenderPass *renderPass);
    void                            DrawSkyPass(const RenderPass *renderPass);
    void                            DrawGuiPass(const RenderPass *renderPass);

    void                            UploadStaticDataToGPU(const RenderPass *renderPass) const;
    void                            SetPassVertexBuffers(RHI::CommandList *commandList, const PassDrawSurf *drawSurf) const;

    void                            DrawGenericSurfaces(const PassDrawSurf **drawSurfs, uint32_t numDrawSurfs);
    void                            DrawSkySurfaces(const PassDrawSurf **drawSurfs, uint32_t numDrawSurfs);

    void                            DrawGenericSurfacesWithoutTask(const PassDrawSurf **drawSurfs, uint32_t numDrawSurfs);
    void                            DrawGenericSurfacesWithTask(const PassDrawSurf **drawSurfs, uint32_t numDrawSurfs);
    void                            DrawGenericSurfacesByTask(RenderBackend::DrawSurfsTaskDesc *taskDesc);
    static void                     DrawGenericSurfacesByTaskFunction(void *data);

    bool                            DispatchLuminance();
    bool                            DispatchBloom();
    bool                            DispatchPyramidBloom();
    bool                            DispatchConvolutionBloom();
    bool                            DispatchConvolutionBloomKernelPrepare();
    bool                            DispatchConvolutionBloomScreenPrefilter();
    bool                            DispatchConvolutionBloomMultiplySpectrum();
    bool                            DispatchConvolutionBloomResolve();
    bool                            DispatchConvolutionBloomFFT(RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer);
    bool                            DispatchConvolutionBloomIFFT(RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer);
    bool                            DispatchToneMapping(bool useBloom);

    uint32_t                        FindSurfaceInstanceBatchEnd(const PassDrawSurf **drawSurfs, uint32_t numDrawSurfs, uint32_t startIndex) const;
    RHI::Buffer *                   AllocInstanceToShaderSurfaceTable(RHI::FrameThreadData *frameThreadData, const PassDrawSurf **drawSurfs, uint32_t instanceCount) const;
    void                            DrawSurfaceInstances(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const PassDrawSurf **drawSurfs, uint32_t instanceCount, uint32_t &drawCount, uint64_t &drawTriangleCount, uint32_t &instanceTableAllocCount, uint32_t &instanceTableUploadBytes);

    void                            RecordDrawStats(uint32_t drawCount, uint64_t drawTriangleCount, uint32_t instanceTableAllocCount, uint32_t instanceTableUploadBytes);
    bool                            UploadFrameConstants();
    bool                            UploadCameraConstants();

    void                            BeginGpuFrameTiming(RHI::CommandList *commandList);
    void                            EndGpuFrameTiming(RHI::CommandList *commandList);
    void                            SetGpuFrameTimingFence(uint64_t fenceValue);
    void                            ReadCompletedGpuTimings();

    RenderFrameData *               frameData = nullptr; // 현재 백엔드가 처리할 프레임 데이터
    RenderContext *                 currentContext = nullptr;
    RHI::CommandList *              mainCommandList = nullptr;
    uint32_t                        frameCount = 0;

    RHI::PipelineState *            currentImagePSO = nullptr;
    RHI::PipelineState *            currentToneMappingPSO = nullptr;
    RHI::PipelineState *            currentLuminancePass1PSO = nullptr;
    RHI::PipelineState *            currentLuminancePass2PSO = nullptr;
    RHI::PipelineState *            currentBloomPrefilterPSO = nullptr;
    RHI::PipelineState *            currentBloomDownsamplePSO = nullptr;
    RHI::PipelineState *            currentBloomUpsamplePSO = nullptr;
    RHI::PipelineState *            currentConvolutionBloomPrefilterPSO = nullptr;
    RHI::PipelineState *            currentConvolutionBloomKernelPreparePSO = nullptr;
    RHI::PipelineState *            currentConvolutionBloomResolvePSO = nullptr;

    GpuFFT                          gpuFFT;

    RHI::ConstantBuffer *           frameConstantBuffer = nullptr;
    RHI::ConstantBuffer *           cameraConstantBuffer = nullptr;

    const VisCamera *               currentVisCamera = nullptr;

    uint32_t                        drawGroupId = -1;
    BE1::Array<DrawSurfsTaskDesc>   drawSurfsTaskDescs;

    uint64_t                        statRenderElapsedUsec = 0;
    int                             statRenderFrameCount = 0;
    std::atomic<float>              statRenderAverageUsec = 0;
    uint32_t                        statDrawCount = 0;
    uint64_t                        statDrawTriangleCount = 0;
    uint32_t                        statInstanceTableAllocCount = 0;
    uint32_t                        statInstanceTableUploadBytes = 0;

    uint64_t                        gpuTimestampFrequency = 0;
    RHI::QueryHeap *                gpuTimestampQueryHeap = nullptr;
    RHI::Buffer *                   gpuTimestampReadbackBuffer = nullptr;
    GpuTimingSlot                   gpuTimingSlots[GpuTimingSlotCount];
    int                             gpuTimingNextSlot = 0;
    int                             gpuTimingCurrentSlot = -1;
};
