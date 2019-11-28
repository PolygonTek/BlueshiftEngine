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

    Plane

    a*x + b*y + c*z = d

    abc = plane normal
    d = plane offset from origin

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

#define ON_EPSILON                  0.1f
#define DEGENERATE_DIST_EPSILON     1e-4f

class Ray;

/// Surface in three-dimensional space.
class BE_API Plane {
public:
    /// Plane type
    struct Type {
        enum Enum {
            X           = 0,
            Y           = 1,
            Z           = 2,
            NegX        = 3,
            NegY        = 4,
            NegZ        = 5,
            TrueAxial   = 6,    // all types < 6 are true axial planes
            ZeroX       = 6,
            ZeroY       = 7,
            ZeroZ       = 8,
            NonAxial    = 9
        };
    };

    /// Plane sides
    struct Side {
        enum Enum {
            Front       = 0,
            Back        = 1,
            On          = 2,
            Cross       = 3
        };
    };

    /// The default constructor does not initialize any members of this class.
    Plane() = default;
    /// Constructs a plane by directly specifying the normal and offset parameters.
    constexpr Plane(const Vec3 &normal, float offset);
    /// Constructs a plane by specifying the surface normal and a single point on the plane.
    Plane(const Vec3 &normal, const Vec3 &point);

    Type::Enum          GetType() const;
    
                        /// Casts this Plane to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const float *       Ptr() const { return (const float *)&normal; }
    float *             Ptr() { return (float *)&normal; }
                        /// Casts this Plane to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const float *() const { return (const float *)&normal; }
                        operator float *() { return (float *)&normal; }

                        /// Accesses an element of this plane.
    float &             At(int index) { return (*this)[index]; }
                        /// Accesses an element of this plane using array notation.
    float               operator[](int index) const;
    float &             operator[](int index);

    bool                FixDegenerateNormal() { return normal.FixDegenerateNormal(); }

                        /// Sets this plane by specifying three points on the plane.
                        /// The normal of the plane will be oriented in counter-clockwise order.
    bool                SetFromPoints(const Vec3 &p1, const Vec3 &p2, const Vec3 &p3, bool fixDegenerate = true);

                        /// Moves this plane by specifying a single point on the plane.
    void                FitThroughPoint(const Vec3 &p) { offset = normal.Dot(p); }

                        /// Reverses the direction of the plane.
    void                Flip();

                        /// Normalize plane. 
                        /// Only normalizes the plane normal, does not adjust d
    float               Normalize(bool fixDegenerate = true);

                        /// Translates this Plane
    Plane               Translate(const Vec3 &translation) const;
    Plane &             TranslateSelf(const Vec3 &translation);

                        /// Rotates this Plane
    Plane               Rotate(const Vec3 &origin, const Mat3 &axis) const;
    Plane &             RotateSelf(const Vec3 &origin, const Mat3 &axis);

                        /// Returns the distance of this plane to the given point.
    float               Distance(const Vec3 &p) const { return normal.Dot(p) - offset; }

                        /// Examines which side of the plane to the given point.
    Side::Enum          GetSide(const Vec3 &p, const float epsilon) const;

                        /// Tests if this plane intersect with the given line segment.
    bool                IsIntersectLine(const Vec3 &p1, const Vec3 &p2) const;

                        /// Intersects a ray with this plane.
                        /// Returns false if there is no intersection.
    bool                IntersectRay(const Ray &ray, bool ignoreBackside, float *hitDist) const;
                        /// Returns hit distance, but returns FLT_MAX if no intersection occurs.
    float               IntersectRay(const Ray &ray, bool ignoreBackside) const;

                        /// Returns "a b c d".
    const char *        ToString() const { return ToString(4); }
                        /// Returns "a b c d" with the given precision.
    const char *        ToString(int precision) const;

                        /// Creates from the string
    static Plane        FromString(const char *str);

                        /// Returns dimension of this type
    int                 GetDimension() const { return 4; }

    Vec3                normal;     ///< The direction this plane is facing at.
    float               offset;     ///< The offset of this plane from the origin.
};

BE_INLINE constexpr Plane::Plane(const Vec3 &n, float o) :
    normal(n), offset(o) {
}

BE_INLINE Plane::Plane(const Vec3 &n, const Vec3 &p) :
    normal(n) {
    offset = normal.Dot(p);
}

BE_INLINE float Plane::operator[](int index) const {
    assert(index >= 0 && index < 4);
    return normal.Ptr()[index];
}

BE_INLINE float &Plane::operator[](int index) {
    assert(index >= 0 && index < 4);
    return normal.Ptr()[index];
}

BE_INLINE bool Plane::SetFromPoints(const Vec3 &p1, const Vec3 &p2, const Vec3 &p3, bool fixDegenerate) {
    normal = (p3 - p2).Cross(p1 - p2);
    if (Normalize(fixDegenerate) == 0.0f) {
        return false;
    }

    offset = normal.Dot(p2);
    return true;
}

BE_INLINE void Plane::Flip() {
    normal = -normal;
    offset = -offset;
}

BE_INLINE float Plane::Normalize(bool fixDegenerate) {
    float length = normal.Normalize();

    if (fixDegenerate) {
        FixDegenerateNormal();
    }
    return length;
}

BE_INLINE Plane Plane::Translate(const Vec3 &translation) const {
    return Plane(normal, offset + translation.Dot(normal));
}

BE_INLINE Plane &Plane::TranslateSelf(const Vec3 &translation) {
    offset += translation.Dot(normal);
    return *this;
}

BE_INLINE Plane Plane::Rotate(const Vec3 &origin, const Mat3 &axis) const {
    Plane p;
    p.normal = axis * normal;
    p.offset = offset - origin.Dot(normal) + origin.Dot(p.normal);
    return p;
}

BE_INLINE Plane &Plane::RotateSelf(const Vec3 &origin, const Mat3 &axis) {
    offset -= origin.Dot(normal);
    normal = axis * normal;
    offset += origin.Dot(normal);
    return *this;
}

BE_INLINE Plane::Side::Enum Plane::GetSide(const Vec3 &v, const float epsilon) const {
    float d = Distance(v);
    if (d > epsilon) {
        return Side::Front;
    } else if (d < -epsilon) {
        return Side::Back;
    } else {
        return Side::On;
    }
}

BE_NAMESPACE_END
