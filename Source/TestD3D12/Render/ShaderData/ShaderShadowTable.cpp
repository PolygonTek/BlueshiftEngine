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
#include "Profiler.h"
#include "ShaderShadowTable.h"
#include "Render/RenderFrame.h"
#include "Render/RenderInternal.h"

static void BuildShaderLocalShadow(const FrameLocalShadow &shadow, const FrameLocalShadowView &firstView, const RenderFrameSettings &settings, ShaderLocalShadow &outShadow) {
    assert(shadow.resolution > 2 * firstView.borderTexels);
    assert(shadow.numShadowViews == 1 || shadow.numShadowViews == 6);

    float filterRadius = settings.shadowMapQuality == 0 ? 0.0f : 1.0f;
    if (settings.shadowMapQuality == 3) {
        constexpr float referenceUsableResolution = static_cast<float>(RenderFrameSettings::ShadowMapReferenceResolution - 2 * RenderFrameSettings::ShadowMapReferenceBorderTexels);
        const float usableResolution = static_cast<float>(shadow.resolution - 2 * firstView.borderTexels);
        const float filterResolutionScale = usableResolution / referenceUsableResolution;
        filterRadius = settings.shadowMapFilterRadius * filterResolutionScale;
    }

    // 품질 전환 후 아직 depth 를 갱신하지 않은 타일도 저장된 border 안에서 샘플링한다.
    const float maxFilterRadius = BE1::Max(0.0f, static_cast<float>(firstView.borderTexels) - 1.0f);

    outShadow.firstShadowView = shadow.firstShadowView;
    outShadow.numShadowViews = shadow.numShadowViews;
    outShadow.filterRadius = BE1::Clamp(filterRadius, 0.0f, maxFilterRadius);
    outShadow.normalizedBorder = static_cast<float>(firstView.borderTexels) / shadow.resolution;
    outShadow.pointOrigin = float3(shadow.pointOrigin);
    outShadow.pointDepthScaleBias = float2(shadow.pointDepthScaleBias);
}

static void BuildShaderLocalShadowView(const FrameLocalShadowView &view, bool point, float invAtlasSize, ShaderLocalShadowView &outView) {
    if (point) {
        // Point 샘플링은 행렬을 사용하지 않는다. 스킵 프레임에는 viewProjMatrix 도 계산하지 않는다.
        outView.worldToShadowCoordMatrix = {};
    } else {
        // XY 만 clip [-W, W] 에서 타일 UV [0, W] 로 변환한다. Z/W 와 perspective divide 용 W 는 보존한다.
        const BE1::Mat4 &matrix = view.viewProjMatrix;

        // 업로드 버퍼의 row-major 메모리 배치에 맞춰 각 행을 연속으로 기록한다.
        for (uint32_t column = 0; column < 4; ++column) {
            outView.worldToShadowCoordMatrix.m[0][column] = (matrix[3][column] + matrix[0][column]) * 0.5f;
        }
        for (uint32_t column = 0; column < 4; ++column) {
            outView.worldToShadowCoordMatrix.m[1][column] = (matrix[3][column] - matrix[1][column]) * 0.5f;
        }
        for (uint32_t column = 0; column < 4; ++column) {
            outView.worldToShadowCoordMatrix.m[2][column] = matrix[2][column];
        }
        for (uint32_t column = 0; column < 4; ++column) {
            outView.worldToShadowCoordMatrix.m[3][column] = matrix[3][column];
        }
    }

    const BE1::Rect &tile = view.tileRect;
    outView.atlasScaleBias = float4(tile.w * invAtlasSize, tile.h * invAtlasSize, tile.x * invAtlasSize, tile.y * invAtlasSize);
}

void ShaderShadowTable::Shutdown() {
    DestroyBuffer(shadowBuffer);
    DestroyBuffer(shadowViewBuffer);
}

bool ShaderShadowTable::Upload(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, const RenderFrame *frame) {
    PROFILER_CPU_SCOPED_EVENT("ShaderShadowTable::Upload");

    const uint32_t shadowCount = frame->NumFrameLocalShadows();
    const uint32_t shadowViewCount = frame->NumFrameLocalShadowViews();
    if (shadowCount == 0) {
        return true;
    }

    if (!EnsureBufferCapacity(shadowBuffer, shadowCount, sizeof(ShaderLocalShadow), "shaderLocalShadowTableBuffer") ||
        !EnsureBufferCapacity(shadowViewBuffer, shadowViewCount, sizeof(ShaderLocalShadowView), "shaderLocalShadowViewTableBuffer")) {
        return false;
    }

    RHI::Buffer *shadowUploadBuffer = frameThreadData->AllocStructuredBuffer(sizeof(ShaderLocalShadow), shadowCount, nullptr);
    if (!shadowUploadBuffer || !shadowUploadBuffer->mappedPtr) {
        return false;
    }

    RHI::Buffer *viewUploadBuffer = frameThreadData->AllocStructuredBuffer(sizeof(ShaderLocalShadowView), shadowViewCount, nullptr);
    if (!viewUploadBuffer || !viewUploadBuffer->mappedPtr) {
        return false;
    }

    ShaderLocalShadow *shadows = reinterpret_cast<ShaderLocalShadow *>(shadowUploadBuffer->mappedPtr);
    ShaderLocalShadowView *views = reinterpret_cast<ShaderLocalShadowView *>(viewUploadBuffer->mappedPtr);
    const RenderFrameSettings &settings = frame->GetRenderFrameSettings();
    const FrameCamera *frameCameras = frame->GetFrameCameras();
    for (uint32_t cameraIndex = 0; cameraIndex < frame->NumFrameCameras(); ++cameraIndex) {
        const FrameShadowAtlas *shadowAtlas = frameCameras[cameraIndex].frameShadowAtlas;
        if (!shadowAtlas || shadowAtlas->firstFrameLocalShadow == UINT32_MAX || shadowAtlas->numShadows == 0) {
            continue;
        }

        if (shadowAtlas->firstFrameLocalShadow > shadowCount || shadowAtlas->numShadows > shadowCount - shadowAtlas->firstFrameLocalShadow ||
            shadowAtlas->firstFrameLocalShadowView > shadowViewCount || shadowAtlas->numShadowViews > shadowViewCount - shadowAtlas->firstFrameLocalShadowView) {
            assert(0);
            return false;
        }

        if (!shadowAtlas->shadowTexture || !shadowAtlas->shadowSRV.IsValid() || shadowAtlas->size == 0 ||
            !shadowAtlas->shadows || !shadowAtlas->shadowViews || shadowAtlas->numShadowViews == 0) {
            return false;
        }

        // 프런트엔드에서 확정한 두 테이블의 카메라별 연속 구간에 기록한다.
        ShaderLocalShadow *cameraShadows = shadows + shadowAtlas->firstFrameLocalShadow;
        for (uint32_t shadowIndex = 0; shadowIndex < shadowAtlas->numShadows; ++shadowIndex) {
            const FrameLocalShadow &shadow = shadowAtlas->shadows[shadowIndex];
            assert(shadow.firstShadowView < shadowAtlas->numShadowViews);
            assert(shadow.numShadowViews <= shadowAtlas->numShadowViews - shadow.firstShadowView);
            BuildShaderLocalShadow(shadow, shadowAtlas->shadowViews[shadow.firstShadowView], settings, cameraShadows[shadowIndex]);
        }

        const float invAtlasSize = 1.0f / shadowAtlas->size;
        ShaderLocalShadowView *cameraViews = views + shadowAtlas->firstFrameLocalShadowView;
        for (uint32_t viewIndex = 0; viewIndex < shadowAtlas->numShadowViews; ++viewIndex) {
            const FrameLocalShadowView &view = shadowAtlas->shadowViews[viewIndex];
            assert(view.shadowIndex < shadowAtlas->numShadows);
            const bool point = shadowAtlas->shadows[view.shadowIndex].numShadowViews == 6;
            BuildShaderLocalShadowView(view, point, invAtlasSize, cameraViews[viewIndex]);
        }
    }

    // 모든 카메라의 CPU 데이터를 준비한 뒤 테이블마다 한 번만 copy 한다. 같은 graphics queue 의 camera draw 보다 먼저 실행된다.
    // 실패/빈 카메라에서는 테이블에 접근하지 않도록, 샘플링 count 는 각 카메라의 depth 기록을 마친 뒤에만 활성화한다.
    UploadBuffer(commandList, shadowBuffer, shadowUploadBuffer, shadowCount * sizeof(ShaderLocalShadow));
    UploadBuffer(commandList, shadowViewBuffer, viewUploadBuffer, shadowViewCount * sizeof(ShaderLocalShadowView));
    return true;
}

bool ShaderShadowTable::EnsureBufferCapacity(TableBuffer &tableBuffer, uint32_t count, uint32_t elementSize, const char *name) {
    const uint32_t maxCapacity = UINT32_MAX / elementSize;
    if (count > maxCapacity) {
        return false;
    }

    if (count <= tableBuffer.capacity) {
        return true;
    }

    uint32_t newCapacity = tableBuffer.capacity > 0 ? tableBuffer.capacity : 1;
    while (newCapacity < count) {
        newCapacity = BE1::Min(newCapacity << 1, maxCapacity);
    }

    const uint32_t bufferSize = newCapacity * elementSize;
    RHI::Buffer *newBuffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::ShaderResource, bufferSize, nullptr);
    if (!newBuffer) {
        return false;
    }
    RHI::renderer->SetName(newBuffer, name);

    const RHI::ViewID newView = RHI::renderer->CreateView(newBuffer, RHI::ViewType::SRV, RHI::Format::Unknown, elementSize, 0, bufferSize);
    if (!newView.IsValid()) {
        RHI::renderer->DestroyBuffer(newBuffer);
        return false;
    }

    // 생성 실패 시 기존 버퍼를 보존한다. 성공하면 이전 버퍼는 RHI 의 지연 해제로 넘긴다.
    DestroyBuffer(tableBuffer);
    tableBuffer.buffer = newBuffer;
    tableBuffer.view = newView;
    tableBuffer.capacity = newCapacity;
    tableBuffer.bindlessIndex = newBuffer->GetBindlessIndex(RHI::ViewType::SRV, newView);
    return true;
}

void ShaderShadowTable::UploadBuffer(RHI::CommandList *commandList, TableBuffer &tableBuffer, RHI::Buffer *uploadBuffer, uint32_t uploadSize) {
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(tableBuffer.buffer, tableBuffer.state, RHI::GPUResourceState::CopyDst));
    RHI::renderer->CopyBuffer(commandList, tableBuffer.buffer, 0, uploadBuffer, 0, uploadSize);
    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(tableBuffer.buffer, RHI::GPUResourceState::CopyDst, RHI::GPUResourceState::ShaderResource));
    tableBuffer.state = RHI::GPUResourceState::ShaderResource;
}

void ShaderShadowTable::DestroyBuffer(TableBuffer &tableBuffer) {
    if (tableBuffer.buffer) {
        RHI::renderer->DestroyBuffer(tableBuffer.buffer);
    }
    tableBuffer = {};
}
