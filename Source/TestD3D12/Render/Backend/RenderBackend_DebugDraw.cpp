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
#include "RenderBackend.h"
#include "Render/DebugDrawVertex.h"
#include "Render/RenderInternal.h"
#include "Render/RenderFrame.h"

static constexpr uint32_t DebugLightCircleLowSegments = 12;
static constexpr uint32_t DebugLightCircleMediumSegments = 24;
static constexpr uint32_t DebugUnitCircleSegmentCount = 48;
static constexpr float DebugLightCircleLowMaxRadiusPixels = 24.0f;
static constexpr float DebugLightCircleMediumMaxRadiusPixels = 96.0f;
static constexpr uint32_t DebugSpotArcSegments = 16;
static constexpr uint32_t DebugSpotConeVertexCount = DebugLightCircleMediumSegments * 2 + 8 + DebugSpotArcSegments * 4;
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

static uint32_t GetDebugSphereCircleSegments(const FrameCamera *frameCamera, const BE1::Vec3 &origin, float radius, float unitRadiusToPixelsScale) {
    const BE1::Vec4 centerClip = frameCamera->viewProjMatrix * BE1::Vec4(origin, 1.0f);
    if (centerClip.w > std::numeric_limits<float>::min()) {
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

static void WriteDebugSphere(DebugDrawVertex *&vertexPtr, const BE1::Vec3 &origin, float radius, uint32_t numSegments, uint32_t color) {
    WriteDebugCircle(vertexPtr, origin, BE1::Mat3::identity[1], BE1::Mat3::identity[2], radius, numSegments, color);
    WriteDebugCircle(vertexPtr, origin, BE1::Mat3::identity[2], BE1::Mat3::identity[0], radius, numSegments, color);
    WriteDebugCircle(vertexPtr, origin, BE1::Mat3::identity[0], BE1::Mat3::identity[1], radius, numSegments, color);
}

static void WriteDebugBox(DebugDrawVertex *&vertexPtr, const FrameLocalFogVolume &volume, const BE1::Vec3 &halfExtents, uint32_t color) {
    static constexpr uint8_t edgeIndices[24] = {
        0, 1, 1, 2, 2, 3, 3, 0,
        4, 5, 5, 6, 6, 7, 7, 4,
        0, 4, 1, 5, 2, 6, 3, 7
    };
    BE1::Vec3 points[8];
    BE1::OBB(volume.origin, halfExtents, volume.axis).ToPoints(points);
    for (uint8_t index : edgeIndices) {
        WriteDebugDrawVertex(vertexPtr, points[index], color);
    }
}

// 안쪽 경계가 한 점으로 줄어들면 볼륨 크기에 비례하는 작은 십자선으로 위치를 표시한다.
static void WriteDebugCross(DebugDrawVertex *&vertexPtr, const BE1::Vec3 &origin, const BE1::Mat3 &axis, float size, uint32_t color) {
    for (uint32_t i = 0; i < 3; ++i) {
        const BE1::Vec3 offset = axis[i] * size;
        WriteDebugDrawVertex(vertexPtr, origin - offset, color);
        WriteDebugDrawVertex(vertexPtr, origin + offset, color);
    }
}

static BE1::Vec3 GetDebugFogInnerBoxHalfExtents(const FrameLocalFogVolume &volume) {
    // 셰이더의 min(distanceToFaces) 와 같은 등거리 경계다. 축별 비율로 축소하지 않는다.
    const float fadeDistance = volume.boundaryFade * BE1::Min3(volume.halfExtents.x, volume.halfExtents.y, volume.halfExtents.z);
    return volume.halfExtents - BE1::Vec3(fadeDistance);
}

static uint32_t GetDebugFogInnerBoxVertexCount(const BE1::Vec3 &halfExtents) {
    const uint32_t dimensions = (halfExtents.x > 0.0f) + (halfExtents.y > 0.0f) + (halfExtents.z > 0.0f);
    // 박스 12변, 사각형 4변, 선 1개, 점 위치의 십자선 3개.
    return dimensions > 0 ? dimensions * (1u << dimensions) : 6u;
}

static void WriteDebugFogInnerBox(DebugDrawVertex *&vertexPtr, const FrameLocalFogVolume &volume, const BE1::Vec3 &halfExtents, uint32_t color) {
    if (halfExtents.x > 0.0f && halfExtents.y > 0.0f && halfExtents.z > 0.0f) {
        WriteDebugBox(vertexPtr, volume, halfExtents, color);
        return;
    }

    // Fade 1 에서 같은 모서리를 중복하거나 길이 0 인 선을 기록하지 않는다.
    BE1::Vec3 halfAxes[2];
    uint32_t dimensions = 0;
    for (uint32_t i = 0; i < 3; ++i) {
        if (halfExtents[i] > 0.0f) {
            halfAxes[dimensions++] = volume.axis[i] * halfExtents[i];
        }
    }
    if (dimensions == 2) {
        const BE1::Vec3 points[4] = {
            volume.origin - halfAxes[0] - halfAxes[1], volume.origin + halfAxes[0] - halfAxes[1],
            volume.origin + halfAxes[0] + halfAxes[1], volume.origin - halfAxes[0] + halfAxes[1]
        };
        for (uint32_t i = 0; i < 4; ++i) {
            WriteDebugDrawVertex(vertexPtr, points[i], color);
            WriteDebugDrawVertex(vertexPtr, points[(i + 1) & 3], color);
        }
    } else if (dimensions == 1) {
        WriteDebugDrawVertex(vertexPtr, volume.origin - halfAxes[0], color);
        WriteDebugDrawVertex(vertexPtr, volume.origin + halfAxes[0], color);
    } else {
        WriteDebugCross(vertexPtr, volume.origin, volume.axis, volume.halfExtents.x * 0.025f, color);
    }
}

// 방사형 range 구면과 콘이 만나는 원, 원점에서 원으로 향하는 네 모서리, 구면 끝의 직교하는 두 arc 를 LineList 로 기록한다.
static void WriteDebugSpotCone(DebugDrawVertex *&vertexPtr, const FrameLight *frameLight, float cosCone, uint32_t color) {
    const float sinCone = BE1::Math::Sqrt(BE1::Max(0.0f, 1.0f - cosCone * cosCone));
    const float coneLength = frameLight->range * cosCone;
    const float coneRadius = frameLight->range * sinCone;
    const BE1::Vec3 &forward = frameLight->axis[EngineAxis::Forward];
    const BE1::Vec3 center = frameLight->origin + forward * coneLength;
    const BE1::Vec3 &left = frameLight->axis[EngineAxis::Left];
    const BE1::Vec3 &up = frameLight->axis[EngineAxis::Up];
    WriteDebugCircle(vertexPtr, center, left, up, coneRadius, DebugLightCircleMediumSegments, color);

    for (uint32_t pointIndex = 0; pointIndex < DebugUnitCircleSegmentCount; pointIndex += DebugUnitCircleSegmentCount / 4) {
        const BE1::Vec2 &point = debugUnitCircle[pointIndex];
        WriteDebugDrawVertex(vertexPtr, frameLight->origin, color);
        WriteDebugDrawVertex(vertexPtr, center + (left * point.x + up * point.y) * coneRadius, color);
    }

    // 한 번 계산한 회전 증분을 두 arc 에 공유하여 세그먼트마다 삼각함수를 계산하지 않는다.
    float sinStep;
    float cosStep;
    BE1::Math::SinCos(2.0f * BE1::Math::ACos(cosCone) / static_cast<float>(DebugSpotArcSegments), sinStep, cosStep);
    float cosAngle = cosCone;
    float sinAngle = -sinCone;
    BE1::Vec3 previousLeft = center - left * coneRadius;
    BE1::Vec3 previousUp = center - up * coneRadius;

    for (uint32_t segmentIndex = 1; segmentIndex <= DebugSpotArcSegments; ++segmentIndex) {
        // 중앙과 끝점은 정확히 맞춰 두 arc 의 교차점과 경계 원의 연결을 유지한다.
        if (segmentIndex == DebugSpotArcSegments / 2) {
            cosAngle = 1.0f;
            sinAngle = 0.0f;
        } else if (segmentIndex == DebugSpotArcSegments) {
            cosAngle = cosCone;
            sinAngle = sinCone;
        } else {
            const float nextCosAngle = cosAngle * cosStep - sinAngle * sinStep;
            sinAngle = sinAngle * cosStep + cosAngle * sinStep;
            cosAngle = nextCosAngle;
        }

        const BE1::Vec3 arcCenter = frameLight->origin + forward * (frameLight->range * cosAngle);
        const float arcRadius = frameLight->range * sinAngle;
        const BE1::Vec3 nextLeft = arcCenter + left * arcRadius;
        const BE1::Vec3 nextUp = arcCenter + up * arcRadius;
        WriteDebugDrawVertex(vertexPtr, previousLeft, color);
        WriteDebugDrawVertex(vertexPtr, nextLeft, color);
        WriteDebugDrawVertex(vertexPtr, previousUp, color);
        WriteDebugDrawVertex(vertexPtr, nextUp, color);
        previousLeft = nextLeft;
        previousUp = nextUp;
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
            const uint32_t numCircleSegments = GetDebugSphereCircleSegments(currentFrameCamera, frameLight->origin, BE1::Max(frameLight->range, MinDebugLightSize), unitRadiusToPixelsScale);
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
            const float radius = BE1::Max(frameLight->range, MinDebugLightSize);
            const uint32_t numCircleSegments = GetDebugSphereCircleSegments(currentFrameCamera, frameLight->origin, radius, unitRadiusToPixelsScale);
            if (numCircleSegments > 0) {
                WriteDebugSphere(vertexPtr, frameLight->origin, radius, numCircleSegments, color);
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

void RenderBackend::DrawDebugLocalFogVolumes() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawDebugLocalFogVolumes");

    RHI::PipelineState *pipelineState = currentFrameCamera->debugDrawPSOs[static_cast<int>(DebugDrawPipelineId::DepthTestedLines)];
    const uint32_t numVolumes = currentFrameCamera->numFrameLocalFogVolumes;
    if (!currentFrameCamera->showDebugLocalFogVolumes || numVolumes == 0 || !pipelineState) {
        return;
    }

    const auto volumes = frame->GetFrameLocalFogVolumes(currentFrameCamera->firstFrameLocalFogVolume);
    bool hasSpheres = false;
    float unitRadiusToPixelsScale = 0.0f;
    uint64_t numVerts = 0;
    auto volume = volumes.begin();
    for (uint32_t i = 0; i < numVolumes; ++i, ++volume) {
        if (volume->shape == LocalFogVolume::Shape::Sphere) {
            if (!hasSpheres) {
                unitRadiusToPixelsScale = GetUnitRadiusToPixelsScale(currentFrameCamera);
                hasSpheres = true;
            }
            const uint32_t numSegments = GetDebugSphereCircleSegments(currentFrameCamera, volume->origin, volume->radius, unitRadiusToPixelsScale);
            numVerts += 6u * numSegments;
            if (numSegments > 0 && volume->boundaryFade > 0.0f) {
                const float innerRadius = volume->radius - volume->radius * volume->boundaryFade;
                numVerts += innerRadius > 0.0f ? 6u * GetDebugSphereCircleSegments(currentFrameCamera, volume->origin, innerRadius, unitRadiusToPixelsScale) : 6u;
            }
        } else {
            numVerts += 24u;
            if (volume->boundaryFade > 0.0f) {
                numVerts += GetDebugFogInnerBoxVertexCount(GetDebugFogInnerBoxHalfExtents(*volume));
            }
        }
    }

    if (numVerts == 0) {
        return;
    }

    if (numVerts > UINT32_MAX / sizeof(DebugDrawVertex)) {
        BE_WARNLOG("RenderBackend::DrawDebugLocalFogVolumes: volume bounds vertex buffer is too large\n");
        return;
    }

    const uint32_t vertexCount = static_cast<uint32_t>(numVerts);
    RHI::VertexBuffer *vertexBuffer = frame->GetThreadData(0)->AllocVertex(sizeof(DebugDrawVertex), vertexCount);
    if (!vertexBuffer || !vertexBuffer->mappedPtr) {
        BE_WARNLOG("RenderBackend::DrawDebugLocalFogVolumes: failed to allocate volume bounds vertex buffer\n");
        return;
    }

    const uint32_t sphereColor = BE1::Color4(0.1f, 0.85f, 1.0f, 1.0f).ToUInt32();
    const uint32_t boxColor = BE1::Color4(1.0f, 0.55f, 0.1f, 1.0f).ToUInt32();
    const uint32_t innerColor = BE1::Color4::white.ToUInt32();
    DebugDrawVertex *vertexPtr = static_cast<DebugDrawVertex *>(vertexBuffer->mappedPtr);
    DebugDrawVertex *const firstVertexPtr = vertexPtr;
    volume = volumes.begin();
    for (uint32_t i = 0; i < numVolumes; ++i, ++volume) {
        if (volume->shape == LocalFogVolume::Shape::Sphere) {
            const uint32_t numSegments = GetDebugSphereCircleSegments(currentFrameCamera, volume->origin, volume->radius, unitRadiusToPixelsScale);
            if (numSegments > 0) {
                WriteDebugSphere(vertexPtr, volume->origin, volume->radius, numSegments, sphereColor);
                if (volume->boundaryFade > 0.0f) {
                    const float innerRadius = volume->radius - volume->radius * volume->boundaryFade;
                    if (innerRadius > 0.0f) {
                        const uint32_t innerSegments = GetDebugSphereCircleSegments(currentFrameCamera, volume->origin, innerRadius, unitRadiusToPixelsScale);
                        if (innerSegments > 0) {
                            WriteDebugSphere(vertexPtr, volume->origin, innerRadius, innerSegments, innerColor);
                        }
                    } else {
                        WriteDebugCross(vertexPtr, volume->origin, BE1::Mat3::identity, volume->radius * 0.025f, innerColor);
                    }
                }
            }
        } else {
            WriteDebugBox(vertexPtr, *volume, volume->halfExtents, boxColor);
            if (volume->boundaryFade > 0.0f) {
                WriteDebugFogInnerBox(vertexPtr, *volume, GetDebugFogInnerBoxHalfExtents(*volume), innerColor);
            }
        }
    }
    assert(static_cast<uint32_t>(vertexPtr - firstVertexPtr) == vertexCount);

    RHI::renderer->SetVertexBuffer(mainCommandList, 0, vertexBuffer);
    RHI::renderer->SetPSO(mainCommandList, pipelineState);
    RHI::renderer->SetConstantBuffer(mainCommandList, 1, cameraConstantBuffer);
    RHI::renderer->Draw(mainCommandList, vertexCount, 0);
}

void RenderBackend::DrawDebugPrimitives() {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawDebugPrimitives");
    assert(cameraConstantBuffer);

    DrawDebugLights();
    DrawDebugLocalFogVolumes();

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
