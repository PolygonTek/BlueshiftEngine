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
#include "ProbeBakeSystem.h"
#include "RenderWork/ProbeBakeRenderWork.h"
#include "RenderCapture.h"
#include "RenderContext.h"
#include "RenderSystem.h"
#include "Render/Frontend/RenderWorld.h"
#include "Render/Texture/Texture.h"
#include "Image/Image.h"

void ProbeBakeSystem::Init() {
    environmentLightRequests.Clear();
    activeEnvironmentLightRequest = {};

    nextBakeSerial = 0;
    activeBakeSerial = 0;
}

void ProbeBakeSystem::Shutdown() {
    environmentLightRequests.Clear();
    activeEnvironmentLightRequest = {};

    nextBakeSerial = 0;
    activeBakeSerial = 0;
}

void ProbeBakeSystem::ScheduleEnvironmentLightProbeBake(RenderWorld *renderWorld, const BE1::Vec3 &captureOrigin) {
    assert(BE1::Engine::IsInMainThread());

    // 이미 같은 renderWorld 에 요청했었다면 리턴
    for (EnvironmentLightBakeRequest &request : environmentLightRequests) {
        if (request.renderWorld == renderWorld) {
            request.captureOrigin = captureOrigin;
            return;
        }
    }

    EnvironmentLightBakeRequest request;
    request.renderWorld = renderWorld;
    request.captureOrigin = captureOrigin;
    environmentLightRequests.Append(request);
}

void ProbeBakeSystem::CancelEnvironmentLightProbeBake(RenderWorld *renderWorld) {
    assert(BE1::Engine::IsInMainThread());

    for (int32_t index = environmentLightRequests.Count() - 1; index >= 0; --index) {
        if (environmentLightRequests[index].renderWorld == renderWorld) {
            environmentLightRequests.RemoveIndex(index);
        }
    }

    if (activeBakeSerial != 0 && activeEnvironmentLightRequest.renderWorld == renderWorld) {
        activeEnvironmentLightRequest = {};
        activeBakeSerial = 0;
    }
}

static bool CreateRadianceCubemap(int32_t size, bool useHDR, SharedPtr<Texture> &outTexture) {
    const BE1::Image::Format format = useHDR ? BE1::Image::Format::R11G11B10_FLOAT : BE1::Image::Format::R8G8B8A8;

    // radiance cubemap 은 항상 linear 포맷을 사용한다.
    SharedPtr<BE1::Image> textureImage = MakeShared<BE1::Image>();
    textureImage->InitFromMemory(size, size, 1, 6, 1, format, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::CubeMap);

    const Texture::Flag textureFlags = Texture::Flag::RenderTarget | Texture::Flag::UnorderedAccess | Texture::Flag::AllocateEmptyMipmaps | Texture::Flag::HighQuality | Texture::Flag::NonPowerOfTwo;
    SharedPtr<Texture> texture = MakeShared<Texture>();
    if (!texture->Create(RHI::TextureType::TextureCube, textureImage, textureFlags)) {
        return false;
    }

    outTexture = std::move(texture);
    return true;
}

void ProbeBakeSystem::Prepare(RenderContext *renderContext) {
    assert(BE1::Engine::IsInMainThread());
    assert(!RenderContext::activeContext);

    if (activeBakeSerial != 0) {
        return;
    }

    EnvironmentLightBakeRequest request;
    int32_t requestIndex = -1;

    for (int32_t index = 0; index < environmentLightRequests.Count(); ) {
        const EnvironmentLightBakeRequest &candidate = environmentLightRequests[index];
        const EnvironmentLight *environmentLight = candidate.renderWorld ? candidate.renderWorld->GetEnvironmentLight() : nullptr;

        if (!environmentLight) {
            environmentLightRequests.RemoveIndex(index);
            continue;
        }

        const SharedPtr<Texture> &radianceCubeMap = environmentLight->GetDesc().radianceCubeMap;
        if (radianceCubeMap) {
            const RHI::Texture *rhiTexture = radianceCubeMap->GetRHITexture();
            const bool preparationPending = radianceCubeMap->GetState() == Texture::State::Loading || (rhiTexture && rhiTexture->GetStatus() == RHI::GPUResourceStatus::Pending);

            // 외부 radiance cubemap 가 아직 GPU 에서 준비가 되지 않았다면, 스킵하고 다음 요청을 처리한다.
            if (preparationPending) {
                ++index;
                continue;
            }
        }

        request = candidate;
        requestIndex = index;
        break;
    }

    if (requestIndex < 0) {
        return;
    }

    const EnvironmentLight *environmentLight = request.renderWorld->GetEnvironmentLight();
    SharedPtr<Texture> radianceCubeMap = environmentLight->GetDesc().radianceCubeMap;
    bool radiancePrepared = true;
    bool generateRadianceMipChain = false;

    // 준비된 radianceCubemap 이 없다면 새로 캡쳐한다.
    if (!radianceCubeMap) {
        static constexpr int32_t radianceCubemapSize = 256;

        radiancePrepared = CreateRadianceCubemap(radianceCubemapSize, true, radianceCubeMap);
        if (radiancePrepared) {
            radiancePrepared = RenderCapture::RenderEnvCubeToTexture(renderContext, request.renderWorld, 0, false, false, BE1::Color4::black, request.captureOrigin, BE1::CmToUnit(5.0f), BE1::MeterToUnit(100.0f), radianceCubeMap, true);
        }
        generateRadianceMipChain = radiancePrepared;
    }

    environmentLightRequests.RemoveIndex(requestIndex);

    if (!radiancePrepared) {
        ApplyEnvironmentLightBakeResult(request.renderWorld, false, nullptr, nullptr);
        BE_WARNLOG("ProbeBakeSystem::Prepare: environment light radiance capture failed\n");
        return;
    }

    // probe bake 는 render work 로 등록한다.
    ProbeBakeJob::Desc bakeJobDesc;
    bakeJobDesc.radianceCubeMap = radianceCubeMap;
    bakeJobDesc.outputFlags = ProbeBakeJob::OutputFlag::All;

    const uint64_t bakeSerial = ++nextBakeSerial;
    UniquePtr<RenderWork> renderWork = MakeUnique<ProbeBakeRenderWork>(bakeSerial, bakeJobDesc, generateRadianceMipChain);

    activeEnvironmentLightRequest = request;
    activeBakeSerial = bakeSerial;

    renderSystem->SubmitRenderWork(std::move(renderWork));
}

void ProbeBakeSystem::CompleteBake(uint64_t bakeSerial, bool succeeded, const SharedPtr<Texture> &diffuseCubeMap, const SharedPtr<Texture> &specularCubeMap) {
    assert(BE1::Engine::IsInMainThread());

    // 이미 새로운 bake 요청이 있었다면 bake 결과를 폐기한다.
    if (activeBakeSerial != bakeSerial) {
        return;
    }

    RenderWorld *renderWorld = activeEnvironmentLightRequest.renderWorld;
    activeEnvironmentLightRequest = {};
    activeBakeSerial = 0;

    // 더 최신 요청이 쌓여있으면 bake 결과를 폐기한다.
    for (const EnvironmentLightBakeRequest &request : environmentLightRequests) {
        if (request.renderWorld == renderWorld) {
            return;
        }
    }

    ApplyEnvironmentLightBakeResult(renderWorld, succeeded, diffuseCubeMap, specularCubeMap);
    if (!succeeded) {
        BE_WARNLOG("ProbeBakeSystem::CompleteBake: environment light cubemap regeneration failed\n");
    }
}

void ProbeBakeSystem::ApplyEnvironmentLightBakeResult(RenderWorld *renderWorld, bool succeeded, const SharedPtr<Texture> &diffuseCubeMap, const SharedPtr<Texture> &specularCubeMap) {
    if (!renderWorld || !renderWorld->hasEnvironmentLight) {
        return;
    }

    renderWorld->environmentLight.ApplyBakeResult(succeeded, diffuseCubeMap, specularCubeMap);
}
