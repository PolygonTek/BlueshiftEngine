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
