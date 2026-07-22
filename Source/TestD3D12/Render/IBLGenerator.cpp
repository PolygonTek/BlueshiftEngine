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
#include "IBLGenerator.h"
#include "Image/Image.h"
#include "Render/ShaderPipeline/ShaderPipeline.h"
#include "Render/ShaderPipeline/ShaderPipelineCache.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"
#include "Shaders/ShaderInterop.h"

static constexpr uint32_t SHCoefficientCount = 9;
static constexpr uint32_t SHCoefficientBufferElementCount = 6 * SHCoefficientCount;

void IBLGenerator::Init() {
    const char *baseDir = "Source/TestD3D12/Shaders";
    const RHI::ShaderModel shaderModel = RHI::ShaderModel::SM_6_6;

    genDiffuseIrradianceShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_genDiffuseIrradianceShader", shaderModel, "PBR/GenDiffuseIrradiance.hlsl", baseDir);
    projectCubemapSHShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_projectCubemapSHShader", shaderModel, "PBR/ProjectCubemapSH.hlsl", baseDir);
    genDiffuseIrradianceSHShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_genDiffuseIrradianceSHShader", shaderModel, "PBR/GenDiffuseIrradianceSH.hlsl", baseDir);
    genLDSumPhongSpecularShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_genLDSumPhongSpecularShader", shaderModel, "PBR/GenLDSumPhongSpecular.hlsl", baseDir);
    genLDSumGGXShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_genLDSumGGXShader", shaderModel, "PBR/GenLDSumGGX.hlsl", baseDir);
    genDFGSumGGXShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_genDFGSumGGXShader", shaderModel, "PBR/GenDFGSumGGX.hlsl", baseDir);

    genDiffuseIrradiancePipeline = shaderPipelineCache.GetComputePipeline(genDiffuseIrradianceShaderTemplate, ShaderPermutationKey());
    projectCubemapSHPipeline = shaderPipelineCache.GetComputePipeline(projectCubemapSHShaderTemplate, ShaderPermutationKey());
    genDiffuseIrradianceSHPipeline = shaderPipelineCache.GetComputePipeline(genDiffuseIrradianceSHShaderTemplate, ShaderPermutationKey());
    genLDSumPhongSpecularPipeline = shaderPipelineCache.GetComputePipeline(genLDSumPhongSpecularShaderTemplate, ShaderPermutationKey());
    genLDSumGGXPipeline = shaderPipelineCache.GetComputePipeline(genLDSumGGXShaderTemplate, ShaderPermutationKey());
    genDFGSumGGXPipeline = shaderPipelineCache.GetComputePipeline(genDFGSumGGXShaderTemplate, ShaderPermutationKey());

    const uint32_t coefficientBufferSize = SHCoefficientBufferElementCount * sizeof(float) * 4;
    shCoefficientBuffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::ShaderResource | RHI::BufferFlag::UnorderedAccess, coefficientBufferSize, nullptr);
    if (shCoefficientBuffer) {
        RHI::renderer->SetName(shCoefficientBuffer, "IBLGenerator::SHCoefficientBuffer");
        shCoefficientSRV = RHI::renderer->CreateView(shCoefficientBuffer, RHI::ViewType::SRV, RHI::Format::Unknown, sizeof(float) * 4, 0, coefficientBufferSize);
        shCoefficientUAV = RHI::renderer->CreateView(shCoefficientBuffer, RHI::ViewType::UAV, RHI::Format::Unknown, sizeof(float) * 4, 0, coefficientBufferSize);

        if (!shCoefficientSRV.IsValid() || !shCoefficientUAV.IsValid()) {
            RHI::renderer->DestroyBuffer(shCoefficientBuffer);
            shCoefficientBuffer = nullptr;
            shCoefficientSRV = RHI::ViewID::InvalidID;
            shCoefficientUAV = RHI::ViewID::InvalidID;
        }
    }
}

void IBLGenerator::Shutdown() {
    if (shCoefficientBuffer) {
        RHI::renderer->DestroyBuffer(shCoefficientBuffer);
        shCoefficientBuffer = nullptr;
    }
    shCoefficientSRV = RHI::ViewID::InvalidID;
    shCoefficientUAV = RHI::ViewID::InvalidID;
    shCoefficientBufferState = RHI::GPUResourceState::Undefined;

    genDiffuseIrradiancePipeline.reset();
    projectCubemapSHPipeline.reset();
    genDiffuseIrradianceSHPipeline.reset();
    genLDSumPhongSpecularPipeline.reset();
    genLDSumGGXPipeline.reset();
    genDFGSumGGXPipeline.reset();

    genDiffuseIrradianceShaderTemplate.reset();
    projectCubemapSHShaderTemplate.reset();
    genDiffuseIrradianceSHShaderTemplate.reset();
    genLDSumPhongSpecularShaderTemplate.reset();
    genLDSumGGXShaderTemplate.reset();
    genDFGSumGGXShaderTemplate.reset();
}

static bool GetOutputMipViewInfo(RHI::Texture *texture, RHI::ViewID outputUAV, uint32_t mipLevel, uint32_t requiredSliceCount, RHI::TextureViewInfo &outViewInfo) {
    if (!texture || texture->GetStatus() != RHI::GPUResourceStatus::Ready || !outputUAV.IsValid()) {
        return false;
    }

    if (!texture->GetViewInfo(RHI::ViewType::UAV, outputUAV, outViewInfo)) {
        return false;
    }

    if (outViewInfo.viewType != RHI::ViewType::UAV ||
        outViewInfo.firstMipLevel != mipLevel || outViewInfo.mipCount != 1 ||
        outViewInfo.firstSlice != 0 || outViewInfo.sliceCount != requiredSliceCount ||
        outViewInfo.width == 0 || outViewInfo.height == 0) {
        return false;
    }
    return true;
}

bool IBLGenerator::GenerateDiffuseIrradiance(RHI::CommandList *commandList, const RHI::Texture *radianceCubeMap, RHI::Texture *outputCubeMap, RHI::ViewID outputUAV) {
    PROFILER_CPU_SCOPED_EVENT("IBLGenerator::GenerateDiffuseIrradiance");

    if (!radianceCubeMap || !outputCubeMap || radianceCubeMap == outputCubeMap ||
        radianceCubeMap->GetStatus() != RHI::GPUResourceStatus::Ready || outputCubeMap->GetStatus() != RHI::GPUResourceStatus::Ready ||
        radianceCubeMap->type != RHI::TextureType::TextureCube || outputCubeMap->type != RHI::TextureType::TextureCube ||
        radianceCubeMap->GetArraySize() != 6 || outputCubeMap->GetArraySize() != 6 ||
        radianceCubeMap->GetWidth() == 0 || radianceCubeMap->GetWidth() != radianceCubeMap->GetHeight() ||
        outputCubeMap->GetWidth() == 0 || outputCubeMap->GetWidth() != outputCubeMap->GetHeight() ||
        outputCubeMap->GetSampleCount() != 1) {
        return false;
    }

    RHI::TextureViewInfo outputViewInfo;
    if (!GetOutputMipViewInfo(outputCubeMap, outputUAV, 0, 6, outputViewInfo) ||
        outputViewInfo.width != outputCubeMap->GetWidth() || outputViewInfo.height != outputCubeMap->GetHeight()) {
        return false;
    }

    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(genDiffuseIrradiancePipeline);
    if (!pso) {
        return false;
    }

    // Mip UAV 가 참조하는 6개 face(array slice) 만 개별적으로 transition 한다.
    RHI::GPUBarrier startBarriers[1 + 6];
    startBarriers[0] = RHI::Renderer::MakeTextureBarrier(radianceCubeMap, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute);
    for (uint32_t faceIndex = 0; faceIndex < 6; faceIndex++) {
        startBarriers[1 + faceIndex] = RHI::Renderer::MakeTextureBarrier(outputCubeMap, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, faceIndex, 0);
    }
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetTexture(commandList, 0, false, radianceCubeMap);
    RHI::renderer->SetTexture(commandList, 0, true, outputCubeMap, outputUAV);

    const uint32_t threadGroupCountX = (outputViewInfo.width + IBLIntegrationBlockSizeX - 1) / IBLIntegrationBlockSizeX;
    const uint32_t threadGroupCountY = (outputViewInfo.height + IBLIntegrationBlockSizeY - 1) / IBLIntegrationBlockSizeY;
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 6);

    RHI::GPUBarrier endBarriers[1 + 6];
    endBarriers[0] = RHI::Renderer::MakeTextureBarrier(radianceCubeMap, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource);
    for (uint32_t faceIndex = 0; faceIndex < 6; faceIndex++) {
        endBarriers[1 + faceIndex] = RHI::Renderer::MakeTextureBarrier(outputCubeMap, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource, faceIndex, 0);
    }
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));
    return true;
}

bool IBLGenerator::GenerateDiffuseIrradianceSH(RHI::CommandList *commandList, const RHI::Texture *radianceCubeMap, RHI::Texture *outputCubeMap, RHI::ViewID outputUAV) {
    PROFILER_CPU_SCOPED_EVENT("IBLGenerator::GenerateDiffuseIrradianceSH");

    if (!radianceCubeMap || !outputCubeMap || radianceCubeMap == outputCubeMap ||
        radianceCubeMap->GetStatus() != RHI::GPUResourceStatus::Ready || outputCubeMap->GetStatus() != RHI::GPUResourceStatus::Ready ||
        radianceCubeMap->type != RHI::TextureType::TextureCube || outputCubeMap->type != RHI::TextureType::TextureCube ||
        radianceCubeMap->GetArraySize() != 6 || outputCubeMap->GetArraySize() != 6 ||
        radianceCubeMap->GetWidth() == 0 || radianceCubeMap->GetWidth() != radianceCubeMap->GetHeight() ||
        outputCubeMap->GetWidth() == 0 || outputCubeMap->GetWidth() != outputCubeMap->GetHeight() ||
        outputCubeMap->GetSampleCount() != 1 ||
        !shCoefficientBuffer || shCoefficientBuffer->GetStatus() != RHI::GPUResourceStatus::Ready ||
        !shCoefficientSRV.IsValid() || !shCoefficientUAV.IsValid()) {
        return false;
    }

    RHI::TextureViewInfo outputViewInfo;
    if (!GetOutputMipViewInfo(outputCubeMap, outputUAV, 0, 6, outputViewInfo) ||
        outputViewInfo.width != outputCubeMap->GetWidth() || outputViewInfo.height != outputCubeMap->GetHeight()) {
        return false;
    }

    RHI::PipelineState *projectPSO = ShaderPipeline::GetReadyPSO(projectCubemapSHPipeline);
    RHI::PipelineState *reconstructPSO = ShaderPipeline::GetReadyPSO(genDiffuseIrradianceSHPipeline);
    if (!projectPSO || !reconstructPSO) {
        return false;
    }

    RHI::GPUBarrier projectBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(radianceCubeMap, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(shCoefficientBuffer, shCoefficientBufferState, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, projectBarriers, COUNT_OF(projectBarriers));

    RHI::renderer->SetPSO(commandList, projectPSO);
    RHI::renderer->SetTexture(commandList, 0, false, radianceCubeMap);
    RHI::renderer->SetBuffer(commandList, 0, true, shCoefficientBuffer, shCoefficientUAV);
    RHI::renderer->Dispatch(commandList, 6, 1, 1);

    // Projection 결과를 다음 compute pass 에서 읽을 수 있도록 전이한다.
    RHI::GPUBarrier reconstructBarriers[1 + 6];
    reconstructBarriers[0] = RHI::Renderer::MakeBufferBarrier(shCoefficientBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute);
    for (uint32_t faceIndex = 0; faceIndex < 6; faceIndex++) {
        reconstructBarriers[1 + faceIndex] = RHI::Renderer::MakeTextureBarrier(outputCubeMap, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, faceIndex, 0);
    }
    RHI::renderer->Barrier(commandList, reconstructBarriers, COUNT_OF(reconstructBarriers));

    RHI::renderer->SetPSO(commandList, reconstructPSO);
    RHI::renderer->SetBuffer(commandList, 0, false, shCoefficientBuffer, shCoefficientSRV);
    RHI::renderer->SetTexture(commandList, 0, true, outputCubeMap, outputUAV);

    const uint32_t threadGroupCountX = (outputViewInfo.width + IBLIntegrationBlockSizeX - 1) / IBLIntegrationBlockSizeX;
    const uint32_t threadGroupCountY = (outputViewInfo.height + IBLIntegrationBlockSizeY - 1) / IBLIntegrationBlockSizeY;
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 6);

    RHI::GPUBarrier endBarriers[2 + 6];
    endBarriers[0] = RHI::Renderer::MakeTextureBarrier(radianceCubeMap, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource);
    endBarriers[1] = RHI::Renderer::MakeBufferBarrier(shCoefficientBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource);
    for (uint32_t faceIndex = 0; faceIndex < 6; faceIndex++) {
        endBarriers[2 + faceIndex] = RHI::Renderer::MakeTextureBarrier(outputCubeMap, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource, faceIndex, 0);
    }
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    shCoefficientBufferState = RHI::GPUResourceState::ShaderResource;
    return true;
}

bool IBLGenerator::GenerateLDSumPhongSpecular(RHI::CommandList *commandList, const RHI::Texture *radianceCubeMap, int32_t maxSpecularPower, RHI::Texture *outputCubeMap, const RHI::ViewID *outputMipUAVs) {
    if (!outputCubeMap || !outputMipUAVs || maxSpecularPower <= 0) {
        return false;
    }

    const uint32_t mipLevelCount = outputCubeMap->GetMipLevelCount();
    for (uint32_t mipLevel = 0; mipLevel < mipLevelCount; mipLevel++) {
        if (!GenerateLDSumPhongSpecularMip(commandList, radianceCubeMap, maxSpecularPower, outputCubeMap, outputMipUAVs[mipLevel], mipLevel)) {
            return false;
        }
    }
    return true;
}

bool IBLGenerator::GenerateLDSumPhongSpecularMip(RHI::CommandList *commandList, const RHI::Texture *radianceCubeMap, int32_t maxSpecularPower, RHI::Texture *outputCubeMap, RHI::ViewID outputMipUAV, uint32_t mipLevel) {
    PROFILER_CPU_SCOPED_EVENT("IBLGenerator::GenerateLDSumPhongSpecularMip");

    if (!radianceCubeMap || !outputCubeMap || radianceCubeMap == outputCubeMap || maxSpecularPower <= 0 ||
        radianceCubeMap->GetStatus() != RHI::GPUResourceStatus::Ready || outputCubeMap->GetStatus() != RHI::GPUResourceStatus::Ready ||
        radianceCubeMap->type != RHI::TextureType::TextureCube || outputCubeMap->type != RHI::TextureType::TextureCube ||
        radianceCubeMap->GetArraySize() != 6 || outputCubeMap->GetArraySize() != 6 ||
        radianceCubeMap->GetWidth() == 0 || radianceCubeMap->GetWidth() != radianceCubeMap->GetHeight() ||
        outputCubeMap->GetWidth() == 0 || outputCubeMap->GetWidth() != outputCubeMap->GetHeight() ||
        outputCubeMap->GetSampleCount() != 1 || mipLevel >= outputCubeMap->GetMipLevelCount()) {
        return false;
    }

    RHI::TextureViewInfo outputViewInfo;
    if (!GetOutputMipViewInfo(outputCubeMap, outputMipUAV, mipLevel, 6, outputViewInfo)) {
        return false;
    }

    const uint32_t expectedMipSize = BE1::Max(outputCubeMap->GetWidth() >> mipLevel, 1u);
    if (outputViewInfo.width != expectedMipSize || outputViewInfo.height != expectedMipSize) {
        return false;
    }

    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(genLDSumPhongSpecularPipeline);
    if (!pso) {
        return false;
    }

    const uint32_t mipLevelCount = outputCubeMap->GetMipLevelCount();
    const float maxPower = static_cast<float>(maxSpecularPower);

    // 레거시와 동일하게 매 mip 마다 일정 비율로 power 를 낮춘다.
    const float powerDropOnMip = BE1::Math::Pow(maxPower, -1.0f / static_cast<float>(mipLevelCount));

    GenLDSumPhongSpecularParams params = {};
    params.specularPower = maxPower * BE1::Math::Pow(powerDropOnMip, static_cast<float>(mipLevel));

    // Mip UAV 가 참조하는 6개 face(array slice) 만 개별적으로 transition 한다.
    RHI::GPUBarrier startBarriers[1 + 6];
    startBarriers[0] = RHI::Renderer::MakeTextureBarrier(radianceCubeMap, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute);
    for (uint32_t faceIndex = 0; faceIndex < 6; faceIndex++) {
        startBarriers[1 + faceIndex] = RHI::Renderer::MakeTextureBarrier(outputCubeMap, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, faceIndex, mipLevel);
    }
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetTexture(commandList, 0, false, radianceCubeMap);
    RHI::renderer->SetTexture(commandList, 0, true, outputCubeMap, outputMipUAV);
    RHI::renderer->SetConstants(commandList, 0, &params, sizeof(params), 0);

    const uint32_t threadGroupCountX = (outputViewInfo.width + IBLIntegrationBlockSizeX - 1) / IBLIntegrationBlockSizeX;
    const uint32_t threadGroupCountY = (outputViewInfo.height + IBLIntegrationBlockSizeY - 1) / IBLIntegrationBlockSizeY;
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 6);

    RHI::GPUBarrier endBarriers[1 + 6];
    endBarriers[0] = RHI::Renderer::MakeTextureBarrier(radianceCubeMap, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource);
    for (uint32_t faceIndex = 0; faceIndex < 6; faceIndex++) {
        endBarriers[1 + faceIndex] = RHI::Renderer::MakeTextureBarrier(outputCubeMap, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource, faceIndex, mipLevel);
    }
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));
    return true;
}

bool IBLGenerator::GenerateLDSumGGX(RHI::CommandList *commandList, const RHI::Texture *radianceCubeMap, RHI::Texture *outputCubeMap, const RHI::ViewID *outputMipUAVs) {
    if (!outputCubeMap || !outputMipUAVs) {
        return false;
    }

    const uint32_t mipLevelCount = outputCubeMap->GetMipLevelCount();
    for (uint32_t mipLevel = 0; mipLevel < mipLevelCount; mipLevel++) {
        if (!GenerateLDSumGGXMip(commandList, radianceCubeMap, outputCubeMap, outputMipUAVs[mipLevel], mipLevel)) {
            return false;
        }
    }
    return true;
}

bool IBLGenerator::GenerateLDSumGGXMip(RHI::CommandList *commandList, const RHI::Texture *radianceCubeMap, RHI::Texture *outputCubeMap, RHI::ViewID outputMipUAV, uint32_t mipLevel) {
    PROFILER_CPU_SCOPED_EVENT("IBLGenerator::GenerateLDSumGGXMip");

    if (!radianceCubeMap || !outputCubeMap || radianceCubeMap == outputCubeMap ||
        radianceCubeMap->GetStatus() != RHI::GPUResourceStatus::Ready || outputCubeMap->GetStatus() != RHI::GPUResourceStatus::Ready ||
        radianceCubeMap->type != RHI::TextureType::TextureCube || outputCubeMap->type != RHI::TextureType::TextureCube ||
        radianceCubeMap->GetArraySize() != 6 || outputCubeMap->GetArraySize() != 6 ||
        radianceCubeMap->GetWidth() == 0 || outputCubeMap->GetWidth() == 0 ||
        outputCubeMap->GetSampleCount() != 1 || mipLevel >= outputCubeMap->GetMipLevelCount()) {
        return false;
    }

    // IntegrateLD 가 sample solid angle 에 따라 source LOD 를 선택하므로 전체 mip chain 이 필요하다.
    const int32_t fullMipLevelCount = BE1::Image::MaxMipLevels(radianceCubeMap->GetWidth(), radianceCubeMap->GetHeight(), 1);
    if (radianceCubeMap->GetMipLevelCount() < fullMipLevelCount) {
        return false;
    }

    RHI::TextureViewInfo outputViewInfo;
    if (!GetOutputMipViewInfo(outputCubeMap, outputMipUAV, mipLevel, 6, outputViewInfo)) {
        return false;
    }

    const uint32_t expectedMipSize = BE1::Max(outputCubeMap->GetWidth() >> mipLevel, 1u);
    if (outputViewInfo.width != expectedMipSize || outputViewInfo.height != expectedMipSize) {
        return false;
    }

    return DispatchGenerateLDMip(commandList, radianceCubeMap, outputCubeMap, outputMipUAV, outputViewInfo, mipLevel);
}

bool IBLGenerator::GenerateDFGSumGGX(RHI::CommandList *commandList, RHI::Texture *outputTexture, RHI::ViewID outputUAV) {
    PROFILER_CPU_SCOPED_EVENT("IBLGenerator::GenerateDFGSumGGX");

    if (!outputTexture || outputTexture->type != RHI::TextureType::Texture2D || outputTexture->GetSampleCount() != 1) {
        return false;
    }

    RHI::TextureViewInfo outputViewInfo;
    if (!GetOutputMipViewInfo(outputTexture, outputUAV, 0, 1, outputViewInfo) || outputViewInfo.format != RHI::Format::R16G16_FLOAT) {
        return false;
    }

    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(genDFGSumGGXPipeline);
    if (!pso) {
        return false;
    }

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(outputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, 0, 0));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetTexture(commandList, 0, true, outputTexture, outputUAV);

    const uint32_t threadGroupCountX = (outputViewInfo.width + IBLIntegrationBlockSizeX - 1) / IBLIntegrationBlockSizeX;
    const uint32_t threadGroupCountY = (outputViewInfo.height + IBLIntegrationBlockSizeY - 1) / IBLIntegrationBlockSizeY;
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeTextureBarrier(outputTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource, 0, 0));
    return true;
}

bool IBLGenerator::GenerateDFGSumGGXToImage(RHI::FrameThreadData *frameThreadData, int32_t size, BE1::Image &outImage) {
    PROFILER_CPU_SCOPED_EVENT("IBLGenerator::GenerateDFGSumGGXToImage");

    outImage.Clear();

    if (size <= 0) {
        return false;
    }

    static constexpr BE1::Image::Format DFGImageFormat = BE1::Image::Format::R16G16_FLOAT;

    // CreateTexture 에 필요한 metadata 만 준비하고 CPU pixel memory 는 할당하지 않는다.
    SharedPtr<BE1::Image> textureImage = MakeShared<BE1::Image>();
    textureImage->InitFromMemory(size, size, 1, 1, 1, DFGImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);

    RHI::Texture *texture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D,
        RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess,
        textureImage, RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!texture) {
        return false;
    }
    RHI::renderer->SetName(texture, "IBLGenerator::DFGSumGGXTexture");

    // 이 함수가 생성한 command list 만 닫고 제출하므로 호출자의 recording scope 에 영향을 주지 않는다.
    RHI::CommandList *commandList = frameThreadData->BeginCommandList(RHI::CommandQueueType::Graphics);
    const bool generated = GenerateDFGSumGGX(commandList, texture);
    commandList->CloseAndExecute();
    if (!generated) {
        RHI::renderer->DestroyTexture(texture);
        return false;
    }

    BE1::Image image;
    image.Create2D(size, size, 1, DFGImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
    if (!RHI::renderer->GetTextureImage2D(texture, 0, DFGImageFormat, image.GetPixels())) {
        RHI::renderer->DestroyTexture(texture);
        return false;
    }

    // 성공한 GetTextureImage2D 는 graphics queue 완료를 기다리므로 immediate destroy 가 안전하다.
    RHI::renderer->DestroyTexture(texture, true);
    outImage = std::move(image);
    return true;
}

bool IBLGenerator::GenerateDFGSumGGXToFile(RHI::FrameThreadData *frameThreadData, int32_t size, const char *filename) {
    if (!filename || !filename[0]) {
        return false;
    }

    BE1::Image image;
    if (!GenerateDFGSumGGXToImage(frameThreadData, size, image)) {
        return false;
    }

    BE1::Str outputPath = filename;
    outputPath.SetFileExtension(".dds");
    if (!image.WriteDDS(outputPath.c_str())) {
        BE_WARNLOG("IBLGenerator::GenerateDFGSumGGXToFile: failed to write \"%s\"\n", outputPath.c_str());
        return false;
    }

    BE_LOG("Generated GGX integration LUT to \"%s\"\n", outputPath.c_str());
    return true;
}

bool IBLGenerator::DispatchGenerateLDMip(RHI::CommandList *commandList, const RHI::Texture *radianceCubeMap, RHI::Texture *outputCubeMap, RHI::ViewID outputMipUAV, const RHI::TextureViewInfo &outputViewInfo, uint32_t mipLevel) {
    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(genLDSumGGXPipeline);
    if (!pso) {
        return false;
    }

    const uint32_t mipLevelCount = outputCubeMap->GetMipLevelCount();
    GenLDSumGGXParams params = {};
    params.radianceCubeMapSize = radianceCubeMap->GetWidth();
    params.minRadianceMipLevel = BE1::Max(BE1::Math::Log(2.0f, static_cast<float>(radianceCubeMap->GetWidth()) / static_cast<float>(outputViewInfo.width)), 0.0f);

    // Perceptual roughness 를 mip chain 에 선형 배분하고 shader 에서 제곱하여 linear roughness 로 변환한다.
    params.roughness = mipLevelCount > 1 ? static_cast<float>(mipLevel) / static_cast<float>(mipLevelCount - 1) : 0.0f;

    // Mip UAV 가 참조하는 6개 face(array slice) 만 개별적으로 transition 한다.
    RHI::GPUBarrier startBarriers[1 + 6];
    startBarriers[0] = RHI::Renderer::MakeTextureBarrier(radianceCubeMap, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute);
    for (uint32_t faceIndex = 0; faceIndex < 6; faceIndex++) {
        startBarriers[1 + faceIndex] = RHI::Renderer::MakeTextureBarrier(outputCubeMap, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, faceIndex, mipLevel);
    }
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetTexture(commandList, 0, false, radianceCubeMap);
    RHI::renderer->SetTexture(commandList, 0, true, outputCubeMap, outputMipUAV);
    RHI::renderer->SetConstants(commandList, 0, &params, sizeof(params), 0);

    const uint32_t threadGroupCountX = (outputViewInfo.width + IBLIntegrationBlockSizeX - 1) / IBLIntegrationBlockSizeX;
    const uint32_t threadGroupCountY = (outputViewInfo.height + IBLIntegrationBlockSizeY - 1) / IBLIntegrationBlockSizeY;
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 6);

    RHI::GPUBarrier endBarriers[1 + 6];
    endBarriers[0] = RHI::Renderer::MakeTextureBarrier(radianceCubeMap, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource);
    for (uint32_t faceIndex = 0; faceIndex < 6; faceIndex++) {
        endBarriers[1 + faceIndex] = RHI::Renderer::MakeTextureBarrier(outputCubeMap, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource, faceIndex, mipLevel);
    }
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));
    return true;
}
