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
#include "RenderUtils.h"

/*
----------------------------------------------------------------------------------------------------

    SetViewMatrixFromEngineAxis

    Build view matrix from engine view axis

    R (Rx, Ry, Rz) : right direction vector
    U (Ux, Uy, Uz) : up direction vector
    B (Bx, By, Bz) : back direction vector

    camera position : P (Px, Py, Pz)

        | Rx,  Ry,  Rz,  -dot(R, P) |
    M = | Ux,  Uy,  Uz,  -dot(U, P) |
        | Bx,  By,  Bz,  -dot(B, P) |
        | 0,   0,   0,    1         |

----------------------------------------------------------------------------------------------------
*/
void SetViewMatrixFromEngineAxis(const BE1::Mat3 &viewAxis, const BE1::Vec3 &viewOrigin, BE1::Mat4 &viewMatrix) {
    // right axis = -engine left axis
    viewMatrix[0][0] = -viewAxis[EngineAxis::Left].x;
    viewMatrix[0][1] = -viewAxis[EngineAxis::Left].y;
    viewMatrix[0][2] = -viewAxis[EngineAxis::Left].z;
    viewMatrix[0][3] = -viewAxis[EngineAxis::Left].Dot(-viewOrigin);

    // up axis = engine up axis
    viewMatrix[1][0] = viewAxis[EngineAxis::Up].x;
    viewMatrix[1][1] = viewAxis[EngineAxis::Up].y;
    viewMatrix[1][2] = viewAxis[EngineAxis::Up].z;
    viewMatrix[1][3] = viewAxis[EngineAxis::Up].Dot(-viewOrigin);

    // back axis = -engine forward axis
    viewMatrix[2][0] = -viewAxis[EngineAxis::Forward].x;
    viewMatrix[2][1] = -viewAxis[EngineAxis::Forward].y;
    viewMatrix[2][2] = -viewAxis[EngineAxis::Forward].z;
    viewMatrix[2][3] = -viewAxis[EngineAxis::Forward].Dot(-viewOrigin);

    viewMatrix[3][0] = 0.0f;
    viewMatrix[3][1] = 0.0f;
    viewMatrix[3][2] = 0.0f;
    viewMatrix[3][3] = 1.0f;
}

/*
----------------------------------------------------------------------------------------------------

    Set2DCropMatrix

    Build orthogonal 2d crop matrix

        | 2 / (R-L),  0,          0,          -(R+L) / (R-L) |
    M = | 0,          2 / (T-B),  0,          -(T+B) / (T-B) |
        | 0,          0,          1,          0,             |
        | 0,          0,          0,          1              |

----------------------------------------------------------------------------------------------------
*/
void Set2DCropMatrix(float xmin, float xmax, float ymin, float ymax, BE1::Mat4 &cropMatrix) {
    assert(xmax != xmin);
    assert(ymax != ymin);

    const double oneDeltaX = 1.0 / (xmax - xmin);
    const double oneDeltaY = 1.0 / (ymax - ymin);

    cropMatrix[0][0] = static_cast<float>(2.0 * oneDeltaX);
    cropMatrix[0][1] = 0.0f;
    cropMatrix[0][2] = 0.0f;
    cropMatrix[0][3] = static_cast<float>(-(xmax + xmin) * oneDeltaX);

    cropMatrix[1][0] = 0.0f;
    cropMatrix[1][1] = static_cast < float>(2.0 * oneDeltaY);
    cropMatrix[1][2] = 0.0f;
    cropMatrix[1][3] = static_cast<float>(-(ymax + ymin) * oneDeltaY);

    cropMatrix[2][0] = 0.0f;
    cropMatrix[2][1] = 0.0f;
    cropMatrix[2][2] = 1.0f;
    cropMatrix[2][3] = 0.0f;

    cropMatrix[3][0] = 0.0f;
    cropMatrix[3][1] = 0.0f;
    cropMatrix[3][2] = 0.0f;
    cropMatrix[3][3] = 1.0f;
}

bool ComputeDirectionalShadowProjection(const BE1::Mat3 &lightAxis, const BE1::Sphere &receiverSphere, const BE1::AABB &casterBounds, uint32_t resolution, BE1::OBB &dirShadowBox, BE1::Mat4 &shadowViewProjMatrix) {
    constexpr uint32_t borderTexels = 2;
    const float radius = receiverSphere.Radius();
    if (radius <= 0.0f || resolution <= 2 * borderTexels) {
        return false;
    }

    // receiver sphere radius 로 shadow view XY 크기를 계산한다.
    const float shadowViewExtent = radius * (static_cast<float>(resolution) / (resolution - 2 * borderTexels));
    const float worldUnitsPerTexel = 2.0f * shadowViewExtent / resolution;
    if (worldUnitsPerTexel <= 0.0f) {
        return false;
    }

    // Directional light 의 위치와 무관한 world-space 원점 기준 격자에 정렬한다.
    const BE1::Vec3 &center = receiverSphere.Center();

    // recever sphere center 의 light space 좌표를 구한다 (XY 는 snap)
    const float distLeft = BE1::Math::Snap(lightAxis[EngineAxis::Left].Dot(center), worldUnitsPerTexel);
    const float distUp = BE1::Math::Snap(lightAxis[EngineAxis::Up].Dot(center), worldUnitsPerTexel);
    const float distForward = lightAxis[EngineAxis::Forward].Dot(center);

    float depthMin = distForward - radius;
    float depthMax = distForward + radius;

    // caster bounds 가 존재하면 depthMin 을 caster bounds 의 최소값으로 확장한다.
    if (!casterBounds.IsCleared()) {
        float casterMin, casterMax;
        casterBounds.ProjectOnAxis(lightAxis[EngineAxis::Forward], casterMin, casterMax);
        depthMin = BE1::Min(depthMin, casterMin);
    }

    // 경계면에서의 부동소수점 오차로 receiver/caster 가 잘리지 않도록 깊이 여유를 둔다.
    const float depthPadding = BE1::Max(0.001f, (depthMax - depthMin) * 0.0001f);
    depthMin -= depthPadding;
    depthMax += depthPadding;
    const float depthRange = depthMax - depthMin;
    if (depthRange <= 0.0f) {
        return false;
    }

    // shadow view matrix 를 계산한다.
    BE1::Mat4 shadowViewMatrix;
    SetViewMatrixFromEngineAxis(lightAxis, BE1::Vec3::origin, shadowViewMatrix);
    shadowViewMatrix[0][3] = distLeft;
    shadowViewMatrix[1][3] = -distUp;
    shadowViewMatrix[2][3] = depthMin;

    // shadow projection matrix 를 계산한다.
    BE1::Mat4 shadowProjMatrix;
    shadowProjMatrix.SetOrthoRH(-shadowViewExtent, shadowViewExtent, -shadowViewExtent, shadowViewExtent, 0.0f, depthRange, false, false);
    shadowViewProjMatrix = shadowProjMatrix * shadowViewMatrix;

    // shadow world box 를 계산한다.
    const BE1::Vec3 shadowBoxCenter = lightAxis[EngineAxis::Forward] * (depthMin + depthRange * 0.5f) + lightAxis[EngineAxis::Left] * distLeft + lightAxis[EngineAxis::Up] * distUp;
    dirShadowBox = BE1::OBB(shadowBoxCenter, BE1::Vec3(depthRange * 0.5f, shadowViewExtent, shadowViewExtent), lightAxis);
    return true;
}

bool CullDirectionalShadowCaster(const BE1::AABB &casterBounds, const BE1::OBB &dirShadowBox) {
    if (casterBounds.IsCleared()) {
        return true;
    }

    const BE1::Vec3 casterCenter = casterBounds.Center();
    const BE1::Vec3 casterExtents = casterBounds[1] - casterCenter;
    const BE1::Vec3 relativeCenter = casterCenter - dirShadowBox.Center();
    const BE1::Vec3 &shadowExtents = dirShadowBox.Extents();
    const BE1::Mat3 &shadowAxis = dirShadowBox.Axis();

    // shadow box X 축 컬링
    const BE1::Vec3 &left = shadowAxis[EngineAxis::Left];
    if (BE1::Math::Abs(left.Dot(relativeCenter)) > shadowExtents[EngineAxis::Left] + casterExtents.AbsDot(left)) {
        return true;
    }

    // shadow box Y 축 컬링
    const BE1::Vec3 &up = shadowAxis[EngineAxis::Up];
    if (BE1::Math::Abs(up.Dot(relativeCenter)) > shadowExtents[EngineAxis::Up] + casterExtents.AbsDot(up)) {
        return true;
    }

    // shadow box far 컬링
    const BE1::Vec3 &forward = shadowAxis[EngineAxis::Forward];
    const float depthMin = forward.Dot(relativeCenter) - casterExtents.AbsDot(forward);
    return depthMin > shadowExtents[EngineAxis::Forward];
}

void CubeMapFaceToViewAxis(RHI::CubemapFace cubemapFace, BE1::Mat3 &axis) {
    switch (cubemapFace) {
    case RHI::CubemapFace::PositiveX:
        axis[0] = BE1::Vec3(1.0f, 0.0f, 0.0f);
        axis[1] = BE1::Vec3(0.0f, 0.0f, 1.0f);
        axis[2] = BE1::Vec3(0.0f, -1.0f, 0.0f);
        break;
    case RHI::CubemapFace::NegativeX:
        axis[0] = BE1::Vec3(-1.0f, 0.0f, 0.0f);
        axis[1] = BE1::Vec3(0.0f, 0.0f, -1.0f);
        axis[2] = BE1::Vec3(0.0f, -1.0f, 0.0f);
        break;
    case RHI::CubemapFace::PositiveY:
        axis[0] = BE1::Vec3(0.0f, 1.0f, 0.0f);
        axis[1] = BE1::Vec3(-1.0f, 0.0f, 0.0f);
        axis[2] = BE1::Vec3(0.0f, 0.0f, 1.0f);
        break;
    case RHI::CubemapFace::NegativeY:
        axis[0] = BE1::Vec3(0.0f, -1.0f, 0.0f);
        axis[1] = BE1::Vec3(-1.0f, 0.0f, 0.0f);
        axis[2] = BE1::Vec3(0.0f, 0.0f, -1.0f);
        break;
    case RHI::CubemapFace::PositiveZ:
        axis[0] = BE1::Vec3(0.0f, 0.0f, 1.0f);
        axis[1] = BE1::Vec3(-1.0f, 0.0f, 0.0f);
        axis[2] = BE1::Vec3(0.0f, -1.0f, 0.0f);
        break;
    case RHI::CubemapFace::NegativeZ:
        axis[0] = BE1::Vec3(0.0f, 0.0f, -1.0f);
        axis[1] = BE1::Vec3(1.0f, 0.0f, 0.0f);
        axis[2] = BE1::Vec3(0.0f, -1.0f, 0.0f);
        break;
    default:
        assert(0);
        break;
    }
}

static void SetFallbackTangents(const BE1::Vec3 &normal, BE1::Vec3 &tangent, BE1::Vec3 &bitangent, float *handedness) {
    normal.OrthogonalBasis(tangent, bitangent);
    if (handedness) {
        *handedness = 1.0f;
    }
}

void TangentsFromTriangle(const BE1::Vec3 &p0, const BE1::Vec3 &p1, const BE1::Vec3 &p2, const BE1::Vec2 &uv0, const BE1::Vec2 &uv1, const BE1::Vec2 &uv2, BE1::Vec3 &tangent, BE1::Vec3 &bitangent, float *handedness) {
    assert(handedness);

    constexpr float kEpsilon = 1e-20f;

    const BE1::Vec3 side0 = p1 - p0;
    const BE1::Vec3 side1 = p2 - p0;

    BE1::Vec3 normal = side0.Cross(side1);
    const float normalLengthSqr = normal.LengthSqr();
    if (normalLengthSqr <= kEpsilon) {
        SetFallbackTangents(BE1::Vec3::unitZ, tangent, bitangent, handedness);
        return;
    }
    normal *= BE1::Math::InvSqrt(normalLengthSqr);

    const float du1 = uv1[0] - uv0[0];
    const float dv1 = uv1[1] - uv0[1];

    const float du2 = uv2[0] - uv0[0];
    const float dv2 = uv2[1] - uv0[1];

    const float det = du1 * dv2 - du2 * dv1;
    if (BE1::Math::Abs(det) <= kEpsilon) {
        SetFallbackTangents(normal, tangent, bitangent, handedness);
        return;
    }

    const float invDet = 1.0f / det;

    tangent.Set(dv2 * side0.x - dv1 * side1.x, dv2 * side0.y - dv1 * side1.y, dv2 * side0.z - dv1 * side1.z);
    tangent *= invDet;
    tangent -= tangent.Dot(normal) * normal;

    float tangentLengthSqr = tangent.LengthSqr();
    if (tangentLengthSqr <= kEpsilon) {
        SetFallbackTangents(normal, tangent, bitangent, handedness);
        return;
    }
    tangent *= BE1::Math::InvSqrt(tangentLengthSqr);

    bitangent.Set(du1 * side1.x - du2 * side0.x, du1 * side1.y - du2 * side0.y, du1 * side1.z - du2 * side0.z);
    bitangent *= invDet;
    bitangent -= bitangent.Dot(normal) * normal;
    //bitangent -= bitangent.Dot(normal) * normal + bitangent.Dot(tangent) * tangent;

    float bitangentLengthSqr = bitangent.LengthSqr();
    if (bitangentLengthSqr <= kEpsilon) {
        SetFallbackTangents(normal, tangent, bitangent, handedness);
        return;
    }
    bitangent *= BE1::Math::InvSqrt(bitangentLengthSqr);

    if (handedness) {
        *handedness = tangent.Cross(bitangent).Dot(normal) < 0.0f ? -1.0f : 1.0f;
    }
}

void GenerateSphereTriangleStripVerts(const BE1::Vec3 &sphereCenter, float sphereRadius, int lats, int longs, BE1::Vec3 *verts) {
    assert(lats > 0);
    assert(longs >= 3);
    assert(verts);

    BE1::Vec3 *vptr = verts;

    for (int i = 0; i < lats; ++i) {
        float lat0 = BE1::Math::Pi * (0.5f + (float)(i) / lats);
        float z0 = BE1::Math::Sin(lat0) * sphereRadius;
        float r0 = BE1::Math::Cos(lat0) * sphereRadius;

        float lat1 = BE1::Math::Pi * (0.5f + (float)(i + 1) / lats);
        float z1 = BE1::Math::Sin(lat1) * sphereRadius;
        float r1 = BE1::Math::Cos(lat1) * sphereRadius;

        for (int j = 0; j <= longs; ++j) {
            float lng = j < longs ? BE1::Math::TwoPi * (float)j / longs : 0.0f;
            float x = BE1::Math::Cos(lng);
            float y = BE1::Math::Sin(lng);

            *vptr++ = sphereCenter + BE1::Vec3(x * r0, y * r0, z0);
            *vptr++ = sphereCenter + BE1::Vec3(x * r1, y * r1, z1);
        }
    }
}
