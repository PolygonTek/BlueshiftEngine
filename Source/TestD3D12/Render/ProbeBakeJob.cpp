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
#include "Texture/Texture.h"
#include "Image/Image.h"

static bool CreateProbeOutputCubemapTexture(int32_t size, bool useHDR, bool allocateMipmaps, SharedPtr<Texture> &outTexture) {
    const BE1::Image::Format format = useHDR ? BE1::Image::Format::R11G11B10_FLOAT : BE1::Image::Format::R8G8B8A8;

    // 필터링된 cubemap 은 항상 linear 포맷을 사용한다.
    SharedPtr<BE1::Image> textureImage = MakeShared<BE1::Image>();
    textureImage->InitCubeFromMemory(size, 1, format, BE1::Image::GammaSpace::Linear, nullptr);

    Texture::Flag textureFlags = Texture::Flag::HighQuality | Texture::Flag::NonPowerOfTwo | Texture::Flag::UnorderedAccess;
    if (allocateMipmaps) {
        textureFlags |= Texture::Flag::AllocateEmptyMipmaps;
    } else {
        textureFlags |= Texture::Flag::NoMipmaps;
    }

    SharedPtr<Texture> texture = MakeShared<Texture>();
    if (!texture->CreateCube(textureImage, textureFlags)) {
        return false;
    }

    outTexture = std::move(texture);
    return true;
}

static bool BakeProbeRadianceSH(RHI::CommandList *commandList, const RHI::Texture *radianceCubeMap, SharedPtr<RHI::Buffer> &outCoefficients) {
    constexpr uint32_t bufferSize = GpuSphericalHarmonics::SH3CoefficientBufferSize;
    constexpr uint32_t stride = GpuSphericalHarmonics::SHCoefficientStride;
    RHI::Buffer *buffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::ShaderResource | RHI::BufferFlag::UnorderedAccess, bufferSize, nullptr);
    if (!buffer) {
        return false;
    }

    SharedPtr<RHI::Buffer> coefficients(buffer, [](RHI::Buffer *buffer) { RHI::renderer->DestroyBuffer(buffer); });
    RHI::renderer->SetName(buffer, "ProbeBakeJob::RadianceSHCoefficients");

    const RHI::ViewID srv = RHI::renderer->CreateView(buffer, RHI::ViewType::SRV, RHI::Format::Unknown, stride, 0, bufferSize);
    const RHI::ViewID uav = RHI::renderer->CreateView(buffer, RHI::ViewType::UAV, RHI::Format::Unknown, stride, 0, bufferSize);
    if (!srv.IsValid() || !uav.IsValid()) {
        return false;
    }

    const RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(radianceCubeMap, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(buffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    const bool projected = renderSystem->GetGpuSphericalHarmonics().ProjectCubemapSH3(commandList, radianceCubeMap, buffer, uav);

    const RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(radianceCubeMap, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeBufferBarrier(buffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));
    if (!projected) {
        return false;
    }

    // Lambert convolution 전의 원본 계수를 보관하고, diffuse 재구성에서도 같은 버퍼를 읽는다.
    outCoefficients = std::move(coefficients);
    return true;
}

bool ProbeBakeJob::Execute(RHI::CommandList *commandList) {
    PROFILER_CPU_SCOPED_EVENT("ProbeBakeJob::Execute");

    assert(!renderSystem->IsUsingRenderThread() || RenderSystem::IsInRenderThread());

    diffuseIrradianceCubeMap.reset();
    specularPrefilteredCubeMap.reset();
    radianceSHCoefficients.reset();

    const bool bakeDiffuseIrradiance = BE1::HasFlag(desc.outputFlags, OutputFlag::DiffuseIrradiance);
    const bool bakeSpecularPrefiltered = BE1::HasFlag(desc.outputFlags, OutputFlag::SpecularPrefiltered);
    const bool bakeRadianceSH = BE1::HasFlag(desc.outputFlags, OutputFlag::RadianceSH);

    if (!desc.radianceCubeMap ||
        (!bakeDiffuseIrradiance && !bakeSpecularPrefiltered && !bakeRadianceSH) ||
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

    SharedPtr<RHI::Buffer> radianceSH;
    if (bakeRadianceSH) {
        if (!BakeProbeRadianceSH(commandList, radianceRHITexture, radianceSH)) {
            BE_WARNLOG("ProbeBakeJob::Execute: failed to generate the radiance SH coefficients\n");
            return false;
        }
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

        // SH 출력이 있으면 이미 투영한 계수를 재사용하여 cubemap 을 다시 투영하지 않는다.
        const bool generated = radianceSH ?
            renderSystem->GetIBLBaker().GenerateDiffuseIrradianceFromSH(commandList, radianceSH.get(), diffuseRHITexture) :
            renderSystem->GetIBLBaker().GenerateDiffuseIrradianceSH(commandList, radianceRHITexture, diffuseRHITexture);
        if (!generated) {
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

    if (radianceSH) {
        RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(radianceSH.get(), RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource));
    }

    diffuseIrradianceCubeMap = std::move(diffuseCubeMap);
    specularPrefilteredCubeMap = std::move(specularCubeMap);
    radianceSHCoefficients = std::move(radianceSH);
    return true;
}
