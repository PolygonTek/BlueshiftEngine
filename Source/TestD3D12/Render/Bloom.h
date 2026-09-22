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

class RenderBackend;
class ShaderTemplate;
class ShaderPipeline;
class ShaderPipelineBootstrap;

class Bloom {
public:
    void                            Init();
    void                            Shutdown();

    void                            TrackBootstrapPipelines(ShaderPipelineBootstrap &bootstrap) const;

    void                            UpdateFramePSOs();

    bool                            Dispatch(RenderBackend &backend) const;

private:
    bool                            DispatchPyramidBloom(RenderBackend &backend) const;
    bool                            DispatchConvolutionBloom(RenderBackend &backend) const;
    bool                            DispatchConvolutionBloomKernelPrepare(RenderBackend &backend) const;
    bool                            DispatchConvolutionBloomScreenPrefilter(RenderBackend &backend) const;
    bool                            DispatchConvolutionBloomScreenPrepare(RenderBackend &backend) const;
    bool                            DispatchConvolutionBloomMultiplySpectrum(RenderBackend &backend) const;
    bool                            DispatchConvolutionBloomResolve(RenderBackend &backend) const;
    bool                            DispatchConvolutionBloomFFT(RenderBackend &backend, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer) const;
    bool                            DispatchConvolutionBloomIFFT(RenderBackend &backend, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer) const;

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
