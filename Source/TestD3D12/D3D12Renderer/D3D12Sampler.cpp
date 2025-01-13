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
    case RHI::TextureAddressMode::MirroredRepeat:
        return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    case RHI::TextureAddressMode::Clamp:
        return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
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
        renderer->samCpuDescriptorPool->Free(descriptorHandle);
        descriptorHandle.ptr = 0;
    }
}

RHI::Sampler *D3D12Renderer::CreateSampler(const RHI::SamplerDesc *desc) {
    D3D12_SAMPLER_DESC samplerDesc;
    samplerDesc.Filter = ToD3D12TextureFilter(desc->filter);
    samplerDesc.AddressU = ToD3D12TextureAddressMode(desc->addressModeU);
    samplerDesc.AddressV = ToD3D12TextureAddressMode(desc->addressModeV);
    samplerDesc.AddressW = ToD3D12TextureAddressMode(desc->addressModeW);
    samplerDesc.MipLODBias = desc->mipLodBias;
    samplerDesc.MaxAnisotropy = desc->maxAnisotropy;
    ToD3D12TextureBorderColor(desc->borderColor, samplerDesc.BorderColor);
    samplerDesc.MinLOD = desc->minLod;
    samplerDesc.MinLOD = desc->maxLod;

    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = {};
    if (samCpuDescriptorPool->Alloc(&descriptorHandle, nullptr)) {
        return nullptr;
    }

    device->CreateSampler(&samplerDesc, descriptorHandle);

    D3D12Sampler *sampler = new D3D12Sampler;
    sampler->desc = *desc;
    sampler->descriptorHandle = descriptorHandle;
    return sampler;
}

void D3D12Renderer::DestroySampler(RHI::Sampler *sampler, bool immediate) {
    delete sampler;
}

void D3D12Renderer::SetSampler(RHI::CommandList *commandList, int slot, RHI::Sampler *sampler) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    D3D12FrameThreadData *threadData = static_cast<D3D12FrameThreadData *>(d3d12CommandList->GetFrameThreadData());

    // 슬롯 (레지스터) 에 대한 루트 파라미터 인덱스를 얻고, 디스크립터 테이블일 경우 테이블 인덱스도 얻어온다.
    const D3D12PipelineState::Binder &binder = d3d12CommandList->currentPSO->binder;
    int rootParameterIndex = binder.rootParameterBinder.samplers[slot];
    int descriptorIndex = binder.descriptorTableBinder.samplers[slot];

    const D3D12Sampler *d3d12Sampler = static_cast<const D3D12Sampler *>(sampler);
    threadData->tableCpuDescriptorHandles[rootParameterIndex][descriptorIndex] = d3d12Sampler->descriptorHandle;

    if (d3d12CommandList->GetCommandListType() == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
        d3d12CommandList->computeRootParametersDirtyMask |= BIT64(rootParameterIndex);
    } else {
        d3d12CommandList->graphicsRootParametersDirtyMask |= BIT64(rootParameterIndex);
    }
}
