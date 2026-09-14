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

#include "../RHI.h"
#include "D3D12Common.h"
#include "D3D12RootBindingData.h"

class D3D12Renderer;
class D3D12CommandList;

/*
---------------------------------------------------------------------------------------------------

    Pipeline State

    - Graphics PSO 생성 시 이미 캐시된 PSO Blob 파일이 있으면 이용해서 PSO 를 생성하고, 없으면 새로 갱신한다.
    - PSO 생성 단계에서 루트 시그니처의 모든 리소스에 대한 레지스터 인덱스에 대한 바인딩 정보를 미리 계산하여 RegisterBinder 구조체에 저장한다.
    - 이를 통해 커맨드 리스트에 루트 시그니처를 설정할 때, 각 리소스의 셰이더 레지스터 인덱스에 해당하는 루트 파라미터 인덱스와 디스크립터 테이블 내의 인덱스를 빠르게 찾아낼 수 있다.

---------------------------------------------------------------------------------------------------
*/

class D3D12PipelineState : public RHI::PipelineState {
    friend class D3D12Renderer;
    friend class D3D12CommandList;

public:
    virtual ~D3D12PipelineState() { Release(); }

    void                            Release();

private:
    struct RegisterBinder {
        void                        Init(const D3D12_ROOT_SIGNATURE_DESC1 &desc);

        // 각 바인딩 리소스 (CBV/SRV/UAV/Sampler/RootConstants) 의 shader register 인덱스에 대한 루트 파라미터 인덱스 Look-Up Table
        struct RootParameterIndexLUT {
            RootParameterIndexLUT() { memset(this, 0xFF, sizeof(*this)); }

            uint8_t                 cbv[16];
            uint8_t                 srv[64];
            uint8_t                 uav[8];
            uint8_t                 samplers[32];
            uint8_t                 constants[4];
        };

        // 각 바인딩 리소스 (CBV/SRV/UAV/Sampler) 의 shader register 인덱스에 대한 테이블 내의 디스크립터 인덱스 Look-Up Table
        struct DescriptorIndexInTableLUT {
            DescriptorIndexInTableLUT() { memset(this, 0xFF, sizeof(*this)); }

            uint8_t                 cbv[16];
            uint8_t                 srv[64];
            uint8_t                 uav[8];
            uint8_t                 samplers[32];
        };

        RootParameterIndexLUT       rootParameterIndexLUT;
        DescriptorIndexInTableLUT   descriptorIndexInTableLUT;

                                    // 바인드리스를 지원하기 위한 fallback 디스크립터 테이블의 루트 파라미터 마스크
        uint64_t                    bindlessResourceRootParameterMask = 0;
        uint64_t                    bindlessSamplerRootParameterMask = 0;
    };

    ID3D12PipelineState *           pso = nullptr;
    ID3D12RootSignature *           rootSignature = nullptr;
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC *rootSignatureDesc = nullptr;
    RegisterBinder                  binder; // 루트 시그니처의 모든 리소스에 대한 레지스터 인덱스의 바인딩 정보를 갖는다.
    D3D12_PRIMITIVE_TOPOLOGY        primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
};

// 루트 시그니처 정보로 리소스 종류 별 shader register 인덱스에 해당하는 루트 파라미터 인덱스와 디스크립터 테이블 내의 인덱스를 미리 계산한다.
// PSO 생성 단계에서 호출된다.
BE_INLINE void D3D12PipelineState::RegisterBinder::Init(const D3D12_ROOT_SIGNATURE_DESC1 &desc) {
    // 루트 파라미터 인덱스는 64 bit dirty mask 에 맞춰야 하므로 64 개를 넘을 수 없다.
    assert(desc.NumParameters <= 64);

    for (int rootParameterIndex = 0; rootParameterIndex < desc.NumParameters; ++rootParameterIndex) {
        const D3D12_ROOT_PARAMETER1 *rootParameter = &desc.pParameters[rootParameterIndex];

        // 루트 파라미터가 디스크립터 테이블인가?
        if (rootParameter->ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
            // 디스크립터 테이블 내에서의 디스크립터 오프셋
            int offsetInTable = 0;

            // 디스크립터 테이블 내의 디스크립터 레인지 개수만큼 반복 (각 레인지는 CBV, SRV, UAV, Sampler 중 하나의 타입을 가진다)
            for (UINT rangeIndex = 0; rangeIndex < rootParameter->DescriptorTable.NumDescriptorRanges; ++rangeIndex) {
                const D3D12_DESCRIPTOR_RANGE1 &range = rootParameter->DescriptorTable.pDescriptorRanges[rangeIndex];

                // 레지스터 스페이스 == 0 인 경우에만 shader register 인덱스에 대한 루트 파라미터 인덱스와 디스크립터 테이블 내의 인덱스를 계산하여 LUT 에 저장한다.
                if (range.RegisterSpace == 0) {
                    switch (range.RangeType) {
                    case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
                        assert(range.BaseShaderRegister + range.NumDescriptors <= COUNT_OF(rootParameterIndexLUT.cbv));
                        assert(range.BaseShaderRegister + range.NumDescriptors <= COUNT_OF(descriptorIndexInTableLUT.cbv));
                        for (UINT i = 0; i < range.NumDescriptors; ++i) {
                            UINT shaderRegister = range.BaseShaderRegister + i;
                            rootParameterIndexLUT.cbv[shaderRegister] = rootParameterIndex;
                            descriptorIndexInTableLUT.cbv[shaderRegister] = offsetInTable + i;
                        }
                        break;
                    case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
                        assert(range.BaseShaderRegister + range.NumDescriptors <= COUNT_OF(rootParameterIndexLUT.srv));
                        assert(range.BaseShaderRegister + range.NumDescriptors <= COUNT_OF(descriptorIndexInTableLUT.srv));
                        for (UINT i = 0; i < range.NumDescriptors; ++i) {
                            UINT shaderRegister = range.BaseShaderRegister + i;
                            rootParameterIndexLUT.srv[shaderRegister] = rootParameterIndex;
                            descriptorIndexInTableLUT.srv[shaderRegister] = offsetInTable + i;
                        }
                        break;
                    case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
                        assert(range.BaseShaderRegister + range.NumDescriptors <= COUNT_OF(rootParameterIndexLUT.uav));
                        assert(range.BaseShaderRegister + range.NumDescriptors <= COUNT_OF(descriptorIndexInTableLUT.uav));
                        for (UINT i = 0; i < range.NumDescriptors; ++i) {
                            UINT shaderRegister = range.BaseShaderRegister + i;
                            rootParameterIndexLUT.uav[shaderRegister] = rootParameterIndex;
                            descriptorIndexInTableLUT.uav[shaderRegister] = offsetInTable + i;
                        }
                        break;
                    case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
                        assert(range.BaseShaderRegister + range.NumDescriptors <= COUNT_OF(rootParameterIndexLUT.samplers));
                        assert(range.BaseShaderRegister + range.NumDescriptors <= COUNT_OF(descriptorIndexInTableLUT.samplers));
                        for (UINT i = 0; i < range.NumDescriptors; ++i) {
                            UINT shaderRegister = range.BaseShaderRegister + i;
                            rootParameterIndexLUT.samplers[shaderRegister] = rootParameterIndex;
                            descriptorIndexInTableLUT.samplers[shaderRegister] = offsetInTable + i;
                        }
                        break;
                    }

                    offsetInTable += range.NumDescriptors;
                } else {
                    // 레지스터 스페이스 > 0 인 경우는 bindless 용으로 사용한다.
                    if (range.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER) {
                        bindlessSamplerRootParameterMask |= BIT64(rootParameterIndex);
                    } else {
                        bindlessResourceRootParameterMask |= BIT64(rootParameterIndex);
                    }
                }
            }
        } else {
            if (rootParameter->ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS) {
                // 루트 상수
                if (rootParameter->Constants.RegisterSpace == 0) {
                    UINT shaderRegister = rootParameter->Constants.ShaderRegister;
                    assert(shaderRegister < COUNT_OF(rootParameterIndexLUT.constants));
                    assert(rootParameter->Constants.Num32BitValues <= D3D12RootBindingData::MaxRootConstant32BitValues);
                    rootParameterIndexLUT.constants[shaderRegister] = rootParameterIndex;
                }
            } else if (rootParameter->ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV) {
                // 루트 CBV
                if (rootParameter->Descriptor.RegisterSpace == 0) {
                    UINT shaderRegister = rootParameter->Descriptor.ShaderRegister;
                    assert(shaderRegister < COUNT_OF(rootParameterIndexLUT.cbv));
                    rootParameterIndexLUT.cbv[shaderRegister] = rootParameterIndex;
                }
            } else if (rootParameter->ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV) {
                // 루트 SRV
                if (rootParameter->Descriptor.RegisterSpace == 0) {
                    UINT shaderRegister = rootParameter->Descriptor.ShaderRegister;
                    assert(shaderRegister < COUNT_OF(rootParameterIndexLUT.srv));
                    rootParameterIndexLUT.srv[shaderRegister] = rootParameterIndex;
                }
            } else if (rootParameter->ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV) {
                // 루트 UAV
                if (rootParameter->Descriptor.RegisterSpace == 0) {
                    UINT shaderRegister = rootParameter->Descriptor.ShaderRegister;
                    assert(shaderRegister < COUNT_OF(rootParameterIndexLUT.uav));
                    rootParameterIndexLUT.uav[shaderRegister] = rootParameterIndex;
                }
            }
        }
    }
}
