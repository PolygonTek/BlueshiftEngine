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

void SceneView::Update(const Parms *viewParms) {
    parms = *viewParms;

    // near/far
    zNear = parms.zNear;
    zFar = parms.zFar;

    // calculate view matrix
    R_SetViewMatrix(parms.axis, parms.origin, viewMatrix);

    if (parms.orthogonal) {
        // view bounding OBB
        Vec3 extents((zFar - zNear) * 0.5f, parms.sizeX, parms.sizeY);
        box.SetCenter(parms.origin + parms.axis[0] * (zNear + extents[0]));
        box.SetExtents(extents);
        box.SetAxis(parms.axis);

        // calculate orthogonal projection matrix
        R_SetOrthogonalProjectionMatrix(parms.sizeX, parms.sizeY, zNear, zFar, projMatrix);
    } else {
        // view bounding frustum
        frustum.SetOrigin(parms.origin);
        frustum.SetAxis(parms.axis);

        frustum.SetSize(zNear, zFar, zFar * Math::Tan(DEG2RAD(parms.fovX) * 0.5f), zFar * Math::Tan(DEG2RAD(parms.fovY) * 0.5f));

        // calculate view frustum planes
        frustum.ToPlanes(frustumPlanes);

        // calculate view frustum points
        frustum.ToPoints(frustumPoints);

        // calculate perspective projection matrix
        R_SetPerspectiveProjectionMatrix(parms.fovX, parms.fovY, zNear, zFar, false, projMatrix);
    }

    viewProjMatrix = projMatrix * viewMatrix;
}

void SceneView::RecalcZFar(float zFar) {
    this->zFar = zFar;

    if (parms.orthogonal) {
        // view bounding OBB
        Vec3 extents((zFar - zNear) * 0.5f, parms.sizeX, parms.sizeY);
        box.SetCenter(parms.origin + parms.axis[0] * (zNear + extents[0]));
        box.SetExtents(extents);

        // calculate orthogonal projection matrix
        R_SetOrthogonalProjectionMatrix(parms.sizeX, parms.sizeY, zNear, zFar, projMatrix);
    } else {
        // view frustum
        frustum.SetSize(zNear, zFar, zFar * Math::Tan(DEG2RAD(parms.fovX) * 0.5f), zFar * Math::Tan(DEG2RAD(parms.fovY) * 0.5f));

        // calculate view frustum planes
        frustum.ToPlanes(frustumPlanes);

        // calculate view frustum points
        frustum.ToPoints(frustumPoints);

        // calculate perspective projection matrix
        R_SetPerspectiveProjectionMatrix(parms.fovX, parms.fovY, zNear, zFar, false, projMatrix);
    }

    viewProjMatrix = projMatrix * viewMatrix;
}

Vec4 SceneView::WorldToNormalizedDevice(const Vec3 &worldCoords) const {
    return viewProjMatrix * Vec4(worldCoords, 1.0f);
}

bool SceneView::NormalizedDeviceToPixel(const Vec4 &normalizedDeviceCoords, Vec3 &pixelCoords) const {
    if (normalizedDeviceCoords.w > 0) {
        float invW = 1.0f / normalizedDeviceCoords.w;

        float y = 1.0f - normalizedDeviceCoords.y;

        pixelCoords.x = (normalizedDeviceCoords.x * invW + 1.0f) * 0.5f * (parms.renderRect.x + parms.renderRect.w);
        pixelCoords.y = (y * invW + 1.0f) * 0.5f * (parms.renderRect.y + parms.renderRect.h);
        pixelCoords.z = normalizedDeviceCoords.z * invW; // depth value
        return true;
    } else {
        return false;
    }
}

bool SceneView::NormalizedDeviceToPixel(const Vec4 &normalizedDeviceCoords, Point &pixelPoint) const {
    if (normalizedDeviceCoords.w > 0) {
        float invW = 1.0f / normalizedDeviceCoords.w;

        float y = 1.0f - normalizedDeviceCoords.y;

        pixelPoint.x = (normalizedDeviceCoords.x * invW + 1.0f) * 0.5f * (parms.renderRect.x + parms.renderRect.w);
        pixelPoint.y = (y * invW + 1.0f) * 0.5f * (parms.renderRect.y + parms.renderRect.h);
        return true;
    } else {
        return false;
    }
}

bool SceneView::WorldToPixel(const Vec3 &worldCoords, Vec3 &pixelCoords) const {
    Vec4 normalizedDeviceCoords = WorldToNormalizedDevice(worldCoords);

    return NormalizedDeviceToPixel(normalizedDeviceCoords, pixelCoords);
}

bool SceneView::WorldToPixel(const Vec3 &worldCoords, Point &pixelPoint) const {
    Vec4 normalizedDeviceCoords = WorldToNormalizedDevice(worldCoords);

    return NormalizedDeviceToPixel(normalizedDeviceCoords, pixelPoint);
}

bool SceneView::GetClipRectFromSphere(const Sphere &sphere, Rect &clipRect) const {
    Vec3 planeNormal1, planeNormal2;
    int xmin, xmax, ymax, ymin;
    float x, y;

    float r2 = sphere.radius * sphere.radius;

    // 카메라가 sphere 안에 있을 경우
    if (parms.origin.DistanceSqr(sphere.origin) < r2) {
        clipRect = parms.renderRect;
        return true;
    }

    // sphere 의 중심좌표(L) 를 카메라 로컬좌표계(X, Y, Z = FORWARD, LEFT, UP) 로 변환
    Vec3 localOrigin = parms.axis.TransposedMulVec(sphere.origin - parms.origin);

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
        xmin = parms.renderRect.x;
        xmax = parms.renderRect.x + parms.renderRect.w;
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

        float e = Math::Tan(DEG2RAD(parms.fovX * 0.5f));

        if (pz1 < 0) {
            x = planeNormal1.y / (planeNormal1.x * e);
            Clamp(x, -1.0f, 1.0f);
            int vx = parms.renderRect.x + (x + 1.0f) * parms.renderRect.w * 0.5f;

            xmin = Max(parms.renderRect.x, vx);
        } else {
            xmin = parms.renderRect.x;
        }

        if (pz2 < 0) {
            x = planeNormal2.y / (planeNormal2.x * e);
            Clamp(x, -1.0f, 1.0f);
            int vx = parms.renderRect.x + (x + 1.0f) * parms.renderRect.w * 0.5f;

            xmax = Min(parms.renderRect.x + parms.renderRect.w, vx);
        } else {
            xmax = parms.renderRect.x + parms.renderRect.w;
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
        ymin = parms.renderRect.y;
        ymax = parms.renderRect.y + parms.renderRect.h;
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

        float e = Math::Tan(DEG2RAD(parms.fovY * 0.5f));

        if (pz1 < 0) {
            y = planeNormal1.x / (planeNormal1.z * e);
            Clamp(y, -1.0f, 1.0f);
            int vy = parms.renderRect.y + (1.0f - y) * parms.renderRect.h * 0.5f;

            ymin = Max(parms.renderRect.y, vy);
        } else {
            ymin = parms.renderRect.y;
        }

        if (pz2 < 0) {
            y = planeNormal2.x / (planeNormal2.z * e);
            Clamp(y, -1.0f, 1.0f);
            int vy = parms.renderRect.y + (1.0f - y) * parms.renderRect.h * 0.5f;

            ymax = Min(parms.renderRect.y + parms.renderRect.h, vy);
        } else {
            ymax = parms.renderRect.y + parms.renderRect.h;
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

bool SceneView::GetClipRectFromAABB(const AABB &aabb, Rect &clipRect) const {
    return GetClipRectFromOBB(OBB(aabb, Vec3::origin, Mat3::identity), clipRect);
}

bool SceneView::GetClipRectFromOBB(const OBB &obb, Rect &clipRect) const {
    AABB bounds;

    if (parms.orthogonal) {
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

    clipRect.x = parms.renderRect.x + (-bounds[1][1] + 1) * parms.renderRect.w * 0.5f;
    clipRect.y = parms.renderRect.y + (-bounds[1][2] + 1) * parms.renderRect.h * 0.5f;
    clipRect.w = parms.renderRect.w * 0.5f * (bounds[1][1] - bounds[0][1]);
    clipRect.h = parms.renderRect.h * 0.5f * (bounds[1][2] - bounds[0][2]);

    return true;
}

bool SceneView::GetClipRectFromFrustum(const Frustum &frustum, Rect &clipRect) const {
    AABB bounds;

    if (parms.orthogonal) {
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

    clipRect.x = parms.renderRect.x + (-bounds[1][1] + 1) * parms.renderRect.w * 0.5f;
    clipRect.y = parms.renderRect.y + (-bounds[1][2] + 1) * parms.renderRect.h * 0.5f;
    clipRect.w = parms.renderRect.w * 0.5f * (bounds[1][1] - bounds[0][1]);
    clipRect.h = parms.renderRect.h * 0.5f * (bounds[1][2] - bounds[0][2]);

    return true;
}

bool SceneView::GetDepthBoundsFromPoints(int numPoints, const Vec3 *points, const Mat4 &mvp, float *depthMin, float *depthMax) const {
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

bool SceneView::GetDepthBoundsFromSphere(const Sphere &sphere, const Mat4 &mvp, float *depthMin, float *depthMax) const {
    Vec3 points[2];
    points[0] = sphere.Origin() + parms.axis[0] * sphere.Radius();
    points[1] = sphere.Origin() - parms.axis[0] * sphere.Radius();
    return GetDepthBoundsFromPoints(2, points, mvp, depthMin, depthMax);
}

bool SceneView::GetDepthBoundsFromAABB(const AABB &aabb, const Mat4 &mvp, float *depthMin, float *depthMax) const {
    Vec3 points[8];
    aabb.ToPoints(points);
    return GetDepthBoundsFromPoints(8, points, mvp, depthMin, depthMax);
}

bool SceneView::GetDepthBoundsFromOBB(const OBB &obb, const Mat4 &mvp, float *depthMin, float *depthMax) const {
    Vec3 points[8];
    obb.ToPoints(points);
    return GetDepthBoundsFromPoints(8, points, mvp, depthMin, depthMax);
}

bool SceneView::GetDepthBoundsFromFrustum(const Frustum &frustum, const Mat4 &mvp, float *depthMin, float *depthMax) const {
    Vec3 points[8];
    frustum.ToPoints(points);
    return GetDepthBoundsFromPoints(8, points, mvp, depthMin, depthMax);
}

bool SceneView::GetDepthBoundsFromLight(const SceneLight *light, const Mat4 &viewProjMatrix, float *depthMin, float *depthMax) const {
    if (light->parms.type == SceneLight::DirectionalLight) {
        if (!GetDepthBoundsFromOBB(light->obb, viewProjMatrix, depthMin, depthMax)) {
            return false;
        }
    } else if (light->parms.type == SceneLight::PointLight) {
        if (light->IsRadiusUniform()) {
            if (!GetDepthBoundsFromSphere(Sphere(light->GetOrigin(), light->GetRadius()[0]), viewProjMatrix, depthMin, depthMax)) {
                return false;
            }
        } else {
            if (!GetDepthBoundsFromOBB(light->obb, viewProjMatrix, depthMin, depthMax)) {
                return false;
            }
        }
    } else if (light->parms.type == SceneLight::SpotLight) {
        if (!GetDepthBoundsFromFrustum(light->frustum, viewProjMatrix, depthMin, depthMax)) {
            return false;
        }
    } else {
        assert(0);
    }	

    return true;
}

void SceneView::ComputeFov(float fromFovX, float fromAspectRatio, float toAspectRatio, float *toFovX, float *toFovY) {
    float tanFovX = Math::Tan(DEG2RAD(fromFovX * 0.5f));
    float tanFovY = tanFovX / fromAspectRatio;

    *toFovX = RAD2DEG(Math::ATan(tanFovY * toAspectRatio) * 2.0f);
    *toFovY = RAD2DEG(Math::ATan(tanFovY) * 2.0f);
}

const Ray SceneView::RayFromScreenND(const SceneView::Parms &sceneView, float ndx, float ndy) {
    Ray ray;

    if (sceneView.orthogonal) {
        ray.origin = sceneView.origin + sceneView.axis[0] * sceneView.zNear; // zNear can be negative number in orthogonal view
        ray.origin -= sceneView.axis[1] * ndx * sceneView.sizeX;
        ray.origin -= sceneView.axis[2] * ndy * sceneView.sizeY;

        ray.direction = sceneView.axis[0];
        ray.direction.Normalize();
    } else {
        float half_w = Math::Tan(DEG2RAD(sceneView.fovX * 0.5f)) * sceneView.zNear;
        float half_h = Math::Tan(DEG2RAD(sceneView.fovY * 0.5f)) * sceneView.zNear;

        ray.direction = sceneView.axis[0] * sceneView.zNear;
        ray.direction -= sceneView.axis[1] * ndx * half_w;
        ray.direction -= sceneView.axis[2] * ndy * half_h;
        
        ray.origin = sceneView.origin + ray.direction;

        ray.direction.Normalize();
    }

    return ray;
}

BE_NAMESPACE_END
