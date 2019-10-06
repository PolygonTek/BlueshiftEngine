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

    RectF

-------------------------------------------------------------------------------
*/

#include "Core/Str.h"

BE_NAMESPACE_BEGIN

class Rect;
class PointF;

/// A 2D floating point precision (x, y), (w, h) - rectangle.
class BE_API RectF {
public:
    /// The default constructor does not initialize any members of this class.
    RectF() = default;
    /// Constructs from coordinates.
    constexpr RectF(float x, float y, float w, float h);
    /// Assignment operator
    RectF &operator=(const RectF &rhs);

#ifdef QRECT_H
    /// Constructs from a QPoint.
    RectF(const QRectF &qrect) {
        this->x = qrect.x();
        this->y = qrect.y();
        this->w = qrect.width();
        this->h = qrect.height();
    }
#endif

    /// Constructs from a point.
    explicit constexpr RectF(const PointF &p);

    float               X2() const { return x + w; }
    float               Y2() const { return y + h; }

                        /// Returns a center point of this rect.
    PointF              GetCenter() const { return PointF(x + (w * 0.5f), y + (h * 0.5f)); }

                        /// Returns size of this rect.
    SizeF               GetSize() const { return SizeF(w, h); }

                        /// Casts this RectF to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const float *       Ptr() const { return (const float *)&x; }
    float *             Ptr() { return (float *)&x; }
                        /// Casts this RectF to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const float *() const { return (const float *)&x; }
                        operator float *() { return (float *)&x; }

                        /// Accesses an element of this RectF using array notation.
    float               operator[](int index) const;
    float &             operator[](int index);

                        /// Compare with another one
    bool                operator==(const RectF &rhs) const { return (x != rhs.x || y != rhs.y || w != rhs.w || h != rhs.h) ? false : true; }
                        /// Compare with another one
    bool                operator!=(const RectF &rhs) const { return (x != rhs.x || y != rhs.y || w != rhs.w || h != rhs.h) ? true : false; }

    bool                IsEmpty() const { return (w <= 0 || h <= 0) ? true : false; }

                        /// Tests whether a point is inside.
    bool                IsContainPoint(const PointF &pt) const;
                        /// Tests whether a point is inside.
    bool                IsContainPoint(const float x, const float y) const;
                        /// Tests whether a rect is inside.
    bool                IsContainRect(const RectF &a) const;
                        /// Tests whether a rect is inside.
    bool                IsContainRect(const float x, const float y, const float w, const float h) const;
                        /// Tests whether a rect is intersect.
    bool                IsIntersectRect(const RectF &a) const;
                        /// Tests whether a rect is intersect.
    bool                IsIntersectRect(const float x, const float y, const float w, const float h) const;

    void                Set(float x, float y, float w, float h);
    void                SetFrom4Coords(float x, float y, float x2, float y2);

    RectF               Add(const RectF &a) const;
    RectF &             Add(const RectF &a, const RectF &b);
    RectF &             AddSelf(const RectF &a);

    RectF               AddPoint(const PointF &a) const;
    RectF &             AddPoint(const PointF &a, const PointF &b);
    RectF &             AddPointSelf(const PointF &a);

    RectF               Intersect(const RectF &a) const;
    RectF &             Intersect(const RectF &a, const RectF &b);
    RectF &             IntersectSelf(const RectF &a);

    RectF               Move(float x, float y) const;
    RectF &             MoveSelf(float x, float y);

    RectF               Shrink(float ax, float ay) const;
    RectF &             ShrinkSelf(float ax, float ay);
    RectF               Expand(float ax, float ay) const;
    RectF &             ExpandSelf(float ax, float ay);

    Rect                ToRect() const { return Rect(x, y, w, h); }

                        /// Returns "x y w h".
    const char *        ToString() const { return ToString(4); }
                        /// Returns "x y w h" with the given precision.
    const char *        ToString(int precision) const;

                        /// Creates from the string.
    static RectF        FromString(const char *str);

#ifdef QRECT_H
    QRectF              ToQRectF() const { return QRectF(x, y, w, h); }
#endif

    static const RectF  empty;

    float               x;
    float               y;
    float               w;
    float               h;
};

BE_INLINE constexpr RectF::RectF(float inX, float inY, float inW, float inH) :
    x(inX), y(inY), w(inW), h(inH) {
}

BE_INLINE constexpr RectF::RectF(const PointF &p) :
    x(p.x), y(p.y), w(1), h(1) {
}

BE_INLINE RectF &RectF::operator=(const RectF &rhs) {
    x = rhs.x;
    y = rhs.y;
    w = rhs.w;
    h = rhs.h;
    return *this;
}

BE_INLINE float RectF::operator[](int index) const {
    assert(index >= 0 && index < 4);
    return ((float *)this)[index];
}

BE_INLINE float &RectF::operator[](int index) {
    assert(index >= 0 && index < 4);
    return ((float *)this)[index];
}

BE_INLINE bool RectF::IsContainPoint(const PointF &pt) const {
    return (pt.x < x || pt.y < y || pt.x >= x + w || pt.y >= y + h) ? false : true;
}

BE_INLINE bool RectF::IsContainPoint(const float x, const float y) const {
    return (x < this->x || y < this->y || x >= x + this->w || y >= y + this->h) ? false : true;
}

BE_INLINE bool RectF::IsContainRect(const RectF &a) const {
    return (a.x < x || a.y < y || a.x + a.w > x + w || a.y + a.h > y + h) ? false : true;
}

BE_INLINE bool RectF::IsContainRect(const float x, const float y, const float w, const float h) const {
    return (x < this->x || y < this->y || x + w > this->x + this->w || y + h > this->y + this->h) ? false : true;
}

BE_INLINE bool RectF::IsIntersectRect(const RectF &a) const {
    return (a.x + a.w < x || a.y + a.h < y || a.x >= x + w || a.y >= y + h) ? false : true;
}

BE_INLINE bool RectF::IsIntersectRect(const float x, const float y, const float w, const float h) const {
    return (x + w < this->x || y + h < this->y || x >= this->x + this->w || y >= this->y + this->h) ? false : true;
}

BE_INLINE void RectF::Set(float x, float y, float w, float h) {
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
}

BE_INLINE void RectF::SetFrom4Coords(float x, float y, float x2, float y2) {
    this->x = x;
    this->y = y;
    this->w = x2 - x;
    this->h = y2 - y;
}

BE_INLINE const char *RectF::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), 4, precision);
}

BE_NAMESPACE_END
