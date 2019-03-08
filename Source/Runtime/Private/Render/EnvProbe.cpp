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

EnvProbe::EnvProbe(int index) {
    this->index = index;
}

EnvProbe::~EnvProbe() {
    if (diffuseProbeTexture) {
        textureManager.ReleaseTexture(diffuseProbeTexture, true);
    }
    if (specularProbeTexture) {
        textureManager.ReleaseTexture(specularProbeTexture, true);
    }
    if (diffuseProbeRT) {
        RenderTarget::Delete(diffuseProbeRT);
    }
    if (specularProbeRT) {
        RenderTarget::Delete(specularProbeRT);
    }
}

void EnvProbe::Update(const EnvProbe::State *stateDef) {
    if (stateDef->type == EnvProbe::Type::Realtime && !needToRefresh) {
        bool resolutionMatch = state.resolution == stateDef->resolution;
        bool useHDRMatch = state.useHDR == stateDef->useHDR;
        bool clearMethodMatch = state.clearMethod == stateDef->clearMethod;
        bool clearColorMatch = state.clearColor == stateDef->clearColor;
        bool clippingNearMatch = state.clippingNear == stateDef->clippingNear;
        bool clippingFarMatch = state.clippingFar == stateDef->clippingFar;
        bool originMatch = state.origin == stateDef->origin;

        if (!resolutionMatch || !useHDRMatch || !clearMethodMatch || (stateDef->clearMethod == ClearMethod::ColorClear && !clearColorMatch) || 
            !clippingNearMatch || !clippingFarMatch || !originMatch) {
            needToRefresh = true;
        }
    }

    state = *stateDef;

    bounces = state.bounces;

    worldAABB = AABB(-state.boxExtent, state.boxExtent);
    worldAABB += state.origin + state.boxOffset;

    if (state.bakedDiffuseProbeTexture && state.bakedDiffuseProbeTexture != diffuseProbeTexture) {
        // Replace diffuse probe texture to new baked one.
        if (diffuseProbeTexture) {
            textureManager.ReleaseTexture(diffuseProbeTexture, true);
        }

        if (diffuseProbeRT) {
            RenderTarget::Delete(diffuseProbeRT);
            diffuseProbeRT = nullptr;
        }

        // Use baked diffuse probe cubemap.
        diffuseProbeTexture = state.bakedDiffuseProbeTexture;
        diffuseProbeTexture->AddRefCount();
    } else {
        if (!diffuseProbeTexture) {
            // Create default diffuse probe cubemap.
            diffuseProbeTexture = textureManager.AllocTexture(va("DiffuseProbe-%s", state.guid.ToString()));
            diffuseProbeTexture->CreateEmpty(RHI::TextureCubeMap, 32, 32, 1, 1, 1,
                state.useHDR ? Image::RGBA_16F_16F_16F_16F : Image::RGB_8_8_8,
                Texture::Clamp | Texture::NoMipmaps | Texture::HighQuality);

            resourceGuidMapper.Set(Guid::CreateGuid(), diffuseProbeTexture->GetHashName());
        }
    }

    if (state.bakedSpecularProbeTexture && state.bakedSpecularProbeTexture != specularProbeTexture) {
        // Replace specular probe texture to new baked one.
        if (specularProbeTexture) {
            textureManager.ReleaseTexture(specularProbeTexture, true);
        }

        if (specularProbeRT) {
            RenderTarget::Delete(specularProbeRT);
            specularProbeRT = nullptr;
        }

        // Use baked specular probe cubemap.
        specularProbeTexture = state.bakedSpecularProbeTexture;
        specularProbeTexture->AddRefCount();
        specularProbeTextureMaxMipLevel = Math::Log(2.0f, specularProbeTexture->GetWidth());
    } else {
        if (!specularProbeTexture) {
            // Create default specular probe cubemap.
            int size = ToActualResolution(state.resolution);
            int numMipLevels = Math::Log(2, size) + 1;

            specularProbeTexture = textureManager.AllocTexture(va("SpecularProbe-%s", state.guid.ToString()));
            specularProbeTexture->CreateEmpty(RHI::TextureCubeMap, size, size, 1, 1, numMipLevels,
                state.useHDR ? Image::RGBA_16F_16F_16F_16F : Image::RGB_8_8_8,
                Texture::Clamp | Texture::HighQuality);

            specularProbeTextureMaxMipLevel = Math::Log(2.0f, specularProbeTexture->GetWidth());

            resourceGuidMapper.Set(Guid::CreateGuid(), specularProbeTexture->GetHashName());
        }
    }
}

int EnvProbe::ToActualResolution(Resolution resolution) {
    // Resolution value same order with EnvProbe::Resolution.
    static const int size[] = {
        16, 32, 64, 128, 256, 512, 1024, 2048
    };
    return size[(int)resolution];
}

void EnvProbeJob::RevalidateDiffuseProbeRT(bool clearToBlack) {
    // fixed size (32) for irradiance cubemap
    int size = 32;

    // Recreate diffuse convolution texture if its format have changed.
    if ((envProbe->state.useHDR ^ Image::IsFloatFormat(envProbe->diffuseProbeTexture->GetFormat()))) {
        Image::Format format = envProbe->state.useHDR ? Image::RGBA_16F_16F_16F_16F : Image::RGB_8_8_8;

        envProbe->diffuseProbeTexture->CreateEmpty(RHI::TextureCubeMap, size, size, 1, 1, 1, format, 
            Texture::Clamp | Texture::NoMipmaps | Texture::HighQuality);
    }

    if (clearToBlack) {
        int dataSize = size * size * Image::BytesPerPixel(envProbe->diffuseProbeTexture->GetFormat());
        byte *data = (byte *)_alloca16(dataSize);
        memset(data, 0, dataSize);

        envProbe->diffuseProbeTexture->Bind();

        for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
            envProbe->diffuseProbeTexture->UpdateCubemap(faceIndex, 0, 0, 0, size, size, envProbe->diffuseProbeTexture->GetFormat(), data);
        }
    }

    if (envProbe->diffuseProbeRT) {
        // Diffuse probe texture is recreated so we must recreate render target too.
        if (envProbe->diffuseProbeRT != envProbe->diffuseProbeTexture->GetRenderTarget()) {
            RenderTarget::Delete(envProbe->diffuseProbeRT);
            envProbe->diffuseProbeRT = nullptr;
        }
    }

    // Create diffuse probe render target if it is not created yet.
    if (!envProbe->diffuseProbeRT) {
        envProbe->diffuseProbeRT = RenderTarget::Create(envProbe->diffuseProbeTexture, nullptr, 0);
    }
}

void EnvProbeJob::RevalidateSpecularProbeRT(bool clearToBlack) {
    int size = envProbe->GetSize();
    int numMipLevels = Math::Log(2, size) + 1;

    // Recreate diffuse convolution texture if its format or size have changed.
    if (size != envProbe->specularProbeTexture->GetWidth() || (envProbe->state.useHDR ^ Image::IsFloatFormat(envProbe->specularProbeTexture->GetFormat()))) {
        Image::Format format = envProbe->state.useHDR ? Image::RGBA_16F_16F_16F_16F : Image::RGB_8_8_8;

        envProbe->specularProbeTexture->CreateEmpty(RHI::TextureCubeMap, size, size, 1, 1, numMipLevels, format,
            Texture::Clamp | Texture::HighQuality);
    }

    if (clearToBlack) {
        int dataSize = size * size * Image::BytesPerPixel(envProbe->specularProbeTexture->GetFormat());
        byte *data = (byte *)_alloca16(dataSize);
        memset(data, 0, dataSize);

        envProbe->specularProbeTexture->Bind();

        for (int mipLevel = 0; size > 0; mipLevel++, size >>= 1) {
            for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
                envProbe->specularProbeTexture->UpdateCubemap(faceIndex, mipLevel, 0, 0, size, size, envProbe->specularProbeTexture->GetFormat(), data);
            }
        }
    }

    if (envProbe->specularProbeRT) {
        // Specular probe texture is recreated so we must recreate render target too.
        if (envProbe->specularProbeRT != envProbe->specularProbeTexture->GetRenderTarget()) {
            RenderTarget::Delete(envProbe->specularProbeRT);
            envProbe->specularProbeRT = nullptr;
        }
    }

    // Create specular probe render target if it is not created yet.
    if (!envProbe->specularProbeRT) {
        envProbe->specularProbeRT = RenderTarget::Create(envProbe->specularProbeTexture, nullptr, RHI::HasDepthBuffer);
    }
}

bool EnvProbeJob::IsFinished() const {
    if (!diffuseProbeCubemapComputed || specularProbeCubemapComputedLevel < specularProbeCubemapMaxLevel) {
        return false;
    }
    return true;
}

bool EnvProbeJob::Refresh(EnvProbe::TimeSlicing timeSlicing) {
    if (specularProbeCubemapComputedLevel == -1) {
        if (specularProbeCubemapComputedLevel0Face == -1) {
            RevalidateSpecularProbeRT(envProbe->bounces == 0);
        }

        // FIXME: use EnvProbeStatic instead of -1
        int staticMask = envProbe->state.type == EnvProbe::Type::Baked ? -1 : 0;

        while (specularProbeCubemapComputedLevel0Face < 5) {
            // We can skip complex calculation of specular convolution cubemap for mipLevel 0.
            // It is same as perfect specular mirror. so we just render environment cubmap.
            renderSystem.CaptureEnvCubeFaceRT(renderWorld,
                envProbe->state.clearMethod == EnvProbe::ClearMethod::ColorClear,
                envProbe->state.clearColor,
                envProbe->state.layerMask, staticMask, envProbe->state.origin,
                envProbe->state.clippingNear, envProbe->state.clippingFar,
                envProbe->specularProbeRT, specularProbeCubemapComputedLevel0Face + 1);

            specularProbeCubemapComputedLevel0Face++;

            if (timeSlicing == EnvProbe::IndividualFaces) {
                break;
            }
        }

        if (specularProbeCubemapComputedLevel0Face == 5) {
            specularProbeCubemapComputedLevel = 0;
        }

        if (timeSlicing != EnvProbe::NoTimeSlicing) {
            return false;
        }
    }

    while (specularProbeCubemapComputedLevel < specularProbeCubemapMaxLevel) {
        // Generate specular convolution cube map from mipLevel 1 to specularProbeCubemapMaxLevel using environment cubemap.
        renderSystem.GenerateGGXLDSumRTLevel(envProbe->specularProbeTexture, envProbe->specularProbeRT, specularProbeCubemapMaxLevel, specularProbeCubemapComputedLevel + 1);

        specularProbeCubemapComputedLevel++;

        if (timeSlicing != EnvProbe::NoTimeSlicing) {
            return false;
        }
    }

    if (!diffuseProbeCubemapComputed) {
        RevalidateDiffuseProbeRT(envProbe->bounces == 0);

        // Generate diffuse convolution cube map using environment cubemap.
        renderSystem.GenerateIrradianceEnvCubeRT(envProbe->specularProbeTexture, envProbe->diffuseProbeRT);

        diffuseProbeCubemapComputed = true;
    }

    envProbe->bounces++;
    envProbe->needToRefresh = false;
    return true;
}

BE_NAMESPACE_END
