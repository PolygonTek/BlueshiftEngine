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

#pragma once

BE_NAMESPACE_BEGIN

class Barycentric {
public:
    /// Compute 2D barycentric coordinates from the point based on 2 simplex 1D points.
    static Vec2 LineSegment1D(float p0, float p1, float p);

    /// Compute 2D barycentric coordinates from the point based on 2 simplex 2D points.
    static Vec2 LineSegment2D(const Vec2 &p0, const Vec2 &p1, const Vec2 &p);

    /// Compute 3D barycentric coordinates from the point based on 3 simplex 2D points.
    static Vec3 Triangle2D(const Vec2 &p0, const Vec2 &p1, const Vec2 &p2, const Vec2 &p);

    /// Compute 3D barycentric coordinates from the point based on 3 simplex 3D points.
    static Vec3 Triangle3D(const Vec3 &p0, const Vec3 &p1, const Vec3 &p2, const Vec3 &p);

    /// Compute 4D barycentric coordinates from the point based on 4 simplex 3D points.
    static Vec4 Tetrahedron3D(const Vec3 &p0, const Vec3 &p1, const Vec3 &p2, const Vec3 &p3, const Vec3 &p);
};

BE_NAMESPACE_END
