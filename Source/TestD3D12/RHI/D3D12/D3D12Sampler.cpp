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

#include "Precompiled.h"
#include "D3D12Renderer.h"
#include "D3D12Sampler.h"
#include "D3D12DescriptorPool.h"
#include "D3D12CommandList.h"

static constexpr D3D12_FILTER ToD3D12TextureFilter(RHI::TextureFilter filter) {
    switch (filter) {
    case RHI::TextureFilter::NearestMipmapNearest:
        return D3D12_FILTER_MIN_MAG_MIP_POINT;
    case RHI::TextureFilter::LinearMipmapNearest:
        return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    case RHI::TextureFilter::NearestMipmapLinear:
        return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
    case RHI::TextureFilter::LinearMipmapLinear:
        return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    case RHI::TextureFilter::Anisotropic:
        return D3D12_FILTER_ANISOTROPIC;
    }
    assert(0);
    return D3D12_FILTER_MIN_MAG_MIP_POINT;
}

static constexpr D3D12_TEXTURE_ADDRESS_MODE ToD3D12TextureAddressMode(RHI::TextureAddressMode addressMode) {
    switch (addressMode) {
    case RHI::TextureAddressMode::Repeat:
        return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    case RHI::TextureAddressMode::Clamp:
        return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    case RHI::TextureAddressMode::MirroredRepeat:
        return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    case RHI::TextureAddressMode::MirroredClamp:
        return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
    case RHI::TextureAddressMode::ClampToBorder:
        return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    }
    assert(0);
    return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
}

static void ToD3D12TextureBorderColor(RHI::TextureBorderColor borderColor, FLOAT *float4BorderColor) {
    switch (borderColor) {
    case RHI::TextureBorderColor::OpaqueBlack:
        float4BorderColor[0] = 0.0f;
        float4BorderColor[1] = 0.0f;
        float4BorderColor[2] = 0.0f;
        float4BorderColor[3] = 1.0f;
        return;
    case RHI::TextureBorderColor::OpaqueWhite:
        float4BorderColor[0] = 1.0f;
        float4BorderColor[1] = 1.0f;
        float4BorderColor[2] = 1.0f;
        float4BorderColor[3] = 1.0f;
        return;
    case RHI::TextureBorderColor::TransparentBlack:
        float4BorderColor[0] = 0.0f;
        float4BorderColor[1] = 0.0f;
        float4BorderColor[2] = 0.0f;
        float4BorderColor[3] = 0.0f;
        return;
    }
    assert(0);
}

void D3D12Sampler::Release() {
    if (descriptorHandle.ptr != 0) {
        D3D12Renderer::GetRenderer()->samplerStagingDescriptorPool->Free(descriptorHandle);
        descriptorHandle = {};
    }

    if (bindlessIndex != D3D12BindlessDescriptorAllocator::NullIndex) {
        D3D12Renderer::GetRenderer()->FreeBindlessSamplerDescriptor(bindlessIndex);
        bindlessIndex = D3D12BindlessDescriptorAllocator::NullIndex;
    }
}

RHI::Sampler *D3D12Renderer::CreateSampler(const RHI::SamplerDesc *desc) {
    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = {};
    if (!samplerStagingDescriptorPool->Alloc(&descriptorHandle, nullptr)) {
        return nullptr;
    }

    D3D12Sampler *sampler = new D3D12Sampler;
    sampler->desc = *desc;
    sampler->descriptorHandle = descriptorHandle;
    sampler->bindlessIndex = AllocBindlessSamplerDescriptor();
    if (sampler->bindlessIndex == D3D12BindlessDescriptorAllocator::NullIndex) {
        samplerStagingDescriptorPool->Free(descriptorHandle);
        sampler->descriptorHandle = {};
        delete sampler;
        return nullptr;
    }

    RecreateSampler(sampler, desc);
    return sampler;
}

void D3D12Renderer::DestroySampler(RHI::Sampler *sampler, bool immediate) {
    if (immediate) {
        delete sampler;
    } else {
        DeleteGPUObject(sampler);
    }
}

void D3D12Renderer::RecreateSampler(RHI::Sampler* sampler, const RHI::SamplerDesc *desc) {
    D3D12_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = ToD3D12TextureFilter(desc->filter);
    samplerDesc.AddressU = ToD3D12TextureAddressMode(desc->addressModeU);
    samplerDesc.AddressV = ToD3D12TextureAddressMode(desc->addressModeV);
    samplerDesc.AddressW = ToD3D12TextureAddressMode(desc->addressModeW);
    samplerDesc.MipLODBias = desc->mipLodBias;
    samplerDesc.MaxAnisotropy = desc->maxAnisotropy;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NONE;
    ToD3D12TextureBorderColor(desc->borderColor, samplerDesc.BorderColor);
    samplerDesc.MinLOD = desc->minLod;
    samplerDesc.MaxLOD = desc->maxLod;

    D3D12Sampler *_sampler = static_cast<D3D12Sampler *>(sampler);
    device->CreateSampler(&samplerDesc, _sampler->descriptorHandle);

    D3D12_CPU_DESCRIPTOR_HANDLE bindlessCpuDescriptorHandle = GetBindlessSamplerCPUDescriptorHandle(_sampler->bindlessIndex);
    device->CopyDescriptorsSimple(1, bindlessCpuDescriptorHandle, _sampler->descriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}

void D3D12Renderer::SetSampler(RHI::CommandList *commandList, uint32_t shaderRegister, RHI::Sampler *sampler) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);

    // 현재 세팅된 PSO 의 셰이더 레지스터에 대한 루트 파라미터 인덱스를 얻는다.
    const D3D12PipelineState::RegisterBinder &binder = _commandList->currentPSO->binder;
    assert(shaderRegister < COUNT_OF(binder.rootParameterIndexLUT.samplers));
    uint8_t rootParameterIndex = binder.rootParameterIndexLUT.samplers[shaderRegister];
    if (rootParameterIndex >= 64) {
        BE_ERRLOG("D3D12Renderer::SetSampler: Invalid root parameter index\n");
        return;
    }

    // 일반 Sampler 는 디스크립터 테이블로만 전달될 수 있으므로, 디스크립터 테이블 내의 인덱스를 얻어온다.
    int indexInTable = binder.descriptorIndexInTableLUT.samplers[shaderRegister];
    if (indexInTable == 0xFF) {
        BE_ERRLOG("D3D12Renderer::SetSampler: Invalid descriptor index in descriptor table\n");
        return;
    }

    // 사용할 디스크립터 테이블 변수에 디스크립터 핸들을 채워 넣는다.
    const D3D12Sampler *_sampler = static_cast<const D3D12Sampler *>(sampler);
    _commandList->GetRootBindingData()->tableDescriptorHandles[rootParameterIndex][indexInTable] = _sampler->descriptorHandle;

    // 변경할 루트 파라미터 인덱스에 대한 dirty mask 를 설정한다.
    if (_commandList->currentPSO->isGraphics) {
        _commandList->graphicsRootParametersDirtyMask |= BIT64(rootParameterIndex);
    } else {
        _commandList->computeRootParametersDirtyMask |= BIT64(rootParameterIndex);
    }
}
