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

#pragma once

#include "Math/Matrix3x4.h"
#include "Math/Matrix4.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/AABB.h"
#include "Math/Frustum.h"
#include "Math/OBB.h"
#include "Math/Plane.h"
#include "RHI/RHI.h"

namespace EngineAxis {
    enum Enum : int {
        Forward,
        Left,
        Up,
        Count
    };
}

// 엔진 좌표계의 뷰 축과 원점으로 뷰 행렬을 만든다.
void SetViewMatrixFromEngineAxis(const BE1::Mat3 &viewAxis, const BE1::Vec3 &viewOrigin, BE1::Mat4 &viewMatrix);

// 2D 영역을 정규화된 크롭 행렬로 만든다.
void Set2DCropMatrix(float xmin, float xmax, float ymin, float ymax, BE1::Mat4 &cropMatrix);

// Uniform/logarithmic 혼합으로 cascadeCount + 1 개의 카메라 forward 거리 경계를 계산한다.
// farDistance 는 min(camera.zFar, shadowDistance). 직교 투영 또는 nearDistance <= 0 이면 균등 분할한다.
// splitDistances 는 cascadeCount + 1 개 이상의 원소를 저장할 수 있어야 한다.
void ComputeCSMSplitDistances(float nearDistance, float farDistance, float splitLambda, uint32_t cascadeCount, bool orthogonal, float *splitDistances);

// 월드 단위당 픽셀 수와 world transform 을 반영하여 로컬 공간의 LOD 허용 오차 제곱을 계산한다.
float CalcLocalLODErrorLimitSqr(const BE1::Mat3x4 &worldMatrix, float pixelsPerWorldUnit, float pixelErrorLimit);

// view volume 과 광원 방향 shadow cull bounds 에 의한 caster bounds 의 컬링 검사.
// Shadow bounds 의 XY/far 만 검사하며, near 밖 caster 는 depth clamp 를 위해 유지한다.
bool CullShadowCaster(const BE1::Plane viewVolumePlanes[6], const BE1::OBB &shadowMapBounds, const BE1::AABB &casterBounds);

// view volume 과 광원 방향 shadow frustum 에 의한 caster bounds 의 컬링 검사.
bool CullShadowCaster(const BE1::Plane viewVolumePlanes[6], const BE1::Frustum &shadowFrustum, const BE1::AABB &casterBounds);

// EngineAxis cubemap face 를 렌더링할 view axis 를 만든다.
void CubeMapFaceToViewAxis(RHI::CubemapFace cubemapFace, BE1::Mat3 &axis);

// 삼각형의 position/UV 로 tangent, bitangent, handedness 를 계산한다.
void TangentsFromTriangle(const BE1::Vec3 &p0, const BE1::Vec3 &p1, const BE1::Vec3 &p2, const BE1::Vec2 &uv0, const BE1::Vec2 &uv1, const BE1::Vec2 &uv2, BE1::Vec3 &tangent, BE1::Vec3 &bitangent, float *handedness);

// sphere 를 lats * (longs + 1) * 2 개의 triangle strip 정점 배열로 생성한다.
void GenerateSphereTriangleStripVerts(const BE1::Vec3 &sphereCenter, float sphereRadius, int lats, int longs, BE1::Vec3 *verts);
