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

#include "Math/Matrix4.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/AABB.h"
#include "Math/OBB.h"
#include "Math/Sphere.h"
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

// Receiver sphere 에 맞춘 단일 directional shadow 투영 계산.
// Caster bounds 가 비어 있으면 receiver 만으로 깊이를 잡고, 주어지면 XY 를 유지하며 깊이를 확장한다.
bool ComputeDirectionalShadowProjection(const BE1::Mat3 &lightAxis, const BE1::Sphere &receiverSphere, const BE1::AABB &casterBounds, uint32_t resolution, BE1::OBB &shadowWorldBox, BE1::Mat4 &shadowViewProjMatrix);

// Shadow box 를 광원 쪽으로 무한히 연장한 영역에 caster AABB 가 닿지 않으면 true.
// Near 경계는 검사하지 않으며, shadow far 경계 뒤에 완전히 놓인 caster 만 제외한다.
bool CullDirectionalShadowCaster(const BE1::AABB &casterBounds, const BE1::OBB &shadowBox);

// 2D 영역을 정규화된 크롭 행렬로 만든다.
void Set2DCropMatrix(float xmin, float xmax, float ymin, float ymax, BE1::Mat4 &cropMatrix);

// EngineAxis cubemap face 를 렌더링할 view axis 를 만든다.
void CubeMapFaceToViewAxis(RHI::CubemapFace cubemapFace, BE1::Mat3 &axis);

// 삼각형의 position/UV 로 tangent, bitangent, handedness 를 계산한다.
void TangentsFromTriangle(const BE1::Vec3 &p0, const BE1::Vec3 &p1, const BE1::Vec3 &p2, const BE1::Vec2 &uv0, const BE1::Vec2 &uv1, const BE1::Vec2 &uv2, BE1::Vec3 &tangent, BE1::Vec3 &bitangent, float *handedness);

// sphere 를 lats * (longs + 1) * 2 개의 triangle strip 정점 배열로 생성한다.
void GenerateSphereTriangleStripVerts(const BE1::Vec3 &sphereCenter, float sphereRadius, int lats, int longs, BE1::Vec3 *verts);
