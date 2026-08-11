// Copyright(c) 2017 POLYGONTEK
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

/*
-------------------------------------------------------------------------------

    Point

-------------------------------------------------------------------------------
*/

#include "Core/Str.h"
#include "Math/MathCommon.h"
#include "Math/Vector2.h"
#include <cassert>
#include <cstdio>
#include <type_traits>
#include <utility>

BE_NAMESPACE_BEGIN

#ifdef QPOINT_H
template <typename T>
struct PointQtType;

template <>
struct PointQtType<int> {
    using Type = QPoint;
};

template <>
struct PointQtType<float> {
    using Type = QPointF;
};
#endif

/// A 2D (x, y) point with the given scalar type.
template <typename T>
class PointT {
    static_assert(std::is_same<T, int>::value || std::is_same<T, float>::value, "PointT supports only int and float scalar types");

public:
    using Scalar = T;

    /// The default constructor does not initialize any members of this class.
    PointT() = default;
    /// Constructs a PointT with the value (x, y).
    constexpr PointT(T x, T y);
    /// Assignment operator.
    PointT &operator=(const PointT &rhs) = default;

#ifdef QPOINT_H
    using QtType = typename PointQtType<T>::Type;

    /// Constructs from the matching Qt point type.
    PointT(const QtType &qpoint) {
        this->x = static_cast<T>(qpoint.x());
        this->y = static_cast<T>(qpoint.y());
    }
#endif

    /// Constructs a PointT from a C array, to the value (data[0], data[1]).
    explicit constexpr PointT(T data[2]);
    /// Constructs from a Vec2.
    explicit PointT(const Vec2 &v);

                        /// Casts this PointT to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const T *           Ptr() const { return (const T *)&x; }
    T *                 Ptr() { return (T *)&x; }
                        /// Casts this PointT to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const T *() const { return (const T *)&x; }
                        operator T *() { return (T *)&x; }

                        /// Accesses an element of this PointT using array notation.
    T                   operator[](int index) const;
    T &                 operator[](int index);

                        /// Unary operator + allows this structure to be used in an expression '+p'.
    PointT              operator+() const { return *this; }

                        /// Performs an unary negation of this PointT.
    PointT              operator-() const & { return PointT(-x, -y); }
    PointT &&           operator-() && { x = -x; y = -y; return std::move(*this); }

                        /// Adds a point to this point.
    PointT              operator+(const PointT &rhs) const & { return PointT(x + rhs.x, y + rhs.y); }
    PointT &&           operator+(const PointT &rhs) && { *this += rhs; return std::move(*this); }

                        /// Subtracts a point from this point.
    PointT              operator-(const PointT &rhs) const & { return PointT(x - rhs.x, y - rhs.y); }
    PointT &&           operator-(const PointT &rhs) && { *this -= rhs; return std::move(*this); }

                        /// Multiplies this point by a scalar.
    PointT              operator*(float rhs) const & { return PointT(x * rhs, y * rhs); }
    PointT &&           operator*(float rhs) && { *this *= rhs; return std::move(*this); }

                        /// Adds a point to this point, in-place.
    PointT &            operator+=(const PointT &rhs);

                        /// Subtracts a point from this point, in-place.
    PointT &            operator-=(const PointT &rhs);

                        /// Multiplies this point by a scalar, in-place.
    PointT &            operator*=(float rhs);

                        /// Compare with another one.
    bool                operator==(const PointT &rhs) const { return (x != rhs.x || y != rhs.y) ? false : true; }
                        /// Compare with another one.
    bool                operator!=(const PointT &rhs) const { return (x != rhs.x || y != rhs.y) ? true : false; }

    void                Set(T x, T y);

                        /// Computes the distance between this point and the given point.
    float               Distance(const PointT &other) const;
                        /// Computes the squared distance between this point and the given point.
    float               DistanceSqr(const PointT &other) const;

    PointT<int>         ToPoint() const;
    PointT<float>       ToPointF() const;

                        /// Converts this point to Vec2.
    Vec2                ToVec2() const { return Vec2(static_cast<float>(x), static_cast<float>(y)); }

#ifdef QPOINT_H
    QPoint              ToQPoint() const { return QPoint(static_cast<int>(x), static_cast<int>(y)); }
    QPointF             ToQPointF() const { return QPointF(static_cast<qreal>(x), static_cast<qreal>(y)); }
#endif

                        /// Returns "x y".
    const char *        ToString() const;
                        /// Returns "x y" with the given precision for floating point points.
    const char *        ToString(int precision) const;

                        /// Creates from the string.
    static PointT       FromString(const char *str);

                        /// Returns dimension of this type.
    constexpr int       GetDimension() const { return 2; }

    static const PointT zero;

    T                   x;
    T                   y;
};

template <typename T>
BE_INLINE constexpr PointT<T>::PointT(T inX, T inY) :
    x(inX), y(inY) {
}

template <typename T>
BE_INLINE constexpr PointT<T>::PointT(T data[2]) :
    x(data[0]), y(data[1]) {
}

template <typename T>
BE_INLINE PointT<T>::PointT(const Vec2 &v) :
    x(static_cast<T>(v.x)), y(static_cast<T>(v.y)) {
}

template <typename T>
BE_INLINE T PointT<T>::operator[](int index) const {
    assert(index >= 0 && index < 2);
    return ((const T *)this)[index];
}

template <typename T>
BE_INLINE T &PointT<T>::operator[](int index) {
    assert(index >= 0 && index < 2);
    return ((T *)this)[index];
}

template <typename T>
BE_INLINE PointT<T> &PointT<T>::operator+=(const PointT &rhs) {
    x += rhs.x;
    y += rhs.y;
    return *this;
}

template <typename T>
BE_INLINE PointT<T> &PointT<T>::operator-=(const PointT &rhs) {
    x -= rhs.x;
    y -= rhs.y;
    return *this;
}

template <typename T>
BE_INLINE PointT<T> &PointT<T>::operator*=(float rhs) {
    x *= rhs;
    y *= rhs;
    return *this;
}

template <typename T>
BE_INLINE void PointT<T>::Set(T x, T y) {
    this->x = x;
    this->y = y;
}

template <typename T>
BE_INLINE float PointT<T>::Distance(const PointT &other) const {
    float dx = x - other.x;
    float dy = y - other.y;
    return Math::Sqrt(dx * dx + dy * dy);
}

template <typename T>
BE_INLINE float PointT<T>::DistanceSqr(const PointT &other) const {
    float dx = x - other.x;
    float dy = y - other.y;
    return dx * dx + dy * dy;
}

template <typename T>
BE_INLINE PointT<int> PointT<T>::ToPoint() const {
    return PointT<int>(static_cast<int>(x), static_cast<int>(y));
}

template <typename T>
BE_INLINE PointT<float> PointT<T>::ToPointF() const {
    return PointT<float>(static_cast<float>(x), static_cast<float>(y));
}

template <typename T>
BE_INLINE const char *PointT<T>::ToString() const {
    if constexpr (std::is_same<T, int>::value) {
        return Str::IntegerArrayToString(Ptr(), GetDimension());
    } else {
        return ToString(4);
    }
}

template <typename T>
BE_INLINE const char *PointT<T>::ToString(int precision) const {
    if constexpr (std::is_same<T, int>::value) {
        return Str::IntegerArrayToString(Ptr(), GetDimension());
    } else {
        return Str::FloatArrayToString(Ptr(), GetDimension(), precision);
    }
}

template <typename T>
BE_INLINE PointT<T> PointT<T>::FromString(const char *str) {
    PointT p;
    int count;

    if constexpr (std::is_same<T, int>::value) {
        count = std::sscanf(str, "%i %i", &p.x, &p.y);
    } else {
        count = std::sscanf(str, "%f %f", &p.x, &p.y);
    }

    assert(count == p.GetDimension());
    return p;
}

template <typename T>
inline const PointT<T> PointT<T>::zero = PointT<T>(T(0), T(0));

using Point = PointT<int>;
using PointF = PointT<float>;

static_assert(std::is_standard_layout<Point>::value, "Point must remain standard-layout");
static_assert(std::is_standard_layout<PointF>::value, "PointF must remain standard-layout");
static_assert(sizeof(Point) == sizeof(int) * 2, "Point layout changed");
static_assert(sizeof(PointF) == sizeof(float) * 2, "PointF layout changed");

BE_NAMESPACE_END
