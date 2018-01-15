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

BE_NAMESPACE_BEGIN

/// RGBA color.
class Color4 {
public:
    /// Specifies the number of elements in this class.
    enum { Size = 4 };

    /// The default constructor does not initialize any members of this class.
    Color4() = default;
    /// Constructs a Color4 with the value (r, g, b, a).
    Color4(float r, float g, float b, float a);
    /// Constructs a Color4 with the value (rgb.r, rgb.g, rgb.b, a).
    Color4(const Color3 &rgb, float a);
    /// Constructs a Color4 from a C array, to the value (data[0], data[1], data[2], data[3]).
    explicit Color4(const float data[4]);
    /// Constructs a Color4 from a single value (s, s, s, s)
    explicit Color4(float s);
    
                        /// Casts this Color4 to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const float *       Ptr() const { return (const float *)&r; }
    float *             Ptr() { return (float *)&r; }
                        /// Casts this Color4 to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const float *() const { return (const float *)&r; }
                        operator float *() { return (float *)&r; }

                        /// Accesses an element of this color.
    float &             At(int index) { return (*this)[index]; }
                        /// Accesses an element of this color using array notation.
    float               operator[](int index) const;
    float &             operator[](int index);

                        /// Performs an unary negation of this color.
                        /// This function is identical to the member function Negate().
    Color4              operator-() const { return Color4(-r, -g, -b, -a); }
                        /// Performs an unary negation of this color.
    Color4              Negate() const { return Color4(-r, -g, -b, -a); }
                        /// return Color4(|r|, |g|, |b|, |a|)
    Color4              Abs() const { return Color4(Math::Fabs(r), Math::Fabs(g), Math::Fabs(b), Math::Fabs(a)); }

                        /// Unary operator + allows this structure to be used in an expression '+v'.
    Color4              operator+() const { return *this; }

                        /// Adds a color to this color.
    Color4              Add(const Color4 &v) const { return *this + v; }
                        /// Adds a color to this color.
                        /// This function is identical to the member function Add().
    Color4              operator+(const Color4 &rhs) const { return Color4(r + rhs.r, g + rhs.g, b + rhs.b, a + rhs.a); }
                        /// Adds the color (s, s, s, s) to this color.
    Color4              AddScalar(float s) const { return *this + s; }
                        /// Adds the color (s, s, s, s) to this color.
                        /// This function is identical to the member function AddScalar().
    Color4              operator+(float rhs) const { return Color4(r + rhs, g + rhs, b + rhs, a + rhs); }
                        /// Adds the color v to color (s, s, s, s).
    friend Color4       operator+(float lhs, const Color4 &rhs) { return Color4(lhs + rhs.r, lhs + rhs.g, lhs + rhs.b, lhs + rhs.a); }

                        /// Subtracts a color from this color.
    Color4              Sub(const Color4 &v) const { return *this - v; }
                        /// Subtracts the given color from this color.
                        /// This function is identical to the member function Sub()
    Color4              operator-(const Color4 &rhs) const { return Color4(r - rhs.r, g - rhs.g, b - rhs.b, a - rhs.a); }
                        /// Subtracts the color (s, s, s, s) from this color.
    Color4              SubScalar(float s) const { return *this - s; }
                        /// Subtracts the color (s, s, s, s) from this color.
                        /// This function is identical to the member function SubScalar()
    Color4              operator-(float rhs) const { return Color4(r - rhs, g - rhs, b - rhs, a - rhs); }
                        /// Subtracts the color v from color (s, s, s, s).
    friend Color4       operator-(float lhs, const Color4 &rhs) { return Color4(lhs - rhs.r, lhs - rhs.g, lhs - rhs.b, lhs - rhs.a); }

                        /// Multiplies this color by a scalar.
    Color4              Mul(float s) const { return *this * s; }
                        /// Multiplies this color by a scalar.
                        /// This function is identical to the member function Mul().
    Color4              operator*(float rhs) const { return Color4(r * rhs, g * rhs, b * rhs, a * rhs); }
                        /// Multiplies color v by a scalar.
    friend Color4       operator*(float lhs, const Color4 &rhs) { return Color4(lhs * rhs.r, lhs * rhs.g, lhs * rhs.b, lhs * rhs.a); }
                        /// Multiplies this color by a color, element-wise.
    Color4              MulComp(const Color4 &v) const { return *this * v; }
                        /// Multiplies this color by a color, element-wise.
                        /// This function is identical to the member function MulComp().
    Color4              operator*(const Color4 &rhs) const { return Color4(r * rhs.r, g * rhs.g, b * rhs.b, a * rhs.a); }

                        /// Divides this color by a scalar.
    Color4              Div(float s) const { return *this / s; }
                        /// Divides this color by a scalar.
                        /// This function is identical to the member function Div().
    Color4              operator/(float rhs) const { float inv = 1.f / rhs; return Color4(r * inv, g * inv, b * inv, a * inv); }
                        /// Divides this color by a color, element-wise.
    Color4              DivComp(const Color4 &v) const { return *this / v; }
                        /// This function is identical to the member function DivComp().
                        /// Divides this color by a color, element-wise.
    Color4              operator/(const Color4 &rhs) const { return Color4(r / rhs.r, g / rhs.g, b / rhs.b, a / rhs.a); }
                        /// Divides color (s, s, s, s) by a color, element-wise.
    friend Color4       operator/(float lhs, const Color4 &rhs) { return Color4(lhs / rhs.r, lhs / rhs.g, lhs / rhs.b, lhs / rhs.a); }

                        /// Assign from another color.
    Color4 &            operator=(const Color4 &rhs);

                        /// Adds a color to this color, in-place.
    Color4 &            AddSelf(const Color4 &v) { *this += v; return *this; }
                        /// Adds a color to this color, in-place.
                        /// This function is identical to the member function AddSelf().
    Color4 &            operator+=(const Color4 &rhs);

                        /// Subtracts a color from this color, in-place.
    Color4 &            SubSelf(const Color4 &v) { *this -= v; return *this; }
                        /// Subtracts a color from this color, in-place.
                        /// This function is identical to the member function SubSelf().
    Color4 &            operator-=(const Color4 &rhs);

                        /// Multiplies this color by a scalar, in-place.
    Color4 &            MulSelf(float s) { *this *= s; return *this; }
                        /// Multiplies this color by a scalar, in-place.
                        /// This function is identical to the member function MulSelf().
    Color4 &            operator*=(float rhs);

                        /// Multiplies this color by a color, element-wise, in-place.
    Color4 &            MulCompSelf(const Color4 &v) { *this *= v; return *this; }
                        /// Multiplies this color by a color, element-wise, in-place.
                        /// This function is identical to the member function MulCompSelf().
    Color4 &            operator*=(const Color4 &rhs);

                        /// Divides this color by a scalar, in-place.
    Color4 &            DivSelf(float s) { *this /= s; return *this; }
                        /// Divides this color by a scalar, in-place.
                        /// This function is identical to the member function DivSelf().
    Color4 &            operator/=(float rhs);

                        /// Divides this color by a color, element-wise, in-place.
    Color4 &            DivCompSelf(const Color4 &v) { *this /= v; return *this; }
                        /// Divides this color by a color, element-wise, in-place.
                        /// This function is identical to the member function DivCompSelf().
    Color4 &            operator/=(const Color4 &rhs);

                        /// Exact compare, no epsilon
    bool                Equals(const Color4 &c) const;
                        /// Compare with epsilon
    bool                Equals(const Color4 &c, const float epsilon) const;
                        /// Exact compare, no epsilon
    bool                operator==(const Color4 &rhs) const { return Equals(rhs); }
                        /// Exact compare, no epsilon
    bool                operator!=(const Color4 &rhs) const { return !Equals(rhs); }

                        /// Sets all element of this color
    void                Set(float r, float g, float b, float a);

                        /// Clip to [0, 1] range.
    void                Clip(bool clipAlpha = false);
                        /// Inverts the RGB channels and optionally the alpha channel as well.
    void                Invert(bool invertAlpha = false);

                        /// Returns "r g b a".
    const char *        ToString() const { return ToString(4); }
                        /// Returns "r g b a" with the given precision
    const char *        ToString(int precision) const;

                        /// Creates from the string
    static Color4       FromString(const char *str);

                        /// Casts this Color4 to a Color3.
    const Color3 &      ToColor3() const;
    Color3 &            ToColor3();

                        /// Casts this Color4 to a Vec3.
    const Vec3 &        ToVec3() const;
    Vec3 &              ToVec3();

                        /// Casts this Color4 to a Vec4.
    const Vec4 &        ToVec4() const;
    Vec4 &              ToVec4();

                        /// Convert color packed to a 32-bit integer, with R component in the lowest 8 bits. Components are clamped to [0, 1] range.
    uint32_t            ToUInt32() const;

                        /// Returns dimension of this type
    int                 GetDimension() const { return Size; }

    static const Color4 zero;       ///< (0.0, 0.0, 0.0, 0.0)
    static const Color4 black;      ///< (0.0, 0.0, 0.0, 1.0)
    static const Color4 white;      ///< (1.0, 1.0, 1.0, 1.0)
    static const Color4 red;        ///< (1.0, 0.0, 0.0, 1.0)
    static const Color4 green;      ///< (0.0, 1.0, 0.0, 1.0)
    static const Color4 blue;       ///< (0.0, 0.0, 1.0, 1.0)
    static const Color4 yellow;     ///< (1.0, 1.0, 0.0, 1.0)
    static const Color4 cyan;       ///< (0.0, 1.0, 1.0, 1.0)
    static const Color4 magenta;    ///< (1.0, 0.0, 1.0, 1.0)
    static const Color4 orange;     ///< (1.0, 0.5, 0.0, 1.0)
    static const Color4 pink;       ///< (1.0, 0.0, 0.5, 1.0)
    static const Color4 lawn;       ///< (0.5, 1.0, 0.0, 1.0)
    static const Color4 mint;       ///< (0.0, 1.0, 0.5, 1.0)
    static const Color4 violet;     ///< (0.5, 0.5, 1.0, 1.0)
    static const Color4 teal;       ///< (0.3, 0.5, 0.6, 1.0)
    static const Color4 grey;       ///< (0.7, 0.7, 0.7, 1.0)
    static const Color4 darkGrey;   ///< (0.3, 0.3, 0.3, 1.0)

    float               r;          ///< The red component.
    float               g;          ///< The green component.
    float               b;          ///< The blue component.
    float               a;          ///< The alpha component.
};

BE_INLINE Color4::Color4(float r, float g, float b, float a) {
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;
}

BE_INLINE Color4::Color4(const Color3 &rgb, float a) {
    this->r = rgb.r;
    this->g = rgb.g;
    this->b = rgb.b;
    this->a = a;
}

BE_INLINE Color4::Color4(const float data[4]) {
    this->r = data[0];
    this->g = data[1];
    this->b = data[2];
    this->a = data[3];
}

BE_INLINE Color4::Color4(float c) {
    this->r = c;
    this->g = c;
    this->b = c;
    this->a = c;
}

BE_INLINE void Color4::Set(float r, float g, float b, float a) {
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;
}

BE_INLINE float Color4::operator[](int index) const {
    assert(index >= 0 && index < Size);
    return ((float *)this)[index];
}

BE_INLINE float &Color4::operator[](int index) {
    assert(index >= 0 && index < Size);
    return ((float *)this)[index];
}

BE_INLINE Color4 &Color4::operator=(const Color4 &rhs) {
    r = rhs.r;
    g = rhs.g;
    b = rhs.b;
    a = rhs.a;
    return *this;
}

BE_INLINE Color4 &Color4::operator+=(const Color4 &rhs) {
    r += rhs.r;
    g += rhs.g;
    b += rhs.b;
    a += rhs.a;
    return *this;
}

BE_INLINE Color4 &Color4::operator-=(const Color4 &rhs) {
    r -= rhs.r;
    g -= rhs.g;
    b -= rhs.b;
    a -= rhs.a;
    return *this;
}

BE_INLINE Color4 &Color4::operator*=(float rhs) {
    r *= rhs;
    g *= rhs;
    b *= rhs;
    a *= rhs;
    return *this;
}

BE_INLINE Color4 &Color4::operator*=(const Color4 &rhs) {
    r *= rhs.r;
    g *= rhs.g;
    b *= rhs.b;
    a *= rhs.a;
    return *this;
};

BE_INLINE Color4 &Color4::operator/=(float rhs) {
    float inv = 1.0f / rhs;
    r *= inv;
    g *= inv;
    b *= inv;
    a *= inv;
    return *this;
}

BE_INLINE Color4 &Color4::operator/=(const Color4 &rhs) {
    r /= rhs.r;
    g /= rhs.g;
    b /= rhs.b;
    a /= rhs.a;
    return *this;
};

BE_INLINE bool Color4::Equals(const Color4 &c) const {
    return ((r == c.r) && (g == c.g) && (b == c.b) && (a == c.a));
}

BE_INLINE bool Color4::Equals(const Color4 &c, const float epsilon) const {
    if (Math::Fabs(r - c.r) > epsilon) {
        return false;
    }
    if (Math::Fabs(g - c.g) > epsilon) {
        return false;
    }
    if (Math::Fabs(b - c.b) > epsilon) {
        return false;
    }
    if (Math::Fabs(a - c.a) > epsilon) {
        return false;
    }
    return true;
}

BE_INLINE const char *Color4::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), Size, precision);
}

BE_INLINE const Color3 &Color4::ToColor3() const {
    return *reinterpret_cast<const Color3 *>(this);
}

BE_INLINE Color3 &Color4::ToColor3() {
    return *reinterpret_cast<Color3 *>(this);
}

BE_INLINE const Vec3 &Color4::ToVec3() const {
    return *reinterpret_cast<const Vec3 *>(this);
}

BE_INLINE Vec3 &Color4::ToVec3() {
    return *reinterpret_cast<Vec3 *>(this);
}

BE_INLINE const Vec4 &Color4::ToVec4() const {
    return *reinterpret_cast<const Vec4 *>(this);
}

BE_INLINE Vec4 &Color4::ToVec4() {
    return *reinterpret_cast<Vec4 *>(this);
}

BE_NAMESPACE_END