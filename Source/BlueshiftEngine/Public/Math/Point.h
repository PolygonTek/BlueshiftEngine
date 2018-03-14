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

    Point

-------------------------------------------------------------------------------
*/

#include "Core/Str.h"

BE_NAMESPACE_BEGIN

/// A 2D integral (x,y) - point.
class BE_API Point {
public:
    /// The default constructor does not initialize any members of this class.
    Point() = default;
    /// Constructs a Point with the value (x, y).
    Point(int x, int y);
    /// Assignment operator
    Point &operator=(const Point &rhs);

#ifdef QPOINT_H
    /// Constructs from a QPoint.
    Point(const QPoint &qpoint) {
        this->x = qpoint.x();
        this->y = qpoint.y();
    }
#endif

    /// Constructs a Point from a C array, to the value (data[0], data[1]).
    explicit Point(int data[2]);
    /// Copy constructor
    explicit Point(const Vec2 &v);

                        /// Casts this Point to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const int *         Ptr() const { return (const int *)&x; }
    int *               Ptr() { return (int *)&x; }
                        /// Casts this Point to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const int *() const { return (const int *)&x; }
                        operator int *() { return (int *)&x; }

                        /// Accesses an element of this Point using array notation.
    int                 operator[](int index) const;
    int &               operator[](int index);

                        /// Performs an unary negation of this Point.
    Point               operator-() const { return Point(-x, -y); }

                        /// Unary operator + allows this structure to be used in an expression '+p'.
    Point               operator+() const { return *this; }

    Point &             operator+=(const Point &rhs);
    Point &             operator-=(const Point &rhs);

    Point               operator+(const Point &rhs) const { return Point(x + rhs.x, y + rhs.y); }
    Point               operator-(const Point &rhs) const { return Point(x - rhs.x, y - rhs.y); }

                        /// Compare with another one
    bool                operator==(const Point &rhs) { return (x != rhs.x || y != rhs.y) ? false : true; }
                        /// Compare with another one
    bool                operator!=(const Point &rhs) { return (x != rhs.x || y != rhs.y) ? true : false; }

    void                Set(int x, int y);

    float               Distance(const Point &other) const;
    float               DistanceSqr(const Point &other) const;

    Vec2                ToVec2() const { return Vec2(x, y); }

#ifdef QPOINT_H
    QPoint              ToQPoint() const { return QPoint(x, y); }
#endif

                        /// Returns "x y".
    const char *        ToString() const;

                        /// Creates from the string
    static Point        FromString(const char *str);

    static const Point  zero;

    int                 x;
    int                 y;
};

BE_INLINE Point::Point(int x, int y) {
    this->x = x;
    this->y = y;
}

BE_INLINE Point::Point(int data[2]) {
    x = data[0];
    y = data[1];
}

BE_INLINE Point::Point(const Vec2 &v) {
    x = (int)v.x;
    y = (int)v.y;
}

BE_INLINE Point &Point::operator=(const Point &rhs) {
    x = rhs.x;
    y = rhs.y;
    return *this;
}

BE_INLINE int Point::operator[](int index) const {
    assert(index >= 0 && index < 2);
    return ((int *)this)[index];
}

BE_INLINE int &Point::operator[](int index) {
    assert(index >= 0 && index < 2);
    return ((int *)this)[index];
}

BE_INLINE Point &Point::operator+=(const Point &rhs) {
    x += rhs.x;
    y += rhs.y;
    return *this;
}

BE_INLINE Point &Point::operator-=(const Point &rhs) {
    x -= rhs.x;
    y -= rhs.y;
    return *this;
}

BE_INLINE void Point::Set(int x, int y) {
    this->x = x;
    this->y = y;
}

BE_INLINE float Point::Distance(const Point &other) const {
    float dx = x - other.x;
    float dy = y - other.y;
    return Math::Sqrt(dx * dx + dy * dy);
}

BE_INLINE float Point::DistanceSqr(const Point &other) const {
    float dx = x - other.x;
    float dy = y - other.y;
    return (dx * dx + dy * dy);
}

BE_INLINE const char *Point::ToString() const {
    return Str::IntegerArrayToString((const int *)(*this), 2);
}

BE_NAMESPACE_END
