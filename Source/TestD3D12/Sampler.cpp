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
#include "Sampler.h"

SamplerManager      samplerManager;

void SamplerManager::Init() {
    samplerHashMap.Init(512, 512, 512);
}

void SamplerManager::Shutdown() {
    samplerHashMap.DeleteContents(true);
}

Sampler *SamplerManager::FindSampler(uint32_t hash) const {
    const auto *entry = samplerHashMap.Get(hash);
    if (entry) {
        return entry->second;
    }
    return nullptr;
}

Sampler *SamplerManager::FindSampler(RHI::TextureFilter filter,
    RHI::TextureAddressMode addressModeU,
    RHI::TextureAddressMode addressModeV,
    RHI::TextureAddressMode addressModeW,
    uint8_t maxAnisotropy, RHI::TextureBorderColor borderColor) const {
    Sampler::Hasher hasher;
    hasher.bits.filter = (uint32_t)filter;
    hasher.bits.addressModeU = (uint32_t)addressModeU;
    hasher.bits.addressModeV = (uint32_t)addressModeV;
    hasher.bits.addressModeW = (uint32_t)addressModeW;
    hasher.bits.maxAnisotropy = (uint32_t)maxAnisotropy;
    hasher.bits.borderColor = (uint32_t)borderColor;
    return FindSampler(hasher.value);
}

Sampler *SamplerManager::GetSampler(RHI::TextureFilter filter,
    RHI::TextureAddressMode addressModeU,
    RHI::TextureAddressMode addressModeV,
    RHI::TextureAddressMode addressModeW,
    uint8_t maxAnisotropy, RHI::TextureBorderColor borderColor) {
    Sampler::Hasher hasher;
    hasher.bits.filter = (uint32_t)filter;
    hasher.bits.addressModeU = (uint32_t)addressModeU;
    hasher.bits.addressModeV = (uint32_t)addressModeV;
    hasher.bits.addressModeW = (uint32_t)addressModeW;
    hasher.bits.maxAnisotropy = (uint32_t)maxAnisotropy;
    hasher.bits.borderColor = (uint32_t)borderColor;
    Sampler *sampler = FindSampler(hasher.value);
    if (sampler) {
        sampler->refCount++;
        return sampler;
    }

    RHI::SamplerDesc samplerDesc;
    samplerDesc.filter = filter;
    samplerDesc.addressModeU = addressModeU;
    samplerDesc.addressModeV = addressModeV;
    samplerDesc.addressModeW = addressModeW;
    samplerDesc.maxAnisotropy = maxAnisotropy;
    samplerDesc.mipLodBias = mipLodBias;
    samplerDesc.minLod = minLod;
    samplerDesc.maxLod = maxLod;

    sampler = new Sampler;
    sampler->sampler = RHI::renderer->CreateSampler(&samplerDesc);
    sampler->refCount = 1;
    sampler->hash = hasher;
    samplerHashMap.Set(hasher.value, sampler);

    return sampler;
}

void SamplerManager::ReleaseSampler(Sampler *sampler) {
    if (sampler->refCount > 0) {
        sampler->refCount--;
    }

    if (sampler->refCount == 0) {
        DestroySampler(sampler);
    }
}

void SamplerManager::DestroySampler(Sampler *sampler) {
    if (sampler->refCount > 1) {
        BE_WARNLOG("SamplerManager::DestroySampler: sampler 0x%x has %i reference count\n", sampler->hash.value, sampler->refCount);
    }

    RHI::renderer->DestroySampler(sampler->sampler);

    samplerHashMap.Remove(sampler->hash.value);

    delete sampler;
}

void SamplerManager::SetMipLODBias(float bias) {
    if (mipLodBias == bias) {
        return;
    }
    mipLodBias = bias;
    needUpdateSamplers = true;
}

void SamplerManager::SetMinLOD(float minLod) {
    if (this->minLod == minLod) {
        return;
    }
    this->minLod = minLod;
    needUpdateSamplers = true;
}

void SamplerManager::SetMaxLOD(float maxLod) {
    if (this->maxLod == maxLod) {
        return;
    }
    this->maxLod = maxLod;
    needUpdateSamplers = true;
}

void SamplerManager::UpdateSamplers() {
    if (!needUpdateSamplers) {
        return;
    }

    for (const auto &entry : samplerHashMap) {
        Sampler *sampler = entry.second;

        RHI::SamplerDesc samplerDesc;
        samplerDesc.filter = (RHI::TextureFilter)sampler->hash.bits.filter;
        samplerDesc.addressModeU = (RHI::TextureAddressMode)sampler->hash.bits.addressModeU;
        samplerDesc.addressModeV = (RHI::TextureAddressMode)sampler->hash.bits.addressModeV;
        samplerDesc.addressModeW = (RHI::TextureAddressMode)sampler->hash.bits.addressModeW;
        samplerDesc.maxAnisotropy = sampler->hash.bits.maxAnisotropy;
        samplerDesc.borderColor = (RHI::TextureBorderColor)sampler->hash.bits.borderColor;
        samplerDesc.mipLodBias = mipLodBias;
        samplerDesc.minLod = minLod;
        samplerDesc.maxLod = maxLod;

        RHI::renderer->RecreateSampler(sampler->sampler, &samplerDesc);
    }

    needUpdateSamplers = false;
}
