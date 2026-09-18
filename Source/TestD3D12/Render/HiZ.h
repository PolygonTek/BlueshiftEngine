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

class GpuTiming;
class ShaderTemplate;
class ShaderPipeline;
class ShaderPipelineBootstrap;
struct HiZFrameData;

class HiZ {
public:
    void                            Init();
    void                            Shutdown();

    void                            TrackBootstrapPipelines(ShaderPipelineBootstrap &bootstrap) const;
    void                            UpdateFramePSOs();

                                    // Records commands only; does not submit or select a command queue.
                                    // Depth and Hi-Z textures enter and leave in ShaderResource state.
    bool                            DispatchGenerate(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, GpuTiming &gpuTiming, RHI::Texture *depthTexture, const RHI::ProjectionPolicy &projectionPolicy, const HiZFrameData &data) const;

                                    // Hi-Z and preview textures enter and leave in ShaderResource state.
    bool                            DispatchDebugPreview(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::ConstantBuffer *cameraConstantBuffer, GpuTiming &gpuTiming, uint32_t requestedMipLevel, const HiZFrameData &data) const;

                                    // The caller transitions Hi-Z to ShaderResourceCompute and output buffers to UnorderedAccess.
                                    // Resource states remain unchanged between batches.
    bool                            DispatchCullInstances(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::ConstantBuffer *cameraConstantBuffer, uint32_t surfaceTableIndex, const RHI::ProjectionPolicy &projectionPolicy, const HiZFrameData &data, uint32_t instanceCount, RHI::Buffer *inputInstanceToShaderSurfaceTable, uint32_t instanceBaseOffset, uint32_t indirectArgsOffset) const;

private:
    SharedPtr<ShaderTemplate>       firstBatchShaderTemplate;
    SharedPtr<ShaderTemplate>       firstBatchMSAAShaderTemplate;
    SharedPtr<ShaderTemplate>       mipBatchShaderTemplate;
    SharedPtr<ShaderTemplate>       oddDimShaderTemplate;
    SharedPtr<ShaderTemplate>       cullInstancesShaderTemplate;
    SharedPtr<ShaderTemplate>       debugPreviewShaderTemplate;

    SharedPtr<ShaderPipeline>       firstBatchPipeline;
    SharedPtr<ShaderPipeline>       firstBatchMSAAPipeline;
    SharedPtr<ShaderPipeline>       mipBatchPipeline;
    SharedPtr<ShaderPipeline>       oddDimPipeline;
    SharedPtr<ShaderPipeline>       cullInstancesPipeline;
    SharedPtr<ShaderPipeline>       debugPreviewPipeline;

    RHI::PipelineState *            frameFirstBatchPSO = nullptr;
    RHI::PipelineState *            frameFirstBatchMSAAPSO = nullptr;
    RHI::PipelineState *            frameMipBatchPSO = nullptr;
    RHI::PipelineState *            frameOddDimPSO = nullptr;
    RHI::PipelineState *            frameCullInstancesPSO = nullptr;
    RHI::PipelineState *            frameDebugPreviewPSO = nullptr;
};
