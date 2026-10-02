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
#include "Render/DebugDrawVertex.h"
#include "Render/RenderInternal.h"
#include "Render/RenderFrame.h"

static constexpr uint32_t DebugLightCircleLowSegments = 12;
static constexpr uint32_t DebugLightCircleMediumSegments = 24;
static constexpr uint32_t DebugUnitCircleSegmentCount = 48;
static constexpr float DebugLightCircleLowMaxRadiusPixels = 24.0f;
static constexpr float DebugLightCircleMediumMaxRadiusPixels = 96.0f;
static constexpr uint32_t DebugSpotConeVertexCount = DebugLightCircleMediumSegments * 2 + 8;
static constexpr float MinDebugLightSize = 0.01f;
static constexpr float MinDebugLightRadiusPixels = 1.0f;

static BE1::Vec2 debugUnitCircle[DebugUnitCircleSegmentCount];

void RenderBackend::InitDebugDraw() {
    for (uint32_t segmentIndex = 0; segmentIndex < DebugUnitCircleSegmentCount; ++segmentIndex) {
        const float angle = BE1::Math::TwoPi * static_cast<float>(segmentIndex) / static_cast<float>(DebugUnitCircleSegmentCount);
        float s;
        float c;
        BE1::Math::SinCos(angle, s, c);
        debugUnitCircle[segmentIndex].Set(c, s);
    }
}

// 월드 공간 반지름을 화면 픽셀 반지름으로 변환할 때 중심점의 clip W 로 나누기 전의 스케일을 계산한다.
static float GetUnitRadiusToPixelsScale(const FrameCamera *frameCamera) {
    // 카메라의 월드 공간 right, up 단위 방향 벡터
    const BE1::Vec3 cameraRightDir(frameCamera->viewMatrix[0].x, frameCamera->viewMatrix[0].y, frameCamera->viewMatrix[0].z);
    const BE1::Vec3 cameraUpDir(frameCamera->viewMatrix[1].x, frameCamera->viewMatrix[1].y, frameCamera->viewMatrix[1].z);

    // 월드 공간에서 right, up 방향으로 1 만큼 이동할 때 클립 공간의 오프셋
    const BE1::Vec4 rightClipOffset = frameCamera->viewProjMatrix * BE1::Vec4(cameraRightDir, 0.0f);
    const BE1::Vec4 upClipOffset = frameCamera->viewProjMatrix * BE1::Vec4(cameraUpDir, 0.0f);

    const float radiusToPixelsX = BE1::Math::Abs(rightClipOffset.x) * static_cast<float>(frameCamera->renderWidth) * 0.5f;
    const float radiusToPixelsY = BE1::Math::Abs(upClipOffset.y) * static_cast<float>(frameCamera->renderHeight) * 0.5f;

    return BE1::Max(radiusToPixelsX, radiusToPixelsY);
}

static uint32_t GetDebugPointLightCircleSegments(const FrameCamera *frameCamera, const FrameLight *frameLight, float unitRadiusToPixelsScale) {
    const BE1::Vec4 centerClip = frameCamera->viewProjMatrix * BE1::Vec4(frameLight->origin, 1.0f);
    if (centerClip.w > std::numeric_limits<float>::min()) {
        const float radius = BE1::Max(frameLight->range, MinDebugLightSize);
        const float radiusPixels = radius * unitRadiusToPixelsScale / centerClip.w;

        if (radiusPixels < MinDebugLightRadiusPixels) {
            return 0;
        }
        if (radiusPixels <= DebugLightCircleLowMaxRadiusPixels) {
            return DebugLightCircleLowSegments;
        }
        if (radiusPixels <= DebugLightCircleMediumMaxRadiusPixels) {
            return DebugLightCircleMediumSegments;
        }
    }
    return DebugUnitCircleSegmentCount;
}

static BE_INLINE void WriteDebugDrawVertex(DebugDrawVertex *&vertexPtr, const BE1::Vec3 &position, uint32_t color) {
    vertexPtr->xyz = position;
    vertexPtr->color = color;
    vertexPtr++;
}

static void WriteDebugCircle(DebugDrawVertex *&vertexPtr, const BE1::Vec3 &origin, const BE1::Vec3 &axisX, const BE1::Vec3 &axisY, float radius, uint32_t numSegments, uint32_t color) {
    assert(numSegments > 0 && DebugUnitCircleSegmentCount % numSegments == 0);

    const BE1::Vec3 radiusVecX = axisX * radius;
    const BE1::Vec3 radiusVecY = axisY * radius;
    const uint32_t segmentStep = DebugUnitCircleSegmentCount / numSegments;
    uint32_t pointIndex = 0;

    for (uint32_t segmentIndex = 0; segmentIndex < numSegments; ++segmentIndex) {
        uint32_t nextPointIndex = pointIndex + segmentStep;
        if (nextPointIndex == DebugUnitCircleSegmentCount) {
            nextPointIndex = 0;
        }

        const BE1::Vec2 &point = debugUnitCircle[pointIndex];
        const BE1::Vec2 &nextPoint = debugUnitCircle[nextPointIndex];

        WriteDebugDrawVertex(vertexPtr, origin + radiusVecX * point.x + radiusVecY * point.y, color);
        WriteDebugDrawVertex(vertexPtr, origin + radiusVecX * nextPoint.x + radiusVecY * nextPoint.y, color);

        pointIndex = nextPointIndex;
    }
}

static void WriteDebugPointLight(DebugDrawVertex *&vertexPtr, const FrameLight *frameLight, uint32_t numSegments, uint32_t color) {
    const float radius = BE1::Max(frameLight->range, MinDebugLightSize);

    WriteDebugCircle(vertexPtr, frameLight->origin, BE1::Mat3::identity[1], BE1::Mat3::identity[2], radius, numSegments, color);
    WriteDebugCircle(vertexPtr, frameLight->origin, BE1::Mat3::identity[2], BE1::Mat3::identity[0], radius, numSegments, color);
    WriteDebugCircle(vertexPtr, frameLight->origin, BE1::Mat3::identity[0], BE1::Mat3::identity[1], radius, numSegments, color);
}

// 방사형 range 구면과 콘이 만나는 원, 원점에서 원으로 향하는 네 모서리를 LineList 로 기록한다.
static void WriteDebugSpotCone(DebugDrawVertex *&vertexPtr, const FrameLight *frameLight, float cosCone, uint32_t color) {
    const float coneLength = frameLight->range * cosCone;
    const float coneRadius = frameLight->range * BE1::Math::Sqrt(BE1::Max(0.0f, 1.0f - cosCone * cosCone));
    const BE1::Vec3 center = frameLight->origin + frameLight->axis[EngineAxis::Forward] * coneLength;
    const BE1::Vec3 &left = frameLight->axis[EngineAxis::Left];
    const BE1::Vec3 &up = frameLight->axis[EngineAxis::Up];
    WriteDebugCircle(vertexPtr, center, left, up, coneRadius, DebugLightCircleMediumSegments, color);

    for (uint32_t pointIndex = 0; pointIndex < DebugUnitCircleSegmentCount; pointIndex += DebugUnitCircleSegmentCount / 4) {
        const BE1::Vec2 &point = debugUnitCircle[pointIndex];
        WriteDebugDrawVertex(vertexPtr, frameLight->origin, color);
        WriteDebugDrawVertex(vertexPtr, center + (left * point.x + up * point.y) * coneRadius, color);
    }
}

static void WriteDebugSpotLight(DebugDrawVertex *&vertexPtr, const FrameLight *frameLight, uint32_t color) {
    WriteDebugSpotCone(vertexPtr, frameLight, frameLight->spotCosCone.y, color);
    if (frameLight->spotCosCone.x < 1.0f) {
        WriteDebugSpotCone(vertexPtr, frameLight, frameLight->spotCosCone.x, color);
    }
}

void RenderBackend::DrawDebugLights() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawDebugLights");

    RHI::PipelineState *pipelineState = currentFrameCamera->debugDrawPSOs[static_cast<int>(DebugDrawPipelineId::DepthTestedLines)];
    if (!currentFrameCamera->showDebugLights || currentFrameCamera->numFrameLocalLights == 0 || !pipelineState) {
        return;
    }

    bool hasPointLights = false;
    float unitRadiusToPixelsScale = 0.0f;
    uint64_t numVerts = 0;

    for (uint32_t lightIndex = 0; lightIndex < currentFrameCamera->numFrameLocalLights; ++lightIndex) {
        const FrameLight *frameLight = currentFrameCamera->frameLocalLights[lightIndex];
        if (!frameLight) {
            continue;
        }

        switch (frameLight->type) {
        case RenderLight::Type::Point: {
            // point light radius 의 화면 픽셀의 크기에 따라서 circle segment 의 개수를 조절한다
            if (!hasPointLights) {
                unitRadiusToPixelsScale = GetUnitRadiusToPixelsScale(currentFrameCamera);
                hasPointLights = true;
            }

            // 너무 작으면 0 을 리턴한다.
            const uint32_t numCircleSegments = GetDebugPointLightCircleSegments(currentFrameCamera, frameLight, unitRadiusToPixelsScale);
            numVerts += 3 * numCircleSegments * 2;
            break;
        }
        case RenderLight::Type::Spot:
            numVerts += DebugSpotConeVertexCount * (frameLight->spotCosCone.x < 1.0f ? 2u : 1u);
            break;
        default:
            assert(0);
            break;
        }
    }

    if (numVerts == 0) {
        return;
    }

    RHI::VertexBuffer *vertexBuffer = frame->GetThreadData(0)->AllocVertex(sizeof(DebugDrawVertex), numVerts);
    if (!vertexBuffer || !vertexBuffer->mappedPtr) {
        BE_WARNLOG("RenderBackend::DrawDebugLights: failed to allocate light bounds vertex buffer\n");
        return;
    }

    DebugDrawVertex *vertexPtr = reinterpret_cast<DebugDrawVertex *>(vertexBuffer->mappedPtr);
    DebugDrawVertex *const firstVertexPtr = vertexPtr;

    for (uint32_t lightIndex = 0; lightIndex < currentFrameCamera->numFrameLocalLights; ++lightIndex) {
        const FrameLight *frameLight = currentFrameCamera->frameLocalLights[lightIndex];
        if (!frameLight) {
            continue;
        }

        const uint32_t color = BE1::Color4(frameLight->intensityRGB, 1.0f).ToUInt32();
        switch (frameLight->type) {
        case RenderLight::Type::Point: {
            const uint32_t numCircleSegments = GetDebugPointLightCircleSegments(currentFrameCamera, frameLight, unitRadiusToPixelsScale);
            if (numCircleSegments > 0) {
                WriteDebugPointLight(vertexPtr, frameLight, numCircleSegments, color);
            }
            break;
        }
        case RenderLight::Type::Spot:
            WriteDebugSpotLight(vertexPtr, frameLight, color);
            break;
        default:
            assert(0);
            break;
        }
    }
    assert(static_cast<uint32_t>(vertexPtr - firstVertexPtr) == numVerts);

    RHI::renderer->SetVertexBuffer(mainCommandList, 0, vertexBuffer);
    RHI::renderer->SetPSO(mainCommandList, pipelineState);
    RHI::renderer->SetConstantBuffer(mainCommandList, 1, cameraConstantBuffer);
    RHI::renderer->Draw(mainCommandList, numVerts, 0);
}

void RenderBackend::DrawDebugPrimitives() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawDebugPrimitives");
    assert(cameraConstantBuffer);

    DrawDebugLights();

    const DebugDrawData *debugDrawData = currentFrameCamera->debugDrawData;
    if (!debugDrawData || !debugDrawData->vertexBuffer || debugDrawData->numBatches == 0) {
        return;
    }

    assert(debugDrawData->numBatches <= DebugDrawData::MaxBatches);

    RHI::renderer->SetVertexBuffer(mainCommandList, 0, debugDrawData->vertexBuffer);

    for (uint32_t batchIndex = 0; batchIndex < debugDrawData->numBatches; ++batchIndex) {
        const DebugDrawBatch &batch = debugDrawData->batches[batchIndex];
        RHI::PipelineState *pipelineState = currentFrameCamera->debugDrawPSOs[static_cast<int>(batch.pipelineId)];
        if (!pipelineState) {
            continue;
        }

        assert(batch.numVerts > 0);
        assert(batch.firstVertex + batch.numVerts <= debugDrawData->numVerts);

        RHI::renderer->SetPSO(mainCommandList, pipelineState);
        RHI::renderer->SetConstantBuffer(mainCommandList, 1, cameraConstantBuffer);
        RHI::renderer->Draw(mainCommandList, batch.numVerts, batch.firstVertex);
    }
}
