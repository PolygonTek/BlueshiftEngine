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

    Mat4 - 4x4 Matrix (row major)

-------------------------------------------------------------------------------
*/

#include "Core/Str.h"

BE_NAMESPACE_BEGIN

/// A 4-by-4 matrix for affine transformations and perspective projections of 3D geometry.
class BE_API Mat4 {
public:
    /// Specifies the height of this matrix.
    enum { Rows = 4 };

    /// Specifies the width of this matrix.
    enum { Cols = 4 };

    /// The default constructor does not initialize any members of this class.
    Mat4() {}
    /// Constructs a Mat4 by explicitly specifying the four row vectors.
    Mat4(const Vec4 &row0, const Vec4 &row1, const Vec4 &row2, const Vec4 &row3);
    /// Constructs a Mat4 by explicitly specifying all the matrix elements.
    Mat4(float _00, float _01, float _02, float _03,
         float _10, float _11, float _12, float _13,
         float _20, float _21, float _22, float _23,
         float _30, float _31, float _32, float _33);
    Mat4(const Mat3 &rotation, const Vec3 &translation);
    /// Constructs a Mat4 from a C array, to the value.
    explicit Mat4(const float data[4][4]);
    explicit Mat4(const float *data);

                        /// Casts this Mat4 to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const float *       Ptr() const { return (const float *)&mat[0]; }
    float *             Ptr() { return (float *)&mat[0]; }
                        /// Casts this Mat4 to a C array.
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
    Mat4                Negate() const { return -(*this); }
                        /// Performs an unary negation of this matrix.
                        /// This function is identical to the member function Negate().
    Mat4                operator-() const;

                        /// Unary operator + allows this structure to be used in an expression '+m'.
    Mat4                operator+() const { return *this; }

                        /// Adds a matrix to this matrix.
    Mat4                Add(const Mat4 &m) const { return *this + m; }
                        /// Adds a matrix to this matrix.
    Mat4                operator+(const Mat4 &rhs) const;

                        /// Subtracts a matrix from this matrix.
    Mat4                Sub(const Mat4 &m) const { return *this - m; }
                        /// Subtracts a matrix from this matrix.
    Mat4                operator-(const Mat4 &rhs) const;

                        /// Multiplies a matrix to this matrix.
    Mat4                Mul(const Mat4 &m) const { return *this * m; }
    Mat4                Mul(const Mat3x4 &m) const { return *this * m; }
                        /// Returns this->Transpose() * m
    Mat4                TransposedMul(const Mat4 &m) const;
    Mat4                TransposedMul(const Mat3x4 &m) const;
                        /// Multiplies a matrix to this matrix.
                        /// This function is identical to the member function Mul().
    Mat4                operator*(const Mat4 &rhs) const;
    Mat4                operator*(const Mat3x4 &rhs) const;

                        /// Multiplies this matrix by a scalar.
    Mat4                MulScalar(float s) const { return *this * s; }
                        /// Multiplies this matrix by a scalar.
                        /// This function is identical to the member function MulScalar().
    Mat4                operator*(float rhs) const;
                        /// Multiplies the given matrix by a scalar.
    friend Mat4         operator*(float lhs, const Mat4 &rhs) { return rhs * lhs; }

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
    friend Vec4         operator*(const Vec4 &lhs, const Mat4 &rhs) { return rhs * lhs; }
    friend Vec3         operator*(const Vec3 &lhs, const Mat4 &rhs) { return rhs * lhs; }

    Vec3                TransformNormal(const Vec3 &n) const;

                        /// Assign from another matrix.
    Mat4 &              operator=(const Mat4 &rhs);
    Mat4 &              operator=(const Mat3x4 &rhs);
    Mat4 &              operator=(const Mat3 &rhs);

                        /// Adds a matrix to this matrix, in-place.
    Mat4 &              AddSelf(const Mat4 &m) { *this += m; return *this; }
                        /// Adds a matrix to this matrix, in-place.
                        /// This function is identical to the member function AddSelf().
    Mat4 &              operator+=(const Mat4 &rhs);

                        /// Subtracts a matrix from this matrix, in-place.
    Mat4 &              SubSelf(const Mat4 &m) { *this -= m; return *this; }
                        /// Subtracts a matrix from this matrix, in-place.
                        /// This function is identical to the member function SubSelf().
    Mat4 &              operator-=(const Mat4 &rhs);
    
                        /// Multiplies this matrix with the given matrix, in-place
    Mat4 &              MulSelf(const Mat4 &m) { *this *= m; return *this; }
    Mat4 &              MulSelf(const Mat3x4 &m) { *this *= m; return *this; }
                        /// Multiplies this matrix with the given matrix, in-place
                        /// This function is identical to the member function MulSelf().
    Mat4 &              operator*=(const Mat4 &rhs);
    Mat4 &              operator*=(const Mat3x4 &rhs);
    
                        /// Multiplies this matrix with the given scalar, in-place
    Mat4 &              MulScalarSelf(float s) { *this *= s; return *this; }
                        /// Multiplies this matrix with the given scalar, in-place
                        /// This function is identical to the member function MulScalarSelf().
    Mat4 &              operator*=(float rhs);
                        
                        /// Multiplies the vector lhs with the given matrix rhs, in-place on vector. i.e. lhs *= rhs
    friend Vec4 &       operator*=(Vec4 &lhs, const Mat4 &rhs) { lhs = rhs * lhs; return lhs; }
                        /// Multiplies the vector lhs with the given matrix rhs, in-place on vector. i.e. lhs *= rhs
    friend Vec3 &       operator*=(Vec3 &lhs, const Mat4 &rhs) { lhs = rhs * lhs; return lhs; }

                        /// Exact compare, no epsilon
    bool                Equals(const Mat4 &m) const;
                        /// Compare with epsilon
    bool                Equals(const Mat4 &m, const float epsilon) const;
                        /// Exact compare, no epsilon
    bool                operator==(const Mat4 &rhs) const { return Equals(rhs); }
                        /// Exact compare, no epsilon
    bool                operator!=(const Mat4 &rhs) const { return !Equals(rhs); }

                        /// Tests if this is the identity matrix, up to the given epsilon.
    bool                IsIdentity(const float epsilon = MATRIX_EPSILON) const;
                        /// Tests if this is the symmetric matrix, up to the given epsilon.
    bool                IsSymmetric(const float epsilon = MATRIX_EPSILON) const;
                        /// Tests if this is the diagonal matrix, up to the given epsilon.
    bool                IsDiagonal(const float epsilon = MATRIX_EPSILON) const;
                        /// Tests if this is the singular matrix.
    bool                IsSingular() const;

    bool                IsAffine() const;

                        /// Sets all the element of this matrix to zero.
    void                SetZero();
                        /// Sets this matrix to equal the identity.
    void                SetIdentity();
                        /// Sets the translation part of this matrix.
    void                SetTranslation(float tx, float ty, float tz);

                        /// Sets perspective projection matrix with given frustum definition.
    void                SetFrustum(float left, float right, float bottom, float top, float znear, float zfar);
                        /// Sets perspective projection matrix.
    void                SetPerspective(float fovy, float aspect, float znear, float zfar);
                        /// Sets orthogonal projection matrix.
    void                SetOrtho(float left, float right, float bottom, float top, float znear, float zfar);
                        /// Sets reflection matrix with the given reflection plane.
    void                SetReflect(const Plane &plane);
                        /// Sets linear transformation matrix which is a combination of translation, rotation and scale.
    void                SetTRS(const Vec3 &translation, const Mat3 &rotation, const Vec3 &scale);
                        /// Sets linear transformation matrix which is a combination of translation, rotation and scale.
    void                SetTQS(const Vec3 &translation, const Quat &rotation, const Vec3 &scale);

                        /// Decomposes this matrix to translation, rotation and scale parts.
    void                GetTRS(Vec3 &translation, Mat3 &rotation, Vec3 &scale) const;
                        /// Decomposes this matrix to translation, rotation and scale parts.
    void                GetTQS(Vec3 &translation, Quat &rotation, Vec3 &scale) const;

                        /// Returns the sum of the diagonal elements of this matrix.
                        /// Mathematically, this means sum of all eigenvalues
    float               Trace() const;
                        /// Returns the determinant of this matrix
                        /// Mathematically, this means multiplication of all eigenvalues
    float               Determinant() const;
    
                        /// Transposes this matrix.
                        /// This operation swaps all elements with respect to the diagonal.
    Mat4                Transpose() const;
                        /// Transposes this matrix, in-place.
    Mat4 &              TransposeSelf();

                        /// Inverts this matrix.
    Mat4                Inverse() const;
                        /// Inverts this matrix, in-place.
                        /// @return False if determinant is zero.
    bool                InverseSelf();

                        /// Inverts a affine matrix.
                        /// If a matrix M is made up of only translation, rotation, reflection, scaling and shearing,
                        /// then M is affine matrix and this function can be used to compute the inverse
    Mat4                AffineInverse() const;
                        /// Inverts a affine matrix, in-place.
    bool                AffineInverseSelf();
    
                        /// Inverts a euclidean matrix
                        /// If a matrix is made up of only translation, rotation, and reflection,
                        /// then M is euclidean matrix and this function can be used to compute the inverse
    Mat4                EuclideanInverse() const;
                        /// Inverts a euclidean matrix, in-place.
    bool                EuclideanInverseSelf();

                        /// Translates by the given offset, in-place.
    Mat4 &              Translate(const Vec3 &t) { return Translate(t.x, t.y, t.z); }
    Mat4 &              Translate(float tx, float ty, float tz);

                        /// Translates right by the given offset, in-place.
    Mat4 &              TranslateRight(const Vec3 &t) { return TranslateRight(t.x, t.y, t.z); }
    Mat4 &              TranslateRight(float tx, float ty, float tz);

                        /// Perform scaling by the given factors, in-place.
    Mat4 &              Scale(float sx, float sy, float sz);
    Mat4 &              Scale(const Vec3 &s) { return Scale(s.x, s.y, s.z); }

                        /// Performs uniform scaling by the given amount, in-place.
    Mat4 &              UniformScale(const float s) { return Scale(s, s, s); }

                        /// Returns translation matrix
    static Mat4         FromTranslation(float tx, float ty, float tz);
    static Mat4         FromTranslation(const Vec3 &t) { return FromTranslation(t.x, t.y, t.z); }

                        /// Returns scaling matrix
    static Mat4         FromScale(float sx, float sy, float sz);
    static Mat4         FromScale(const Vec3 &s) { return FromTranslation(s.x, s.y, s.z); }

                        /// LU decomposition, in-place.
    bool                DecompLU();

                        /// Solve Ax = b with LU decomposition.
    Vec4                SolveLU(const Vec4 &b) const;
    
                        /// Returns upper left 3x3 part.
    Mat3                ToMat3() const;
                        /// Returns upper 3x4 part.
    const Mat3x4 &      ToMat3x4() const;
    Mat3x4 &            ToMat3x4();
                        /// Returns scale part.
    Vec3                ToScaleVec3() const;
                        /// Returns translation vector part.
    Vec3                ToTranslationVec3() const;

                        /// Returns "_00 _01 _02 _03 _10 _11 _12 _13 _20 _21 _22 _23 _30 _31 _32 _33".
    const char *        ToString() const { return ToString(4); }
                        /// Returns "_00 _01 _02 _03 _10 _11 _12 _13 _20 _21 _22 _23 _30 _31 _32 _33" with the given precisions.
    const char *        ToString(int precision) const;

                        /// Creates from the string
    static Mat4         FromString(const char *str);

                        /// Returns dimension of this type
    int                 GetDimension() const { return Rows * Cols; }

    static const Mat4   zero;
    static const Mat4   identity;

    Vec4                mat[Rows];
};

BE_INLINE Mat4::Mat4(const Vec4 &row0, const Vec4 &row1, const Vec4 &row2, const Vec4 &row3) {
    mat[0] = row0;
    mat[1] = row1;
    mat[2] = row2;
    mat[3] = row3;
}

BE_INLINE Mat4::Mat4(
    float _00, float _01, float _02, float _03, 
    float _10, float _11, float _12, float _13, 
    float _20, float _21, float _22, float _23, 
    float _30, float _31, float _32, float _33) {
    mat[0][0] = _00; 
    mat[0][1] = _01; 
    mat[0][2] = _02; 
    mat[0][3] = _03;

    mat[1][0] = _10; 
    mat[1][1] = _11; 
    mat[1][2] = _12; 
    mat[1][3] = _13;

    mat[2][0] = _20; 
    mat[2][1] = _21; 
    mat[2][2] = _22;
    mat[2][3] = _23;

    mat[3][0] = _30; 
    mat[3][1] = _31; 
    mat[3][2] = _32; 
    mat[3][3] = _33;
}

BE_INLINE Mat4::Mat4(const Mat3 &rotation, const Vec3 &translation) {
    mat[0][0] = rotation[0][0];
    mat[0][1] = rotation[1][0];
    mat[0][2] = rotation[2][0];
    mat[0][3] = translation[0];

    mat[1][0] = rotation[0][1];
    mat[1][1] = rotation[1][1];
    mat[1][2] = rotation[2][1];
    mat[1][3] = translation[1];

    mat[2][0] = rotation[0][2];
    mat[2][1] = rotation[1][2];
    mat[2][2] = rotation[2][2];
    mat[2][3] = translation[2];

    mat[3][0] = 0.0f;
    mat[3][1] = 0.0f;
    mat[3][2] = 0.0f;
    mat[3][3] = 1.0f;
}

BE_INLINE Mat4::Mat4(const float data[4][4]) {
    memcpy(mat, data, sizeof(float) * Rows * Cols);
}

BE_INLINE Mat4::Mat4(const float *data) {
    memcpy(mat, data, sizeof(float) * Rows * Cols);
}

BE_INLINE const Vec4 &Mat4::operator[](int index) const {
    assert(index >= 0 && index < Rows);
    return mat[index];
}

BE_INLINE Vec4 &Mat4::operator[](int index) {
    assert(index >= 0 && index < Rows);
    return mat[index];
}

BE_INLINE Mat4 Mat4::operator-() const {
    return Mat4(
        -mat[0][0], -mat[0][1], -mat[0][2], -mat[0][3], 
        -mat[1][0], -mat[1][1], -mat[1][2], -mat[1][3], 
        -mat[2][0], -mat[2][1], -mat[2][2], -mat[2][3], 
        -mat[3][0], -mat[3][1], -mat[3][2], -mat[3][3]);
}

BE_INLINE Mat4 Mat4::operator+(const Mat4 &a) const {
    return Mat4(
        mat[0].x + a[0].x, mat[0].y + a[0].y, mat[0].z + a[0].z, mat[0].w + a[0].w, 
        mat[1].x + a[1].x, mat[1].y + a[1].y, mat[1].z + a[1].z, mat[1].w + a[1].w, 
        mat[2].x + a[2].x, mat[2].y + a[2].y, mat[2].z + a[2].z, mat[2].w + a[2].w, 
        mat[3].x + a[3].x, mat[3].y + a[3].y, mat[3].z + a[3].z, mat[3].w + a[3].w);
}

BE_INLINE Mat4 Mat4::operator-(const Mat4 &a) const {
    return Mat4(
        mat[0].x - a[0].x, mat[0].y - a[0].y, mat[0].z - a[0].z, mat[0].w - a[0].w, 
        mat[1].x - a[1].x, mat[1].y - a[1].y, mat[1].z - a[1].z, mat[1].w - a[1].w, 
        mat[2].x - a[2].x, mat[2].y - a[2].y, mat[2].z - a[2].z, mat[2].w - a[2].w, 
        mat[3].x - a[3].x, mat[3].y - a[3].y, mat[3].z - a[3].z, mat[3].w - a[3].w);
}

BE_INLINE Mat4 Mat4::operator*(const Mat4 &a) const {
    Mat4 dst;
    float *dstPtr = dst.Ptr();
    const float *m1Ptr = Ptr();
    const float *m2Ptr = a.Ptr();

    for (int c = 0; c < Cols; c++) {
        for (int r = 0; r < Rows; r++) {
            *dstPtr = 
                m1Ptr[0] * m2Ptr[0 * Cols + r] +
                m1Ptr[1] * m2Ptr[1 * Cols + r] +
                m1Ptr[2] * m2Ptr[2 * Cols + r] +
                m1Ptr[3] * m2Ptr[3 * Cols + r];
            dstPtr++;
        }
        m1Ptr += Cols;
    }
    return dst;
}

BE_INLINE Mat4 Mat4::TransposedMul(const Mat4 &a) const {
    Mat4 dst;
    float *dstPtr = dst.Ptr();
    const float *m1Ptr = Ptr();
    const float *m2Ptr = a.Ptr();

    for (int c = 0; c < Cols; c++) {
        for (int r = 0; r < Rows; r++) {
            *dstPtr = 
                m1Ptr[0 * Cols] * m2Ptr[0 * Cols + r] +
                m1Ptr[1 * Cols] * m2Ptr[1 * Cols + r] +
                m1Ptr[2 * Cols] * m2Ptr[2 * Cols + r] +
                m1Ptr[3 * Cols] * m2Ptr[3 * Cols + r];
            dstPtr++;
        }
        m1Ptr += 1;
    }
    return dst;
}

BE_INLINE Mat4 Mat4::operator*(float a) const {
    return Mat4(
        mat[0].x * a, mat[0].y * a, mat[0].z * a, mat[0].z * a,
        mat[1].x * a, mat[1].y * a, mat[1].z * a, mat[1].w * a,
        mat[2].x * a, mat[2].y * a, mat[2].z * a, mat[2].w * a,
        mat[3].x * a, mat[3].y * a, mat[3].z * a, mat[3].w * a);
}

BE_INLINE Vec4 Mat4::operator*(const Vec4 &vec) const {
    return Vec4(
        mat[0].x * vec.x + mat[0].y * vec.y + mat[0].z * vec.z + mat[0].w * vec.w, 
        mat[1].x * vec.x + mat[1].y * vec.y + mat[1].z * vec.z + mat[1].w * vec.w, 
        mat[2].x * vec.x + mat[2].y * vec.y + mat[2].z * vec.z + mat[2].w * vec.w, 
        mat[3].x * vec.x + mat[3].y * vec.y + mat[3].z * vec.z + mat[3].w * vec.w);
}

BE_INLINE Vec3 Mat4::operator*(const Vec3 &vec) const {
    // homogeneous w
    float hw = mat[3].x * vec.x + mat[3].y * vec.y + mat[3].z * vec.z + mat[3].w;

    if (hw == 0.0f) {
        return Vec3(0.0f, 0.0f, 0.0f);
    }

    if (hw == 1.0f) {
        return Vec3(
            mat[0].x * vec.x + mat[0].y * vec.y + mat[0].z * vec.z + mat[0].w, 
            mat[1].x * vec.x + mat[1].y * vec.y + mat[1].z * vec.z + mat[1].w, 
            mat[2].x * vec.x + mat[2].y * vec.y + mat[2].z * vec.z + mat[2].w);
    } else {
        float rhw = 1.0f / hw;
        return Vec3(
            (mat[0].x * vec.x + mat[0].y * vec.y + mat[0].z * vec.z + mat[0].w) * rhw, 
            (mat[1].x * vec.x + mat[1].y * vec.y + mat[1].z * vec.z + mat[1].w) * rhw, 
            (mat[2].x * vec.x + mat[2].y * vec.y + mat[2].z * vec.z + mat[2].w) * rhw);
    }
}

BE_INLINE Vec4 Mat4::TransposedMulVec(const Vec4 &vec) const {
    return Vec4(
        mat[0].x * vec.x + mat[1].x * vec.y + mat[2].x * vec.z + mat[3].x * vec.w,
        mat[0].y * vec.x + mat[1].y * vec.y + mat[2].y * vec.z + mat[3].y * vec.w,
        mat[0].z * vec.x + mat[1].z * vec.y + mat[2].z * vec.z + mat[3].z * vec.w,
        mat[0].w * vec.x + mat[1].w * vec.y + mat[2].w * vec.z + mat[3].w * vec.w);
}

BE_INLINE Vec3 Mat4::TransposedMulVec(const Vec3 &vec) const {
    // homogeneous w
    float hw = mat[0].w * vec.x + mat[1].w * vec.y + mat[2].w * vec.z + mat[3].w;

    if (hw == 0.0f) {
        return Vec3(0.0f, 0.0f, 0.0f);
    }

    if (hw == 1.0f) {
        return Vec3(
            mat[0].x * vec.x + mat[1].x * vec.y + mat[2].x * vec.z + mat[3].x,
            mat[0].y * vec.x + mat[1].y * vec.y + mat[2].y * vec.z + mat[3].y,
            mat[0].z * vec.x + mat[1].z * vec.y + mat[2].z * vec.z + mat[3].z);
    } else {
        float rhw = 1.0f / hw;
        return Vec3(
            (mat[0].x * vec.x + mat[1].x * vec.y + mat[2].x * vec.z + mat[3].x) * rhw,
            (mat[0].y * vec.x + mat[1].y * vec.y + mat[2].y * vec.z + mat[3].y) * rhw,
            (mat[0].z * vec.x + mat[1].z * vec.y + mat[2].z * vec.z + mat[3].z) * rhw);
    }
}

BE_INLINE Mat4 &Mat4::operator=(const Mat4 &rhs) {
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

    mat[3][0] = rhs[3][0];
    mat[3][1] = rhs[3][1];
    mat[3][2] = rhs[3][2];
    mat[3][3] = rhs[3][3];

    return *this;
}

BE_INLINE Mat4 &Mat4::operator=(const Mat3 &rhs) {
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

    mat[3][0] = 0;
    mat[3][1] = 0;
    mat[3][2] = 0;
    mat[3][3] = 1;

    return *this;
}

BE_INLINE Mat4 &Mat4::operator+=(const Mat4 &a) {
    mat[0].x += a[0].x;
    mat[0].y += a[0].y;
    mat[0].z += a[0].z;
    mat[0].w += a[0].w;

    mat[1].x += a[1].x;
    mat[1].y += a[1].y;
    mat[1].z += a[1].z;
    mat[1].w += a[1].w;

    mat[2].x += a[2].x;
    mat[2].y += a[2].y;
    mat[2].z += a[2].z;
    mat[2].w += a[2].w;

    mat[3].x += a[3].x;
    mat[3].y += a[3].y;
    mat[3].z += a[3].z;
    mat[3].w += a[3].w;

    return *this;
}

BE_INLINE Mat4 &Mat4::operator-=(const Mat4 &a) {
    mat[0].x -= a[0].x;
    mat[0].y -= a[0].y;
    mat[0].z -= a[0].z;
    mat[0].w -= a[0].w;

    mat[1].x -= a[1].x;
    mat[1].y -= a[1].y;
    mat[1].z -= a[1].z;
    mat[1].w -= a[1].w;

    mat[2].x -= a[2].x;
    mat[2].y -= a[2].y;
    mat[2].z -= a[2].z;
    mat[2].w -= a[2].w;

    mat[3].x -= a[3].x;
    mat[3].y -= a[3].y;
    mat[3].z -= a[3].z;
    mat[3].w -= a[3].w;

    return *this;
}

BE_INLINE Mat4 &Mat4::operator*=(const Mat4 &a) {
    *this = (*this) * a;
    return *this;
}

BE_INLINE Mat4 &Mat4::operator*=(const Mat3x4 &a) {
    *this = (*this) * a;
    return *this;
}

BE_INLINE Mat4 &Mat4::operator*=(float a) {
    mat[0].x *= a;
    mat[0].y *= a;
    mat[0].z *= a;
    mat[0].w *= a;

    mat[1].x *= a;
    mat[1].y *= a;
    mat[1].z *= a;
    mat[1].w *= a;

    mat[2].x *= a;
    mat[2].y *= a;
    mat[2].z *= a;
    mat[2].w *= a;

    mat[3].x *= a;
    mat[3].y *= a;
    mat[3].z *= a;
    mat[3].w *= a;

    return *this;
}

BE_INLINE bool Mat4::Equals(const Mat4 &a) const {
    const float *ptr1 = reinterpret_cast<const float *>(mat);
    const float *ptr2 = reinterpret_cast<const float *>(a.mat);
    for (int i = 0; i < Rows * Cols; i++) {
        if (ptr1[i] != ptr2[i]) {
            return false;
        }
    }
    return true;
}

BE_INLINE bool Mat4::Equals(const Mat4 &a, const float epsilon) const {
    const float *ptr1 = reinterpret_cast<const float *>(mat);
    const float *ptr2 = reinterpret_cast<const float *>(a.mat);
    for (int i = 0; i < Rows * Cols; i++) {
        if (Math::Fabs(ptr1[i] - ptr2[i]) > epsilon) {
            return false;
        }
    }
    return true;
}

BE_INLINE void Mat4::SetZero() {
    *this = Mat4::zero;
}

BE_INLINE void Mat4::SetIdentity() {
    *this = Mat4::identity;
}

BE_INLINE void Mat4::SetTranslation(float tx, float ty, float tz) {
    mat[0][3] = tx;
    mat[1][3] = ty;
    mat[2][3] = tz;
}

BE_INLINE bool Mat4::IsIdentity(const float epsilon) const {
    return Equals(Mat4::identity, epsilon);
}

BE_INLINE bool Mat4::IsSymmetric(const float epsilon) const {
    for (int i = 1; i < 4; i++) {
        for (int j = 0; j < i; j++) {
            if (Math::Fabs(mat[i][j] - mat[j][i]) > epsilon) {
                return false;
            }
        }
    }
    return true;
}

BE_INLINE bool Mat4::IsDiagonal(const float epsilon) const {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (i != j && Math::Fabs(mat[i][j]) > epsilon) {
                return false;
            }
        }
    }
    return true;
}

BE_INLINE bool Mat4::IsSingular() const {
    return Determinant() == 0 ? true : false;
}

BE_INLINE bool Mat4::IsAffine() const {
    if (mat[3][0] != 0.0f || mat[3][1] != 0.0f || mat[3][2] != 0.0f || mat[3][3] != 1.0f) {
        return false;
    }
    return true;
}

BE_INLINE float Mat4::Trace() const {
    return mat[0][0] + mat[1][1] + mat[2][2] + mat[3][3];
};

BE_INLINE Vec3 Mat4::TransformNormal(const Vec3 &n) const {
    return Vec3(
        mat[0].x * n.x + mat[0].y * n.y + mat[0].z * n.z, 
        mat[1].x * n.x + mat[1].y * n.y + mat[1].z * n.z,
        mat[2].x * n.x + mat[2].y * n.y + mat[2].z * n.z);
}

BE_INLINE Mat4 Mat4::Inverse() const {
    Mat4 invMat = *this;
    bool r = invMat.InverseSelf();
    assert(r);
    return invMat;
}

BE_INLINE Mat4 Mat4::AffineInverse() const {
    Mat4 invMat = *this;
    bool r = invMat.AffineInverseSelf();
    assert(r);
    return invMat;
}

BE_INLINE Mat4 Mat4::EuclideanInverse() const {
    Mat4 invMat = *this;
    bool r = invMat.EuclideanInverseSelf();
    assert(r);
    return invMat;
}

BE_INLINE Mat3 Mat4::ToMat3() const {
    return Mat3(
        mat[0][0], mat[1][0], mat[2][0], 
        mat[0][1], mat[1][1], mat[2][1], 
        mat[0][2], mat[1][2], mat[2][2]);
}

BE_INLINE Mat4 Mat3::ToMat4() const {
    return Mat4(
        mat[0][0], mat[1][0], mat[2][0], 0.0f, 
        mat[0][1], mat[1][1], mat[2][1], 0.0f, 
        mat[0][2], mat[1][2], mat[2][2], 0.0f, 
        0.0f, 0.0f, 0.0f, 1.0f);
}

BE_INLINE Vec3 Mat4::ToScaleVec3() const {
    return Vec3(
        Math::Sqrt(mat[0][0] * mat[0][0] + mat[1][0] * mat[1][0] + mat[2][0] * mat[2][0]),
        Math::Sqrt(mat[0][1] * mat[0][1] + mat[1][1] * mat[1][1] + mat[2][1] * mat[2][1]),
        Math::Sqrt(mat[0][2] * mat[0][2] + mat[1][2] * mat[1][2] + mat[2][2] * mat[2][2]));
}

BE_INLINE Vec3 Mat4::ToTranslationVec3() const {
    return Vec3(mat[0][3], mat[1][3], mat[2][3]);
}

BE_INLINE Mat4 Mat4::FromTranslation(float tx, float ty, float tz) {
    Mat4 m;
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

    m.mat[3][0] = 0;
    m.mat[3][1] = 0;
    m.mat[3][2] = 0;
    m.mat[3][3] = 1;
    return m;
}

BE_INLINE Mat4 Mat4::FromScale(float sx, float sy, float sz) {
    Mat4 m;
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

    m.mat[3][0] = 0;
    m.mat[3][1] = 0;
    m.mat[3][2] = 0;
    m.mat[3][3] = 1;
    return m;
}

BE_INLINE const char *Mat4::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), Rows * Cols, precision);
}

BE_NAMESPACE_END
