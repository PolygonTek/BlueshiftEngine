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

    Size

-------------------------------------------------------------------------------
*/

#include "Core/Str.h"
#include "Math/Vector2.h"
#include <cassert>
#include <cstdio>
#include <type_traits>
#include <utility>

BE_NAMESPACE_BEGIN

#ifdef QSIZE_H
template <typename T>
struct SizeQtType;

template <>
struct SizeQtType<int> {
    using Type = QSize;
};

template <>
struct SizeQtType<float> {
    using Type = QSizeF;
};
#endif

/// A 2D (w, h) size with the given scalar type.
template <typename T>
class SizeT {
    static_assert(std::is_same<T, int>::value || std::is_same<T, float>::value, "SizeT supports only int and float scalar types");

public:
    using Scalar = T;

    /// The default constructor does not initialize any members of this class.
    SizeT() = default;
    /// Constructs a SizeT with the value (w, h).
    constexpr SizeT(T w, T h);
    /// Assignment operator.
    SizeT &operator=(const SizeT &rhs) = default;

#ifdef QSIZE_H
    using QtType = typename SizeQtType<T>::Type;

    /// Constructs from the matching Qt size type.
    SizeT(const QtType &qsize) {
        this->w = static_cast<T>(qsize.width());
        this->h = static_cast<T>(qsize.height());
    }
#endif

    /// Constructs a SizeT from a C array, to the value (data[0], data[1]).
    explicit constexpr SizeT(T data[2]);
    /// Constructs from a Vec2.
    explicit SizeT(const Vec2 &v);

                        /// Casts this SizeT to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const T *           Ptr() const { return (const T *)&w; }
    T *                 Ptr() { return (T *)&w; }
                        /// Casts this SizeT to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const T *() const { return (const T *)&w; }
                        operator T *() { return (T *)&w; }

                        /// Accesses an element of this SizeT using array notation.
    T                   operator[](int index) const;
    T &                 operator[](int index);

                        /// Unary operator + allows this structure to be used in an expression '+p'.
    SizeT               operator+() const { return *this; }

                        /// Performs an unary negation of this SizeT.
    SizeT               operator-() const & { return SizeT(-w, -h); }
    SizeT &&            operator-() && { w = -w; h = -h; return std::move(*this); }

                        /// Adds a size to this size.
    SizeT               operator+(const SizeT &rhs) const & { return SizeT(w + rhs.w, h + rhs.h); }
    SizeT &&            operator+(const SizeT &rhs) && { *this += rhs; return std::move(*this); }

                        /// Subtracts a size from this size.
    SizeT               operator-(const SizeT &rhs) const & { return SizeT(w - rhs.w, h - rhs.h); }
    SizeT &&            operator-(const SizeT &rhs) && { *this -= rhs; return std::move(*this); }

                        /// Multiplies this size by a scalar.
    SizeT               operator*(T rhs) const & { return SizeT(w * rhs, h * rhs); }
    SizeT &&            operator*(T rhs) && { *this *= rhs; return std::move(*this); }

                        /// Adds a size to this size, in-place.
    SizeT &             operator+=(const SizeT &rhs);

                        /// Subtracts a size from this size, in-place.
    SizeT &             operator-=(const SizeT &rhs);

                        /// Multiplies this size by a scalar, in-place.
    SizeT &             operator*=(T rhs);

                        /// Compare with another one.
    bool                operator==(const SizeT &rhs) const { return (w != rhs.w || h != rhs.h) ? false : true; }
                        /// Compare with another one.
    bool                operator!=(const SizeT &rhs) const { return (w != rhs.w || h != rhs.h) ? true : false; }

    bool                IsEmpty() const { return (w <= 0 || h <= 0) ? true : false; }

    void                Set(T w, T h);

    SizeT<int>          ToSize() const;
    SizeT<float>        ToSizeF() const;

    Vec2                ToVec2() const { return Vec2(static_cast<float>(w), static_cast<float>(h)); }

#ifdef QSIZE_H
    QSize               ToQSize() const { return QSize(static_cast<int>(w), static_cast<int>(h)); }
    QSizeF              ToQSizeF() const { return QSizeF(static_cast<qreal>(w), static_cast<qreal>(h)); }
#endif

                        /// Returns "w h".
    const char *        ToString() const;
                        /// Returns "w h" with given precision for floating point sizes.
    const char *        ToString(int precision) const;

                        /// Creates from the string.
    static SizeT        FromString(const char *str);

                        /// Returns dimension of this type.
    constexpr int       GetDimension() const { return 2; }

    static const SizeT  zero;

    T                   w;
    T                   h;
};

template <typename T>
BE_INLINE constexpr SizeT<T>::SizeT(T inW, T inH) :
    w(inW), h(inH) {
}

template <typename T>
BE_INLINE constexpr SizeT<T>::SizeT(T data[2]) :
    w(data[0]), h(data[1]) {
}

template <typename T>
BE_INLINE SizeT<T>::SizeT(const Vec2 &v) :
    w(static_cast<T>(v.x)), h(static_cast<T>(v.y)) {
}

template <typename T>
BE_INLINE T SizeT<T>::operator[](int index) const {
    assert(index >= 0 && index < 2);
    return ((const T *)this)[index];
}

template <typename T>
BE_INLINE T &SizeT<T>::operator[](int index) {
    assert(index >= 0 && index < 2);
    return ((T *)this)[index];
}

template <typename T>
BE_INLINE SizeT<T> &SizeT<T>::operator+=(const SizeT &rhs) {
    w += rhs.w;
    h += rhs.h;
    return *this;
}

template <typename T>
BE_INLINE SizeT<T> &SizeT<T>::operator-=(const SizeT &rhs) {
    w -= rhs.w;
    h -= rhs.h;
    return *this;
}

template <typename T>
BE_INLINE SizeT<T> &SizeT<T>::operator*=(T rhs) {
    w *= rhs;
    h *= rhs;
    return *this;
}

template <typename T>
BE_INLINE void SizeT<T>::Set(T w, T h) {
    this->w = w;
    this->h = h;
}

template <typename T>
BE_INLINE SizeT<int> SizeT<T>::ToSize() const {
    return SizeT<int>(static_cast<int>(w), static_cast<int>(h));
}

template <typename T>
BE_INLINE SizeT<float> SizeT<T>::ToSizeF() const {
    return SizeT<float>(static_cast<float>(w), static_cast<float>(h));
}

template <typename T>
BE_INLINE const char *SizeT<T>::ToString() const {
    if constexpr (std::is_same<T, int>::value) {
        return Str::IntegerArrayToString(Ptr(), GetDimension());
    } else {
        return ToString(4);
    }
}

template <typename T>
BE_INLINE const char *SizeT<T>::ToString(int precision) const {
    if constexpr (std::is_same<T, int>::value) {
        return Str::IntegerArrayToString(Ptr(), GetDimension());
    } else {
        return Str::FloatArrayToString(Ptr(), GetDimension(), precision);
    }
}

template <typename T>
BE_INLINE SizeT<T> SizeT<T>::FromString(const char *str) {
    SizeT s;
    int count;

    if constexpr (std::is_same<T, int>::value) {
        count = std::sscanf(str, "%i %i", &s.w, &s.h);
    } else {
        count = std::sscanf(str, "%f %f", &s.w, &s.h);
    }

    assert(count == s.GetDimension());
    return s;
}

template <typename T>
inline const SizeT<T> SizeT<T>::zero = SizeT<T>(T(0), T(0));

using Size = SizeT<int>;
using SizeF = SizeT<float>;

static_assert(std::is_standard_layout<Size>::value, "Size must remain standard-layout");
static_assert(std::is_standard_layout<SizeF>::value, "SizeF must remain standard-layout");
static_assert(sizeof(Size) == sizeof(int) * 2, "Size layout changed");
static_assert(sizeof(SizeF) == sizeof(float) * 2, "SizeF layout changed");

BE_NAMESPACE_END
