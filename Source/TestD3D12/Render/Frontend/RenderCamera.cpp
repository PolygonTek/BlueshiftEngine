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
#include "RenderCamera.h"
#include "Render/RenderInternal.h"
#include "Render/RenderUtils.h"
#include "Math/Halton.h"
#include "RHI/RHI.h"
#include "Shaders/ShaderInterop.h"

RenderCamera::~RenderCamera() {
    DestroyPostProcessResources();
    DestroySSRResources();
    DestroyTransparentSSRResources();
    screenSpaceAOResources.Destroy();
    ssrSceneColorHistoryResources.Destroy();
    ssrHiZResources.Destroy();
    hiZResources.Destroy();

    DestroyResolvedDepthTexture();
    DestroyRenderTextures();
}

void RenderCamera::Update(const Desc &desc) {
    this->desc = desc;

    const bool reverseZ = desc.projectionPolicy.reverseZ;

    SetViewMatrixFromEngineAxis(desc.axis, desc.origin, viewMatrix);

    if (desc.orthogonal) {
        // Set bounding volume for orthogonal view.
        BE1::Vec3 extents((desc.zFar - desc.zNear) * 0.5f, desc.sizeX, desc.sizeY);
        box.SetCenter(desc.origin + desc.axis[0] * (desc.zNear + extents[0]));
        box.SetExtents(extents);
        box.SetAxis(desc.axis);

        // Calculate view volume planes.
        box.ToPlanes(viewPlanes);

        // 직교 투영 행렬을 계산한다. viewMatrix 가 오른손 좌표계이므로, 오른손 좌표계용 투영 행렬을 계산한다.
        projMatrix.SetOrthoRH(-desc.sizeX, desc.sizeX, -desc.sizeY, desc.sizeY, desc.zNear, desc.zFar, false, reverseZ);
    } else {
        tanHalfFovX = BE1::Math::Tan(DEG2RAD(desc.fovX * 0.5f));
        tanHalfFovY = BE1::Math::Tan(DEG2RAD(desc.fovY * 0.5f));

        const float aspectRatio = tanHalfFovX / tanHalfFovY;

        // Set bounding frustum for perspective view.
        frustum.SetOrigin(desc.origin);
        frustum.SetAxis(desc.axis);
        frustum.SetSize(desc.zNear, desc.zFar, desc.zFar * tanHalfFovX, desc.zFar * tanHalfFovY);

        // Calculate view volume planes.
        frustum.ToPlanes(viewPlanes);

        // 원근 투영 행렬을 계산한다. viewMatrix 가 오른손 좌표계이므로, 오른손 좌표계용 투영 행렬을 계산한다.
        projMatrix.SetPerspectiveRH(desc.fovY, aspectRatio, desc.zNear, desc.zFar, false, reverseZ, desc.infiniteZFar);
    }

    viewProjMatrix = projMatrix * viewMatrix;
    invViewProjMatrix = viewProjMatrix.Inverse();
    jitteredViewProjMatrix = viewProjMatrix;
    jitteredInvViewProjMatrix = invViewProjMatrix;
}

void RenderCamera::CalcSlicedViewVolume(float nearDistance, float farDistance, BE1::Sphere *outBoundingSphere, BE1::Plane *outViewPlanes) const {
    assert(farDistance > nearDistance);
    assert(outBoundingSphere || outViewPlanes);

    if (desc.orthogonal) {
        BE1::OBB viewBox = box;
        const float halfDepth = (farDistance - nearDistance) * 0.5f;
        viewBox.SetCenter(desc.origin + desc.axis[EngineAxis::Forward] * (nearDistance + halfDepth));
        viewBox.SetExtents(BE1::Vec3(halfDepth, desc.sizeX, desc.sizeY));

        if (outViewPlanes) {
            viewBox.ToPlanes(outViewPlanes);
        }

        if (outBoundingSphere) {
            *outBoundingSphere = viewBox.ToSphere();
        }
        return;
    }

    BE1::Frustum viewFrustum = frustum;
    viewFrustum.MoveNearDistance(nearDistance);
    viewFrustum.MoveFarDistance(farDistance);

    if (outViewPlanes) {
        viewFrustum.ToPlanes(outViewPlanes);
    }

    if (outBoundingSphere) {
        *outBoundingSphere = viewFrustum.ToSphere();
    }
}

bool RenderCamera::Cull(const BE1::AABB &bounds) const {
    if (desc.orthogonal) {
        return !box.IsIntersectAABB(bounds);
    }
    return frustum.CullAABB(bounds);
}

bool RenderCamera::Cull(const BE1::OBB &bounds) const {
    if (desc.orthogonal) {
        return !box.IsIntersectOBB(bounds);
    }
    return frustum.CullOBB(bounds);
}

bool RenderCamera::Cull(const BE1::Sphere &bounds) const {
    if (desc.orthogonal) {
        return !box.IsIntersectSphere(bounds);
    }
    return frustum.CullSphere(bounds);
}

bool RenderCamera::Cull(const BE1::Frustum &bounds) const {
    if (desc.orthogonal) {
        return bounds.CullOBB(box);
    }
    return frustum.CullFrustum(bounds);
}

bool RenderCamera::TransformWorldToNDC(const BE1::Vec3 &worldPosition, BE1::Vec3 &normalizedDeviceCoords) const {
    const BE1::Vec4 clipCoords = viewProjMatrix * BE1::Vec4(worldPosition, 1.0f);
    if (clipCoords.w > 0) {
        const float invW = 1.0f / clipCoords.w;

        normalizedDeviceCoords.x = clipCoords.x * invW; // Clipping range is [-1, 1]
        normalizedDeviceCoords.y = clipCoords.y * invW; // Clipping range is [-1, 1]
        normalizedDeviceCoords.z = clipCoords.z * invW; // Clipping range is [0, 1]
        return true;
    }
    return false;
}

void RenderCamera::TransformNDCToPixel(const BE1::Vec3 &normalizedDeviceCoords, BE1::Vec3 &pixelCoords) const {
    const float fx = (normalizedDeviceCoords.x + 1.0f) * 0.5f; // Valid range is [0, 1]
    const float fy = 1.0f - (normalizedDeviceCoords.y + 1.0f) * 0.5f; // Valid range is [0, 1]

    pixelCoords.x = desc.outputRect.x + fx * desc.outputRect.w;
    pixelCoords.y = desc.outputRect.y + fy * desc.outputRect.h;
    pixelCoords.z = normalizedDeviceCoords.z; // NDC Z equals device depth in [0, 1].
}

bool RenderCamera::TransformWorldToPixel(const BE1::Vec3 &worldPosition, BE1::Vec3 &pixelCoords) const {
    BE1::Vec3 normalizedDeviceCoords;
    if (!TransformWorldToNDC(worldPosition, normalizedDeviceCoords)) {
        return false;
    }

    TransformNDCToPixel(normalizedDeviceCoords, pixelCoords);
    return true;
}

void RenderCamera::UntransformPixelToNDC(const BE1::Vec3 &pixelCoords, BE1::Vec3 &normalizedDeviceCoords) const {
    const float fx = (pixelCoords.x - desc.outputRect.x) / desc.outputRect.w;
    const float fy = 1.0f - (pixelCoords.y - desc.outputRect.y) / desc.outputRect.h;

    normalizedDeviceCoords.x = (fx * 2.0f) - 1.0f;
    normalizedDeviceCoords.y = (fy * 2.0f) - 1.0f;
    normalizedDeviceCoords.z = pixelCoords.z; // Device depth equals NDC Z in [0, 1].
}

bool RenderCamera::UntransformNDCToWorld(const BE1::Vec3 &normalizedDeviceCoords, BE1::Vec3 &worldCoords) const {
    const BE1::Vec4 worldPosition = viewProjMatrix.Inverse() * BE1::Vec4(normalizedDeviceCoords, 1.0f);
    if (worldPosition.w > 0) {
        float invW = 1.0f / worldPosition.w;

        worldCoords.x = worldPosition.x * invW;
        worldCoords.y = worldPosition.y * invW;
        worldCoords.z = worldPosition.z * invW;
        return true;
    }
    return false;
}

bool RenderCamera::UntransformPixelToWorld(const BE1::Vec3 &pixelCoords, BE1::Vec3 &worldCoords) const {
    BE1::Vec3 normalizedDeviceCoords;
    UntransformPixelToNDC(pixelCoords, normalizedDeviceCoords);

    if (UntransformNDCToWorld(normalizedDeviceCoords, worldCoords)) {
        return true;
    }
    return false;
}

bool RenderCamera::CalcClipRectFromSphere(const BE1::Sphere &sphere, BE1::Rect &clipRect) const {
    const float r = sphere.radius;
    const BE1::Vec3 localOrigin = desc.axis.TransposedMulVec(sphere.center - desc.origin);

    const int rectLeft = desc.outputRect.x;
    const int rectTop = desc.outputRect.y;
    const int rectRight = rectLeft + desc.outputRect.w;
    const int rectBottom = rectTop + desc.outputRect.h;
    const float halfWidth = desc.outputRect.w * 0.5f;
    const float halfHeight = desc.outputRect.h * 0.5f;

    // Orthogonal projection does not use perspective tangent planes.
    if (desc.orthogonal) {
        const float pixelsPerUnitX = halfWidth / desc.sizeX;
        const float pixelsPerUnitY = halfHeight / desc.sizeY;

        const float centerX = rectLeft + halfWidth - localOrigin.y * pixelsPerUnitX;
        const float centerY = rectTop + halfHeight - localOrigin.z * pixelsPerUnitY;
        const float extentX = r * pixelsPerUnitX;
        const float extentY = r * pixelsPerUnitY;

        const int xmin = BE1::Max(rectLeft, static_cast<int>(BE1::Math::Floor(centerX - extentX)));
        const int xmax = BE1::Min(rectRight, static_cast<int>(BE1::Math::Ceil(centerX + extentX)));
        const int ymin = BE1::Max(rectTop, static_cast<int>(BE1::Math::Floor(centerY - extentY)));
        const int ymax = BE1::Min(rectBottom, static_cast<int>(BE1::Math::Ceil(centerY + extentY)));

        if (xmax - xmin <= 0 || ymax - ymin <= 0) {
            return false;
        }

        clipRect.x = xmin;
        clipRect.y = ymin;
        clipRect.w = xmax - xmin;
        clipRect.h = ymax - ymin;
        return true;
    }

    const float r2 = r * r;
    const float x2 = localOrigin.x * localOrigin.x;
    const float y2 = localOrigin.y * localOrigin.y;
    const float z2 = localOrigin.z * localOrigin.z;

    // Camera axes are orthonormal, so local and world squared distances are identical.
    if (x2 + y2 + z2 < r2) {
        clipRect = desc.outputRect;
        return true;
    }

    int xmin = rectLeft;
    int xmax = rectRight;
    int ymin = rectTop;
    int ymax = rectBottom;

    const float rx = r * localOrigin.x;
    constexpr float tangentDiscriminantEpsilon = 0.001f;

    // 카메라 원점에서 XY 평면의 구 단면으로 그은 두 접선의 NDC X 를 직접 계산한다.
    // 기존 접평면 법선 계산에서 공통 분모를 소거한 식이다.
    const float horizontalTangentDistance2 = x2 + y2 - r2;
    const float horizontalDiscriminant = x2 * horizontalTangentDistance2;

    if (horizontalDiscriminant > tangentDiscriminantEpsilon) {
        const float tangentDistance = BE1::Math::Sqrt(horizontalTangentDistance2);
        const float xt = localOrigin.x * tangentDistance;
        const float ry = r * localOrigin.y;
        const float yt = localOrigin.y * tangentDistance;

        // The signs match the previous pz1/pz2 < 0 front-facing tests.
        const float leftDenominator = xt - ry;
        const float rightDenominator = xt + ry;

        if (leftDenominator <= 0.0f && rightDenominator <= 0.0f) {
            return false;
        }

        if (leftDenominator > 0.0f) {
            float ndcX = -(rx + yt) / (leftDenominator * tanHalfFovX);
            ndcX = BE1::Clamp(ndcX, -1.0f, 1.0f);

            const int vx = static_cast<int>(BE1::Math::Floor(rectLeft + (ndcX + 1.0f) * halfWidth));
            xmin = BE1::Max(rectLeft, vx);
        }

        if (rightDenominator > 0.0f) {
            float ndcX = (rx - yt) / (rightDenominator * tanHalfFovX);
            ndcX = BE1::Clamp(ndcX, -1.0f, 1.0f);

            const int vx = static_cast<int>(BE1::Math::Ceil(rectLeft + (ndcX + 1.0f) * halfWidth));
            xmax = BE1::Min(rectRight, vx);
        }

        if (xmax - xmin <= 0) {
            return false;
        }
    }

    // 카메라 원점에서 XZ 평면의 구 단면으로 그은 두 접선의 NDC Y 를 직접 계산한다.
    const float verticalTangentDistance2 = x2 + z2 - r2;
    const float verticalDiscriminant = x2 * verticalTangentDistance2;

    if (verticalDiscriminant > tangentDiscriminantEpsilon) {
        const float tangentDistance = BE1::Math::Sqrt(verticalTangentDistance2);
        const float xt = localOrigin.x * tangentDistance;
        const float rz = r * localOrigin.z;
        const float zt = localOrigin.z * tangentDistance;

        const float topDenominator = xt - rz;
        const float bottomDenominator = xt + rz;

        if (topDenominator <= 0.0f && bottomDenominator <= 0.0f) {
            return false;
        }

        if (topDenominator > 0.0f) {
            float ndcY = (rx + zt) / (topDenominator * tanHalfFovY);
            ndcY = BE1::Clamp(ndcY, -1.0f, 1.0f);

            const int vy = static_cast<int>(BE1::Math::Floor(rectTop + (1.0f - ndcY) * halfHeight));
            ymin = BE1::Max(rectTop, vy);
        }

        if (bottomDenominator > 0.0f) {
            float ndcY = (zt - rx) / (bottomDenominator * tanHalfFovY);
            ndcY = BE1::Clamp(ndcY, -1.0f, 1.0f);

            const int vy = static_cast<int>(BE1::Math::Ceil(rectTop + (1.0f - ndcY) * halfHeight));
            ymax = BE1::Min(rectBottom, vy);
        }

        if (ymax - ymin <= 0) {
            return false;
        }
    }

    clipRect.x = xmin;
    clipRect.y = ymin;
    clipRect.w = xmax - xmin;
    clipRect.h = ymax - ymin;
    return true;
}

bool RenderCamera::CalcClipRectFromAABB(const BE1::AABB &aabb, BE1::Rect &clipRect) const {
    return CalcClipRectFromOBB(BE1::OBB(aabb, BE1::Vec3::origin, BE1::Mat3::identity), clipRect);
}

bool RenderCamera::CalcClipRectFromOBB(const BE1::OBB &obb, BE1::Rect &clipRect) const {
    BE1::AABB bounds;

    if (desc.orthogonal) {
        if (!box.ProjectionBounds(obb, bounds)) {
            return false;
        }
    } else {
        if (!frustum.ProjectionBounds(obb, bounds)) {
            return false;
        }
    }

    bounds[0][1] = BE1::Clamp(bounds[0][1], -1.0f, 1.0f);
    bounds[1][1] = BE1::Clamp(bounds[1][1], -1.0f, 1.0f);
    bounds[0][2] = BE1::Clamp(bounds[0][2], -1.0f, 1.0f);
    bounds[1][2] = BE1::Clamp(bounds[1][2], -1.0f, 1.0f);

    if (bounds[1][1] - bounds[0][1] <= 0) {
        return false;
    }

    if (bounds[1][2] - bounds[0][2] <= 0) {
        return false;
    }

    clipRect.x = desc.outputRect.x + (-bounds[1][1] + 1) * desc.outputRect.w * 0.5f;
    clipRect.y = desc.outputRect.y + (-bounds[1][2] + 1) * desc.outputRect.h * 0.5f;
    clipRect.w = desc.outputRect.w * 0.5f * (bounds[1][1] - bounds[0][1]);
    clipRect.h = desc.outputRect.h * 0.5f * (bounds[1][2] - bounds[0][2]);

    return true;
}

bool RenderCamera::CalcClipRectFromFrustum(const BE1::Frustum &frustum, BE1::Rect &clipRect) const {
    BE1::AABB bounds;

    if (desc.orthogonal) {
        if (!this->box.ProjectionBounds(frustum, bounds)) {
            return false;
        }
    } else {
        // CHECK: is valid AABB ?
        if (!this->frustum.ProjectionBounds(frustum, bounds)) {
            return false;
        }
    }

    bounds[0][1] = BE1::Clamp(bounds[0][1], -1.0f, 1.0f);
    bounds[1][1] = BE1::Clamp(bounds[1][1], -1.0f, 1.0f);
    bounds[0][2] = BE1::Clamp(bounds[0][2], -1.0f, 1.0f);
    bounds[1][2] = BE1::Clamp(bounds[1][2], -1.0f, 1.0f);

    if (bounds[1][1] - bounds[0][1] <= 0) {
        return false;
    }

    if (bounds[1][2] - bounds[0][2] <= 0) {
        return false;
    }

    clipRect.x = desc.outputRect.x + (-bounds[1][1] + 1) * desc.outputRect.w * 0.5f;
    clipRect.y = desc.outputRect.y + (-bounds[1][2] + 1) * desc.outputRect.h * 0.5f;
    clipRect.w = desc.outputRect.w * 0.5f * (bounds[1][1] - bounds[0][1]);
    clipRect.h = desc.outputRect.h * 0.5f * (bounds[1][2] - bounds[0][2]);

    return true;
}

bool RenderCamera::CalcDepthBoundsFromPoints(int numPoints, const BE1::Vec3 *points, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const {
    const float ndcMin = 0.0f;
    const float ndcMax = 1.0f;
    const float clippedNearZ = desc.projectionPolicy.reverseZ ? ndcMax : ndcMin;

    float localMin = BE1::Math::BigNumber;
    float localMax = -BE1::Math::BigNumber;

    for (int i = 0; i < numPoints; ++i) {
        const BE1::Vec3 &v = points[i];

        float cz = v[0] * mvp[2][0] + v[1] * mvp[2][1] + v[2] * mvp[2][2] + mvp[2][3];
        float cw = v[0] * mvp[3][0] + v[1] * mvp[3][1] + v[2] * mvp[3][2] + mvp[3][3];

        if (cw > std::numeric_limits<float>::min()) {
            cz = cz / cw;
        } else {
            cz = clippedNearZ;
        }

        localMin = BE1::Min(localMin, cz);
        localMax = BE1::Max(localMax, cz);
    }

    if (localMax < ndcMin || localMin > ndcMax) {
        return false;
    }

    localMin = BE1::Max(localMin, ndcMin);
    localMax = BE1::Min(localMax, ndcMax);

    *depthMin = localMin;
    *depthMax = localMax;
    return true;
}

bool RenderCamera::CalcDepthBoundsFromSphere(const BE1::Sphere &sphere, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const {
    BE1::Vec3 points[2];
    points[0] = sphere.center + desc.axis[0] * sphere.radius;
    points[1] = sphere.center - desc.axis[0] * sphere.radius;
    return CalcDepthBoundsFromPoints(2, points, mvp, depthMin, depthMax);
}

bool RenderCamera::CalcDepthBoundsFromAABB(const BE1::AABB &aabb, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const {
    BE1::Vec3 points[8];
    aabb.ToPoints(points);
    return CalcDepthBoundsFromPoints(8, points, mvp, depthMin, depthMax);
}

bool RenderCamera::CalcDepthBoundsFromOBB(const BE1::OBB &obb, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const {
    BE1::Vec3 points[8];
    obb.ToPoints(points);
    return CalcDepthBoundsFromPoints(8, points, mvp, depthMin, depthMax);
}

bool RenderCamera::CalcDepthBoundsFromFrustum(const BE1::Frustum &frustum, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const {
    BE1::Vec3 points[8];
    frustum.ToPoints(points);
    return CalcDepthBoundsFromPoints(8, points, mvp, depthMin, depthMax);
}

float RenderCamera::CalcLocalLODErrorLimitSqr(const BE1::Mat3x4 &worldMatrix, const BE1::AABB &worldBounds, float pixelErrorLimit) const {
    if (!(pixelErrorLimit > 0.0f) || renderWidth == 0 || renderHeight == 0) {
        return 0.0f;
    }

    // 투영 행렬과 내부 렌더링 해상도를 사용한다.
    float pixelsPerWorldUnit = 0.5f * BE1::Max(BE1::Math::Abs(projMatrix[0][0]) * renderWidth, BE1::Math::Abs(projMatrix[1][1]) * renderHeight);

    if (!desc.orthogonal) {
        // 중심보다 카메라에 가까운 경계를 기준으로 평가한다.
        const BE1::Vec3 &forward = desc.axis[0];
        const float depth = (worldBounds.Center() - desc.origin).Dot(forward) - worldBounds.Extents().AbsDot(forward);

        // near plane 에 걸치거나 카메라 뒤에 있는 경우 LOD 0 를 사용한다.
        if (!(depth > desc.zNear)) {
            return 0.0f;
        }

        pixelsPerWorldUnit /= depth;
    }

    return ::CalcLocalLODErrorLimitSqr(worldMatrix, pixelsPerWorldUnit, pixelErrorLimit);
}

void RenderCamera::CalculateFov(float fromFovX, float fromAspectRatio, float toAspectRatio, float *toFovX, float *toFovY) {
    const float tanFovX = BE1::Math::Tan(DEG2RAD(fromFovX * 0.5f));
    const float tanFovY = tanFovX / fromAspectRatio;

    *toFovX = RAD2DEG(BE1::Math::ATan(tanFovY * toAspectRatio) * 2.0f);
    *toFovY = RAD2DEG(BE1::Math::ATan(tanFovY) * 2.0f);
}

// needDepthHistory off
// - depthTexture : MSAA sample count 를 따르는 DSV / SRV
// - depthHistoryTextures : 없음
//
// needDepthHistory on, MSAA off
// - depthTexture : 없음
// - depthHistoryTextures : DSV / SRV 두 개를 ping-pong depth attachment 로 사용
//
// needDepthHistory on, MSAA on
// - depthTexture : MSAA DSV / SRV
// - depthHistoryTextures : SRV / UAV 두 개를 ping-pong 하며 MSAA depthTexture 의 resolve 결과를 저장
bool RenderCamera::EnsureRenderTextures(uint32_t width, uint32_t height, RHI::Format colorFormat, RHI::Format depthFormat, uint32_t sampleCount, bool needDepthHistory, bool needNormalRoughnessTexture, bool needVelocityTexture, const BE1::Color4 &optimizedClearColor) {
    sampleCount = BE1::Max(sampleCount, 1u);

    const bool useDepthHistoryRenderTarget = needDepthHistory && sampleCount == 1;
    const bool hasExpectedDepthTextures = useDepthHistoryRenderTarget ? (depthHistoryAsRenderTarget && depthHistoryTextures[0] && depthHistoryTextures[1] && !depthTexture) : (!depthHistoryAsRenderTarget && depthTexture);
    const bool hasExpectedNormalRoughnessTextures = (normalRoughnessTexture != nullptr) == needNormalRoughnessTexture && (resolvedNormalRoughnessTexture != nullptr) == (needNormalRoughnessTexture && sampleCount > 1);
    const bool hasExpectedVelocityTextures = (velocityTexture != nullptr) == needVelocityTexture && (resolvedVelocityTexture != nullptr) == (needVelocityTexture && sampleCount > 1);
    const bool needTAAReactiveMask = needDepthHistory && needVelocityTexture;
    const bool hasExpectedTAAReactiveMaskTextures = (taaReactiveMaskTexture != nullptr) == needTAAReactiveMask && (resolvedTAAReactiveMaskTexture != nullptr) == (needTAAReactiveMask && sampleCount > 1);
    const bool hasExpectedResolvedColorTexture = sampleCount > 1 ? resolvedColorTexture != nullptr : resolvedColorTexture == nullptr;

    if (colorTexture &&
        hasExpectedResolvedColorTexture &&
        hasExpectedDepthTextures &&
        hasExpectedNormalRoughnessTextures &&
        hasExpectedVelocityTextures &&
        hasExpectedTAAReactiveMaskTextures &&
        renderWidth == width && renderHeight == height &&
        renderColorFormat == colorFormat &&
        renderDepthFormat == depthFormat &&
        renderSampleCount == sampleCount) {
        return true;
    }

    // 렌더 텍스처 재생성 후 이전 내용은 사용하지 않는다.
    currentHistoryTextureIndex = 0;
    historyValid = false;
    ssrSceneColorHistoryResources.Invalidate();
    ssrTemporalHistoryValid = false;
    transparentSSRTemporalHistoryValid = false;

    if (!CreateRenderTextures(width, height, colorFormat, depthFormat, sampleCount, needDepthHistory, needNormalRoughnessTexture, needVelocityTexture, optimizedClearColor)) {
        DestroyRenderTextures();
        return false;
    }

    return true;
}

bool RenderCamera::EnsureShadowAtlas(const RenderFrameSettings &renderFrameSettings, bool hasShadowCandidates) {
    if (!renderFrameSettings.shadows || renderFrameSettings.shadowAtlasMaxViews == 0) {
        shadowAtlas.Destroy();
        return true;
    }

    // 후보가 생길 때 처음 생성한다. 이미 생성한 atlas 는 후보가 잠시 없어져도 재사용한다.
    if (!hasShadowCandidates && !shadowAtlas.GetShadowTexture()) {
        return true;
    }

    return shadowAtlas.Ensure(renderFrameSettings.shadowAtlasSize, renderFrameSettings.shadowAtlasMaxTileResolution, RenderFrameSettings::ShadowAtlasMinTileResolution);
}

void RenderCamera::EnsurePostProcessResources(bool skipPostProcess, RHI::Format colorFormat, bool needColorHistory, bool needDepthHistory, const RenderFrameSettings &renderFrameSettings) {
    if (skipPostProcess) {
        DestroyPostProcessResources();
        return;
    }

    if (renderWidth == 0 || renderHeight == 0) {
        DestroyPostProcessResources();
        return;
    }

    if (!postProcessColorTexture || postProcessColorTexture->GetWidth() != renderWidth || postProcessColorTexture->GetHeight() != renderHeight || postProcessColorTexture->GetFormat() != colorFormat) {
        if (!CreatePostProcessColorTexture(renderWidth, renderHeight, colorFormat)) {
            DestroyPostProcessResources();
            return;
        }
    }

    if (needColorHistory) {
        if (!EnsureColorHistoryTextures(renderWidth, renderHeight, renderColorFormat)) {
            DestroyColorHistoryTextures();
        }
    } else {
        DestroyColorHistoryTextures();
    }

    if (needDepthHistory) {
        if (!depthHistoryAsRenderTarget && !EnsureDepthHistoryResolveTargets(renderWidth, renderHeight)) {
            DestroyDepthHistoryTextures();
        }
    } else {
        if (!depthHistoryAsRenderTarget) {
            DestroyDepthHistoryTextures();
        }
    }

    bloomResources.Ensure(renderWidth, renderHeight, renderFrameSettings);

    if (!luminanceBuffer) {
        // Tonemapping 에 사용할 Luminance 버퍼 생성
        CreateLuminanceBuffer();
    }
}

void RenderCamera::InvalidateTemporalHistory() {
    localShadowResolutionHistory.Reset();
    temporalHistoryInvalidated = true;
    temporalFrameIndex = 0;
    temporalJitterSampleCount = 0;
    temporalJitterActive = false;
    historyValid = false;
    ssrSceneColorHistoryResources.Invalidate();
    ssrTemporalHistoryValid = false;
    transparentSSRTemporalHistoryValid = false;

    currJitter = BE1::Vec2::zero;
    prevJitter = BE1::Vec2::zero;
    prevJitteredViewProjMatrix = viewProjMatrix;
    prevJitteredInvViewProjMatrix = invViewProjMatrix;
    hiZResources.SetHistoryValid(false);
}

void RenderCamera::UpdateTemporalJitter(bool enableJitter, uint32_t sampleCount) {
    static constexpr uint32_t MaxTemporalJitterSampleCount = 64;

    const bool requestedJitter = enableJitter && sampleCount > 1;
    const uint32_t requestedSampleCount = requestedJitter ? BE1::Min(sampleCount, MaxTemporalJitterSampleCount) : 0;
    const bool jitterSettingChanged = temporalJitterActive != requestedJitter || temporalJitterSampleCount != requestedSampleCount;

    if (jitterSettingChanged) {
        temporalHistoryInvalidated = true;
        ssrSceneColorHistoryResources.Invalidate();
        ssrTemporalHistoryValid = false;
        transparentSSRTemporalHistoryValid = false;
        temporalFrameIndex = 0;
        prevJitter = BE1::Vec2::zero;
        prevJitteredViewProjMatrix = viewProjMatrix;
        prevJitteredInvViewProjMatrix = invViewProjMatrix;
    }

    temporalJitterActive = requestedJitter;
    temporalJitterSampleCount = requestedSampleCount;

    if (!requestedJitter) {
        currJitter = BE1::Vec2::zero;
        prevJitter = BE1::Vec2::zero;
        temporalFrameIndex = 0;

        jitteredViewProjMatrix = viewProjMatrix;
        jitteredInvViewProjMatrix = invViewProjMatrix;
        return;
    }

    // 이전 프레임의 jitter offset 을 저장한다.
    prevJitter = jitterSettingChanged ? BE1::Vec2::zero : currJitter;

    // deterministic offset 이 Vec2::zero 이므로 0 번 샘플은 (0, 0) 이다.
    // jitter 로 변환하면 (-0.5, -0.5) 이 되어 최대 offset 이 되므로 1 번 샘플부터 사용한다.
    static const BE1::Halton<MaxTemporalJitterSampleCount + 1> jitterSamples(2, 3, BE1::Vec2::zero);
    const uint32_t sampleIndex = (temporalFrameIndex % temporalJitterSampleCount) + 1;
    const BE1::Vec2 &sample = jitterSamples.GetSample(sampleIndex);

    currJitter = BE1::Vec2(sample.x - 0.5f, sample.y - 0.5f); // [-0.5, 0.5] 범위
    const float ndcJitterX = currJitter.x * (2.0f / static_cast<float>(renderWidth));
    const float ndcJitterY = currJitter.y * (-2.0f / static_cast<float>(renderHeight));

    // clip.x = row0 dot position
    // clip.y = row1 dot position
    // clip.z = row2 dot position
    // clip.w = row3 dot position
    // jitteredClip.x = clip.x + clip.w * ndcJitterX = row0 dot position + row3 dot position * ndcJitterX = (row0 + row3 * ndcJitterX) dot position
    // jitteredClip.y = clip.y + clip.w * ndcJitterY = row1 dot position + row3 dot position * ndcJitterY = (row1 + row3 * ndcJitterY) dot position
    jitteredViewProjMatrix = viewProjMatrix;
    jitteredViewProjMatrix[0] += viewProjMatrix[3] * ndcJitterX;
    jitteredViewProjMatrix[1] += viewProjMatrix[3] * ndcJitterY;

    jitteredInvViewProjMatrix = jitteredViewProjMatrix.Inverse();

    temporalFrameIndex++;
}

void RenderCamera::DestroyPostProcessResources() {
    DestroyLuminanceBuffer();
    DestroyPostProcessColorTexture();
    DestroyColorHistoryTextures();

    if (!depthHistoryAsRenderTarget) {
        // depthHistoryAsRenderTarget == false 일 때만 depthHistoryTextures 가 post process textures 이므로, 그때만 파괴한다.
        // depthHistoryAsRenderTarget == true 라면, render textures 이므로 DestroyRenderTextures 에서 파괴된다.
        DestroyDepthHistoryTextures();
    }
    bloomResources.Destroy();
}

bool RenderCamera::CreateRenderTextures(uint32_t width, uint32_t height, RHI::Format colorFormat, RHI::Format depthFormat, uint32_t sampleCount, bool needDepthHistory, bool needNormalRoughnessTexture, bool needVelocityTexture, const BE1::Color4 &optimizedClearColor) {
    DestroyRenderTextures();

    if (width == 0 || height == 0) {
        return false;
    }

    sampleCount = BE1::Max(sampleCount, 1u);

    if (!CreateColorTextures(width, height, colorFormat, sampleCount, optimizedClearColor)) {
        return false;
    }

    if (needDepthHistory && sampleCount == 1) {
        if (!CreateDepthHistoryRenderTargets(width, height, depthFormat)) {
            return false;
        }
    } else {
        if (!CreateDepthTexture(width, height, depthFormat, sampleCount)) {
            return false;
        }
    }

    if (needNormalRoughnessTexture && !CreateNormalRoughnessTextures(width, height, sampleCount)) {
        return false;
    }

    if (needVelocityTexture && !CreateVelocityTextures(width, height, sampleCount, needDepthHistory)) {
        return false;
    }

    renderWidth = width;
    renderHeight = height;
    renderColorFormat = colorFormat;
    renderDepthFormat = depthFormat;
    renderSampleCount = sampleCount;

    return true;
}

void RenderCamera::DestroyRenderTextures() {
    DestroySceneSnapshotResources();

    if (colorTexture) {
        RHI::renderer->DestroyTexture(colorTexture);
        colorTexture = nullptr;
    }

    if (resolvedColorTexture) {
        RHI::renderer->DestroyTexture(resolvedColorTexture);
        resolvedColorTexture = nullptr;
    }

    if (depthTexture) {
        RHI::renderer->DestroyTexture(depthTexture);
        depthTexture = nullptr;
    }

    if (normalRoughnessTexture) {
        RHI::renderer->DestroyTexture(normalRoughnessTexture);
        normalRoughnessTexture = nullptr;
    }

    if (resolvedNormalRoughnessTexture) {
        RHI::renderer->DestroyTexture(resolvedNormalRoughnessTexture);
        resolvedNormalRoughnessTexture = nullptr;
    }

    if (velocityTexture) {
        RHI::renderer->DestroyTexture(velocityTexture);
        velocityTexture = nullptr;
    }

    if (resolvedVelocityTexture) {
        RHI::renderer->DestroyTexture(resolvedVelocityTexture);
        resolvedVelocityTexture = nullptr;
    }

    if (taaReactiveMaskTexture) {
        RHI::renderer->DestroyTexture(taaReactiveMaskTexture);
        taaReactiveMaskTexture = nullptr;
    }

    if (resolvedTAAReactiveMaskTexture) {
        RHI::renderer->DestroyTexture(resolvedTAAReactiveMaskTexture);
        resolvedTAAReactiveMaskTexture = nullptr;
    }

    if (depthHistoryAsRenderTarget) {
        DestroyDepthHistoryTextures();
    }

    renderWidth = 0;
    renderHeight = 0;
    renderColorFormat = RHI::Format::Unknown;
    renderDepthFormat = RHI::Format::Unknown;
    renderSampleCount = 1;
}

bool RenderCamera::CreateColorTextures(uint32_t width, uint32_t height, RHI::Format colorFormat, uint32_t sampleCount, const BE1::Color4 &optimizedClearColor) {
    BE1::Image::Format imageFormat = BE1::Image::Format::Unknown;
    RHI::Renderer::RHIFormatToImageFormat(colorFormat, &imageFormat, nullptr);

    const RHI::ClearValue optimizedClearValue = RHI::ClearValue::Color(optimizedClearColor);

    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Init2DFromMemory(width, height, 1, imageFormat, BE1::Image::GammaSpace::Linear, nullptr);

    const bool useMSAA = sampleCount > 1;
    RHI::TextureFlag textureFlags = RHI::TextureFlag::RenderTarget;
    if (!useMSAA) {
        textureFlags |= RHI::TextureFlag::ShaderResource;
    }

    colorTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, textureFlags, image, RHI::TextureMipInitMode::FromImage, optimizedClearValue, sampleCount, RHI::GPUResourceState::ShaderResource);
    if (!colorTexture) {
        DestroyRenderTextures();
        BE_WARNLOG("RenderCamera::CreateColorTextures: failed to create color texture\n");
        return false;
    }
    RHI::renderer->SetName(colorTexture, "RenderCamera::colorTexture");

    if (useMSAA) {
        resolvedColorTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource, image, RHI::TextureMipInitMode::FromImage, optimizedClearValue, 1, RHI::GPUResourceState::ShaderResource);
        if (!resolvedColorTexture) {
            DestroyRenderTextures();
            BE_WARNLOG("RenderCamera::CreateColorTextures: failed to create resolved color texture\n");
            return false;
        }
        RHI::renderer->SetName(resolvedColorTexture, "RenderCamera::resolvedColorTexture");
    }
    return true;
}

bool RenderCamera::CreateDepthTexture(uint32_t width, uint32_t height, RHI::Format depthFormat, uint32_t sampleCount) {
    BE1::Image::Format imageFormat = BE1::Image::Format::Unknown;
    RHI::Renderer::RHIFormatToImageFormat(depthFormat, &imageFormat, nullptr);

    const RHI::ClearValue optimizedClearValue = RHI::ClearValue::DepthStencil(desc.projectionPolicy.GetDepthClearValue(), 0);

    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Init2DFromMemory(width, height, 1, imageFormat, BE1::Image::GammaSpace::Linear, nullptr);
    depthTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::DepthStencil | RHI::TextureFlag::ShaderResource | RHI::TextureFlag::Typeless, image, RHI::TextureMipInitMode::FromImage, optimizedClearValue, sampleCount, RHI::GPUResourceState::ShaderResource);
    if (!depthTexture) {
        BE_WARNLOG("RenderCamera::CreateDepthTexture: failed to create depth texture\n");
        DestroyRenderTextures();
        return false;
    }
    RHI::renderer->SetName(depthTexture, "RenderCamera::depthTexture");

    if (!RHI::renderer->CreateView(depthTexture, RHI::ViewType::SRV).IsValid() ||
        !RHI::renderer->CreateView(depthTexture, RHI::ViewType::DSV).IsValid()) {
        BE_WARNLOG("RenderCamera::CreateDepthTexture: failed to create depth texture views\n");
        DestroyRenderTextures();
        return false;
    }

    return true;
}

bool RenderCamera::CreateNormalRoughnessTextures(uint32_t width, uint32_t height, uint32_t sampleCount) {
    const BE1::Image::Format imageFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R10G10B10A2, RHI::UAVAccessFlag::Store);

    const RHI::ClearValue optimizedClearValue = RHI::ClearValue::Color(0.5f, 0.5f, 1.0f, 0.0f);

    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Init2DFromMemory(width, height, 1, imageFormat, BE1::Image::GammaSpace::Linear, nullptr);

    const bool useMSAA = sampleCount > 1;
    RHI::TextureFlag textureFlags = RHI::TextureFlag::RenderTarget | RHI::TextureFlag::ShaderResource;
    if (!useMSAA) {
        textureFlags |= RHI::TextureFlag::UnorderedAccess;
    }

    normalRoughnessTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, textureFlags, image, RHI::TextureMipInitMode::FromImage, optimizedClearValue, sampleCount, RHI::GPUResourceState::ShaderResource);
    if (!normalRoughnessTexture) {
        BE_WARNLOG("RenderCamera::CreateNormalRoughnessTextures: failed to create normal/roughness texture\n");
        DestroyRenderTextures();
        return false;
    }
    RHI::renderer->SetName(normalRoughnessTexture, "RenderCamera::normalRoughnessTexture");

    if (useMSAA) {
        resolvedNormalRoughnessTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::FromImage, optimizedClearValue, 1, RHI::GPUResourceState::ShaderResource);
        if (!resolvedNormalRoughnessTexture) {
            BE_WARNLOG("RenderCamera::CreateNormalRoughnessTextures: failed to create resolved normal/roughness texture\n");
            DestroyRenderTextures();
            return false;
        }
        RHI::renderer->SetName(resolvedNormalRoughnessTexture, "RenderCamera::resolvedNormalRoughnessTexture");
    }
    return true;
}

bool RenderCamera::CreateVelocityTextures(uint32_t width, uint32_t height, uint32_t sampleCount, bool needTAAReactiveMask) {
    BE1::Image::Format imageFormat = BE1::Image::Format::Unknown;
    RHI::Renderer::RHIFormatToImageFormat(RHI::Format::R16G16_FLOAT, &imageFormat, nullptr);

    const RHI::ClearValue optimizedClearValue = RHI::ClearValue::Color(0.0f, 0.0f, 0.0f, 0.0f);

    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Init2DFromMemory(width, height, 1, imageFormat, BE1::Image::GammaSpace::Linear, nullptr);

    const bool useMSAA = sampleCount > 1;
    RHI::TextureFlag textureFlags = RHI::TextureFlag::RenderTarget | RHI::TextureFlag::ShaderResource;
    if (!useMSAA) {
        textureFlags |= RHI::TextureFlag::UnorderedAccess;
    }

    velocityTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, textureFlags, image, RHI::TextureMipInitMode::FromImage, optimizedClearValue, sampleCount, RHI::GPUResourceState::ShaderResource);
    if (!velocityTexture) {
        BE_WARNLOG("RenderCamera::CreateVelocityTextures: failed to create velocity texture\n");
        DestroyRenderTextures();
        return false;
    }
    RHI::renderer->SetName(velocityTexture, "RenderCamera::velocityTexture");

    if (useMSAA) {
        resolvedVelocityTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::FromImage, optimizedClearValue, 1, RHI::GPUResourceState::ShaderResource);
        if (!resolvedVelocityTexture) {
            BE_WARNLOG("RenderCamera::CreateVelocityTextures: failed to create resolved velocity texture\n");
            DestroyRenderTextures();
            return false;
        }
        RHI::renderer->SetName(resolvedVelocityTexture, "RenderCamera::resolvedVelocityTexture");
    }

    if (!needTAAReactiveMask) {
        return true;
    }

    // Velocity 두 채널은 그대로 보존하고 TAA 반응도는 1 byte/pixel mask 로 전달한다.
    // MSAA mask 는 post-opaque render pass 에서 resolve 하므로 UAV 가 필요하지 않다.
    RHI::Renderer::RHIFormatToImageFormat(RHI::Format::R8_UNORM, &imageFormat, nullptr);
    SharedPtr<BE1::Image> reactiveMaskImage = MakeShared<BE1::Image>();
    reactiveMaskImage->Init2DFromMemory(width, height, 1, imageFormat, BE1::Image::GammaSpace::Linear, nullptr);
    const RHI::TextureFlag reactiveMaskFlags = RHI::TextureFlag::RenderTarget | RHI::TextureFlag::ShaderResource;
    taaReactiveMaskTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, reactiveMaskFlags, reactiveMaskImage, RHI::TextureMipInitMode::FromImage, optimizedClearValue, sampleCount, RHI::GPUResourceState::ShaderResource);
    if (!taaReactiveMaskTexture) {
        BE_WARNLOG("RenderCamera::CreateVelocityTextures: failed to create TAA reactive mask texture\n");
        DestroyRenderTextures();
        return false;
    }
    RHI::renderer->SetName(taaReactiveMaskTexture, "RenderCamera::taaReactiveMaskTexture");

    if (useMSAA) {
        resolvedTAAReactiveMaskTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, reactiveMaskFlags, reactiveMaskImage, RHI::TextureMipInitMode::FromImage, optimizedClearValue, 1, RHI::GPUResourceState::ShaderResource);
        if (!resolvedTAAReactiveMaskTexture) {
            BE_WARNLOG("RenderCamera::CreateVelocityTextures: failed to create resolved TAA reactive mask texture\n");
            DestroyRenderTextures();
            return false;
        }
        RHI::renderer->SetName(resolvedTAAReactiveMaskTexture, "RenderCamera::resolvedTAAReactiveMaskTexture");
    }
    return true;
}

bool RenderCamera::EnsureSceneSnapshotResources(bool needed, bool halfResolution) {
    if (!needed) {
        DestroySceneSnapshotResources();
        return true;
    }

    const uint32_t snapshotWidth = halfResolution ? BE1::Max((renderWidth + 1u) / 2u, 1u) : renderWidth;
    const uint32_t snapshotHeight = halfResolution ? BE1::Max((renderHeight + 1u) / 2u, 1u) : renderHeight;
    const RHI::Format snapshotColorFormat = renderColorFormat;
    const RHI::Format snapshotDepthFormat = halfResolution ? RHI::Format::R32_FLOAT : renderDepthFormat;

    if (snapshotColorTexture && snapshotDepthTexture &&
        snapshotColorTexture->GetWidth() == snapshotWidth && snapshotColorTexture->GetHeight() == snapshotHeight && snapshotColorTexture->GetFormat() == snapshotColorFormat &&
        snapshotDepthTexture->GetWidth() == snapshotWidth && snapshotDepthTexture->GetHeight() == snapshotHeight && snapshotDepthTexture->GetFormat() == snapshotDepthFormat) {
        return true;
    }

    DestroySceneSnapshotResources();

    return CreateSceneSnapshotResources(halfResolution);
}

bool RenderCamera::CreateSceneSnapshotResources(bool halfResolution) {
    if (renderWidth == 0 || renderHeight == 0 || renderColorFormat == RHI::Format::Unknown || renderDepthFormat == RHI::Format::Unknown) {
        return false;
    }

    BE1::Image::Format colorImageFormat = BE1::Image::Format::Unknown;
    if (!RHI::Renderer::RHIFormatToImageFormat(renderColorFormat, &colorImageFormat, nullptr)) {
        return false;
    }

    const uint32_t snapshotWidth = halfResolution ? BE1::Max((renderWidth + 1u) / 2u, 1u) : renderWidth;
    const uint32_t snapshotHeight = halfResolution ? BE1::Max((renderHeight + 1u) / 2u, 1u) : renderHeight;
    // MSAA opaque rendering resolves directly into the full-resolution snapshot; SSR composite then applies its radiance replacement through the RTV.
    const RHI::TextureFlag snapshotColorFlags = RHI::TextureFlag::RenderTarget | RHI::TextureFlag::ShaderResource;

    SharedPtr<BE1::Image> colorImage = MakeShared<BE1::Image>();
    colorImage->Init2DFromMemory(snapshotWidth, snapshotHeight, 1, colorImageFormat, BE1::Image::GammaSpace::Linear, nullptr);
    snapshotColorTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, snapshotColorFlags, colorImage, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!snapshotColorTexture) {
        BE_WARNLOG("RenderCamera::CreateSceneSnapshotResources: failed to create scene color texture\n");
        return false;
    }
    RHI::renderer->SetName(snapshotColorTexture, "RenderCamera::snapshotColorTexture");

    if (halfResolution) {
        SharedPtr<BE1::Image> depthImage = MakeShared<BE1::Image>();
        depthImage->Init2DFromMemory(snapshotWidth, snapshotHeight, 1, BE1::Image::Format::R32_FLOAT, BE1::Image::GammaSpace::Linear, nullptr);
        snapshotDepthTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::RenderTarget | RHI::TextureFlag::ShaderResource | RHI::TextureFlag::SkipDefaultViews, depthImage, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
        if (snapshotDepthTexture) {
            // Half-resolution depth 는 MRT color output 과 ocean shader input 양쪽에서 사용한다.
            const RHI::ViewID srvViewID = RHI::renderer->CreateView(snapshotDepthTexture, RHI::ViewType::SRV);
            const RHI::ViewID rtvViewID = RHI::renderer->CreateView(snapshotDepthTexture, RHI::ViewType::RTV);
            if (!srvViewID.IsPrimary() || !rtvViewID.IsPrimary()) {
                RHI::renderer->DestroyTexture(snapshotDepthTexture);
                snapshotDepthTexture = nullptr;
            }
        }
    } else {
        BE1::Image::Format depthImageFormat = BE1::Image::Format::Unknown;
        if (!RHI::Renderer::RHIFormatToImageFormat(renderDepthFormat, &depthImageFormat, nullptr)) {
            DestroySceneSnapshotResources();
            return false;
        }

        const RHI::ClearValue depthClearValue = RHI::ClearValue::DepthStencil(desc.projectionPolicy.GetDepthClearValue(), 0);
        SharedPtr<BE1::Image> depthImage = MakeShared<BE1::Image>();
        depthImage->Init2DFromMemory(snapshotWidth, snapshotHeight, 1, depthImageFormat, BE1::Image::GammaSpace::Linear, nullptr);
        snapshotDepthTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::DepthStencil | RHI::TextureFlag::ShaderResource | RHI::TextureFlag::Typeless, depthImage, RHI::TextureMipInitMode::FromImage, depthClearValue, 1, RHI::GPUResourceState::ShaderResource);
        if (snapshotDepthTexture && (!RHI::renderer->CreateView(snapshotDepthTexture, RHI::ViewType::SRV).IsValid() || !RHI::renderer->CreateView(snapshotDepthTexture, RHI::ViewType::DSV).IsValid())) {
            RHI::renderer->DestroyTexture(snapshotDepthTexture);
            snapshotDepthTexture = nullptr;
        }
    }

    if (!snapshotDepthTexture) {
        BE_WARNLOG("RenderCamera::CreateSceneSnapshotResources: failed to create scene depth texture\n");
        DestroySceneSnapshotResources();
        return false;
    }
    RHI::renderer->SetName(snapshotDepthTexture, "RenderCamera::snapshotDepthTexture");

    return true;
}

void RenderCamera::DestroySceneSnapshotResources() {
    if (snapshotColorTexture) {
        RHI::renderer->DestroyTexture(snapshotColorTexture);
        snapshotColorTexture = nullptr;
    }

    if (snapshotDepthTexture) {
        RHI::renderer->DestroyTexture(snapshotDepthTexture);
        snapshotDepthTexture = nullptr;
    }
}

bool RenderCamera::EnsureSSRResources(uint32_t width, uint32_t height, uint32_t resolutionMode, bool needed) {
    if (!needed || width == 0 || height == 0) {
        DestroySSRResources();
        return false;
    }

    if (!ssrHistoryRenderState) {
        ssrHistoryRenderState = MakeShared<SSRHistoryRenderState>();
    }

    assert(resolutionMode < RenderFrameSettings::SSRResolutionModeCount);
    const uint32_t traceWidth = BE1::Max((width + (1u << resolutionMode) - 1u) >> resolutionMode, 1u);
    const uint32_t traceHeight = BE1::Max((height + (1u << resolutionMode) - 1u) >> resolutionMode, 1u);

    if (ssrResolutionMode != resolutionMode) {
        ssrTemporalHistoryValid = false;
        ssrResolutionMode = resolutionMode;
    }

    const bool validTraceTexture = ssrTraceTexture && ssrTraceTexture->GetWidth() == traceWidth && ssrTraceTexture->GetHeight() == traceHeight;
    const bool validReceiverSurfaceTexture = ssrReceiverSurfaceTexture && ssrReceiverSurfaceTexture->GetWidth() == traceWidth && ssrReceiverSurfaceTexture->GetHeight() == traceHeight;
    const bool validResolveReflectionTexture = ssrResolveReflectionTexture && ssrResolveReflectionTexture->GetWidth() == traceWidth && ssrResolveReflectionTexture->GetHeight() == traceHeight;
    const bool validTemporalReflectionTextures = ssrTemporalReflectionTextures[0] && ssrTemporalReflectionTextures[1] &&
        ssrTemporalReflectionTextures[0]->GetWidth() == traceWidth && ssrTemporalReflectionTextures[0]->GetHeight() == traceHeight &&
        ssrTemporalReflectionTextures[1]->GetWidth() == traceWidth && ssrTemporalReflectionTextures[1]->GetHeight() == traceHeight;
    const bool validUpsampledReflectionTexture = ssrUpsampledReflectionTexture && ssrUpsampledReflectionTexture->GetWidth() == width && ssrUpsampledReflectionTexture->GetHeight() == height;

    if (!validTraceTexture || !validReceiverSurfaceTexture || !validResolveReflectionTexture || !validTemporalReflectionTextures || !validUpsampledReflectionTexture) {
        DestroySSRResources();

        if (!CreateSSRTraceTexture(traceWidth, traceHeight) ||
            !CreateSSRReceiverSurfaceTexture(traceWidth, traceHeight) ||
            !CreateSSRResolveReflectionTexture(traceWidth, traceHeight) ||
            !CreateSSRTemporalReflectionTextures(traceWidth, traceHeight) ||
            !CreateSSRUpsampledReflectionTexture(width, height)) {
            DestroySSRResources();
            return false;
        }
    }

    return true;
}

bool RenderCamera::EnsureTransparentSSRResources(uint32_t width, uint32_t height, RHI::Format depthFormat, uint32_t resolutionMode, bool needed) {
    if (!needed || width == 0 || height == 0) {
        DestroyTransparentSSRResources();
        return false;
    }

    assert(resolutionMode < RenderFrameSettings::SSRResolutionModeCount);
    const uint32_t traceWidth = BE1::Max((width + (1u << resolutionMode) - 1u) >> resolutionMode, 1u);
    const uint32_t traceHeight = BE1::Max((height + (1u << resolutionMode) - 1u) >> resolutionMode, 1u);
    if (transparentSSRResolutionMode != resolutionMode) {
        transparentSSRTemporalHistoryValid = false;
        transparentSSRResolutionMode = resolutionMode;
    }
    const bool validDepthTextures = transparentSSRDepthFormat == depthFormat && transparentSSRDepthTextures[0] && transparentSSRDepthTextures[1] &&
        transparentSSRDepthTextures[0]->GetWidth() == width && transparentSSRDepthTextures[0]->GetHeight() == height &&
        transparentSSRDepthTextures[1]->GetWidth() == width && transparentSSRDepthTextures[1]->GetHeight() == height;
    const bool validSurfaceTextures = transparentSSRNormalRoughnessTexture && transparentSSRVelocityTexture &&
        transparentSSRNormalRoughnessTexture->GetWidth() == width && transparentSSRNormalRoughnessTexture->GetHeight() == height &&
        transparentSSRVelocityTexture->GetWidth() == width && transparentSSRVelocityTexture->GetHeight() == height;
    const bool validTemporalTextures = transparentSSRTemporalReflectionTextures[0] && transparentSSRTemporalReflectionTextures[1] &&
        transparentSSRTemporalReflectionTextures[0]->GetWidth() == traceWidth && transparentSSRTemporalReflectionTextures[0]->GetHeight() == traceHeight &&
        transparentSSRTemporalReflectionTextures[1]->GetWidth() == traceWidth && transparentSSRTemporalReflectionTextures[1]->GetHeight() == traceHeight;
    const bool validUpsampledReflectionTexture = transparentSSRUpsampledReflectionTexture &&
        transparentSSRUpsampledReflectionTexture->GetWidth() == width && transparentSSRUpsampledReflectionTexture->GetHeight() == height;

    if (!validDepthTextures || !validSurfaceTextures || !validTemporalTextures || !validUpsampledReflectionTexture) {
        DestroyTransparentSSRResources();
        if (!CreateTransparentSSRResources(width, height, depthFormat, resolutionMode)) {
            DestroyTransparentSSRResources();
            return false;
        }
    }

    return true;
}

bool RenderCamera::CreateTransparentSSRResources(uint32_t width, uint32_t height, RHI::Format depthFormat, uint32_t resolutionMode) {
    BE1::Image::Format depthImageFormat = BE1::Image::Format::Unknown;
    RHI::renderer->RHIFormatToImageFormat(depthFormat, &depthImageFormat, nullptr);
    SharedPtr<BE1::Image> depthImage = MakeShared<BE1::Image>();
    depthImage->Init2DFromMemory(width, height, 1, depthImageFormat, BE1::Image::GammaSpace::Linear, nullptr);
    const RHI::ClearValue depthClearValue = RHI::ClearValue::DepthStencil(desc.projectionPolicy.GetDepthClearValue(), 0);
    for (uint32_t index = 0; index < COUNT_OF(transparentSSRDepthTextures); ++index) {
        transparentSSRDepthTextures[index] = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::DepthStencil | RHI::TextureFlag::ShaderResource | RHI::TextureFlag::Typeless, depthImage, RHI::TextureMipInitMode::FromImage, depthClearValue, 1, RHI::GPUResourceState::ShaderResource);
        if (!transparentSSRDepthTextures[index] ||
            !RHI::renderer->CreateView(transparentSSRDepthTextures[index], RHI::ViewType::SRV).IsValid() ||
            !RHI::renderer->CreateView(transparentSSRDepthTextures[index], RHI::ViewType::DSV).IsValid()) {
            BE_WARNLOG("RenderCamera::CreateTransparentSSRResources: failed to create depth texture\n");
            return false;
        }
        RHI::renderer->SetName(transparentSSRDepthTextures[index], BE1::va("RenderCamera::transparentSSRDepthTextures[%u]", index));
    }

    SharedPtr<BE1::Image> normalRoughnessImage = MakeShared<BE1::Image>();
    // RG stores the UNORM octahedral normal; B stores perceptual roughness. Velocity has its own texture.
    normalRoughnessImage->Init2DFromMemory(width, height, 1, BE1::Image::Format::R10G10B10A2, BE1::Image::GammaSpace::Linear, nullptr);
    transparentSSRNormalRoughnessTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::RenderTarget | RHI::TextureFlag::ShaderResource, std::move(normalRoughnessImage), RHI::TextureMipInitMode::FromImage, RHI::ClearValue::Color(0.5f, 0.5f, 1.0f, 0.0f), 1, RHI::GPUResourceState::ShaderResource);
    if (!transparentSSRNormalRoughnessTexture) {
        BE_WARNLOG("RenderCamera::CreateTransparentSSRResources: failed to create normal/roughness texture\n");
        return false;
    }
    RHI::renderer->SetName(transparentSSRNormalRoughnessTexture, "RenderCamera::transparentSSRNormalRoughnessTexture");

    SharedPtr<BE1::Image> velocityImage = MakeShared<BE1::Image>();
    velocityImage->Init2DFromMemory(width, height, 1, BE1::Image::Format::R16G16_FLOAT, BE1::Image::GammaSpace::Linear, nullptr);
    transparentSSRVelocityTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::RenderTarget | RHI::TextureFlag::ShaderResource, std::move(velocityImage), RHI::TextureMipInitMode::FromImage, RHI::ClearValue::Color(0.0f, 0.0f, 0.0f, 0.0f), 1, RHI::GPUResourceState::ShaderResource);
    if (!transparentSSRVelocityTexture) {
        BE_WARNLOG("RenderCamera::CreateTransparentSSRResources: failed to create velocity texture\n");
        return false;
    }
    RHI::renderer->SetName(transparentSSRVelocityTexture, "RenderCamera::transparentSSRVelocityTexture");

    assert(resolutionMode < RenderFrameSettings::SSRResolutionModeCount);
    const uint32_t traceWidth = BE1::Max((width + (1u << resolutionMode) - 1u) >> resolutionMode, 1u);
    const uint32_t traceHeight = BE1::Max((height + (1u << resolutionMode) - 1u) >> resolutionMode, 1u);
    const BE1::Image::Format reflectionImageFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R16G16B16A16_FLOAT);
    SharedPtr<BE1::Image> historyImage = MakeShared<BE1::Image>();
    historyImage->Init2DFromMemory(traceWidth, traceHeight, 1, reflectionImageFormat, BE1::Image::GammaSpace::Linear, nullptr);
    for (uint32_t index = 0; index < COUNT_OF(transparentSSRTemporalReflectionTextures); ++index) {
        transparentSSRTemporalReflectionTextures[index] = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, historyImage, RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
        if (!transparentSSRTemporalReflectionTextures[index]) {
            BE_WARNLOG("RenderCamera::CreateTransparentSSRResources: failed to create temporal history texture\n");
            return false;
        }
        RHI::renderer->SetName(transparentSSRTemporalReflectionTextures[index], BE1::va("RenderCamera::transparentSSRTemporalReflectionTextures[%u]", index));
    }

    SharedPtr<BE1::Image> reflectionImage = MakeShared<BE1::Image>();
    reflectionImage->Init2DFromMemory(width, height, 1, reflectionImageFormat, BE1::Image::GammaSpace::Linear, nullptr);
    transparentSSRUpsampledReflectionTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, std::move(reflectionImage), RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!transparentSSRUpsampledReflectionTexture) {
        BE_WARNLOG("RenderCamera::CreateTransparentSSRResources: failed to create reflection texture\n");
        return false;
    }
    RHI::renderer->SetName(transparentSSRUpsampledReflectionTexture, "RenderCamera::transparentSSRUpsampledReflectionTexture");

    transparentSSRHistoryTextureIndex = 0;
    transparentSSRTemporalHistoryValid = false;
    transparentSSRDepthFormat = depthFormat;
    return true;
}

void RenderCamera::CommitTransparentSSRTemporalHistory(bool valid) {
    transparentSSRTemporalHistoryValid = valid;
    if (valid) {
        transparentSSRHistoryTextureIndex ^= 1u;
    }
}

void RenderCamera::DestroyTransparentSSRResources() {
    for (RHI::Texture *&texture : transparentSSRDepthTextures) {
        if (texture) {
            RHI::renderer->DestroyTexture(texture);
            texture = nullptr;
        }
    }

    if (transparentSSRNormalRoughnessTexture) {
        RHI::renderer->DestroyTexture(transparentSSRNormalRoughnessTexture);
        transparentSSRNormalRoughnessTexture = nullptr;
    }

    if (transparentSSRVelocityTexture) {
        RHI::renderer->DestroyTexture(transparentSSRVelocityTexture);
        transparentSSRVelocityTexture = nullptr;
    }

    for (RHI::Texture *&texture : transparentSSRTemporalReflectionTextures) {
        if (texture) {
            RHI::renderer->DestroyTexture(texture);
            texture = nullptr;
        }
    }

    if (transparentSSRUpsampledReflectionTexture) {
        RHI::renderer->DestroyTexture(transparentSSRUpsampledReflectionTexture);
        transparentSSRUpsampledReflectionTexture = nullptr;
    }
    transparentSSRHistoryTextureIndex = 0;
    transparentSSRTemporalHistoryValid = false;
    transparentSSRDepthFormat = RHI::Format::Unknown;
}

bool RenderCamera::CreateSSRTraceTexture(uint32_t width, uint32_t height) {
    const BE1::Image::Format imageFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R16G16B16A16_FLOAT);
    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Init2DFromMemory(width, height, 1, imageFormat, BE1::Image::GammaSpace::Linear, nullptr);

    ssrTraceTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, std::move(image), RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!ssrTraceTexture) {
        BE_WARNLOG("RenderCamera::CreateSSRTraceTexture: failed to create trace texture\n");
        return false;
    }

    RHI::renderer->SetName(ssrTraceTexture, "RenderCamera::ssrTraceTexture");
    return true;
}

// SSR receiver surface texture layout (trace resolution):
// RG: 현재 프레임의 화면상 ray 길이 (원본 픽셀 단위). FP16 비트의 하위/상위 바이트를 RG8 UNORM 에 저장한다.
// B: 1x1/2x2/4x4 블록에서 카메라에 가장 가까운 픽셀의 블록 내부 인덱스 / max(blockPixelCount - 1, 1).
// A: 출발 표면의 perceptual roughness.
bool RenderCamera::CreateSSRReceiverSurfaceTexture(uint32_t width, uint32_t height) {
    const BE1::Image::Format imageFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R8G8B8A8);
    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Init2DFromMemory(width, height, 1, imageFormat, BE1::Image::GammaSpace::Linear, nullptr);

    ssrReceiverSurfaceTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, std::move(image), RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!ssrReceiverSurfaceTexture) {
        BE_WARNLOG("RenderCamera::CreateSSRReceiverSurfaceTexture: failed to create receiver surface texture\n");
        return false;
    }

    RHI::renderer->SetName(ssrReceiverSurfaceTexture, "RenderCamera::ssrReceiverSurfaceTexture");
    return true;
}

bool RenderCamera::CreateSSRResolveReflectionTexture(uint32_t width, uint32_t height) {
    const BE1::Image::Format imageFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R16G16B16A16_FLOAT);
    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Init2DFromMemory(width, height, 1, imageFormat, BE1::Image::GammaSpace::Linear, nullptr);

    ssrResolveReflectionTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, std::move(image), RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!ssrResolveReflectionTexture) {
        BE_WARNLOG("RenderCamera::CreateSSRResolveReflectionTexture: failed to create trace-resolution reflection texture\n");
        return false;
    }

    RHI::renderer->SetName(ssrResolveReflectionTexture, "RenderCamera::ssrResolveReflectionTexture");
    return true;
}

bool RenderCamera::CreateSSRTemporalReflectionTextures(uint32_t width, uint32_t height) {
    const BE1::Image::Format imageFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R16G16B16A16_FLOAT);
    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Init2DFromMemory(width, height, 1, imageFormat, BE1::Image::GammaSpace::Linear, nullptr);

    for (uint32_t index = 0; index < COUNT_OF(ssrTemporalReflectionTextures); ++index) {
        ssrTemporalReflectionTextures[index] = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
        if (!ssrTemporalReflectionTextures[index]) {
            BE_WARNLOG("RenderCamera::CreateSSRTemporalReflectionTextures: failed to create temporal reflection texture\n");
            return false;
        }

        RHI::renderer->SetName(ssrTemporalReflectionTextures[index], BE1::va("RenderCamera::ssrTemporalReflectionTextures[%u]", index));
    }

    ssrTemporalReflectionTextureIndex = 0;
    ssrTemporalHistoryValid = false;
    return true;
}

void RenderCamera::CommitSSRTemporalHistory(bool valid) {
    ssrTemporalHistoryValid = valid;
    if (valid) {
        ssrTemporalReflectionTextureIndex ^= 1u;
    }
}

bool RenderCamera::CreateSSRUpsampledReflectionTexture(uint32_t width, uint32_t height) {
    const BE1::Image::Format imageFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R16G16B16A16_FLOAT);
    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Init2DFromMemory(width, height, 1, imageFormat, BE1::Image::GammaSpace::Linear, nullptr);

    ssrUpsampledReflectionTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, std::move(image), RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!ssrUpsampledReflectionTexture) {
        BE_WARNLOG("RenderCamera::CreateSSRUpsampledReflectionTexture: failed to create full-resolution reflection texture\n");
        return false;
    }

    RHI::renderer->SetName(ssrUpsampledReflectionTexture, "RenderCamera::ssrUpsampledReflectionTexture");
    return true;
}

void RenderCamera::DestroySSRResources() {
    if (ssrTraceTexture) {
        RHI::renderer->DestroyTexture(ssrTraceTexture);
        ssrTraceTexture = nullptr;
    }

    if (ssrReceiverSurfaceTexture) {
        RHI::renderer->DestroyTexture(ssrReceiverSurfaceTexture);
        ssrReceiverSurfaceTexture = nullptr;
    }

    if (ssrResolveReflectionTexture) {
        RHI::renderer->DestroyTexture(ssrResolveReflectionTexture);
        ssrResolveReflectionTexture = nullptr;
    }

    for (RHI::Texture *&texture : ssrTemporalReflectionTextures) {
        if (texture) {
            RHI::renderer->DestroyTexture(texture);
            texture = nullptr;
        }
    }

    ssrTemporalReflectionTextureIndex = 0;
    ssrTemporalHistoryValid = false;
    if (ssrUpsampledReflectionTexture) {
        RHI::renderer->DestroyTexture(ssrUpsampledReflectionTexture);
        ssrUpsampledReflectionTexture = nullptr;
    }
}

// MSAA AO/SSR share a full-resolution resolve target, independent of their own resources.
bool RenderCamera::EnsureResolvedDepthTexture(uint32_t width, uint32_t height, uint32_t sampleCount, bool needed) {
    if (!needed || sampleCount <= 1 || width == 0 || height == 0) {
        DestroyResolvedDepthTexture();
        return false;
    }

    if (!resolvedDepthTexture || resolvedDepthTexture->GetWidth() != width || resolvedDepthTexture->GetHeight() != height) {
        DestroyResolvedDepthTexture();
        return CreateResolvedDepthTexture(width, height);
    }

    return true;
}

bool RenderCamera::CreateResolvedDepthTexture(uint32_t width, uint32_t height) {
    const BE1::Image::Format imageFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R32_FLOAT);
    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Init2DFromMemory(width, height, 1, imageFormat, BE1::Image::GammaSpace::Linear, nullptr);

    resolvedDepthTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, std::move(image), RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!resolvedDepthTexture) {
        BE_WARNLOG("RenderCamera::CreateResolvedDepthTexture: failed to create resolved depth texture\n");
        return false;
    }

    RHI::renderer->SetName(resolvedDepthTexture, "RenderCamera::resolvedDepthTexture");
    return true;
}

void RenderCamera::DestroyResolvedDepthTexture() {
    if (resolvedDepthTexture) {
        RHI::renderer->DestroyTexture(resolvedDepthTexture);
        resolvedDepthTexture = nullptr;
    }
}

bool RenderCamera::CreatePostProcessColorTexture(uint32_t width, uint32_t height, RHI::Format colorFormat) {
    DestroyPostProcessColorTexture();

    if (width == 0 || height == 0 || colorFormat == RHI::Format::Unknown) {
        return false;
    }

    BE1::Image::Format postProcessColorImageFormat = BE1::Image::Format::Unknown;
    if (!RHI::Renderer::RHIFormatToImageFormat(colorFormat, &postProcessColorImageFormat, nullptr)) {
        return false;
    }

    SharedPtr<BE1::Image> postProcessColorImage = MakeShared<BE1::Image>();
    postProcessColorImage->Init2DFromMemory(width, height, 1, postProcessColorImageFormat, BE1::Image::GammaSpace::Linear, nullptr);
    postProcessColorTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, std::move(postProcessColorImage), RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!postProcessColorTexture) {
        BE_WARNLOG("RenderCamera::CreatePostProcessColorTexture: failed to create post process color texture\n");
        return false;
    }

    RHI::renderer->SetName(postProcessColorTexture, "RenderCamera::postProcessColorTexture");
    return true;
}

void RenderCamera::DestroyPostProcessColorTexture() {
    if (postProcessColorTexture) {
        RHI::renderer->DestroyTexture(postProcessColorTexture);
        postProcessColorTexture = nullptr;
    }
}

bool RenderCamera::EnsureColorHistoryTextures(uint32_t width, uint32_t height, RHI::Format colorFormat) {
    if (colorHistoryTextures[0] && colorHistoryTextures[1] &&
        colorHistoryTextures[0]->GetWidth() == width && colorHistoryTextures[0]->GetHeight() == height && colorHistoryTextures[0]->GetFormat() == colorFormat &&
        colorHistoryTextures[1]->GetWidth() == width && colorHistoryTextures[1]->GetHeight() == height && colorHistoryTextures[1]->GetFormat() == colorFormat) {
        return true;
    }

    return CreateColorHistoryTextures(width, height, colorFormat);
}

bool RenderCamera::CreateColorHistoryTextures(uint32_t width, uint32_t height, RHI::Format colorFormat) {
    DestroyColorHistoryTextures();

    if (width == 0 || height == 0) {
        return false;
    }

    BE1::Image::Format imageFormat = BE1::Image::Format::Unknown;
    if (!RHI::Renderer::RHIFormatToImageFormat(colorFormat, &imageFormat, nullptr)) {
        BE_WARNLOG("RenderCamera::CreateColorHistoryTextures: failed to convert color history texture format\n");
        return false;
    }

    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Init2DFromMemory(width, height, 1, imageFormat, BE1::Image::GammaSpace::Linear, nullptr);

    for (uint32_t textureIndex = 0; textureIndex < 2; ++textureIndex) {
        colorHistoryTextures[textureIndex] = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
        if (!colorHistoryTextures[textureIndex]) {
            BE_WARNLOG("RenderCamera::CreateColorHistoryTextures: failed to create color history texture %i\n", textureIndex);
            DestroyColorHistoryTextures();
            return false;
        }
        RHI::renderer->SetName(colorHistoryTextures[textureIndex], BE1::va("RenderCamera::colorHistoryTextures[%i]", textureIndex));
    }

    currentHistoryTextureIndex = 0;
    historyValid = false;

    return true;
}

void RenderCamera::DestroyColorHistoryTextures() {
    for (uint32_t textureIndex = 0; textureIndex < 2; ++textureIndex) {
        if (colorHistoryTextures[textureIndex]) {
            RHI::renderer->DestroyTexture(colorHistoryTextures[textureIndex]);
            colorHistoryTextures[textureIndex] = nullptr;
        }
    }

    currentHistoryTextureIndex = 0;
    historyValid = false;
}

bool RenderCamera::EnsureDepthHistoryResolveTargets(uint32_t width, uint32_t height) {
    if (depthHistoryTextures[0] && depthHistoryTextures[1] &&
        depthHistoryTextures[0]->GetWidth() == width && depthHistoryTextures[0]->GetHeight() == height &&
        depthHistoryTextures[1]->GetWidth() == width && depthHistoryTextures[1]->GetHeight() == height) {
        return true;
    }

    return CreateDepthHistoryResolveTargets(width, height);
}

// MSAA on 상태에서 TAA 가 사용할 최종 depth resolve 대상으로 depthHistoryTextures (SRV/UAV) 를 생성한다.
bool RenderCamera::CreateDepthHistoryResolveTargets(uint32_t width, uint32_t height) {
    DestroyDepthHistoryTextures();

    if (width == 0 || height == 0) {
        return false;
    }

    const BE1::Image::Format imageFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R32_FLOAT);
    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Init2DFromMemory(width, height, 1, imageFormat, BE1::Image::GammaSpace::Linear, nullptr);

    for (uint32_t textureIndex = 0; textureIndex < 2; ++textureIndex) {
        depthHistoryTextures[textureIndex] = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
        if (!depthHistoryTextures[textureIndex]) {
            BE_WARNLOG("RenderCamera::CreateDepthHistoryResolveTargets: failed to create depth history resolve target %i\n", textureIndex);
            DestroyDepthHistoryTextures();
            return false;
        }
        RHI::renderer->SetName(depthHistoryTextures[textureIndex], BE1::va("RenderCamera::depthHistoryTextures[%i]", textureIndex));
    }

    currentHistoryTextureIndex = 0;
    historyValid = false;

    return true;
}

// depthHistoryTextures 를 depth attachment 용으로 생성한다.
bool RenderCamera::CreateDepthHistoryRenderTargets(uint32_t width, uint32_t height, RHI::Format depthFormat) {
    DestroyDepthHistoryTextures();

    if (width == 0 || height == 0) {
        return false;
    }

    BE1::Image::Format imageFormat = BE1::Image::Format::Unknown;
    if (!RHI::Renderer::RHIFormatToImageFormat(depthFormat, &imageFormat, nullptr)) {
        BE_WARNLOG("RenderCamera::CreateDepthHistoryRenderTargets: failed to convert depth texture format\n");
        return false;
    }

    const RHI::ClearValue optimizedClearValue = RHI::ClearValue::DepthStencil(desc.projectionPolicy.GetDepthClearValue(), 0);
    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Init2DFromMemory(width, height, 1, imageFormat, BE1::Image::GammaSpace::Linear, nullptr);

    for (uint32_t textureIndex = 0; textureIndex < 2; ++textureIndex) {
        depthHistoryTextures[textureIndex] = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::DepthStencil | RHI::TextureFlag::ShaderResource | RHI::TextureFlag::Typeless, image, RHI::TextureMipInitMode::FromImage, optimizedClearValue, 1, RHI::GPUResourceState::ShaderResource);
        if (!depthHistoryTextures[textureIndex] ||
            !RHI::renderer->CreateView(depthHistoryTextures[textureIndex], RHI::ViewType::SRV).IsValid() ||
            !RHI::renderer->CreateView(depthHistoryTextures[textureIndex], RHI::ViewType::DSV).IsValid()) {
            BE_WARNLOG("RenderCamera::CreateDepthHistoryRenderTargets: failed to create depth history texture %i\n", textureIndex);
            DestroyDepthHistoryTextures();
            return false;
        }
        RHI::renderer->SetName(depthHistoryTextures[textureIndex], BE1::va("RenderCamera::depthHistoryTextures[%i]", textureIndex));
    }

    depthHistoryAsRenderTarget = true;
    currentHistoryTextureIndex = 0;
    historyValid = false;

    return true;
}

void RenderCamera::DestroyDepthHistoryTextures() {
    for (uint32_t textureIndex = 0; textureIndex < 2; ++textureIndex) {
        if (depthHistoryTextures[textureIndex]) {
            RHI::renderer->DestroyTexture(depthHistoryTextures[textureIndex]);
            depthHistoryTextures[textureIndex] = nullptr;
        }
    }

    depthHistoryAsRenderTarget = false;
    currentHistoryTextureIndex = 0;
    historyValid = false;
}

void RenderCamera::CreateLuminanceBuffer() {
    DestroyLuminanceBuffer();

    // Luminance buffer : exposure (float) + luminance (float) + histogram (uint32_t * 1024)
    static constexpr uint32_t luminanceBufferSize = LuminanceBufferOffsetHistogram + LuminanceHistogramBinCount * sizeof(uint32_t);
    static constexpr uint32_t luminanceBufferDwordCount = luminanceBufferSize / sizeof(uint32_t);
    const uint32_t initialLuminanceBufferData[luminanceBufferDwordCount] = {};

    luminanceBuffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::ShaderResource | RHI::BufferFlag::UnorderedAccess, luminanceBufferSize, initialLuminanceBufferData);
    if (!luminanceBuffer) {
        BE_WARNLOG("RenderCamera::CreateLuminanceBuffer: failed to create luminance buffer\n");
        return;
    }

    RHI::renderer->SetName(luminanceBuffer, "RenderCamera::luminanceBuffer");

    RHI::ViewID srvViewID = RHI::renderer->CreateView(luminanceBuffer, RHI::ViewType::SRV, RHI::Format::Unknown, 0, 0, luminanceBufferSize);
    if (!srvViewID.IsValid()) {
        BE_WARNLOG("RenderCamera::CreateLuminanceBuffer: failed to create luminance SRV\n");
        RHI::renderer->DestroyBuffer(luminanceBuffer);
        luminanceBuffer = nullptr;
        return;
    }

    RHI::ViewID uavViewID = RHI::renderer->CreateView(luminanceBuffer, RHI::ViewType::UAV, RHI::Format::Unknown, 0, 0, luminanceBufferSize);
    if (!uavViewID.IsValid()) {
        BE_WARNLOG("RenderCamera::CreateLuminanceBuffer: failed to create luminance UAV\n");
        RHI::renderer->DestroyBuffer(luminanceBuffer);
        luminanceBuffer = nullptr;
        return;
    }
}

void RenderCamera::DestroyLuminanceBuffer() {
    if (luminanceBuffer) {
        RHI::renderer->DestroyBuffer(luminanceBuffer);
        luminanceBuffer = nullptr;
    }
}
