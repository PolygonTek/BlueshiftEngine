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
#include "RenderWorld.h"
#include "Render/RenderCvars.h"

static constexpr uint32_t DebugCircleSegments = 48;
static constexpr uint32_t DebugEllipseSegments = 64;
static constexpr uint32_t DebugVolumeSegments = 24;
static constexpr uint32_t DebugHemisphereRings = 6;
static constexpr uint32_t DebugSphereRings = 11;
static constexpr float DebugDirectionEpsilonSqr = 1e-12f;

static uint32_t GetDebugDrawMaxPrimitives() {
    return static_cast<uint32_t>(BE1::Max(r_debugDrawMaxPrimitives.GetInteger(), 1));
}

static uint32_t GetDebugDrawMaxVertices() {
    return static_cast<uint32_t>(BE1::Max(r_debugDrawMaxVertices.GetInteger(), 1));
}

static constexpr uint8_t DebugBoxTriangleIndices[36] = {
    0, 2, 1, 0, 3, 2,
    4, 5, 6, 4, 6, 7,
    0, 1, 5, 0, 5, 4,
    3, 7, 6, 3, 6, 2,
    0, 4, 7, 0, 7, 3,
    1, 2, 6, 1, 6, 5
};

static constexpr uint8_t DebugBoxEdgeIndices[24] = {
    0, 1, 1, 2, 2, 3, 3, 0,
    4, 5, 5, 6, 6, 7, 7, 4,
    0, 4, 1, 5, 2, 6, 3, 7
};

static bool BuildDebugBasis(const BE1::Vec3 &direction, BE1::Vec3 &left, BE1::Vec3 &up) {
    if (direction.LengthSqr() <= DebugDirectionEpsilonSqr) {
        return false;
    }

    BE1::Vec3 normalizedDirection = direction;
    normalizedDirection.Normalize();
    normalizedDirection.OrthogonalBasis(left, up);
    return true;
}

static void BuildDebugRing(BE1::Vec3 *points, uint32_t numSegments, const BE1::Vec3 &origin, const BE1::Vec3 &xRadius, const BE1::Vec3 &yRadius) {
    for (uint32_t segmentIndex = 0; segmentIndex < numSegments; ++segmentIndex) {
        const float angle = BE1::Math::TwoPi * static_cast<float>(segmentIndex) / static_cast<float>(numSegments);
        float s;
        float c;
        BE1::Math::SinCos(angle, s, c);
        points[segmentIndex] = origin + xRadius * c + yRadius * s;
    }
}

DebugDrawVertex *RenderWorld::ReserveDebugVertices(DebugDrawPipelineId pipelineId, uint32_t numVerts, const BE1::Color4 &color, int lifeTime) {
    assert(BE1::Engine::IsInMainThread());
    assert(lifeTime >= 0);

    if (numVerts == 0 || color.a <= 0.0f) {
        return nullptr;
    }

    const uint32_t maxPrimitives = GetDebugDrawMaxPrimitives();
    const uint32_t maxVertices = GetDebugDrawMaxVertices();
    if (debugPrimitiveCount >= maxPrimitives || debugPrimitiveVertexCount >= maxVertices || numVerts > maxVertices - debugPrimitiveVertexCount) {
        // 프레임 당 한번만 경고한다.
        const uint64_t targetFrameIndex = GetCurrentOrNextFrameIndex();
        if (debugCapacityWarningFrameIndex != targetFrameIndex) {
            debugCapacityWarningFrameIndex = targetFrameIndex;
            BE_WARNLOG("RenderWorld::ReserveDebugVertices: debug primitive capacity exceeded (%u primitives, %u vertices)\n", maxPrimitives, maxVertices);
        }
        return nullptr;
    }

    DebugPrimitiveBucket &bucket = debugPrimitiveBuckets[static_cast<int>(pipelineId)];
    const uint32_t firstVertex = static_cast<uint32_t>(bucket.vertices.Count());
    const uint32_t newBucketVertexCount = firstVertex + numVerts;

    bucket.vertices.Reserve(newBucketVertexCount);
    bucket.vertices.SetCount(newBucketVertexCount, false);

    const uint32_t packedColor = color.ToUInt32();
    for (uint32_t vertexIndex = firstVertex; vertexIndex < newBucketVertexCount; ++vertexIndex) {
        bucket.vertices[vertexIndex].color = packedColor;
    }

    DebugPrimitiveRange &primitive = bucket.primitives.Alloc();
    primitive.firstVertex = firstVertex;
    primitive.numVerts = numVerts;

    if (lifeTime > 0) {
        primitive.expireFrameIndex = ~uint64_t(0);
        primitive.expireTimeUsec = BE1::PlatformTime::Microseconds() + static_cast<uint64_t>(lifeTime) * 1000;
    } else {
        primitive.expireFrameIndex = GetCurrentOrNextFrameIndex();
        primitive.expireTimeUsec = 0;
    }

    debugPrimitiveCount++;
    debugPrimitiveVertexCount += numVerts;

    return bucket.vertices.Ptr() + firstVertex;
}

DebugDrawVertex *RenderWorld::ReserveDebugLineVertices(uint32_t numVerts, bool depthTest, int lifeTime) {
    const DebugDrawPipelineId pipelineId = depthTest ? DebugDrawPipelineId::DepthTestedLines : DebugDrawPipelineId::Lines;
    return ReserveDebugVertices(pipelineId, numVerts, debugLineColor, lifeTime);
}

DebugDrawVertex *RenderWorld::ReserveDebugFillVertices(uint32_t numVerts, bool twoSided, bool depthTest, int lifeTime) {
    DebugDrawPipelineId pipelineId;
    if (depthTest) {
        pipelineId = twoSided ? DebugDrawPipelineId::DepthTestedTwoSidedFill : DebugDrawPipelineId::DepthTestedOneSidedFill;
    } else {
        pipelineId = twoSided ? DebugDrawPipelineId::TwoSidedFill : DebugDrawPipelineId::OneSidedFill;
    }
    return ReserveDebugVertices(pipelineId, numVerts, debugFillColor, lifeTime);
}

void RenderWorld::ExpireDebugPrimitives(uint64_t currentTimeUsec) {
    uint32_t remainingPrimitiveCount = 0;
    uint32_t remainingVertexCount = 0;

    for (DebugPrimitiveBucket &bucket : debugPrimitiveBuckets) {
        uint32_t writePrimitiveIndex = 0;
        uint32_t writeVertexIndex = 0;

        const uint32_t numPrimitives = static_cast<uint32_t>(bucket.primitives.Count());
        for (uint32_t primitiveIndex = 0; primitiveIndex < numPrimitives; ++primitiveIndex) {
            const DebugPrimitiveRange primitive = bucket.primitives[static_cast<int>(primitiveIndex)];
            const bool expiredByFrame = primitive.expireFrameIndex != ~uint64_t(0) && primitive.expireFrameIndex < frameIndex;
            const bool expiredByTime = primitive.expireTimeUsec != 0 && primitive.expireTimeUsec <= currentTimeUsec;
            if (expiredByFrame || expiredByTime) {
                continue;
            }

            // 아직 만료되지 않았다면 다시 버텍스 데이터들을 카피해온다.
            if (primitive.firstVertex != writeVertexIndex) {
                for (uint32_t vertexIndex = 0; vertexIndex < primitive.numVerts; ++vertexIndex) {
                    bucket.vertices[static_cast<int>(writeVertexIndex + vertexIndex)] = bucket.vertices[static_cast<int>(primitive.firstVertex + vertexIndex)];
                }
            }

            DebugPrimitiveRange &retainedPrimitive = bucket.primitives[static_cast<int>(writePrimitiveIndex)];
            retainedPrimitive = primitive;
            retainedPrimitive.firstVertex = writeVertexIndex;

            writePrimitiveIndex++;
            writeVertexIndex += primitive.numVerts;
        }

        bucket.primitives.SetCount(static_cast<int>(writePrimitiveIndex), false);
        bucket.vertices.SetCount(static_cast<int>(writeVertexIndex), false);

        remainingPrimitiveCount += writePrimitiveIndex;
        remainingVertexCount += writeVertexIndex;
    }

    debugPrimitiveCount = remainingPrimitiveCount;
    debugPrimitiveVertexCount = remainingVertexCount;
}

void RenderWorld::SetDebugColor(const BE1::Color4 &lineColor, const BE1::Color4 &fillColor) {
    assert(BE1::Engine::IsInMainThread());

    debugLineColor = lineColor;
    debugFillColor = fillColor;
}

void RenderWorld::ClearDebugPrimitives() {
    assert(BE1::Engine::IsInMainThread());

    if (debugPrimitiveCount == 0) {
        return;
    }

    for (DebugPrimitiveBucket &bucket : debugPrimitiveBuckets) {
        bucket.vertices.SetCount(0, false);
        bucket.primitives.SetCount(0, false);
    }

    debugPrimitiveCount = 0;
    debugPrimitiveVertexCount = 0;
}

void RenderWorld::DrawDebugPoint(const BE1::Vec3 &origin, float size, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    assert(std::isfinite(origin.x) && std::isfinite(origin.y) && std::isfinite(origin.z));
    assert(std::isfinite(size));

    if (size <= 0.0f) {
        return;
    }

    DebugDrawVertex *verts = ReserveDebugLineVertices(6, depthTest, lifeTime);
    if (!verts) {
        return;
    }

    const BE1::Vec3 xExtent(size, 0.0f, 0.0f);
    const BE1::Vec3 yExtent(0.0f, size, 0.0f);
    const BE1::Vec3 zExtent(0.0f, 0.0f, size);

    verts[0].xyz = origin - xExtent;
    verts[1].xyz = origin + xExtent;
    verts[2].xyz = origin - yExtent;
    verts[3].xyz = origin + yExtent;
    verts[4].xyz = origin - zExtent;
    verts[5].xyz = origin + zExtent;
}

void RenderWorld::DrawDebugLine(const BE1::Vec3 &start, const BE1::Vec3 &end, bool depthTest, int lifeTime) {
    assert(std::isfinite(start.x) && std::isfinite(start.y) && std::isfinite(start.z));
    assert(std::isfinite(end.x) && std::isfinite(end.y) && std::isfinite(end.z));

    DebugDrawVertex *verts = ReserveDebugLineVertices(2, depthTest, lifeTime);
    if (!verts) {
        return;
    }

    verts[0].xyz = start;
    verts[1].xyz = end;
}

void RenderWorld::DrawDebugPolyline(const BE1::Vec3 *points, uint32_t numPoints, bool closed /*= false*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    assert(points || numPoints == 0);

    if (!points || numPoints < 2) {
        return;
    }

    for (uint32_t pointIndex = 0; pointIndex < numPoints; ++pointIndex) {
        const BE1::Vec3 &point = points[pointIndex];
        assert(std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z));
    }

    const bool closeLoop = closed && numPoints > 2;
    const uint32_t numSegments = numPoints - 1 + (closeLoop ? 1 : 0);
    DebugDrawVertex *verts = ReserveDebugLineVertices(numSegments * 2, depthTest, lifeTime);
    if (!verts) {
        return;
    }

    uint32_t vertexIndex = 0;
    for (uint32_t pointIndex = 0; pointIndex + 1 < numPoints; ++pointIndex) {
        verts[vertexIndex++].xyz = points[pointIndex];
        verts[vertexIndex++].xyz = points[pointIndex + 1];
    }
    if (closeLoop) {
        verts[vertexIndex++].xyz = points[numPoints - 1];
        verts[vertexIndex++].xyz = points[0];
    }
    assert(vertexIndex == numSegments * 2);
}

void RenderWorld::DrawDebugAxis(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float size, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    assert(std::isfinite(origin.x) && std::isfinite(origin.y) && std::isfinite(origin.z));
    assert(std::isfinite(axis[0].x) && std::isfinite(axis[0].y) && std::isfinite(axis[0].z));
    assert(std::isfinite(axis[1].x) && std::isfinite(axis[1].y) && std::isfinite(axis[1].z));
    assert(std::isfinite(axis[2].x) && std::isfinite(axis[2].y) && std::isfinite(axis[2].z));
    assert(std::isfinite(size));

    if (size <= 0.0f) {
        return;
    }

    DebugDrawVertex *verts = ReserveDebugLineVertices(6, depthTest, lifeTime);
    if (!verts) {
        return;
    }

    const uint32_t axisColors[3] = {
        BE1::Color4(1.0f, 0.0f, 0.0f, debugLineColor.a).ToUInt32(),
        BE1::Color4(0.0f, 1.0f, 0.0f, debugLineColor.a).ToUInt32(),
        BE1::Color4(0.0f, 0.0f, 1.0f, debugLineColor.a).ToUInt32()
    };
    for (uint32_t axisIndex = 0; axisIndex < 3; ++axisIndex) {
        verts[axisIndex * 2 + 0].xyz = origin;
        verts[axisIndex * 2 + 1].xyz = origin + axis[axisIndex] * size;
        verts[axisIndex * 2 + 0].color = axisColors[axisIndex];
        verts[axisIndex * 2 + 1].color = axisColors[axisIndex];
    }
}

void RenderWorld::DrawDebugTriangle(const BE1::Vec3 &a, const BE1::Vec3 &b, const BE1::Vec3 &c, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    assert(std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z));
    assert(std::isfinite(b.x) && std::isfinite(b.y) && std::isfinite(b.z));
    assert(std::isfinite(c.x) && std::isfinite(c.y) && std::isfinite(c.z));

    DebugDrawVertex *fillVerts = ReserveDebugFillVertices(3, twoSided, depthTest, lifeTime);
    if (fillVerts) {
        fillVerts[0].xyz = a;
        fillVerts[1].xyz = b;
        fillVerts[2].xyz = c;
    }

    DebugDrawVertex *lineVerts = ReserveDebugLineVertices(6, depthTest, lifeTime);
    if (lineVerts) {
        lineVerts[0].xyz = a;
        lineVerts[1].xyz = b;
        lineVerts[2].xyz = b;
        lineVerts[3].xyz = c;
        lineVerts[4].xyz = c;
        lineVerts[5].xyz = a;
    }
}

void RenderWorld::DrawDebugQuad(const BE1::Vec3 &origin, const BE1::Vec3 &right, const BE1::Vec3 &up, float size, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    assert(std::isfinite(origin.x) && std::isfinite(origin.y) && std::isfinite(origin.z));
    assert(std::isfinite(right.x) && std::isfinite(right.y) && std::isfinite(right.z));
    assert(std::isfinite(up.x) && std::isfinite(up.y) && std::isfinite(up.z));
    assert(std::isfinite(size));

    if (size <= 0.0f) {
        return;
    }

    const BE1::Vec3 scaledRight = right * size;
    const BE1::Vec3 scaledUp = up * size;
    const BE1::Vec3 points[4] = {
        origin + scaledRight - scaledUp,
        origin - scaledRight - scaledUp,
        origin - scaledRight + scaledUp,
        origin + scaledRight + scaledUp
    };

    DebugDrawVertex *fillVerts = ReserveDebugFillVertices(6, twoSided, depthTest, lifeTime);
    if (fillVerts) {
        fillVerts[0].xyz = points[0];
        fillVerts[1].xyz = points[3];
        fillVerts[2].xyz = points[2];
        fillVerts[3].xyz = points[0];
        fillVerts[4].xyz = points[2];
        fillVerts[5].xyz = points[1];
    }

    DebugDrawVertex *lineVerts = ReserveDebugLineVertices(8, depthTest, lifeTime);
    if (lineVerts) {
        static constexpr uint8_t edgeIndices[8] = { 0, 3, 3, 2, 2, 1, 1, 0 };
        for (uint32_t vertexIndex = 0; vertexIndex < COUNT_OF(edgeIndices); ++vertexIndex) {
            lineVerts[vertexIndex].xyz = points[edgeIndices[vertexIndex]];
        }
    }
}

void RenderWorld::DrawDebugPlane(const BE1::Vec3 &origin, const BE1::Vec3 &normal, float size, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    assert(std::isfinite(origin.x) && std::isfinite(origin.y) && std::isfinite(origin.z));
    assert(std::isfinite(normal.x) && std::isfinite(normal.y) && std::isfinite(normal.z));
    assert(std::isfinite(size));

    if (size <= 0.0f || normal.LengthSqr() <= DebugDirectionEpsilonSqr) {
        return;
    }

    BE1::Vec3 normalizedNormal = normal;
    normalizedNormal.Normalize();

    BE1::Vec3 right;
    BE1::Vec3 up;
    normalizedNormal.OrthogonalBasis(right, up);

    DrawDebugQuad(origin, right, up, size, true, depthTest, lifeTime);
    DrawDebugArrow(origin, origin + normalizedNormal * size, size * 0.25f, size * 0.08f, depthTest, lifeTime);
}

void RenderWorld::DrawDebugCircle(const BE1::Vec3 &origin, const BE1::Vec3 &dir, const float radius, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    assert(std::isfinite(origin.x) && std::isfinite(origin.y) && std::isfinite(origin.z));
    assert(std::isfinite(dir.x) && std::isfinite(dir.y) && std::isfinite(dir.z));
    assert(std::isfinite(radius));

    if (radius <= 0.0f) {
        return;
    }

    BE1::Vec3 left;
    BE1::Vec3 up;
    if (!BuildDebugBasis(dir, left, up)) {
        return;
    }

    BE1::Vec3 points[DebugCircleSegments];
    BuildDebugRing(points, DebugCircleSegments, origin, left * radius, up * radius);

    DebugDrawVertex *fillVerts = ReserveDebugFillVertices(DebugCircleSegments * 3, twoSided, depthTest, lifeTime);
    if (fillVerts) {
        for (uint32_t segmentIndex = 0; segmentIndex < DebugCircleSegments; ++segmentIndex) {
            const uint32_t nextIndex = (segmentIndex + 1) % DebugCircleSegments;
            fillVerts[segmentIndex * 3 + 0].xyz = origin;
            fillVerts[segmentIndex * 3 + 1].xyz = points[segmentIndex];
            fillVerts[segmentIndex * 3 + 2].xyz = points[nextIndex];
        }
    }

    DebugDrawVertex *lineVerts = ReserveDebugLineVertices(DebugCircleSegments * 2, depthTest, lifeTime);
    if (lineVerts) {
        for (uint32_t segmentIndex = 0; segmentIndex < DebugCircleSegments; ++segmentIndex) {
            const uint32_t nextIndex = (segmentIndex + 1) % DebugCircleSegments;
            lineVerts[segmentIndex * 2 + 0].xyz = points[segmentIndex];
            lineVerts[segmentIndex * 2 + 1].xyz = points[nextIndex];
        }
    }
}

void RenderWorld::DrawDebugHollowCircle(const BE1::Vec3 &origin, const BE1::Vec3 &dir, const float radius1, const float radius2, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    assert(std::isfinite(origin.x) && std::isfinite(origin.y) && std::isfinite(origin.z));
    assert(std::isfinite(dir.x) && std::isfinite(dir.y) && std::isfinite(dir.z));
    assert(std::isfinite(radius1));
    assert(std::isfinite(radius2));

    const float innerRadius = radius1 < radius2 ? radius1 : radius2;
    const float outerRadius = radius1 < radius2 ? radius2 : radius1;
    if (innerRadius <= 0.0f || outerRadius <= innerRadius) {
        if (outerRadius > 0.0f) {
            DrawDebugCircle(origin, dir, outerRadius, twoSided, depthTest, lifeTime);
        }
        return;
    }

    BE1::Vec3 left;
    BE1::Vec3 up;
    if (!BuildDebugBasis(dir, left, up)) {
        return;
    }

    BE1::Vec3 innerPoints[DebugCircleSegments];
    BE1::Vec3 outerPoints[DebugCircleSegments];
    BuildDebugRing(innerPoints, DebugCircleSegments, origin, left * innerRadius, up * innerRadius);
    BuildDebugRing(outerPoints, DebugCircleSegments, origin, left * outerRadius, up * outerRadius);

    DebugDrawVertex *fillVerts = ReserveDebugFillVertices(DebugCircleSegments * 6, twoSided, depthTest, lifeTime);
    if (fillVerts) {
        for (uint32_t segmentIndex = 0; segmentIndex < DebugCircleSegments; ++segmentIndex) {
            const uint32_t nextIndex = (segmentIndex + 1) % DebugCircleSegments;
            const uint32_t firstVertex = segmentIndex * 6;
            fillVerts[firstVertex + 0].xyz = innerPoints[segmentIndex];
            fillVerts[firstVertex + 1].xyz = outerPoints[segmentIndex];
            fillVerts[firstVertex + 2].xyz = outerPoints[nextIndex];
            fillVerts[firstVertex + 3].xyz = innerPoints[segmentIndex];
            fillVerts[firstVertex + 4].xyz = outerPoints[nextIndex];
            fillVerts[firstVertex + 5].xyz = innerPoints[nextIndex];
        }
    }

    DebugDrawVertex *lineVerts = ReserveDebugLineVertices(DebugCircleSegments * 4, depthTest, lifeTime);
    if (lineVerts) {
        for (uint32_t segmentIndex = 0; segmentIndex < DebugCircleSegments; ++segmentIndex) {
            const uint32_t nextIndex = (segmentIndex + 1) % DebugCircleSegments;
            const uint32_t firstVertex = segmentIndex * 4;
            lineVerts[firstVertex + 0].xyz = innerPoints[segmentIndex];
            lineVerts[firstVertex + 1].xyz = innerPoints[nextIndex];
            lineVerts[firstVertex + 2].xyz = outerPoints[segmentIndex];
            lineVerts[firstVertex + 3].xyz = outerPoints[nextIndex];
        }
    }
}

void RenderWorld::DrawDebugArc(const BE1::Vec3 &origin, const BE1::Vec3 &right, const BE1::Vec3 &up, const float radius, float angle1, float angle2, bool drawSector, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    assert(std::isfinite(origin.x) && std::isfinite(origin.y) && std::isfinite(origin.z));
    assert(std::isfinite(right.x) && std::isfinite(right.y) && std::isfinite(right.z));
    assert(std::isfinite(up.x) && std::isfinite(up.y) && std::isfinite(up.z));
    assert(std::isfinite(radius));
    assert(std::isfinite(angle1));
    assert(std::isfinite(angle2));

    const float deltaAngle = angle2 - angle1;
    if (radius <= 0.0f || deltaAngle == 0.0f) {
        return;
    }

    uint32_t numSegments = static_cast<uint32_t>(BE1::Math::Ceil(BE1::Math::Fabs(deltaAngle) / 7.5f));
    numSegments = numSegments < 1 ? 1 : (numSegments > 96 ? 96 : numSegments);

    const float startAngle = angle1 * (BE1::Math::Pi / 180.0f);
    const float deltaRadians = deltaAngle * (BE1::Math::Pi / 180.0f);
    const BE1::Vec3 xRadius = right * radius;
    const BE1::Vec3 yRadius = up * radius;

    BE1::Vec3 points[97];
    for (uint32_t pointIndex = 0; pointIndex <= numSegments; ++pointIndex) {
        float sinAngle;
        float cosAngle;
        const float angle = startAngle + deltaRadians * static_cast<float>(pointIndex) / static_cast<float>(numSegments);
        BE1::Math::SinCos(angle, sinAngle, cosAngle);
        points[pointIndex] = origin + xRadius * cosAngle + yRadius * sinAngle;
    }

    if (drawSector) {
        DebugDrawVertex *fillVerts = ReserveDebugFillVertices(numSegments * 3, twoSided, depthTest, lifeTime);
        if (fillVerts) {
            for (uint32_t segmentIndex = 0; segmentIndex < numSegments; ++segmentIndex) {
                fillVerts[segmentIndex * 3 + 0].xyz = origin;
                fillVerts[segmentIndex * 3 + 1].xyz = points[segmentIndex];
                fillVerts[segmentIndex * 3 + 2].xyz = points[segmentIndex + 1];
            }
        }
    }

    const uint32_t numLineVertices = numSegments * 2 + (drawSector ? 4 : 0);
    DebugDrawVertex *lineVerts = ReserveDebugLineVertices(numLineVertices, depthTest, lifeTime);
    if (lineVerts) {
        uint32_t vertexIndex = 0;
        if (drawSector) {
            lineVerts[vertexIndex++].xyz = origin;
            lineVerts[vertexIndex++].xyz = points[0];
        }
        for (uint32_t segmentIndex = 0; segmentIndex < numSegments; ++segmentIndex) {
            lineVerts[vertexIndex++].xyz = points[segmentIndex];
            lineVerts[vertexIndex++].xyz = points[segmentIndex + 1];
        }
        if (drawSector) {
            lineVerts[vertexIndex++].xyz = points[numSegments];
            lineVerts[vertexIndex++].xyz = origin;
        }
        assert(vertexIndex == numLineVertices);
    }
}

void RenderWorld::DrawDebugEllipse(const BE1::Vec3 &origin, const BE1::Vec3 &right, const BE1::Vec3 &up, const float radius1, const float radius2, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    assert(std::isfinite(origin.x) && std::isfinite(origin.y) && std::isfinite(origin.z));
    assert(std::isfinite(right.x) && std::isfinite(right.y) && std::isfinite(right.z));
    assert(std::isfinite(up.x) && std::isfinite(up.y) && std::isfinite(up.z));
    assert(std::isfinite(radius1));
    assert(std::isfinite(radius2));

    if (radius1 <= 0.0f || radius2 <= 0.0f) {
        return;
    }

    BE1::Vec3 points[DebugEllipseSegments];
    BuildDebugRing(points, DebugEllipseSegments, origin, right * radius1, up * radius2);

    DebugDrawVertex *fillVerts = ReserveDebugFillVertices(DebugEllipseSegments * 3, twoSided, depthTest, lifeTime);
    if (fillVerts) {
        for (uint32_t segmentIndex = 0; segmentIndex < DebugEllipseSegments; ++segmentIndex) {
            const uint32_t nextIndex = (segmentIndex + 1) % DebugEllipseSegments;
            fillVerts[segmentIndex * 3 + 0].xyz = origin;
            fillVerts[segmentIndex * 3 + 1].xyz = points[segmentIndex];
            fillVerts[segmentIndex * 3 + 2].xyz = points[nextIndex];
        }
    }

    DebugDrawVertex *lineVerts = ReserveDebugLineVertices(DebugEllipseSegments * 2, depthTest, lifeTime);
    if (lineVerts) {
        for (uint32_t segmentIndex = 0; segmentIndex < DebugEllipseSegments; ++segmentIndex) {
            const uint32_t nextIndex = (segmentIndex + 1) % DebugEllipseSegments;
            lineVerts[segmentIndex * 2 + 0].xyz = points[segmentIndex];
            lineVerts[segmentIndex * 2 + 1].xyz = points[nextIndex];
        }
    }
}

void RenderWorld::DrawDebugHemisphere(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float radius, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    assert(std::isfinite(origin.x) && std::isfinite(origin.y) && std::isfinite(origin.z));
    assert(std::isfinite(axis[0].x) && std::isfinite(axis[0].y) && std::isfinite(axis[0].z));
    assert(std::isfinite(axis[1].x) && std::isfinite(axis[1].y) && std::isfinite(axis[1].z));
    assert(std::isfinite(axis[2].x) && std::isfinite(axis[2].y) && std::isfinite(axis[2].z));
    assert(std::isfinite(radius));

    if (radius <= 0.0f) {
        return;
    }

    BE1::Vec3 rings[DebugHemisphereRings][DebugVolumeSegments];
    for (uint32_t ringIndex = 0; ringIndex < DebugHemisphereRings; ++ringIndex) {
        float sinTheta;
        float cosTheta;
        const float theta = BE1::Math::Pi * static_cast<float>(ringIndex + 1) / 12.0f;
        BE1::Math::SinCos(theta, sinTheta, cosTheta);
        BuildDebugRing(rings[ringIndex], DebugVolumeSegments,
            origin + axis[2] * (cosTheta * radius),
            axis[0] * (sinTheta * radius),
            axis[1] * (sinTheta * radius));
    }

    const BE1::Vec3 top = origin + axis[2] * radius;
    constexpr uint32_t numFillVertices = DebugVolumeSegments * 3 + (DebugHemisphereRings - 1) * DebugVolumeSegments * 6;
    DebugDrawVertex *fillVerts = ReserveDebugFillVertices(numFillVertices, twoSided, depthTest, lifeTime);
    if (fillVerts) {
        uint32_t vertexIndex = 0;
        for (uint32_t segmentIndex = 0; segmentIndex < DebugVolumeSegments; ++segmentIndex) {
            const uint32_t nextIndex = (segmentIndex + 1) % DebugVolumeSegments;
            fillVerts[vertexIndex++].xyz = top;
            fillVerts[vertexIndex++].xyz = rings[0][segmentIndex];
            fillVerts[vertexIndex++].xyz = rings[0][nextIndex];
        }

        for (uint32_t ringIndex = 0; ringIndex + 1 < DebugHemisphereRings; ++ringIndex) {
            for (uint32_t segmentIndex = 0; segmentIndex < DebugVolumeSegments; ++segmentIndex) {
                const uint32_t nextIndex = (segmentIndex + 1) % DebugVolumeSegments;
                fillVerts[vertexIndex++].xyz = rings[ringIndex][segmentIndex];
                fillVerts[vertexIndex++].xyz = rings[ringIndex + 1][segmentIndex];
                fillVerts[vertexIndex++].xyz = rings[ringIndex][nextIndex];
                fillVerts[vertexIndex++].xyz = rings[ringIndex][nextIndex];
                fillVerts[vertexIndex++].xyz = rings[ringIndex + 1][segmentIndex];
                fillVerts[vertexIndex++].xyz = rings[ringIndex + 1][nextIndex];
            }
        }
        assert(vertexIndex == numFillVertices);
    }

    constexpr uint32_t numLineVertices = DebugHemisphereRings * DebugVolumeSegments * 2 + DebugHemisphereRings * DebugVolumeSegments * 2;
    DebugDrawVertex *lineVerts = ReserveDebugLineVertices(numLineVertices, depthTest, lifeTime);
    if (lineVerts) {
        uint32_t vertexIndex = 0;
        for (uint32_t ringIndex = 0; ringIndex < DebugHemisphereRings; ++ringIndex) {
            for (uint32_t segmentIndex = 0; segmentIndex < DebugVolumeSegments; ++segmentIndex) {
                const uint32_t nextIndex = (segmentIndex + 1) % DebugVolumeSegments;
                lineVerts[vertexIndex++].xyz = rings[ringIndex][segmentIndex];
                lineVerts[vertexIndex++].xyz = rings[ringIndex][nextIndex];
            }
        }
        for (uint32_t segmentIndex = 0; segmentIndex < DebugVolumeSegments; ++segmentIndex) {
            lineVerts[vertexIndex++].xyz = top;
            lineVerts[vertexIndex++].xyz = rings[0][segmentIndex];
            for (uint32_t ringIndex = 0; ringIndex + 1 < DebugHemisphereRings; ++ringIndex) {
                lineVerts[vertexIndex++].xyz = rings[ringIndex][segmentIndex];
                lineVerts[vertexIndex++].xyz = rings[ringIndex + 1][segmentIndex];
            }
        }
        assert(vertexIndex == numLineVertices);
    }
}

void RenderWorld::DrawDebugHemisphereSimple(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float radius, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    assert(std::isfinite(origin.x) && std::isfinite(origin.y) && std::isfinite(origin.z));
    assert(std::isfinite(axis[0].x) && std::isfinite(axis[0].y) && std::isfinite(axis[0].z));
    assert(std::isfinite(axis[1].x) && std::isfinite(axis[1].y) && std::isfinite(axis[1].z));
    assert(std::isfinite(axis[2].x) && std::isfinite(axis[2].y) && std::isfinite(axis[2].z));
    assert(std::isfinite(radius));

    if (radius <= 0.0f) {
        return;
    }

    BE1::Vec3 baseRing[DebugCircleSegments];
    BuildDebugRing(baseRing, DebugCircleSegments, origin, axis[0] * radius, axis[1] * radius);

    constexpr uint32_t arcSegments = 24;
    constexpr uint32_t numLineVertices = DebugCircleSegments * 2 + arcSegments * 4;
    DebugDrawVertex *lineVerts = ReserveDebugLineVertices(numLineVertices, depthTest, lifeTime);
    if (lineVerts) {
        uint32_t vertexIndex = 0;
        for (uint32_t segmentIndex = 0; segmentIndex < DebugCircleSegments; ++segmentIndex) {
            const uint32_t nextIndex = (segmentIndex + 1) % DebugCircleSegments;
            lineVerts[vertexIndex++].xyz = baseRing[segmentIndex];
            lineVerts[vertexIndex++].xyz = baseRing[nextIndex];
        }

        for (uint32_t arcIndex = 0; arcIndex < 2; ++arcIndex) {
            const BE1::Vec3 &right = axis[arcIndex];
            BE1::Vec3 previousPoint = origin + right * radius;
            for (uint32_t segmentIndex = 1; segmentIndex <= arcSegments; ++segmentIndex) {
                float sinAngle;
                float cosAngle;
                const float angle = BE1::Math::Pi * static_cast<float>(segmentIndex) / static_cast<float>(arcSegments);
                BE1::Math::SinCos(angle, sinAngle, cosAngle);
                const BE1::Vec3 point = origin + right * (cosAngle * radius) + axis[2] * (sinAngle * radius);
                lineVerts[vertexIndex++].xyz = previousPoint;
                lineVerts[vertexIndex++].xyz = point;
                previousPoint = point;
            }
        }
        assert(vertexIndex == numLineVertices);
    }
}

void RenderWorld::DrawDebugSphere(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float radius, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    assert(std::isfinite(origin.x) && std::isfinite(origin.y) && std::isfinite(origin.z));
    assert(std::isfinite(axis[0].x) && std::isfinite(axis[0].y) && std::isfinite(axis[0].z));
    assert(std::isfinite(axis[1].x) && std::isfinite(axis[1].y) && std::isfinite(axis[1].z));
    assert(std::isfinite(axis[2].x) && std::isfinite(axis[2].y) && std::isfinite(axis[2].z));
    assert(std::isfinite(radius));

    if (radius <= 0.0f) {
        return;
    }

    BE1::Vec3 rings[DebugSphereRings][DebugVolumeSegments];
    for (uint32_t ringIndex = 0; ringIndex < DebugSphereRings; ++ringIndex) {
        float sinTheta;
        float cosTheta;
        const float theta = BE1::Math::Pi * static_cast<float>(ringIndex + 1) / 12.0f;
        BE1::Math::SinCos(theta, sinTheta, cosTheta);
        BuildDebugRing(rings[ringIndex], DebugVolumeSegments,
            origin + axis[2] * (cosTheta * radius),
            axis[0] * (sinTheta * radius),
            axis[1] * (sinTheta * radius));
    }

    const BE1::Vec3 top = origin + axis[2] * radius;
    const BE1::Vec3 bottom = origin - axis[2] * radius;
    constexpr uint32_t numFillVertices = DebugVolumeSegments * 6 + (DebugSphereRings - 1) * DebugVolumeSegments * 6;
    DebugDrawVertex *fillVerts = ReserveDebugFillVertices(numFillVertices, twoSided, depthTest, lifeTime);
    if (fillVerts) {
        uint32_t vertexIndex = 0;
        for (uint32_t segmentIndex = 0; segmentIndex < DebugVolumeSegments; ++segmentIndex) {
            const uint32_t nextIndex = (segmentIndex + 1) % DebugVolumeSegments;
            fillVerts[vertexIndex++].xyz = top;
            fillVerts[vertexIndex++].xyz = rings[0][segmentIndex];
            fillVerts[vertexIndex++].xyz = rings[0][nextIndex];
        }

        for (uint32_t ringIndex = 0; ringIndex + 1 < DebugSphereRings; ++ringIndex) {
            for (uint32_t segmentIndex = 0; segmentIndex < DebugVolumeSegments; ++segmentIndex) {
                const uint32_t nextIndex = (segmentIndex + 1) % DebugVolumeSegments;
                fillVerts[vertexIndex++].xyz = rings[ringIndex][segmentIndex];
                fillVerts[vertexIndex++].xyz = rings[ringIndex + 1][segmentIndex];
                fillVerts[vertexIndex++].xyz = rings[ringIndex][nextIndex];
                fillVerts[vertexIndex++].xyz = rings[ringIndex][nextIndex];
                fillVerts[vertexIndex++].xyz = rings[ringIndex + 1][segmentIndex];
                fillVerts[vertexIndex++].xyz = rings[ringIndex + 1][nextIndex];
            }
        }

        for (uint32_t segmentIndex = 0; segmentIndex < DebugVolumeSegments; ++segmentIndex) {
            const uint32_t nextIndex = (segmentIndex + 1) % DebugVolumeSegments;
            fillVerts[vertexIndex++].xyz = bottom;
            fillVerts[vertexIndex++].xyz = rings[DebugSphereRings - 1][nextIndex];
            fillVerts[vertexIndex++].xyz = rings[DebugSphereRings - 1][segmentIndex];
        }
        assert(vertexIndex == numFillVertices);
    }

    constexpr uint32_t numLineVertices = DebugSphereRings * DebugVolumeSegments * 2 + (DebugSphereRings + 1) * DebugVolumeSegments * 2;
    DebugDrawVertex *lineVerts = ReserveDebugLineVertices(numLineVertices, depthTest, lifeTime);
    if (lineVerts) {
        uint32_t vertexIndex = 0;
        for (uint32_t ringIndex = 0; ringIndex < DebugSphereRings; ++ringIndex) {
            for (uint32_t segmentIndex = 0; segmentIndex < DebugVolumeSegments; ++segmentIndex) {
                const uint32_t nextIndex = (segmentIndex + 1) % DebugVolumeSegments;
                lineVerts[vertexIndex++].xyz = rings[ringIndex][segmentIndex];
                lineVerts[vertexIndex++].xyz = rings[ringIndex][nextIndex];
            }
        }
        for (uint32_t segmentIndex = 0; segmentIndex < DebugVolumeSegments; ++segmentIndex) {
            lineVerts[vertexIndex++].xyz = top;
            lineVerts[vertexIndex++].xyz = rings[0][segmentIndex];
            for (uint32_t ringIndex = 0; ringIndex + 1 < DebugSphereRings; ++ringIndex) {
                lineVerts[vertexIndex++].xyz = rings[ringIndex][segmentIndex];
                lineVerts[vertexIndex++].xyz = rings[ringIndex + 1][segmentIndex];
            }
            lineVerts[vertexIndex++].xyz = rings[DebugSphereRings - 1][segmentIndex];
            lineVerts[vertexIndex++].xyz = bottom;
        }
        assert(vertexIndex == numLineVertices);
    }
}

void RenderWorld::DrawDebugSphereSimple(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float radius, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    assert(std::isfinite(origin.x) && std::isfinite(origin.y) && std::isfinite(origin.z));
    assert(std::isfinite(axis[0].x) && std::isfinite(axis[0].y) && std::isfinite(axis[0].z));
    assert(std::isfinite(axis[1].x) && std::isfinite(axis[1].y) && std::isfinite(axis[1].z));
    assert(std::isfinite(axis[2].x) && std::isfinite(axis[2].y) && std::isfinite(axis[2].z));
    assert(std::isfinite(radius));

    if (radius <= 0.0f) {
        return;
    }

    BE1::Vec3 rings[3][DebugCircleSegments];
    BuildDebugRing(rings[0], DebugCircleSegments, origin, axis[1] * radius, axis[2] * radius);
    BuildDebugRing(rings[1], DebugCircleSegments, origin, axis[2] * radius, axis[0] * radius);
    BuildDebugRing(rings[2], DebugCircleSegments, origin, axis[0] * radius, axis[1] * radius);

    DebugDrawVertex *lineVerts = ReserveDebugLineVertices(3 * DebugCircleSegments * 2, depthTest, lifeTime);
    if (lineVerts) {
        uint32_t vertexIndex = 0;
        for (uint32_t ringIndex = 0; ringIndex < 3; ++ringIndex) {
            for (uint32_t segmentIndex = 0; segmentIndex < DebugCircleSegments; ++segmentIndex) {
                const uint32_t nextIndex = (segmentIndex + 1) % DebugCircleSegments;
                lineVerts[vertexIndex++].xyz = rings[ringIndex][segmentIndex];
                lineVerts[vertexIndex++].xyz = rings[ringIndex][nextIndex];
            }
        }
    }
}

void RenderWorld::DrawDebugAABB(const BE1::AABB &aabb, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    if (aabb.IsCleared()) {
        return;
    }

    BE1::Vec3 points[8];
    aabb.ToPoints(points);

    DebugDrawVertex *fillVerts = ReserveDebugFillVertices(COUNT_OF(DebugBoxTriangleIndices), twoSided, depthTest, lifeTime);
    if (fillVerts) {
        for (uint32_t vertexIndex = 0; vertexIndex < COUNT_OF(DebugBoxTriangleIndices); ++vertexIndex) {
            fillVerts[vertexIndex].xyz = points[DebugBoxTriangleIndices[vertexIndex]];
        }
    }

    DebugDrawVertex *lineVerts = ReserveDebugLineVertices(COUNT_OF(DebugBoxEdgeIndices), depthTest, lifeTime);
    if (lineVerts) {
        for (uint32_t vertexIndex = 0; vertexIndex < COUNT_OF(DebugBoxEdgeIndices); ++vertexIndex) {
            lineVerts[vertexIndex].xyz = points[DebugBoxEdgeIndices[vertexIndex]];
        }
    }
}

void RenderWorld::DrawDebugOBB(const BE1::OBB &obb, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    if (obb.IsCleared()) {
        return;
    }

    BE1::Vec3 points[8];
    obb.ToPoints(points);

    DebugDrawVertex *fillVerts = ReserveDebugFillVertices(COUNT_OF(DebugBoxTriangleIndices), twoSided, depthTest, lifeTime);
    if (fillVerts) {
        for (uint32_t vertexIndex = 0; vertexIndex < COUNT_OF(DebugBoxTriangleIndices); ++vertexIndex) {
            fillVerts[vertexIndex].xyz = points[DebugBoxTriangleIndices[vertexIndex]];
        }
    }

    DebugDrawVertex *lineVerts = ReserveDebugLineVertices(COUNT_OF(DebugBoxEdgeIndices), depthTest, lifeTime);
    if (lineVerts) {
        for (uint32_t vertexIndex = 0; vertexIndex < COUNT_OF(DebugBoxEdgeIndices); ++vertexIndex) {
            lineVerts[vertexIndex].xyz = points[DebugBoxEdgeIndices[vertexIndex]];
        }
    }
}

void RenderWorld::DrawDebugFrustum(const BE1::Frustum &frustum, bool showFromOrigin, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    BE1::Vec3 points[8];
    frustum.ToPoints(points);

    DebugDrawVertex *fillVerts = ReserveDebugFillVertices(COUNT_OF(DebugBoxTriangleIndices), twoSided, depthTest, lifeTime);
    if (fillVerts) {
        for (uint32_t vertexIndex = 0; vertexIndex < COUNT_OF(DebugBoxTriangleIndices); ++vertexIndex) {
            fillVerts[vertexIndex].xyz = points[DebugBoxTriangleIndices[vertexIndex]];
        }
    }

    const bool drawNearPlane = frustum.GetNearDistance() > 0.0f;
    const uint32_t numEdges = 8 + (drawNearPlane ? 4 : 0) + (drawNearPlane && showFromOrigin ? 4 : 0);
    DebugDrawVertex *lineVerts = ReserveDebugLineVertices(numEdges * 2, depthTest, lifeTime);
    if (lineVerts) {
        uint32_t vertexIndex = 0;
        if (drawNearPlane) {
            for (uint32_t cornerIndex = 0; cornerIndex < 4; ++cornerIndex) {
                lineVerts[vertexIndex++].xyz = points[cornerIndex];
                lineVerts[vertexIndex++].xyz = points[(cornerIndex + 1) & 3];
            }

            if (showFromOrigin) {
                for (uint32_t cornerIndex = 0; cornerIndex < 4; ++cornerIndex) {
                    lineVerts[vertexIndex++].xyz = frustum.GetOrigin();
                    lineVerts[vertexIndex++].xyz = points[cornerIndex];
                }
            }
        }

        for (uint32_t cornerIndex = 0; cornerIndex < 4; ++cornerIndex) {
            lineVerts[vertexIndex++].xyz = points[4 + cornerIndex];
            lineVerts[vertexIndex++].xyz = points[4 + ((cornerIndex + 1) & 3)];
            lineVerts[vertexIndex++].xyz = points[cornerIndex];
            lineVerts[vertexIndex++].xyz = points[4 + cornerIndex];
        }
        assert(vertexIndex == numEdges * 2);
    }
}

void RenderWorld::DrawDebugCone(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float height, float radius1, float radius2, bool drawCap, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    assert(std::isfinite(origin.x) && std::isfinite(origin.y) && std::isfinite(origin.z));
    assert(std::isfinite(axis[0].x) && std::isfinite(axis[0].y) && std::isfinite(axis[0].z));
    assert(std::isfinite(axis[1].x) && std::isfinite(axis[1].y) && std::isfinite(axis[1].z));
    assert(std::isfinite(axis[2].x) && std::isfinite(axis[2].y) && std::isfinite(axis[2].z));
    assert(std::isfinite(height));
    assert(std::isfinite(radius1));
    assert(std::isfinite(radius2));

    if (height <= 0.0f || radius1 < 0.0f || radius2 < 0.0f || (radius1 == 0.0f && radius2 == 0.0f)) {
        return;
    }

    const BE1::Vec3 topCenter = origin + axis[2] * height;
    BE1::Vec3 topRing[DebugVolumeSegments];
    BE1::Vec3 bottomRing[DebugVolumeSegments];
    if (radius1 > 0.0f) {
        BuildDebugRing(topRing, DebugVolumeSegments, topCenter, axis[0] * radius1, axis[1] * radius1);
    }
    if (radius2 > 0.0f) {
        BuildDebugRing(bottomRing, DebugVolumeSegments, origin, axis[0] * radius2, axis[1] * radius2);
    }

    uint32_t numFillVertices = radius1 > 0.0f && radius2 > 0.0f ? DebugVolumeSegments * 6 : DebugVolumeSegments * 3;
    if (drawCap) {
        numFillVertices += (radius1 > 0.0f ? DebugVolumeSegments * 3 : 0);
        numFillVertices += (radius2 > 0.0f ? DebugVolumeSegments * 3 : 0);
    }

    DebugDrawVertex *fillVerts = ReserveDebugFillVertices(numFillVertices, twoSided, depthTest, lifeTime);
    if (fillVerts) {
        uint32_t vertexIndex = 0;
        for (uint32_t segmentIndex = 0; segmentIndex < DebugVolumeSegments; ++segmentIndex) {
            const uint32_t nextIndex = (segmentIndex + 1) % DebugVolumeSegments;
            if (radius1 == 0.0f) {
                fillVerts[vertexIndex++].xyz = topCenter;
                fillVerts[vertexIndex++].xyz = bottomRing[segmentIndex];
                fillVerts[vertexIndex++].xyz = bottomRing[nextIndex];
            } else if (radius2 == 0.0f) {
                fillVerts[vertexIndex++].xyz = origin;
                fillVerts[vertexIndex++].xyz = topRing[nextIndex];
                fillVerts[vertexIndex++].xyz = topRing[segmentIndex];
            } else {
                fillVerts[vertexIndex++].xyz = topRing[segmentIndex];
                fillVerts[vertexIndex++].xyz = bottomRing[segmentIndex];
                fillVerts[vertexIndex++].xyz = topRing[nextIndex];
                fillVerts[vertexIndex++].xyz = topRing[nextIndex];
                fillVerts[vertexIndex++].xyz = bottomRing[segmentIndex];
                fillVerts[vertexIndex++].xyz = bottomRing[nextIndex];
            }
        }

        if (drawCap && radius1 > 0.0f) {
            for (uint32_t segmentIndex = 0; segmentIndex < DebugVolumeSegments; ++segmentIndex) {
                const uint32_t nextIndex = (segmentIndex + 1) % DebugVolumeSegments;
                fillVerts[vertexIndex++].xyz = topCenter;
                fillVerts[vertexIndex++].xyz = topRing[segmentIndex];
                fillVerts[vertexIndex++].xyz = topRing[nextIndex];
            }
        }
        if (drawCap && radius2 > 0.0f) {
            for (uint32_t segmentIndex = 0; segmentIndex < DebugVolumeSegments; ++segmentIndex) {
                const uint32_t nextIndex = (segmentIndex + 1) % DebugVolumeSegments;
                fillVerts[vertexIndex++].xyz = origin;
                fillVerts[vertexIndex++].xyz = bottomRing[nextIndex];
                fillVerts[vertexIndex++].xyz = bottomRing[segmentIndex];
            }
        }
        assert(vertexIndex == numFillVertices);
    }

    const uint32_t numLineEdges = DebugVolumeSegments + (radius1 > 0.0f ? DebugVolumeSegments : 0) + (radius2 > 0.0f ? DebugVolumeSegments : 0);
    DebugDrawVertex *lineVerts = ReserveDebugLineVertices(numLineEdges * 2, depthTest, lifeTime);
    if (lineVerts) {
        uint32_t vertexIndex = 0;
        for (uint32_t segmentIndex = 0; segmentIndex < DebugVolumeSegments; ++segmentIndex) {
            const uint32_t nextIndex = (segmentIndex + 1) % DebugVolumeSegments;
            if (radius1 > 0.0f) {
                lineVerts[vertexIndex++].xyz = topRing[segmentIndex];
                lineVerts[vertexIndex++].xyz = topRing[nextIndex];
            }
            if (radius2 > 0.0f) {
                lineVerts[vertexIndex++].xyz = bottomRing[segmentIndex];
                lineVerts[vertexIndex++].xyz = bottomRing[nextIndex];
            }
            lineVerts[vertexIndex++].xyz = radius1 > 0.0f ? topRing[segmentIndex] : topCenter;
            lineVerts[vertexIndex++].xyz = radius2 > 0.0f ? bottomRing[segmentIndex] : origin;
        }
        assert(vertexIndex == numLineEdges * 2);
    }
}

void RenderWorld::DrawDebugCone(const BE1::Vec3 &apex, const BE1::Vec3 &bottom, float radius1, float radius2, bool drawCap, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    assert(std::isfinite(apex.x) && std::isfinite(apex.y) && std::isfinite(apex.z));
    assert(std::isfinite(bottom.x) && std::isfinite(bottom.y) && std::isfinite(bottom.z));

    BE1::Mat3 axis;
    axis[2] = apex - bottom;
    if (axis[2].LengthSqr() <= DebugDirectionEpsilonSqr) {
        return;
    }
    float height = axis[2].Normalize();
    axis[2].OrthogonalBasis(axis[0], axis[1]);

    DrawDebugCone(bottom, axis, height, radius1, radius2, drawCap, twoSided, depthTest, lifeTime);
}

void RenderWorld::DrawDebugConeSimple(const BE1::Vec3 &origin, const BE1::Mat3 &axis, float height, float radius1, float radius2, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    assert(std::isfinite(origin.x) && std::isfinite(origin.y) && std::isfinite(origin.z));
    assert(std::isfinite(axis[0].x) && std::isfinite(axis[0].y) && std::isfinite(axis[0].z));
    assert(std::isfinite(axis[1].x) && std::isfinite(axis[1].y) && std::isfinite(axis[1].z));
    assert(std::isfinite(axis[2].x) && std::isfinite(axis[2].y) && std::isfinite(axis[2].z));
    assert(std::isfinite(height));
    assert(std::isfinite(radius1));
    assert(std::isfinite(radius2));

    if (height <= 0.0f || radius1 < 0.0f || radius2 < 0.0f || (radius1 == 0.0f && radius2 == 0.0f)) {
        return;
    }

    const BE1::Vec3 topCenter = origin + axis[2] * height;
    BE1::Vec3 topRing[DebugCircleSegments];
    BE1::Vec3 bottomRing[DebugCircleSegments];
    if (radius1 > 0.0f) {
        BuildDebugRing(topRing, DebugCircleSegments, topCenter, axis[0] * radius1, axis[1] * radius1);
    }
    if (radius2 > 0.0f) {
        BuildDebugRing(bottomRing, DebugCircleSegments, origin, axis[0] * radius2, axis[1] * radius2);
    }

    const uint32_t numLineEdges = (radius1 > 0.0f ? DebugCircleSegments : 0) +
        (radius2 > 0.0f ? DebugCircleSegments : 0) + 4;
    DebugDrawVertex *lineVerts = ReserveDebugLineVertices(numLineEdges * 2, depthTest, lifeTime);
    if (lineVerts) {
        uint32_t vertexIndex = 0;
        for (uint32_t segmentIndex = 0; segmentIndex < DebugCircleSegments; ++segmentIndex) {
            const uint32_t nextIndex = (segmentIndex + 1) % DebugCircleSegments;
            if (radius1 > 0.0f) {
                lineVerts[vertexIndex++].xyz = topRing[segmentIndex];
                lineVerts[vertexIndex++].xyz = topRing[nextIndex];
            }
            if (radius2 > 0.0f) {
                lineVerts[vertexIndex++].xyz = bottomRing[segmentIndex];
                lineVerts[vertexIndex++].xyz = bottomRing[nextIndex];
            }
        }

        constexpr uint32_t generatorIndices[4] = {
            0, DebugCircleSegments / 4, DebugCircleSegments / 2, DebugCircleSegments * 3 / 4
        };
        for (uint32_t generatorIndex : generatorIndices) {
            lineVerts[vertexIndex++].xyz = radius2 > 0.0f ? bottomRing[generatorIndex] : origin;
            lineVerts[vertexIndex++].xyz = radius1 > 0.0f ? topRing[generatorIndex] : topCenter;
        }
        assert(vertexIndex == numLineEdges * 2);
    }
}

void RenderWorld::DrawDebugCylinder(const BE1::Vec3 &center, const BE1::Mat3 &axis, float height, float radius, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    BE1::Vec3 origin = center - axis[2] * height * 0.5f;

    DrawDebugCone(origin, axis, height, radius, radius, true, twoSided, depthTest, lifeTime);
}

void RenderWorld::DrawDebugCylinder(const BE1::Vec3 &top, const BE1::Vec3 &bottom, float radius, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    DrawDebugCone(top, bottom, radius, radius, true, twoSided, depthTest, lifeTime);
}

void RenderWorld::DrawDebugCylinderSimple(const BE1::Vec3 &center, const BE1::Mat3 &axis, float height, float radius, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    const BE1::Vec3 origin = center - axis[2] * height * 0.5f;
    DrawDebugConeSimple(origin, axis, height, radius, radius, depthTest, lifeTime);
}

void RenderWorld::DrawDebugCapsule(const BE1::Vec3 &center, const BE1::Mat3 &axis, float height, float radius, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    assert(std::isfinite(center.x) && std::isfinite(center.y) && std::isfinite(center.z));
    assert(std::isfinite(axis[0].x) && std::isfinite(axis[0].y) && std::isfinite(axis[0].z));
    assert(std::isfinite(axis[1].x) && std::isfinite(axis[1].y) && std::isfinite(axis[1].z));
    assert(std::isfinite(axis[2].x) && std::isfinite(axis[2].y) && std::isfinite(axis[2].z));
    assert(std::isfinite(height));
    assert(std::isfinite(radius));

    if (height < 0.0f || radius <= 0.0f) {
        return;
    }
    if (height <= DebugDirectionEpsilonSqr) {
        DrawDebugSphere(center, axis, radius, twoSided, depthTest, lifeTime);
        return;
    }

    constexpr uint32_t numProfileRings = DebugHemisphereRings * 2;
    BE1::Vec3 rings[numProfileRings][DebugVolumeSegments];
    const BE1::Vec3 topCenter = center + axis[2] * (height * 0.5f);
    const BE1::Vec3 bottomCenter = center - axis[2] * (height * 0.5f);

    for (uint32_t ringIndex = 0; ringIndex < DebugHemisphereRings; ++ringIndex) {
        float sinTheta;
        float cosTheta;
        const float theta = BE1::Math::Pi * static_cast<float>(ringIndex + 1) / 12.0f;
        BE1::Math::SinCos(theta, sinTheta, cosTheta);
        BuildDebugRing(rings[ringIndex], DebugVolumeSegments,
            topCenter + axis[2] * (cosTheta * radius),
            axis[0] * (sinTheta * radius),
            axis[1] * (sinTheta * radius));
    }
    for (uint32_t ringIndex = 0; ringIndex < DebugHemisphereRings; ++ringIndex) {
        float sinTheta;
        float cosTheta;
        const float theta = BE1::Math::Pi * (6.0f + static_cast<float>(ringIndex)) / 12.0f;
        BE1::Math::SinCos(theta, sinTheta, cosTheta);
        BuildDebugRing(rings[DebugHemisphereRings + ringIndex], DebugVolumeSegments,
            bottomCenter + axis[2] * (cosTheta * radius),
            axis[0] * (sinTheta * radius),
            axis[1] * (sinTheta * radius));
    }

    const BE1::Vec3 top = topCenter + axis[2] * radius;
    const BE1::Vec3 bottom = bottomCenter - axis[2] * radius;
    constexpr uint32_t numFillVertices = DebugVolumeSegments * 6 + (numProfileRings - 1) * DebugVolumeSegments * 6;
    DebugDrawVertex *fillVerts = ReserveDebugFillVertices(numFillVertices, twoSided, depthTest, lifeTime);
    if (fillVerts) {
        uint32_t vertexIndex = 0;
        for (uint32_t segmentIndex = 0; segmentIndex < DebugVolumeSegments; ++segmentIndex) {
            const uint32_t nextIndex = (segmentIndex + 1) % DebugVolumeSegments;
            fillVerts[vertexIndex++].xyz = top;
            fillVerts[vertexIndex++].xyz = rings[0][segmentIndex];
            fillVerts[vertexIndex++].xyz = rings[0][nextIndex];
        }

        for (uint32_t ringIndex = 0; ringIndex + 1 < numProfileRings; ++ringIndex) {
            for (uint32_t segmentIndex = 0; segmentIndex < DebugVolumeSegments; ++segmentIndex) {
                const uint32_t nextIndex = (segmentIndex + 1) % DebugVolumeSegments;
                fillVerts[vertexIndex++].xyz = rings[ringIndex][segmentIndex];
                fillVerts[vertexIndex++].xyz = rings[ringIndex + 1][segmentIndex];
                fillVerts[vertexIndex++].xyz = rings[ringIndex][nextIndex];
                fillVerts[vertexIndex++].xyz = rings[ringIndex][nextIndex];
                fillVerts[vertexIndex++].xyz = rings[ringIndex + 1][segmentIndex];
                fillVerts[vertexIndex++].xyz = rings[ringIndex + 1][nextIndex];
            }
        }

        for (uint32_t segmentIndex = 0; segmentIndex < DebugVolumeSegments; ++segmentIndex) {
            const uint32_t nextIndex = (segmentIndex + 1) % DebugVolumeSegments;
            fillVerts[vertexIndex++].xyz = bottom;
            fillVerts[vertexIndex++].xyz = rings[numProfileRings - 1][nextIndex];
            fillVerts[vertexIndex++].xyz = rings[numProfileRings - 1][segmentIndex];
        }
        assert(vertexIndex == numFillVertices);
    }

    constexpr uint32_t numLineVertices = numProfileRings * DebugVolumeSegments * 2 + (numProfileRings + 1) * DebugVolumeSegments * 2;
    DebugDrawVertex *lineVerts = ReserveDebugLineVertices(numLineVertices, depthTest, lifeTime);
    if (lineVerts) {
        uint32_t vertexIndex = 0;
        for (uint32_t ringIndex = 0; ringIndex < numProfileRings; ++ringIndex) {
            for (uint32_t segmentIndex = 0; segmentIndex < DebugVolumeSegments; ++segmentIndex) {
                const uint32_t nextIndex = (segmentIndex + 1) % DebugVolumeSegments;
                lineVerts[vertexIndex++].xyz = rings[ringIndex][segmentIndex];
                lineVerts[vertexIndex++].xyz = rings[ringIndex][nextIndex];
            }
        }
        for (uint32_t segmentIndex = 0; segmentIndex < DebugVolumeSegments; ++segmentIndex) {
            lineVerts[vertexIndex++].xyz = top;
            lineVerts[vertexIndex++].xyz = rings[0][segmentIndex];
            for (uint32_t ringIndex = 0; ringIndex + 1 < numProfileRings; ++ringIndex) {
                lineVerts[vertexIndex++].xyz = rings[ringIndex][segmentIndex];
                lineVerts[vertexIndex++].xyz = rings[ringIndex + 1][segmentIndex];
            }
            lineVerts[vertexIndex++].xyz = rings[numProfileRings - 1][segmentIndex];
            lineVerts[vertexIndex++].xyz = bottom;
        }
        assert(vertexIndex == numLineVertices);
    }
}

void RenderWorld::DrawDebugCapsule(const BE1::Vec3 &a, const BE1::Vec3 &b, float radius, bool twoSided /*= true*/, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    assert(std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z));
    assert(std::isfinite(b.x) && std::isfinite(b.y) && std::isfinite(b.z));

    BE1::Mat3 axis;
    axis[2] = a - b;
    if (axis[2].LengthSqr() <= DebugDirectionEpsilonSqr) {
        DrawDebugSphere(a, BE1::Mat3::identity, radius, twoSided, depthTest, lifeTime);
        return;
    }
    const float height = axis[2].Normalize();
    axis[2].OrthogonalBasis(axis[0], axis[1]);

    DrawDebugCapsule((a + b) * 0.5f, axis, height, radius, twoSided, depthTest, lifeTime);
}

void RenderWorld::DrawDebugCapsuleSimple(const BE1::Vec3 &center, const BE1::Mat3 &axis, float height, float radius, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    assert(std::isfinite(center.x) && std::isfinite(center.y) && std::isfinite(center.z));
    assert(std::isfinite(axis[0].x) && std::isfinite(axis[0].y) && std::isfinite(axis[0].z));
    assert(std::isfinite(axis[1].x) && std::isfinite(axis[1].y) && std::isfinite(axis[1].z));
    assert(std::isfinite(axis[2].x) && std::isfinite(axis[2].y) && std::isfinite(axis[2].z));
    assert(std::isfinite(height));
    assert(std::isfinite(radius));

    if (height < 0.0f || radius <= 0.0f) {
        return;
    }
    if (height <= DebugDirectionEpsilonSqr) {
        DrawDebugSphereSimple(center, axis, radius, depthTest, lifeTime);
        return;
    }

    const BE1::Vec3 topCenter = center + axis[2] * (height * 0.5f);
    const BE1::Vec3 bottomCenter = center - axis[2] * (height * 0.5f);
    BE1::Vec3 topRing[DebugCircleSegments];
    BE1::Vec3 bottomRing[DebugCircleSegments];
    BuildDebugRing(topRing, DebugCircleSegments, topCenter, axis[0] * radius, axis[1] * radius);
    BuildDebugRing(bottomRing, DebugCircleSegments, bottomCenter, axis[0] * radius, axis[1] * radius);

    constexpr uint32_t arcSegments = 24;
    constexpr uint32_t numLineVertices = 8 + DebugCircleSegments * 4 + arcSegments * 8;
    DebugDrawVertex *lineVerts = ReserveDebugLineVertices(numLineVertices, depthTest, lifeTime);
    if (lineVerts) {
        uint32_t vertexIndex = 0;
        constexpr uint32_t generatorIndices[4] = {
            0, DebugCircleSegments / 4, DebugCircleSegments / 2, DebugCircleSegments * 3 / 4
        };
        for (uint32_t generatorIndex : generatorIndices) {
            lineVerts[vertexIndex++].xyz = topRing[generatorIndex];
            lineVerts[vertexIndex++].xyz = bottomRing[generatorIndex];
        }

        for (uint32_t segmentIndex = 0; segmentIndex < DebugCircleSegments; ++segmentIndex) {
            const uint32_t nextIndex = (segmentIndex + 1) % DebugCircleSegments;
            lineVerts[vertexIndex++].xyz = topRing[segmentIndex];
            lineVerts[vertexIndex++].xyz = topRing[nextIndex];
            lineVerts[vertexIndex++].xyz = bottomRing[segmentIndex];
            lineVerts[vertexIndex++].xyz = bottomRing[nextIndex];
        }

        for (uint32_t hemisphereIndex = 0; hemisphereIndex < 2; ++hemisphereIndex) {
            const BE1::Vec3 &hemisphereCenter = hemisphereIndex == 0 ? topCenter : bottomCenter;
            const BE1::Vec3 hemisphereUp = hemisphereIndex == 0 ? axis[2] : -axis[2];
            for (uint32_t arcIndex = 0; arcIndex < 2; ++arcIndex) {
                const BE1::Vec3 &right = axis[arcIndex];
                BE1::Vec3 previousPoint = hemisphereCenter + right * radius;
                for (uint32_t segmentIndex = 1; segmentIndex <= arcSegments; ++segmentIndex) {
                    float sinAngle;
                    float cosAngle;
                    const float angle = BE1::Math::Pi * static_cast<float>(segmentIndex) / static_cast<float>(arcSegments);
                    BE1::Math::SinCos(angle, sinAngle, cosAngle);
                    const BE1::Vec3 point = hemisphereCenter + right * (cosAngle * radius) + hemisphereUp * (sinAngle * radius);
                    lineVerts[vertexIndex++].xyz = previousPoint;
                    lineVerts[vertexIndex++].xyz = point;
                    previousPoint = point;
                }
            }
        }
        assert(vertexIndex == numLineVertices);
    }
}

void RenderWorld::DrawDebugArrow(const BE1::Vec3 &start, const BE1::Vec3 &end, float coneSize, float coneRadius, bool depthTest /*= false*/, int lifeTime /*= 0*/) {
    assert(std::isfinite(start.x) && std::isfinite(start.y) && std::isfinite(start.z));
    assert(std::isfinite(end.x) && std::isfinite(end.y) && std::isfinite(end.z));
    assert(std::isfinite(coneSize));
    assert(std::isfinite(coneRadius));

    BE1::Vec3 direction = end - start;
    if (direction.LengthSqr() <= DebugDirectionEpsilonSqr) {
        return;
    }
    const float length = direction.Normalize();
    if (coneSize <= 0.0f || coneRadius <= 0.0f) {
        DrawDebugLine(start, end, depthTest, lifeTime);
        return;
    }

    coneSize = coneSize < length ? coneSize : length;
    const BE1::Vec3 coneBottom = end - direction * coneSize;
    BE1::Vec3 left;
    BE1::Vec3 up;
    direction.OrthogonalBasis(left, up);
    BE1::Vec3 ring[DebugVolumeSegments];
    BuildDebugRing(ring, DebugVolumeSegments, coneBottom, left * coneRadius, up * coneRadius);

    DebugDrawVertex *fillVerts = ReserveDebugFillVertices(DebugVolumeSegments * 3, true, depthTest, lifeTime);
    if (fillVerts) {
        for (uint32_t segmentIndex = 0; segmentIndex < DebugVolumeSegments; ++segmentIndex) {
            const uint32_t nextIndex = (segmentIndex + 1) % DebugVolumeSegments;
            fillVerts[segmentIndex * 3 + 0].xyz = end;
            fillVerts[segmentIndex * 3 + 1].xyz = ring[segmentIndex];
            fillVerts[segmentIndex * 3 + 2].xyz = ring[nextIndex];
        }
    }

    const bool drawShaft = coneSize < length;
    constexpr uint32_t numGeneratorEdges = 4;
    constexpr uint32_t numHeadLineEdges = DebugVolumeSegments + numGeneratorEdges * 2;
    const uint32_t numLineVertices = numHeadLineEdges * 2 + (drawShaft ? 2 : 0);
    DebugDrawVertex *lineVerts = ReserveDebugLineVertices(numLineVertices, depthTest, lifeTime);
    if (lineVerts) {
        uint32_t vertexIndex = 0;
        if (drawShaft) {
            lineVerts[vertexIndex++].xyz = start;
            lineVerts[vertexIndex++].xyz = end;
        }
        for (uint32_t segmentIndex = 0; segmentIndex < DebugVolumeSegments; ++segmentIndex) {
            const uint32_t nextIndex = (segmentIndex + 1) % DebugVolumeSegments;
            lineVerts[vertexIndex++].xyz = ring[segmentIndex];
            lineVerts[vertexIndex++].xyz = ring[nextIndex];
        }
        constexpr uint32_t generatorIndices[numGeneratorEdges] = {
            0, DebugVolumeSegments / 4, DebugVolumeSegments / 2, DebugVolumeSegments * 3 / 4
        };
        for (uint32_t generatorIndex : generatorIndices) {
            lineVerts[vertexIndex++].xyz = end;
            lineVerts[vertexIndex++].xyz = ring[generatorIndex];
            lineVerts[vertexIndex++].xyz = ring[generatorIndex];
            lineVerts[vertexIndex++].xyz = coneBottom;
        }
        assert(vertexIndex == numLineVertices);
    }
}
