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

    Rect

-------------------------------------------------------------------------------
*/

#include "Core/Str.h"

BE_NAMESPACE_BEGIN

class Point;
class RectF;

/// A 2D integral (x,y), (w,h) - rectangle.
class BE_API Rect {
public:
    /// The default constructor does not initialize any members of this class.
    Rect() = default;
    /// Constructs from coordinates.
    constexpr Rect(int x, int y, int w, int h);
    /// Assignment operator.
    Rect &operator=(const Rect &rhs);

#ifdef QRECT_H
    /// Constructs from a QRect.
    Rect(const QRect &qrect) {
        this->x = qrect.x();
        this->y = qrect.y();
        this->w = qrect.width();
        this->h = qrect.height();
    }
#endif

    /// Constructs from a point.
    explicit constexpr Rect(const Point &p);

                        /// Returns the x-coordinate of edge of the rectangle.
    int                 X2() const { return x + w; }
                        /// Returns the y-coordinate of edge of the rectangle.
    int                 Y2() const { return y + h; }

                        /// Returns a point of this rect with the given index.
    Point               GetPoint(int index) const;

                        /// Returns a center point of this rect.
    Point               GetCenter() const { return Point(x + (w >> 1), y + (h >> 1)); }

                        /// Returns size of this rect.
    Size                GetSize() const { return Size(w, h); }

                        /// Casts this Rect to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const int *         Ptr() const { return (const int *)&x; }
    int *               Ptr() { return (int *)&x; }
                        /// Casts this Rect to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const int *() const { return (const int *)&x; }
                        operator int *() { return (int *)&x; }

                        /// Accesses an element of this Rect using array notation.
    int                 operator[](int index) const;
    int &               operator[](int index);

                        /// Compare with another one.
    bool                operator==(const Rect &rhs) const { return (x != rhs.x || y != rhs.y || w != rhs.w || h != rhs.h) ? false : true; }
                        /// Compare with another one.
    bool                operator!=(const Rect &rhs) const { return (x != rhs.x || y != rhs.y || w != rhs.w || h != rhs.h) ? true : false; }

                        /// Returns if this rectangle is empty.
    bool                IsEmpty() const { return (w <= 0 || h <= 0) ? true : false; }

                        /// Tests whether a point is inside.
    bool                IsContainPoint(const Point &pt) const;
                        /// Tests whether a point is inside.
    bool                IsContainPoint(const int x, const int y) const;
                        /// Tests whether a rect is inside.
    bool                IsContainRect(const Rect &a) const;
                        /// Tests whether a rect is inside.
    bool                IsContainRect(const int x, const int y, const int w, const int h) const;
                        /// Tests whether a rect is intersect.
    bool                IsIntersectRect(const Rect &a) const;
                        /// Tests whether a rect is intersect.
    bool                IsIntersectRect(const int x, const int y, const int w, const int h) const;

    void                Set(int x, int y, int w, int h);
    void                SetPosition(const Point &position);
    void                SetSize(const Size &size);
    void                SetFrom4Coords(int x, int y, int x2, int y2);

    Rect                Add(const Rect &a) const &;
    Rect &&             Add(const Rect &a) && { AddSelf(a); return std::move(*this); }
    Rect &              Add(const Rect &a, const Rect &b);
    Rect &              AddSelf(const Rect &a);

    Rect                AddPoint(const Point &a) const &;
    Rect &&             AddPoint(const Point &a) && { AddPointSelf(a); return std::move(*this); }
    Rect &              AddPoint(const Point &a, const Point &b);
    Rect &              AddPointSelf(const Point &a);

    Rect                Intersect(const Rect &a) const &;
    Rect &&             Intersect(const Rect &a) && { IntersectSelf(a); return std::move(*this); }
    Rect &              Intersect(const Rect &a, const Rect &b);
    Rect &              IntersectSelf(const Rect &a);

    Rect                Move(int x, int y) const &;
    Rect &&             Move(int x, int y) && { MoveSelf(x, y); return std::move(*this); }
    Rect &              MoveSelf(int x, int y);

    Rect                Shrink(int ax, int ay) const &;
    Rect &&             Shrink(int ax, int ay) && { ShrinkSelf(ax, ay); return std::move(*this); }
    Rect &              ShrinkSelf(int ax, int ay);

    Rect                Expand(int ax, int ay) const &;
    Rect &&             Expand(int ax, int ay) && { ExpandSelf(ax, ay); return std::move(*this); }
    Rect &              ExpandSelf(int ax, int ay);

    RectF               ToRectF() const;

                        /// Returns "x y w h".
    const char *        ToString() const;

                        /// Creates from the string.
    static Rect         FromString(const char *str);

                        /// Returns dimension of this type.
    constexpr int       GetDimension() const { return 4; }

#ifdef QRECT_H
    QRect               ToQRect() const { return QRect(x, y, w, h); }
#endif

    static const Rect   zero;

    int                 x;
    int                 y;
    int                 w;
    int                 h;
};

BE_INLINE constexpr Rect::Rect(int inX, int inY, int inW, int inH) :
    x(inX), y(inY), w(inW), h(inH) {
}

BE_INLINE constexpr Rect::Rect(const Point &p) :
    x(p.x), y(p.y), w(1), h(1) {
}

BE_INLINE Point Rect::GetPoint(int index) const {
    switch (index) {
    case 0: return Point(x, y);
    case 1: return Point(x + w, y);
    case 2: return Point(x + w, y + h);
    case 3: return Point(x, y + h);
    }
    return Point::zero;
}

BE_INLINE Rect &Rect::operator=(const Rect &rhs) {
    x = rhs.x;
    y = rhs.y;
    w = rhs.w;
    h = rhs.h;
    return *this;
}

BE_INLINE int Rect::operator[](int index) const {
    assert(index >= 0 && index < 4);
    return ((int *)this)[index];
}

BE_INLINE int &Rect::operator[](int index) {
    assert(index >= 0 && index < 4);
    return ((int *)this)[index];
}

BE_INLINE bool Rect::IsContainPoint(const Point &pt) const {
    return (pt.x < x || pt.y < y || pt.x >= x + w || pt.y >= y + h) ? false : true;
}

BE_INLINE bool Rect::IsContainPoint(const int x, const int y) const {
    return (x < this->x || y < this->y || x >= this->x + this->w || y >= this->y + this->h) ? false : true;
}

BE_INLINE bool Rect::IsContainRect(const Rect &a) const {
    return (a.x < x || a.y < y || a.x + a.w > x + w || a.y + a.h > y + h) ? false : true;
}

BE_INLINE bool Rect::IsContainRect(const int x, const int y, const int w, const int h) const {
    return (x < this->x || y < this->y || x + w > this->x + this->w || y + h > this->y + this->h) ? false : true;
}

BE_INLINE bool Rect::IsIntersectRect(const Rect &a) const {
    return (a.x + a.w < x || a.y + a.h < y || a.x >= x + w || a.y >= y + h) ? false : true;
}

BE_INLINE bool Rect::IsIntersectRect(const int x, const int y, const int w, const int h) const {
    return (x + w < this->x || y + h < this->y || x >= this->x + this->w || y >= this->y + this->h) ? false : true;
}

BE_INLINE void Rect::Set(int x, int y, int w, int h) {
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
}

BE_INLINE void Rect::SetPosition(const Point &position) {
    this->x = position.x;
    this->y = position.y;
}

BE_INLINE void Rect::SetSize(const Size &size) {
    this->w = size.w;
    this->h = size.h;
}

BE_INLINE void Rect::SetFrom4Coords(int x, int y, int x2, int y2) {
    this->x = x;
    this->y = y;
    this->w = x2 - x;
    this->h = y2 - y;
}

BE_INLINE const char *Rect::ToString() const {
    return Str::IntegerArrayToString((const int *)(*this), GetDimension());
}

BE_NAMESPACE_END
