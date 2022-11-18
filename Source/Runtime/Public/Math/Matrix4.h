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

                        /// Unary operator + allows this structure to be used in an expression '+m'.
    const Mat4 &        operator+() const { return *this; }

                        /// Performs an unary negation of this matrix.
    Mat4                Negate() const & { return -(*this); }
    Mat4 &&             Negate() && { return -std::move(*this); }
                        /// Performs an unary negation of this matrix.
                        /// This function is identical to the member function Negate().
    Mat4                operator-() const &;
    Mat4 &&             operator-() &&;

                        /// Adds a matrix to this matrix.
    Mat4                Add(const Mat4 &m) const & { return *this + m; }
    Mat4 &&             Add(const Mat4 &m) && { *this += m; return std::move(*this); }
                        /// Adds a matrix to this matrix.
    Mat4                operator+(const Mat4 &rhs) const &;
    Mat4 &&             operator+(const Mat4 &rhs) && { *this += rhs; return std::move(*this); }

                        /// Subtracts a matrix from this matrix.
    Mat4                Sub(const Mat4 &m) const & { return *this - m; }
    Mat4 &&             Sub(const Mat4 &m) && { *this -= m; return std::move(*this); }
                        /// Subtracts a matrix from this matrix.
    Mat4                operator-(const Mat4 &rhs) const &;
    Mat4 &&             operator-(const Mat4 &rhs) && { *this -= rhs; return std::move(*this); }

                        /// Multiplies a matrix to this matrix.
    Mat4                Mul(const Mat4 &m) const & { return *this * m; }
    Mat4 &&             Mul(const Mat4 &m) && { *this *= m; return std::move(*this); }
    Mat4                Mul(const Mat3x4 &m) const & { return *this * m; }
    Mat4 &&             Mul(const Mat3x4 &m) && { *this *= m; return std::move(*this); }
                        /// Multiplies a matrix to this matrix.
                        /// This function is identical to the member function Mul().
    Mat4                operator*(const Mat4 &rhs) const &;
    Mat4 &&             operator*(const Mat4 &rhs) && { *this *= rhs; return std::move(*this); }
    Mat4                operator*(const Mat3x4 &rhs) const &;
    Mat4 &&             operator*(const Mat3x4 &rhs) && { *this *= rhs; return std::move(*this); }

                        /// Returns this->Transpose() * m
    Mat4                TransposedMul(const Mat4 &m) const;
    Mat4                TransposedMul(const Mat3x4 &m) const;

                        /// Multiplies this matrix by a scalar.
    Mat4                MulScalar(float s) const & { return *this * s; }
    Mat4 &&             MulScalar(float s) && { *this *= s; return std::move(*this); }
                        /// Multiplies this matrix by a scalar.
                        /// This function is identical to the member function MulScalar().
    Mat4                operator*(float rhs) const &;
    Mat4 &&             operator*(float rhs) && { *this *= rhs; return std::move(*this); }
                        /// Multiplies the given matrix by a scalar.
    friend Mat4         operator*(float lhs, const Mat4 &rhs) { return rhs * lhs; }
    friend Mat4 &&      operator*(float lhs, Mat4 &&rhs) { rhs *= lhs; return std::move(rhs); }

                        /// Transforms the given vector by this matrix
    Vec4                MulVec(const Vec4 &v) const { return *this * v; }
                        /// Transforms the given vector by this matrix.
                        /// This function is identical to the member function MulVec().
    Vec4                operator*(const Vec4 &rhs) const;

                        /// Returns this->Transpose() * v
    Vec4                TransposedMulVec(const Vec4 &v) const;
                        /// Transforms the given vector by the given matrix in the order v * M (!= M * v).
    friend Vec4         operator*(const Vec4 &lhs, const Mat4 &rhs) { return rhs.TransposedMulVec(lhs); }

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
    
                        /// Multiplies this matrix with the given matrix, in-place.
    Mat4 &              MulSelf(const Mat4 &m) { *this *= m; return *this; }
    Mat4 &              MulSelf(const Mat3x4 &m) { *this *= m; return *this; }
                        /// Multiplies this matrix with the given matrix, in-place.
                        /// This function is identical to the member function MulSelf().
    Mat4 &              operator*=(const Mat4 &rhs);
    Mat4 &              operator*=(const Mat3x4 &rhs);
    
                        /// Multiplies this matrix with the given scalar, in-place.
    Mat4 &              MulScalarSelf(float s) { *this *= s; return *this; }
                        /// Multiplies this matrix with the given scalar, in-place.
                        /// This function is identical to the member function MulScalarSelf().
    Mat4 &              operator*=(float rhs);

                        /// Exact compare, no epsilon.
    bool                Equals(const Mat4 &m) const;
                        /// Compare with epsilon.
    bool                Equals(const Mat4 &m, const float epsilon) const;
                        /// Exact compare, no epsilon.
    bool                operator==(const Mat4 &rhs) const { return Equals(rhs); }
                        /// Exact compare, no epsilon.
    bool                operator!=(const Mat4 &rhs) const { return !Equals(rhs); }

                        /// Tests if this is the identity matrix, up to the given epsilon.
    bool                IsIdentity(const float epsilon = MATRIX_EPSILON) const;
                        /// Tests if this is the upper triangular matrix, up to the given epsilon.
    bool                IsUpperTriangular(const float epsilon = MATRIX_EPSILON) const;
                        /// Tests if this is the lower triangular matrix, up to the given epsilon.
    bool                IsLowerTriangular(const float epsilon = MATRIX_EPSILON) const;
                        /// Tests if this is the symmetric matrix, up to the given epsilon.
    bool                IsSymmetric(const float epsilon = MATRIX_EPSILON) const;
                        /// Tests if this is the diagonal matrix, up to the given epsilon.
    bool                IsDiagonal(const float epsilon = MATRIX_EPSILON) const;
                        /// Tests if this is the singular matrix, up to the given epsilon.
    bool                IsSingular(const float epsilon = MATRIX_EPSILON) const;
                        /// Tests if this is the affine transform matrix, up to the given epsilon.
    bool                IsAffine(const float epsilon = MATRIX_EPSILON) const;

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
                        /// Mathematically, this means sum of all eigenvalues.
    float               Trace() const;
                        /// Returns the determinant of this matrix.
                        /// Mathematically, this means multiplication of all eigenvalues.
    float               Determinant() const;
    
                        /// Transposes this matrix.
                        /// This operation swaps all elements with respect to the diagonal.
    Mat4                Transpose() const &;
    Mat4 &&             Transpose() && { return std::move(TransposeSelf()); }
                        /// Transposes this matrix, in-place.
    Mat4 &              TransposeSelf();

                        /// Inverts this matrix.
    Mat4                Inverse() const &;
    Mat4 &&             Inverse() && { InverseSelf(); return std::move(*this); }
                        /// Inverts this matrix, in-place.
                        /// @return False if determinant is zero.
    bool                InverseSelf();

                        /// Inverts a affine matrix. (affine matrix means equivalent to a 3x4 matrix)
                        /// If a matrix M is made up of only translation, rotation, reflection, scaling and shearing,
                        /// then M is affine matrix and this function can be used to compute the inverse.
    Mat4                InverseAffine() const &;
    Mat4 &&             InverseAffine() && { InverseAffineSelf(); return std::move(*this); }
                        /// Inverts a affine matrix, in-place.
    bool                InverseAffineSelf();

                        /// Inverts a orthogonal matrix.
                        /// If a matrix M is made up of only translation, rotation, and scaling,
                        /// then M is orthogonal matrix and this function can be used to compute the inverse.
    Mat4                InverseOrthogonal() const &;
    Mat4 &&             InverseOrthogonal() && { InverseOrthogonalSelf(); return std::move(*this); }
                        /// Inverts a orthogonal matrix, in-place.
    bool                InverseOrthogonalSelf();

                        /// Inverts a orthogonal uniform-scale matrix.
                        /// If a matrix M is made up of only translation, rotation, and uniform scaling,
                        /// then M is orthogonal uniform-scale matrix and this function can be used to compute the inverse.
    Mat4                InverseOrthogonalUniformScale() const &;
    Mat4 &&             InverseOrthogonalUniformScale() && { InverseOrthogonalUniformScaleSelf(); return std::move(*this); }
                        /// Inverts a orthogonal-uniform-scale matrix, in-place.
    bool                InverseOrthogonalUniformScaleSelf();
    
                        /// Inverts a orthogonal no-scale matrix.
                        /// If a matrix M is made up of only translation and rotation.
                        /// then M is orthogonal no-scale matrix and this function can be used to compute the inverse.
    Mat4                InverseOrthogonalNoScale() const &;
    Mat4 &&             InverseOrthogonalNoScale() && { InverseOrthogonalNoScaleSelf(); return std::move(*this); }
                        /// Inverts a orthogonal no-scale matrix, in-place.
    bool                InverseOrthogonalNoScaleSelf();

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

                        /// Returns translation matrix.
    static Mat4         FromTranslation(float tx, float ty, float tz);
    static Mat4         FromTranslation(const Vec3 &t) { return FromTranslation(t.x, t.y, t.z); }

                        /// Returns scaling matrix.
    static Mat4         FromScale(float sx, float sy, float sz);
    static Mat4         FromScale(const Vec3 &s) { return FromTranslation(s.x, s.y, s.z); }

    static Mat4         FromOuterProduct(const Vec4 &a, const Vec4 &b);

                        /// Transforms the given point vector by this matrix M, i.e. returns M * (x, y, z, 1).
                        /// The suffix "Pos" in this function means that the w component of the input vector is
                        /// assumed to be 1, i.e. the input vector represents a point (a position).
    Vec3                TransformPos(const Vec3 &pos) const;

                        /// Transforms the given direction vector by this matrix M, i.e. returns M * (x, y, z, 0).
                        /// The suffix "Dir" in this function just means that the w component of the input vector is
                        /// assumed to be 0, i.e. the input vector represents a direction.
                        /// The input vector does not need to be normalized.
    Vec3                TransformDir(const Vec3 &dir) const;

                        /// Performs a batch transform of the given array of point vectors.
    void                BatchTransformPos(Vec3 *posArray, int numElements) const;

                        /// Performs a batch transform of the given array of direction vectors.
    void                BatchTransformDir(Vec3 *dirArray, int numElements) const;

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
                        /// Returns rotation part in 3x3 matrix;
    Mat3                ToRotationMat3() const;

                        /// Returns "_00 _01 _02 _03 _10 _11 _12 _13 _20 _21 _22 _23 _30 _31 _32 _33".
    const char *        ToString() const { return ToString(4); }
                        /// Returns "_00 _01 _02 _03 _10 _11 _12 _13 _20 _21 _22 _23 _30 _31 _32 _33" with the given precisions.
    const char *        ToString(int precision) const;

                        /// Creates from the string.
    static Mat4         FromString(const char *str);

                        /// Returns dimension of this type.
    constexpr int       GetDimension() const { return Rows * Cols; }

    ALIGN_AS32 static const Mat4 zero;
    ALIGN_AS32 static const Mat4 identity;

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

BE_INLINE bool Mat4::IsUpperTriangular(const float epsilon) const {
    return (Math::Fabs(mat[1][0]) <= epsilon) &&
           (Math::Fabs(mat[2][0]) <= epsilon) &&
           (Math::Fabs(mat[2][1]) <= epsilon) &&
           (Math::Fabs(mat[3][0]) <= epsilon) &&
           (Math::Fabs(mat[3][1]) <= epsilon) &&
           (Math::Fabs(mat[3][2]) <= epsilon);
}

BE_INLINE bool Mat4::IsLowerTriangular(const float epsilon) const {
    return (Math::Fabs(mat[0][1]) <= epsilon) &&
           (Math::Fabs(mat[0][2]) <= epsilon) &&
           (Math::Fabs(mat[0][3]) <= epsilon) &&
           (Math::Fabs(mat[1][2]) <= epsilon) &&
           (Math::Fabs(mat[1][3]) <= epsilon) &&
           (Math::Fabs(mat[2][3]) <= epsilon);
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

BE_INLINE bool Mat4::IsSingular(const float epsilon) const {
    return Math::Fabs(Determinant()) > epsilon ? false : true;
}

BE_INLINE bool Mat4::IsAffine(const float epsilon) const {
    if (Math::Fabs(mat[3][0]) > epsilon || 
        Math::Fabs(mat[3][1]) > epsilon || 
        Math::Fabs(mat[3][2]) > epsilon || 
        Math::Fabs(mat[3][3] - 1.0f) > epsilon) {
        return false;
    }
    return true;
}

BE_INLINE float Mat4::Trace() const {
    return mat[0][0] + mat[1][1] + mat[2][2] + mat[3][3];
};

BE_INLINE float Mat4::Determinant() const {
    // 2x2 sub-determinants
    float det2_01_01 = mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];
    float det2_01_02 = mat[0][0] * mat[1][2] - mat[0][2] * mat[1][0];
    float det2_01_03 = mat[0][0] * mat[1][3] - mat[0][3] * mat[1][0];
    float det2_01_12 = mat[0][1] * mat[1][2] - mat[0][2] * mat[1][1];
    float det2_01_13 = mat[0][1] * mat[1][3] - mat[0][3] * mat[1][1];
    float det2_01_23 = mat[0][2] * mat[1][3] - mat[0][3] * mat[1][2];

    // 3x3 sub-determinants
    float det3_201_012 = mat[2][0] * det2_01_12 - mat[2][1] * det2_01_02 + mat[2][2] * det2_01_01;
    float det3_201_013 = mat[2][0] * det2_01_13 - mat[2][1] * det2_01_03 + mat[2][3] * det2_01_01;
    float det3_201_023 = mat[2][0] * det2_01_23 - mat[2][2] * det2_01_03 + mat[2][3] * det2_01_02;
    float det3_201_123 = mat[2][1] * det2_01_23 - mat[2][2] * det2_01_13 + mat[2][3] * det2_01_12;

    return (-det3_201_123 * mat[3][0] + det3_201_023 * mat[3][1] - det3_201_013 * mat[3][2] + det3_201_012 * mat[3][3]);
}

BE_INLINE Vec3 Mat4::TransformPos(const Vec3 &pos) const {
    Vec4 result = (*this) * Vec4(pos, 1.0f);
    Vec3 &ret = result.ToVec3();
    float hw = result.w;
    if (hw == 0.0f) {
        ret.Set(0.0f, 0.0f, 0.0f);
    }
    if (hw != 1.0f) {
        ret /= hw;
    }
    return ret;
}

BE_INLINE void Mat4::BatchTransformPos(Vec3 *posArray, int numElements) const {
    for (int i = 0; i < numElements; i++) {
        posArray[i] = TransformPos(posArray[i]);
    }
}

BE_INLINE Mat4 Mat4::Inverse() const & {
    Mat4 invMat = *this;
    bool r = invMat.InverseSelf();
    assert(r);
    return invMat;
}

BE_INLINE Mat4 Mat4::InverseAffine() const & {
    Mat4 invMat = *this;
    bool r = invMat.InverseAffineSelf();
    assert(r);
    return invMat;
}

BE_INLINE Mat4 Mat4::InverseOrthogonal() const & {
    Mat4 invMat = *this;
    bool r = invMat.InverseOrthogonalSelf();
    assert(r);
    return invMat;
}

BE_INLINE Mat4 Mat4::InverseOrthogonalUniformScale() const & {
    Mat4 invMat = *this;
    bool r = invMat.InverseOrthogonalUniformScaleSelf();
    assert(r);
    return invMat;
}

BE_INLINE Mat4 Mat4::InverseOrthogonalNoScale() const & {
    Mat4 invMat = *this;
    bool r = invMat.InverseOrthogonalNoScaleSelf();
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

BE_INLINE Vec3 Mat4::ToTranslationVec3() const {
    return Vec3(mat[0][3], mat[1][3], mat[2][3]);
}

BE_INLINE Mat3 Mat4::ToRotationMat3() const {
    Vec3 scale = ToScaleVec3();
    Mat3 rotation = ToMat3();
    rotation[0] *= 1.0f / scale.x;
    rotation[1] *= 1.0f / scale.y;
    rotation[2] *= 1.0f / scale.z;
    return rotation;
}

BE_INLINE Mat4 Mat4::FromTranslation(float tx, float ty, float tz) {
    return Mat4(
        1, 0, 0, tx,
        0, 1, 0, ty,
        0, 0, 1, tz,
        0, 0, 0, 1);
}

BE_INLINE Mat4 Mat4::FromScale(float sx, float sy, float sz) {
    return Mat4(
        sx, 0, 0, 0,
        0, sy, 0, 0,
        0, 0, sz, 0,
        0, 0, 0, 1);
}

BE_INLINE Mat4 Mat4::FromOuterProduct(const Vec4 &a, const Vec4 &b) {
    return Mat4(
        a[0] * b,
        a[1] * b,
        a[2] * b,
        a[3] * b);
}

BE_INLINE const char *Mat4::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), GetDimension(), precision);
}

BE_NAMESPACE_END
