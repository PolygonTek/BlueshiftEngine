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

// environment cubemap face 에 대응하는 엔진 좌표계 축을 만든다.
void EnvCubeMapFaceToEngineAxis(RHI::CubemapFace cubemapFace, BE1::Mat3 &axis);

// 삼각형의 position/UV 로 tangent, bitangent, handedness 를 계산한다.
void TangentsFromTriangle(const BE1::Vec3 &p0, const BE1::Vec3 &p1, const BE1::Vec3 &p2, const BE1::Vec2 &uv0, const BE1::Vec2 &uv1, const BE1::Vec2 &uv2, BE1::Vec3 &tangent, BE1::Vec3 &bitangent, float *handedness);

// sphere 를 lats * (longs + 1) * 2 개의 triangle strip 정점 배열로 생성한다.
void GenerateSphereTriangleStripVerts(const BE1::Vec3 &sphereCenter, float sphereRadius, int lats, int longs, BE1::Vec3 *verts);
