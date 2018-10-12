// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Render/Render.h"
#include "RenderInternal.h"

BE_NAMESPACE_BEGIN

void RenderView::Update(const State *stateCopy) {
    state = *stateCopy;

    // near/far
    zNear = state.zNear;
    zFar = state.zFar;

    if (state.orthogonal) {
        // Set bounding volume for orthogonal view
        Vec3 extents((zFar - zNear) * 0.5f, state.sizeX, state.sizeY);
        box.SetCenter(state.origin + state.axis[0] * (zNear + extents[0]));
        box.SetExtents(extents);
        box.SetAxis(state.axis);

        // Calculate orthogonal projection matrix
        R_SetOrthogonalProjectionMatrix(state.sizeX, state.sizeY, zNear, zFar, projMatrix);
    } else {
        // Set bounding frustum for perspective view
        frustum.SetOrigin(state.origin);
        frustum.SetAxis(state.axis);
        frustum.SetSize(zNear, zFar, zFar * Math::Tan(DEG2RAD(state.fovX) * 0.5f), zFar * Math::Tan(DEG2RAD(state.fovY) * 0.5f));

        // Calculate view frustum planes
        frustum.ToPlanes(frustumPlanes);

        // Calculate view frustum points
        frustum.ToPoints(frustumPoints);

        // Calculate perspective projection matrix
        R_SetPerspectiveProjectionMatrix(state.fovX, state.fovY, zNear, zFar, false, projMatrix);
    }

    // Calculate view matrix
    R_SetViewMatrix(state.axis, state.origin, viewMatrix);

    // Calculate view projection matrix
    viewProjMatrix = projMatrix * viewMatrix;
}

void RenderView::RecalcZFar(float zFar) {
    this->zFar = zFar;

    if (state.orthogonal) {
        // Set bounding volume for orthogonal view
        Vec3 extents((zFar - zNear) * 0.5f, state.sizeX, state.sizeY);
        box.SetCenter(state.origin + state.axis[0] * (zNear + extents[0]));
        box.SetExtents(extents);

        // Calculate orthogonal projection matrix
        R_SetOrthogonalProjectionMatrix(state.sizeX, state.sizeY, zNear, zFar, projMatrix);
    } else {
        // Set bounding frustum for perspective view
        frustum.SetSize(zNear, zFar, zFar * Math::Tan(DEG2RAD(state.fovX) * 0.5f), zFar * Math::Tan(DEG2RAD(state.fovY) * 0.5f));

        // Calculate view frustum planes
        frustum.ToPlanes(frustumPlanes);

        // Calculate view frustum points
        frustum.ToPoints(frustumPoints);

        // Calculate perspective projection matrix
        R_SetPerspectiveProjectionMatrix(state.fovX, state.fovY, zNear, zFar, false, projMatrix);
    }

    // Re-calculate view projection matrix
    viewProjMatrix = projMatrix * viewMatrix;
}

Vec4 RenderView::WorldToNormalizedDevice(const Vec3 &worldPosition) const {
    return viewProjMatrix * Vec4(worldPosition, 1.0f);
}

bool RenderView::NormalizedDeviceToPixel(const Vec4 &normalizedDeviceCoords, Vec3 &pixelCoords) const {
    if (normalizedDeviceCoords.w > 0) {
        float invW = 1.0f / normalizedDeviceCoords.w;

        float fx = (normalizedDeviceCoords.x * invW + 1.0f) * 0.5f;
        float fy = (normalizedDeviceCoords.y * invW + 1.0f) * 0.5f;

        fy = 1.0f - fy; // invert Y axis

        pixelCoords.x = fx * (state.renderRect.x + state.renderRect.w);
        pixelCoords.y = fy * (state.renderRect.y + state.renderRect.h);
        pixelCoords.z = normalizedDeviceCoords.z * invW; // depth value
        return true;
    } else {
        return false;
    }
}

bool RenderView::NormalizedDeviceToPixel(const Vec4 &normalizedDeviceCoords, Point &pixelPoint) const {
    if (normalizedDeviceCoords.w > 0) {
        float invW = 1.0f / normalizedDeviceCoords.w;

        float fx = (normalizedDeviceCoords.x * invW + 1.0f) * 0.5f;
        float fy = (normalizedDeviceCoords.y * invW + 1.0f) * 0.5f;

        fy = 1.0f - fy; // invert Y axis

        pixelPoint.x = fx * (state.renderRect.x + state.renderRect.w);
        pixelPoint.y = fy * (state.renderRect.y + state.renderRect.h);
        return true;
    } else {
        return false;
    }
}

bool RenderView::WorldToPixel(const Vec3 &worldPosition, Vec3 &pixelCoords) const {
    Vec4 normalizedDeviceCoords = WorldToNormalizedDevice(worldPosition);

    return NormalizedDeviceToPixel(normalizedDeviceCoords, pixelCoords);
}

bool RenderView::WorldToPixel(const Vec3 &worldPosition, Point &pixelPoint) const {
    Vec4 normalizedDeviceCoords = WorldToNormalizedDevice(worldPosition);

    return NormalizedDeviceToPixel(normalizedDeviceCoords, pixelPoint);
}

bool RenderView::GetClipRectFromSphere(const Sphere &sphere, Rect &clipRect) const {
    Vec3 planeNormal1, planeNormal2;
    int xmin, xmax, ymax, ymin;
    float x, y;

    float r2 = sphere.radius * sphere.radius;

    // 카메라가 sphere 안에 있을 경우
    if (state.origin.DistanceSqr(sphere.origin) < r2) {
        clipRect = state.renderRect;
        return true;
    }

    // sphere 의 중심좌표(L) 를 카메라 로컬좌표계(X, Y, Z = FORWARD, LEFT, UP) 로 변환
    Vec3 localOrigin = state.axis.TransposedMulVec(sphere.origin - state.origin);

    float x2 = localOrigin.x * localOrigin.x;
    float y2 = localOrigin.y * localOrigin.y;
    float z2 = localOrigin.z * localOrigin.z;

    // 세로 접평면 T 는 z 축에 평행하고 카메라(C) 를 지나므로,
    // T = (Nx, Ny, 0, 0) 이면
    //
    // 1) T dot L = r 
    // 2) Nx^2 + Ny^2 = 1

    // 식1 을 식2 에 대입해서 이차방정식으로 풀수 있다.
    // 접평면의 Nx 에 대한 판별식
    float d = r2 * y2 - (y2 + x2) * (r2 - x2);

    if (d <= 0.001f) {
        xmin = state.renderRect.x;
        xmax = state.renderRect.x + state.renderRect.w;
    } else {
        d = Math::Sqrt(d);

        // sphere 가 카메라 뒤에 있다면..
        if (localOrigin.x < 0) {
            d = -d;
        }

        // planeNormal1 = 왼쪽 평면
        planeNormal1.x = (sphere.radius * -localOrigin.y + d) / (y2 + x2);
        planeNormal1.y = (sphere.radius - planeNormal1.x * -localOrigin.y) / -localOrigin.x;
        planeNormal1.z = 0.0f;

        // planeNormal2 = 오른쪽 평면
        planeNormal2.x = (sphere.radius * -localOrigin.y - d) / (y2 + x2);
        planeNormal2.y = (sphere.radius - planeNormal2.x * -localOrigin.y) / -localOrigin.x;
        planeNormal2.z = 0.0f;

        float pz1 = -localOrigin.x - sphere.radius * planeNormal1.y;
        float pz2 = -localOrigin.x - sphere.radius * planeNormal2.y;

        if (pz1 >= 0 && pz2 >= 0) {
            return false;
        }

        float e = Math::Tan(DEG2RAD(state.fovX * 0.5f));

        if (pz1 < 0) {
            x = planeNormal1.y / (planeNormal1.x * e);
            Clamp(x, -1.0f, 1.0f);
            int vx = state.renderRect.x + (x + 1.0f) * state.renderRect.w * 0.5f;

            xmin = Max(state.renderRect.x, vx);
        } else {
            xmin = state.renderRect.x;
        }

        if (pz2 < 0) {
            x = planeNormal2.y / (planeNormal2.x * e);
            Clamp(x, -1.0f, 1.0f);
            int vx = state.renderRect.x + (x + 1.0f) * state.renderRect.w * 0.5f;

            xmax = Min(state.renderRect.x + state.renderRect.w, vx);
        } else {
            xmax = state.renderRect.x + state.renderRect.w;
        }

        if (xmax - xmin <= 0) {
            return false;
        }
    }	

    // 가로 접평면 T 는 y 축에 평행하고 카메라(C) 를 지나므로,
    // T = (Nx, 0, Nz, 0) 이면
    //
    // 1) T dot L = r 
    // 2) Nx^2 + Nz^2 = 1

    // 접평면의 Nz 에 대한 판별식
    d = r2 * z2 - (z2 + x2) * (r2 - x2);

    if (d <= 0.001f) {
        ymin = state.renderRect.y;
        ymax = state.renderRect.y + state.renderRect.h;
    } else {
        d = Math::Sqrt(d);

        // sphere 가 카메라 뒤에 있다면..
        if (localOrigin.x < 0) {
            d = -d;
        }

        // planeNormal1 = 위쪽 평면
        planeNormal1.z = (sphere.radius * localOrigin.z - d) / (z2 + x2);
        planeNormal1.x = (sphere.radius - planeNormal1.z * localOrigin.z) / -localOrigin.x;
        planeNormal1.y = 0.0f;

        // planeNormal2 = 아래쪽 평면
        planeNormal2.z = (sphere.radius * localOrigin.z + d) / (z2 + x2);
        planeNormal2.x = (sphere.radius - planeNormal2.z * localOrigin.z) / -localOrigin.x;
        planeNormal2.y = 0.0f;

        float pz1 = -localOrigin.x - sphere.radius * planeNormal1.x;
        float pz2 = -localOrigin.x - sphere.radius * planeNormal2.x;

        if (pz1 >= 0 && pz2 >= 0) {
            return false;
        }

        float e = Math::Tan(DEG2RAD(state.fovY * 0.5f));

        if (pz1 < 0) {
            y = planeNormal1.x / (planeNormal1.z * e);
            Clamp(y, -1.0f, 1.0f);
            int vy = state.renderRect.y + (1.0f - y) * state.renderRect.h * 0.5f;

            ymin = Max(state.renderRect.y, vy);
        } else {
            ymin = state.renderRect.y;
        }

        if (pz2 < 0) {
            y = planeNormal2.x / (planeNormal2.z * e);
            Clamp(y, -1.0f, 1.0f);
            int vy = state.renderRect.y + (1.0f - y) * state.renderRect.h * 0.5f;

            ymax = Min(state.renderRect.y + state.renderRect.h, vy);
        } else {
            ymax = state.renderRect.y + state.renderRect.h;
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

bool RenderView::GetClipRectFromAABB(const AABB &aabb, Rect &clipRect) const {
    return GetClipRectFromOBB(OBB(aabb, Vec3::origin, Mat3::identity), clipRect);
}

bool RenderView::GetClipRectFromOBB(const OBB &obb, Rect &clipRect) const {
    AABB bounds;

    if (state.orthogonal) {
        if (!box.ProjectionBounds(obb, bounds)) {
            return false;
        }
    } else {
        if (!frustum.ProjectionBounds(obb, bounds)) {
            return false;
        }
    }

    if (bounds[1][1] - bounds[0][1] <= 0) {
        return false;
    }

    if (bounds[1][2] - bounds[0][2] <= 0) {
        return false;
    }

    clipRect.x = state.renderRect.x + (-bounds[1][1] + 1) * state.renderRect.w * 0.5f;
    clipRect.y = state.renderRect.y + (-bounds[1][2] + 1) * state.renderRect.h * 0.5f;
    clipRect.w = state.renderRect.w * 0.5f * (bounds[1][1] - bounds[0][1]);
    clipRect.h = state.renderRect.h * 0.5f * (bounds[1][2] - bounds[0][2]);

    return true;
}

bool RenderView::GetClipRectFromFrustum(const Frustum &frustum, Rect &clipRect) const {
    AABB bounds;

    if (state.orthogonal) {
        if (!this->box.ProjectionBounds(frustum, bounds)) {
            return false;
        }
    } else {
        // CHECK: is valid AABB ?
        if (!this->frustum.ProjectionBounds(frustum, bounds)) {
            return false;
        }
    }

    if (bounds[1][1] - bounds[0][1] <= 0) {
        return false;
    }

    if (bounds[1][2] - bounds[0][2] <= 0) {
        return false;
    }

    clipRect.x = state.renderRect.x + (-bounds[1][1] + 1) * state.renderRect.w * 0.5f;
    clipRect.y = state.renderRect.y + (-bounds[1][2] + 1) * state.renderRect.h * 0.5f;
    clipRect.w = state.renderRect.w * 0.5f * (bounds[1][1] - bounds[0][1]);
    clipRect.h = state.renderRect.h * 0.5f * (bounds[1][2] - bounds[0][2]);

    return true;
}

bool RenderView::GetDepthBoundsFromPoints(int numPoints, const Vec3 *points, const Mat4 &mvp, float *depthMin, float *depthMax) const {
    float localMin = Math::Infinity;
    float localMax = -Math::Infinity;

    for (int i = 0; i < numPoints; i++) {
        const Vec3 &v = points[i];

        float cz = v[0] * mvp[2][0] + v[1] * mvp[2][1] + v[2] * mvp[2][2] + mvp[2][3];
        float cw = v[0] * mvp[3][0] + v[1] * mvp[3][1] + v[2] * mvp[3][2] + mvp[3][3];

        if (cw > Math::FloatSmallestNonDenormal) {
            cz = cz / cw;
        } else {
            cz = -1.0f;
        }

        localMin = Min(localMin, cz);
        localMax = Max(localMax, cz);
    }

    if (localMin <= localMax && (localMin >= -1.0 || localMax <= 1.0)) {
        *depthMin = localMin * 0.5f + 0.5f;
        *depthMax = localMax * 0.5f + 0.5f;
        return true;
    }

    return false;
}

bool RenderView::GetDepthBoundsFromSphere(const Sphere &sphere, const Mat4 &mvp, float *depthMin, float *depthMax) const {
    Vec3 points[2];
    points[0] = sphere.Origin() + state.axis[0] * sphere.Radius();
    points[1] = sphere.Origin() - state.axis[0] * sphere.Radius();
    return GetDepthBoundsFromPoints(2, points, mvp, depthMin, depthMax);
}

bool RenderView::GetDepthBoundsFromAABB(const AABB &aabb, const Mat4 &mvp, float *depthMin, float *depthMax) const {
    Vec3 points[8];
    aabb.ToPoints(points);
    return GetDepthBoundsFromPoints(8, points, mvp, depthMin, depthMax);
}

bool RenderView::GetDepthBoundsFromOBB(const OBB &obb, const Mat4 &mvp, float *depthMin, float *depthMax) const {
    Vec3 points[8];
    obb.ToPoints(points);
    return GetDepthBoundsFromPoints(8, points, mvp, depthMin, depthMax);
}

bool RenderView::GetDepthBoundsFromFrustum(const Frustum &frustum, const Mat4 &mvp, float *depthMin, float *depthMax) const {
    Vec3 points[8];
    frustum.ToPoints(points);
    return GetDepthBoundsFromPoints(8, points, mvp, depthMin, depthMax);
}

bool RenderView::GetDepthBoundsFromLight(const RenderLight *light, const Mat4 &viewProjMatrix, float *depthMin, float *depthMax) const {
    if (light->state.type == RenderLight::DirectionalLight) {
        if (!GetDepthBoundsFromOBB(light->worldOBB, viewProjMatrix, depthMin, depthMax)) {
            return false;
        }
    } else if (light->state.type == RenderLight::PointLight) {
        if (light->IsRadiusUniform()) {
            if (!GetDepthBoundsFromSphere(Sphere(light->GetOrigin(), light->GetRadius()[0]), viewProjMatrix, depthMin, depthMax)) {
                return false;
            }
        } else {
            if (!GetDepthBoundsFromOBB(light->worldOBB, viewProjMatrix, depthMin, depthMax)) {
                return false;
            }
        }
    } else if (light->state.type == RenderLight::SpotLight) {
        if (!GetDepthBoundsFromFrustum(light->worldFrustum, viewProjMatrix, depthMin, depthMax)) {
            return false;
        }
    } else {
        assert(0);
    }

    return true;
}

void RenderView::ComputeFov(float fromFovX, float fromAspectRatio, float toAspectRatio, float *toFovX, float *toFovY) {
    float tanFovX = Math::Tan(DEG2RAD(fromFovX * 0.5f));
    float tanFovY = tanFovX / fromAspectRatio;

    *toFovX = RAD2DEG(Math::ATan(tanFovY * toAspectRatio) * 2.0f);
    *toFovY = RAD2DEG(Math::ATan(tanFovY) * 2.0f);
}

const Ray RenderView::RayFromScreenND(const RenderView::State &renderView, float ndx, float ndy) {
    Ray ray;

    if (renderView.orthogonal) {
        ray.origin = renderView.origin + renderView.axis[0] * renderView.zNear; // zNear can be negative number in orthogonal view
        ray.origin -= renderView.axis[1] * ndx * renderView.sizeX;
        ray.origin -= renderView.axis[2] * ndy * renderView.sizeY;

        ray.direction = renderView.axis[0];
        ray.direction.Normalize();
    } else {
        float half_w = Math::Tan(DEG2RAD(renderView.fovX * 0.5f)) * renderView.zNear;
        float half_h = Math::Tan(DEG2RAD(renderView.fovY * 0.5f)) * renderView.zNear;

        ray.direction = renderView.axis[0] * renderView.zNear;
        ray.direction -= renderView.axis[1] * ndx * half_w;
        ray.direction -= renderView.axis[2] * ndy * half_h;
        
        ray.origin = renderView.origin + ray.direction;

        ray.direction.Normalize();
    }

    return ray;
}

BE_NAMESPACE_END
