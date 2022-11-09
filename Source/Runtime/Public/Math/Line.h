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

    Line

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class LineSegment;

/// A line in 3D space is defined by an origin point and a direction, and extends to infinity in two directions.
class BE_API Line {
public:
    /// The default constructor does not initialize any members of this class.
    Line() = default;
    /// Constructs a new line by explicitly specifying the member variables.
    Line(const Vec3 &pos, const Vec3 &dir);
    /// Constructs a line from a line segment.
    Line(const LineSegment &lineSegment);
    /// Constructs a line from a ray.
    Line(const Ray &ray);

                        /// Casts this Line to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const float *       Ptr() const { return (const float *)&pos; }
    float *             Ptr() { return (float *)&pos; }
                        /// Casts this Line to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const float *() const { return (const float *)&pos; }
                        operator float *() { return (float *)&pos; }

                        /// Accesses an element of this line.
    Vec3 &              At(int index) { return (*this)[index]; }
                        /// Accesses an element of this line using array notation.
    const Vec3 &        operator[](int index) const;
    Vec3 &              operator[](int index);

                        /// Exact compare, no epsilon.
    bool                Equals(const Line &line) const;
                        /// Compare with epsilon.
    bool                Equals(const Line &line, const float epsilon) const;
                        /// Exact compare, no epsilon.
    bool                operator==(const Line &rhs) const { return Equals(rhs); }
                        /// Exact compare, no epsilon.
    bool                operator!=(const Line &rhs) const { return !Equals(rhs); }

                        /// Gets a point along the line at the given distance.
    Vec3                GetPoint(float fraction) const { return pos + dir * fraction; }

                        /// Translates this line with the given translation offset.
    Line                Translate(const Vec3 &translation) const &;
    Line &&             Translate(const Vec3 &translation) && { TranslateSelf(translation); return std::move(*this); };
                        /// Translates this line with the given translation offset, in-place.
    Line &              TranslateSelf(const Vec3 &translation);

                        /// Applies a transformation to this line, in-place.
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

                        /// Converts this Line to a LineSegment.
    LineSegment         ToLineSegment(float d) const;
                        /// Converts this Line to a LineSegment.
    LineSegment         ToLineSegment(float dStart, float dEnd) const;
                        /// Converts this Line to a Ray.
    Ray                 ToRay() const;

                        /// Returns "pos.x pos.y pos.z dir.x dir.y dir.z".
    const char *        ToString() const { return ToString(4); }
                        /// Returns "pos.x pos.y pos.z dir.x dir.y dir.z" with the given precision.
    const char *        ToString(int precision) const;

                        /// Creates from the string.
    static Line         FromString(const char *str);

                        /// Returns dimension of this type.
    constexpr int       GetDimension() const { return 3*3; }

    Vec3                pos;    ///< Specifies the origin of this line.
    Vec3                dir;    ///< The normalized direction vector of this ray.
};

BE_INLINE Line::Line(const Vec3 &inPos, const Vec3 &inDir) :
    pos(inPos), dir(inDir) {
}

BE_INLINE Line::Line(const Ray &ray) {
    pos = ray.origin;
    dir = ray.dir;
}

BE_INLINE const Vec3 &Line::operator[](int index) const {
    assert(index >= 0 && index < 2);
    return ((const Vec3 *)this)[index];
}

BE_INLINE Vec3 &Line::operator[](int index) {
    assert(index >= 0 && index < 2);
    return ((Vec3 *)this)[index];
}

BE_INLINE bool Line::Equals(const Line &line) const {
    return pos.Equals(line.pos) && dir.Equals(line.dir);
}

BE_INLINE bool Line::Equals(const Line &line, const float epsilon) const {
    return pos.Equals(line.pos , epsilon) && dir.Equals(line.dir, epsilon);
}

BE_INLINE Line Line::Translate(const Vec3 &translation) const & {
    return Line(pos + translation, dir);
}

BE_INLINE Line &Line::TranslateSelf(const Vec3 &translation) {
    pos += translation;
    return *this;
}

BE_INLINE void Line::Transform(const Mat3 &transform) {
    pos = transform * pos;
    dir = transform * dir;
}

BE_INLINE void Line::Transform(const Mat3x4 &transform) {
    pos = transform.TransformPos(pos);
    dir = transform.TransformDir(dir);
}

BE_INLINE void Line::Transform(const Mat4 &transform) {
    pos = transform.TransformPos(pos);
    dir = transform.TransformDir(dir);
}

BE_INLINE void Line::Transform(const Quat &transform) {
    pos = transform * pos;
    dir = transform * dir;
}

BE_INLINE const char *Line::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), GetDimension(), precision);
}

BE_NAMESPACE_END
