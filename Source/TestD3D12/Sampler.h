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

#include "RHI.h"

class SamplerManager;

class Sampler {
    friend class SamplerManager;

public:
    union Hasher {
        struct {
            uint32_t            filter : 5;
            uint32_t            addressModeU : 5;
            uint32_t            addressModeV : 5;
            uint32_t            addressModeW : 5;
            uint32_t            maxAnisotropy : 5;
            uint32_t            borderColor : 5;
        } bits;
        uint32_t                value;
    };

    RHI::TextureFilter          GetFilter() const { return (RHI::TextureFilter)hash.bits.filter; }
    RHI::TextureAddressMode     GetAddressModeU() const { return (RHI::TextureAddressMode)hash.bits.addressModeU; }
    RHI::TextureAddressMode     GetAddressModeV() const { return (RHI::TextureAddressMode)hash.bits.addressModeV; }
    RHI::TextureAddressMode     GetAddressModeW() const { return (RHI::TextureAddressMode)hash.bits.addressModeW; }
    uint32_t                    GetMaxAnisotropy() const { return hash.bits.maxAnisotropy; }
    RHI::TextureBorderColor     GetBorderColor() const { return (RHI::TextureBorderColor)hash.bits.borderColor; }
    RHI::Sampler *              GetRHISampler() const { return sampler; }

private:
    Hasher                      hash;
    mutable int                 refCount = 0;
    RHI::Sampler *              sampler = nullptr;
};

class SamplerManager {
    friend class Sampler;

public:
    void                        Init();
    void                        Shutdown();

    Sampler *                   FindSampler(uint32_t hash) const;
    Sampler *                   FindSampler(RHI::TextureFilter filter,
                                    RHI::TextureAddressMode addressModeU = RHI::TextureAddressMode::Clamp,
                                    RHI::TextureAddressMode addressModeV = RHI::TextureAddressMode::Clamp,
                                    RHI::TextureAddressMode addressModeW = RHI::TextureAddressMode::Clamp,
                                    uint8_t maxAnisotropy = 1, RHI::TextureBorderColor borderColor = RHI::TextureBorderColor::OpaqueBlack) const;
    Sampler *                   GetSampler(RHI::TextureFilter filter,
                                    RHI::TextureAddressMode addressModeU = RHI::TextureAddressMode::Clamp,
                                    RHI::TextureAddressMode addressModeV = RHI::TextureAddressMode::Clamp,
                                    RHI::TextureAddressMode addressModeW = RHI::TextureAddressMode::Clamp,
                                    uint8_t maxAnisotropy = 1, RHI::TextureBorderColor borderColor = RHI::TextureBorderColor::OpaqueBlack);
    void                        ReleaseSampler(Sampler *sampler);
    void                        DestroySampler(Sampler *sampler);

                                // 샘플러의 전역 설정 변경 함수들은 마지막에 UpdateSamplers 를 호출해줘야 반영된다.
    void                        SetMipLODBias(float bias);
    void                        SetMinLOD(float minLod);
    void                        SetMaxLOD(float maxLod);
    void                        UpdateSamplers();

private:
    float                       mipLodBias = 0.0f;
    float                       minLod = 0.0f;
    float                       maxLod = std::numeric_limits<float>::max();
    bool                        needUpdateSamplers = false;

    BE1::HashMap<uint32_t, Sampler *> samplerHashMap;
};

extern SamplerManager           samplerManager;
