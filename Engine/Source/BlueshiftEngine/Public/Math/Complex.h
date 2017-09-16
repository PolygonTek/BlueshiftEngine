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

    Complex number

-------------------------------------------------------------------------------
*/

#include "Core/Str.h"

BE_NAMESPACE_BEGIN

class BE_API Complex {
public:
    /// The default constructor does not initialize any members of this class.
    Complex() {}
    /// Set complex number with the given real part and imagenary part.
    Complex(const float re, const float im);
    /// Assginment operator
    Complex &operator=(const Complex &rhs);

    void                    Set(const float re, const float im);
    void                    SetZero();

                            /// Casts this Complex to a C array.
                            operator const float *() const;
                            operator float *();

                            /// Accesses an element of this complex value using array notation.
    float                   operator[](int index) const;
    float &                 operator[](int index);

    Complex &               operator*=(const Complex &rhs);
    Complex &               operator*=(const float rhs);
    Complex &               operator/=(const Complex &rhs);
    Complex &               operator/=(const float rhs);
    Complex &               operator+=(const Complex &rhs);
    Complex &               operator+=(const float rhs);
    Complex &               operator-=(const Complex &rhs);
    Complex &               operator-=(const float rhs);

    Complex                 operator+() const;
    Complex                 operator-() const;

    Complex                 operator*(const Complex &rhs) const;
    Complex                 operator*(const float rhs) const;
    friend Complex          operator*(const float lhs, const Complex &rhs);
    Complex                 operator/(const Complex &rhs) const;
    Complex                 operator/(const float rhs) const;
    friend Complex          operator/(const float lhs, const Complex &rhs);
    Complex                 operator+(const Complex &rhs) const;
    Complex                 operator+(const float rhs) const;
    friend Complex          operator+(const float lhs, const Complex &rhs);
    Complex                 operator-(const Complex &rhs) const;
    Complex                 operator-(const float rhs) const;
    friend Complex          operator-(const float lhs, const Complex &rhs);

                            /// Exact compare, no epsilon
    bool                    Equals(const Complex &a) const;
                            /// Compare with epsilon
    bool                    Equals(const Complex &a, const float epsilon) const;
                            /// Exact compare, no epsilon
    bool                    operator==(const Complex &a) const { return Equals(a); }
                            /// Exact compare, no epsilon
    bool                    operator!=(const Complex &a) const { return !Equals(a); }

                            /// Returns the complex conjugate
    Complex                 Conjugate() const;

    Complex                 Reciprocal() const;

    Complex                 Sqrt() const;
    
                            /// Returns the magnitude of a complex number
    float                   Abs() const;

                            /// Returns "re im".
    const char *            ToString() const { return ToString(4); }
                            /// Returns "re im" with the given precision.
    const char *            ToString(int precision) const;

                            /// Returns dimension of this type
    int                     GetDimension() const { return 2; }

    static const Complex    origin;
    static const Complex    zero;

    float                   re;     ///< real part
    float                   im;     ///< imaginary part
};

BE_INLINE Complex::Complex(const float re, const float im) {
    this->re = re;
    this->im = im;
}

BE_INLINE Complex &Complex::operator=(const Complex &rhs) {
    re = rhs.re;
    im = rhs.im;
    return *this;
}

BE_INLINE void Complex::Set(const float re, const float im) {
    this->re = re;
    this->im = im;
}

BE_INLINE void Complex::SetZero() {
    re = im = 0.0f;
}

BE_INLINE Complex::operator const float *() const {
    return &re;
}

BE_INLINE Complex::operator float *() {
    return &re;
}

BE_INLINE float Complex::operator[](int index) const {
    assert(index >= 0 && index < 2);
    return (&re)[index];
}

BE_INLINE float& Complex::operator[](int index) {
    assert(index >= 0 && index < 2);
    return (&re)[index];
}

BE_INLINE Complex &Complex::operator*=(const Complex &rhs) {
    *this = Complex(re * rhs.re - im * rhs.im, im * rhs.re + re * rhs.im);
    return *this;
}

BE_INLINE Complex &Complex::operator/=(const Complex &rhs) {
    float s, t;
    if (Math::Fabs(rhs.re) >= Math::Fabs(rhs.im)) {
        s = rhs.im / rhs.re;
        t = 1.0f / (rhs.re + s * rhs.im);
        *this = Complex((re + s * im) * t, (im - s * re) * t);
    } else {
        s = rhs.re / rhs.im;
        t = 1.0f / (s * rhs.re + rhs.im);
        *this = Complex((re * s + im) * t, (im * s - re) * t);
    }
    return *this;
}

BE_INLINE Complex &Complex::operator+=(const Complex &rhs) {
    re += rhs.re;
    im += rhs.im;
    return *this;
}

BE_INLINE Complex &Complex::operator-=(const Complex &rhs) {
    re -= rhs.re;
    im -= rhs.im;
    return *this;
}

BE_INLINE Complex &Complex::operator*=(const float rhs) {
    re *= rhs;
    im *= rhs;
    return *this;
}

BE_INLINE Complex &Complex::operator/=(const float rhs) {
    float s = 1.0f / rhs;
    re *= s;
    im *= s;
    return *this;
}

BE_INLINE Complex &Complex::operator+=(const float rhs) {
    re += rhs;
    return *this;
}

BE_INLINE Complex &Complex::operator-=(const float rhs) {
    re -= rhs;
    return *this;
}

BE_INLINE Complex Complex::operator+() const {
    return Complex(re, im);
}

BE_INLINE Complex Complex::operator-() const {
    return Complex(-re, -im);
}

BE_INLINE Complex Complex::operator*(const Complex &rhs) const {
    return Complex(re * rhs.re - im * rhs.im, im * rhs.re + re * rhs.im);
}

BE_INLINE Complex Complex::operator/(const Complex &rhs) const {
    float s, t;
    if (Math::Fabs(rhs.re) >= Math::Fabs(rhs.im)) {
        s = rhs.im / rhs.re;
        t = 1.0f / (rhs.re + s * rhs.im);
        return Complex((re + s * im) * t, (im - s * re) * t);
    } else {
        s = rhs.re / rhs.im;
        t = 1.0f / (s * rhs.re + rhs.im);
        return Complex((re * s + im) * t, (im * s - re) * t);
    }
}

BE_INLINE Complex Complex::operator+(const Complex &rhs) const {
    return Complex(re + rhs.re, im + rhs.im);
}

BE_INLINE Complex Complex::operator-(const Complex &rhs) const {
    return Complex(re - rhs.re, im - rhs.im);
}

BE_INLINE Complex Complex::operator*(const float rhs) const {
    return Complex(re * rhs, im * rhs);
}

BE_INLINE Complex Complex::operator/(const float rhs) const {
    float s = 1.0f / rhs;
    return Complex(re * s, im * s);
}

BE_INLINE Complex Complex::operator+(const float rhs) const {
    return Complex(re + rhs, im);
}

BE_INLINE Complex Complex::operator-(const float rhs) const {
    return Complex(re - rhs, im);
}

BE_INLINE Complex operator*(const float lhs, const Complex &rhs) {
    return Complex(lhs * rhs.re, lhs * rhs.im);
}

BE_INLINE Complex operator/(const float lhs, const Complex &rhs) {
    float s, t;
    if (Math::Fabs(rhs.re) >= Math::Fabs(rhs.im)) {
        s = rhs.im / rhs.re;
        t = lhs / (rhs.re + s * rhs.im);
        return Complex(t, - s * t);
    } else {
        s = rhs.re / rhs.im;
        t = lhs / (s * rhs.re + rhs.im);
        return Complex(s * t, - t);
    }
}

BE_INLINE Complex operator+(const float lhs, const Complex &rhs) {
    return Complex(lhs + rhs.re, rhs.im);
}

BE_INLINE Complex operator-(const float lhs, const Complex &rhs) {
    return Complex(lhs - rhs.re, -rhs.im);
}

BE_INLINE Complex Complex::Conjugate() const {
    return Complex(re, -im);
}

BE_INLINE Complex Complex::Reciprocal() const {
    float s, t;
    if (Math::Fabs(re) >= Math::Fabs(im)) {
        s = im / re;
        t = 1.0f / (re + s * im);
        return Complex(t, - s * t);
    } else {
        s = re / im;
        t = 1.0f / (s * re + im);
        return Complex(s * t, - t);
    }
}

BE_INLINE Complex Complex::Sqrt() const {
    if (re == 0.0f && im == 0.0f) {
        return Complex(0.0f, 0.0f);
    }
    
    float w;
    float x = Math::Fabs(re);
    float y = Math::Fabs(im);
    
    if (x >= y) {
        w = y / x;
        w = Math::Sqrt(x) * Math::Sqrt(0.5f * (1.0f + Math::Sqrt(1.0f + w * w)));
    } else {
        w = x / y;
        w = Math::Sqrt(y) * Math::Sqrt(0.5f * (w + Math::Sqrt(1.0f + w * w)));
    }

    if (w == 0.0f) {
        return Complex(0.0f, 0.0f);
    }

    if (re >= 0.0f) {
        return Complex(w, 0.5f * im / w);
    } else {
        return Complex(0.5f * y / w, (im >= 0.0f) ? w : -w);
    }
}

BE_INLINE float Complex::Abs() const {
    float t;
    float x = Math::Fabs(re);
    float y = Math::Fabs(im);
    
    if (x == 0.0f) {
        return y;
    } else if (y == 0.0f) {
        return x;
    } else if (x > y) {
        t = y / x;
        return x * Math::Sqrt(1.0f + t * t);
    } else {
        t = x / y;
        return y * Math::Sqrt(1.0f + t * t);
    }
}

BE_INLINE bool Complex::Equals(const Complex &a) const {
    return ((re == a.re) && (im == a.im));
}

BE_INLINE bool Complex::Equals(const Complex &a, const float epsilon) const {
    if (Math::Fabs(re - a.re) > epsilon) {
        return false;
    }
    if (Math::Fabs(im - a.im) > epsilon) {
        return false;
    }
    return true;
}

BE_INLINE const char *Complex::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), 2, precision);
}

BE_NAMESPACE_END
