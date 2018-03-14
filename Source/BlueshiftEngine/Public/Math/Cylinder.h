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

/*
-------------------------------------------------------------------------------

    Cylinder

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class OBB;

/// A cylinder
class BE_API Cylinder {
public:
    /// The default constructor does not initialize any members of this class.
    Cylinder() = default;
    Cylinder(const Vec3 &p1, const Vec3 &p2, float radius);

    void                SetP1(const Vec3 &p1) { p[0] = p1; }
    void                SetP2(const Vec3 &p2) { p[1] = p2; }
    void                SetRadius(float radius) { this->radius = radius; }

    const Vec3 &        P1() const { return p[0]; }
    const Vec3 &        P2() const { return p[1]; }
    float               Radius() const { return radius; }

                        /// Exact compare, no epsilon
    bool                Equals(const Cylinder &other) const;
                        /// Compare with epsilon
    bool                Equals(const Cylinder &other, const float epsilon) const;
                        /// Exact compare, no epsilon
    bool                operator==(const Cylinder &rhs) const { return Equals(rhs); }
                        /// Exact compare, no epsilon
    bool                operator!=(const Cylinder &rhs) const { return !Equals(rhs); }

    bool                LineIntersection(const Vec3 &start, const Vec3 &end) const;
    float               RayIntersection(const Vec3 &start, const Vec3 &dir) const;

    Sphere              ToSphere() const;
    OBB                 ToOBB() const;

                        /// Returns dimension of this type
    int                 GetDimension() const { return 7; }

    Vec3                p[2];
    float               radius;
};

BE_INLINE Cylinder::Cylinder(const Vec3 &p1, const Vec3 &p2, float r) {
    p[0] = p1;
    p[1] = p2;
    radius = r;
}

BE_INLINE bool Cylinder::Equals(const Cylinder &other) const {
    return (p[0].Equals(other.p[0]) && p[1].Equals(other.p[1]) && radius == other.radius);
}

BE_INLINE bool Cylinder::Equals(const Cylinder &other, const float epsilon) const {
    return (p[0].Equals(other.p[0], epsilon) && p[1].Equals(other.p[1], epsilon) && Math::Fabs(radius - other.radius) <= epsilon);
}

BE_NAMESPACE_END
