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

    SizeF

-------------------------------------------------------------------------------
*/

#include "Core/Str.h"

BE_NAMESPACE_BEGIN

class Size;

/// A 2D floating point precision (w, h) - size.
class BE_API SizeF {
public:
    /// The default constructor does not initialize any members of this class.
    SizeF() = default;
    /// Constructs a SizeF with the value (w, h).
    constexpr SizeF(float w, float h);
    /// Assignment operator
    SizeF &operator=(const SizeF &rhs);

#ifdef QSIZE_H
    /// Constructs from a QSize.
    SizeF(const QSizeF &qsize) {
        this->w = qsize.width();
        this->h = qsize.height();
    }
#endif

    /// Constructs a SizeF from a C array, to the value (data[0], data[1]).
    explicit constexpr SizeF(float data[2]);
    /// Copy constructor
    explicit SizeF(const Vec2 &v);

                        /// Casts this SizeF to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const float *       Ptr() const { return (const float *)&w; }
    float *             Ptr() { return (float *)&w; }
                        /// Casts this SizeF to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const float *() const { return (const float *)&w; }
                        operator float *() { return (float *)&w; }

                        /// Accesses an element of this SizeF using array notation.
    float               operator[](int index) const;
    float &             operator[](int index);

                        /// Unary operator + allows this structure to be used in an expression '+p'.
    SizeF               operator+() const { return *this; }

                        /// Performs an unary negation of this SizeF.
    SizeF               operator-() const & { return SizeF(-w, -h); }
    SizeF &&            operator-() && { w = -w; h = -h; return std::move(*this); }

                        /// Adds a size to this size.
    SizeF               operator+(const SizeF &rhs) const & { return SizeF(w + rhs.w, h + rhs.h); }
    SizeF &&            operator+(const SizeF &rhs) && { *this += rhs; return std::move(*this); }

                        /// Subtracts a size to this size.
    SizeF               operator-(const SizeF &rhs) const & { return SizeF(w - rhs.w, h - rhs.h); }
    SizeF &&            operator-(const SizeF &rhs) && { *this -= rhs; return std::move(*this); }

                        /// Multiplies this size by a scalar.
    SizeF               operator*(float rhs) const & { return SizeF(w * rhs, h * rhs); }
    SizeF &&            operator*(float rhs) && { *this *= rhs; return std::move(*this); }

                        /// Adds a size to this size, in-place.
    SizeF &             operator+=(const SizeF &rhs);

                        /// Subtracts a size to this size, in-place.
    SizeF &             operator-=(const SizeF &rhs);

                        /// Multiplies this size by a scalar, in-place.
    SizeF &             operator*=(float rhs);

                        /// Compare with another one.
    bool                operator==(const SizeF &rhs) const { return (w != rhs.w || h != rhs.h) ? false : true; }
                        /// Compare with another one.
    bool                operator!=(const SizeF &rhs) const { return (w != rhs.w || h != rhs.h) ? true : false; }

    bool                IsEmpty() const { return (w <= 0 || h <= 0) ? true : false; }

    void                Set(float x, float y);

    Size                ToSize() const { return Size(w, h); }

    Vec2                ToVec2() const { return Vec2(w, h); }

#ifdef QSIZE_H
    QSizeF              ToQSizeF() const { return QSizeF(w, h); }
#endif

                        /// Returns "w h".
    const char *        ToString() const { return ToString(4); }
                        /// Returns "w h" with given precision.
    const char *        ToString(int precision) const;

                        /// Creates from the string.
    static SizeF        FromString(const char *str);

                        /// Returns dimension of this type.
    constexpr int       GetDimension() const { return 2; }

    static const SizeF  zero;

    float               w;
    float               h;
};

BE_INLINE constexpr SizeF::SizeF(float inW, float inH) :
    w(inW), h(inH) {
}

BE_INLINE constexpr SizeF::SizeF(float data[2]) :
    w(data[0]), h(data[1]) {
}

BE_INLINE SizeF::SizeF(const Vec2 &v) {
    w = v.x;
    h = v.y;
}

BE_INLINE SizeF &SizeF::operator=(const SizeF &rhs) {
    w = rhs.w;
    h = rhs.h;
    return *this;
}

BE_INLINE float SizeF::operator[](int index) const {
    assert(index >= 0 && index < 2);
    return ((float *)this)[index];
}

BE_INLINE float &SizeF::operator[](int index) {
    assert(index >= 0 && index < 2);
    return ((float *)this)[index];
}

BE_INLINE SizeF &SizeF::operator+=(const SizeF &rhs) {
    w += rhs.w;
    h += rhs.h;
    return *this;
}

BE_INLINE SizeF &SizeF::operator-=(const SizeF &rhs) {
    w -= rhs.w;
    h -= rhs.h;
    return *this;
}

BE_INLINE SizeF &SizeF::operator*=(float rhs) {
    w *= rhs;
    h *= rhs;
    return *this;
}

BE_INLINE void SizeF::Set(float w, float h) {
    this->w = w;
    this->h = h;
}

BE_INLINE const char *SizeF::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), GetDimension(), precision);
}

BE_NAMESPACE_END
