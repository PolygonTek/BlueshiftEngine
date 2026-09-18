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

class GpuFFT;
class GpuTiming;
class ShaderTemplate;
class ShaderPipeline;
class ShaderPipelineBootstrap;
struct BloomFrameData;
struct RenderFrameSettings;

class Bloom {
public:
    void                            Init();
    void                            Shutdown();

    void                            TrackBootstrapPipelines(ShaderPipelineBootstrap &bootstrap) const;

    void                            UpdateFramePSOs();

                                    // Records commands only; does not submit or select a command queue.
                                    // Input and output textures enter and leave in ShaderResource state.
    bool                            Dispatch(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, GpuFFT &gpuFFT, GpuTiming &gpuTiming, RHI::Texture *inputColorTexture, const BloomFrameData &data, const RenderFrameSettings &renderFrameSettings) const;

private:
    bool                            DispatchPyramidBloom(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, GpuTiming &gpuTiming, RHI::Texture *inputColorTexture, const BloomFrameData &data, const RenderFrameSettings &renderFrameSettings) const;
    bool                            DispatchConvolutionBloom(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, GpuFFT &gpuFFT, GpuTiming &gpuTiming, RHI::Texture *inputColorTexture, const BloomFrameData &data, const RenderFrameSettings &renderFrameSettings) const;
    bool                            DispatchConvolutionBloomKernelPrepare(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, GpuTiming &gpuTiming, const BloomFrameData &data) const;
    bool                            DispatchConvolutionBloomScreenPrefilter(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, GpuTiming &gpuTiming, RHI::Texture *inputColorTexture, const BloomFrameData &data, const RenderFrameSettings &renderFrameSettings) const;
    bool                            DispatchConvolutionBloomScreenPrepare(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, GpuTiming &gpuTiming, const BloomFrameData &data) const;
    bool                            DispatchConvolutionBloomMultiplySpectrum(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, GpuFFT &gpuFFT, GpuTiming &gpuTiming, const BloomFrameData &data, const RenderFrameSettings &renderFrameSettings) const;
    bool                            DispatchConvolutionBloomResolve(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, GpuTiming &gpuTiming, const BloomFrameData &data) const;
    bool                            DispatchConvolutionBloomFFT(RHI::CommandList *commandList, GpuFFT &gpuFFT, GpuTiming &gpuTiming, const BloomFrameData &data, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer) const;
    bool                            DispatchConvolutionBloomIFFT(RHI::CommandList *commandList, GpuFFT &gpuFFT, GpuTiming &gpuTiming, const BloomFrameData &data, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer) const;

    SharedPtr<ShaderTemplate>       bloomPrefilterShaderTemplate;
    SharedPtr<ShaderTemplate>       bloomDownsampleShaderTemplate;
    SharedPtr<ShaderTemplate>       bloomUpsampleShaderTemplate;
    SharedPtr<ShaderTemplate>       convolutionBloomScreenPrepareShaderTemplate;
    SharedPtr<ShaderTemplate>       convolutionBloomKernelPrepareShaderTemplate;
    SharedPtr<ShaderTemplate>       convolutionBloomResolveShaderTemplate;

    SharedPtr<ShaderPipeline>       bloomPrefilterPipeline;
    SharedPtr<ShaderPipeline>       bloomDownsamplePipeline;
    SharedPtr<ShaderPipeline>       bloomUpsamplePipeline;
    SharedPtr<ShaderPipeline>       convolutionBloomScreenPreparePipeline;
    SharedPtr<ShaderPipeline>       convolutionBloomKernelPreparePipeline;
    SharedPtr<ShaderPipeline>       convolutionBloomResolvePipeline;

    RHI::PipelineState *            frameBloomPrefilterPSO = nullptr;
    RHI::PipelineState *            frameBloomDownsamplePSO = nullptr;
    RHI::PipelineState *            frameBloomUpsamplePSO = nullptr;
    RHI::PipelineState *            frameConvolutionBloomScreenPreparePSO = nullptr;
    RHI::PipelineState *            frameConvolutionBloomKernelPreparePSO = nullptr;
    RHI::PipelineState *            frameConvolutionBloomResolvePSO = nullptr;
};
