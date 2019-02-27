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

BE_NAMESPACE_BEGIN

// resolution value same order with ReflectionProbe::Resolution
static const int resolutions[] = {
    16, 32, 64, 128, 256, 1024, 2048
};

ReflectionProbe::ReflectionProbe(int index) {
    memset(&state, 0, sizeof(state));

    this->index = index;

    diffuseSumCubeTexture = textureManager.AllocTexture(va("diffuseSum(%i)", index));
    specularSumCubeTexture = textureManager.AllocTexture(va("specularSum(%i)", index));
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

void ReflectionProbe::ForceToRefresh(RenderWorld *renderWorld) {
    int size = resolutions[state.resolution];
    int numMipLevels = Math::Log(2, size) + 1;
    Image::Format format = state.useHDR ? Image::RGB_16F_16F_16F : Image::RGB_8_8_8;

    if ((state.useHDR ^ Image::IsFloatFormat(diffuseSumCubeTexture->GetFormat()))) {
        diffuseSumCubeTexture->CreateEmpty(RHI::TextureCubeMap, 64, 64, 1, 1, 1, format, // fixed size (64) for irradiance cubemap
            Texture::Clamp | Texture::Nearest | Texture::NoMipmaps | Texture::HighQuality);
    }

    if (!diffuseSumCubeRT) {
        diffuseSumCubeRT = RenderTarget::Create(diffuseSumCubeTexture, nullptr, 0);
    }

    if (resolutions[state.resolution] != specularSumCubeTexture->GetWidth() ||
        (state.useHDR ^ Image::IsFloatFormat(specularSumCubeTexture->GetFormat()))) {

        specularSumCubeTexture->CreateEmpty(RHI::TextureCubeMap, size, size, 1, 1, numMipLevels, format,
            Texture::Clamp | Texture::Nearest | Texture::NoMipmaps | Texture::HighQuality);
    }

    if (!specularSumCubeRT) {
        specularSumCubeRT = RenderTarget::Create(specularSumCubeTexture, nullptr, 0);
    }

    RenderContext *renderContext = renderSystem.GetMainRenderContext();

    // Capture environment probe
    renderContext->CaptureEnvCubeRT(renderWorld, state.layerMask, state.origin, renderContext->envCubeRT);

    // Generate irradiance cube map
    renderContext->GenerateIrradianceEnvCubeRT(renderContext->envCubeTexture, diffuseSumCubeRT);

    // Generate prefiltered specular cube map
    renderContext->GenerateGGXLDSumRT(renderContext->envCubeTexture, specularSumCubeRT);

    needToRefresh = false;
}

BE_NAMESPACE_END
