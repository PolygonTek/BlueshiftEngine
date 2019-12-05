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

    PointF

-------------------------------------------------------------------------------
*/

#include "Core/Str.h"

BE_NAMESPACE_BEGIN

class Point;

/// A 2D floating point precision (x, y) - point.
class BE_API PointF {
public:
    /// The default constructor does not initialize any members of this class.
    PointF() = default;
    /// Constructs a PointF with the value (x, y).
    constexpr PointF(float x, float y);
    /// Assignment operator
    PointF &operator=(const PointF &rhs);

#ifdef QPOINT_H
    /// Constructs from a QPoint.
    PointF(const QPointF &qpoint) {
        this->x = qpoint.x();
        this->y = qpoint.y();
    }
#endif

    /// Constructs a PointF from a C array, to the value (data[0], data[1]).
    explicit constexpr PointF(float data[2]);
    /// Copy constructor.
    explicit PointF(const Vec2 &v);

                        /// Casts this PointF to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const float *       Ptr() const { return (const float *)&x; }
    float *             Ptr() { return (float *)&x; }
                        /// Casts this PointF to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const float *() const { return (const float *)&x; }
                        operator float *() { return (float *)&x; }

                        /// Accesses an element of this PointF using array notation.
    float               operator[](int index) const;
    float &             operator[](int index);

                        /// Performs an unary negation of this PointF.
    PointF              operator-() const { return PointF(-x, -y); }

                        /// Unary operator + allows this structure to be used in an expression '+p'.
    PointF              operator+() const { return *this; }

    PointF &            operator+=(const PointF &rhs);
    PointF &            operator-=(const PointF &rhs);
    PointF &            operator*=(float rhs);

    PointF              operator+(const PointF &rhs) const { return PointF(x + rhs.x, y + rhs.y); }
    PointF              operator-(const PointF &rhs) const { return PointF(x - rhs.x, y - rhs.y); }
    PointF              operator*(float rhs) const { return PointF(x * rhs, y * rhs); }

                        /// Compare with another one.
    bool                operator==(const PointF &rhs) const { return (x != rhs.x || y != rhs.y) ? false : true; }
                        /// Compare with another one.
    bool                operator!=(const PointF &rhs) const { return (x != rhs.x || y != rhs.y) ? true : false; }

    void                Set(float x, float y);

    float               Distance(const PointF &other) const;
    float               DistanceSqr(const PointF &other) const;

    Point               ToPoint() const { return Point(x, y); }

    Vec2                ToVec2() const { return Vec2(x, y); }

#ifdef QPOINT_H
    QPointF             ToQPointF() const { return QPointF(x, y); }
#endif

                        /// Returns "x y".
    const char *        ToString() const { return ToString(4); }
                        /// Returns "x y" with the given precision.
    const char *        ToString(int precision) const;

                        /// Creates from the string.
    static PointF       FromString(const char *str);

                        /// Returns dimension of this type.
    constexpr int       GetDimension() const { return 2; }

    static const PointF zero;

    float               x;
    float               y;
};

BE_INLINE constexpr PointF::PointF(float inX, float inY) :
    x(inX), y(inY) {
}

BE_INLINE constexpr PointF::PointF(float data[2]) :
    x(data[0]), y(data[1]) {
}

BE_INLINE PointF::PointF(const Vec2 &v) {
    x = v.x;
    y = v.y;
}

BE_INLINE PointF &PointF::operator=(const PointF &rhs) {
    x = rhs.x;
    y = rhs.y;
    return *this;
}

BE_INLINE float PointF::operator[](int index) const {
    assert(index >= 0 && index < 2);
    return ((float *)this)[index];
}

BE_INLINE float &PointF::operator[](int index) {
    assert(index >= 0 && index < 2);
    return ((float *)this)[index];
}

BE_INLINE PointF &PointF::operator+=(const PointF &rhs) {
    x += rhs.x;
    y += rhs.y;
    return *this;
}

BE_INLINE PointF &PointF::operator-=(const PointF &rhs) {
    x -= rhs.x;
    y -= rhs.y;
    return *this;
}

BE_INLINE PointF &PointF::operator*=(float rhs) {
    x *= rhs;
    y *= rhs;
    return *this;
}

BE_INLINE void PointF::Set(float x, float y) {
    this->x = x;
    this->y = y;
}

BE_INLINE float PointF::Distance(const PointF &other) const {
    float dx = x - other.x;
    float dy = y - other.y;
    return Math::Sqrt(dx * dx + dy * dy);
}

BE_INLINE float PointF::DistanceSqr(const PointF &other) const {
    float dx = x - other.x;
    float dy = y - other.y;
    return (dx * dx + dy * dy);
}

BE_INLINE const char *PointF::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), GetDimension(), precision);
}

BE_NAMESPACE_END
