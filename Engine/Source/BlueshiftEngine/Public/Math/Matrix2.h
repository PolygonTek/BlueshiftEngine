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

#include "Core/Str.h"

BE_NAMESPACE_BEGIN

/*
-------------------------------------------------------------------------------

    Mat2 - 2x2 Matrix (row major)

-------------------------------------------------------------------------------
*/

/// A 2-by-2 matrix for linear transformations of 2D geometry.
class BE_API Mat2 {
public:
    /// Specifies the height of this matrix.
    enum { Rows = 2 };

    /// Specifies the width of this matrix.
    enum { Cols = 2 };

    /// The default constructor does not initialize any members of this class.
    Mat2() {}
    /// Constructs a Mat2 by explicitly specifying the two row vectors.
    Mat2(const Vec2 &row0, const Vec2 &row1);
    /// Constructs a Mat2 by explicitly specifying all the matrix elements.
    Mat2(float _00, float _01, float _10, float _11);
    /// Constructs a Mat2 from a C array, to the value.
    explicit Mat2(const float data[2][2]);
    explicit Mat2(const float *data);

                        /// Casts this Mat2 to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const float *       Ptr() const { return (const float *)&mat[0]; }
    float *             Ptr() { return (float *)&mat[0]; }
                        /// Casts this Mat2 to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const float *() const { return (const float *)&mat[0]; }
                        operator float *() { return (float *)&mat[0]; }

                        /// Accesses an element of this matrix.
    Vec2 &              At(int index) { return (*this)[index]; }
                        /// Accesses an element of this matrix using array notation.
    const Vec2 &        operator[](int index) const;
    Vec2 &              operator[](int index);

                        /// Performs an unary negation of this matrix.
    Mat2                Negate() const { return -(*this); }
                        /// Performs an unary negation of this matrix.
                        /// This function is identical to the member function Negate().
    Mat2                operator-() const;

                        /// Unary operator + allows this structure to be used in an expression '+m'.
    Mat2                operator+() const { return *this; }

                        /// Adds a matrix to this matrix.
    Mat2                Add(const Mat2 &m) const { return *this + m; }
                        /// Adds a matrix to this matrix.
    Mat2                operator+(const Mat2 &rhs) const;

                        /// Subtracts a matrix from this matrix.
    Mat2                Sub(const Mat2 &m) const { return *this - m; }
                        /// Subtracts a matrix from this matrix.
    Mat2                operator-(const Mat2 &rhs) const;

                        /// Multiplies a matrix to this matrix.
    Mat2                Mul(const Mat2 &m) const { return *this * m; }
                        /// Returns this->Transpose() * m
    Mat2                TransposedMul(const Mat2 &m) const;
                        /// Multiplies a matrix to this matrix.
                        /// This function is identical to the member function Mul().
    Mat2                operator*(const Mat2 &rhs) const;

                        /// Multiplies this matrix by a scalar.
    Mat2                MulScalar(float s) const { return *this * s; }
                        /// Multiplies this matrix by a scalar.
                        /// This function is identical to the member function MulScalar().
    Mat2                operator*(float rhs) const;
                        /// Multiplies the given matrix by a scalar.
    friend Mat2         operator*(float s, const Mat2 &m) { return m * s; }

                        /// Transforms the given vector by this matrix
    Vec2                MulVec(const Vec2 &v) const { return *this * v; }
                        /// Returns this->Transpose() * v
    Vec2                TransposedMulVec(const Vec2 &v) const;
                        /// Transforms the given vector by this matrix.
                        /// This function is identical to the member function MulVec().
    Vec2                operator*(const Vec2 &rhs) const;
                        /// Transforms the given vector by the given matrix.
    friend Vec2         operator*(const Vec2 &lhs, const Mat2 &rhs) { return rhs * lhs; }

                        /// Assign from another matrix.
    Mat2 &              operator=(const Mat2 &rhs);

                        /// Adds a matrix to this matrix, in-place.
    Mat2 &              AddSelf(const Mat2 &m) { *this += m; return *this; }
                        /// Adds a matrix to this matrix, in-place.
                        /// This function is identical to the member function AddSelf().
    Mat2 &              operator+=(const Mat2 &rhs);

                        /// Subtracts a matrix from this matrix, in-place.
    Mat2 &              SubSelf(const Mat2 &m) { *this -= m; return *this; }
                        /// Subtracts a matrix from this matrix, in-place.
                        /// This function is identical to the member function SubSelf().
    Mat2 &              operator-=(const Mat2 &rhs);
    
                        /// Multiplies this matrix with the given matrix, in-place
    Mat2 &              MulSelf(const Mat2 &m) { *this *= m; return *this; }
                        /// Multiplies this matrix with the given matrix, in-place
                        /// This function is identical to the member function MulSelf().
    Mat2 &              operator*=(const Mat2 &rhs);
    
                        /// Multiplies this matrix with the given scalar, in-place
    Mat2 &              MulScalarSelf(float s) { *this *= s; return *this; }
                        /// Multiplies this matrix with the given scalar, in-place
                        /// This function is identical to the member function MulScalarSelf().
    Mat2 &              operator*=(float rhs);
                        
                        /// Multiplies the vector lhs with the given matrix rhs, in-place on vector. i.e. lhs *= rhs
    friend Vec2 &       operator*=(Vec2 &lhs, const Mat2 &rhs) { lhs = rhs * lhs; return lhs; }
    
                        /// Exact compare, no epsilon
    bool                Equals(const Mat2 &m) const;
                        /// Compare with epsilon
    bool                Equals(const Mat2 &m, const float epsilon) const;
                        /// Exact compare, no epsilon
    bool                operator==(const Mat2 &rhs) const { return Equals(rhs); }
                        /// Exact compare, no epsilon
    bool                operator!=(const Mat2 &rhs) const { return !Equals(rhs); }

                        /// Tests if this is the identity matrix, up to the given epsilon.
    bool                IsIdentity(const float epsilon = MATRIX_EPSILON) const;
                        /// Tests if this is the symmetric matrix, up to the given epsilon.
    bool                IsSymmetric(const float epsilon = MATRIX_EPSILON) const;
                        /// Tests if this is the diagonal matrix, up to the given epsilon.
    bool                IsDiagonal(const float epsilon = MATRIX_EPSILON) const;
                        /// Tests if this is the singular matrix.
    bool                IsSingular() const;

                        /// Sets all the element of this matrix to zero.
    void                SetZero();
                        /// Sets this matrix to equal the identity.
    void                SetIdentity();

                        /// Returns the sum of the diagonal elements of this matrix.
                        /// Mathematically, this means sum of all eigenvalues
    float               Trace() const;
                        /// Returns the determinant of this matrix
                        /// Mathematically, this means multiplication of all eigenvalues
    float               Determinant() const;

                        /// Transposes this matrix.
                        /// This operation swaps all elements with respect to the diagonal.
    Mat2                Transpose() const;
                        /// Transposes this matrix, in-place.
    Mat2 &              TransposeSelf();

                        /// Inverts this matrix.
    Mat2                Inverse() const;
                        /// Inverts this matrix, in-place.
                        /// @return False if determinant is zero.
    bool                InverseSelf();

                        /// Returns "_00 _01 _10 _11".
    const char *        ToString() const { return ToString(4); }
                        /// Returns "_00 _01 _10 _11" with the given precision.
    const char *        ToString(int precision) const;

                        /// Creates from the string
    static Mat2         FromString(const char *str);

    /// Returns dimension of this type
    int                 GetDimension() const { return Rows * Cols; }

    static const Mat2   zero;
    static const Mat2   identity;

    Vec2                mat[Rows];
};

BE_INLINE Mat2::Mat2(const Vec2 &row0, const Vec2 &row1) {
    mat[0] = row0;
    mat[1] = row1;
}

BE_INLINE Mat2::Mat2(float _00, float _01, float _10, float _11) {
    mat[0][0] = _00; 
    mat[0][1] = _01;

    mat[1][0] = _10;
    mat[1][1] = _11;
}

BE_INLINE Mat2::Mat2(const float data[2][2]) {
    mat[0][0] = data[0][0];
    mat[0][1] = data[0][1];
    mat[1][0] = data[1][0];
    mat[1][1] = data[1][1];
}

BE_INLINE Mat2::Mat2(const float *data) {
    mat[0][0] = data[0];
    mat[0][1] = data[1];
    mat[1][0] = data[2];
    mat[1][1] = data[2];
}

BE_INLINE const Vec2 &Mat2::operator[](int index) const {
    assert((index >= 0) && (index < Rows));
    return mat[index];
}

BE_INLINE Vec2 &Mat2::operator[](int index) {
    assert((index >= 0) && (index < Rows));
    return mat[index];
}

BE_INLINE Mat2 Mat2::operator-() const {
    return Mat2(
        -mat[0][0], -mat[0][1], 
        -mat[1][0], -mat[1][1]);
}

BE_INLINE Mat2 Mat2::operator+(const Mat2 &a) const {
    return Mat2(
        mat[0].x + a[0].x, mat[0].y + a[0].y, 
        mat[1].x + a[1].x, mat[1].y + a[1].y);
}
    
BE_INLINE Mat2 Mat2::operator-(const Mat2 &a) const {
    return Mat2(
        mat[0].x - a[0].x, mat[0].y - a[0].y, 
        mat[1].x - a[1].x, mat[1].y - a[1].y);
}

BE_INLINE Mat2 Mat2::operator*(const Mat2 &a) const {
    return Mat2(
        mat[0].x * a[0].x + mat[0].y * a[1].x,
        mat[0].x * a[0].y + mat[0].y * a[1].y,
        mat[1].x * a[0].x + mat[1].y * a[1].x,
        mat[1].x * a[0].y + mat[1].y * a[1].y);
}

BE_INLINE Mat2 Mat2::TransposedMul(const Mat2 &a) const {
    return Mat2(
        mat[0].x * a[0].x + mat[1].x * a[1].x,
        mat[0].x * a[0].y + mat[1].x * a[1].y,
        mat[0].y * a[0].x + mat[1].y * a[1].x,
        mat[0].y * a[0].y + mat[1].y * a[1].y);
}

BE_INLINE Mat2 Mat2::operator*(float a) const {
    return Mat2(
        mat[0].x * a, mat[0].y * a, 
        mat[1].x * a, mat[1].y * a);
}

BE_INLINE Vec2 Mat2::operator*(const Vec2 &vec) const {
    return Vec2(
        mat[0].x * vec.x + mat[0].y * vec.y, 
        mat[1].x * vec.x + mat[1].y * vec.y);
}

BE_INLINE Vec2 Mat2::TransposedMulVec(const Vec2 &vec) const {
    return Vec2(
        mat[0].x * vec.x + mat[1].x * vec.y,
        mat[0].y * vec.x + mat[1].y * vec.y);
}

BE_INLINE Mat2 &Mat2::operator=(const Mat2 &rhs) {
    mat[0][0] = rhs[0][0];
    mat[0][1] = rhs[0][1];

    mat[1][0] = rhs[1][0];
    mat[1][1] = rhs[1][1];

    return *this;
}

BE_INLINE Mat2 &Mat2::operator+=(const Mat2 &a) {
    mat[0].x += a[0].x;
    mat[0].y += a[0].y;

    mat[1].x += a[1].x;
    mat[1].y += a[1].y;

    return *this;
}

BE_INLINE Mat2 &Mat2::operator-=(const Mat2 &a) {
    mat[0].x -= a[0].x;
    mat[0].y -= a[0].y;

    mat[1].x -= a[1].x;
    mat[1].y -= a[1].y;

    return *this;
}

BE_INLINE Mat2 &Mat2::operator*=(const Mat2 &a) {
    *this = (*this) * a;
    return *this;
}

BE_INLINE Mat2 &Mat2::operator*=(float a) {
    mat[0].x *= a;
    mat[0].y *= a;

    mat[1].x *= a;
    mat[1].y *= a;

    return *this;
}

BE_INLINE bool Mat2::Equals(const Mat2 &a) const {
    if (mat[0].Equals(a[0]) && 
        mat[1].Equals(a[1])) {
        return true;
    }
    return false;
}

BE_INLINE bool Mat2::Equals(const Mat2 &a, const float epsilon) const {
    if (mat[0].Equals(a[0], epsilon) && 
        mat[1].Equals(a[1], epsilon)) {
        return true;
    }
    return false;
}

BE_INLINE void Mat2::SetZero() {
    *this = Mat2::zero;
}

BE_INLINE void Mat2::SetIdentity() {
    *this = Mat2::identity;
}

BE_INLINE bool Mat2::IsIdentity(const float epsilon) const {
    return Equals(Mat2::identity, epsilon);
}

BE_INLINE bool Mat2::IsSymmetric(const float epsilon) const {
    return (Math::Fabs(mat[0][1] - mat[1][0]) < epsilon);
}

BE_INLINE bool Mat2::IsDiagonal(const float epsilon) const {
    if (Math::Fabs(mat[0][1]) > epsilon || Math::Fabs(mat[1][0]) > epsilon)
        return false;
    return true;
}

BE_INLINE bool Mat2::IsSingular() const {
    return Determinant() == 0 ? true : false;
}

BE_INLINE float Mat2::Trace() const {
    return mat[0][0] + mat[1][1];
};

BE_INLINE float Mat2::Determinant() const {
    return mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];
}

BE_INLINE Mat2 Mat2::Transpose() const {
    return Mat2(
        mat[0][0], mat[1][0], 
        mat[0][1], mat[1][1]);
}

BE_INLINE Mat2 &Mat2::TransposeSelf() {
    float tmp = mat[0][1];
    mat[0][1] = mat[1][0];
    mat[1][0] = tmp;
    return *this;
}

BE_INLINE Mat2 Mat2::Inverse() const {
    Mat2 invMat = *this;
    int r = invMat.InverseSelf();
    assert(r);
    return invMat;
}

BE_INLINE const char *Mat2::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), Rows * Cols, precision);
}

BE_NAMESPACE_END
