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

    Mat3 - 3x3 Matrix (column major)

-------------------------------------------------------------------------------
*/

#include "Core/Str.h"

BE_NAMESPACE_BEGIN

/// A 3-by-3 matrix for linear transformations of 3D geometry.
class BE_API Mat3 {
public:
    /// Specifies the height of this matrix.
    enum { Rows = 3 };

    /// Specifies the width of this matrix.
    enum { Cols = 3 };

    /// The default constructor does not initialize any members of this class.
    Mat3() {}
    /// Constructs a Mat3 by explicitly specifying the three column vectors.
    Mat3(const Vec3 &col0, const Vec3 &col1, const Vec3 &col2);
    /// Constructs a Mat3 by explicitly specifying all the matrix elements.
    Mat3(float _00, float _01, float _02,
         float _10, float _11, float _12,
         float _20, float _21, float _22);
    /// Constructs a Mat3 from a C array, to the value.
    explicit Mat3(const float data[3][3]);
    explicit Mat3(const float *data);

                        /// Casts this Mat3 to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const float *       Ptr() const { return (const float *)&mat[0]; }
    float *             Ptr() { return (float *)&mat[0]; }
                        /// Casts this Mat3 to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const float *() const { return (const float *)&mat[0]; }
                        operator float *() { return (float *)&mat[0]; }

                        /// Accesses an element of this matrix.
    Vec3 &              At(int index) { return (*this)[index]; }
                        /// Accesses an element of this matrix using array notation.
    const Vec3 &        operator[](int index) const;
    Vec3 &              operator[](int index);

                        /// Unary operator + allows this structure to be used in an expression '+m'.
    const Mat3 &        operator+() const { return *this; }

                        /// Performs an unary negation of this matrix.
    Mat3                Negate() const & { return -(*this); }
    Mat3 &&             Negate() && { return -std::move(*this); }
                        /// Performs an unary negation of this matrix.
                        /// This function is identical to the member function Negate().
    Mat3                operator-() const &;
    Mat3 &&             operator-() &&;
        
                        /// Adds a matrix to this matrix.
    Mat3                Add(const Mat3 &m) const & { return *this + m; }
    Mat3 &&             Add(const Mat3 &m) && { *this += m; return std::move(*this); }
                        /// Adds a matrix to this matrix.
    Mat3                operator+(const Mat3 &rhs) const &;
    Mat3 &&             operator+(const Mat3 &rhs) && { *this += rhs; return std::move(*this); }

                        /// Subtracts a matrix from this matrix.
    Mat3                Sub(const Mat3 &m) const & { return *this - m; }
    Mat3 &&             Sub(const Mat3 &m) && { *this -= m; return std::move(*this); }
                        /// Subtracts a matrix from this matrix.
    Mat3                operator-(const Mat3 &rhs) const &;
    Mat3 &&             operator-(const Mat3 &rhs) && { *this -= rhs; return std::move(*this); }

                        /// Multiplies a matrix to this matrix.
    Mat3                Mul(const Mat3 &m) const & { return *this * m; }
    Mat3 &&             Mul(const Mat3 &m) && { *this *= m; return std::move(*this); }
                        /// Returns this->Transpose() * m
    Mat3                TransposedMul(const Mat3 &m) const;
                        /// Multiplies a matrix to this matrix.
                        /// This function is identical to the member function Mul().
    Mat3                operator*(const Mat3 &rhs) const &;
    Mat3 &&             operator*(const Mat3 &rhs) && { *this *= rhs; return std::move(*this); }

                        /// Multiplies this matrix by a scalar.
    Mat3                MulScalar(float s) const & { return *this * s; }
    Mat3 &&             MulScalar(float s) && { *this *= s; return std::move(*this); }
                        /// Multiplies this matrix by a scalar.
                        /// This function is identical to the member function MulScalar().
    Mat3                operator*(float rhs) const &;
    Mat3 &&             operator*(float rhs) && { *this *= rhs; return std::move(*this); }
                        /// Multiplies the given matrix by a scalar.
    friend Mat3         operator*(float lhs, const Mat3 &rhs) { return rhs * lhs; }
    friend Mat3 &&      operator*(float lhs, Mat3 &&rhs) { rhs *= lhs; return std::move(rhs); }

                        /// Transforms the given vector by this matrix.
    Vec3                MulVec(const Vec3 &v) const { return *this * v; }
                        /// Returns this->Transpose() * v
    Vec3                TransposedMulVec(const Vec3 &v) const;
                        /// Transforms the given vector by this matrix.
                        /// This function is identical to the member function MulVec().
    Vec3                operator*(const Vec3 &rhs) const;
                        /// Transforms the given vector by the given matrix in the order v * M (!= M * v).
    friend Vec3         operator*(const Vec3 &lhs, const Mat3 &rhs) { return rhs.TransposedMulVec(lhs); }

                        /// Assign from another matrix.
    Mat3 &              operator=(const Mat3 &rhs);

                        /// Adds a matrix to this matrix, in-place.
    Mat3 &              AddSelf(const Mat3 &m) { *this += m; return *this; }
                        /// Adds a matrix to this matrix, in-place.
                        /// This function is identical to the member function AddSelf().
    Mat3 &              operator+=(const Mat3 &rhs);

                        /// Subtracts a matrix from this matrix, in-place.
    Mat3 &              SubSelf(const Mat3 &m) { *this -= m; return *this; }
                        /// Subtracts a matrix from this matrix, in-place.
                        /// This function is identical to the member function SubSelf().
    Mat3 &              operator-=(const Mat3 &rhs);
    
                        /// Multiplies this matrix with the given matrix, in-place.
    Mat3 &              MulSelf(const Mat3 &m) { *this *= m; return *this; }
                        /// Multiplies this matrix with the given matrix, in-place.
                        /// This function is identical to the member function MulSelf().
    Mat3 &              operator*=(const Mat3 &rhs);
    
                        /// Multiplies this matrix with the given scalar, in-place.
    Mat3 &              MulScalarSelf(float s) { *this *= s; return *this; }
                        /// Multiplies this matrix with the given scalar, in-place.
                        /// This function is identical to the member function MulScalarSelf().
    Mat3 &              operator*=(float rhs);

                        /// Exact compare, no epsilon.
    bool                Equals(const Mat3 &m) const;
                        /// Compare with epsilon.
    bool                Equals(const Mat3 &m, const float epsilon) const;
                        /// Exact compare, no epsilon.
    bool                operator==(const Mat3 &rhs) const { return Equals(rhs); }
                        /// Exact compare, no epsilon.
    bool                operator!=(const Mat3 &rhs) const { return !Equals(rhs); }

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
                        /// Tests if this is the singular matrix.
    bool                IsSingular() const;

                        /// Sets all the element of this matrix to zero.
    void                SetZero();
                        /// Sets this matrix to equal the identity.
    void                SetIdentity();

                        /// Sets matrix by given view direction.
                        /// No need to normalize viewDir direction vector.
    bool                SetFromLookAt(const Vec3 &viewDir, const Vec3 &up = Vec3::unitZ);

                        /// Fix degenerate axial cases.
    bool                FixDegeneracies();
                        /// Change tiny numbers to zero.
    bool                FixDenormals();

                        /// Returns the sum of the diagonal elements of this matrix.
                        /// Mathematically, this means sum of all eigenvalues
    float               Trace() const;
                        /// Returns the determinant of this matrix
                        /// Mathematically, this means multiplication of all eigenvalues
    float               Determinant() const;
        
                        /// Transposes this matrix.
                        /// This operation swaps all elements with respect to the diagonal.
    Mat3                Transpose() const;
                        /// Transposes this matrix, in-place.
    Mat3 &              TransposeSelf();

                        /// Inverts this matrix.
    Mat3                Inverse() const;
                        /// Inverts this matrix, in-place.
                        /// @return False if determinant is zero.
    bool                InverseSelf();

                        /// Orthonormalizes the basis formed by the column vectors of this matrix.
    Mat3                OrthoNormalize() const;
                        /// Orthonormalizes the basis formed by the column vectors of this matrix, in-place.
    Mat3 &              OrthoNormalizeSelf();

                        /// Rotates about the given axis by the given angle in radians, in-place.
    Mat3 &              Rotate(const Vec3 &axis, float angle);

                        /// Rotates about one of the principal axes by the given angle in radians, in-place.
                        /// Calling RotateX, RotateY or RotateZ is slightly faster than calling the more generic Rotate() function.
    Mat3 &              RotateX(float ex);
    Mat3 &              RotateY(float ey);
    Mat3 &              RotateZ(float ez);

                        /// Sets rotation matrix about one of the principal axes by the given angle in radians.
    static Mat3         FromRotationX(float ex);
    static Mat3         FromRotationY(float ey);
    static Mat3         FromRotationZ(float ez);

                        /// Returns 3x3 matrix from the given sequence of Euler rotation angles in radians.
                        /// The FromRotationABC function returns a matrix M = A(ea) * B(eb) * C(ec).
                        /// Rotation C is applied first, followed by B and then A.
    static Mat3         FromRotationXYZ(float ex, float ey, float ez);
    static Mat3         FromRotationXZY(float ex, float ez, float ey);
    static Mat3         FromRotationYXZ(float ey, float ex, float ez);
    static Mat3         FromRotationYZX(float ey, float ez, float ex);
    static Mat3         FromRotationZXY(float ez, float ex, float ey);
    static Mat3         FromRotationZYX(float ez, float ey, float ex);

    static Mat3         FromRotationXYX(float ex2, float ey, float ex1);
    static Mat3         FromRotationXZX(float ex2, float ez, float ex1);
    static Mat3         FromRotationYXY(float ey2, float ex, float ey1);
    static Mat3         FromRotationYZY(float ey2, float ez, float ey1);
    static Mat3         FromRotationZXZ(float ez2, float ex, float ez1);
    static Mat3         FromRotationZYZ(float ez2, float ey, float ez1);

                        /// Decomposes the matrix and outputs the angles of rotation in parameters ex, ey and ez, in radians.
                        /// The matrix must be orthonormal.
                        /// These functions are adapted from http://www.geometrictools.com/Documentation/EulerAngles.pdf
    void                ToRotationXYZ(float &ex, float &ey, float &ez) const;
    void                ToRotationXZY(float &ex, float &ez, float &ey) const;
    void                ToRotationYXZ(float &ey, float &ex, float &ez) const;
    void                ToRotationYZX(float &ey, float &ez, float &ex) const;
    void                ToRotationZXY(float &ez, float &ex, float &ey) const;
    void                ToRotationZYX(float &ez, float &ey, float &ex) const;

    void                ToRotationXYX(float &ex2, float &ey, float &ex1) const;
    void                ToRotationXZX(float &ex2, float &ez, float &ex1) const;
    void                ToRotationYXY(float &ey2, float &ex, float &ey1) const;
    void                ToRotationYZY(float &ey2, float &ez, float &ey1) const;
    void                ToRotationZXZ(float &ez2, float &ex, float &ez1) const;
    void                ToRotationZYZ(float &ez2, float &ey, float &ez1) const;

                        /// Scales by the given factors, in-place.
    Mat3 &              Scale(const Vec3 &scale);

                        /// Returns scaling matrix.
    static Mat3         FromScale(float sx, float sy, float sz);
    static Mat3         FromScale(const Vec3 &s) { return FromScale(s.x, s.y, s.z); }

    static Mat3         FromOuterProduct(const Vec3 &a, const Vec3 &b);

                        /// Converts this 3x3 matrix to 4x4 matrix.
    Mat4                ToMat4() const;

                        /// Returns scale part.
    Vec3                ToScaleVec3() const;

                        /// Converts this 3x3 matrix to to axis-angle rotation representation.
    Rotation            ToRotation() const;

                        /// Converts this 3x3 matrix to quaternion.
    Quat                ToQuat() const;

                        /// Converts this 3x3 matrix to Euler angles.
    Angles              ToAngles() const;

                        /// Returns "_00 _01 _02 _10 _11 _12 _20 _21 _22".
    const char *        ToString() const { return ToString(4); }
                        /// Returns "_00 _01 _02 _10 _11 _12 _20 _21 _22" with the given precisions.
    const char *        ToString(int precision) const;

                        /// Creates from the string.
    static Mat3         FromString(const char *str);

                        /// Returns dimension of this type.
    constexpr int       GetDimension() const { return Rows * Cols; }

    static const Mat3   zero;
    static const Mat3   identity;

    Vec3                mat[Cols];
};

BE_INLINE Mat3::Mat3(const Vec3 &col0, const Vec3 &col1, const Vec3 &col2) {
    mat[0] = col0;
    mat[1] = col1;
    mat[2] = col2;
}

BE_INLINE Mat3::Mat3(
    float _00, float _01, float _02,
    float _10, float _11, float _12,
    float _20, float _21, float _22) {
    mat[0][0] = _00; 
    mat[0][1] = _01; 
    mat[0][2] = _02; 

    mat[1][0] = _10; 
    mat[1][1] = _11; 
    mat[1][2] = _12; 

    mat[2][0] = _20; 
    mat[2][1] = _21; 
    mat[2][2] = _22;
}

BE_INLINE Mat3::Mat3(const float data[3][3]) {
    memcpy(mat, data, sizeof(float) * Rows * Cols);
}

BE_INLINE Mat3::Mat3(const float *data) {
    memcpy(mat, data, sizeof(float) * Rows * Cols);
}

BE_INLINE const Vec3 &Mat3::operator[](int index) const {
    assert(index >= 0 && index < Cols);
    return mat[index];
}

BE_INLINE Vec3 &Mat3::operator[](int index) {
    assert(index >= 0 && index < Cols);
    return mat[index];
}

BE_INLINE Mat3 Mat3::operator-() const & {
    return Mat3(
        -mat[0][0], -mat[0][1], -mat[0][2],
        -mat[1][0], -mat[1][1], -mat[1][2],
        -mat[2][0], -mat[2][1], -mat[2][2]);
}

BE_INLINE Mat3 &&Mat3::operator-() && {
    mat[0][0] = -mat[0][0];
    mat[0][1] = -mat[0][1];
    mat[0][2] = -mat[0][2];
    mat[1][0] = -mat[1][0];
    mat[1][1] = -mat[1][1];
    mat[1][2] = -mat[1][2];
    mat[2][0] = -mat[2][0];
    mat[2][1] = -mat[2][1];
    mat[2][2] = -mat[2][2];
    return std::move(*this);
}

BE_INLINE Mat3 Mat3::operator+(const Mat3 &a) const & {
    return Mat3(
        mat[0].x + a[0].x, mat[0].y + a[0].y, mat[0].z + a[0].z,
        mat[1].x + a[1].x, mat[1].y + a[1].y, mat[1].z + a[1].z,
        mat[2].x + a[2].x, mat[2].y + a[2].y, mat[2].z + a[2].z);
}

BE_INLINE Mat3 Mat3::operator-(const Mat3 &a) const & {
    return Mat3(
        mat[0].x - a[0].x, mat[0].y - a[0].y, mat[0].z - a[0].z,
        mat[1].x - a[1].x, mat[1].y - a[1].y, mat[1].z - a[1].z,
        mat[2].x - a[2].x, mat[2].y - a[2].y, mat[2].z - a[2].z);
}

BE_INLINE Mat3 Mat3::operator*(const Mat3 &a) const & {
    Mat3 dst;
    float *dstPtr = dst.Ptr();
    const float *m1Ptr = Ptr();
    const float *m2Ptr = a.Ptr();

    for (int r = 0; r < Rows; r++) {
        for (int c = 0; c < Cols; c++) {
            *dstPtr = 
                m1Ptr[0 * Rows + c] * m2Ptr[0] +
                m1Ptr[1 * Rows + c] * m2Ptr[1] +
                m1Ptr[2 * Rows + c] * m2Ptr[2];
            dstPtr++;
        }
        m2Ptr += Rows;
    }
    return dst;
}

BE_INLINE Mat3 Mat3::TransposedMul(const Mat3 &a) const {
    Mat3 dst;
    float *dstPtr = dst.Ptr();
    const float *m1Ptr = Ptr();
    const float *m2Ptr = a.Ptr();

    for (int r = 0; r < Rows; r++) {
        for (int c = 0; c < Cols; c++) {
            *dstPtr =
                m1Ptr[c * Cols + 0] * m2Ptr[0] +
                m1Ptr[c * Cols + 1] * m2Ptr[1] +
                m1Ptr[c * Cols + 2] * m2Ptr[2];
            dstPtr++;
        }
        m2Ptr += Rows;
    }
    return dst;
}

BE_INLINE Vec3 Mat3::operator*(const Vec3 &vec) const {
    return Vec3(
        mat[0].x * vec.x + mat[1].x * vec.y + mat[2].x * vec.z,
        mat[0].y * vec.x + mat[1].y * vec.y + mat[2].y * vec.z,
        mat[0].z * vec.x + mat[1].z * vec.y + mat[2].z * vec.z);
}

BE_INLINE Vec3 Mat3::TransposedMulVec(const Vec3 &vec) const {
    return Vec3(
        mat[0].x * vec.x + mat[0].y * vec.y + mat[0].z * vec.z,
        mat[1].x * vec.x + mat[1].y * vec.y + mat[1].z * vec.z,
        mat[2].x * vec.x + mat[2].y * vec.y + mat[2].z * vec.z);
}

BE_INLINE Mat3 Mat3::operator*(float a) const & {
    return Mat3(
        mat[0].x * a, mat[0].y * a, mat[0].z * a,
        mat[1].x * a, mat[1].y * a, mat[1].z * a,
        mat[2].x * a, mat[2].y * a, mat[2].z * a);
}

BE_INLINE Mat3 &Mat3::operator=(const Mat3 &rhs) {
    mat[0][0] = rhs[0][0];
    mat[0][1] = rhs[0][1];
    mat[0][2] = rhs[0][2];

    mat[1][0] = rhs[1][0];
    mat[1][1] = rhs[1][1];
    mat[1][2] = rhs[1][2];

    mat[2][0] = rhs[2][0];
    mat[2][1] = rhs[2][1];
    mat[2][2] = rhs[2][2];

    return *this;
}

BE_INLINE Mat3 &Mat3::operator+=(const Mat3 &a) {
    mat[0].x += a[0].x; 
    mat[0].y += a[0].y; 
    mat[0].z += a[0].z;

    mat[1].x += a[1].x; 
    mat[1].y += a[1].y; 
    mat[1].z += a[1].z;

    mat[2].x += a[2].x; 
    mat[2].y += a[2].y; 
    mat[2].z += a[2].z;

    return *this;
}

BE_INLINE Mat3 &Mat3::operator-=(const Mat3 &a) {
    mat[0].x -= a[0].x; 
    mat[0].y -= a[0].y; 
    mat[0].z -= a[0].z;

    mat[1].x -= a[1].x; 
    mat[1].y -= a[1].y; 
    mat[1].z -= a[1].z;

    mat[2].x -= a[2].x; 
    mat[2].y -= a[2].y; 
    mat[2].z -= a[2].z;

    return *this;
}

BE_INLINE Mat3 &Mat3::operator*=(const Mat3 &rhs) {
#if 0
    *this = (*this) * rhs;
#else
    float dst[3];

    dst[0] = mat[0][0] * rhs.mat[0][0] + mat[1][0] * rhs.mat[0][1] + mat[2][0] * rhs.mat[0][2];
    dst[1] = mat[0][0] * rhs.mat[1][0] + mat[1][0] * rhs.mat[1][1] + mat[2][0] * rhs.mat[1][2];
    dst[2] = mat[0][0] * rhs.mat[2][0] + mat[1][0] * rhs.mat[2][1] + mat[2][0] * rhs.mat[2][2];
    mat[0][0] = dst[0];
    mat[1][0] = dst[1];
    mat[2][0] = dst[2];

    dst[0] = mat[0][1] * rhs.mat[0][0] + mat[1][1] * rhs.mat[0][1] + mat[2][1] * rhs.mat[0][2];
    dst[1] = mat[0][1] * rhs.mat[1][0] + mat[1][1] * rhs.mat[1][1] + mat[2][1] * rhs.mat[1][2];
    dst[2] = mat[0][1] * rhs.mat[2][0] + mat[1][1] * rhs.mat[2][1] + mat[2][1] * rhs.mat[2][2];
    mat[0][1] = dst[0];
    mat[1][1] = dst[1];
    mat[2][1] = dst[2];

    dst[0] = mat[0][2] * rhs.mat[0][0] + mat[1][2] * rhs.mat[0][1] + mat[2][2] * rhs.mat[0][2];
    dst[1] = mat[0][2] * rhs.mat[1][0] + mat[1][2] * rhs.mat[1][1] + mat[2][2] * rhs.mat[1][2];
    dst[2] = mat[0][2] * rhs.mat[2][0] + mat[1][2] * rhs.mat[2][1] + mat[2][2] * rhs.mat[2][2];
    mat[0][2] = dst[0];
    mat[1][2] = dst[1];
    mat[2][2] = dst[2];
#endif
    return *this;
}

BE_INLINE Mat3 &Mat3::operator*=(float rhs) {
    mat[0].x *= rhs;
    mat[0].y *= rhs;
    mat[0].z *= rhs;

    mat[1].x *= rhs;
    mat[1].y *= rhs;
    mat[1].z *= rhs;

    mat[2].x *= rhs;
    mat[2].y *= rhs;
    mat[2].z *= rhs;

    return *this;
}

BE_INLINE float Mat3::Trace() const {
    return mat[0][0] + mat[1][1] + mat[2][2];
};

BE_INLINE bool Mat3::Equals(const Mat3 &a) const {
    if (mat[0].Equals(a[0]) && 
        mat[1].Equals(a[1]) && 
        mat[2].Equals(a[2])) {
        return true;
    }
    return false;
}

BE_INLINE bool Mat3::Equals(const Mat3 &a, const float epsilon) const {
    if (mat[0].Equals(a[0], epsilon) && 
        mat[1].Equals(a[1], epsilon) && 
        mat[2].Equals(a[2], epsilon)) {
        return true;
    }
    return false;
}

BE_INLINE void Mat3::SetZero() {
    *this = Mat3::zero;
}

BE_INLINE void Mat3::SetIdentity() {
    *this = Mat3::identity;
}

BE_INLINE bool Mat3::IsIdentity(const float epsilon) const {
    return Equals(Mat3::identity, epsilon);
}

BE_INLINE bool Mat3::IsUpperTriangular(const float epsilon) const {
    return (Math::Fabs(mat[0][1]) <= epsilon) &&
           (Math::Fabs(mat[0][2]) <= epsilon) &&
           (Math::Fabs(mat[1][2]) <= epsilon);
}

BE_INLINE bool Mat3::IsLowerTriangular(const float epsilon) const {
    return (Math::Fabs(mat[1][0]) <= epsilon) &&
           (Math::Fabs(mat[2][0]) <= epsilon) &&
           (Math::Fabs(mat[2][1]) <= epsilon);
}

BE_INLINE bool Mat3::IsSymmetric(const float epsilon) const {
    if (Math::Fabs(mat[0][1] - mat[1][0]) > epsilon) {
        return false;
    }
    if (Math::Fabs(mat[0][2] - mat[2][0]) > epsilon) {
        return false;
    }
    if (Math::Fabs(mat[1][2] - mat[2][1]) > epsilon) {
        return false;
    }
    return true;
}

BE_INLINE bool Mat3::IsDiagonal(const float epsilon) const {
    if (Math::Fabs(mat[0][1]) > epsilon ||
        Math::Fabs(mat[0][2]) > epsilon ||
        Math::Fabs(mat[1][0]) > epsilon ||
        Math::Fabs(mat[1][2]) > epsilon ||
        Math::Fabs(mat[2][0]) > epsilon ||
        Math::Fabs(mat[2][1]) > epsilon) {
        return false;
    }
    return true;
}

BE_INLINE bool Mat3::IsSingular() const {
    return Determinant() == 0 ? true : false;
}

BE_INLINE bool Mat3::FixDegeneracies() {
    bool r = mat[0].FixDegenerateNormal();
    r |= mat[1].FixDegenerateNormal();
    r |= mat[2].FixDegenerateNormal();
    return r;
}

BE_INLINE bool Mat3::FixDenormals() {
    bool r = mat[0].FixDenormals();
    r |= mat[1].FixDenormals();
    r |= mat[2].FixDenormals();
    return r;
}

BE_INLINE Mat3 Mat3::Inverse() const {
    Mat3 invMat = *this;
    bool r = invMat.InverseSelf();
    assert(r);
    return invMat;
}

BE_INLINE Mat3 Mat3::OrthoNormalize() const {
    Mat3 ortho = *this;
    ortho[0].Normalize();
    ortho[2].SetFromCross(mat[0], mat[1]);
    ortho[2].Normalize();
    ortho[1].SetFromCross(mat[2], mat[0]);
    ortho[1].Normalize();
    return ortho;
}

BE_INLINE Mat3 &Mat3::OrthoNormalizeSelf() {
    mat[0].Normalize();
    mat[2].SetFromCross(mat[0], mat[1]);
    mat[2].Normalize();
    mat[1].SetFromCross(mat[2], mat[0]);
    mat[1].Normalize();
    return *this;
}

BE_INLINE Mat3 Mat3::Transpose() const {
    return Mat3(
        mat[0][0], mat[1][0], mat[2][0], 
        mat[0][1], mat[1][1], mat[2][1], 
        mat[0][2], mat[1][2], mat[2][2]);
}

BE_INLINE Mat3 &Mat3::TransposeSelf() {
    float tmp0 = mat[0][1];
    mat[0][1] = mat[1][0];
    mat[1][0] = tmp0;
    float tmp1 = mat[0][2];
    mat[0][2] = mat[2][0];
    mat[2][0] = tmp1;
    float tmp2 = mat[1][2];
    mat[1][2] = mat[2][1];
    mat[2][1] = tmp2;

    return *this;
}

BE_INLINE Mat3 Mat3::FromScale(float sx, float sy, float sz) {
    Mat3 m;
    m.mat[0][0] = sx;
    m.mat[0][1] = 0;
    m.mat[0][2] = 0;

    m.mat[1][0] = 0;
    m.mat[1][1] = sy;
    m.mat[1][2] = 0;

    m.mat[2][0] = 0;
    m.mat[2][1] = 0;
    m.mat[2][2] = sz;
    return m;
}

BE_INLINE Mat3 Mat3::FromOuterProduct(const Vec3 &a, const Vec3 &b) {
    Mat3 m;
    m.mat[0] = a * b[0];
    m.mat[1] = a * b[1];
    m.mat[2] = a * b[2];
    return m;
}

BE_INLINE Vec3 Mat3::ToScaleVec3() const {
    return Vec3(
        Math::Sqrt(mat[0][0] * mat[0][0] + mat[1][0] * mat[1][0] + mat[2][0] * mat[2][0]),
        Math::Sqrt(mat[0][1] * mat[0][1] + mat[1][1] * mat[1][1] + mat[2][1] * mat[2][1]),
        Math::Sqrt(mat[0][2] * mat[0][2] + mat[1][2] * mat[1][2] + mat[2][2] * mat[2][2]));
}

BE_INLINE const char *Mat3::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), GetDimension(), precision);
}

BE_NAMESPACE_END
