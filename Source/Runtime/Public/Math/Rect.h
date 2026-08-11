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

    Rect

-------------------------------------------------------------------------------
*/

#include "Core/Str.h"
#include "Math/Point.h"
#include "Math/Size.h"
#include <cassert>
#include <cstdio>
#include <type_traits>
#include <utility>

BE_NAMESPACE_BEGIN

#ifdef QRECT_H
template <typename T>
struct RectQtType;

template <>
struct RectQtType<int> {
    using Type = QRect;
};

template <>
struct RectQtType<float> {
    using Type = QRectF;
};
#endif

/// A 2D (x, y), (w, h) rectangle with the given scalar type.
template <typename T>
class RectT {
    static_assert(std::is_same<T, int>::value || std::is_same<T, float>::value, "RectT supports only int and float scalar types");

    static constexpr T MinValue(const T &a, const T &b) { return (a < b) ? a : b; }
    static constexpr T MaxValue(const T &a, const T &b) { return (a > b) ? a : b; }

public:
    using Scalar = T;
    using PointType = PointT<T>;
    using SizeType = SizeT<T>;

    /// The default constructor does not initialize any members of this class.
    RectT() = default;
    /// Constructs from coordinates.
    constexpr RectT(T x, T y, T w, T h);
    /// Assignment operator.
    RectT &operator=(const RectT &rhs) = default;

#ifdef QRECT_H
    using QtType = typename RectQtType<T>::Type;

    /// Constructs from the matching Qt rectangle type.
    RectT(const QtType &qrect) {
        this->x = static_cast<T>(qrect.x());
        this->y = static_cast<T>(qrect.y());
        this->w = static_cast<T>(qrect.width());
        this->h = static_cast<T>(qrect.height());
    }
#endif

    /// Constructs from a point.
    explicit constexpr RectT(const PointType &p);

                        /// Returns the x-coordinate of edge of the rectangle.
    T                   X2() const { return x + w; }
                        /// Returns the y-coordinate of edge of the rectangle.
    T                   Y2() const { return y + h; }

                        /// Returns a point of this rect with the given index.
    PointType           GetPoint(int index) const;

                        /// Returns a center point of this rect.
    PointType           GetCenter() const;

                        /// Returns size of this rect.
    SizeType            GetSize() const { return SizeType(w, h); }

                        /// Casts this RectT to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const T *           Ptr() const { return (const T *)&x; }
    T *                 Ptr() { return (T *)&x; }
                        /// Casts this RectT to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const T *() const { return (const T *)&x; }
                        operator T *() { return (T *)&x; }

                        /// Accesses an element of this RectT using array notation.
    T                   operator[](int index) const;
    T &                 operator[](int index);

                        /// Compare with another one.
    bool                operator==(const RectT &rhs) const { return (x != rhs.x || y != rhs.y || w != rhs.w || h != rhs.h) ? false : true; }
                        /// Compare with another one.
    bool                operator!=(const RectT &rhs) const { return (x != rhs.x || y != rhs.y || w != rhs.w || h != rhs.h) ? true : false; }

                        /// Returns if this rectangle is empty.
    bool                IsEmpty() const { return (w <= 0 || h <= 0) ? true : false; }

                        /// Tests whether a point is inside.
    bool                IsContainPoint(const PointType &pt) const;
                        /// Tests whether a point is inside.
    bool                IsContainPoint(T x, T y) const;
                        /// Tests whether a rect is inside.
    bool                IsContainRect(const RectT &a) const;
                        /// Tests whether a rect is inside.
    bool                IsContainRect(T x, T y, T w, T h) const;
                        /// Tests whether a rect is intersect.
    bool                IsIntersectRect(const RectT &a) const;
                        /// Tests whether a rect is intersect.
    bool                IsIntersectRect(T x, T y, T w, T h) const;

    void                Set(T x, T y, T w, T h);
    void                SetPosition(const PointType &position);
    void                SetSize(const SizeType &size);
    void                SetFrom4Coords(T x, T y, T x2, T y2);

    RectT               Add(const RectT &a) const &;
    RectT &&            Add(const RectT &a) && { AddSelf(a); return std::move(*this); }
    RectT &             Add(const RectT &a, const RectT &b);
    RectT &             AddSelf(const RectT &a);

    RectT               AddPoint(const PointType &a) const &;
    RectT &&            AddPoint(const PointType &a) && { AddPointSelf(a); return std::move(*this); }
    RectT &             AddPoint(const PointType &a, const PointType &b);
    RectT &             AddPointSelf(const PointType &a);

    RectT               Intersect(const RectT &a) const &;
    RectT &&            Intersect(const RectT &a) && { IntersectSelf(a); return std::move(*this); }
    RectT &             Intersect(const RectT &a, const RectT &b);
    RectT &             IntersectSelf(const RectT &a);

    RectT               Move(T x, T y) const &;
    RectT &&            Move(T x, T y) && { MoveSelf(x, y); return std::move(*this); }
    RectT &             MoveSelf(T x, T y);

    RectT               Shrink(T ax, T ay) const &;
    RectT &&            Shrink(T ax, T ay) && { ShrinkSelf(ax, ay); return std::move(*this); }
    RectT &             ShrinkSelf(T ax, T ay);

    RectT               Expand(T ax, T ay) const &;
    RectT &&            Expand(T ax, T ay) && { ExpandSelf(ax, ay); return std::move(*this); }
    RectT &             ExpandSelf(T ax, T ay);

    RectT<int>          ToRect() const;
    RectT<float>        ToRectF() const;

                        /// Returns "x y w h".
    const char *        ToString() const;
                        /// Returns "x y w h" with the given precision for floating point rectangles.
    const char *        ToString(int precision) const;

                        /// Creates from the string.
    static RectT        FromString(const char *str);

                        /// Returns dimension of this type.
    constexpr int       GetDimension() const { return 4; }

#ifdef QRECT_H
    QRect               ToQRect() const { return QRect(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h)); }
    QRectF              ToQRectF() const { return QRectF(static_cast<qreal>(x), static_cast<qreal>(y), static_cast<qreal>(w), static_cast<qreal>(h)); }
#endif

    static const RectT  zero;

    T                   x;
    T                   y;
    T                   w;
    T                   h;
};

template <typename T>
BE_INLINE constexpr RectT<T>::RectT(T inX, T inY, T inW, T inH) :
    x(inX), y(inY), w(inW), h(inH) {
}

template <typename T>
BE_INLINE constexpr RectT<T>::RectT(const PointType &p) :
    x(p.x), y(p.y), w(T(1)), h(T(1)) {
}

template <typename T>
BE_INLINE typename RectT<T>::PointType RectT<T>::GetPoint(int index) const {
    switch (index) {
    case 0: return PointType(x, y);
    case 1: return PointType(x + w, y);
    case 2: return PointType(x + w, y + h);
    case 3: return PointType(x, y + h);
    }
    return PointType::zero;
}

template <typename T>
BE_INLINE typename RectT<T>::PointType RectT<T>::GetCenter() const {
    if constexpr (std::is_same<T, int>::value) {
        return PointType(x + (w >> 1), y + (h >> 1));
    } else {
        return PointType(x + (w * 0.5f), y + (h * 0.5f));
    }
}

template <typename T>
BE_INLINE T RectT<T>::operator[](int index) const {
    assert(index >= 0 && index < 4);
    return ((const T *)this)[index];
}

template <typename T>
BE_INLINE T &RectT<T>::operator[](int index) {
    assert(index >= 0 && index < 4);
    return ((T *)this)[index];
}

template <typename T>
BE_INLINE bool RectT<T>::IsContainPoint(const PointType &pt) const {
    return (pt.x < x || pt.y < y || pt.x >= x + w || pt.y >= y + h) ? false : true;
}

template <typename T>
BE_INLINE bool RectT<T>::IsContainPoint(T x, T y) const {
    return (x < this->x || y < this->y || x >= this->x + this->w || y >= this->y + this->h) ? false : true;
}

template <typename T>
BE_INLINE bool RectT<T>::IsContainRect(const RectT &a) const {
    return (a.x < x || a.y < y || a.x + a.w > x + w || a.y + a.h > y + h) ? false : true;
}

template <typename T>
BE_INLINE bool RectT<T>::IsContainRect(T x, T y, T w, T h) const {
    return (x < this->x || y < this->y || x + w > this->x + this->w || y + h > this->y + this->h) ? false : true;
}

template <typename T>
BE_INLINE bool RectT<T>::IsIntersectRect(const RectT &a) const {
    return (a.x + a.w < x || a.y + a.h < y || a.x >= x + w || a.y >= y + h) ? false : true;
}

template <typename T>
BE_INLINE bool RectT<T>::IsIntersectRect(T x, T y, T w, T h) const {
    return (x + w < this->x || y + h < this->y || x >= this->x + this->w || y >= this->y + this->h) ? false : true;
}

template <typename T>
BE_INLINE void RectT<T>::Set(T x, T y, T w, T h) {
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
}

template <typename T>
BE_INLINE void RectT<T>::SetPosition(const PointType &position) {
    this->x = position.x;
    this->y = position.y;
}

template <typename T>
BE_INLINE void RectT<T>::SetSize(const SizeType &size) {
    this->w = size.w;
    this->h = size.h;
}

template <typename T>
BE_INLINE void RectT<T>::SetFrom4Coords(T x, T y, T x2, T y2) {
    this->x = x;
    this->y = y;
    this->w = x2 - x;
    this->h = y2 - y;
}

template <typename T>
BE_INLINE RectT<T> RectT<T>::Add(const RectT &a) const & {
    RectT rc;
    rc.x = MinValue(a.x, x);
    rc.y = MinValue(a.y, y);
    rc.w = MaxValue(a.x + a.w, x + w) - rc.x;
    rc.h = MaxValue(a.y + a.h, y + h) - rc.y;
    return rc;
}

template <typename T>
BE_INLINE RectT<T> &RectT<T>::Add(const RectT &a, const RectT &b) {
    x = MinValue(a.x, b.x);
    y = MinValue(a.y, b.y);
    w = MaxValue(a.x + a.w, b.x + b.w) - x;
    h = MaxValue(a.y + a.h, b.y + b.h) - y;
    return *this;
}

template <typename T>
BE_INLINE RectT<T> &RectT<T>::AddSelf(const RectT &a) {
    w = MaxValue(a.x + a.w, x + w);
    h = MaxValue(a.y + a.h, y + h);
    x = MinValue(a.x, x);
    y = MinValue(a.y, y);
    w -= x;
    h -= y;
    return *this;
}

template <typename T>
BE_INLINE RectT<T> RectT<T>::AddPoint(const PointType &a) const & {
    RectT rc;
    rc.x = MinValue(a.x, x);
    rc.y = MinValue(a.y, y);
    rc.w = MaxValue(a.x, x + w) - rc.x;
    rc.h = MaxValue(a.y, y + h) - rc.y;
    return rc;
}

template <typename T>
BE_INLINE RectT<T> &RectT<T>::AddPoint(const PointType &a, const PointType &b) {
    x = MinValue(a.x, b.x);
    y = MinValue(a.y, b.y);
    w = MaxValue(a.x, b.x) - x;
    h = MaxValue(a.y, b.y) - y;
    return *this;
}

template <typename T>
BE_INLINE RectT<T> &RectT<T>::AddPointSelf(const PointType &a) {
    w = MaxValue(a.x, x + w);
    h = MaxValue(a.y, y + h);
    x = MinValue(a.x, x);
    y = MinValue(a.y, y);
    w -= x;
    h -= y;
    return *this;
}

template <typename T>
BE_INLINE RectT<T> RectT<T>::Intersect(const RectT &a) const & {
    RectT rc;
    rc.x = MaxValue(a.x, x);
    rc.y = MaxValue(a.y, y);
    rc.w = MinValue(a.x + a.w, x + w) - rc.x;
    rc.h = MinValue(a.y + a.h, y + h) - rc.y;
    return rc;
}

template <typename T>
BE_INLINE RectT<T> &RectT<T>::Intersect(const RectT &a, const RectT &b) {
    x = MaxValue(a.x, b.x);
    y = MaxValue(a.y, b.y);
    w = MinValue(a.x + a.w, b.x + b.w) - x;
    h = MinValue(a.y + a.h, b.y + b.h) - y;
    return *this;
}

template <typename T>
BE_INLINE RectT<T> &RectT<T>::IntersectSelf(const RectT &a) {
    w = MinValue(a.x + a.w, x + w);
    h = MinValue(a.y + a.h, y + h);
    x = MaxValue(a.x, x);
    y = MaxValue(a.y, y);
    w -= x;
    h -= y;
    return *this;
}

template <typename T>
BE_INLINE RectT<T> RectT<T>::Move(T ax, T ay) const & {
    return RectT(x + ax, y + ay, w, h);
}

template <typename T>
BE_INLINE RectT<T> &RectT<T>::MoveSelf(T ax, T ay) {
    x += ax;
    y += ay;
    return *this;
}

template <typename T>
BE_INLINE RectT<T> RectT<T>::Shrink(T ax, T ay) const & {
    return RectT(x + ax, y + ay, w - ax * T(2), h - ay * T(2));
}

template <typename T>
BE_INLINE RectT<T> &RectT<T>::ShrinkSelf(T ax, T ay) {
    x += ax;
    y += ay;
    w -= ax * T(2);
    h -= ay * T(2);
    return *this;
}

template <typename T>
BE_INLINE RectT<T> RectT<T>::Expand(T ax, T ay) const & {
    return RectT(x - ax, y - ay, w + ax * T(2), h + ay * T(2));
}

template <typename T>
BE_INLINE RectT<T> &RectT<T>::ExpandSelf(T ax, T ay) {
    x -= ax;
    y -= ay;
    w += ax * T(2);
    h += ay * T(2);
    return *this;
}

template <typename T>
BE_INLINE RectT<int> RectT<T>::ToRect() const {
    return RectT<int>(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h));
}

template <typename T>
BE_INLINE RectT<float> RectT<T>::ToRectF() const {
    return RectT<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h));
}

template <typename T>
BE_INLINE const char *RectT<T>::ToString() const {
    if constexpr (std::is_same<T, int>::value) {
        return Str::IntegerArrayToString(Ptr(), GetDimension());
    } else {
        return ToString(4);
    }
}

template <typename T>
BE_INLINE const char *RectT<T>::ToString(int precision) const {
    if constexpr (std::is_same<T, int>::value) {
        return Str::IntegerArrayToString(Ptr(), GetDimension());
    } else {
        return Str::FloatArrayToString(Ptr(), GetDimension(), precision);
    }
}

template <typename T>
BE_INLINE RectT<T> RectT<T>::FromString(const char *str) {
    RectT r;
    int count;

    if constexpr (std::is_same<T, int>::value) {
        count = std::sscanf(str, "%i %i %i %i", &r.x, &r.y, &r.w, &r.h);
    } else {
        count = std::sscanf(str, "%f %f %f %f", &r.x, &r.y, &r.w, &r.h);
    }

    assert(count == r.GetDimension());
    return r;
}

template <typename T>
inline const RectT<T> RectT<T>::zero = RectT<T>(T(0), T(0), T(0), T(0));

using Rect = RectT<int>;
using RectF = RectT<float>;

static_assert(std::is_standard_layout<Rect>::value, "Rect must remain standard-layout");
static_assert(std::is_standard_layout<RectF>::value, "RectF must remain standard-layout");
static_assert(sizeof(Rect) == sizeof(int) * 4, "Rect layout changed");
static_assert(sizeof(RectF) == sizeof(float) * 4, "RectF layout changed");

BE_NAMESPACE_END
