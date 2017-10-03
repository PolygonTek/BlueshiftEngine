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

/// RGB color.
class Color3 {
public:
    /// Specifies the number of elements in this class.
    enum { Size = 3 };

    /// The default constructor does not initialize any members of this class.
    Color3() {}
    /// Constructs a Color3 with the value (r, g, b).
    Color3(float r, float g, float b);
    /// Constructs a Color3 from a C array, to the value (data[0], data[1], data[2]).
    explicit Color3(const float data[3]);
    /// Constructs a Color3 from a single value (s, s, s)
    explicit Color3(float s);    

                        /// Casts this Color3 to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const float *       Ptr() const { return (const float *)&r; }
    float *             Ptr() { return (float *)&r; }
                        /// Casts this Color3 to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const float *() const { return (const float *)&r; }
                        operator float *() { return (float *)&r; }

                        /// Accesses an element of this color.
    float &             At(int index) { return (*this)[index]; }
                        /// Accesses an element of this color using array notation.
    float               operator[](int index) const;
    float &             operator[](int index);

                        /// Multiplies this color by a scalar.
    Color3              Mul(float s) const { return *this * s; }
                        /// Multiplies this color by a scalar.
                        /// This function is identical to the member function Mul().
    Color3              operator*(float rhs) const { return Color3(r * rhs, g * rhs, b * rhs); }
                        /// Multiplies color v by a scalar.
    friend Color3       operator*(float lhs, const Color3 &rhs) { return Color3(lhs * rhs.r, lhs * rhs.g, lhs * rhs.b); }

                        /// Divides this color by a scalar.
    Color3              Div(float s) const { return *this / s; }
                        /// Divides this color by a scalar.
                        /// This function is identical to the member function Div().
    Color3              operator/(float rhs) const { float inv = 1.f / rhs; return Color3(r * inv, g * inv, b * inv); }

                        /// Multiplies this color by a scalar, in-place.
    Color3 &            MulSelf(float s) { *this *= s; return *this; }
                        /// Multiplies this color by a scalar, in-place.
                        /// This function is identical to the member function MulSelf().
    Color3 &            operator*=(float rhs);

                        /// Divides this color by a scalar, in-place.
    Color3 &            DivSelf(float s) { *this /= s; return *this; }
                        /// Divides this color by a scalar, in-place.
                        /// This function is identical to the member function DivSelf().
    Color3 &            operator/=(float rhs);

                        /// Exact compare, no epsilon
    bool                Equals(const Color3 &c) const;
                        /// Compare with epsilon
    bool                Equals(const Color3 &c, const float epsilon) const;
                        /// Exact compare, no epsilon
    bool                operator==(const Color3 &rhs) const { return Equals(rhs); }
                        /// Exact compare, no epsilon
    bool                operator!=(const Color3 &rhs) const { return !Equals(rhs); }

                        /// Sets all element of this color
    void                Set(float r, float g, float b);

                        /// Clip to [0, 1] range.
    void                Clip();
                        /// Inverts the RGB channels.
    void                Invert();

                        /// Returns "r g b".
    const char *        ToString() const { return ToString(4); }
                        /// Returns "r g b" with the given precision
    const char *        ToString(int precision) const;

                        /// Casts this Color3 to a Vec3.
    const Vec3 &        ToVec3() const;
    Vec3 &              ToVec3();

                        /// Convert color to a 32-bit integer, with R component in the lowest 8 bits. Components are clamped to [0, 1] range.
    uint32_t            ToUInt32() const;

                        /// Convert RGB to HSL
    Color3              ToHSL() const;
                        /// Convert HSL to RGB
    Color3              FromHSL() const;

                        /// Convert sRGB to Linear
    Color3              SRGBtoLinear() const;
                        /// Convert Linear to sRGB
    Color3              LinearToSRGB() const;

                        /// Returns dimension of this type
    int                 GetDimension() const { return Size; }

    static const Color3 zero;       ///< (0.0, 0.0, 0.0)
    static const Color3 black;      ///< (0.0, 0.0, 0.0)
    static const Color3 white;      ///< (1.0, 1.0, 1.0)
    static const Color3 red;        ///< (1.0, 0.0, 0.0)
    static const Color3 green;      ///< (0.0, 1.0, 0.0)
    static const Color3 blue;       ///< (0.0, 0.0, 1.0)
    static const Color3 yellow;     ///< (1.0, 1.0, 0.0)
    static const Color3 cyan;       ///< (0.0, 1.0, 1.0)
    static const Color3 magenta;    ///< (1.0, 0.0, 1.0)
    static const Color3 orange;     ///< (1.0, 0.5, 0.0)
    static const Color3 pink;       ///< (1.0, 0.0, 0.5)
    static const Color3 lawn;       ///< (0.5, 1.0, 0.0)
    static const Color3 mint;       ///< (0.0, 1.0, 0.5)
    static const Color3 violet;     ///< (0.5, 0.5, 1.0)
    static const Color3 teal;       ///< (0.3, 0.5, 0.6)
    static const Color3 grey;       ///< (0.7, 0.7, 0.7)
    static const Color3 darkGrey;   ///< (0.3, 0.3, 0.3)

    float               r;          ///< The red component.
    float               g;          ///< The green component.
    float               b;          ///< The blue component.
};

BE_INLINE Color3::Color3(float r, float g, float b) {
    this->r = r;
    this->g = g;
    this->b = b;
}

BE_INLINE Color3::Color3(const float data[3]) {
    this->r = data[0];
    this->g = data[1];
    this->b = data[2];
}

BE_INLINE Color3::Color3(float c) {
    this->r = c;
    this->g = c;
    this->b = c;
}

BE_INLINE void Color3::Set(float r, float g, float b) {
    this->r = r;
    this->g = g;
    this->b = b;
}

BE_INLINE float Color3::operator[](int index) const {
    assert(index >= 0 && index < Size);
    return ((float *)this)[index];
}

BE_INLINE float &Color3::operator[](int index) {
    assert(index >= 0 && index < Size);
    return ((float *)this)[index];
}


BE_INLINE Color3 &Color3::operator*=(float rhs) {
    r *= rhs;
    g *= rhs;
    b *= rhs;
    return *this;
}

BE_INLINE Color3 &Color3::operator/=(float rhs) {
    float inv = 1.0f / rhs;
    r *= inv;
    g *= inv;
    b *= inv;
    return *this;
}

BE_INLINE bool Color3::Equals(const Color3 &rhs) const {
    return ((r == rhs.r) && (g == rhs.g) && (b == rhs.b));
}

BE_INLINE bool Color3::Equals(const Color3 &rhs, const float epsilon) const {
    if (Math::Fabs(r - rhs.r) > epsilon) {
        return false;
    }
    if (Math::Fabs(g - rhs.g) > epsilon) {
        return false;
    }
    if (Math::Fabs(b - rhs.b) > epsilon) {
        return false;
    }
    return true;
}

BE_INLINE const char *Color3::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), Size, precision);
}

BE_INLINE const Vec3 &Color3::ToVec3() const {
    return *reinterpret_cast<const Vec3 *>(this);
}

BE_INLINE Vec3 &Color3::ToVec3() {
    return *reinterpret_cast<Vec3 *>(this);
}

BE_NAMESPACE_END