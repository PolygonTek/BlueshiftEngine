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

    Mat3x4 - 3x4 Matrix (row major)

-------------------------------------------------------------------------------
*/

#include "Core/Str.h"

BE_NAMESPACE_BEGIN

/// A 3-by-4 matrix for affine transform on 3 dimensional coordinates.
class BE_API Mat3x4 {
public:
    /// Specifies the height of this matrix.
    enum { Rows = 3 };

    /// Specifies the width of this matrix.
    enum { Cols = 4 };

    /// The default constructor does not initialize any members of this class.
    Mat3x4() {}
    /// Constructs a Mat3x4 by explicitly specifying the three row vectors.
    Mat3x4(const Vec4 &row0, const Vec4 &row1, const Vec4 &row2);
    Mat3x4(const float xx, const float xy, const float xz, const float tx,
           const float yx, const float yy, const float yz, const float ty,
           const float zx, const float zy, const float zz, const float tz);
    Mat3x4(const Mat3 &rot, const Vec3 &pos);
    Mat3x4(const Vec3 &s, const Mat3 &rot, const Vec3 &pos);
    Mat3x4(const Mat3 &m);
    Mat3x4(const Mat4 &m);
    /// Constructs a Mat3x4 from a C array, to the value.
    explicit Mat3x4(const float *data);
    
                        /// Casts this Mat3x4 to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const float *       Ptr() const { return (const float *)&mat[0]; }
    float *             Ptr() { return (float *)&mat[0]; }
                        /// Casts this Mat3x4 to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const float *() const { return (const float *)&mat[0]; }
                        operator float *() { return (float *)&mat[0]; }

                        /// Accesses an element of this matrix.
    Vec4 &              At(int index) { return (*this)[index]; }
                        /// Accesses an element of this matrix using array notation.
    const Vec4 &        operator[](int index) const;
    Vec4 &              operator[](int index);

                        /// Performs an unary negation of this matrix.
    Mat3x4              Negate() const { return -(*this); }
                        /// Performs an unary negation of this matrix.
                        /// This function is identical to the member function Negate().
    Mat3x4              operator-() const;

                        /// Unary operator + allows this structure to be used in an expression '+m'.
    Mat3x4              operator+() const { return *this; }

                        /// Adds a matrix to this matrix.
    Mat3x4              Add(const Mat3x4 &m) const { return *this + m; }
                        /// Adds a matrix to this matrix.
    Mat3x4              operator+(const Mat3x4 &rhs) const;

                        /// Subtracts a matrix from this matrix.
    Mat3x4              Sub(const Mat3x4 &m) const { return *this - m; }
                        /// Subtracts a matrix from this matrix.
    Mat3x4              operator-(const Mat3x4 &rhs) const;

                        /// Multiplies a matrix to this matrix.
    Mat3x4              Mul(const Mat3x4 &m) const { return *this * m; }
                        /// Multiplies a matrix to this matrix.
                        /// This function is identical to the member function Mul().
    Mat3x4              operator*(const Mat3x4 &rhs) const;
    
                        /// Multiplies this matrix by a scalar.
    Mat3x4              MulScalar(float s) const { return *this * s; }
                        /// Multiplies this matrix by a scalar.
                        /// This function is identical to the member function MulScalar().
    Mat3x4              operator*(const float rhs) const;

                        /// Transforms the given vector by this matrix
    Vec4                MulVec(const Vec4 &v) const { return *this * v; }
    Vec3                MulVec(const Vec3 &v) const { return *this * v; }
                        /// Returns this->Transpose() * v
    Vec4                TransposedMulVec(const Vec4 &v) const;
    Vec3                TransposedMulVec(const Vec3 &v) const;
                        /// Transforms the given vector by this matrix.
                        /// This function is identical to the member function MulVec().
    Vec4                operator*(const Vec4 &rhs) const;
    Vec3                operator*(const Vec3 &rhs) const;
                        /// Transforms the given vector by the given matrix m.
    friend Vec4         operator*(const Vec4 &lhs, const Mat3x4 &rhs) { return rhs * lhs; }
    friend Vec3         operator*(const Vec3 &lhs, const Mat3x4 &rhs) { return rhs * lhs; }

                        /// Assign from another matrix.
    Mat3x4 &            operator=(const Mat3x4 &rhs);
    Mat3x4 &            operator=(const Mat3 &rhs);

                        /// Adds a matrix to this matrix, in-place.
    Mat3x4 &            AddSelf(const Mat3x4 &m) { *this += m; return *this; }
                        /// Adds a matrix to this matrix, in-place.
                        /// This function is identical to the member function AddSelf().
    Mat3x4 &            operator+=(const Mat3x4 &rhs);

                        /// Subtracts a matrix from this matrix, in-place.
    Mat3x4 &            SubSelf(const Mat3x4 &m) { *this -= m; return *this; }
                        /// Subtracts a matrix from this matrix, in-place.
                        /// This function is identical to the member function SubSelf().
    Mat3x4 &            operator-=(const Mat3x4 &rhs);

                        /// Multiplies this matrix with the given matrix, in-place
    Mat3x4 &            MulSelf(const Mat3x4 &m) { *this *= m; return *this; }
                        /// Multiplies this matrix with the given matrix, in-place
                        /// This function is identical to the member function MulSelf().
    Mat3x4 &            operator*=(const Mat3x4 &rhs);
    
                        /// Multiplies this matrix with the given scalar, in-place
    Mat3x4 &            MulScalarSelf(float s) { *this *= s; return *this; }
                        /// Multiplies this matrix with the given scalar, in-place
                        /// This function is identical to the member function MulScalarSelf().
    Mat3x4 &            operator*=(float rhs);

                        /// Exact compare, no epsilon
    bool                Equals(const Mat3x4 &a) const;
                        /// Compare with epsilon
    bool                Equals(const Mat3x4 &a, const float epsilon) const;
                        /// Exact compare, no epsilon
    bool                operator==(const Mat3x4 &rhs) const;
                        /// Exact compare, no epsilon
    bool                operator!=(const Mat3x4 &rhs) const;

                        /// Tests if this is the identity matrix, up to the given epsilon.
    bool                IsIdentity(const float epsilon = MATRIX_EPSILON) const;

                        /// Sets all the element of this matrix to zero.
    void                SetZero();
                        /// Sets this matrix to equal the identity.
    void                SetIdentity();

                        /// Fix degenerate axial cases
    bool                FixDegeneracies();
                        /// Change tiny numbers to zero
    bool                FixDenormals();

    void                SetRotationScale(const Mat3 &rotation, const Vec3 &scale);
                        /// Sets the translation part of this matrix.
    void                SetTranslation(const Vec3 &t);
                        /// Sets linear transformation matrix which is a combination of translation, rotation and scale.
    void                SetTRS(const Vec3 &translation, const Mat3 &rotation, const Vec3 &scale);
                        /// Sets linear transformation matrix which is a combination of translation, rotation and scale.
    void                SetTQS(const Vec3 &translation, const Quat &rotation, const Vec3 &scale);

                        /// Decomposes this matrix to translation, rotation and scale parts.
    void                GetTRS(Vec3 &translation, Mat3 &rotation, Vec3 &scale) const;
                        /// Decomposes this matrix to translation, rotation and scale parts.
    void                GetTQS(Vec3 &translation, Quat &rotation, Vec3 &scale) const;

                        /// Inverts this matrix.
    Mat3x4              Inverse() const;
                        /// Inverts this matrix, in-place.
    void                InverseSelf();

                        /// Translates by the given offset, in-place.
    Mat3x4 &            Translate(const Vec3 &t) { return Translate(t.x, t.y, t.z); }
    Mat3x4 &            Translate(float tx, float ty, float tz);

                        /// Translates right by the given offset, in-place.
    Mat3x4 &            TranslateRight(const Vec3 &t) { return TranslateRight(t.x, t.y, t.z); }
    Mat3x4 &            TranslateRight(float tx, float ty, float tz);

                        /// Scales by the given factors, in-place.
    Mat3x4 &            Scale(float sx, float sy, float sz);
    Mat3x4 &            Scale(const Vec3 &s) { return Scale(s.x, s.y, s.z); }

                        /// Performs uniform scaling by the given amount, in-place.
    Mat3x4 &            UniformScale(const float s) { return Scale(s, s, s); }

                        /// Returns translation matrix
    static Mat3x4       FromTranslation(float tx, float ty, float tz);
    static Mat3x4       FromTranslation(const Vec3 &t) { return FromTranslation(t.x, t.y, t.z); }

                        /// Returns scaling matrix
    static Mat3x4       FromScale(float sx, float sy, float sz);
    static Mat3x4       FromScale(const Vec3 &s) { return FromScale(s.x, s.y, s.z); }

    Vec3                Transform(const Vec3 &v) const;
    Vec3                Transform(const Vec4 &v) const;
    Vec3                TransformNormal(const Vec3 &v) const;
    Mat3x4 &            TransformSelf(const Mat3x4 &a);

    Mat3x4 &            UntransformSelf(const Mat3x4 &a);

                        /// Returns upper left 3x3 part.
    Mat3                ToMat3() const;
                        /// Returns 4x4 square matrix.
    Mat4                ToMat4() const;
                        /// Returns scale part.
    Vec3                ToScaleVec3() const;
                        /// Returns translation vector part.
    Vec3                ToTranslationVec3() const;

                        /// Returns "_00 _01 _02 _03 _10 _11 _12 _13 _20 _21 _22 _23".
    const char *        ToString() const { return ToString(4); }
                        /// Returns "_00 _01 _02 _03 _10 _11 _12 _13 _20 _21 _22 _23" with the given precisions.
    const char *        ToString(int precision) const;

                        /// Creates from the string
    static Mat3x4       FromString(const char *str);

                        /// Returns dimension of this type
    int                 GetDimension() const { return Rows * Cols; }
    
    static const Mat3x4 zero;
    static const Mat3x4 identity;

    Vec4                mat[Rows];
};

BE_INLINE Mat3x4::Mat3x4(const Vec4 &row0, const Vec4 &row1, const Vec4 &row2) {
    mat[0] = row0;
    mat[1] = row1;
    mat[2] = row2;
}

BE_INLINE Mat3x4::Mat3x4(const float xx, const float xy, const float xz, const float tx,
    const float yx, const float yy, const float yz, const float ty,
    const float zx, const float zy, const float zz, const float tz) {
    mat[0][0] = xx;
    mat[0][1] = xy;
    mat[0][2] = xz;
    mat[0][3] = tx;

    mat[1][0] = yx;
    mat[1][1] = yy;
    mat[1][2] = yz;
    mat[1][3] = ty;

    mat[2][0] = zx;
    mat[2][1] = zy;
    mat[2][2] = zz;
    mat[2][3] = tz;
}

BE_INLINE Mat3x4::Mat3x4(const Mat3 &rot, const Vec3 &pos) {
    mat[0][0] = rot[0][0];
    mat[0][1] = rot[1][0];
    mat[0][2] = rot[2][0];
    mat[0][3] = pos[0];

    mat[1][0] = rot[0][1];
    mat[1][1] = rot[1][1];
    mat[1][2] = rot[2][1];
    mat[1][3] = pos[1];

    mat[2][0] = rot[0][2];
    mat[2][1] = rot[1][2];
    mat[2][2] = rot[2][2];
    mat[2][3] = pos[2];
}

BE_INLINE Mat3x4::Mat3x4(const Vec3 &s, const Mat3 &rot, const Vec3 &pos) {
    mat[0][0] = rot[0][0] * s[0];
    mat[0][1] = rot[1][0] * s[1];
    mat[0][2] = rot[2][0] * s[2];
    mat[0][3] = pos[0];

    mat[1][0] = rot[0][1] * s[0];
    mat[1][1] = rot[1][1] * s[1];
    mat[1][2] = rot[2][1] * s[2];
    mat[1][3] = pos[1];

    mat[2][0] = rot[0][2] * s[0];
    mat[2][1] = rot[1][2] * s[1];
    mat[2][2] = rot[2][2] * s[2];
    mat[2][3] = pos[2];
}

BE_INLINE Mat3x4::Mat3x4(const Mat3 &m) {
    mat[0][0] = m[0][0];
    mat[0][1] = m[1][0];
    mat[0][2] = m[2][0];
    mat[0][3] = 0;

    mat[1][0] = m[0][1];
    mat[1][1] = m[1][1];
    mat[1][2] = m[2][1];
    mat[1][3] = 0;

    mat[2][0] = m[0][2];
    mat[2][1] = m[1][2];
    mat[2][2] = m[2][2];
    mat[2][3] = 0;
}

BE_INLINE Mat3x4::Mat3x4(const Mat4 &m) {
    mat[0][0] = m[0][0];
    mat[0][1] = m[0][1];
    mat[0][2] = m[0][2];
    mat[0][3] = m[0][3];

    mat[1][0] = m[1][0];
    mat[1][1] = m[1][1];
    mat[1][2] = m[1][2];
    mat[1][3] = m[1][3];

    mat[2][0] = m[2][0];
    mat[2][1] = m[2][1];
    mat[2][2] = m[2][2];
    mat[2][3] = m[2][3];
}

BE_INLINE Mat3x4::Mat3x4(const float *data) {
    memcpy(mat, data, sizeof(float) * Rows * Cols);
}

BE_INLINE void Mat3x4::SetRotationScale(const Mat3 &rotation, const Vec3 &scale) {
    mat[0][0] = rotation[0][0] * scale[0];
    mat[0][1] = rotation[1][0] * scale[1];
    mat[0][2] = rotation[2][0] * scale[2];

    mat[1][0] = rotation[0][1] * scale[0];
    mat[1][1] = rotation[1][1] * scale[1];
    mat[1][2] = rotation[2][1] * scale[2];

    mat[2][0] = rotation[0][2] * scale[0];
    mat[2][1] = rotation[1][2] * scale[1];
    mat[2][2] = rotation[2][2] * scale[2];
}

BE_INLINE void Mat3x4::SetTranslation(const Vec3 &t) {
    mat[0][3] = t[0];
    mat[1][3] = t[1];
    mat[2][3] = t[2];
}

BE_INLINE const Vec4 &Mat3x4::operator[](int index) const {
    assert(index >= 0 && index < Rows);
    return mat[index];
}

BE_INLINE Vec4 &Mat3x4::operator[](int index) {
    assert(index >= 0 && index < Rows);
    return mat[index];
}

BE_INLINE Mat3x4 Mat3x4::operator-() const {
    return Mat3x4(
        -mat[0][0], -mat[0][1], -mat[0][2], -mat[0][3],
        -mat[1][0], -mat[1][1], -mat[1][2], -mat[1][3],
        -mat[2][0], -mat[2][1], -mat[2][2], -mat[2][3]);
}

BE_INLINE Mat3x4 Mat3x4::operator+(const Mat3x4 &a) const {
    return Mat3x4(
        mat[0].x + a[0].x, mat[0].y + a[0].y, mat[0].z + a[0].z, mat[0].w + a[0].w,
        mat[1].x + a[1].x, mat[1].y + a[1].y, mat[1].z + a[1].z, mat[1].w + a[1].w,
        mat[2].x + a[2].x, mat[2].y + a[2].y, mat[2].z + a[2].z, mat[2].w + a[2].w);
}

BE_INLINE Mat3x4 Mat3x4::operator-(const Mat3x4 &a) const {
    return Mat3x4(
        mat[0].x - a[0].x, mat[0].y - a[0].y, mat[0].z - a[0].z, mat[0].w - a[0].w,
        mat[1].x - a[1].x, mat[1].y - a[1].y, mat[1].z - a[1].z, mat[1].w - a[1].w,
        mat[2].x - a[2].x, mat[2].y - a[2].y, mat[2].z - a[2].z, mat[2].w - a[2].w);
}

BE_INLINE bool Mat3x4::Equals(const Mat3x4 &a) const {
    if (mat[0].Equals(a[0]) &&
        mat[1].Equals(a[1]) &&
        mat[2].Equals(a[2])) {
        return true;
    }
    return false;
}

BE_INLINE bool Mat3x4::Equals(const Mat3x4 &a, const float epsilon) const {
    if (mat[0].Equals(a[0], epsilon) &&
        mat[1].Equals(a[1], epsilon) &&
        mat[2].Equals(a[2], epsilon)) {
        return true;
    }
    return false;
}

BE_INLINE bool Mat3x4::operator==(const Mat3x4 &a) const {
    return Equals(a);
}

BE_INLINE bool Mat3x4::operator!=(const Mat3x4 &a) const {
    return !Equals(a);
}

BE_INLINE bool Mat3x4::IsIdentity(const float epsilon) const {
    return Equals(Mat3x4::identity, epsilon);
}

BE_INLINE void Mat3x4::SetZero() {
    *this = Mat3x4::zero;
}

BE_INLINE void Mat3x4::SetIdentity() {
    *this = Mat3x4::identity;
}

BE_INLINE bool Mat3x4::FixDegeneracies() {
    bool r = mat[0].ToVec3().FixDegenerateNormal();
    r |= mat[1].ToVec3().FixDegenerateNormal();
    r |= mat[2].ToVec3().FixDegenerateNormal();
    return r;
}

BE_INLINE bool Mat3x4::FixDenormals() {
    bool r = mat[0].ToVec3().FixDenormals();
    r |= mat[1].ToVec3().FixDenormals();
    r |= mat[2].ToVec3().FixDenormals();
    return r;
}

BE_INLINE Mat3x4 Mat3x4::Inverse() const {
    Mat3x4 invMat = *this;
    invMat.InverseSelf();
    return invMat;
}

BE_INLINE Mat3x4 Mat3x4::operator*(const float rhs) const {
    return Mat3x4(
        mat[0][0] * rhs, mat[0][1] * rhs, mat[0][2] * rhs, mat[0][3] * rhs, 
        mat[1][1] * rhs, mat[1][1] * rhs, mat[1][2] * rhs, mat[1][3] * rhs,
        mat[2][0] * rhs, mat[2][1] * rhs, mat[2][2] * rhs, mat[2][3] * rhs);
}

BE_INLINE Vec4 Mat3x4::operator*(const Vec4 &vec) const {
    return Vec4(
        mat[0].x * vec.x + mat[0].y * vec.y + mat[0].z * vec.z + mat[0].w * vec.w,
        mat[1].x * vec.x + mat[1].y * vec.y + mat[1].z * vec.z + mat[1].w * vec.w,
        mat[2].x * vec.x + mat[2].y * vec.y + mat[2].z * vec.z + mat[2].w * vec.w,
        vec.w);
}

BE_INLINE Vec3 Mat3x4::operator*(const Vec3 &vec) const {
    return Vec3(
        mat[0].x * vec.x + mat[0].y * vec.y + mat[0].z * vec.z + mat[0].w,
        mat[1].x * vec.x + mat[1].y * vec.y + mat[1].z * vec.z + mat[1].w,
        mat[2].x * vec.x + mat[2].y * vec.y + mat[2].z * vec.z + mat[2].w);
}

BE_INLINE Vec4 Mat3x4::TransposedMulVec(const Vec4 &vec) const {
    return Vec4(
        mat[0].x * vec.x + mat[1].x * vec.y + mat[2].x * vec.z,
        mat[0].y * vec.x + mat[1].y * vec.y + mat[2].y * vec.z,
        mat[0].z * vec.x + mat[1].z * vec.y + mat[2].z * vec.z,
        mat[0].w * vec.x + mat[1].w * vec.y + mat[2].w * vec.z + vec.w);
}

BE_INLINE Vec3 Mat3x4::TransposedMulVec(const Vec3 &vec) const {
    return Vec3(
        mat[0].x * vec.x + mat[1].x * vec.y + mat[2].x * vec.z,
        mat[0].y * vec.x + mat[1].y * vec.y + mat[2].y * vec.z,
        mat[0].z * vec.x + mat[1].z * vec.y + mat[2].z * vec.z);
}

BE_INLINE Mat3x4 &Mat3x4::operator=(const Mat3x4 &rhs) {
    mat[0][0] = rhs[0][0];
    mat[0][1] = rhs[0][1];
    mat[0][2] = rhs[0][2];
    mat[0][3] = rhs[0][3];

    mat[1][0] = rhs[1][0];
    mat[1][1] = rhs[1][1];
    mat[1][2] = rhs[1][2];
    mat[1][3] = rhs[1][3];

    mat[2][0] = rhs[2][0];
    mat[2][1] = rhs[2][1];
    mat[2][2] = rhs[2][2];
    mat[2][3] = rhs[2][3];

    return *this;
}

BE_INLINE Mat3x4 &Mat3x4::operator=(const Mat3 &rhs) {
    mat[0][0] = rhs[0][0];
    mat[0][1] = rhs[1][0];
    mat[0][2] = rhs[2][0];
    mat[0][3] = 0;

    mat[1][0] = rhs[0][1];
    mat[1][1] = rhs[1][1];
    mat[1][2] = rhs[2][1];
    mat[1][3] = 0;

    mat[2][0] = rhs[0][2];
    mat[2][1] = rhs[1][2];
    mat[2][2] = rhs[2][2];
    mat[2][3] = 0;

    return *this;
}

BE_INLINE Mat3x4 &Mat3x4::operator+=(const Mat3x4 &rhs) {
    mat[0][0] += rhs.mat[0][0];
    mat[0][1] += rhs.mat[0][1];
    mat[0][2] += rhs.mat[0][2];
    mat[0][3] += rhs.mat[0][3];

    mat[1][0] += rhs.mat[1][0];
    mat[1][1] += rhs.mat[1][1];
    mat[1][2] += rhs.mat[1][2];
    mat[1][3] += rhs.mat[1][3];

    mat[2][0] += rhs.mat[2][0];
    mat[2][1] += rhs.mat[2][1];
    mat[2][2] += rhs.mat[2][2];
    mat[2][3] += rhs.mat[2][3];

    return *this;
}

BE_INLINE Mat3x4 &Mat3x4::operator-=(const Mat3x4 &rhs) {
    mat[0][0] -= rhs.mat[0][0];
    mat[0][1] -= rhs.mat[0][1];
    mat[0][2] -= rhs.mat[0][2];
    mat[0][3] -= rhs.mat[0][3];

    mat[1][0] -= rhs.mat[1][0];
    mat[1][1] -= rhs.mat[1][1];
    mat[1][2] -= rhs.mat[1][2];
    mat[1][3] -= rhs.mat[1][3];

    mat[2][0] -= rhs.mat[2][0];
    mat[2][1] -= rhs.mat[2][1];
    mat[2][2] -= rhs.mat[2][2];
    mat[2][3] -= rhs.mat[2][3];

    return *this;
}

BE_INLINE Mat3x4 &Mat3x4::operator*=(const Mat3x4 &rhs) {
    *this = (*this) * rhs;
    return *this;
}

BE_INLINE Mat3x4 &Mat3x4::operator*=(float rhs) {
    mat[0].x *= rhs;
    mat[0].y *= rhs;
    mat[0].z *= rhs;
    mat[0].w *= rhs;

    mat[1].x *= rhs;
    mat[1].y *= rhs;
    mat[1].z *= rhs;
    mat[1].w *= rhs;

    mat[2].x *= rhs;
    mat[2].y *= rhs;
    mat[2].z *= rhs;
    mat[2].w *= rhs;

    return *this;
}

BE_INLINE Vec3 Mat3x4::Transform(const Vec3 &v) const {
    return Vec3(
        mat[0][0] * v[0] + mat[0][1] * v[1] + mat[0][2] * v[2] + mat[0][3],
        mat[1][0] * v[0] + mat[1][1] * v[1] + mat[1][2] * v[2] + mat[1][3],
        mat[2][0] * v[0] + mat[2][1] * v[1] + mat[2][2] * v[2] + mat[2][3]);
}

BE_INLINE Vec3 Mat3x4::Transform(const Vec4 &v) const {
    return Vec3(
        mat[0][0] * v[0] + mat[0][1] * v[1] + mat[0][2] * v[2] + mat[0][3] * v[3],
        mat[1][0] * v[0] + mat[1][1] * v[1] + mat[1][2] * v[2] + mat[1][3] * v[3],
        mat[2][0] * v[0] + mat[2][1] * v[1] + mat[2][2] * v[2] + mat[2][3] * v[3]);
}

BE_INLINE Vec3 Mat3x4::TransformNormal(const Vec3 &v) const {
    return Vec3(
        mat[0][0] * v[0] + mat[0][1] * v[1] + mat[0][2] * v[2],
        mat[1][0] * v[0] + mat[1][1] * v[1] + mat[1][2] * v[2],
        mat[2][0] * v[0] + mat[2][1] * v[1] + mat[2][2] * v[2]);
}

BE_INLINE Mat3 Mat3x4::ToMat3() const {
    return Mat3(
        mat[0][0], mat[1][0], mat[2][0],
        mat[0][1], mat[1][1], mat[2][1],
        mat[0][2], mat[1][2], mat[2][2]);
}

BE_INLINE Mat4 Mat3x4::ToMat4() const {
    return Mat4(
        mat[0][0], mat[0][1], mat[0][2], mat[0][3],
        mat[1][0], mat[1][1], mat[1][2], mat[1][3],
        mat[2][0], mat[2][1], mat[2][2], mat[2][3],
        0.0f, 0.0f, 0.0f, 1.0f);
}

BE_INLINE const Mat3x4 &Mat4::ToMat3x4() const {
    return *reinterpret_cast<const Mat3x4 *>(this);
}

BE_INLINE Mat3x4 &Mat4::ToMat3x4() {
    return *reinterpret_cast<Mat3x4 *>(this);
}

BE_INLINE Vec3 Mat3x4::ToScaleVec3() const {
    return Vec3(
        Math::Sqrt(mat[0][0] * mat[0][0] + mat[1][0] * mat[1][0] + mat[2][0] * mat[2][0]),
        Math::Sqrt(mat[0][1] * mat[0][1] + mat[1][1] * mat[1][1] + mat[2][1] * mat[2][1]),
        Math::Sqrt(mat[0][2] * mat[0][2] + mat[1][2] * mat[1][2] + mat[2][2] * mat[2][2]));
}

BE_INLINE Vec3 Mat3x4::ToTranslationVec3() const {
    return Vec3(mat[0][3], mat[1][3], mat[2][3]);
}

BE_INLINE Mat3x4 Mat3x4::FromTranslation(float tx, float ty, float tz) {
    Mat3x4 m;
    m.mat[0][0] = 1;
    m.mat[0][1] = 0;
    m.mat[0][2] = 0;
    m.mat[0][3] = tx;

    m.mat[1][0] = 0;
    m.mat[1][1] = 1;
    m.mat[1][2] = 0;
    m.mat[1][3] = ty;

    m.mat[2][0] = 0;
    m.mat[2][1] = 0;
    m.mat[2][2] = 1;
    m.mat[2][3] = tz;
    return m;
}

BE_INLINE Mat3x4 Mat3x4::FromScale(float sx, float sy, float sz) {
    Mat3x4 m;
    m.mat[0][0] = sx;
    m.mat[0][1] = 0;
    m.mat[0][2] = 0;
    m.mat[0][3] = 0;

    m.mat[0][0] = 0;
    m.mat[0][1] = sy;
    m.mat[0][2] = 0;
    m.mat[0][3] = 0;

    m.mat[0][0] = 0;
    m.mat[0][1] = 0;
    m.mat[0][2] = sz;
    m.mat[0][3] = 0;
    return m;
}

BE_INLINE const char *Mat3x4::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), Rows * Cols, precision);
}

BE_NAMESPACE_END
