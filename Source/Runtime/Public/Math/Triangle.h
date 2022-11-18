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

    Triangle

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

/// Specifies a triangle through three points in 3D space.
class BE_API Triangle {
public:
    /// The default constructor does not initialize any members of this class.
    Triangle() = default;
    /// Constructs a triangle from three given endpoints.
    constexpr Triangle(const Vec3 &a, const Vec3 &b, const Vec3 &c);

                        /// Casts this Triangle to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const float *       Ptr() const { return (const float *)&a; }
    float *             Ptr() { return (float *)&a; }
                        /// Casts this Triangle to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const float *() const { return (const float *)&a; }
                        operator float *() { return (float *)&a; }

                        /// Accesses an element of this triangle.
    Vec3 &              At(int index) { return (*this)[index]; }
                        /// Accesses an element of this triangle using array notation.
    const Vec3 &        operator[](int index) const;
    Vec3 &              operator[](int index);

                        /// Exact compare, no epsilon.
    bool                Equals(const Triangle &t) const;
                        /// Compare with epsilon.
    bool                Equals(const Triangle &t, const float epsilon) const;
                        /// Exact compare, no epsilon.
    bool                operator==(const Triangle &rhs) const { return Equals(rhs); }
                        /// Exact compare, no epsilon.
    bool                operator!=(const Triangle &rhs) const { return !Equals(rhs); }

                        /// Computes the center of mass of this triangle.
    Vec3                Centroid() const;

                        /// Computes the surface area of this triangle.
    float               Area() const;

                        /// Computes the total edge length of this triangle.
    float               Perimeter() const;

                        /// Translates this triangle with the given translation offset.
    Triangle            Translate(const Vec3 &translation) const &;
    Triangle &&         Translate(const Vec3 &translation) && { TranslateSelf(translation); return std::move(*this); }
                        /// Translates this triangle with the given translation offset, in-place.
    Triangle &          TranslateSelf(const Vec3 &translation);

                        /// Applies a transformation to this triangle, in-place.
    void                Transform(const Mat3 &transform);
    void                Transform(const Mat3x4 &transform);
    void                Transform(const Mat4 &transform);
    void                Transform(const Quat &transform);

                        /// Expresses the given point in barycentric (u, v, w) coordinates.
    Vec3                BarycentricUVW(const Vec3 &point) const;
                        /// Expresses the given point in barycentric (u, v) coordinates.
    Vec2                BarycentricUV(const Vec3 &point) const;

                        /// Returns the point at the given barycentric (u, v, w) coordinates.
    Vec3                PointFromBarycentricUVW(const Vec3 &uvw) const;
                        /// Returns the point at the given barycentric (u, v) coordinates.
    Vec3                PointFromBarycentricUV(const Vec2 &uv) const;

                        /// Computes an unnormalized counter-clockwise oriented triangle normal vector.
    Vec3                UnnormalizedNormalCCW() const;
                        /// Computes an unnormalized clockwise-oriented triangle normal vector.
    Vec3                UnnormalizedNormalCW() const;

                        /// Returns the normalized triangle normal pointing to the counter-clockwise direction of this triangle.
    Vec3                NormalCCW() const;
                        /// Returns the normalized triangle normal pointing to the clockwise direction of this triangle.
    Vec3                NormalCW() const;

                        /// Returns the counterclockwise-oriented plane this triangle lies on.
    Plane               PlaneCCW() const;
                        /// Returns the clockwise-oriented plane this triangle lies on.
    Plane               PlaneCW() const;

                        /// Tests if this triangle contain the given point.
    bool                IsContainPoint(const Vec3 &point, float thicknessSq = 1e-5f) const;

                        /// Intersects a ray with this triangle.
                        /// Returns false if there is no intersection.
    bool                IntersectRay(const Ray &ray, float *hitDist) const;
                        /// Returns hit distance, but returns FLT_MAX if no intersection occurs.
    float               IntersectRay(const Ray &ray) const;

                        /// Returns the tight AABB that encloses this triangle.
    AABB                ToAABB() const;

                        /// Returns "a.x a.y a.z b.x b.y b.z c.x c.y c.z".
    const char *        ToString() const { return ToString(4); }
                        /// Returns "a.x a.y a.z b.x b.y b.z c.x c.y c.z" with the given precision.
    const char *        ToString(int precision) const;

                        /// Creates from the string.
    static Triangle     FromString(const char *str);

                        /// Returns dimension of this type.
    constexpr int       GetDimension() const { return 3*3; }

    Vec3                a;
    Vec3                b;
    Vec3                c;
};

BE_INLINE constexpr Triangle::Triangle(const Vec3 &inA, const Vec3 &inB, const Vec3 &inC) :
    a(inA), b(inB), c(inC) {
}

BE_INLINE const Vec3 &Triangle::operator[](int index) const {
    assert(index >= 0 && index < 3);
    return ((const Vec3 *)this)[index];
}

BE_INLINE Vec3 &Triangle::operator[](int index) {
    assert(index >= 0 && index < 3);
    return ((Vec3 *)this)[index];
}

BE_INLINE bool Triangle::Equals(const Triangle &t) const {
    return a.Equals(t.a) && b.Equals(t.b) && c.Equals(t.c); 
}

BE_INLINE bool Triangle::Equals(const Triangle &t, const float epsilon) const {
    return a.Equals(t.a, epsilon) && b.Equals(t.b, epsilon) && c.Equals(t.c, epsilon); 
}

BE_INLINE Triangle Triangle::Translate(const Vec3 &translation) const & {
    return Triangle(a + translation, b + translation, c + translation);
}

BE_INLINE Triangle &Triangle::TranslateSelf(const Vec3 &translation) {
    a += translation;
    b += translation;
    c += translation;
    return *this;
}

BE_INLINE void Triangle::Transform(const Mat3 &transform) {
    transform.BatchTransform((Vec3 *)&a, 3);
}

BE_INLINE void Triangle::Transform(const Mat3x4 &transform) {
    transform.BatchTransformPos((Vec3 *)&a, 3);
}

BE_INLINE void Triangle::Transform(const Mat4 &transform) {
    transform.BatchTransformPos((Vec3 *)&a, 3);
}

BE_INLINE void Triangle::Transform(const Quat &transform) {
    a = transform.RotateVector(a);
    b = transform.RotateVector(b);
    c = transform.RotateVector(c);
}

BE_INLINE const char *Triangle::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), GetDimension(), precision);
}

BE_NAMESPACE_END
