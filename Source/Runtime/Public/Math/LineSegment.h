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

    Line Segment

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

/// A line segment in 3D space is a finite line with a start and end point.
class BE_API LineSegment {
public:
    /// The default constructor does not initialize any members of this class.
    LineSegment() = default;
    /// Constructs a line segment through the given end points.
    LineSegment(const Vec3 &a, const Vec3 &b);
    /// Constructs a line segment from a ray.
    LineSegment(const Ray &ray, float d);

                        /// Casts this LineSegment to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const float *       Ptr() const { return (const float *)&a; }
    float *             Ptr() { return (float *)&a; }
                        /// Casts this LineSegment to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const float *() const { return (const float *)&a; }
                        operator float *() { return (float *)&a; }

                        /// Accesses an element of this line segment.
    Vec3 &              At(int index) { return (*this)[index]; }
                        /// Accesses an element of this line segment using array notation.
    const Vec3 &        operator[](int index) const;
    Vec3 &              operator[](int index);

                        /// Exact compare, no epsilon.
    bool                Equals(const LineSegment &t) const;
                        /// Compare with epsilon.
    bool                Equals(const LineSegment &t, const float epsilon) const;
                        /// Exact compare, no epsilon.
    bool                operator==(const LineSegment &rhs) const { return Equals(rhs); }
                        /// Exact compare, no epsilon.
    bool                operator!=(const LineSegment &rhs) const { return !Equals(rhs); }

                        /// Returns the normalized direction vector that points in the direction a->b.
    Vec3                Dir() const;

                        /// Returns the length of this line segment.
    float               Length() const { return a.Distance(b); }
                        /// Returns the squared length of this line segment.
    float               LengthSqr() const { return a.DistanceSqr(b); }

                        /// Reverses the direction of this line segment.
    void                Reverse() { Swap(a, b); }

                        /// Returns the center point of this line segment.
    Vec3                CenterPoint() const { return (a + b) * 0.5f; }

                        /// Returns a point on the line with the given normalized distance along the line segment.
    Vec3                GetPoint(float fraction) const { return (1.0f - fraction) * a + fraction * b; }

                        /// Translates this line segment with the given translation offset.
    LineSegment         Translate(const Vec3 &translation) const &;
    LineSegment &&      Translate(const Vec3 &translation) && { TranslateSelf(translation); return std::move(*this); }
                        /// Translates this line segment with the given translation offset, in-place.
    LineSegment &       TranslateSelf(const Vec3 &translation);

                        /// Applies a transformation to this line segment, in-place.
    void                Transform(const Mat3 &transform);
    void                Transform(const Mat3x4 &transform);
    void                Transform(const Mat4 &transform);
    void                Transform(const Quat &transform);

                        /// Computes the closest point on this line to the given point.
    Vec3                ClosestPoint(const Vec3 &point) const;

                        /// Computes the distance between this line and the given point.
    float               Distance(const Vec3 &point) const;
                        /// Computes the squared distance between this line and the given point.
    float               DistanceSqr(const Vec3 &point) const;

                        /// Converts this LineSegment to a Ray.
    Ray                 ToRay() const;

                        /// Returns "a.x a.y a.z b.x b.y b.z".
    const char *        ToString() const { return ToString(4); }
                        /// Returns "a.x a.y a.z b.x b.y b.z" with the given precision.
    const char *        ToString(int precision) const;

                        /// Creates from the string.
    static LineSegment  FromString(const char *str);

                        /// Returns dimension of this type.
    constexpr int       GetDimension() const { return 3*3; }

    Vec3                a;  ///< The starting point of this line segment.
    Vec3                b;  ///< The end point of this line segment.
};

BE_INLINE LineSegment::LineSegment(const Vec3 &inA, const Vec3 &inB) :
    a(inA), b(inB) {
}

BE_INLINE const Vec3 &LineSegment::operator[](int index) const {
    assert(index >= 0 && index < 2);
    return ((const Vec3 *)this)[index];
}

BE_INLINE Vec3 &LineSegment::operator[](int index) {
    assert(index >= 0 && index < 2);
    return ((Vec3 *)this)[index];
}

BE_INLINE bool LineSegment::Equals(const LineSegment &t) const {
    return a.Equals(t.a) && b.Equals(t.b);
}

BE_INLINE bool LineSegment::Equals(const LineSegment &t, const float epsilon) const {
    return a.Equals(t.a, epsilon) && b.Equals(t.b, epsilon);
}

BE_INLINE Vec3 LineSegment::Dir() const {
    return (b - a).Normalized();
}

BE_INLINE LineSegment LineSegment::Translate(const Vec3 &translation) const & {
    return LineSegment(a + translation, b + translation);
}

BE_INLINE LineSegment &LineSegment::TranslateSelf(const Vec3 &translation) {
    a += translation;
    b += translation;
    return *this;
}

BE_INLINE void LineSegment::Transform(const Mat3 &transform) {
    a = transform * a;
    b = transform * b;
}

BE_INLINE void LineSegment::Transform(const Mat3x4 &transform) {
    a = transform.TransformPos(a);
    b = transform.TransformPos(b);
}

BE_INLINE void LineSegment::Transform(const Mat4 &transform) {
    a = transform.TransformPos(a);
    b = transform.TransformPos(b);
}

BE_INLINE void LineSegment::Transform(const Quat &transform) {
    a = transform * a;
    b = transform * b;
}

BE_INLINE const char *LineSegment::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), GetDimension(), precision);
}

BE_NAMESPACE_END
