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
#include "RenderBackend.h"
#include "Render/RenderFrame.h"
#include "Render/RenderInternal.h"
#include "Render/RenderSystem.h"
#include "Render/Texture/Texture.h"
#include "Shaders/ShaderInterop.h"

bool RenderBackend::DispatchSSRTrace(bool transparent) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchSSRTrace");

    RHI::Texture *renderDepthTexture = currentFrameCamera->depthTexture;
    const bool useMSAA = renderDepthTexture->GetSampleCount() > 1;

    // 화면의 깊이와 속도 텍스쳐를 가져온다.
    RHI::Texture *sceneDepthTexture;
    RHI::Texture *sceneVelocityTexture;
    if (useMSAA) {
        sceneDepthTexture = currentFrameCamera->resolvedDepthTexture;
        sceneVelocityTexture = currentFrameCamera->resolvedVelocityTexture;
    } else {
        sceneDepthTexture = renderDepthTexture;
        sceneVelocityTexture = currentFrameCamera->velocityTexture;
    }

    // 반사 표면 (receiver surface) 의 깊이, 법선, 러프니스 텍스처를 가져온다.
    RHI::Texture *receiverDepthTexture;
    RHI::Texture *receiverNormalRoughnessTexture;
    if (transparent) {
        receiverDepthTexture = currentFrameCamera->transparentSSRDepthTexture;
        receiverNormalRoughnessTexture = currentFrameCamera->transparentSSRNormalRoughnessTexture;
    } else {
        receiverDepthTexture = sceneDepthTexture;
        if (useMSAA) {
            receiverNormalRoughnessTexture = currentFrameCamera->resolvedNormalRoughnessTexture;
        } else {
            receiverNormalRoughnessTexture = currentFrameCamera->normalRoughnessTexture;
        }
    }

    RHI::Texture *ssrHiZTexture = currentFrameCamera->ssrHiZ.texture;
    RHI::Texture *ssrPrevDepthExtremaTexture = currentFrameCamera->ssrSceneColorHistory.depthExtremaTexture;
    RHI::Texture *ssrTraceTexture = currentFrameCamera->ssrTraceTexture;
    RHI::Texture *ssrReceiverSurfaceTexture = currentFrameCamera->ssrReceiverSurfaceTexture;

    const RenderFrameSettings &frameSettings = frame->GetRenderFrameSettings();
    const uint32_t divisor = 1u << frameSettings.ssrResolutionMode;
    assert(receiverDepthTexture->GetSampleCount() == 1 && sceneDepthTexture->GetSampleCount() == 1 && receiverNormalRoughnessTexture->GetSampleCount() == 1);
    assert(ssrTraceTexture->GetWidth() == BE1::DivCeil(receiverDepthTexture->GetWidth(), divisor) && ssrTraceTexture->GetHeight() == BE1::DivCeil(receiverDepthTexture->GetHeight(), divisor));

    RHI::PipelineState *tracePSO = frameSSRTracePSO;
    if (!tracePSO) {
        return false;
    }

    SSRTraceParams params = {};
    params.inputSize = uint2(receiverDepthTexture->GetWidth(), receiverDepthTexture->GetHeight());
    params.outputSize = uint2(ssrTraceTexture->GetWidth(), ssrTraceTexture->GetHeight());
    params.reverseZ = currentFrameCamera->projectionPolicy.reverseZ ? 1u : 0u;
    params.debugMode = BE1::Min(frameSettings.ssrDebugMode, SSRDebugModeIterations);

    // UV 공간의 jitter 변화량 (현재 - 이전). 이전 프레임으로 재투영할 때 뺀다.
    params.uvJitterDelta = float2((currentFrameCamera->ndcJitter - currentFrameCamera->prevNdcJitter) * BE1::Vec2(0.5f, -0.5f));
    params.ssrHiZMipCount = currentFrameCamera->ssrHiZ.mipCount;
    params.maxIterations = frameSettings.ssrTraceMaxIterations;
    params.thickness = frameSettings.ssrTraceThickness;
    params.rayRoughnessScale = frameSettings.ssrRayRoughnessScale;
    params.screenEdgeFade = frameSettings.ssrScreenEdgeFade;
    params.roughnessFadeStart = frameSettings.ssrRoughnessFadeStart;
    params.roughnessFadeEnd = frameSettings.ssrRoughnessFadeEnd;
    params.noiseIndex = GetFrameIndex() & 63u;

    // non-MSAA 투명 SSR 에서는 불투명 렌더링 후 DepthWrite 상태인 sceneDepthTexture 를 ShaderResource 로 전환한다.
    const bool shouldTransitionSceneDepth = transparent && !useMSAA;

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::SSRTrace);

    RHI::GPUBarrier startBarriers[3];
    uint32_t startBarrierCount = 0;
    if (shouldTransitionSceneDepth) {
        startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(sceneDepthTexture, RHI::GPUResourceState::DepthWrite, RHI::GPUResourceState::ShaderResource);
    }
    startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(ssrTraceTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess);
    startBarriers[startBarrierCount++] = RHI::Renderer::MakeTextureBarrier(ssrReceiverSurfaceTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess);
    RHI::renderer->Barrier(mainCommandList, startBarriers, startBarrierCount);

    RHI::renderer->SetPSO(mainCommandList, tracePSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, sceneDepthTexture);
    RHI::renderer->SetTexture(mainCommandList, 1, false, sceneVelocityTexture);
    RHI::renderer->SetTexture(mainCommandList, 2, false, receiverDepthTexture); // 반사 표면 깊이 텍스쳐. transparent 인 경우 receiverDepthTexture 는 sceneDepthTexture 와 다르다.
    RHI::renderer->SetTexture(mainCommandList, 3, false, receiverNormalRoughnessTexture); // 반사 표면 법선/러프니스 텍스쳐.
    RHI::renderer->SetTexture(mainCommandList, 4, false, ssrHiZTexture); // 깊이 max/min 밉맵 체인 (mip 0 는 POT 로 반올림된 반해상도)
    RHI::renderer->SetTexture(mainCommandList, 5, false, ssrPrevDepthExtremaTexture); // 이전 프레임의 반해상도 깊이 max/min 텍스쳐 (이전 프레임 ssrHiZTexture mip 0 의 유효 영역과 같다)
    RHI::renderer->SetTexture(mainCommandList, 0, true, ssrTraceTexture); // 반사색 복원에 사용할 history UV 와 trace 신뢰도/상태를 기록한다. B 는 이터레이션 디버그용이다.
    RHI::renderer->SetTexture(mainCommandList, 1, true, ssrReceiverSurfaceTexture); // 패킹한 화면상 ray 길이와 출발 표면의 블록 내부 픽셀 인덱스와 roughness 를 기록한다.
    RHI::renderer->SetConstants(mainCommandList, 0, &params, sizeof(params), 0);
    RHI::renderer->SetConstantBuffer(mainCommandList, 1, cameraConstantBuffer);
    RHI::renderer->SetConstantBuffer(mainCommandList, 2, frameConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(params.outputSize.x, SSRTraceBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(params.outputSize.y, SSRTraceBlockSizeY);
    RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[3];
    uint32_t endBarrierCount = 0;
    if (shouldTransitionSceneDepth) {
        endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(sceneDepthTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::DepthWrite);
    }
    endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(ssrTraceTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource);
    endBarriers[endBarrierCount++] = RHI::Renderer::MakeTextureBarrier(ssrReceiverSurfaceTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource);
    RHI::renderer->Barrier(mainCommandList, endBarriers, endBarrierCount);

    return true;
}

// trace 의 prevHitUV 로 이전 프레임 radiance 를 샘플링해 trace 해상도의 reflection 텍스처를 생성한다.
// cone 필터링에서는 trace 가 기록한 현재 프레임의 픽셀 단위 ray 길이와 roughness 로 radiance mip level 을 결정한다. (Stochastic SSR 로 전환하면 없어질 예정)
bool RenderBackend::DispatchSSRRadianceResolve() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchSSRRadianceResolve");

    const RenderFrameSettings &frameSettings = frame->GetRenderFrameSettings();
    const uint32_t debugMode = BE1::Min(frameSettings.ssrDebugMode, SSRDebugModeIterations);
    RHI::Texture *radianceTexture = currentFrameCamera->ssrSceneColorHistory.radianceTexture;
    const bool useRadiancePyramid = debugMode == SSRDebugModeOff || debugMode == SSRDebugModeUpsampledReflection;
    RHI::Texture *radiancePyramidTexture = currentFrameCamera->ssrSceneColorHistory.radiancePyramidTexture;
    RHI::Texture *inputRadiancePyramidTexture = useRadiancePyramid ? radiancePyramidTexture : radianceTexture;
    RHI::Texture *ssrTraceTexture = currentFrameCamera->ssrTraceTexture;
    RHI::Texture *ssrReceiverSurfaceTexture = currentFrameCamera->ssrReceiverSurfaceTexture;
    RHI::Texture *ssrResolveReflectionTexture = currentFrameCamera->ssrResolveReflectionTexture;

    if (!frameSSRRadianceResolvePSO) {
        return false;
    }

    assert(radianceTexture->GetSampleCount() == 1 && inputRadiancePyramidTexture->GetSampleCount() == 1);
    assert(!useRadiancePyramid || (radiancePyramidTexture->GetWidth() == BE1::Max((radianceTexture->GetWidth() + 1u) >> 1, 1u) && radiancePyramidTexture->GetHeight() == BE1::Max((radianceTexture->GetHeight() + 1u) >> 1, 1u)));

    SSRRadianceResolveParams params = {};
    params.outputSize = uint2(ssrResolveReflectionTexture->GetWidth(), ssrResolveReflectionTexture->GetHeight());
    params.debugMode = debugMode;
    params.coneRoughnessScale = BE1::Math::Sqrt(BE1::Max(1.0f - frameSettings.ssrRayRoughnessScale * frameSettings.ssrRayRoughnessScale, 0.0f));

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::SSRRadianceResolve);

    RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(ssrResolveReflectionTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess));

    RHI::renderer->SetPSO(mainCommandList, frameSSRRadianceResolvePSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, radianceTexture);
    RHI::renderer->SetTexture(mainCommandList, 1, false, inputRadiancePyramidTexture);
    RHI::renderer->SetTexture(mainCommandList, 2, false, ssrTraceTexture);
    RHI::renderer->SetTexture(mainCommandList, 3, false, ssrReceiverSurfaceTexture);
    RHI::renderer->SetTexture(mainCommandList, 0, true, ssrResolveReflectionTexture);
    RHI::renderer->SetConstants(mainCommandList, 0, &params, sizeof(params), 0);

    const uint32_t threadGroupCountX = BE1::DivCeil(params.outputSize.x, SSRRadianceResolveBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(params.outputSize.y, SSRRadianceResolveBlockSizeY);
    RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(ssrResolveReflectionTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource));

    return true;
}

// 깊이 검증을 통과한 이전 reflection 을 현재 reflection 과 누적해 시간적 노이즈를 줄인다.
bool RenderBackend::DispatchSSRTemporal(bool transparent) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchSSRTemporal");

    const bool fullResolution = frame->GetRenderFrameSettings().ssrResolutionMode == 0;

    RHI::Texture *renderDepthTexture = currentFrameCamera->depthTexture;
    const bool useMSAA = renderDepthTexture->GetSampleCount() > 1;

    RHI::Texture *currReflectionTexture = currentFrameCamera->ssrResolveReflectionTexture;
    RHI::Texture *prevReflectionTexture = transparent ? currentFrameCamera->transparentSSRPreviousTemporalReflectionTexture : currentFrameCamera->ssrPreviousTemporalReflectionTexture;
    RHI::Texture *outputReflectionTexture = transparent ? currentFrameCamera->transparentSSRCurrentTemporalReflectionTexture : currentFrameCamera->ssrCurrentTemporalReflectionTexture;
    RHI::Texture *ssrTraceTexture = currentFrameCamera->ssrTraceTexture;
    RHI::Texture *ssrReceiverSurfaceTexture = currentFrameCamera->ssrReceiverSurfaceTexture;

    // Full-resolution/transparent SSR uses device depth; reduced-resolution opaque SSR uses half-resolution max/min depth.
    RHI::Texture *prevDepthTexture = transparent ? currentFrameCamera->transparentSSRPreviousDepthTexture : (fullResolution ? currentFrameCamera->ssrSceneColorHistory.fullResolutionDepthTexture : currentFrameCamera->ssrSceneColorHistory.depthExtremaTexture);
    RHI::Texture *currDepthTexture = transparent ? currentFrameCamera->transparentSSRDepthTexture : (useMSAA ? currentFrameCamera->resolvedDepthTexture : renderDepthTexture);
    RHI::Texture *velocityTexture = transparent ? currentFrameCamera->transparentSSRVelocityTexture : (useMSAA ? currentFrameCamera->resolvedVelocityTexture : currentFrameCamera->velocityTexture);

    if (!frameSSRTemporalPSO) {
        return false;
    }

    assert(currDepthTexture->GetSampleCount() == 1);
    const uint32_t prevDepthDivisor = transparent || fullResolution ? 1u : 2u;
    assert(prevDepthTexture->GetWidth() == BE1::DivCeil(currDepthTexture->GetWidth(), prevDepthDivisor) && prevDepthTexture->GetHeight() == BE1::DivCeil(currDepthTexture->GetHeight(), prevDepthDivisor));

    SSRTemporalParams params = {};
    params.inputSize = uint2(currentFrameCamera->renderWidth, currentFrameCamera->renderHeight);
    params.outputSize = uint2(outputReflectionTexture->GetWidth(), outputReflectionTexture->GetHeight());

    // UV 공간의 jitter 변화량 (currentJitter - previousJitter). 이전 프레임으로 재투영할 때 뺀다.
    params.uvJitterDelta = float2((currentFrameCamera->ndcJitter - currentFrameCamera->prevNdcJitter) * BE1::Vec2(0.5f, -0.5f));
    params.historyWeight = frame->GetRenderFrameSettings().ssrTemporalHistoryWeight;

    // 직전 프레임에서 SSR 기록이 있는지 확인한다. (SSR 기록이 없으면 historyValid = 0 으로 설정)
    const SSRHistoryRenderState &historyState = *currentFrameCamera->ssrHistoryRenderState;
    const uint64_t producedSerial = transparent ? historyState.transparentTemporalReflectionSerial : historyState.opaqueTemporalReflectionSerial;
    const bool previousViewProducedHistory = currentFrameCamera->ssrHistorySerial > 1 && producedSerial == currentFrameCamera->ssrHistorySerial - 1;
    params.historyValid = previousViewProducedHistory && (transparent ? currentFrameCamera->transparentSSRTemporalHistoryValid : currentFrameCamera->ssrTemporalHistoryValid) ? 1u : 0u;

    params.transparentReceiver = transparent ? 1u : 0u;

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::SSRTemporal);

    RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(outputReflectionTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess));

    RHI::renderer->SetPSO(mainCommandList, frameSSRTemporalPSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, currReflectionTexture);
    RHI::renderer->SetTexture(mainCommandList, 1, false, prevReflectionTexture);
    RHI::renderer->SetTexture(mainCommandList, 2, false, ssrTraceTexture);
    RHI::renderer->SetTexture(mainCommandList, 3, false, ssrReceiverSurfaceTexture);
    RHI::renderer->SetTexture(mainCommandList, 4, false, prevDepthTexture);
    RHI::renderer->SetTexture(mainCommandList, 5, false, currDepthTexture);
    RHI::renderer->SetTexture(mainCommandList, 6, false, velocityTexture);
    RHI::renderer->SetTexture(mainCommandList, 0, true, outputReflectionTexture);
    RHI::renderer->SetConstants(mainCommandList, 0, &params, sizeof(params), 0);
    RHI::renderer->SetConstantBuffer(mainCommandList, 1, cameraConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(params.outputSize.x, SSRTemporalBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(params.outputSize.y, SSRTemporalBlockSizeY);
    RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(outputReflectionTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource));

    return true;
}

bool RenderBackend::DispatchSSRUpsample(bool transparent) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DispatchSSRUpsample");

    RHI::Texture *renderDepthTexture = currentFrameCamera->depthTexture;
    const bool useMSAA = renderDepthTexture->GetSampleCount() > 1;
    const bool accumulate = transparent ? currentFrameCamera->accumulateTransparentSSR : currentFrameCamera->accumulateSSR;

    RHI::Texture *depthTexture = transparent ? currentFrameCamera->transparentSSRDepthTexture : (useMSAA ? currentFrameCamera->resolvedDepthTexture : renderDepthTexture);
    RHI::Texture *normalRoughnessTexture = transparent ? currentFrameCamera->transparentSSRNormalRoughnessTexture : (useMSAA ? currentFrameCamera->resolvedNormalRoughnessTexture : currentFrameCamera->normalRoughnessTexture);
    RHI::Texture *inputReflectionTexture = accumulate ? (transparent ? currentFrameCamera->transparentSSRCurrentTemporalReflectionTexture : currentFrameCamera->ssrCurrentTemporalReflectionTexture) : currentFrameCamera->ssrResolveReflectionTexture;
    RHI::Texture *receiverSurfaceTexture = currentFrameCamera->ssrReceiverSurfaceTexture;
    RHI::Texture *outputReflectionTexture = transparent ? currentFrameCamera->transparentSSRUpsampledReflectionTexture : currentFrameCamera->ssrUpsampledReflectionTexture;

    if (!frameSSRUpsamplePSO) {
        return false;
    }

    const uint32_t divisor = 1u << frame->GetRenderFrameSettings().ssrResolutionMode;
    assert(depthTexture->GetSampleCount() == 1 && normalRoughnessTexture->GetSampleCount() == 1);
    assert(inputReflectionTexture->GetWidth() == BE1::DivCeil(outputReflectionTexture->GetWidth(), divisor) && inputReflectionTexture->GetHeight() == BE1::DivCeil(outputReflectionTexture->GetHeight(), divisor));

    SSRUpsampleParams params = {};
    params.outputSize = uint2(outputReflectionTexture->GetWidth(), outputReflectionTexture->GetHeight());
    params.debugMode = BE1::Min(frame->GetRenderFrameSettings().ssrDebugMode, SSRDebugModeIterations);

    GpuTiming::ScopedTimer gpuScope(gpuTiming, mainCommandList, GpuTiming::Tag::SSRUpsample);

    RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(outputReflectionTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess));

    RHI::renderer->SetPSO(mainCommandList, frameSSRUpsamplePSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, depthTexture);
    RHI::renderer->SetTexture(mainCommandList, 1, false, normalRoughnessTexture);
    RHI::renderer->SetTexture(mainCommandList, 2, false, inputReflectionTexture);
    RHI::renderer->SetTexture(mainCommandList, 3, false, receiverSurfaceTexture);
    RHI::renderer->SetTexture(mainCommandList, 0, true, outputReflectionTexture);
    RHI::renderer->SetConstants(mainCommandList, 0, &params, sizeof(params), 0);
    RHI::renderer->SetConstantBuffer(mainCommandList, 1, cameraConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(params.outputSize.x, SSRUpsampleBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(params.outputSize.y, SSRUpsampleBlockSizeY);
    RHI::renderer->Dispatch(mainCommandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::renderer->Barrier(mainCommandList, RHI::Renderer::MakeTextureBarrier(outputReflectionTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource));

    return true;
}
