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
#include "DebugDrawVertex.h"
#include "RenderInternal.h"

static constexpr uint32_t DebugLightCircleLowSegments = 12;
static constexpr uint32_t DebugLightCircleMediumSegments = 24;
static constexpr uint32_t DebugUnitCircleSegmentCount = 48;
static constexpr float DebugLightCircleLowMaxRadiusPixels = 24.0f;
static constexpr float DebugLightCircleMediumMaxRadiusPixels = 96.0f;
static constexpr uint32_t DebugVolumeLightVertexCount = 24;
static constexpr float MinDebugLightSize = 0.01f;
static constexpr float MinDebugLightRadiusPixels = 1.0f;

static BE1::Vec2 debugUnitCircle[DebugUnitCircleSegmentCount];

static constexpr uint8_t DebugLightBoxEdgeIndices[DebugVolumeLightVertexCount] = {
    0, 1, 1, 2, 2, 3, 3, 0,
    4, 5, 5, 6, 6, 7, 7, 4,
    0, 4, 1, 5, 2, 6, 3, 7
};

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
static float GetUnitRadiusToPixelsScale(const VisCamera *visCamera) {
    // 카메라의 월드 공간 right, up 단위 방향 벡터
    const BE1::Vec3 cameraRightDir(visCamera->viewMatrix[0].x, visCamera->viewMatrix[0].y, visCamera->viewMatrix[0].z);
    const BE1::Vec3 cameraUpDir(visCamera->viewMatrix[1].x, visCamera->viewMatrix[1].y, visCamera->viewMatrix[1].z);

    // 월드 공간에서 right, up 방향으로 1 만큼 이동할 때 클립 공간의 오프셋
    const BE1::Vec4 rightClipOffset = visCamera->viewProjMatrix * BE1::Vec4(cameraRightDir, 0.0f);
    const BE1::Vec4 upClipOffset = visCamera->viewProjMatrix * BE1::Vec4(cameraUpDir, 0.0f);

    const float radiusToPixelsX = BE1::Math::Fabs(rightClipOffset.x) * static_cast<float>(visCamera->renderWidth) * 0.5f;
    const float radiusToPixelsY = BE1::Math::Fabs(upClipOffset.y) * static_cast<float>(visCamera->renderHeight) * 0.5f;

    return BE1::Max(radiusToPixelsX, radiusToPixelsY);
}

static uint32_t GetDebugPointLightCircleSegments(const VisCamera *visCamera, const VisLight *visLight, float unitRadiusToPixelsScale) {
    const BE1::Vec4 centerClip = visCamera->viewProjMatrix * BE1::Vec4(visLight->origin, 1.0f);
    if (centerClip.w > BE1::Math::FloatSmallestNonDenormal) {
        const float radius = BE1::Max(visLight->size[0], MinDebugLightSize);
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

static void WriteDebugPointLight(DebugDrawVertex *&vertexPtr, const VisLight *visLight, uint32_t numSegments, uint32_t color) {
    const float radius = BE1::Max(visLight->size[0], MinDebugLightSize);

    WriteDebugCircle(vertexPtr, visLight->origin, BE1::Mat3::identity[1], BE1::Mat3::identity[2], radius, numSegments, color);
    WriteDebugCircle(vertexPtr, visLight->origin, BE1::Mat3::identity[2], BE1::Mat3::identity[0], radius, numSegments, color);
    WriteDebugCircle(vertexPtr, visLight->origin, BE1::Mat3::identity[0], BE1::Mat3::identity[1], radius, numSegments, color);
}

// 볼륨의 8 개 꼭짓점을 12 개 모서리로 연결해 LineList 정점을 기록한다.
static void WriteDebugVolumeEdges(DebugDrawVertex *&vertexPtr, const BE1::Vec3 points[8], uint32_t color) {
    for (uint32_t vertexIndex = 0; vertexIndex < COUNT_OF(DebugLightBoxEdgeIndices); ++vertexIndex) {
        WriteDebugDrawVertex(vertexPtr, points[DebugLightBoxEdgeIndices[vertexIndex]], color);
    }
}

static void WriteDebugBoxDirLight(DebugDrawVertex *&vertexPtr, const VisLight *visLight, uint32_t color) {
    const BE1::Vec3 effectiveSize(
        BE1::Max(visLight->size[0], MinDebugLightSize),
        BE1::Max(visLight->size[1], MinDebugLightSize),
        BE1::Max(visLight->size[2], MinDebugLightSize));
    const BE1::OBB obb(
        visLight->origin + visLight->axis[0] * effectiveSize[0] * 0.5f,
        BE1::Vec3(effectiveSize[0] * 0.5f, effectiveSize[1], effectiveSize[2]),
        visLight->axis);

    BE1::Vec3 points[8];
    obb.ToPoints(points);
    WriteDebugVolumeEdges(vertexPtr, points, color);
}

static void WriteDebugSpotLight(DebugDrawVertex *&vertexPtr, const VisLight *visLight, uint32_t color) {
    const float zNear = BE1::Max(visLight->zNear, MinDebugLightSize);
    const float zFar = BE1::Max(BE1::Max(visLight->size[0], MinDebugLightSize), zNear + MinDebugLightSize);
    const float left = BE1::Max(visLight->size[1], MinDebugLightSize);
    const float up = BE1::Max(visLight->size[2], MinDebugLightSize);

    BE1::Frustum frustum;
    frustum.SetOrigin(visLight->origin);
    frustum.SetAxis(visLight->axis);
    frustum.SetSize(zNear, zFar, left, up);

    BE1::Vec3 points[8];
    frustum.ToPoints(points);
    WriteDebugVolumeEdges(vertexPtr, points, color);
}

void RenderBackend::DrawDebugLights() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawDebugLights");

    RHI::PipelineState *pipelineState = currentVisCamera->debugDrawPSOs[static_cast<int>(DebugDrawPipelineId::DepthTestedLines)];
    if (!currentVisCamera->showDebugLights || currentVisCamera->numVisLocalLights == 0 || !pipelineState) {
        return;
    }

    bool hasPointLights = false;
    float unitRadiusToPixelsScale = 0.0f;
    uint64_t numVerts = 0;

    for (uint32_t lightIndex = 0; lightIndex < currentVisCamera->numVisLocalLights; ++lightIndex) {
        const VisLight *visLight = currentVisCamera->visLocalLights[lightIndex];
        if (!visLight) {
            continue;
        }

        switch (visLight->type) {
        case RenderLight::Type::Point: {
            // point light radius 의 화면 픽셀의 크기에 따라서 circle segment 의 개수를 조절한다
            if (!hasPointLights) {
                unitRadiusToPixelsScale = GetUnitRadiusToPixelsScale(currentVisCamera);
                hasPointLights = true;
            }

            // 너무 작으면 0 을 리턴한다.
            const uint32_t numCircleSegments = GetDebugPointLightCircleSegments(currentVisCamera, visLight, unitRadiusToPixelsScale);
            numVerts += 3 * numCircleSegments * 2;
            break;
        }
        case RenderLight::Type::Spot:
        case RenderLight::Type::BoxDir:
            numVerts += DebugVolumeLightVertexCount;
            break;
        default:
            assert(0);
            break;
        }
    }

    if (numVerts == 0) {
        return;
    }

    RHI::VertexBuffer *vertexBuffer = frameData->GetThreadData(0)->AllocVertex(sizeof(DebugDrawVertex), numVerts);
    if (!vertexBuffer || !vertexBuffer->mappedPtr) {
        BE_WARNLOG("RenderBackend::DrawDebugLights: failed to allocate light bounds vertex buffer\n");
        return;
    }

    DebugDrawVertex *vertexPtr = reinterpret_cast<DebugDrawVertex *>(vertexBuffer->mappedPtr);
    DebugDrawVertex *const firstVertexPtr = vertexPtr;

    for (uint32_t lightIndex = 0; lightIndex < currentVisCamera->numVisLocalLights; ++lightIndex) {
        const VisLight *visLight = currentVisCamera->visLocalLights[lightIndex];
        if (!visLight) {
            continue;
        }

        const uint32_t color = BE1::Color4(visLight->lightColor, 1.0f).ToUInt32();
        switch (visLight->type) {
        case RenderLight::Type::Point: {
            const uint32_t numCircleSegments = GetDebugPointLightCircleSegments(currentVisCamera, visLight, unitRadiusToPixelsScale);
            if (numCircleSegments > 0) {
                WriteDebugPointLight(vertexPtr, visLight, numCircleSegments, color);
            }
            break;
        }
        case RenderLight::Type::Spot:
            WriteDebugSpotLight(vertexPtr, visLight, color);
            break;
        case RenderLight::Type::BoxDir:
            WriteDebugBoxDirLight(vertexPtr, visLight, color);
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

    const DebugDrawData *debugDrawData = currentVisCamera->debugDrawData;
    if (!debugDrawData || !debugDrawData->vertexBuffer || debugDrawData->numBatches == 0) {
        return;
    }

    assert(debugDrawData->numBatches <= DebugDrawData::MaxBatches);

    RHI::renderer->SetVertexBuffer(mainCommandList, 0, debugDrawData->vertexBuffer);

    for (uint32_t batchIndex = 0; batchIndex < debugDrawData->numBatches; ++batchIndex) {
        const DebugDrawBatch &batch = debugDrawData->batches[batchIndex];
        RHI::PipelineState *pipelineState = currentVisCamera->debugDrawPSOs[static_cast<int>(batch.pipelineId)];
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
