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

    struct Binder {
        void                                Init(const D3D12_ROOT_SIGNATURE_DESC1 &desc);

        struct DescriptorTableBinder {
            uint8_t                         cbv[64];
            uint8_t                         srv[64];
            uint8_t                         uav[64];
            uint8_t                         samplers[64];
        };

        struct RootParameterBinder {
            uint8_t                         cbv[16];
            uint8_t                         srv[16];
            uint8_t                         uav[16];
            uint8_t                         samplers[16];
            uint8_t                         constants;
        };

        DescriptorTableBinder               descriptorTableBinder;
        RootParameterBinder                 rootParameterBinder;
    };

    ID3D12PipelineState *                   pso = nullptr;
    ID3D12RootSignature *                   rootSignature = nullptr;
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC *rootSignatureDesc = nullptr;
    Binder                                  binder;
    D3D12_PRIMITIVE_TOPOLOGY                primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
};

BE_INLINE void D3D12PipelineState::Binder::Init(const D3D12_ROOT_SIGNATURE_DESC1 &desc) {
    // 레지스터 인덱스 별 디스크립터 테이블 인덱스와 루트 파라미터 인덱스를 미리 계산한다.
    for (int rootParameterIndex = 0; rootParameterIndex < desc.NumParameters; ++rootParameterIndex) {
        const D3D12_ROOT_PARAMETER1 *rootParameter = &desc.pParameters[rootParameterIndex];

        if (rootParameter->ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
            int descriptorOffset = 0;

            for (UINT rangeIndex = 0; rangeIndex < rootParameter->DescriptorTable.NumDescriptorRanges; ++rangeIndex) {
                const D3D12_DESCRIPTOR_RANGE1 &descriptorRange = rootParameter->DescriptorTable.pDescriptorRanges[rangeIndex];

                switch (descriptorRange.RangeType) {
                case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
                    assert(descriptorRange.BaseShaderRegister + descriptorRange.NumDescriptors <= COUNT_OF(rootParameterBinder.cbv));
                    assert(descriptorRange.BaseShaderRegister + descriptorRange.NumDescriptors <= COUNT_OF(descriptorTableBinder.cbv));
                    for (UINT i = 0; i < descriptorRange.NumDescriptors; ++i) {
                        UINT shaderRegister = descriptorRange.BaseShaderRegister + i;
                        rootParameterBinder.cbv[shaderRegister] = rootParameterIndex;
                        descriptorTableBinder.cbv[shaderRegister] = descriptorOffset + i;
                    }
                    break;
                case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
                    assert(descriptorRange.BaseShaderRegister + descriptorRange.NumDescriptors <= COUNT_OF(rootParameterBinder.srv));
                    assert(descriptorRange.BaseShaderRegister + descriptorRange.NumDescriptors <= COUNT_OF(descriptorTableBinder.srv));
                    for (UINT i = 0; i < descriptorRange.NumDescriptors; ++i) {
                        UINT shaderRegister = descriptorRange.BaseShaderRegister + i;
                        rootParameterBinder.srv[shaderRegister] = rootParameterIndex;
                        descriptorTableBinder.srv[shaderRegister] = descriptorOffset + i;
                    }
                    break;
                case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
                    assert(descriptorRange.BaseShaderRegister + descriptorRange.NumDescriptors <= COUNT_OF(rootParameterBinder.uav));
                    assert(descriptorRange.BaseShaderRegister + descriptorRange.NumDescriptors <= COUNT_OF(descriptorTableBinder.uav));
                    for (UINT i = 0; i < descriptorRange.NumDescriptors; ++i) {
                        UINT shaderRegister = descriptorRange.BaseShaderRegister + i;
                        rootParameterBinder.uav[shaderRegister] = rootParameterIndex;
                        descriptorTableBinder.uav[shaderRegister] = descriptorOffset + i;
                    }
                    break;
                case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
                    assert(descriptorRange.BaseShaderRegister + descriptorRange.NumDescriptors <= COUNT_OF(rootParameterBinder.samplers));
                    assert(descriptorRange.BaseShaderRegister + descriptorRange.NumDescriptors <= COUNT_OF(descriptorTableBinder.samplers));
                    for (UINT i = 0; i < descriptorRange.NumDescriptors; ++i) {
                        UINT shaderRegister = descriptorRange.BaseShaderRegister + i;
                        rootParameterBinder.samplers[shaderRegister] = rootParameterIndex;
                        descriptorTableBinder.samplers[shaderRegister] = descriptorOffset + i;
                    }
                    break;
                }

                descriptorOffset += descriptorRange.NumDescriptors;
            }
        } else if (rootParameter->ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS) {
            // NOTE: 현재 구현에서는 32bit 상수를 한개의 루트 파라미터만 지원한다.
            rootParameterBinder.constants = rootParameterIndex;
        } else if (rootParameter->ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV) {
            UINT shaderRegister = rootParameter->Descriptor.ShaderRegister;
            assert(shaderRegister < COUNT_OF(rootParameterBinder.cbv));
            rootParameterBinder.cbv[shaderRegister] = rootParameterIndex;
        } else if (rootParameter->ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV) {
            UINT shaderRegister = rootParameter->Descriptor.ShaderRegister;
            assert(shaderRegister < COUNT_OF(rootParameterBinder.srv));
            rootParameterBinder.srv[shaderRegister] = rootParameterIndex;
        } else if (rootParameter->ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV) {
            UINT shaderRegister = rootParameter->Descriptor.ShaderRegister;
            assert(shaderRegister < COUNT_OF(rootParameterBinder.uav));
            rootParameterBinder.uav[shaderRegister] = rootParameterIndex;
        }
    }
}
