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
#include "ProbeBakeJob.h"
#include "RenderSystem.h"
#include "Render/Texture/Texture.h"
#include "Image/Image.h"

static bool CreateProbeOutputCubemapTexture(int32_t size, bool useHDR, bool allocateMipmaps, SharedPtr<Texture> &outTexture) {
    const BE1::Image::Format format = useHDR ? BE1::Image::Format::R11G11B10_FLOAT : BE1::Image::Format::R8G8B8A8;

    // 필터링된 cubemap 은 항상 linear 포맷을 사용한다.
    SharedPtr<BE1::Image> textureImage = MakeShared<BE1::Image>();
    textureImage->InitFromMemory(size, size, 1, 6, 1, format, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::CubeMap);

    Texture::Flag textureFlags = Texture::Flag::HighQuality | Texture::Flag::NonPowerOfTwo | Texture::Flag::UnorderedAccess;
    if (allocateMipmaps) {
        textureFlags |= Texture::Flag::AllocateEmptyMipmaps;
    } else {
        textureFlags |= Texture::Flag::NoMipmaps;
    }

    SharedPtr<Texture> texture = MakeShared<Texture>();
    if (!texture->Create(RHI::TextureType::TextureCube, textureImage, textureFlags)) {
        return false;
    }

    outTexture = std::move(texture);
    return true;
}

bool ProbeBakeJob::Execute(RHI::CommandList *commandList) {
    PROFILER_CPU_SCOPED_EVENT("ProbeBakeJob::Execute");

    assert(!renderSystem->IsUsingRenderThread() || RenderSystem::IsInRenderThread());

    diffuseIrradianceCubeMap.reset();
    specularPrefilteredCubeMap.reset();

    const bool bakeDiffuseIrradiance = BE1::HasFlag(desc.outputFlags, OutputFlag::DiffuseIrradiance);
    const bool bakeSpecularPrefiltered = BE1::HasFlag(desc.outputFlags, OutputFlag::SpecularPrefiltered);

    if (!desc.radianceCubeMap ||
        (!bakeDiffuseIrradiance && !bakeSpecularPrefiltered) ||
        (bakeDiffuseIrradiance && desc.diffuseIrradianceCubemapSize <= 0) ||
        (bakeSpecularPrefiltered && desc.specularPrefilteredSize <= 0)) {
        BE_WARNLOG("ProbeBakeJob::Execute: invalid bake parameters\n");
        return false;
    }

    RHI::Texture *radianceRHITexture = desc.radianceCubeMap->GetRHITexture();
    if (!radianceRHITexture || radianceRHITexture->GetStatus() != RHI::GPUResourceStatus::Ready) {
        BE_WARNLOG("ProbeBakeJob::Execute: radiance cubemap resource is not ready\n");
        return false;
    }

    SharedPtr<Texture> diffuseCubeMap;
    if (bakeDiffuseIrradiance) {
        if (!CreateProbeOutputCubemapTexture(desc.diffuseIrradianceCubemapSize, desc.useHDR, false, diffuseCubeMap)) {
            BE_WARNLOG("ProbeBakeJob::Execute: failed to create the diffuse irradiance cubemap\n");
            return false;
        }

        RHI::Texture *diffuseRHITexture = diffuseCubeMap->GetRHITexture();
        if (!diffuseRHITexture || diffuseRHITexture->GetStatus() != RHI::GPUResourceStatus::Ready) {
            BE_WARNLOG("ProbeBakeJob::Execute: diffuse irradiance cubemap resource is not ready\n");
            return false;
        }

#if 0
        if (!renderSystem->GetIBLBaker().GenerateDiffuseIrradiance(commandList, radianceRHITexture, diffuseRHITexture)) {
#else
        if (!renderSystem->GetIBLBaker().GenerateDiffuseIrradianceSH(commandList, radianceRHITexture, diffuseRHITexture)) {
#endif
            BE_WARNLOG("ProbeBakeJob::Execute: failed to generate the diffuse irradiance cubemap\n");
            return false;
        }
    }

    SharedPtr<Texture> specularCubeMap;
    if (bakeSpecularPrefiltered) {
        if (!CreateProbeOutputCubemapTexture(desc.specularPrefilteredSize, desc.useHDR, true, specularCubeMap)) {
            BE_WARNLOG("ProbeBakeJob::Execute: failed to create the specular prefiltered cubemap\n");
            return false;
        }

        RHI::Texture *specularRHITexture = specularCubeMap->GetRHITexture();
        if (!specularRHITexture || specularRHITexture->GetStatus() != RHI::GPUResourceStatus::Ready) {
            BE_WARNLOG("ProbeBakeJob::Execute: specular prefiltered cubemap resource is not ready\n");
            return false;
        }

        const uint32_t specularMipLevelCount = specularRHITexture->GetMipLevelCount();
        RHI::ViewID *specularMipUAVs = static_cast<RHI::ViewID *>(_alloca(sizeof(RHI::ViewID) * specularMipLevelCount));
        for (uint32_t mipLevel = 0; mipLevel < specularMipLevelCount; ++mipLevel) {
            specularMipUAVs[mipLevel] = RHI::renderer->CreateView(specularRHITexture, RHI::ViewType::UAV, RHI::Format::Auto, 0, specularRHITexture->GetArraySize(), mipLevel, 1);
            if (!specularMipUAVs[mipLevel].IsValid()) {
                BE_WARNLOG("ProbeBakeJob::Execute: failed to create specular cubemap mip views\n");
                return false;
            }
        }

        if (!renderSystem->GetIBLBaker().GenerateLDSumGGX(commandList, radianceRHITexture, specularRHITexture, specularMipUAVs)) {
            BE_WARNLOG("ProbeBakeJob::Execute: failed to generate the specular prefiltered cubemap\n");
            return false;
        }
    }

    diffuseIrradianceCubeMap = std::move(diffuseCubeMap);
    specularPrefilteredCubeMap = std::move(specularCubeMap);
    return true;
}
