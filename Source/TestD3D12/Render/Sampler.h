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

class SamplerManager;

class Sampler {
    friend class SamplerManager;

public:
    union Hasher {
        struct {
            uint32_t            filter : 3;
            uint32_t            addressModeU : 3;
            uint32_t            addressModeV : 3;
            uint32_t            addressModeW : 3;
            uint32_t            maxAnisotropy : 5;
            uint32_t            borderColor : 3;
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
    uint32_t                    GetBindlessIndex() const { return sampler->GetBindlessIndex(); }

private:
    Hasher                      hash;
    mutable int32_t             refCount = 0;
    RHI::Sampler *              sampler = nullptr;
};
