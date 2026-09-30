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
#include "Render/RenderUtils.h"
#include "Math/Halton.h"
#include "RHI/RHI.h"
#include "Shaders/ShaderInterop.h"

RenderCamera::~RenderCamera() {
    DestroyPostProcessResources();
    screenSpaceAOResources.Destroy();
    DestroySSRTraceTexture();
    DestroyResolvedDepthTexture();
    screenSpaceDepthPyramidResources.Destroy();
    hiZResources.Destroy();
    DestroyRenderTextures();
}

void RenderCamera::Update(const Desc &desc) {
    this->desc = desc;

    const bool zNegativeOneToOne = desc.projectionPolicy.IsZNegativeOneToOne();
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
        projMatrix.SetOrthoRH(-desc.sizeX, desc.sizeX, -desc.sizeY, desc.sizeY, desc.zNear, desc.zFar, zNegativeOneToOne, reverseZ);
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
        projMatrix.SetPerspectiveRH(desc.fovY, aspectRatio, desc.zNear, desc.zFar, zNegativeOneToOne, reverseZ, desc.infiniteZFar);
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
        normalizedDeviceCoords.z = clipCoords.z * invW; // Clipping range is [-1, 1] or [0, 1] in D3D
        return true;
    }
    return false;
}

void RenderCamera::TransformNDCToPixel(const BE1::Vec3 &normalizedDeviceCoords, BE1::Vec3 &pixelCoords) const {
    const float fx = (normalizedDeviceCoords.x + 1.0f) * 0.5f; // Valid range is [0, 1]
    const float fy = 1.0f - (normalizedDeviceCoords.y + 1.0f) * 0.5f; // Valid range is [0, 1]
    const float fz = desc.projectionPolicy.NDCZToDeviceDepth(normalizedDeviceCoords.z); // Valid range is [0, 1]

    pixelCoords.x = desc.outputRect.x + fx * desc.outputRect.w;
    pixelCoords.y = desc.outputRect.y + fy * desc.outputRect.h;
    pixelCoords.z = fz; // device depth value
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
    const float fz = pixelCoords.z; // device depth value

    normalizedDeviceCoords.x = (fx * 2.0f) - 1.0f;
    normalizedDeviceCoords.y = (fy * 2.0f) - 1.0f;
    normalizedDeviceCoords.z = desc.projectionPolicy.DeviceDepthToNDCZ(fz);
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
    const float ndcMin = desc.projectionPolicy.IsZNegativeOneToOne() ? -1.0f : 0.0f;
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

    *depthMin = desc.projectionPolicy.NDCZToDeviceDepth(localMin);
    *depthMax = desc.projectionPolicy.NDCZToDeviceDepth(localMax);
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
bool RenderCamera::EnsureRenderTextures(uint32_t width, uint32_t height, RHI::Format colorFormat, RHI::Format depthFormat, uint32_t sampleCount, bool needDepthHistory, bool needNormalTexture, bool needVelocityTexture, const BE1::Color4 &optimizedClearColor) {
    sampleCount = BE1::Max(sampleCount, 1u);

    const bool useDepthHistoryRenderTarget = needDepthHistory && sampleCount == 1;
    const bool hasExpectedDepthTextures = useDepthHistoryRenderTarget ? (depthHistoryAsRenderTarget && depthHistoryTextures[0] && depthHistoryTextures[1] && !depthTexture) : (!depthHistoryAsRenderTarget && depthTexture);
    const bool hasExpectedNormalTextures = (normalTexture != nullptr) == needNormalTexture && (resolvedNormalTexture != nullptr) == (needNormalTexture && sampleCount > 1);
    const bool hasExpectedVelocityTextures = (velocityTexture != nullptr) == needVelocityTexture && (resolvedVelocityTexture != nullptr) == (needVelocityTexture && sampleCount > 1);
    const bool hasExpectedResolvedColorTexture = sampleCount > 1 ? resolvedColorTexture != nullptr : resolvedColorTexture == nullptr;

    if (colorTexture &&
        hasExpectedResolvedColorTexture &&
        hasExpectedDepthTextures &&
        hasExpectedNormalTextures &&
        hasExpectedVelocityTextures &&
        renderWidth == width && renderHeight == height &&
        renderColorFormat == colorFormat &&
        renderDepthFormat == depthFormat &&
        renderSampleCount == sampleCount) {
        return true;
    }

    // 렌더 텍스처 재생성 후 이전 내용은 사용하지 않는다.
    currentHistoryTextureIndex = 0;
    historyValid = false;

    if (!CreateRenderTextures(width, height, colorFormat, depthFormat, sampleCount, needDepthHistory, needNormalTexture, needVelocityTexture, optimizedClearColor)) {
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

    currentJitter = BE1::Vec2::zero;
    previousJitter = BE1::Vec2::zero;
    previousJitteredViewProjMatrix = viewProjMatrix;
    previousJitteredInvViewProjMatrix = invViewProjMatrix;
    hiZResources.SetHistoryValid(false);
}

void RenderCamera::UpdateTemporalJitter(bool enableJitter, uint32_t sampleCount) {
    static constexpr uint32_t MaxTemporalJitterSampleCount = 64;

    const bool requestedJitter = enableJitter && sampleCount > 1;
    const uint32_t requestedSampleCount = requestedJitter ? BE1::Min(sampleCount, MaxTemporalJitterSampleCount) : 0;
    const bool jitterSettingChanged = temporalJitterActive != requestedJitter || temporalJitterSampleCount != requestedSampleCount;

    if (jitterSettingChanged) {
        temporalHistoryInvalidated = true;
        temporalFrameIndex = 0;
        previousJitter = BE1::Vec2::zero;
        previousJitteredViewProjMatrix = viewProjMatrix;
        previousJitteredInvViewProjMatrix = invViewProjMatrix;
    }

    temporalJitterActive = requestedJitter;
    temporalJitterSampleCount = requestedSampleCount;

    if (!requestedJitter) {
        currentJitter = BE1::Vec2::zero;
        previousJitter = BE1::Vec2::zero;
        temporalFrameIndex = 0;

        jitteredViewProjMatrix = viewProjMatrix;
        jitteredInvViewProjMatrix = invViewProjMatrix;
        return;
    }

    // 이전 프레임의 jitter offset 을 저장한다.
    previousJitter = jitterSettingChanged ? BE1::Vec2::zero : currentJitter;

    // deterministic offset 이 Vec2::zero 이므로 0 번 샘플은 (0, 0) 이다.
    // jitter 로 변환하면 (-0.5, -0.5) 이 되어 최대 offset 이 되므로 1 번 샘플부터 사용한다.
    static const BE1::Halton<MaxTemporalJitterSampleCount + 1> jitterSamples(2, 3, BE1::Vec2::zero);
    const uint32_t sampleIndex = (temporalFrameIndex % temporalJitterSampleCount) + 1;
    const BE1::Vec2 &sample = jitterSamples.GetSample(sampleIndex);

    currentJitter = BE1::Vec2(sample.x - 0.5f, sample.y - 0.5f);

    assert(renderWidth > 0 && renderHeight > 0);
    const float ndcJitterX = currentJitter.x * (2.0f / static_cast<float>(renderWidth));
    const float ndcJitterY = currentJitter.y * (-2.0f / static_cast<float>(renderHeight));

    jitteredViewProjMatrix = viewProjMatrix;

    // newClip.x = (row0 + row3 * ndcJitterX) dot position
    // newClip.y = (row1 + row3 * ndcJitterY) dot position
    // newMatrixRow0 = row0 + row3 * ndcJitterX
    // newMatrixRow1 = row1 + row3 * ndcJitterY
    for (int i = 0; i < 4; ++i) {
        jitteredViewProjMatrix[0][i] += viewProjMatrix[3][i] * ndcJitterX;
        jitteredViewProjMatrix[1][i] += viewProjMatrix[3][i] * ndcJitterY;
    }

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

bool RenderCamera::CreateRenderTextures(uint32_t width, uint32_t height, RHI::Format colorFormat, RHI::Format depthFormat, uint32_t sampleCount, bool needDepthHistory, bool needNormalTexture, bool needVelocityTexture, const BE1::Color4 &optimizedClearColor) {
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

    if (needNormalTexture && !CreateNormalTextures(width, height, sampleCount)) {
        return false;
    }

    if (needVelocityTexture && !CreateVelocityTextures(width, height, sampleCount)) {
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
    if (normalTexture) {
        RHI::renderer->DestroyTexture(normalTexture);
        normalTexture = nullptr;
    }
    if (resolvedNormalTexture) {
        RHI::renderer->DestroyTexture(resolvedNormalTexture);
        resolvedNormalTexture = nullptr;
    }
    if (velocityTexture) {
        RHI::renderer->DestroyTexture(velocityTexture);
        velocityTexture = nullptr;
    }
    if (resolvedVelocityTexture) {
        RHI::renderer->DestroyTexture(resolvedVelocityTexture);
        resolvedVelocityTexture = nullptr;
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

    const RHI::ClearValue optimizedClearValue = RHI::ClearValue::Color(optimizedClearColor[0], optimizedClearColor[1], optimizedClearColor[2], optimizedClearColor[3]);

    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Init2DFromMemory(width, height, 1, imageFormat, BE1::Image::GammaSpace::Linear, nullptr);

    const bool useMSAA = sampleCount > 1;
    RHI::TextureFlag textureFlags = RHI::TextureFlag::RenderTarget;
    if (!useMSAA) {
        textureFlags |= RHI::TextureFlag::ShaderResource;
    }

    colorTexture = RHI::renderer->CreateTexture2D(textureFlags, image, RHI::TextureMipInitMode::FromImage, optimizedClearValue, sampleCount, RHI::GPUResourceState::ShaderResource);
    if (!colorTexture) {
        DestroyRenderTextures();
        BE_WARNLOG("RenderCamera::CreateColorTextures: failed to create color texture\n");
        return false;
    }
    RHI::renderer->SetName(colorTexture, "RenderCamera::colorTexture");

    if (useMSAA) {
        resolvedColorTexture = RHI::renderer->CreateTexture2D(RHI::TextureFlag::ShaderResource, image, RHI::TextureMipInitMode::FromImage, optimizedClearValue, 1, RHI::GPUResourceState::ShaderResource);
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
    depthTexture = RHI::renderer->CreateTexture2D(RHI::TextureFlag::DepthStencil | RHI::TextureFlag::ShaderResource | RHI::TextureFlag::Typeless, image, RHI::TextureMipInitMode::FromImage, optimizedClearValue, sampleCount, RHI::GPUResourceState::ShaderResource);
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

bool RenderCamera::CreateNormalTextures(uint32_t width, uint32_t height, uint32_t sampleCount) {
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

    normalTexture = RHI::renderer->CreateTexture2D(textureFlags, image, RHI::TextureMipInitMode::FromImage, optimizedClearValue, sampleCount, RHI::GPUResourceState::ShaderResource);
    if (!normalTexture) {
        BE_WARNLOG("RenderCamera::CreateNormalTextures: failed to create normal texture\n");
        DestroyRenderTextures();
        return false;
    }
    RHI::renderer->SetName(normalTexture, "RenderCamera::normalTexture");

    if (useMSAA) {
        resolvedNormalTexture = RHI::renderer->CreateTexture2D(RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::FromImage, optimizedClearValue, 1, RHI::GPUResourceState::ShaderResource);
        if (!resolvedNormalTexture) {
            BE_WARNLOG("RenderCamera::CreateNormalTextures: failed to create resolved normal texture\n");
            DestroyRenderTextures();
            return false;
        }
        RHI::renderer->SetName(resolvedNormalTexture, "RenderCamera::resolvedNormalTexture");
    }
    return true;
}

bool RenderCamera::CreateVelocityTextures(uint32_t width, uint32_t height, uint32_t sampleCount) {
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

    velocityTexture = RHI::renderer->CreateTexture2D(textureFlags, image, RHI::TextureMipInitMode::FromImage, optimizedClearValue, sampleCount, RHI::GPUResourceState::ShaderResource);
    if (!velocityTexture) {
        BE_WARNLOG("RenderCamera::CreateVelocityTextures: failed to create velocity texture\n");
        DestroyRenderTextures();
        return false;
    }
    RHI::renderer->SetName(velocityTexture, "RenderCamera::velocityTexture");

    if (useMSAA) {
        resolvedVelocityTexture = RHI::renderer->CreateTexture2D(RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::FromImage, optimizedClearValue, 1, RHI::GPUResourceState::ShaderResource);
        if (!resolvedVelocityTexture) {
            BE_WARNLOG("RenderCamera::CreateVelocityTextures: failed to create resolved velocity texture\n");
            DestroyRenderTextures();
            return false;
        }
        RHI::renderer->SetName(resolvedVelocityTexture, "RenderCamera::resolvedVelocityTexture");
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
    const RHI::TextureFlag snapshotColorFlags = halfResolution ? RHI::TextureFlag::RenderTarget | RHI::TextureFlag::ShaderResource : RHI::TextureFlag::ShaderResource;

    SharedPtr<BE1::Image> colorImage = MakeShared<BE1::Image>();
    colorImage->Init2DFromMemory(snapshotWidth, snapshotHeight, 1, colorImageFormat, BE1::Image::GammaSpace::Linear, nullptr);
    snapshotColorTexture = RHI::renderer->CreateTexture2D(snapshotColorFlags, colorImage, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!snapshotColorTexture) {
        BE_WARNLOG("RenderCamera::CreateSceneSnapshotResources: failed to create scene color texture\n");
        return false;
    }
    RHI::renderer->SetName(snapshotColorTexture, "RenderCamera::snapshotColorTexture");

    if (halfResolution) {
        SharedPtr<BE1::Image> depthImage = MakeShared<BE1::Image>();
        depthImage->Init2DFromMemory(snapshotWidth, snapshotHeight, 1, BE1::Image::Format::R32_FLOAT, BE1::Image::GammaSpace::Linear, nullptr);
        snapshotDepthTexture = RHI::renderer->CreateTexture2D(RHI::TextureFlag::RenderTarget | RHI::TextureFlag::ShaderResource | RHI::TextureFlag::SkipDefaultViews, depthImage, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
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
        snapshotDepthTexture = RHI::renderer->CreateTexture2D(RHI::TextureFlag::DepthStencil | RHI::TextureFlag::ShaderResource | RHI::TextureFlag::Typeless, depthImage, RHI::TextureMipInitMode::FromImage, depthClearValue, 1, RHI::GPUResourceState::ShaderResource);
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

void RenderCamera::EnsureSSRTraceResources(uint32_t width, uint32_t height, bool needed) {
    if (!needed || width == 0 || height == 0) {
        DestroySSRTraceTexture();
        return;
    }

    const uint32_t traceWidth = BE1::Max((width + 1u) >> 1, 1u);
    const uint32_t traceHeight = BE1::Max((height + 1u) >> 1, 1u);

    if (!ssrTraceTexture || ssrTraceTexture->GetWidth() != traceWidth || ssrTraceTexture->GetHeight() != traceHeight) {
        DestroySSRTraceTexture();

        CreateSSRTraceTexture(traceWidth, traceHeight);
    }
}

bool RenderCamera::CreateSSRTraceTexture(uint32_t width, uint32_t height) {
    const BE1::Image::Format imageFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R16G16B16A16_FLOAT);
    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Init2DFromMemory(width, height, 1, imageFormat, BE1::Image::GammaSpace::Linear, nullptr);

    ssrTraceTexture = RHI::renderer->CreateTexture2D(RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!ssrTraceTexture) {
        BE_WARNLOG("RenderCamera::CreateSSRTraceTexture: failed to create trace texture\n");
        return false;
    }

    RHI::renderer->SetName(ssrTraceTexture, "RenderCamera::ssrTraceTexture");
    return true;
}

void RenderCamera::DestroySSRTraceTexture() {
    if (ssrTraceTexture) {
        RHI::renderer->DestroyTexture(ssrTraceTexture);
        ssrTraceTexture = nullptr;
    }
}

// MSAA AO/SSR share a full-resolution resolve target, independent of their own resources.
void RenderCamera::EnsureResolvedDepthTexture(uint32_t width, uint32_t height, uint32_t sampleCount, bool needed) {
    if (!needed || sampleCount <= 1 || width == 0 || height == 0) {
        DestroyResolvedDepthTexture();
        return;
    }

    if (!resolvedDepthTexture || resolvedDepthTexture->GetWidth() != width || resolvedDepthTexture->GetHeight() != height) {
        DestroyResolvedDepthTexture();
        CreateResolvedDepthTexture(width, height);
    }
}

bool RenderCamera::CreateResolvedDepthTexture(uint32_t width, uint32_t height) {
    const BE1::Image::Format imageFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R32_FLOAT);
    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Init2DFromMemory(width, height, 1, imageFormat, BE1::Image::GammaSpace::Linear, nullptr);

    resolvedDepthTexture = RHI::renderer->CreateTexture2D(RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
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
    postProcessColorTexture = RHI::renderer->CreateTexture2D(RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, postProcessColorImage, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
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
        colorHistoryTextures[textureIndex] = RHI::renderer->CreateTexture2D(RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
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
        depthHistoryTextures[textureIndex] = RHI::renderer->CreateTexture2D(RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
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
        depthHistoryTextures[textureIndex] = RHI::renderer->CreateTexture2D(RHI::TextureFlag::DepthStencil | RHI::TextureFlag::ShaderResource | RHI::TextureFlag::Typeless, image, RHI::TextureMipInitMode::FromImage, optimizedClearValue, 1, RHI::GPUResourceState::ShaderResource);
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
