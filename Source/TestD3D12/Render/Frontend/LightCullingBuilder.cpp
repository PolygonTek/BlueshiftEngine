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
#include "LightCullingBuilder.h"
#include "Render/RenderFrame.h"
#include "Render/RenderInternal.h"
#include "Shaders/ShaderInterop.h"
#include <algorithm>

static BE_INLINE uint16_t PackLightTileCoord(uint32_t x, uint32_t y) {
    assert(x <= FrameLight::TileCoordMask);
    assert(y <= FrameLight::TileCoordMask);
    return static_cast<uint16_t>(x | (y << FrameLight::TileCoordBitCount));
}

// Light-culling 타일 범위를 계산하고, 렌더 픽셀 기준 광원 화면 영역의 긴 변 길이를 반환한다.
uint32_t LightCullingBuilder::ComputeTileRange(const FrameCamera *frameCamera, const BE1::Rect &screenClipRect, uint16_t &packedTileMin, uint16_t &packedTileMax) {
    if (frameCamera->outputRect.w <= 0 || frameCamera->outputRect.h <= 0 || frameCamera->renderWidth == 0 || frameCamera->renderHeight == 0) {
        assert(0);
        packedTileMin = 0;
        packedTileMax = 0;
        return 0;
    }

    const int32_t outputMinX = frameCamera->outputRect.x;
    const int32_t outputMinY = frameCamera->outputRect.y;
    const int32_t outputMaxX = outputMinX + frameCamera->outputRect.w;
    const int32_t outputMaxY = outputMinY + frameCamera->outputRect.h;

    int32_t clipMinX = screenClipRect.x;
    int32_t clipMinY = screenClipRect.y;
    int32_t clipMaxX = static_cast<int32_t>(screenClipRect.x) + screenClipRect.w;
    int32_t clipMaxY = static_cast<int32_t>(screenClipRect.y) + screenClipRect.h;

    // 1 픽셀보다 작은 투영 영역은 정수 Rect 에서 크기가 0 이 될 수 있으므로 최소 한 픽셀을 보수적으로 포함한다.
    if (clipMinX >= clipMaxX && screenClipRect.w == 0) {
        clipMinX = BE1::Clamp<int32_t>(screenClipRect.x, outputMinX, outputMaxX - 1);
        clipMaxX = clipMinX + 1;
    }
    if (clipMinY >= clipMaxY && screenClipRect.h == 0) {
        clipMinY = BE1::Clamp<int32_t>(screenClipRect.y, outputMinY, outputMaxY - 1);
        clipMaxY = clipMinY + 1;
    }

    if (clipMinX >= clipMaxX || clipMinY >= clipMaxY) {
        assert(0);
        packedTileMin = 0;
        packedTileMax = 0;
        return 0;
    }

    const uint32_t outputWidth = static_cast<uint32_t>(frameCamera->outputRect.w);
    const uint32_t outputHeight = static_cast<uint32_t>(frameCamera->outputRect.h);
    const uint32_t renderWidth = frameCamera->renderWidth;
    const uint32_t renderHeight = frameCamera->renderHeight;

    // screenClipRect 는 outputRect 좌표계이므로, 동적 해상도를 반영한 렌더 타겟 좌표계로 변환한다.
    const uint32_t renderMinX = static_cast<uint32_t>(clipMinX - outputMinX) * renderWidth / outputWidth;
    const uint32_t renderMinY = static_cast<uint32_t>(clipMinY - outputMinY) * renderHeight / outputHeight;
    const uint32_t renderMaxX = (static_cast<uint32_t>(clipMaxX - outputMinX) * renderWidth + outputWidth - 1) / outputWidth;
    const uint32_t renderMaxY = (static_cast<uint32_t>(clipMaxY - outputMinY) * renderHeight + outputHeight - 1) / outputHeight;

    // 렌더 타겟 좌표계 기준으로 타일 좌표 (MinX, MinY, MaxX, MaxY) 를 구한다.
    const uint32_t tileMinX = static_cast<uint32_t>(renderMinX / ShaderLightTileSize);
    const uint32_t tileMinY = static_cast<uint32_t>(renderMinY / ShaderLightTileSize);
    const uint32_t tileMaxX = static_cast<uint32_t>((renderMaxX - 1) / ShaderLightTileSize);
    const uint32_t tileMaxY = static_cast<uint32_t>((renderMaxY - 1) / ShaderLightTileSize);

    packedTileMin = PackLightTileCoord(tileMinX, tileMinY);
    packedTileMax = PackLightTileCoord(tileMaxX, tileMaxY);

    // 타일 단위로 양자화하기 전의 크기를 shadow 요청 해상도 계산에서도 공유한다.
    return BE1::Max(renderMaxX - renderMinX, renderMaxY - renderMinY);
}

void LightCullingBuilder::Build(RenderFrame *frame, FrameCamera *frameCamera, float zNear, float zFar) {
    PROFILER_CPU_SCOPED_EVENT("LightCullingBuilder::Build");

    // 현재 카메라의 visible lights 를 minmax z 에 따라 정렬하고, z-bins 를 작성한다.
    BuildZBins(frame, frameCamera, zNear, zFar);

    if (frameCamera->lightCulling.zBins) {
        // 현재 카메라의 Z-bin 이 시작되는 프레임 공용 light Z-bin 테이블을 예약한다.
        frameCamera->lightCulling.zBinTableOffset = frame->ReserveLightCullingZBins();
    }

    if (frameCamera->numFrameLocalLights > 0) {
        assert(frameCamera->renderWidth > 0 && frameCamera->renderHeight > 0);

        // 렌더링 화면에 필요한 타일 개수
        frameCamera->lightCulling.tileCountX = (frameCamera->renderWidth + ShaderLightTileSize - 1) / ShaderLightTileSize;
        frameCamera->lightCulling.tileCountY = (frameCamera->renderHeight + ShaderLightTileSize - 1) / ShaderLightTileSize;

        // 각 타일이 필요한 비트 마스크 워드의 개수
        frameCamera->lightCulling.wordsPerTile = (frameCamera->numFrameLocalLights + ShaderLightWordBits - 1) / ShaderLightWordBits;

        // 전체 타일에 필요한 비트 마스크 워드의 개수
        const uint64_t numLightCullingWords = static_cast<uint64_t>(frameCamera->lightCulling.tileCountX) * frameCamera->lightCulling.tileCountY * frameCamera->lightCulling.wordsPerTile;

        if (numLightCullingWords <= UINT32_MAX) {
            // 현재 카메라의 타일 마스크 워드가 시작되는 프레임 공용 테이블을 예약한다.
            frameCamera->lightCulling.tileWordTableOffset = frame->ReserveLightCullingWords(static_cast<uint32_t>(numLightCullingWords));
        }
    }
}

static BE_INLINE uint32_t PackLightZBin(uint32_t minLightIndex, uint32_t maxLightIndex) {
    assert(minLightIndex <= ShaderLightZBinIndexMask);
    assert(maxLightIndex <= ShaderLightZBinIndexMask);
    return (maxLightIndex << 16) | minLightIndex;
}

void LightCullingBuilder::BuildZBins(RenderFrame *frame, FrameCamera *frameCamera, float zNear, float zFar) {
    PROFILER_CPU_SCOPED_EVENT("LightCullingBuilder::BuildZBins");

    const uint32_t numFrameLocalLights = frameCamera->numFrameLocalLights;
    if (numFrameLocalLights == 0) {
        return;
    }

    if (numFrameLocalLights > 1) {
        // viewZMin, viewZMax 순으로 오름차순 정렬하고, 두 값이 같으면 index 가 작은 라이트를 먼저 배치해 순서를 고정한다.
        std::sort(frameCamera->frameLocalLights, frameCamera->frameLocalLights + numFrameLocalLights, [](const FrameLight *lhs, const FrameLight *rhs) {
            if (lhs->viewZMin < rhs->viewZMin) {
                return true;
            }
            if (lhs->viewZMin > rhs->viewZMin) {
                return false;
            }
            if (lhs->viewZMax < rhs->viewZMax) {
                return true;
            }
            if (lhs->viewZMax > rhs->viewZMax) {
                return false;
            }
            return lhs->index < rhs->index;
        });
    }

    // Z-bin light index 는 16 비트로 패킹된다. 오버플로가 발생하면 라이트를 누락하지 않고 GPU 경로에서
    // 폴백할 수 있도록 정렬된 라이트 목록은 유지하고 lightCulling.zBins 는 nullptr 로 둔다.
    if (numFrameLocalLights > ShaderLightZBinMaxLightCount) {
        return;
    }

    if (zFar <= zNear) {
        return;
    }

    const float viewZRange = zFar - zNear;
    const float zBinSize = viewZRange / static_cast<float>(ShaderLightZBinCount);

    // linear view Z 를 Z-bin index 로 변환하기 위한 scale 과 bias 를 미리 계산한다.
    frameCamera->lightCulling.zBinScale = static_cast<float>(ShaderLightZBinCount) / viewZRange;
    frameCamera->lightCulling.zBinBias = -zNear * frameCamera->lightCulling.zBinScale;
    frameCamera->lightCulling.zBins = (uint32_t *)frame->MemAlloc(ShaderLightZBinCount * sizeof(frameCamera->lightCulling.zBins[0]));

    uint32_t startLightIndex = 0;
    uint32_t endLightIndex = 0;

    // 각 Z-bin 에 라이트 범위 [startLightIndex, endLightIndex) 를 패킹한다. (endLightIndex 의 라이트는 포함되지 않는다)
    for (uint32_t zBinIndex = 0; zBinIndex < ShaderLightZBinCount; ++zBinIndex) {
        const float currentZBinMin = zNear + static_cast<float>(zBinIndex) * zBinSize;
        const float currentZBinMax = zBinIndex + 1 == ShaderLightZBinCount ? zFar : currentZBinMin + zBinSize;

        while (startLightIndex < numFrameLocalLights && frameCamera->frameLocalLights[startLightIndex]->viewZMax < currentZBinMin) {
            ++startLightIndex;
        }
        while (endLightIndex < numFrameLocalLights && frameCamera->frameLocalLights[endLightIndex]->viewZMin <= currentZBinMax) {
            ++endLightIndex;
        }

        if (startLightIndex < endLightIndex) {
            frameCamera->lightCulling.zBins[zBinIndex] = PackLightZBin(startLightIndex, endLightIndex - 1);
        } else {
            frameCamera->lightCulling.zBins[zBinIndex] = ShaderLightZBinEmpty;
        }
    }
}
