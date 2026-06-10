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
#include "RHI.h"
#include "RenderFrameData.h"

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
    };

    const void *                    ExecuteBeginContext(const void *data);
    const void *                    ExecuteDrawCamera(const void *data);
    const void *                    ExecuteScreenshot(const void *data);
    const void *                    ExecuteSwapBuffers(const void *data);

    void                            DrawCamera3D();
    void                            DrawCamera2D();

    void                            DrawMainPass(const RenderPass *renderPass);
    void                            DrawGuiPass(const RenderPass *renderPass);

    void                            SetPassVertexBuffers(RHI::CommandList *commandList, const PassDrawSurf *drawSurf) const;

    void                            DrawLitSurfaces(const PassDrawSurf **drawSurfs, uint32_t numDrawSurfs);
    void                            DrawLitSurfacesInstanced(const PassDrawSurf **instanceSurfs, uint32_t instanceCount);

    void                            DrawLitSurfacesWithoutTask(const PassDrawSurf **drawSurfs, uint32_t numDrawSurfs);
    void                            DrawLitSurfacesWithTask(const PassDrawSurf **drawSurfs, uint32_t numDrawSurfs, uint32_t numTasks);
    void                            DrawLitSurfacesByTask(RenderBackend::DrawSurfsTaskDesc *taskDesc);
    static void                     DrawLitSurfacesByTaskFunction(void *data);

    void                            DrawLitSurface(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const PassDrawSurf *drawSurf);
    void                            DrawLitSurfaceInstanced(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const PassDrawSurf **instanceSurfs, int instanceCount);
    void                            DrawGuiSurface(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const PassDrawSurf *drawSurf);

    void                            RecordDrawStats(uint32_t drawCount, uint64_t triangleCount);
    bool                            UploadFrameConstants();

    void                            BeginGpuFrameTiming(RHI::CommandList *commandList);
    void                            EndGpuFrameTiming(RHI::CommandList *commandList);
    void                            SetGpuFrameTimingFence(uint64_t fenceValue);
    void                            ReadCompletedGpuTimings();

    RenderFrameData *               frameData = nullptr; // 현재 백엔드가 처리할 프레임 데이터
    RenderContext *                 currentContext = nullptr;
    RHI::CommandList *              mainCommandList = nullptr;
    uint32_t                        frameCount = 0;
    RHI::PipelineState *            currentImagePSO = nullptr;
    RHI::ConstantBuffer *           frameConstantBuffer = nullptr;

    const VisCamera *               currentVisCamera = nullptr;

    uint32_t                        drawGroupId = -1;
    BE1::Array<DrawSurfsTaskDesc>   drawSurfsTaskDescs;

    uint64_t                        statRenderElapsedUsec = 0;
    int                             statRenderFrameCount = 0;
    std::atomic<float>              statRenderAverageUsec = 0;

    uint64_t                        gpuTimestampFrequency = 0;
    RHI::QueryHeap *                gpuTimestampQueryHeap = nullptr;
    RHI::Buffer *                   gpuTimestampReadbackBuffer = nullptr;
    GpuTimingSlot                   gpuTimingSlots[GpuTimingSlotCount];
    int                             gpuTimingNextSlot = 0;
    int                             gpuTimingCurrentSlot = -1;
};
