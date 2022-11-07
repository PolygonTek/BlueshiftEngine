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

    Size

-------------------------------------------------------------------------------
*/

#include "Core/Str.h"

BE_NAMESPACE_BEGIN

class SizeF;

/// A 2D integral (w, h) - size.
class BE_API Size {
public:
    /// The default constructor does not initialize any members of this class.
    Size() = default;
    /// Constructs a Size with the value (w, h).
    constexpr Size(int w, int h);
    /// Assignment operator
    Size &operator=(const Size &rhs);

#ifdef QSIZE_H
    /// Constructs from a QSize.
    Size(const QSize &qsize) {
        this->w = qsize.width();
        this->h = qsize.height();
    }
#endif

    /// Constructs a Size from a C array, to the value (data[0], data[1]).
    explicit constexpr Size(int data[2]);
    /// Copy constructor.
    explicit Size(const Vec2 &v);

                        /// Casts this Size to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const int *         Ptr() const { return (const int *)&w; }
    int *               Ptr() { return (int *)&w; }
                        /// Casts this Size to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const int *() const { return (const int *)&w; }
                        operator int *() { return (int *)&w; }

                        /// Accesses an element of this Size using array notation.
    int                 operator[](int index) const;
    int &               operator[](int index);

                        /// Unary operator + allows this structure to be used in an expression '+p'.
    Size                operator+() const { return *this; }

                        /// Performs an unary negation of this Size.
    Size                operator-() const & { return Size(-w, -h); }
    Size &&             operator-() && { w = -w; h = -h; return std::move(*this); }

                        /// Adds a size to this size.
    Size                operator+(const Size &rhs) const & { return Size(w + rhs.w, h + rhs.h); }
    Size &&             operator+(const Size &rhs) && { *this += rhs; return std::move(*this); }

                        /// Subtracts a size to this size.
    Size                operator-(const Size &rhs) const & { return Size(w - rhs.w, h - rhs.h); }
    Size &&             operator-(const Size &rhs) && { *this -= rhs; return std::move(*this); }

                        /// Multiplies this size by a scalar.
    Size                operator*(int rhs) const & { return Size(w * rhs, h * rhs); }
    Size &&             operator*(int rhs) && { *this *= rhs; return std::move(*this); }

                        /// Adds a size to this size, in-place.
    Size &              operator+=(const Size &rhs);

                        /// Subtracts a size to this size, in-place.
    Size &              operator-=(const Size &rhs);

                        /// Multiplies this size by a scalar, in-place.
    Size &              operator*=(int rhs);

                        /// Compare with another one.
    bool                operator==(const Size &rhs) const { return (w != rhs.w || h != rhs.h) ? false : true; }
                        /// Compare with another one.
    bool                operator!=(const Size &rhs) const { return (w != rhs.w || h != rhs.h) ? true : false; }

    bool                IsEmpty() const { return (w <= 0 || h <= 0) ? true : false; }

    void                Set(int x, int y);

    SizeF               ToSizeF() const;

    Vec2                ToVec2() const { return Vec2(w, h); }

#ifdef QSIZE_H
    QSize               ToQSize() const { return QSize(w, h); }
#endif

                        /// Returns "w h".
    const char *        ToString() const;

                        /// Creates from the string.
    static Size         FromString(const char *str);

                        /// Returns dimension of this type.
    constexpr int       GetDimension() const { return 2; }

    static const Size   zero;

    int                 w;
    int                 h;
};

BE_INLINE constexpr Size::Size(int inW, int inH) :
    w(inW), h(inH) {
}

BE_INLINE constexpr Size::Size(int data[2]) :
    w(data[0]), h(data[1]) {
}

BE_INLINE Size::Size(const Vec2 &v) {
    w = (int)v.x;
    h = (int)v.y;
}

BE_INLINE Size &Size::operator=(const Size &rhs) {
    w = rhs.w;
    h = rhs.h;
    return *this;
}

BE_INLINE int Size::operator[](int index) const {
    assert(index >= 0 && index < 2);
    return ((int *)this)[index];
}

BE_INLINE int &Size::operator[](int index) {
    assert(index >= 0 && index < 2);
    return ((int *)this)[index];
}

BE_INLINE Size &Size::operator+=(const Size &rhs) {
    w += rhs.w;
    h += rhs.h;
    return *this;
}

BE_INLINE Size &Size::operator-=(const Size &rhs) {
    w -= rhs.w;
    h -= rhs.h;
    return *this;
}

BE_INLINE Size &Size::operator*=(int rhs) {
    w *= rhs;
    h *= rhs;
    return *this;
}

BE_INLINE void Size::Set(int w, int h) {
    this->w = w;
    this->h = h;
}

BE_INLINE const char *Size::ToString() const {
    return Str::IntegerArrayToString((const int *)(*this), GetDimension());
}

BE_NAMESPACE_END
