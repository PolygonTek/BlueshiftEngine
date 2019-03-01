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
#include "Render/Render.h"
#include "RenderInternal.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

ReflectionProbe::ReflectionProbe(int index) {
    memset(&state, 0, sizeof(state));

    this->index = index;

    // Create default diffuse convolution cubemap 
    diffuseSumCubeTexture = textureManager.AllocTexture(va("diffuseSum(%i)", index));
    diffuseSumCubeTexture->CreateEmpty(RHI::TextureCubeMap, 16, 16, 1, 1, 1, Image::RGB_8_8_8,
        Texture::Clamp | Texture::NoMipmaps | Texture::HighQuality);

    resourceGuidMapper.Set(Guid::CreateGuid(), diffuseSumCubeTexture->GetHashName());

    // Create default specular convolution cubemap
    int size = 16;
    int numMipLevels = Math::Log(2, size) + 1;

    specularSumCubeTexture = textureManager.AllocTexture(va("specularSum(%i)", index));
    specularSumCubeTexture->CreateEmpty(RHI::TextureCubeMap, size, size, 1, 1, numMipLevels, Image::RGB_8_8_8,
        Texture::Clamp | Texture::HighQuality);

    resourceGuidMapper.Set(Guid::CreateGuid(), specularSumCubeTexture->GetHashName());
}

ReflectionProbe::~ReflectionProbe() {
    textureManager.ReleaseTexture(diffuseSumCubeTexture);
    textureManager.ReleaseTexture(specularSumCubeTexture);
}

void ReflectionProbe::Update(const ReflectionProbe::State *stateDef) {
    if (!needToRefresh) {
        bool resolutionMatch = state.resolution == stateDef->resolution;
        bool useHDRMatch = state.useHDR == stateDef->useHDR;
        bool clearMethodMatch = state.clearMethod == stateDef->clearMethod;
        bool clearColorMatch = state.clearColor == stateDef->clearColor;
        bool clippingNearMatch = state.clippingNear == stateDef->clippingNear;
        bool clippingFarMatch = state.clippingFar == stateDef->clippingFar;
        bool originMatch = state.origin == stateDef->origin;

        if (!resolutionMatch || !useHDRMatch || !clearMethodMatch || (stateDef->clearMethod == ClearMethod::ColorClear && !clearColorMatch) || !clippingNearMatch || !clippingFarMatch || !originMatch) {
            Invalidate();
        }
    }

    state = *stateDef;

    worldAABB = AABB(-state.boxSize, state.boxSize);
    worldAABB += state.origin + state.boxOffset;
}

void ReflectionProbe::Invalidate() {
    needToRefresh = true;
}

int ReflectionProbe::ToActualResolution(Resolution resolution) {
    // resolution value same order with ReflectionProbe::Resolution
    static const int size[] = {
        16, 32, 64, 128, 256, 1024, 2048
    };
    return size[(int)resolution];
}

void ReflectionProbeJob::RevalidateDiffuseConvolutionCubemap() {
    // Recreate diffuse convolution texture if its format have changed.
    if ((reflectionProbe->state.useHDR ^ Image::IsFloatFormat(reflectionProbe->diffuseSumCubeTexture->GetFormat()))) {
        Image::Format format = reflectionProbe->state.useHDR ? Image::RGB_16F_16F_16F : Image::RGB_8_8_8;

        reflectionProbe->diffuseSumCubeTexture->CreateEmpty(RHI::TextureCubeMap, 64, 64, 1, 1, 1, format, // fixed size (64) for irradiance cubemap
            Texture::Clamp | Texture::NoMipmaps | Texture::HighQuality);
    }

    // Create diffuse convolution render target if it is not created yet.
    if (!reflectionProbe->diffuseSumCubeRT) {
        reflectionProbe->diffuseSumCubeRT = RenderTarget::Create(reflectionProbe->diffuseSumCubeTexture, nullptr, 0);
    }
}

void ReflectionProbeJob::RevalidateSpecularConvolutionCubemap() {
    int size = reflectionProbe->GetSize();

    // Recreate diffuse convolution texture if its format or size have changed.
    if (size != reflectionProbe->specularSumCubeTexture->GetWidth() ||
        (reflectionProbe->state.useHDR ^ Image::IsFloatFormat(reflectionProbe->specularSumCubeTexture->GetFormat()))) {
        Image::Format format = reflectionProbe->state.useHDR ? Image::RGB_16F_16F_16F : Image::RGB_8_8_8;

        int numMipLevels = Math::Log(2, size) + 1;

        reflectionProbe->specularSumCubeTexture->CreateEmpty(RHI::TextureCubeMap, size, size, 1, 1, numMipLevels, format,
            Texture::Clamp | Texture::HighQuality);
    }

    // Create specular convolution render target if it is not created yet.
    if (!reflectionProbe->specularSumCubeRT) {
        reflectionProbe->specularSumCubeRT = RenderTarget::Create(reflectionProbe->specularSumCubeTexture, nullptr, RHI::HasDepthBuffer);
    }
}

bool ReflectionProbeJob::IsFinished() const {
    if (!diffuseConvolutionCubemapComputed || specularConvolutionCubemapComputedLevel < specularConvolutionCubemapMaxLevel) {
        return false;
    }
    return true;
}

void ReflectionProbeJob::Refresh() {
    if (specularConvolutionCubemapComputedLevel == -1) {
        if (specularConvolutionCubemapComputedLevel0Face == -1) {
            RevalidateSpecularConvolutionCubemap();
        }

        if (specularConvolutionCubemapComputedLevel0Face < 5) {
            // FIXME: use ReflectionProbeStatic instead of -1
            int staticMask = reflectionProbe->state.type == ReflectionProbe::Type::Baked ? -1 : 0;

            // We can skip complex calculation of specular convolution cubemap for mipLevel 0.
            // It is same as perfect specular mirror. so we just render environment cubmap.
            renderSystem.CaptureEnvCubeRTFace(renderWorld, reflectionProbe->state.layerMask, staticMask, reflectionProbe->state.origin,
                reflectionProbe->state.clippingNear, reflectionProbe->state.clippingFar, 
                reflectionProbe->specularSumCubeRT, specularConvolutionCubemapComputedLevel0Face + 1);

            specularConvolutionCubemapComputedLevel0Face++;            
            return;
        } else {
            specularConvolutionCubemapComputedLevel = 0;
        }
    }

    if (!diffuseConvolutionCubemapComputed) {
        RevalidateDiffuseConvolutionCubemap();

        // Generate diffuse convolution cube map using environment cubemap.
        renderSystem.GenerateIrradianceEnvCubeRT(reflectionProbe->specularSumCubeTexture, reflectionProbe->diffuseSumCubeRT);

        diffuseConvolutionCubemapComputed = true;
        return;
    }

    if (specularConvolutionCubemapComputedLevel < specularConvolutionCubemapMaxLevel) {
        // Generate specular convolution cube map from mipLevel 1 to specularConvolutionCubemapMaxLevel using environment cubemap.
        renderSystem.GenerateGGXLDSumRTLevel(reflectionProbe->specularSumCubeTexture, reflectionProbe->specularSumCubeRT, specularConvolutionCubemapMaxLevel, specularConvolutionCubemapComputedLevel + 1);

        specularConvolutionCubemapComputedLevel++;
    }

    if (specularConvolutionCubemapComputedLevel == specularConvolutionCubemapMaxLevel) {
        reflectionProbe->needToRefresh = false;
    }
}

BE_NAMESPACE_END
