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

#include "../RHIRenderer.h"
#include "D3D12Common.h"

class D3D12PipelineState : public RHIRenderer::PipelineState {
public:
    virtual ~D3D12PipelineState() { Release(); }

    void                                    Release();

    struct GraphicsPSStream1 {
        CD3DX12_PIPELINE_STATE_STREAM_FLAGS flags;
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitiveTopologyType;
    };

    struct GraphicsPSStream2 {
        CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC blendDesc;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1 depthStencil;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT depthStencilFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER rasterizer;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS renderTargetFormats;
        CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC sampleDesc;
        CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK sampleMask;
    };

    struct GraphicsPSStream {
        GraphicsPSStream1                   stream1;
        CD3DX12_PIPELINE_STATE_STREAM_VS    vs;
        CD3DX12_PIPELINE_STATE_STREAM_GS    gs;
        CD3DX12_PIPELINE_STATE_STREAM_HS    hs;
        CD3DX12_PIPELINE_STATE_STREAM_DS    ds;
        CD3DX12_PIPELINE_STATE_STREAM_PS    ps;
        //CD3DX12_PIPELINE_STATE_STREAM_MS    ms;
        //CD3DX12_PIPELINE_STATE_STREAM_AS    as;
        GraphicsPSStream2                   stream2;
        CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO shaderCachedPSO;
    };

    struct ComputePSStream {
        CD3DX12_PIPELINE_STATE_STREAM_FLAGS flags;
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_CS    cs;
    };

    ID3D12PipelineState *                   pso = nullptr;
    ID3D12RootSignature *                   rootSignature = nullptr;
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC *rootSignatureDesc = nullptr;
    D3D12_PRIMITIVE_TOPOLOGY                primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
};
